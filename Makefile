ARDUINO_DIR   = /usr/share/arduino
ARDMK_DIR     = /usr/share/arduino

BOARD_TAG    = nano328
MONITOR_PORT = /dev/ttyUSB*
USER_LIB_PATH := $(realpath ./libs)

include $(ARDMK_DIR)/Arduino.mk
