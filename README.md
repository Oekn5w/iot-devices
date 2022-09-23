# IoT devices

Repository of a set of IoT devices that were developed for a dumb boiler that is located in the cellar, and a new shutter control for Home Assistant.

The software can be flashed from command line, as the currently flashed SW should be stored alongside the Home Assistant configuration on a headless server as a `git submodule`.

## Description

All programs are communicating with HA via MQTT.

Each controller has their own PCB (their schematics and layouts are currently not stored in this repository)

The controllers used are:

* ESP32 (NodeMCU by AZ Delivery)
* ESP8266 (D1-Mini by AZ Delivery)

### Boiler

The boiler program is designed to report the boiler temperature to HA and act as an actuator for HA to request the central heating to heat the boiler.

### Boiler-Solar

Boiler-Solar is designed for the ESP controlling an immersion heater for the boiler via by continously setting the surplus power by solar. The Boiler program functionality will later be incorporated in this program once the PCB has been designed.

### Shutter

The shutter has two programs, as one is used to control 3 shutter simulatenously. 

That one is using a ESP32.

Physical space is very limited for this one, so a total of 4 PCBs control the 3 shutters (one each containing the two relais, one the MCU).

The secondary program is for a single shutter, with the PCB containing the two relais and the MCU.

### Setup

A minimal program for either ESP8266 or ESP32 that sets up the WiFi and OTA capabilities.

The IP and MAC Addresses are printed to the MQTT Broker every 5 seconds (no retain flag is set) the ESP device is powered up.

To be flashed via USB by running

```
# make ESP32=0 flash # for ESP8266
# make ESP32=1 flash # for ESP32
```

## Dependencies

Dependencies are:
* ESP Arduino libraries
* Makefile for ESP Arduino projects, https://github.com/plerup/makeEspArduino

They are downloaded and prepared using the top-level `get_deps.sh` script.

For MQTT functionality, the [PubSubClient](https://github.com/knolleary/pubsubclient) is used. 
