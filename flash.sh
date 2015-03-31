#!/bin/bash

export PATH=$HOME/apps/esp/esptool/:$PATH

export ESPSDK=$HOME/apps/esp/sdk

cd $ESPSDK/bin

PORT=$1
WHICH=$2


ATAPP=at_app_0x40000.bin
ESPSDK=esp_sdk_0x00000.bin

if [ "$WHICH" == "both" ];then
   echo "Falshing both SDK and AT"
   esptool.py --port $PORT -b 115200 write_flash 0x00000 $ATAPP 0x40000 $ESPSDK
   exit 0
fi

esptool.py --port $PORT -b 115200 write_flash 0x00000 $ATAPP 
