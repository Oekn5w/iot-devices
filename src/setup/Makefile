THIS_DIR := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
ESP_DEP_ROOT := $(realpath $(THIS_DIR)/../../deps)

ifndef ESP32
$(error ESP32 must be passed as variable!)
endif

ifeq ($(ESP32),1)
CHIP=esp32
ESP_ROOT := $(ESP_DEP_ROOT)/esp32
BUILD_DIR := $(THIS_DIR)/build_esp32
CUSTOM_LIBS := $(realpath $(THIS_DIR)/../libs) $(ESP_ROOT)/libraries
else
ESP_ROOT := $(ESP_DEP_ROOT)/esp8266
BUILD_DIR := $(THIS_DIR)/build_esp8266
CUSTOM_LIBS := $(realpath $(THIS_DIR)/../libs)
endif

CPP_EXTRA := -DESP32=$(ESP32)

include $(ESP_DEP_ROOT)/makeEspArduino/makeEspArduino.mk
