THIS_DIR := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
ESP_DEP_ROOT := $(realpath $(THIS_DIR)/../../deps)

ESP_ROOT := $(ESP_DEP_ROOT)/esp8266
BUILD_DIR := $(THIS_DIR)/build
CUSTOM_LIBS := $(realpath $(THIS_DIR)/../libs)

ESP32 := 0

ifdef SHUTTER_ID
SHUTTER_SINGLE_ID := $(SHUTTER_ID)
endif

ifndef SHUTTER_SINGLE_ID
SHUTTER_SINGLE_ID := 0
endif

ifneq ($(SHUTTER_SINGLE_ID), 0)
BUILD_DIR := $(THIS_DIR)/build-id$(SHUTTER_SINGLE_ID)
endif

CPP_EXTRA := -DSHUTTER_SINGLE_ID=$(SHUTTER_SINGLE_ID)

include $(THIS_DIR)/secrets.mk

include $(ESP_DEP_ROOT)/makeEspArduino/makeEspArduino.mk
