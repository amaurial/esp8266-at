/*
 * File	: user_main.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "at.h"
#include "at_merg.h"

extern uint8_t at_wifiMode;
extern void user_esp_platform_load_param(void *param, uint16 len);

void user_init(void)
{
  esp_StoreType tempUart;

  uart_init(BIT_RATE_115200, BIT_RATE_9600);
  uart0_sendStr("INIT\n");
  user_esp_platform_load_param((uint32 *)&tempUart, sizeof(esp_StoreType));
  at_wifiMode = wifi_get_opmode();

  #ifdef DEBUG
    char temp[255];
    os_sprintf(temp, "merg command: state-%d saved-%d ssid-%s passwd-%s cmdid-%d cmdsubid-%d ssidlen-%d passwdlen-%d cwmode-%d cwmux-%d port-%d wpa-%d channel-%d dhcpmode-%d dhcpen-%d servermode-%d timeout-%d\n",
        tempUart.state,
        tempUart.saved,
        tempUart.ssid,
        tempUart.passwd,
        tempUart.cmdid,
        tempUart.cmdsubid,
        tempUart.ssidlen,
        tempUart.passwdlen,
        tempUart.cwmode,
        tempUart.cwmux,
        tempUart.port,
        tempUart.wpa,
        tempUart.channel,
        tempUart.dhcp_mode,
        tempUart.dhcp_enable,
        tempUart.server_mode,
        tempUart.timeout);
    uart0_sendStr(temp);
    #endif // DEBUG
  //create the server
  if (tempUart.state==1){
    uart0_sendStr("STARTING SAVED STATE\n");
    //at_setupCmdCwmodeEsp(tempUart.cwmode);
   // at_setupCmdCwdhcpEsp(tempUart.dhcp_mode,tempUart.dhcp_enable);

    setupServer(&tempUart);
    tempUart.state==1;
    tempUart.saved==0;
    //user_esp_platform_save_param((uint32 *)&tempUart, sizeof(esp_StoreType));

  }

  //TODO Change message
  os_printf("ready!!!\n");
  uart0_sendStr("ready\n");
  at_init();
}
