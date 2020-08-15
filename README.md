# IoT devices

Repository of a set of IoT devices that were developed for a dumb boiler, that is located in the cellar, and a new shutter control for Home Assistant.

The software can be flashed from command line, as I wanted to have the currently flashed SW stored alongside the Home Assistant configuration on a headless server.

## Description

All programs are communicating with Home Assistant via MQTT.

### Boiler

TBD

### Shutter

The shutter has two programs, as one is used to control 3 shutter simulatenously. 

That one is using a NodeMCU ESP32 (is my case from AZ Delivery).

Space is very limited for this one, so a total of 4 PCBs control the 3 shutters (One each containing the relais, one the MCU).

The secondary program is for a single shutter, with the PCB contaning the two relais and the MCU. The MCU used is therefore a D1-mini ESP8266.

## Dependencies

Dependencies are:
* ESP Arduino libraries
* Makefile for ESP Arduino projects, https://github.com/plerup/makeEspArduino

They are downloaded and prepared using the top-level `get_deps.sh` script.

For MQTT functionality, the [PubSubClient](https://github.com/knolleary/pubsubclient) is used. 
