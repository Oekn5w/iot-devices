#include "Shutter.h"
#include "math.h"

Shutter::Shutter(stPins Pins, stTimings Timings, String topicBase,
    PubSubClient* client, float motorValueMax)
{
  this->Pins = Pins;
  this->Timings = Timings;
  this->topicBase = topicBase;
  this->motorValueMax = motorValueMax;
  this->motorValueMax = max(105.0f, this->motorValueMax);
  this->motorValueMax = min(200.0f, this->motorValueMax);
  this->mqttClient = client;
}

void Shutter::callback(String topic, String payload)
{
  if (this->timeoutConfidenceSubscription && topic == (this->topicBase + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED))
  {
    this->stateMovement = mvSTOPPED;
    this->percentageClosed = payload.toFloat();
    mqttClient->unsubscribe(topic.c_str());
    this->timeoutConfidenceSubscription = 0;
    this->isConfident = true;
    return;
  }
  if (topic.startsWith(this->topicBase))
  {
    topic = topic.substring(this->topicBase.length());
    if (this->mode == modeNORMAL)
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
    }
    if (topic == SHUTTER_TOPIC_MODE_COMMAND)
    {
      if (payload == SHUTTER_PAYLOAD_MODE_NORMAL)
      {
        this->setModeNormal();
      }
      else if (payload == SHUTTER_PAYLOAD_MODE_CALIBRATE)
      {
        this->setModeCalibration();
      }
      else if (payload == SHUTTER_PAYLOAD_MODE_DIRECT)
      {
        this->setModeDirect();
      }
    }
  }
}

void Shutter::setupMQTT()
{
  String tempTopic = this->topicBase + SHUTTER_TOPIC_POSITION_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topicBase + SHUTTER_TOPIC_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
  if (!this->isConfident)
  {
    tempTopic = this->topicBase + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
    mqttClient->subscribe(tempTopic.c_str());
    this->timeoutConfidenceSubscription = millis() + SHUTTER_MQTT_SUBSCRIPTION_RETAIN_TIMEOUT;
  }
  this->timeoutModeSubscription = millis() + SHUTTER_MQTT_SUBSCRIPTION_RETAIN_TIMEOUT;
  tempTopic = this->topicBase + SHUTTER_TOPIC_CONFIG;
  String payload = this->Timings.toString();
  mqttClient->publish(tempTopic.c_str(), payload.c_str(), true);
}

void Shutter::handleInput()
{
  int read_up = digitalRead(Pins.input.up);
  int read_down = digitalRead(Pins.input.down);
  int debounced;
  if(read_down ^ this->save_down)
  {
#if BUTTON_ACTIVE_LOW
    if(!read_down)
#else
  	if(read_down)
#endif
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.down);
#if BUTTON_ACTIVE_LOW
      if(!debounced)
#else
      if(debounced)
#endif
      {
        this->ButtonDownwardsPressed();
      }
    }
    else
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.down);
#if BUTTON_ACTIVE_LOW
      if(debounced)
#else
      if(!debounced)
#endif
      {
        this->ButtonDownwardsReleased();
      }
    }
    this->save_down = read_down;
  }
  if(read_up ^ this->save_up)
  {
#if BUTTON_ACTIVE_LOW
    if(!read_up)
#else
  	if(read_up)
#endif
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.up);
#if BUTTON_ACTIVE_LOW
      if(!debounced)
#else
      if(debounced)
#endif
      {
        this->ButtonUpwardsPressed();
      }
    }
    else
    {
      delay(BUTTON_DEBOUNCE_TIME);
      debounced = digitalRead(Pins.input.up);
#if BUTTON_ACTIVE_LOW
      if(debounced)
#else
      if(!debounced)
#endif
      {
        this->ButtonUpwardsReleased();
      }
    }
    this->save_up = read_up;
  }
}

void Shutter::setup()
{
  this->percentageClosed = NAN;
  this->isConfident = false;
  this->targetValueQueued = -1.0f;
  this->stateMovement = stMovementState::mvSTOPPED;
  this->mode = modeNORMAL;
  this->modeTimeout = 0;
  this->percentageClosedPublished = -1.0f;
  this->statePublished = UNDEFINED;

  this->timeActuation = 0;
  this->timePublish = 0;
  this->timeoutConfidenceSubscription = 0;
  this->timeButton = 0;

  this->timeoutModeSubscription = 0;
  this->calibrationState = 0;

  digitalWrite(Pins.actuator.down, RELAIS_LOW);
  digitalWrite(Pins.actuator.up, RELAIS_LOW);
  delay(1);
  pinMode(Pins.actuator.down, OUTPUT);
  pinMode(Pins.actuator.up, OUTPUT);
  pinMode(Pins.input.down, INPUT_PULLUP);
  pinMode(Pins.input.up, INPUT_PULLUP);
  delay(5);
  this->save_down = digitalRead(Pins.input.down);
  this->save_up = digitalRead(Pins.input.up);
}

void Shutter::loop()
{
  if (this->timeoutConfidenceSubscription && millis() > this->timeoutConfidenceSubscription)
  {
    // no data recoverable from MQTT Broker (timed out) -> confidence only gainable by opening fully
    String tempTopic = this->topicBase + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
    mqttClient->unsubscribe(tempTopic.c_str());
    this->timeoutConfidenceSubscription = 0;
  }
  if (this->timeoutModeSubscription && millis() > this->timeoutModeSubscription)
  {
    String tempTopic = this->topicBase + SHUTTER_TOPIC_MODE_COMMAND;
    mqttClient->subscribe(tempTopic.c_str());
    this->timeoutModeSubscription = 0;
  }
  this->handleInput();
  if (this->modeTimeout && millis() > this->modeTimeout)
  {
    this->setModeNormal();
    this->modeTimeout = 0;
  }
  if (this->mode == modeNORMAL)
  {
    this->actuationLoop();
  }
}

void Shutter::actuation(float targetValue)
{
  targetValue = clampPercentage(targetValue);
  if (this->stateMovement == mvSTOPPED)
  {
    if (equalWithEps(this->percentageClosed - targetValue))
    {
      return;
    }
    typeDeltaTime t0, t1, duration, extra = 0;
    stMovementState nextMove;
    if (this->percentageClosed > targetValue)
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
    t0 = this->getRelativeTime(this->percentageClosed, nextMove);
    t1 = this->getRelativeTime(targetValue, nextMove);
    duration = t1 - t0 + extra;
    this->actuationRaw(nextMove, duration);
  }
  else
  {
    this->targetValueQueued = targetValue;
  }
  
}

void Shutter::actuationRaw(stMovementState toMove, typeDeltaTime duration)
{
  if (toMove == mvSTOPPED)
  {
    this->timeActuation = millis();
    this->actuationLoop();
  }
  else
  {
    this->stateMovement = toMove;
    typeTime time = this->updateOutput();
    this->calcBase.state = toMove;
    this->calcBase.startTime = time;
    this->calcBase.t0 = 0;
    this->calcBase.endTime = time + duration;
    this->timeActuation = this->calcBase.endTime;
    this->calcBase.startPercentage = this->percentageClosed;
    this->timePublish = time;
    this->publishAll(true);
  }  
}

void Shutter::actuationLoop()
{
  typeTime time = millis();
  if (this->timeActuation)
  {
    this->percentageClosed = this->getIntermediatePercentage(time);
    if (time >= this->timeActuation)
    {
      if (this->stateMovement != mvSTOPPED)
      {
        this->stateMovement = mvSTOPPED;
        time = this->updateOutput();
        this->percentageClosed = this->getIntermediatePercentage(time);
        this->timePublish = 0;
        this->publishAll(true);
        if (this->targetValueQueued != -1.0f)
        {
          // 500ms delay before queued value is executed
          this->timeActuation = time + 500;
        }
        else
        {
          this->timeActuation = 0;
        }
      }
      else if (this->targetValueQueued != -1.0f)
      {
        this->actuation(this->targetValueQueued);
        this->targetValueQueued = -1.0f;
      }
      else
      {
        this->timeActuation = 0;
      }
    }
    else
    {
      if(this->timePublish && time >= this->timePublish)
      {
        this->publishAll(false);
        this->timePublish += SHUTTER_PUBLISH_INTERVAL;
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
  switch(this->stateMovement)
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
        return this->motorValueMax;
      }
      if (trel < this->Timings.full_opening)
      {
        return this->motorValueMax - ((trel * 100.0f) / this->Timings.full_opening);
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
        return 100.0f + ((trel * (this->motorValueMax - 100.0f)) / this->Timings.full_closing);
      }
      return this->motorValueMax;
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
      if (equalWithEps(percentage - this->motorValueMax))
      {
        return 0;
      }
      if (percentage < this->motorValueMax)
      {
        return (unsigned int)((this->Timings.full_opening * (this->motorValueMax - percentage)) / (this-> motorValueMax - 100.0f));
      }
      return 0;
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
      if (equalWithEps(percentage - this->motorValueMax))
      {
        return this->Timings.closing + this->Timings.full_closing;
      }
      if (percentage < this->motorValueMax)
      {
        return (unsigned int)(this->Timings.closing + (this->Timings.full_closing * (percentage - 100.0f) / (this->motorValueMax - 100.0f)));
      }
      return this->Timings.closing + this->Timings.full_closing;
      break;
    default:
      return 0;
  }
}

void Shutter::ButtonUpwardsPressed()
{
  switch(this->mode)
  {
    case modeNORMAL:
      if (this->stateMovement == mvSTOPPED)
      {
        this->setTarget(0.0f);
        this->timeButton = millis() + SHUTTER_TIMEOUT_BUTTON;
      }
      else
      {
        this->actuationRaw(stMovementState::mvSTOPPED, 0);
      }
      break;
    case modeCALIBRATING:
      if (this->calibrationState != 1)
      {
        this->stateMovement = mvOPENING;
        this->calibrationCache = this->updateOutput();
      }
      break;
    case modeDIRECT:
      this->stateMovement = mvOPENING;
      this->updateOutput();
      break;
  }
}

void Shutter::ButtonUpwardsReleased()
{
  switch(this->mode)
  {
    case modeNORMAL:
      if (this->timeButton && millis() > this->timeButton)
      {
        this->actuationRaw(stMovementState::mvSTOPPED, 0);
      }
      this->timeButton = 0;
      break;
    case modeCALIBRATING:
      if (this->calibrationState != 1)
      {
        this->stateMovement = mvSTOPPED;
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
      break;
    case modeDIRECT:
      this->stateMovement = mvSTOPPED;
      this->updateOutput();
      break;
  }
}

void Shutter::ButtonDownwardsPressed()
{
  switch(this->mode)
  {
    case modeNORMAL:
      if (this->stateMovement == mvSTOPPED)
      {
        this->setTarget(SHUTTER_PAYLOAD_COMMAND_CLOSE_TARGET);
        this->timeButton = millis() + SHUTTER_TIMEOUT_BUTTON;
      }
      else
      {
        this->actuationRaw(stMovementState::mvSTOPPED, 0);
      }
      break;
    case modeCALIBRATING:
      if (this->calibrationState <= 1)
      {
        this->stateMovement = mvCLOSING;
        this->calibrationCache = this->updateOutput();
      }
      break;
    case modeDIRECT:
      this->stateMovement = mvCLOSING;
      this->updateOutput();
      break;
  }
}

void Shutter::ButtonDownwardsReleased()
{
  switch(this->mode)
  {
    case modeNORMAL:
      if (this->timeButton && millis() > this->timeButton)
      {
        this->actuationRaw(stMovementState::mvSTOPPED, 0);
      }
      this->timeButton = 0;
      break;
    case modeCALIBRATING:
      if (this->calibrationState <= 1)
      {
        this->stateMovement = mvSTOPPED;
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
      break;
    case modeDIRECT:
      this->stateMovement = mvSTOPPED;
      this->updateOutput();
      break;
  }
}

void Shutter::setTarget(float targetValue)
{
  targetValue = clampPercentage(targetValue);
  if (!this->isConfident)
  {
    if (equalWithEps(targetValue))
    {
      this->targetValueQueued = -1.0f;
    }
    else
    {
      this->targetValueQueued = targetValue;
    }
    this->calibrate();
    this->isConfident = true;
  }
  else
  {
    this->actuation(targetValue);
  }
}

Shutter::stMovementState Shutter::getMovementState() const
{
  return this->stateMovement;
}

Shutter::stState Shutter::getState() const
{
  if (this->stateMovement == mvSTOPPED)
  {
    if (!this->isConfident)
    {
      return stState::UNDEFINED;
    }
    else if (this->percentageClosed < SHUTTER_PAYLOAD_STATE_THRESHOLD)
    {
      return stState::OPEN;
    }
    else
    {
      return stState::CLOSED;
    }
  }
  else if (this->stateMovement == mvOPENING)
  {
    return stState::OPENING;
  }
  else if (this->stateMovement == mvCLOSING)
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
    if (forcePublish || tempState != this->statePublished)
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
      if (this->mode == modeCALIBRATING)
      {
        payload = "calibrating";
        retain = false;
      }
      if (this->mode == modeDIRECT)
      {
        payload = "direct";
        retain = false;
      }
      String topic = this->topicBase + SHUTTER_TOPIC_STATE_PUBLISH;
      this->mqttClient->publish(topic.c_str(), payload.c_str(), retain);
      this->statePublished = tempState;
    }
    if (forcePublish || this->stateMovement != this->stateMovementPublished)
    {
      String payload = "";
      bool retain = true;
      switch (this->stateMovement)
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
      String topic = this->topicBase + SHUTTER_TOPIC_MVSTATE_PUBLISH;
      this->mqttClient->publish(topic.c_str(), payload.c_str(), retain);
      this->stateMovementPublished = this->stateMovement;
    }
  }
}

void Shutter::setModeCalibration()
{
  if (this->mode == modeCALIBRATING)
  {
    return;
  }
  this->mode = modeCALIBRATING;
  this->modeTimeout = millis() + SHUTTER_MODE_TIMEOUT;
  this->stateMovement = mvSTOPPED;
  this->updateOutput();
  this->calibrationState = 0;
  this->calibrationCache = 0;
  this->calibrationTimings = {0, 0, 0, 0};
  this->publishState(true, true);
}

void Shutter::setModeDirect()
{
  if (this->mode == modeDIRECT)
  {
    return;
  }
  this->mode = modeDIRECT;
  this->modeTimeout = millis() + SHUTTER_MODE_TIMEOUT;
  this->stateMovement = mvSTOPPED;
  this->updateOutput();
  this->publishState(true, true);
}

void Shutter::setModeNormal()
{
  if (this->mode == modeNORMAL)
  {
    return;
  }
  if (this->mode == modeCALIBRATING)
  {
    this->calibrationPublish();
  }
  this->mode = modeNORMAL;
  this->modeTimeout = 0;
  this->stateMovement = mvSTOPPED;
  this->updateOutput();
  this->publishState(true, true);
}

void Shutter::calibrationPublish(bool publishFinal)
{
  String tempTopic = this->topicBase + SHUTTER_TOPIC_CALIBRATE_PUBLISH_INTERMEDIATE;
  String payload = this->calibrationTimings.toString();
  this->mqttClient->publish(tempTopic.c_str(), payload.c_str(), false);
  if (publishFinal)
  {
    tempTopic = this->topicBase + SHUTTER_TOPIC_CALIBRATE_PUBLISH;
    this->mqttClient->publish(tempTopic.c_str(), payload.c_str(), false);
  }
}

void Shutter::publishValue(bool checkConnectivity, bool forcePublish)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    if (forcePublish || this->percentageClosedPublished < 0.0f || ! equalWithEps(this->percentageClosed - this->percentageClosedPublished))
    {
      char payload[10];
      bool retain = (this->stateMovement == mvSTOPPED);

      String topic = this->topicBase + SHUTTER_TOPIC_POSITION_PUBLISH;
      snprintf(payload, 10, "%.0f", min(this->percentageClosed, 100.0f));
      this->mqttClient->publish(topic.c_str(), payload, retain);

      snprintf(payload, 10, "%.0f", this->percentageClosed);
      topic = this->topicBase + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
      this->mqttClient->publish(topic.c_str(), payload, retain);

      this->percentageClosedPublished = this->percentageClosed;
    }
  }
}

bool Shutter::equalWithEps(float value)
{
  return (abs(value) < 0.1f);
}

float Shutter::clampPercentage(float value) const
{
  value = min(value, this->motorValueMax);
  value = max(value, 0.0f);
  return value;
}
