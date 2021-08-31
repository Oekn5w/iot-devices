#include "Shutter.h"
#include "math.h"

#if BUTTON_ACTIVE_LOW
  #define BUTTON_IF_OPERATOR !
#else
  #define BUTTON_IF_OPERATOR
#endif
#define BUTTON_DEBOUNCE_TIME (10)

Shutter::Shutter(stPins Pins, stTimings Timings, String topic_base, PubSubClient* client)
{
  this->Pins = Pins;
  this->Timings = Timings;
  this->topic_base = topic_base;
  this->mqttClient = client;
}

void Shutter::callback(String topic, String payload)
{
  if (this->confidence_subscription_timeout && topic == (this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED))
  {
    this->movement_state = mvSTOPPED;
    this->percentage_closed = payload.toFloat();
    mqttClient->unsubscribe(topic.c_str());
    this->confidence_subscription_timeout = 0;
    this->is_confident = true;
    return;
  }
  if (topic.startsWith(this->topic_base))
  {
    topic = topic.substring(this->topic_base.length());
    if (!this->calibrationMode)
    {
      if (topic == SHUTTER_TOPIC_COMMAND)
      {
        if (payload == SHUTTER_PAYLOAD_COMMAND_OPEN)
        {
          this->setTarget(0.0f);
        }
        else if (payload == SHUTTER_PAYLOAD_COMMAND_CLOSE)
        {
          this->setTarget(SHUTTER_PAYLOAD_COMMAND_CLOSE_TARGET);
        }
        else if (payload == SHUTTER_PAYLOAD_COMMAND_STOP)
        {
          this->actuationRaw(stMovementState::mvSTOPPED, 0);
        }
      }
      else if (topic == SHUTTER_TOPIC_POSITION_COMMAND)
      {
        float target = payload.toFloat();
        this->setTarget(target);
      }
      else if (topic == SHUTTER_TOPIC_CALIBRATE_COMMAND && payload == SHUTTER_PAYLOAD_CALIBRATE_START)
      {
        this->calibrationInit();
      }
    }
    else
    {
      if (topic == SHUTTER_TOPIC_CALIBRATE_COMMAND && payload == SHUTTER_PAYLOAD_CALIBRATE_ABORT)
      {
        this->calibrationAbort();
      }
    }
  }
}

void Shutter::setupMQTT()
{
  String tempTopic = this->topic_base + SHUTTER_TOPIC_POSITION_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topic_base + SHUTTER_TOPIC_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
  if (!this->is_confident)
  {
    tempTopic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
    mqttClient->subscribe(tempTopic.c_str());
    this->confidence_subscription_timeout = millis() + SHUTTER_MQTT_SUBSCRIPTION_RETAIN_TIMEOUT;
  }
  this->calibration_subscription_timeout = millis() + SHUTTER_MQTT_SUBSCRIPTION_RETAIN_TIMEOUT;
  tempTopic = this->topic_base + SHUTTER_TOPIC_CONFIG;
  String payload = this->Timings.toString();
  mqttClient->publish(tempTopic.c_str(), payload.c_str(), true);
}

void Shutter::interrupt()
{
  int read_up = digitalRead(Pins.input.up);
  int read_down = digitalRead(Pins.input.down);
  int debounced;
  if(read_down ^ this->save_down)
  {
    if( BUTTON_IF_OPERATOR read_down)
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.down);
      if( BUTTON_IF_OPERATOR debounced)
      {
        this->ButtonDownwardsPressed();
      }
    }
    else
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.down);
      if(! (BUTTON_IF_OPERATOR debounced))
      {
        this->ButtonDownwardsReleased();
      }
    }
    this->save_down = read_down;
  }
  if(read_up ^ this->save_up)
  {
    if( BUTTON_IF_OPERATOR read_up)
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.up);
      if( BUTTON_IF_OPERATOR debounced)
      {
        this->ButtonUpwardsPressed();
      }
    }
    else
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.up);
      if(! (BUTTON_IF_OPERATOR debounced))
      {
        this->ButtonUpwardsReleased();
      }
    }
    this->save_up = read_up;
  }
}

void Shutter::setup(void (* fcn_interrupt)())
{
  this->percentage_closed = NAN;
  this->is_confident = false;
  this->queued_target_value = -1.0f;
  this->movement_state = stMovementState::mvSTOPPED;
  this->percentage_closed_published = -1.0f;
  this->state_published = UNDEFINED;

  this->actuation_time = 0;
  this->publish_time = 0;
  this->confidence_subscription_timeout = 0;
  this->button_time = 0;

  this->calibration_subscription_timeout = 0;
  this->calibration_timeout = 0;
  this->calibrationMode = false;
  this->calibrationState = 0;

  digitalWrite(Pins.actuator.down, RELAIS_LOW);
  digitalWrite(Pins.actuator.up, RELAIS_LOW);
  delay(1);
  pinMode(Pins.actuator.down, OUTPUT);
  pinMode(Pins.actuator.up, OUTPUT);
  pinMode(Pins.input.down, INPUT_PULLUP);
  pinMode(Pins.input.up, INPUT_PULLUP);
  delay(1);
  this->save_down = digitalRead(Pins.input.down);
  this->save_up = digitalRead(Pins.input.up);
  attachInterrupt(digitalPinToInterrupt(Pins.input.down), fcn_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pins.input.up), fcn_interrupt, CHANGE);
}

void Shutter::loop()
{
  if (this->confidence_subscription_timeout && millis() > this->confidence_subscription_timeout)
  {
    // no data recoverable from MQTT Broker (timed out) -> confidence only gainable by opening fully
    String tempTopic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
    mqttClient->unsubscribe(tempTopic.c_str());
    this->confidence_subscription_timeout = 0;
  }
  if (this->calibration_subscription_timeout && millis() > this->calibration_subscription_timeout)
  {
    String tempTopic = this->topic_base + SHUTTER_TOPIC_CALIBRATE_COMMAND;
    mqttClient->subscribe(tempTopic.c_str());
    this->calibration_subscription_timeout = 0;
  }
  if (!this->calibrationMode)
  {
    this->actuationLoop();
  }
  else
  {
    this->calibrationLoop();
  }
}

void Shutter::actuation(float targetValue)
{
  targetValue = clampPercentage(targetValue);
  if (this->movement_state == mvSTOPPED)
  {
    if (equalWithEps(this->percentage_closed - targetValue))
    {
      return;
    }
    typeDeltaTime t0, t1, duration, extra = 0;
    stMovementState nextMove;
    if (this->percentage_closed > targetValue)
    {
      // OPEN
      nextMove = mvOPENING;
      if (equalWithEps(targetValue)) { extra = SHUTTER_CALIB_OVERSHOOT; }
    }
    else
    {
      // CLOSE
      nextMove = mvCLOSING;
    }
    t0 = this->getRelativeTime(this->percentage_closed, nextMove);
    t1 = this->getRelativeTime(targetValue, nextMove);
    duration = t1 - t0 + extra;
    this->actuationRaw(nextMove, duration);
  }
  else
  {
    this->queued_target_value = targetValue;
  }
  
}

void Shutter::actuationRaw(stMovementState toMove, typeDeltaTime duration)
{
  if (toMove == mvSTOPPED)
  {
    this->actuation_time = millis();
    this->actuationLoop();
  }
  else
  {
    this->movement_state = toMove;
    typeTime time = this->updateOutput();
    this->calcBase.state = toMove;
    this->calcBase.startTime = time;
    this->calcBase.t0 = 0;
    this->calcBase.endTime = time + duration;
    this->actuation_time = this->calcBase.endTime;
    this->calcBase.startPercentage = this->percentage_closed;
    this->publish_time = time;
    this->publishAll(true);
  }  
}

void Shutter::actuationLoop()
{
  typeTime time = millis();
  if (this->actuation_time)
  {
    this->percentage_closed = this->getIntermediatePercentage(time);
    if (time >= this->actuation_time)
    {
      if (this->movement_state != mvSTOPPED)
      {
        this->movement_state = mvSTOPPED;
        time = this->updateOutput();
        this->percentage_closed = this->getIntermediatePercentage(time);
        this->publish_time = 0;
        this->publishAll(true);
        if (this->queued_target_value != -1.0f)
        {
          // 500ms delay before queued value is executed
          this->actuation_time = time + 500;
        }
        else
        {
          this->actuation_time = 0;
        }
      }
      else if (this->queued_target_value != -1.0f)
      {
        this->actuation(this->queued_target_value);
        this->queued_target_value = -1.0f;
      }
      else
      {
        this->actuation_time = 0;
      }
    }
    else
    {
      if(this->publish_time && time >= this->publish_time)
      {
        this->publishAll(false);
        this->publish_time += SHUTTER_PUBLISH_INTERVAL;
      }
    }
  }
}

void Shutter::calibrate()
{
  this->actuationRaw(stMovementState::mvOPENING, this->Timings.opening + this->Timings.full_opening + SHUTTER_CALIB_OVERSHOOT);
}

typeTime Shutter::updateOutput()
{
  switch(this->movement_state)
  {
    case mvSTOPPED:
      digitalWrite(Pins.actuator.down, RELAIS_LOW);
      digitalWrite(Pins.actuator.up, RELAIS_LOW);
      break;
    case mvCLOSING:
      digitalWrite(Pins.actuator.up, RELAIS_LOW);
      digitalWrite(Pins.actuator.down, RELAIS_HIGH);
      break;
    case mvOPENING:
      digitalWrite(Pins.actuator.down, RELAIS_LOW);
      digitalWrite(Pins.actuator.up, RELAIS_HIGH);
      break;
  }
  return millis();
}

float Shutter::getIntermediatePercentage(typeTime time)
{
  if (time > this->calcBase.endTime) { time = this->calcBase.endTime; }
  if (!this->calcBase.t0)
  {
    switch (this->calcBase.state)
    {
      case mvOPENING:
        if (!this->calcBase.t0)
        {
          this->calcBase.t0 = this->calcBase.startTime - this->getRelativeTime(this->calcBase.startPercentage, mvOPENING);
        }
        return this->getPercentage(time - this->calcBase.t0, mvOPENING);
        break;
      case mvCLOSING:
        if (!this->calcBase.t0)
        {
          this->calcBase.t0 = this->calcBase.startTime - this->getRelativeTime(this->calcBase.startPercentage, mvCLOSING);
        }
        break;
      default:
        return this->calcBase.startPercentage;
    }
  }
  return this->getPercentage(time - this->calcBase.t0, this->calcBase.state);
}

float Shutter::getPercentage(typeDeltaTime trel, stMovementState movement, float fallback) const
{
  switch (movement)
  {
    case mvOPENING:
      if (trel == 0)
      {
        return 200.0f;
      }
      if (trel < this->Timings.full_opening)
      {
        return 200.0f - ((trel * 100.0f) / this->Timings.full_opening);
      }
      if (trel == this->Timings.full_opening)
      {
        return 100.0f;
      }
      trel -= this->Timings.full_opening;
      if (trel < this->Timings.opening)
      {
        return 100.0f - ((trel * 100.0f) / this->Timings.opening);
      }
      return 0.0f;
      break;
    case mvCLOSING:
      if (trel == 0)
      {
        return 0.0f;
      }
      if (trel < this->Timings.closing)
      {
        return ((trel * 100.0f) / this->Timings.closing);
      }
      if (trel == this->Timings.closing)
      {
        return 100.0f;
      }
      trel -= this->Timings.closing;
      if (trel < this->Timings.full_closing)
      {
        return 100.0f + ((trel * 100.0f) / this->Timings.full_closing);
      }
      return 200.0f;
      break;
    default:
      return fallback;
  }
}

typeDeltaTime Shutter::getRelativeTime(float percentage, stMovementState movement) const
{
  percentage = clampPercentage(percentage);
  switch (movement)
  {
    case mvOPENING:
      if (equalWithEps(percentage - 0.0f))
      {
        return this->Timings.full_opening + this->Timings.opening;
      }
      if (equalWithEps(percentage - 100.0f))
      {
        return this->Timings.full_opening;
      }
      if (percentage < 100.0f)
      {
        return (unsigned int)(this->Timings.full_opening + ((this->Timings.opening * (100.0f - percentage)) / 100.0f));
      }
      if (equalWithEps(percentage - 200.0f))
      {
        return 0;
      }
      return (unsigned int)((this->Timings.full_opening * (200.0f - percentage)) / 100.0f);
      break;
    case mvCLOSING:
      if (equalWithEps(percentage - 0.0f))
      {
        return 0;
      }
      if (equalWithEps(percentage - 100.0f))
      {
        return this->Timings.closing;
      }
      if (percentage < 100.0f)
      {
        return (unsigned int)((this->Timings.closing * percentage) / 100.0f);
      }
      if (equalWithEps(percentage - 200.0f))
      {
        return this->Timings.closing + this->Timings.full_closing;
      }
      return (unsigned int)(this->Timings.closing + (this->Timings.full_closing * (percentage - 100.0f) / 100.0f));
      break;
    default:
      return 0;
  }
}

void Shutter::ButtonUpwardsPressed()
{
  if (!this->calibrationMode)
  {
    this->setTarget(0.0f);
    this->button_time = millis() + SHUTTER_TIMEOUT_BUTTON;
  }
  else
  {
    if (this->calibrationState != 1)
    {
      this->movement_state = mvOPENING;
      this->calibrationCache = this->updateOutput();
    }
  }
}

void Shutter::ButtonUpwardsReleased()
{
  if (!this->calibrationMode)
  {
    if (this->button_time && millis() > this->button_time)
    {
      this->actuationRaw(stMovementState::mvSTOPPED, 0);
    }
    this->button_time = 0;
  }
  else
  {
    if (this->calibrationState != 1)
    {
      this->movement_state = mvSTOPPED;
      typeTime tmptime = this->updateOutput();
      if (tmptime - this->calibrationCache < 20) { return; }
      if (this->calibrationState == 2)
      {
        this->calibrationTimings.full_opening = tmptime - this->calibrationCache;
        this->calibrationPublish();
        this->calibrationState = 3;
      }
      else if (this->calibrationState == 3)
      {
        this->calibrationTimings.opening = tmptime - this->calibrationCache;
        this->calibrationPublish(true);
        this->calibrationTimings = {0, 0, 0, 0};
        this->calibrationState = 0;
      }
    }
  }
}

void Shutter::ButtonDownwardsPressed()
{
  if (!this->calibrationMode)
  {
    this->setTarget(SHUTTER_PAYLOAD_COMMAND_CLOSE_TARGET);
    this->button_time = millis() + SHUTTER_TIMEOUT_BUTTON;
  }
  else
  {
    if (this->calibrationState <= 1)
    {
      this->movement_state = mvCLOSING;
      this->calibrationCache = this->updateOutput();
    }
  }
}

void Shutter::ButtonDownwardsReleased()
{
  if (!this->calibrationMode)
  {
    if (this->button_time && millis() > this->button_time)
    {
      this->actuationRaw(stMovementState::mvSTOPPED, 0);
    }
    this->button_time = 0;
  }
  else
  {
    if (this->calibrationState <= 1)
    {
      this->movement_state = mvSTOPPED;
      typeTime tmptime = this->updateOutput();
      if (tmptime - this->calibrationCache < 20) { return; }
      if (this->calibrationState == 0)
      {
        this->calibrationTimings.closing = tmptime - this->calibrationCache;
        this->calibrationPublish();
        this->calibrationState = 1;
      }
      else if (this->calibrationState == 1)
      {
        this->calibrationTimings.full_closing = tmptime - this->calibrationCache;
        this->calibrationPublish();
        this->calibrationState = 2;
      }
    }
  }
}

void Shutter::setTarget(float targetValue)
{
  targetValue = clampPercentage(targetValue);
  if (!this->is_confident)
  {
    if (equalWithEps(targetValue))
    {
      this->queued_target_value = -1.0f;
    }
    else
    {
      this->queued_target_value = targetValue;
    }
    this->calibrate();
    this->is_confident = true;
  }
  else
  {
    this->actuation(targetValue);
  }
}

Shutter::stMovementState Shutter::getMovementState() const
{
  return this->movement_state;
}

Shutter::stState Shutter::getState() const
{
  if (this->movement_state == mvSTOPPED)
  {
    if (!this->is_confident)
    {
      return stState::UNDEFINED;
    }
    else if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD)
    {
      return stState::OPEN;
    }
    else
    {
      return stState::CLOSED;
    }
  }
  else if (this->movement_state == mvOPENING)
  {
    return stState::OPENING;
  }
  else if (this->movement_state == mvCLOSING)
  {
    return stState::CLOSING;
  }
  return stState::UNDEFINED;
}

void Shutter::publishAll(bool forcePublish)
{
  if (this->mqttClient->connected())
  {
    this->publishState(false, forcePublish);
    this->publishValue(false, forcePublish);
  }
}

void Shutter::publishState(bool checkConnectivity, bool forcePublish)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    stState tempState = this->getState();
    if (forcePublish || tempState != this->state_published)
    {
      String payload = "";
      bool retain = true;
      switch (tempState)
      {
        case OPEN:
          payload = SHUTTER_PAYLOAD_STATE_OPEN;
          break;
        case CLOSED:
          payload = SHUTTER_PAYLOAD_STATE_CLOSED;
          break;
        case OPENING:
          payload = SHUTTER_PAYLOAD_STATE_OPENING;
          retain = false;
          break;
        case CLOSING:
          payload = SHUTTER_PAYLOAD_STATE_CLOSING;
          retain = false;
          break;
        default:
          payload = "undefined";
          break;
      }
      if (this->calibrationMode)
      {
        payload = "calibrating";
        retain = false;
      }
      String topic = this->topic_base + SHUTTER_TOPIC_STATE_PUBLISH;
      this->mqttClient->publish(topic.c_str(), payload.c_str(), retain);
      this->state_published = tempState;
    }
    if (forcePublish || this->movement_state != this->movement_state_published)
    {
      String payload = "";
      bool retain = true;
      switch (this->movement_state)
      {
        case mvSTOPPED:
          payload = SHUTTER_PAYLOAD_STATE_STOPPED;
          break;
        case mvCLOSING:
          payload = SHUTTER_PAYLOAD_STATE_CLOSING;
          retain = false;
          break;
        case mvOPENING:
          payload = SHUTTER_PAYLOAD_STATE_OPENING;
          retain = false;
          break;
      }
      String topic = this->topic_base + SHUTTER_TOPIC_MVSTATE_PUBLISH;
      this->mqttClient->publish(topic.c_str(), payload.c_str(), retain);
      this->movement_state_published = this->movement_state;
    }
  }
}

void Shutter::calibrationLoop()
{
  typeTime time = millis();
  if (this->calibration_timeout && time > this->calibration_timeout)
  {
    this->calibrationAbort();
  }
}

void Shutter::calibrationInit()
{
  this->calibrationMode = true;
  this->calibration_timeout = millis() + SHUTTER_CALIBRATION_TIMEOUT;
  this->movement_state = mvSTOPPED;
  this->updateOutput();
  this->calibrationState = 0;
  this->calibrationCache = 0;
  this->calibrationTimings = {0, 0, 0, 0};
  this->publishState(true, true);
}

void Shutter::calibrationAbort()
{
  this->calibrationMode = false;
  this->calibration_timeout = 0;
  this->movement_state = mvSTOPPED;
  this->updateOutput();
  this->calibrationPublish();
  this->publishState(true, true);
}

void Shutter::calibrationPublish(bool publishFinal)
{
  String tempTopic = this->topic_base + SHUTTER_TOPIC_CALIBRATE_PUBLISH_INTERMEDIATE;
  String payload = this->calibrationTimings.toString();
  this->mqttClient->publish(tempTopic.c_str(), payload.c_str(), false);
  if (publishFinal)
  {
    tempTopic = this->topic_base + SHUTTER_TOPIC_CALIBRATE_PUBLISH;
    this->mqttClient->publish(tempTopic.c_str(), payload.c_str(), false);
  }
}

void Shutter::publishValue(bool checkConnectivity, bool forcePublish)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    if (forcePublish || this->percentage_closed_published < 0.0f || ! equalWithEps(this->percentage_closed - this->percentage_closed_published))
    {
      char payload[10];
      bool retain = (this->movement_state == mvSTOPPED);

      String topic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH;
      snprintf(payload, 10, "%.0f", min(this->percentage_closed, 100.0f));
      this->mqttClient->publish(topic.c_str(), payload, retain);

      snprintf(payload, 10, "%.0f", this->percentage_closed);
      topic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
      this->mqttClient->publish(topic.c_str(), payload, retain);

      this->percentage_closed_published = this->percentage_closed;
    }
  }
}

bool Shutter::equalWithEps(float value)
{
  return (abs(value) < 0.1f);
}

float Shutter::clampPercentage(float value)
{
  value = min(value, 200.0f);
  value = max(value, 0.0f);
  return value;
}
