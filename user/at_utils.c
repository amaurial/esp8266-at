#include "at_utils.h"


//TODO
bool ICACHE_FLASH_ATTR
setParamToEsp(char *param,uint8_t cmdid,esp_StoreType *espdata)
{
    char *val;
    uint8_t len;
    #ifdef DEBUG
        char tempStr[50];
        os_sprintf(tempStr,"mergid:%d\n",cmdid);
        uart0_sendStr(tempStr);
    #endif // DEBUG

    switch (cmdid){
    case CMD_MERG_CONFIG_AP_EXT:
        /*<SOH>
            <CMD_MERG_CONFIG_AP_EXTENDED><=>
                <CWMODE>
                <CWDHCP_P1>
                <CWDHCP_P2>
                <ssid>,<passwd>,
                <CWSAP_p3> channel
                <CWSAP_p4> wpa type
                <CIPMUX>
                <CREATESERVER>
                <PORT>
        <EOH>
        */
        //param starts with "="

        espdata->cmdid=cmdid;
        espdata->cmdsubid=0;
        val=param++;
        espdata->cwmode=*val-'0';
        val=param++;
        espdata->dhcp_mode=*val-'0';
        val=param++;
        espdata->dhcp_enable=*val-'0';
        //param++;
        //get the ssid
        len=at_dataStrCpyWithDelim(espdata->ssid, param, 16,CANWII_STR_SEP);
        if (len==-1){
            #ifdef DEBUG
                uart0_sendStr("failed to get the sssid\n");
            #endif // DEBUG
            return false;
        }
        espdata->ssidlen=len;
        param+=(len+1);
        len=at_dataStrCpyWithDelim(espdata->passwd, param, 16,CANWII_STR_SEP);
        if (len==-1){
            #ifdef DEBUG
            uart0_sendStr("failed to get the password\n");
            #endif // DEBUG
            return false;
        }
        espdata->passwdlen=len;
        param+=(len+1);
        val=param++;
        espdata->channel=*val-'0';
        val=param++;
        espdata->wpa=*val-'0';
        val=param++;
        espdata->cwmux=*val-'0';
        val=param++;
        espdata->server_mode=*val-'0';
        val=param++;
        espdata->port=*val;
        espdata->timeout=TCP_SERVER_TIMEOUT;
        return true;
    break;
    case CMD_MERG_CONFIG_AP:
        /*<SOH>
            <CMD_MERG_CONFIG_APT><=>
                <ssid>,<passwd>,
                <CWSAP_p3> channel
                <CWSAP_p4> wpa type
                <PORT>
        <EOH>
        */
        //param starts with "="

        espdata->cmdid=cmdid;
        espdata->cmdsubid=0;
        espdata->cwmode=3;
        espdata->dhcp_mode=2;
        espdata->dhcp_enable=0;
        espdata->cwmux=1;
        espdata->server_mode=1;
        espdata->timeout=TCP_SERVER_TIMEOUT;
        //param++;
        //get the ssid
        len=at_dataStrCpyWithDelim(espdata->ssid, param, 16,CANWII_STR_SEP);
        if (len==-1){
            #ifdef DEBUG
                uart0_sendStr("failed to get the sssid\n");
            #endif // DEBUG
            return false;
        }
        espdata->ssidlen=len;
        param+=(len+1);
        len=at_dataStrCpyWithDelim(espdata->passwd, param, 16,CANWII_STR_SEP);
        if (len==-1){
            #ifdef DEBUG
            uart0_sendStr("failed to get the password\n");
            #endif // DEBUG
            return false;
        }
        espdata->passwdlen=len;
        param+=(len+1);
        val=param++;
        espdata->channel=*val-'0';
        val=param++;
        espdata->wpa=*val-'0';
        val=param++;
        espdata->port=*val;
        return true;
    break;
    }
    #ifdef DEBUG
           uart0_sendStr("invalid merg id\n");
   #endif // DEBUG
    return false;

}

void ICACHE_FLASH_ATTR
logMessage(char *msg){
    uart0_sendStr(msg);
}


/**
  * @brief  Copy param from receive data to dest. Start to copy until find the delimiter.
  * @param  pDest: point to dest
  * @param  pSrc: point to source
  * @param  maxLen: copy max number of byte
  * @retval the length of param
  *   @arg -1: failure
  */
uint8_t ICACHE_FLASH_ATTR
at_dataStrCpyWithDelim(void *pDest, const void *pSrc, int8_t maxLen,char delim)
{

  char *pTempD = pDest;
  const char *pTempS = pSrc;
  int8_t len;

  //pTempS++;
  for(len=0; len<maxLen; len++)
  {
    if(*pTempS == delim)
    {
      *pTempD = '\0';
      break;
    }
    else
    {
      *pTempD = *pTempS++;
      pTempD++;
    }
  }
  if(len == maxLen)
  {
    return -1;
  }
  return len;
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
        case MSG_NOAP:
            os_sprintf(temp,"NO AP\n");
            break;
        }
    #else
        //char temp[5];
        os_sprintf(temp,"%d%d%d%d%d\n",CANWII_SOH,
             CANWII_ERR,
             msgtype.msgid,
             msgtype.param0,
             CANWII_EOH);
  #endif // VERBOSE

   uart0_sendStr(temp);
}
