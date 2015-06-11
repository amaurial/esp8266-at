/*
 * File	: at_wifiCmd.c
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

#include "at.h"
#include "at_wifiCmd.h"
#include "osapi.h"
#include "c_types.h"
#include "mem.h"
#include "at_utils.h"

at_mdStateType mdState = m_unlink;

extern BOOL specialAtState;
extern at_stateType at_state;
extern at_funcationType at_fun[];

uint8_t at_wifiMode;
os_timer_t at_japDelayChack;
//struct_MSGType generalMSG;

/** @defgroup AT_WSIFICMD_Functions
  * @{
  */

/**
  * @brief  Copy param from receive data to dest.
  * @param  pDest: point to dest
  * @param  pSrc: point to source
  * @param  maxLen: copy max number of byte
  * @retval the length of param
  *   @arg -1: failure
  */
int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen)
{

  char *pTempD = pDest;
  const char *pTempS = pSrc;
  int8_t len;

  if(*pTempS != '\"')
  {
    return -1;
  }
  pTempS++;
  for(len=0; len<maxLen; len++)
  {
    if(*pTempS == '\"')
    {
      *pTempD = '\0';
      break;
    }
    else
    {
      *pTempD++ = *pTempS++;
    }
  }
  if(len == maxLen)
  {
    return -1;
  }
  return len;
}




/**
  * @brief  Query commad of set wifi mode.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwmode(uint8_t id)
{
  char temp[32];

  at_wifiMode = wifi_get_opmode();
  #ifdef VERBOSE
    os_sprintf(temp, "%s:%d\n", at_fun[id].at_cmdName, at_wifiMode);
  #else
    os_sprintf(temp, "%d%d%d%d\n", CANWII_SOH, at_fun[id].at_cmdCode, at_wifiMode,CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of set wifi mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwmode(uint8_t id, char *pPara)
{
  uint8_t mode;
  pPara++;
  mode = atoi(pPara);
  if (at_setupCmdCwmodeEsp(mode)==0){
    at_backOk;
    return;
  }
  at_backError;
}

uint8_t ICACHE_FLASH_ATTR
at_setupCmdCwmodeEsp(uint8_t mode)
{
  if(mode == at_wifiMode)
  {
    return 0;
  }
  if((mode >= 1) && (mode <= 3))
  {
    ETS_UART_INTR_DISABLE();
    wifi_set_opmode(mode);
    ETS_UART_INTR_ENABLE();
    at_wifiMode = mode;
    return 0;
  }
  return 1;
}

/**
  * @brief  Wifi ap scan over callback to display.
  * @param  arg: contain the aps information
  * @param  status: scan over status
  * @retval None
  */
static void ICACHE_FLASH_ATTR
scan_done(void *arg, STATUS status)
{
  uint8 ssid[33];
  char temp[128];

  if (status == OK)
  {
    struct bss_info *bss_link = (struct bss_info *)arg;
    bss_link = bss_link->next.stqe_next;//ignore first

    while (bss_link != NULL)
    {
      os_memset(ssid, 0, 33);
      if (os_strlen(bss_link->ssid) <= 32)
      {
        os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
      }
      else
      {
        os_memcpy(ssid, bss_link->ssid, 32);
      }
      #ifdef VERBOSE
        os_sprintf(temp,"+CWLAP:(%d,\"%s\",%d,\""MACSTR"\",%d)\n",
                 bss_link->authmode, ssid, bss_link->rssi,
                 MAC2STR(bss_link->bssid),bss_link->channel);
      #else
        os_sprintf(temp,"%d%d%d,%d,%d,%d,%d%d\n",CANWII_SOH,CMD_CWLAP,
                 bss_link->authmode, ssid, bss_link->rssi,
                 MAC2STR(bss_link->bssid),bss_link->channel,CANWII_EOH);
      #endif // VERBOSE

      uart0_sendStr(temp);
      bss_link = bss_link->next.stqe_next;
    }
    at_backOk;
  }
  else
  {

    at_backError;
  }
  specialAtState = TRUE;
  at_state = at_statIdle;
}


void ICACHE_FLASH_ATTR
at_setupCmdCwlap(uint8_t id, char *pPara)
{
  struct scan_config config;
  int8_t len;
  char ssid[32];
  char bssidT[18];
  char bssid[6];
  uint8_t channel;
  uint8_t i;

  pPara++;

  len = at_dataStrCpy(ssid, pPara, 32);

  if(len == -1)
  {
    at_backError;
    return;
  }
  if(len == 0)
  {
    config.ssid = NULL;
  }
  else
  {
    config.ssid = ssid;
  }
  pPara += (len+3);
  len = at_dataStrCpy(bssidT, pPara, 18);
  if(len == -1)
  {
    at_backError;
    return;
  }
  if(len == 0)
  {
    config.bssid = NULL;
  }
  else
  {
    if(os_str2macaddr(bssid, bssidT) == 0)
    {
      at_backError;
      return;
    }
    config.bssid = bssid;
  }

  pPara += (len+3);
  config.channel = atoi(pPara);
  if(wifi_station_scan(&config, scan_done))
  {
    specialAtState = FALSE;
  }
  else
  {
    at_backError;
  }
}

/**
  * @brief  Execution commad of list wifi aps.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCwlap(uint8_t id)//need add mode chack
{
  if(at_wifiMode == SOFTAP_MODE)
  {
    at_backError;
    return;
  }
  wifi_station_scan(NULL,scan_done);
  specialAtState = FALSE;
}

/**
  * @brief  Query commad of join to wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwjap(uint8_t id)
{
  char temp[64];
  struct station_config stationConf;
  wifi_station_get_config(&stationConf);
  struct ip_info pTempIp;

  wifi_get_ip_info(0x00, &pTempIp);
  if(pTempIp.ip.addr == 0)
  {
    generalMSG.msgid=MSG_NOAP;
    sendGeneralMsg(generalMSG);
    at_backError;
    return;
  }
  mdState = m_gotip;
  if(stationConf.ssid != 0)
  {
    #ifdef VERBOSE
        os_sprintf(temp, "%s:\"%s\"\n", at_fun[id].at_cmdName, stationConf.ssid);
    #else
        os_sprintf(temp, "%d%d%d%d\n",CANWII_SOH, at_fun[id].at_cmdCode, stationConf.ssid,CANWII_EOH);
    #endif

    uart0_sendStr(temp);
    at_backOk;
  }
  else
  {
    at_backError;
  }
}

/**
  * @brief  Transparent data through ip.
  * @param  arg: no used
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_japChack(void *arg)
{
  static uint8_t chackTime = 0;
  uint8_t japState;
  char temp[32];
  struct_MSGType generalMSG;

  os_timer_disarm(&at_japDelayChack);
  chackTime++;
  japState = wifi_station_get_connect_status();
  if(japState == STATION_GOT_IP)
  {
    chackTime = 0;
    at_backOk;
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }
  else if(chackTime >= 7)
  {
    wifi_station_disconnect();
    chackTime = 0;
    #ifdef VERBOSE
        os_sprintf(temp,"+CWJAP:%d\n",japState);
    #else
        os_sprintf(temp,"%d%d%d%d\n",CANWII_SOH,CMD_CWJAP,japState,CANWII_EOH);
    #endif // VERBOSE

    uart0_sendStr(temp);

    generalMSG.msgid=MSG_FAIL;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }
  os_timer_arm(&at_japDelayChack, 2000, 0);

}

/**
  * @brief  Setup commad of join to wifi ap.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwjap(uint8_t id, char *pPara)
{
  char temp[64];
  struct station_config stationConf;

  int8_t len;

//  stationConf /////////
  os_bzero(&stationConf, sizeof(struct station_config));
  if (at_wifiMode == SOFTAP_MODE)
  {
    at_backError;
    return;
  }
  pPara++;
  len = at_dataStrCpy(&stationConf.ssid, pPara, 32);
  if(len != -1)
  {
    pPara += (len+3);
    len = at_dataStrCpy(&stationConf.password, pPara, 64);
  }
  if(len != -1)
  {
    wifi_station_disconnect();
    mdState = m_wdact;
    ETS_UART_INTR_DISABLE();
    wifi_station_set_config(&stationConf);
    ETS_UART_INTR_ENABLE();
    wifi_station_connect();
    os_timer_disarm(&at_japDelayChack);
    os_timer_setfn(&at_japDelayChack, (os_timer_func_t *)at_japChack, NULL);
    os_timer_arm(&at_japDelayChack, 3000, 0);
    specialAtState = FALSE;
  }
  else
  {
    at_backError;
  }
}


/**
  * @brief  Execution commad of quit wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCwqap(uint8_t id)
{
  wifi_station_disconnect();
  mdState = m_wdact;
  at_backOk;
}


/**
  * @brief  Query commad of module as wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwsap(uint8_t id)
{
  struct softap_config apConfig;
  char temp[128];

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
    return;
  }
  wifi_softap_get_config(&apConfig);
  #ifdef VERBOSE
    os_sprintf(temp,"%s:\"%s\",\"%s\",%d,%d\n",
             at_fun[id].at_cmdName,
             apConfig.ssid,
             apConfig.password,
             apConfig.channel,
             apConfig.authmode);
  #else
    os_sprintf(temp,"%d%d%d,%d,%d,%d%d\n",CANWII_SOH
             at_fun[id].at_cmdCode,
             apConfig.ssid,
             apConfig.password,
             apConfig.channel,
             apConfig.authmode,CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of module as wifi ap.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwsap(uint8_t id, char *pPara)
{
  int8_t len,passLen;
  struct softap_config apConfig;

  os_bzero(&apConfig, sizeof(struct softap_config));
  wifi_softap_get_config(&apConfig);

  pPara++;
  len = at_dataStrCpy(apConfig.ssid, pPara, 32);
  apConfig.ssid_len = len;

  if(len < 1)
  {
    at_backError;
    return;
  }
  pPara += (len+3);
  passLen = at_dataStrCpy(apConfig.password, pPara, 64);
  if(passLen == -1 )
  {
    at_backError;
    return;
  }
  pPara += (passLen+3);
  apConfig.channel = atoi(pPara);

  pPara++;
  pPara = strchr(pPara, ',');
  pPara++;
  apConfig.authmode = atoi(pPara);

  if (at_setupCmdCwsapEsp(&apConfig,passLen)==0){
    at_backOk;
    return;
  }
  at_backError;

}

uint8_t ICACHE_FLASH_ATTR
at_setupCmdCwsapEsp(struct softap_config *apConfig,uint8_t passwdlen)
{
    bool ret;

    if(at_wifiMode == STATION_MODE)
    {
        return 1;
    }

    if(apConfig->ssid_len < 1 || passwdlen==-1)
    {
        return 1;
    }
    if(apConfig->channel<1 || apConfig->channel>13)
    {
        return 1;
    }
    if(apConfig->authmode >= 5)
    {
        return 1;
    }
    if((apConfig->authmode != 0)&&(passwdlen < 5))
    {
        return 1;
    }
    //ETS_UART_INTR_DISABLE();
    ret=wifi_softap_set_config(apConfig);
    //ETS_UART_INTR_ENABLE();
    return ((ret==true)?0:1);

}
void ICACHE_FLASH_ATTR
at_exeCmdCwlif(uint8_t id)
{
  struct station_info *station;
  struct station_info *next_station;
  char temp[128];

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
    return;
  }
  station = wifi_softap_get_station_info();
  while(station)
  {
    #ifdef VERBOSE
        os_sprintf(temp, "%d.%d.%d.%d,"MACSTR"\n",
               IP2STR(&station->ip), MAC2STR(station->bssid));
    #else
        os_sprintf(temp, "%d%d%d,%d%d\n",CANWII_SOH,CMD_CWLIF,
               IP2STR(&station->ip), MAC2STR(station->bssid),CANWII_EOH);
    #endif // VERBOSE


    uart0_sendStr(temp);
    next_station = STAILQ_NEXT(station, next);
    os_free(station);
    station = next_station;
  }
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCwdhcp(uint8_t id)
{
	//char temp[32];
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCwdhcp(uint8_t id, char *pPara)
{
	uint8_t mode,opt;
	pPara++;
	mode = 0;
	mode = atoi(pPara);
	pPara ++;
	pPara = strchr(pPara, ',');
	pPara++;
	opt = atoi(pPara);
    if (at_setupCmdCwdhcpEsp(mode,opt)!=0){
        at_backError;
    }
	at_backOk;
	return;
}

uint8_t ICACHE_FLASH_ATTR
at_setupCmdCwdhcpEsp(uint8_t mode, uint8_t opt)
{
    int8_t ret = 0;
    switch (mode)
	{
	case 0:
	  if(opt)
	  {
	  	ret = wifi_softap_dhcps_start();
	  }
	  else
	  {
	  	ret = wifi_softap_dhcps_stop();
	  }
		break;

	case 1:
		if(opt)
	  {
	  	ret = wifi_station_dhcpc_start();
	  }
	  else
	  {
	  	ret = wifi_station_dhcpc_stop();
	  }
		break;

	case 2:
		if(opt)
	  {
	  	ret = wifi_softap_dhcps_start();
	  	ret |= wifi_station_dhcpc_start();
	  }
	  else
	  {
	  	ret = wifi_softap_dhcps_stop();
	  	ret |= wifi_station_dhcpc_stop();
	  }
		break;

	default:

		break;
	}
	if(ret)
	{
	  return 0;
	}

	return 1;
}

void ICACHE_FLASH_ATTR
at_queryCmdCipstamac(uint8_t id)
{
	char temp[64];
  uint8 bssid[6];

  //os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
  //uart0_sendStr(temp);

  wifi_get_macaddr(STATION_IF, bssid);
  #ifdef VERBOSE
    os_sprintf(temp,"%s:\""MACSTR"\" %s\n", at_fun[id].at_cmdName, MAC2STR(bssid));
  #else
    os_sprintf(temp, "%d%d%d%d\n",CANWII_SOH,at_fun[id].at_cmdCode, MAC2STR(bssid),CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipstamac(uint8_t id, char *pPara)
{
	int8_t len,i;
  uint8 bssid[6];
  char temp[64];

	pPara++;

  len = at_dataStrCpy(temp, pPara, 32);
  if(len != 17)
  {
    at_backError;
    return;
  }

  pPara++;

  for(i=0;i<6;i++)
  {
    bssid[i] = strtol(pPara,&pPara,16);
    pPara += 1;
  }

  os_printf(MACSTR"\n", MAC2STR(bssid));
  wifi_set_macaddr(STATION_IF, bssid);
	at_backOk;
}


void ICACHE_FLASH_ATTR
at_queryCmdCipapmac(uint8_t id)
{
	char temp[64];
  uint8 bssid[6];

  //os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
  //uart0_sendStr(temp);

  wifi_get_macaddr(SOFTAP_IF, bssid);

  #ifdef VERBOSE
    os_sprintf(temp,"%s:\""MACSTR"\" %s\n", at_fun[id].at_cmdName, MAC2STR(bssid));
  #else
    os_sprintf(temp, "%d%d%d%d\n",CANWII_SOH,at_fun[id].at_cmdCode, MAC2STR(bssid),CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipapmac(uint8_t id, char *pPara)
{
  int8_t len,i;
  uint8 bssid[6];
  char temp[64];

	pPara++;

  len = at_dataStrCpy(temp, pPara, 32);
  if(len != 17)
  {
    at_backError;
    return;
  }

  pPara++;

  for(i=0;i<6;i++)
  {
    bssid[i] = strtol(pPara,&pPara,16);
    pPara += 1;
  }

  os_printf(MACSTR"\n", MAC2STR(bssid));
  wifi_set_macaddr(SOFTAP_IF, bssid);
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCipsta(uint8_t id)
{
	struct ip_info pTempIp;
  char temp[64];

  wifi_get_ip_info(0x00, &pTempIp);
  //os_sprintf(temp, "%s:", at_fun[id].at_cmdName);
  //uart0_sendStr(temp);
  #ifdef VERBOSE
    os_sprintf(temp, "%s:\"%d.%d.%d.%d\"\n",at_fun[id].at_cmdName, IP2STR(&pTempIp.ip));
  #else
    os_sprintf(temp, "%d%d%d.%d.%d.%d%d\n",CANWII_SOH,at_fun[id].at_cmdCode, IP2STR(&pTempIp.ip),CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipsta(uint8_t id, char *pPara)
{
	struct ip_info pTempIp;
  int8_t len;
  char temp[64];

  wifi_station_dhcpc_stop();

  pPara++;

  len = at_dataStrCpy(temp, pPara, 32);
  if(len == -1)
  {
    at_backError;
    return;
  }
  pPara++;
  wifi_get_ip_info(0x00, &pTempIp);
  pTempIp.ip.addr = ipaddr_addr(temp);

  os_printf("%d.%d.%d.%d\n",
                 IP2STR(&pTempIp.ip));

  if(!wifi_set_ip_info(0x00, &pTempIp))
  {
    at_backError;
    wifi_station_dhcpc_start();
    return;
  }
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCipap(uint8_t id)
{
	struct ip_info pTempIp;
  char temp[64];

  wifi_get_ip_info(0x01, &pTempIp);

  #ifdef VERBOSE
    os_sprintf(temp, "%s:\"%d.%d.%d.%d\"\n",at_fun[id].at_cmdName, IP2STR(&pTempIp.ip));
  #else
    os_sprintf(temp, "%d%d%d.%d.%d.%d%d\n",CANWII_SOH,at_fun[id].at_cmdCode, IP2STR(&pTempIp.ip),CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipap(uint8_t id, char *pPara)
{
	struct ip_info pTempIp;
  int8_t len;
  char temp[64];

  wifi_softap_dhcps_stop();

  pPara++;

  len = at_dataStrCpy(temp, pPara, 32);
  if(len == -1)
  {
    at_backError;
    return;
  }
  pPara++;
  wifi_get_ip_info(0x01, &pTempIp);
  pTempIp.ip.addr = ipaddr_addr(temp);

  os_printf("%d.%d.%d.%d\n",
                 IP2STR(&pTempIp.ip));

  if(!wifi_set_ip_info(0x01, &pTempIp))
  {
    at_backError;
    wifi_softap_dhcps_start();
    return;
  }
  wifi_softap_dhcps_start();
  at_backOk;
}



/**
  * @}
  */
