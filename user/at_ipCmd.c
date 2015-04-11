/*
 * File	: at_ipCmd.c
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
#include "c_types.h"
#include "user_interface.h"
#include "at_version.h"
#include "espconn.h"
#include "mem.h"
#include "at.h"
#include "at_ipCmd.h"
#include "osapi.h"
#include "driver/uart.h"
#include <stdlib.h>
#include "upgrade.h"

extern at_mdStateType mdState;
extern BOOL specialAtState;
extern at_stateType at_state;
extern at_funcationType at_fun[];
extern uint8_t *pDataLine;
extern uint8_t at_dataLine[];
extern uint8_t at_wifiMode;
extern int8_t at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen);

uint16_t at_sendLen; //now is 256
uint16_t at_tranLen; //now is 256
os_timer_t at_delayCheck;
BOOL IPMODE;
uint8_t ipDataSendFlag = 0;
struct_MSGType generalMSG;


static BOOL at_ipMux = FALSE;
static BOOL disAllFlag = FALSE;

static at_linkConType pLink[at_linkMax];
static uint8_t sendingID;
static BOOL serverEn = FALSE;
static at_linkNum = 0;


static uint16_t server_timeover = 180;
static struct espconn *pTcpServer;
static struct espconn *pUdpServer;

/** @defgroup AT_IPCMD_Functions
  * @{
  */

static void at_tcpclient_discon_cb(void *arg);

/**
  * @brief  Test commad of at_testQueryCmdCwsap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdGeneric(uint8_t id)
{
    char temp[32];
    #ifdef VERBOSE
        os_sprintf(temp, "%s:(1-3)\n", at_fun[id].at_cmdName);
    #else
        os_sprintf(temp, "%d%d%d",CANWII_SOH, at_fun[id].at_cmdCode,CANWII_EOH);
    #endif // VERBOSE
    uart0_sendStr(temp);
    at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCifsr(uint8_t id, char *pPara)
{
  struct ip_info pTempIp;
  int8_t len;
  char ipTemp[64];
//  char temp[64];

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
    return;
  }
  pPara = strchr(pPara, '\"');
  len = at_dataStrCpy(ipTemp, pPara, 32);
  if(len == -1)
  {

    #ifdef VERBOSE
        uart0_sendStr("IP ERROR\n");
    #else
        os_sprintf(temp, "%d%d%d",CANWII_SOH, RSP_IP_ERROR,CANWII_EOH);
    #endif // VERBOSE


    return;
  }

  wifi_get_ip_info(0x01, &pTempIp);
  pTempIp.ip.addr = ipaddr_addr(ipTemp);

  os_printf("%d.%d.%d.%d\n",
                 IP2STR(&pTempIp.ip));

  if(!wifi_set_ip_info(0x01, &pTempIp))
  {
    at_backError;
    return;
  }
  at_backOk;
  return;
}

/**
  * @brief  Execution commad of get module ip.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCifsr(uint8_t id)//add get station ip and ap ip
{
  struct ip_info pTempIp;
  char temp[64];
  uint8 bssid[6];

  if((at_wifiMode == SOFTAP_MODE)||(at_wifiMode == STATIONAP_MODE))
  {
    wifi_get_ip_info(0x01, &pTempIp);
    wifi_get_macaddr(SOFTAP_IF, bssid);
     #ifdef VERBOSE
        os_sprintf(temp, "%s:APIP,", at_fun[id].at_cmdName);
        uart0_sendStr(temp);

        os_sprintf(temp, "\"%d.%d.%d.%d\"\n",
                   IP2STR(&pTempIp.ip));
        uart0_sendStr(temp);

        os_sprintf(temp, "%s:APMAC,", at_fun[id].at_cmdName);
        uart0_sendStr(temp);

        os_sprintf(temp, "\""MACSTR"\"\n",
                   MAC2STR(bssid));
        uart0_sendStr(temp);
    #else
        //<SOH><CMD><P1><IP><P2><MAC><EOH>

        os_sprintf(temp, "%d%d%d%d.%d.%d.%s%d",CANWII_SOH, at_fun[id].at_cmdCode,
                    1,IP2STR(&pTempIp.ip),2,MAC2STR(bssid),CANWII_EOH);
        uart0_sendStr(temp);
    #endif // VERBOSE


//    mdState = m_gotip; /////////
  }
  if((at_wifiMode == STATION_MODE)||(at_wifiMode == STATIONAP_MODE))
  {
    wifi_get_ip_info(0x00, &pTempIp);
    wifi_get_macaddr(STATION_IF, bssid);

    #ifdef VERBOSE
        os_sprintf(temp, "%s:STAIP,", at_fun[id].at_cmdName);
        uart0_sendStr(temp);

        os_sprintf(temp, "\"%d.%d.%d.%d\"\n",
                   IP2STR(&pTempIp.ip));
        uart0_sendStr(temp);

        os_sprintf(temp, "%s:STAMAC,", at_fun[id].at_cmdName);
        uart0_sendStr(temp);


        os_sprintf(temp, "\""MACSTR"\"\n",
                   MAC2STR(bssid));
        uart0_sendStr(temp);
    #else
        os_sprintf(temp, "%d%d%d%d.%d.%d.%s%d",CANWII_SOH, at_fun[id].at_cmdCode,
                    1,IP2STR(&pTempIp.ip),2,MAC2STR(bssid),CANWII_EOH);
        uart0_sendStr(temp);
    #endif // VERBOSE

  }
  mdState = m_gotip;
  at_backOk;
}

/**
  * @brief  Execution commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipstatus(uint8_t id)
{
  char temp[64];
  uint8_t i;


    #ifdef VERBOSE
        os_sprintf(temp, "STATUS:%d\n",
         mdState);
    #else
        os_sprintf(temp, "%d%d",CANWII_SOH, CMD_CIPSTATUS);
    #endif // VERBOSE
  uart0_sendStr(temp);
  if(serverEn)
  {

  }
  for(i=0; i<at_linkMax; i++)
  {
    if(pLink[i].linkEn)
    {
      if(pLink[i].pCon->type == ESPCONN_TCP)
      {


        #ifdef VERBOSE
        os_sprintf(temp, "%s:%d,\"TCP\",\"%d.%d.%d.%d\",%d,%d\n",
                   at_fun[id].at_cmdName,
                   pLink[i].linkId,
                   IP2STR(pLink[i].pCon->proto.tcp->remote_ip),
                   pLink[i].pCon->proto.tcp->remote_port,
                   pLink[i].teType);
        #else
            os_sprintf(temp, "%d,%d,%d.%d.%d.%d,%d,%d\n",
                   pLink[i].linkId,
                   CANWII_TCP,
                   IP2STR(pLink[i].pCon->proto.tcp->remote_ip),
                   pLink[i].pCon->proto.tcp->remote_port,
                   pLink[i].teType);
            uart0_sendStr(temp);
        #endif // VERBOSE
      }
      else
      {

        #ifdef VERBOSE
        os_sprintf(temp, "%s:%d,\"UDP\",\"%d.%d.%d.%d\",%d,%d,%d\n",
                   at_fun[id].at_cmdName,
                   pLink[i].linkId,
                   IP2STR(pLink[i].pCon->proto.udp->remote_ip),
                   pLink[i].pCon->proto.udp->remote_port,
                   pLink[i].pCon->proto.udp->local_port,
                   pLink[i].teType);
        #else
            os_sprintf(temp, "%d,%d,%d.%d.%d.%d,%d,%d\n",
                   pLink[i].linkId,
                   CANWII_UDP,
                   IP2STR(pLink[i].pCon->proto.tcp->remote_ip),
                   pLink[i].pCon->proto.tcp->remote_port,
                   pLink[i].teType);
            uart0_sendStr(temp);
        #endif // VERBOSE

      }

    }
  }
  uart_tx_one_char(CANWII_EOH);
  at_backOk;
}

/**
  * @brief  Test commad of start client.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCipstart(uint8_t id)
{
  char temp[64];

  if(at_ipMux)
  {
    os_sprintf(temp, "%s:(\"type\"),(\"ip address\"),(port)\n",
               at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    os_sprintf(temp, "%s:(\"type\"),(\"domain name\"),(port)\n",
               at_fun[id].at_cmdName);
    uart0_sendStr(temp);
  }
  else
  {
    os_sprintf(temp, "%s:(id)(\"type\"),(\"ip address\"),(port)\n",
               at_fun[id].at_cmdName);
    uart0_sendStr(temp);
    os_sprintf(temp, "%s:((id)\"type\"),(\"domain name\"),(port)\n",
               at_fun[id].at_cmdName);
    uart0_sendStr(temp);
  }
  at_backOk;
}

/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
  at_sendData(pdata,len,linkTemp->linkId);
}

/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_udpclient_recv(void *arg, char *pdata, unsigned short len)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;

  os_printf("recv\n");
  if(linkTemp->changType == 0) //if when sending, receive data???
  {
    os_memcpy(pespconn->proto.udp->remote_ip, linkTemp->remoteIp, 4);
    pespconn->proto.udp->remote_port = linkTemp->remotePort;
  }
  else if(linkTemp->changType == 1)
  {
    os_memcpy(linkTemp->remoteIp, pespconn->proto.udp->remote_ip, 4);
    linkTemp->remotePort = pespconn->proto.udp->remote_port;
    linkTemp->changType = 0;
  }

  at_sendData(pdata,len,linkTemp->linkId);
}

void ICACHE_FLASH_ATTR
at_sendData(char *pdata, unsigned short len,uint8_t linkId)
{
    char temp[32];
#ifdef VERBOSE
      os_printf("recv\n");
      if(at_ipMux)
      {
        os_sprintf(temp, "\n+IPD,%d,%d:",linkId, len);
        uart0_sendStr(temp);
        uart0_tx_buffer(pdata, len);
      }
      else if(IPMODE == FALSE)
      {
        os_sprintf(temp, "\n+IPD,%d:", len);
        uart0_sendStr(temp);
        uart0_tx_buffer(pdata, len);
      }
      else
      {
        uart0_tx_buffer(pdata, len);
        return;
      }
      at_backOk;
    #else
      os_printf("recv\n");
      if(at_ipMux)
      {
        os_sprintf(temp, "%d%d%d%d",CANWII_SOH,CMD_IPD,linkId, len);
        uart0_sendStr(temp);
        uart0_tx_buffer(pdata, len);
        uart_tx_one_char(CANWII_EOH);
      }
      else if(IPMODE == FALSE)
      {
        os_sprintf(temp, "%d%d%d%d",CANWII_SOH,CMD_IPD,255, len);
        uart0_sendStr(temp);
        uart0_tx_buffer(pdata, len);
        uart_tx_one_char(CANWII_EOH);
      }
      else
      {
        uart0_tx_buffer(pdata, len);
        return;
      }
      at_backOk;
    #endif // VERBOSE
}

/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg)
{
//	os_free(at_dataLine);
//  os_printf("send_cb\r\n");
  if(IPMODE == TRUE)
  {
    ipDataSendFlag = 0;
  	os_timer_disarm(&at_delayCheck);
  	os_timer_arm(&at_delayCheck, 20, 0);
  	system_os_post(at_recvTaskPrio, 0, 0); ////
    ETS_UART_INTR_ENABLE();
    return;
  }
  specialAtState = TRUE;
  at_state = at_statIdle;
  //TODO:change message
  generalMSG.msgid=MSG_SEND;
  generalMSG.param0=NULLPARAM;
  sendGeneralMsg(generalMSG);
  //uart0_sendStr("\nSEND OK\n");
}

///**
//  * @brief  Send over callback function.
//  * @param  arg: contain the ip link information
//  * @retval None
//  */
//static void ICACHE_FLASH_ATTR
//at_udp_sent_cb(void *arg)
//{
////  os_free(at_dataLine);
////  os_printf("send_cb\r\n");
//  if(IPMODE == TRUE)
//  {
//    ipDataSendFlag = 0;
//    os_timer_disarm(&at_delayCheck);
//    os_timer_arm(&at_delayCheck, 20, 0);
//    system_os_post(at_recvTaskPrio, 0, 0); ////
//    ETS_UART_INTR_ENABLE();
//    return;
//  }
//  uart0_sendStr("\r\nSEND OK\r\n");
//  specialAtState = TRUE;
//  at_state = at_statIdle;
//}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_connect_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
  char temp[16];

  os_printf("tcp client connect\n");
  os_printf("pespconn %p\n", pespconn);

  linkTemp->linkEn = TRUE;
  linkTemp->teType = teClient;
  linkTemp->repeaTime = 0;
  espconn_regist_disconcb(pespconn, at_tcpclient_discon_cb);
  espconn_regist_recvcb(pespconn, at_tcpclient_recv);////////
  espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);///////

  mdState = m_linked;
  if(at_state == at_statIpTraning)
 	{
 		return;
  }
  if(at_ipMux)
  {
    //TODO:change message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=linkTemp->linkId;
    sendGeneralMsg(generalMSG);
    //os_sprintf(temp,"%d,CONNECT\n", linkTemp->linkId);
    //uart0_sendStr(temp);
  }
  else
  {
    //TODO:change message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=-1;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CONNECT\n");
  }
  at_backOk;

  specialAtState = TRUE;
  at_state = at_statIdle;
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_recon_cb(void *arg, sint8 errType)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
  struct ip_info ipconfig;
  os_timer_t sta_timer;
  char temp[16];


  if(at_state == at_statIpTraning)
  {
  	linkTemp->repeaTime++;
    ETS_UART_INTR_ENABLE();
    os_printf("Traning recon\n");
    if(linkTemp->repeaTime > 10)
    {
    	linkTemp->repeaTime = 10;
    }
    os_delay_us(linkTemp->repeaTime * 10000);
    pespconn->proto.tcp->local_port = espconn_port();
    espconn_connect(pespconn);
    return;
  }
  //TODO: change the message
    generalMSG.msgid=MSG_CLOSED;
    generalMSG.param0=linkTemp->linkId;
    sendGeneralMsg(generalMSG);
  //os_sprintf(temp,"%d,CLOSED\n", linkTemp->linkId);
  //uart0_sendStr(temp);

  if(linkTemp->teToff == TRUE)
  {
    linkTemp->teToff = FALSE;
    linkTemp->repeaTime = 0;
    if(pespconn->proto.tcp != NULL)
    {
      os_free(pespconn->proto.tcp);
    }
    os_free(pespconn);
    linkTemp->linkEn = false;
    at_linkNum--;
    if(at_linkNum == 0)
    {
      at_backOk;
      mdState = m_unlink;
      disAllFlag = false;
      specialAtState = TRUE;
      at_state = at_statIdle;
    }
  }
  else
  {
    linkTemp->repeaTime++;
    if(linkTemp->repeaTime >= 1)
    {
      os_printf("repeat over %d\n", linkTemp->repeaTime);
      linkTemp->repeaTime = 0;
      if(errType == ESPCONN_CLSD)
      {
        at_backOk;
      }
      else
      {
        at_backError;
      }
      if(pespconn->proto.tcp != NULL)
      {
        os_free(pespconn->proto.tcp);
      }
      os_free(pespconn);
      linkTemp->linkEn = false;
      os_printf("disconnect\n");
      at_linkNum--;
      if (at_linkNum == 0)
      {
        mdState = m_unlink;
        disAllFlag = false;
      }
      ETS_UART_INTR_ENABLE();
      specialAtState = true;
      at_state = at_statIdle;
      return;
    }

    specialAtState = true;
    at_state = at_statIdle;
    os_printf("link repeat %d\n", linkTemp->repeaTime);
    pespconn->proto.tcp->local_port = espconn_port();
    espconn_connect(pespconn);
  }
}


static ip_addr_t host_ip;
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct espconn *pespconn = (struct espconn *) arg;
  at_linkConType *linkTemp = (at_linkConType *) pespconn->reverse;
  char temp[16];

  if(ipaddr == NULL)
  {
    linkTemp->linkEn = FALSE;
    //TODO: change the message
    generalMSG.msgid=MSG_DNS_FAIL;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("DNS Fail\n");
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }

  os_printf("DNS found: %d.%d.%d.%d\n",
            *((uint8 *) &ipaddr->addr),
            *((uint8 *) &ipaddr->addr + 1),
            *((uint8 *) &ipaddr->addr + 2),
            *((uint8 *) &ipaddr->addr + 3));

  if(host_ip.addr == 0 && ipaddr->addr != 0)
  {
    if(pespconn->type == ESPCONN_TCP)
    {
      os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
      espconn_connect(pespconn);
      at_linkNum++;
    }
    else
    {
      os_memcpy(pespconn->proto.udp->remote_ip, &ipaddr->addr, 4);
      os_memcpy(linkTemp->remoteIp, &ipaddr->addr, 4);
      espconn_connect(pespconn);
      specialAtState = TRUE;
      at_state = at_statIdle;
      at_linkNum++;
      //TODO: change the message
      generalMSG.msgid=MSG_CONNECT;
      generalMSG.param0=linkTemp->linkId;
      sendGeneralMsg(generalMSG);
      //os_sprintf(temp,"%d,CONNECT\n", linkTemp->linkId);
      //uart0_sendStr(temp);
      at_backOk;
    }
  }
}

/**
  * @brief  Setup commad of start client.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipstart(uint8_t id, char *pPara)
{
  char temp[64];
//  enum espconn_type linkType;
  int8_t len;
  enum espconn_type linkType = ESPCONN_INVALID;
  uint32_t ip = 0;
  char ipTemp[128];
  int32_t remotePort,localPort;
  uint8_t linkID;
  uint8_t changType;

  char ret;

  remotePort = 0;
  localPort = 0;
  if(at_wifiMode == 1)
  {
    if(wifi_station_get_connect_status() != STATION_GOT_IP)
    {
      uart0_sendStr("no ip\n");
      return;
    }
  }
  pPara++;
  if(at_ipMux)
  {
    linkID = atoi(pPara);
    pPara++;
    pPara = strchr(pPara, '\"');
  }
  else
  {
    linkID = 0;
  }
  if(linkID >= at_linkMax)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_ID_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("ID ERROR\n");
    return;
  }
  len = at_dataStrCpy(temp, pPara, 6);
  if(len == -1)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_LINK_TYPE_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("Link typ ERROR\n");
    return;
  }
  if(os_strcmp(temp, "TCP") == 0)
  {
    linkType = ESPCONN_TCP;
  }
  else if(os_strcmp(temp, "UDP") == 0)
  {
    linkType = ESPCONN_UDP;
  }
  else
  {
    //TODO: change the message
    generalMSG.msgid=MSG_LINK_TYPE_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("Link typ ERROR\n");
    return;
  }
  pPara += (len+3);
  len = at_dataStrCpy(ipTemp, pPara, 64);
  os_printf("%s\n", ipTemp);
  if(len == -1)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_IP_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("IP ERROR\n");
    return;
  }
  pPara += (len+2);
  if(*pPara != ',')
  {
    //TODO: change the message
    generalMSG.msgid=MSG_ENTRY_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("ENTRY ERROR\n");
    return;
  }
  pPara += (1);
  remotePort = atoi(pPara);

  if(linkType == ESPCONN_UDP)
  {
    os_printf("remote port:%d\n", remotePort);
    pPara = strchr(pPara, ',');
    if(pPara == NULL)
    {
      if((remotePort == 0)|(ipTemp[0] == 0))
      {
        //TODO: change the message
        generalMSG.msgid=MSG_MISS_PARAM;
        generalMSG.param0=1;
        sendGeneralMsg(generalMSG);
        //uart0_sendStr("Miss param\n");
        return;
      }
    }
    else
    {
      pPara += 1;
      localPort = atoi(pPara);
      if(localPort == 0)
      {
        //TODO: change the message
        generalMSG.msgid=MSG_MISS_PARAM;
        generalMSG.param0=2;
        sendGeneralMsg(generalMSG);
        //uart0_sendStr("Miss param2\n");
        return;
      }
      os_printf("local port:%d\n", localPort);

      pPara = strchr(pPara, ',');
      if(pPara == NULL)
      {
        changType = 0;
      }
      else
      {
        pPara += 1;
        changType = atoi(pPara);
      }
      os_printf("change type:%d\n", changType);
    }
  }

  if(pLink[linkID].linkEn)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_ALREADY_CONNECT;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("ALREADY CONNECT\n");
    return;
  }
  pLink[linkID].pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
  if (pLink[linkID].pCon == NULL)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CONNECT_FAIL;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CONNECT FAIL\n");
    return;
  }
  pLink[linkID].pCon->type = linkType;
  pLink[linkID].pCon->state = ESPCONN_NONE;
  pLink[linkID].linkId = linkID;

  switch(linkType)
  {
  case ESPCONN_TCP:
    ip = ipaddr_addr(ipTemp);
    pLink[linkID].pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pLink[linkID].pCon->proto.tcp->local_port = espconn_port();
    pLink[linkID].pCon->proto.tcp->remote_port = remotePort;

    os_memcpy(pLink[linkID].pCon->proto.tcp->remote_ip, &ip, 4);

    pLink[linkID].pCon->reverse = &pLink[linkID];

    espconn_regist_connectcb(pLink[linkID].pCon, at_tcpclient_connect_cb);
    espconn_regist_reconcb(pLink[linkID].pCon, at_tcpclient_recon_cb);
    specialAtState = FALSE;
    if((ip == 0xffffffff) && (os_memcmp(ipTemp,"255.255.255.255",16) != 0))
    {
      espconn_gethostbyname(pLink[linkID].pCon, ipTemp, &host_ip, at_dns_found);
    }
    else
    {
      espconn_connect(pLink[linkID].pCon);
      at_linkNum++;
    }
    break;

  case ESPCONN_UDP:
    pLink[linkID].pCon->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    if(localPort == 0)
    {
      pLink[linkID].pCon->proto.udp->local_port = espconn_port();
    }
    else
    {
      pLink[linkID].pCon->proto.udp->local_port = localPort;
    }
    if(remotePort == 0)
    {
      pLink[linkID].pCon->proto.udp->remote_port = espconn_port();
    }
    else
    {
      pLink[linkID].pCon->proto.udp->remote_port = remotePort;
    }

    pLink[linkID].changType = changType;
    pLink[linkID].remotePort = pLink[linkID].pCon->proto.udp->remote_port;

    pLink[linkID].pCon->reverse = &pLink[linkID];
//    os_printf("%d\r\n",pLink[linkID].pCon->proto.udp->local_port);///

    pLink[linkID].linkId = linkID;
    pLink[linkID].linkEn = TRUE;
    pLink[linkID].teType = teClient;
    espconn_regist_recvcb(pLink[linkID].pCon, at_udpclient_recv);
    espconn_regist_sentcb(pLink[linkID].pCon, at_tcpclient_sent_cb);
    if(ipTemp[0] == 0)
    {
      ip = 0xffffffff;
      os_memcpy(ipTemp,"255.255.255.255",16);
    }
    else
    {
      ip = ipaddr_addr(ipTemp);
    }
    os_memcpy(pLink[linkID].pCon->proto.udp->remote_ip, &ip, 4);
    os_memcpy(pLink[linkID].remoteIp, &ip, 4);
    if((ip == 0xffffffff) && (os_memcmp(ipTemp,"255.255.255.255",16) != 0))
    {
      specialAtState = FALSE;
      espconn_gethostbyname(pLink[linkID].pCon, ipTemp, &host_ip, at_dns_found);
    }
    else
    {
      ret = espconn_create(pLink[linkID].pCon);
      //TODO: change the message
      generalMSG.msgid=MSG_CONNECT;
      generalMSG.param0=linkID;
      sendGeneralMsg(generalMSG);
      //os_sprintf(temp,"%d,CONNECT\n", linkID);
      //uart0_sendStr(temp);
      at_linkNum++;
      at_backOk;
    }
    break;

  default:
    break;
  }
}

/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpclient_discon_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
  uint8_t idTemp;
  char temp[16];

  if(pespconn == NULL)
  {
    return;
  }
  if(at_state == at_statIpTraning)
  {
    ETS_UART_INTR_ENABLE();
    os_printf("Traning nodiscon\n");
    pespconn->proto.tcp->local_port = espconn_port();
    espconn_connect(pespconn);
    return;
  }
  if(pespconn->proto.tcp != NULL)
  {
    os_free(pespconn->proto.tcp);
  }
  os_free(pespconn);

  linkTemp->linkEn = FALSE;
  if(at_ipMux)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CLOSED;
    generalMSG.param0=linkTemp->linkId;
    sendGeneralMsg(generalMSG);
    //os_sprintf(temp,"%d,CLOSED\n", linkTemp->linkId);
    //uart0_sendStr(temp);
  }
  else
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CLOSED;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CLOSED\n");
  }


  at_linkNum--;

  if(disAllFlag == FALSE)
  {
    at_backOk;
  }
  if(at_linkNum == 0)
  {
    mdState = m_unlink;
    if(disAllFlag)
    {
      at_backOk;
    }
    disAllFlag = FALSE;
  }

  if(disAllFlag)
  {
    idTemp = linkTemp->linkId + 1;
    for(; idTemp<at_linkMax; idTemp++)
    {
      if(pLink[idTemp].linkEn)
      {
        if(pLink[idTemp].teType == teServer)
        {
          continue;
        }
        if(pLink[idTemp].pCon->type == ESPCONN_TCP)
        {
        	specialAtState = FALSE;
          espconn_disconnect(pLink[idTemp].pCon);
        	break;
        }
        else
        {
          //TODO: change the message
          generalMSG.msgid=MSG_CLOSED;
          generalMSG.param0=pLink[idTemp].linkId;
          sendGeneralMsg(generalMSG);
          //os_sprintf(temp,"%d,CLOSED\n", pLink[idTemp].linkId);
          //uart0_sendStr(temp);
          pLink[idTemp].linkEn = FALSE;
          espconn_delete(pLink[idTemp].pCon);
          os_free(pLink[idTemp].pCon->proto.udp);
          os_free(pLink[idTemp].pCon);
          at_linkNum--;
          if(at_linkNum == 0)
          {
            mdState = m_unlink;
            at_backOk;
            disAllFlag = FALSE;
          }
        }
      }
    }
  }
  ETS_UART_INTR_ENABLE();
  specialAtState = TRUE;
  at_state = at_statIdle;
}


/**
  * @brief  Setup commad of close ip link.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipclose(uint8_t id, char *pPara)
{
  char temp[64];
  uint8_t linkID;
  uint8_t i;

  pPara++;
  if(at_ipMux == 0)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_MUX;
    generalMSG.param0=0;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("MUX=0\n");
    return;
  }
  linkID = atoi(pPara);
  if(linkID > at_linkMax)
  {
    at_backError;
    return;
  }
  if(linkID == at_linkMax)
  {
    if(serverEn)
    {
      /* restart */
      //TODO: change the message
      generalMSG.msgid=MSG_RESTART;
      generalMSG.param0=NULLPARAM;
      sendGeneralMsg(generalMSG);
      //uart0_sendStr("we must restart\n");
      return;
    }
    for(linkID=0; linkID<at_linkMax; linkID++)
    {
      if(pLink[linkID].linkEn)
      {
        if(pLink[linkID].pCon->type == ESPCONN_TCP)
        {
          pLink[linkID].teToff = TRUE;
          specialAtState = FALSE;
          espconn_disconnect(pLink[linkID].pCon);
          disAllFlag = TRUE;

          break;
        }
        else
        {
          pLink[linkID].linkEn = FALSE;
          //TODO: change the message
          generalMSG.msgid=MSG_CLOSED;
          generalMSG.param0=linkID;
          sendGeneralMsg(generalMSG);
          //os_sprintf(temp,"%d,CLOSED\n", linkID);
          uart0_sendStr(temp);
          espconn_delete(pLink[linkID].pCon);
          os_free(pLink[linkID].pCon->proto.udp);
          os_free(pLink[linkID].pCon);
          at_linkNum--;
          if(at_linkNum == 0)
          {
            mdState = m_unlink;
            at_backOk;
          }
        }
      }
    }
  }
  else
  {
    if(pLink[linkID].linkEn == FALSE)
    {
      //TODO: change the message
      generalMSG.msgid=MSG_LINK_SET_FAIL;
      generalMSG.param0=pLink[linkID].linkId;
      sendGeneralMsg(generalMSG);
      //uart0_sendStr("link is not\n");
      return;
    }
    if(pLink[linkID].teType == teServer)
    {
      if(pLink[linkID].pCon->type == ESPCONN_TCP)
      {
        pLink[linkID].teToff = TRUE;
        specialAtState = FALSE;
        espconn_disconnect(pLink[linkID].pCon);
      }
      else
      {
        pLink[linkID].linkEn = FALSE;
        //TODO: change the message
        generalMSG.msgid=MSG_CLOSED;
        generalMSG.param0=linkID;
        sendGeneralMsg(generalMSG);
        //os_sprintf(temp,"%d,CLOSED\n", linkID);
        //uart0_sendStr(temp);
        espconn_delete(pLink[linkID].pCon);
        at_linkNum--;
        at_backOk;
        if(at_linkNum == 0)
        {
          mdState = m_unlink;

        }
      }
    }
    else
    {
      if(pLink[linkID].pCon->type == ESPCONN_TCP)
      {
        pLink[linkID].teToff = TRUE;
        specialAtState = FALSE;
        espconn_disconnect(pLink[linkID].pCon);
      }
      else
      {
        pLink[linkID].linkEn = FALSE;
        //TODO: change the message
        generalMSG.msgid=MSG_CLOSED;
        generalMSG.param0=linkID;
        sendGeneralMsg(generalMSG);
        //os_sprintf(temp,"%d,CLOSED\n", linkID);
        //uart0_sendStr(temp);
        espconn_delete(pLink[linkID].pCon);
        os_free(pLink[linkID].pCon->proto.udp);
        os_free(pLink[linkID].pCon);
        at_linkNum--;
        at_backOk;
        if(at_linkNum == 0)
        {
          mdState = m_unlink;
        }
      }
    }
  }
}

/**
  * @brief  Execution commad of close ip link.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipclose(uint8_t id)
{
  char temp[64];

  if(at_ipMux)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_MUX;
    generalMSG.param0=1;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("MUX=1\n");
    return;
  }
  if(pLink[0].linkEn)
  {
    if(serverEn)
    {
      /* restart */
      //TODO: change the message
      generalMSG.msgid=MSG_RESTART;
      generalMSG.param0=NULLPARAM;
      sendGeneralMsg(generalMSG);
      //uart0_sendStr("we must restart\n");
      return;
    }
    else
    {
      if(pLink[0].pCon->type == ESPCONN_TCP)
      {
        specialAtState = FALSE;
        pLink[0].teToff = TRUE;
        espconn_disconnect(pLink[0].pCon);
      }
      else
      {
        pLink[0].linkEn = FALSE;
        //TODO: change the message
        generalMSG.msgid=MSG_CLOSED;
        generalMSG.param0=NULLPARAM;
        sendGeneralMsg(generalMSG);
        //uart0_sendStr("CLOSED\n");
        espconn_delete(pLink[0].pCon);
        os_free(pLink[0].pCon->proto.udp);
        os_free(pLink[0].pCon);
        at_linkNum--;
        if(at_linkNum == 0)
        {
          mdState = m_unlink;
          at_backOk;
        }
      }
    }
  }
  else
  {
    at_backError;
  }
}

char * ICACHE_FLASH_ATTR
at_checkLastNum(char *pPara, uint8_t maxLen)
{
  int8_t ret = -1;
  char *pTemp;
  uint8_t i;

  pTemp = pPara;
  for(i=0;i<maxLen;i++)
  {
    if((*pTemp > '9')||(*pTemp < '0'))
    {
      break;
    }
    pTemp++;
  }
  if(i == maxLen)
  {
    return NULL;
  }
  else
  {
    return pTemp;
  }
}
/**
  * @brief  Setup commad of send ip data.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipsend(uint8_t id, char *pPara)
{

  if(IPMODE == TRUE)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_IP_MODE;
    generalMSG.param0=1;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("IPMODE=1\n");
    at_backError;
    return;
  }
  pPara++;
  if(at_ipMux)
  {
    sendingID = atoi(pPara);
    if(sendingID >= at_linkMax)
    {
      at_backError;
      return;
    }
    pPara++;
    if(*pPara != ',') //ID must less 10
    {
      at_backError;
      return;
    }
    pPara++;
  }
  else
  {
    sendingID = 0;
  }
  if(pLink[sendingID].linkEn == FALSE)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_LINK_SET_FAIL;
    generalMSG.param0=pLink[sendingID].linkId;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("link is not\n");
    return;
  }
  at_sendLen = atoi(pPara);
  if(at_sendLen > 2048)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_TOO_LONG;
    generalMSG.param0=at_sendLen;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("too long\n");
    return;
  }
  pPara = at_checkLastNum(pPara, 5);
  if((pPara == NULL)||(*pPara != CANWII_EOH))
  {
    //TODO: change the message
    generalMSG.msgid=MSG_TYPE_ERROR;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("type error\n");
    return;
  }

  pDataLine = at_dataLine;

  specialAtState = FALSE;
  at_state = at_statIpSending;
  //TODO:Review
  uart0_sendStr("> ");
}

/**
  * @brief  Send data through ip.
  * @param  pAtRcvData: point to data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataSending(uint8_t *pAtRcvData)
{
  espconn_sent(pLink[sendingID].pCon, pAtRcvData, at_sendLen);
  os_printf("id:%d,Len:%d,dp:%p\n",sendingID,at_sendLen,pAtRcvData);
  //bug if udp,send is ok
//  if(pLink[sendingID].pCon->type == ESPCONN_UDP)
//  {
//    uart0_sendStr("\r\nSEND OK\r\n");
//    specialAtState = TRUE;
//    at_state = at_statIdle;
//  }
}

/**
  * @brief  Transparent data through ip.
  * @param  arg: no used
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataTransparent(void *arg)
{
	if(at_state != at_statIpTraning)
	{
		return;
	}

	os_timer_disarm(&at_delayCheck);
	if((at_tranLen == 3) && (os_memcmp(at_dataLine, "+++", 3) == 0))
	{

		specialAtState = TRUE;
        at_state = at_statIdle;
		return;
	}
	else if(at_tranLen)
	{
	  ETS_UART_INTR_DISABLE();
    espconn_sent(pLink[0].pCon, at_dataLine, at_tranLen);
    ipDataSendFlag = 1;
    pDataLine = at_dataLine;
  	at_tranLen = 0;
  	return;
  }
  os_timer_arm(&at_delayCheck, 20, 0);
}

/**
  * @brief  Send data through ip.
  * @param  pAtRcvData: point to data
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_ipDataSendNow(void)
{
  espconn_sent(pLink[0].pCon, at_dataLine, at_tranLen);
  ipDataSendFlag = 1;
  pDataLine = at_dataLine;
  at_tranLen = 0;
}

/**
  * @brief  Setup commad of send ip data.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCipsend(uint8_t id)
{
	if((serverEn) || (IPMODE == FALSE))
	{
		at_backError;
		return;
	}
	if(pLink[0].linkEn == FALSE)
  {
	  at_backError;
	  return;
  }
	pDataLine = at_dataLine;
	at_tranLen = 0;
  specialAtState = FALSE;
  at_state = at_statIpTraning;
  os_timer_disarm(&at_delayCheck);
  os_timer_setfn(&at_delayCheck, (os_timer_func_t *)at_ipDataTransparent, NULL);
  os_timer_arm(&at_delayCheck, 20, 0);
  //TODO: change the message
  uart0_sendStr("\n>");
}

/**
  * @brief  Query commad of set multilink mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCipmux(uint8_t id)
{
  char temp[32];

  #ifdef VERBOSE
    os_sprintf(temp, "%s:%d\n",at_fun[id].at_cmdName, at_ipMux);
  #else
    os_sprintf(temp, "%d%d%d%d",CANWII_SOH,at_fun[id].at_cmdCode, at_ipMux,CANWII_EOH);
  #endif // VERBOSE

  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of set multilink mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipmux(uint8_t id, char *pPara)
{
  uint8_t muxTemp;
  if(mdState == m_linked)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_LINK_DONE;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("link is builded\n");
    return;
  }
  pPara++;
  muxTemp = atoi(pPara);
  if(muxTemp == 1)
  {
    at_ipMux = TRUE;
  }
  else if(muxTemp == 0)
  {
    at_ipMux = FALSE;
  }
  else
  {
    at_backError;
    return;
  }
  at_backOk;
}


/**
  * @brief  Tcp server disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *) arg;
  at_linkConType *linkTemp = (at_linkConType *) pespconn->reverse;
  char temp[16];

  os_printf("S conect C: %p\n", arg);

  linkTemp->linkEn = FALSE;
  linkTemp->pCon = NULL;

  if(at_ipMux)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CLOSED;
    generalMSG.param0=linkTemp->linkId;
    sendGeneralMsg(generalMSG);
    //os_sprintf(temp,"%d,CLOSED\n", linkTemp->linkId);
    //uart0_sendStr(temp);
  }
  else
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CLOSED;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CLOSED\n");
  }
  if(linkTemp->teToff == TRUE)
  {
    linkTemp->teToff = FALSE;
    at_backOk;
  }
  at_linkNum--;
  if (at_linkNum == 0)
  {
    mdState = m_unlink;
//    uart0_sendStr("Unlink\r\n");
    disAllFlag = false;
  }
  ETS_UART_INTR_ENABLE();
  specialAtState = true;
  at_state = at_statIdle;
}

/**
  * @brief  Tcp server connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType)
{
  struct espconn *pespconn = (struct espconn *)arg;
  at_linkConType *linkTemp = (at_linkConType *)pespconn->reverse;
  char temp[16];

  os_printf("S conect C: %p\n", arg);

  if(pespconn == NULL)
  {
    return;
  }

  linkTemp->linkEn = false;
  linkTemp->pCon = NULL;
  os_printf("con EN? %d\n", linkTemp->linkId);
  at_linkNum--;
  if (at_linkNum == 0)
  {
    mdState = m_unlink;
  }

  if(at_ipMux)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=linkTemp->linkId;
    sendGeneralMsg(generalMSG);
    //os_sprintf(temp, "%d,CONNECT\n", linkTemp->linkId);
    //uart0_sendStr(temp);
  }
  else
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CONNECT\n");
  }
  disAllFlag = false;
  if(linkTemp->teToff == TRUE)
  {
    linkTemp->teToff = FALSE;
    at_backOk;
  }
  ETS_UART_INTR_ENABLE();
  specialAtState = true;
  at_state = at_statIdle;
}

/**
  * @brief  Tcp server listend callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
at_tcpserver_listen(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  uint8_t i;
  char temp[16];

  os_printf("get tcpClient:\n");
  for(i=0;i<at_linkMax;i++)
  {
    if(pLink[i].linkEn == FALSE)
    {
      pLink[i].linkEn = TRUE;
      break;
    }
  }
  if(i>=5)
  {
    return;
  }
  pLink[i].teToff = FALSE;
  pLink[i].linkId = i;
  pLink[i].teType = teServer;
  pLink[i].repeaTime = 0;
  pLink[i].pCon = pespconn;
  mdState = m_linked;
  at_linkNum++;
  pespconn->reverse = &pLink[i];
  espconn_regist_recvcb(pespconn, at_tcpclient_recv);
  espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
  espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
  espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);///////
  if(at_ipMux)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=i;
    sendGeneralMsg(generalMSG);
    //os_sprintf(temp, "%d,CONNECT\n", i);
    //uart0_sendStr(temp);
  }
  else
  {
    //TODO: change the message
    generalMSG.msgid=MSG_CONNECT;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("CONNECT\n");
  }
}

///**
//  * @brief  Udp server receive data callback function.
//  * @param  arg: contain the ip link information
//  * @retval None
//  */
//LOCAL void ICACHE_FLASH_ATTR
//at_udpserver_recv(void *arg, char *pusrdata, unsigned short len)
//{
//  struct espconn *pespconn = (struct espconn *)arg;
//  at_linkConType *linkTemp;
//  char temp[32];
//  uint8_t i;
//
//  os_printf("get udpClient:\r\n");
//
//  if(pespconn->reverse == NULL)
//  {
//    for(i = 0;i < at_linkMax;i++)
//    {
//      if(pLink[i].linkEn == FALSE)
//      {
//        pLink[i].linkEn = TRUE;
//        break;
//      }
//    }
//    if(i >= 5)
//    {
//      return;
//    }
//    pLink[i].teToff = FALSE;
//    pLink[i].linkId = i;
//    pLink[i].teType = teServer;
//    pLink[i].repeaTime = 0;
//    pLink[i].pCon = pespconn;
//    espconn_regist_sentcb(pLink[i].pCon, at_tcpclient_sent_cb);
//    mdState = m_linked;
//    at_linkNum++;
//    pespconn->reverse = &pLink[i];
//    uart0_sendStr("Link\r\n");
//  }
//  linkTemp = (at_linkConType *)pespconn->reverse;
//  if(pusrdata == NULL)
//  {
//    return;
//  }
//  os_sprintf(temp, "\r\n+IPD,%d,%d:",
//             linkTemp->linkId, len);
//  uart0_sendStr(temp);
//  uart0_tx_buffer(pusrdata, len);
//  at_backOk;
//}

/**
  * @brief  Setup commad of module as server.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipserver(uint8_t id, char *pPara)
{
  BOOL serverEnTemp;
  int32_t port;
  char temp[32];

  if(at_ipMux == FALSE)
  {
    at_backError;
    return;
  }
  pPara++;
  serverEnTemp = atoi(pPara);
  pPara++;
  if(serverEnTemp == 0)
  {
    if(*pPara != CANWII_EOH)
    {
      at_backError;
      return;
    }
  }
  else if(serverEnTemp == 1)
  {
    if(*pPara == ',')
    {
      pPara++;
      port = atoi(pPara);
    }
    else
    {
      port = 333;
    }
  }
  else
  {
    at_backError;
    return;
  }
  if(serverEnTemp == serverEn)
  {
    //TODO: change the message
    generalMSG.msgid=MSG_NO_CHANGE;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("no change\n");
    return;
  }

  if(serverEnTemp)
  {
    pTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    if (pTcpServer == NULL)
    {
      //TODO: change the message
      generalMSG.msgid=MSG_TCP_SERVER_FAIL;
      generalMSG.param0=NULLPARAM;
      sendGeneralMsg(generalMSG);
      //uart0_sendStr("TcpServer Failure\n");
      return;
    }
    pTcpServer->type = ESPCONN_TCP;
    pTcpServer->state = ESPCONN_NONE;
    pTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pTcpServer->proto.tcp->local_port = port;
    espconn_regist_connectcb(pTcpServer, at_tcpserver_listen);
    espconn_accept(pTcpServer);
    espconn_regist_time(pTcpServer, server_timeover, 0);

//    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
//    if (pUdpServer == NULL)
//    {
//      uart0_sendStr("UdpServer Failure\r\n");
//      return;
//    }
//    pUdpServer->type = ESPCONN_UDP;
//    pUdpServer->state = ESPCONN_NONE;
//    pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
//    pUdpServer->proto.udp->local_port = port;
//    pUdpServer->reverse = NULL;
//    espconn_regist_recvcb(pUdpServer, at_udpserver_recv);
//    espconn_create(pUdpServer);

//    if(pLink[0].linkEn)
//    {
//      uart0_sendStr("Link is builded\r\n");
//      return;
//    }
//    pLink[0].pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
//    if (pLink[0].pCon == NULL)
//    {
//      uart0_sendStr("Link buile Failure\r\n");
//      return;
//    }
//    pLink[0].pCon->type = ESPCONN_TCP;
//    pLink[0].pCon->state = ESPCONN_NONE;
//    pLink[0].linkId = 0;
//    pLink[0].linkEn = TRUE;
//
//    pLink[0].pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
//    pLink[0].pCon->proto.tcp->local_port = port;
//
//    pLink[0].pCon->reverse = &pLink[0];
//
//    espconn_regist_connectcb(pLink[0].pCon, user_test_tcpserver_listen);
//    espconn_accept(pLink[0].pCon);
//    at_linkNum++;
  }
  else
  {
    /* restart */
    //TODO: change the message
    generalMSG.msgid=MSG_RESTART;
    generalMSG.param0=NULLPARAM;
    sendGeneralMsg(generalMSG);
    //uart0_sendStr("we must restart\n");
    return;
  }
  serverEn = serverEnTemp;
  at_backOk;
}

/**
  * @brief  Query commad of set transparent mode.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCipmode(uint8_t id)
{
	char temp[32];
  #ifdef VERBOSE
    os_sprintf(temp, "%s:%d\n", at_fun[id].at_cmdName, IPMODE);
  #else
    os_sprintf(temp, "%d%d%d%d", CANWII_SOH,at_fun[id].at_cmdCode, IPMODE,CANWII_EOH);
  #endif

  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of transparent.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipmode(uint8_t id, char *pPara)
{
	uint8_t mode;
  char temp[32];

  pPara++;
  if((at_ipMux) || (serverEn))
  {
  	at_backError;
  	return;
  }
  mode = atoi(pPara);
  if(mode > 1)
  {
  	at_backError;
  	return;
  }
  IPMODE = mode;
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_queryCmdCipsto(uint8_t id)
{
  char temp[32];

  #ifdef VERBOSE
    os_sprintf(temp, "%s:%d\n",
             at_fun[id].at_cmdName, server_timeover);
  #else
    os_sprintf(temp, "%d%d%d%d", CANWII_SOH,at_fun[id].at_cmdCode, server_timeover,CANWII_EOH);
  #endif

  uart0_sendStr(temp);
  at_backOk;
}

void ICACHE_FLASH_ATTR
at_setupCmdCipsto(uint8_t id, char *pPara)
{
  char temp[64];
  uint16_t timeOver;

  if(serverEn == FALSE)
  {
    at_backError;
    return;
  }
  pPara++;
  timeOver = atoi(pPara);
  if(timeOver>28800)
  {
    at_backError;
    return;
  }
  if(timeOver != server_timeover)
  {
    server_timeover = timeOver;
    espconn_regist_time(pTcpServer, server_timeover, 0);
  }
  at_backOk;
  return;
}

#define ESP_PARAM_SAVE_SEC_0    1
#define ESP_PARAM_SAVE_SEC_1    2
#define ESP_PARAM_SEC_FLAG      3
#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"
#define pheadbuffer "Connection: keep-alive\n\
Cache-Control: no-cache\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \n\
Accept: */*\n\
Authorization: token %s\n\
Accept-Encoding: gzip,deflate,sdch\n\
Accept-Language: zh-CN,zh;q=0.8\n\n"
#define pheadbuffer "Connection: keep-alive\n\
Cache-Control: no-cache\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \n\
Accept: */*\n\
Authorization: token %s\n\
Accept-Encoding: gzip,deflate,sdch\n\
Accept-Language: zh-CN,zh;q=0.8\n\n"

#define test
#ifdef test
#define KEY "39cdfe29a1863489e788efc339f514d78b78f0de"
#else
#define KEY "4ec90c1abbd5ffc0b339f34560a2eb8d71733861"
#endif

//TODO:add other params
struct espconn *pespconn;
struct upgrade_server_info *upServer = NULL;
struct esp_platform_saved_param {
    uint8 devkey[40];
    uint8 token[40];
    uint8 activeflag;
    uint8 pad[3];
};
struct esp_platform_sec_flag_param {
    uint8 flag;
    uint8 pad[3];
};
//static struct esp_platform_saved_param esp_param;

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_upDate_rsp(void *arg)
{
  struct upgrade_server_info *server = arg;


  if(server->upgrade_flag == true)
  {
    os_printf("device_upgrade_success\n");
    at_backOk;
    system_upgrade_reboot();
  }
  else
  {
    os_printf("device_upgrade_failed\n");
    at_backError;
  }

  os_free(server->url);
  server->url = NULL;
  os_free(server);
  server = NULL;

//  espconn_disconnect(pespconn);
  specialAtState = TRUE;
  at_state = at_statIdle;
}

///******************************************************************************
// * FunctionName : user_esp_platform_load_param
// * Description  : load parameter from flash, toggle use two sector by flag value.
// * Parameters   : param--the parame point which write the flash
// * Returns      : none
//*******************************************************************************/
//void ICACHE_FLASH_ATTR
//user_esp_platform_load_param(struct esp_platform_saved_param *param)
//{
//    struct esp_platform_sec_flag_param flag;
//
//    load_user_param(ESP_PARAM_SEC_FLAG, 0, &flag, sizeof(struct esp_platform_sec_flag_param));
//
//    if (flag.flag == 0) {
//        load_user_param(ESP_PARAM_SAVE_SEC_0, 0, param, sizeof(struct esp_platform_saved_param));
//    } else {
//        load_user_param(ESP_PARAM_SAVE_SEC_1, 0, param, sizeof(struct esp_platform_saved_param));
//    }
//}

/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_discon_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  uint8_t idTemp;

  if(pespconn->proto.tcp != NULL)
  {
    os_free(pespconn->proto.tcp);
  }
  if(pespconn != NULL)
  {
    os_free(pespconn);
  }

  os_printf("disconnect\n");

  if(system_upgrade_start(upServer) == false)
  {
//    uart0_sendStr("+CIPUPDATE:0/r/n");
    at_backError;
    specialAtState = TRUE;
    at_state = at_statIdle;
  }
  else
  {
    //TODO: change the message
    uart0_sendStr("+CIPUPDATE:4\n");
  }
}

/**
  * @brief  Udp server receive data callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
at_upDate_recv(void *arg, char *pusrdata, unsigned short len)
{
  struct espconn *pespconn = (struct espconn *)arg;
  char temp[32];
  char *pTemp;
  uint8_t user_bin[9] = {0};
//  uint8_t devkey[41] = {0};
  uint8_t i;

  os_timer_disarm(&at_delayCheck);

//TODO: change the message
  uart0_sendStr("+CIPUPDATE:3\n");

//  os_printf("%s",pusrdata);
  pTemp = (char *)os_strstr(pusrdata,"rom_version\": ");
  if(pTemp == NULL)
  {
    return;
  }
  pTemp += sizeof("rom_version\": ");

//  user_esp_platform_load_param(&esp_param);

  upServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
  os_memcpy(upServer->upgrade_version, pTemp, 5);
  upServer->upgrade_version[5] = '\0';
  os_sprintf(upServer->pre_version, "v%d.%d", AT_VERSION_main, AT_VERSION_sub);

  upServer->pespconn = pespconn;

//  os_memcpy(devkey, esp_param.devkey, 40);
  os_memcpy(upServer->ip, pespconn->proto.tcp->remote_ip, 4);

  upServer->port = 80;

  upServer->check_cb = at_upDate_rsp;
  upServer->check_times = 60000;

  if(upServer->url == NULL)
  {
    upServer->url = (uint8 *) os_zalloc(512);
  }

  if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
  {
    os_memcpy(user_bin, "user2.bin", 10);
  }
  else if(system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
  {
    os_memcpy(user_bin, "user1.bin", 10);
  }

  os_sprintf(upServer->url,
        "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.1\nHost: "IPSTR":%d\n"pheadbuffer"",
        upServer->upgrade_version, user_bin, IP2STR(upServer->ip),
        upServer->port, KEY);

}

LOCAL void ICACHE_FLASH_ATTR
at_upDate_wait(void *arg)
{
  struct espconn *pespconn = arg;
  os_timer_disarm(&at_delayCheck);
  if(pespconn != NULL)
  {
    espconn_disconnect(pespconn);
  }
  else
  {
    at_backError;
    specialAtState = TRUE;
    at_state = at_statIdle;
  }
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
at_upDate_sent_cb(void *arg)
{
  struct espconn *pespconn = arg;
  os_timer_disarm(&at_delayCheck);
  os_timer_setfn(&at_delayCheck, (os_timer_func_t *)at_upDate_wait, pespconn);
  os_timer_arm(&at_delayCheck, 5000, 0);
  os_printf("at_upDate_sent_cb\n");
}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_connect_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  uint8_t user_bin[9] = {0};

  char *temp;
  //TODO: change the message
  uart0_sendStr("+CIPUPDATE:2\n");


  espconn_regist_disconcb(pespconn, at_upDate_discon_cb);
  espconn_regist_recvcb(pespconn, at_upDate_recv);
  espconn_regist_sentcb(pespconn, at_upDate_sent_cb);

  temp = (uint8 *) os_zalloc(512);

  os_sprintf(temp,"GET /v1/device/rom/?is_format_simple=true HTTP/1.0\nHost: "IPSTR":%d\n"pheadbuffer"",
             IP2STR(pespconn->proto.tcp->remote_ip),
             80, KEY);

  espconn_sent(pespconn, temp, os_strlen(temp));
  os_free(temp);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_upDate_recon_cb(void *arg, sint8 errType)
{
  struct espconn *pespconn = (struct espconn *)arg;

    at_backError;
    if(pespconn->proto.tcp != NULL)
    {
      os_free(pespconn->proto.tcp);
    }
    os_free(pespconn);
    os_printf("disconnect\n");

    if(upServer != NULL)
    {
      os_free(upServer);
      upServer = NULL;
    }
    at_backError;
    specialAtState = TRUE;
    at_state = at_statIdle;
}

/******************************************************************************
 * FunctionName : upServer_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
upServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct espconn *pespconn = (struct espconn *) arg;
//  char temp[32];

  if(ipaddr == NULL)
  {
    at_backError;
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }
  //TODO: change the message
  uart0_sendStr("+CIPUPDATE:1\n");

  if(host_ip.addr == 0 && ipaddr->addr != 0)
  {
    if(pespconn->type == ESPCONN_TCP)
    {
      os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
      espconn_regist_connectcb(pespconn, at_upDate_connect_cb);
      espconn_regist_reconcb(pespconn, at_upDate_recon_cb);
      espconn_connect(pespconn);
    }
  }
}

void ICACHE_FLASH_ATTR
at_exeCmdCiupdate(uint8_t id)
{
  pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
  pespconn->type = ESPCONN_TCP;
  pespconn->state = ESPCONN_NONE;
  pespconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
  pespconn->proto.tcp->local_port = espconn_port();
  pespconn->proto.tcp->remote_port = 80;

  specialAtState = FALSE;
  espconn_gethostbyname(pespconn, "iot.espressif.cn", &host_ip, upServer_dns_found);
}

void ICACHE_FLASH_ATTR
at_exeCmdCiping(uint8_t id)
{
	at_backOk;
}

void ICACHE_FLASH_ATTR
at_exeCmdCipappup(uint8_t id)
{

}

//send general messages back
void ICACHE_FLASH_ATTR
sendGeneralMsg(struct_MSGType msgtype)
{
    char temp[32];
    #ifdef VERBOSE
        switch (msgtype.msgid){
        case MSG_CONNECT:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"%d,CONNECT\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"CONNECT\n");
            }
            break;
        case MSG_SEND:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"SEND OK %d\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"SEND OK\n");
            }
            break;
        case MSG_CLOSED:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"%d,CLOSED\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"CLOSED\n");
            }
            break;
        case MSG_DNS_FAIL:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"DNS Fail %d\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"DNS Fail\n");
            }
            break;
        case MSG_ID_ERROR:
            os_sprintf(temp,"ID ERROR\n");
            break;
        case MSG_LINK_TYPE_ERROR:
            os_sprintf(temp,"LINK TYPE ERROR\n");
            break;
        case MSG_IP_ERROR:
            os_sprintf(temp,"IP ERROR\n");
            break;
        case MSG_ENTRY_ERROR:
            os_sprintf(temp,"ENTRY ERROR\n");
            break;
        case MSG_MISS_PARAM:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"MISS PARAM %d\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"MISS PARAM\n");
            }
            break;
        case MSG_ALREADY_CONNECT:
            os_sprintf(temp,"ALREADY CONNECTED\n");
            break;
        case MSG_CONNECT_FAIL:
            os_sprintf(temp,"CONNECT FAIL\n");
            break;
        case MSG_MUX:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"MUX=%d\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"MUX\n");
            }
            break;
        case MSG_RESTART:
            os_sprintf(temp,"Restarting\n");
            break;
        case MSG_LINK_SET_FAIL:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"Link not set %d\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"Link not set\n");
            }
            break;
        case MSG_TOO_LONG:
            if (msgtype.param0!=NULLPARAM){
                os_sprintf(temp,"Message length %d > 2048\n",msgtype.param0);
            }
            else{
                os_sprintf(temp,"Message too big\n");
            }
            break;
        case MSG_TYPE_ERROR:
            os_sprintf(temp,"TYPE ERROR\n");
            break;
        case MSG_LINK_DONE:
            os_sprintf(temp,"LINK SET\n");
            break;
        case MSG_NO_CHANGE:
            os_sprintf(temp,"NO CHANGE\n");
            break;
        case MSG_TCP_SERVER_FAIL:
            os_sprintf(temp,"TCP SERVER FAIL\n");
            break;
        }
    #else

    #endif // VERBOSE
    uart0_sendStr(temp);
}


/**
  * @}
  */
