THIS_DIR := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
ESP_DEP_ROOT := $(realpath $(THIS_DIR)/../../deps)

ESP_ROOT := $(ESP_DEP_ROOT)/esp8266
BUILD_DIR := $(THIS_DIR)/build
CUSTOM_LIBS := $(realpath $(THIS_DIR)/../libs)

ESP32 := 0

include $(ESP_DEP_ROOT)/makeEspArduino/makeEspArduino.mk
