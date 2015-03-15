#!/bin/sh
export ESPSDK=$HOME/apps/esp-open-sdk/esp8266-wiki/sdk/sdk
export ESPAT=$ESPSDK/esp8266_at
export PATH=/opt/esp/esptool/:$PATH

echo "ESPSDK:$ESPSDK"
echo "ESP AT:$ESPAT"

cd $ESPAT
cd .output/eagle/debug/image
esptool -eo eagle.app.v6.out -bo eagle.app.v6.flash.bin -bs .text -bs .data -bs .rodata -bc -ec
xtensa-lx106-elf-objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin
cp eagle.app.v6.flash.bin $ESPSDK/bin/at_app_0x40000.bin
cp eagle.app.v6.irom0text.bin $ESPSDK/bin/esp_sdk_0x00000.bin
