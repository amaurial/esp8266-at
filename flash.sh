#!/bin/sh

export PATH=/opt/esp/esptool/:$PATH

export ESPSDK=$HOME/apps/esp-open-sdk/esp8266-wiki/sdk/sdk

cd $ESPSDK/bin

PORT=$1

ATAPP=at_app_0x40000.bin
ESPSDK=esp_sdk_0x00000.bin
esptool.py --port $PORT -b 115200 write_flash 0x00000 $ATAPP 0x40000 $ESPSDK
