THIS_DIR := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
ESP_DEP_ROOT := $(realpath $(THIS_DIR)/../../deps)

CHIP=esp32
ESP_ROOT := $(ESP_DEP_ROOT)/esp32
BUILD_DIR := $(THIS_DIR)/build
CUSTOM_LIBS := $(realpath $(THIS_DIR)/../libs) $(ESP_ROOT)/libraries

include $(THIS_DIR)/secrets.mk

include $(ESP_DEP_ROOT)/makeEspArduino/makeEspArduino.mk
