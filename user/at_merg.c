#include "at_merg.h"

/**
  * @brief  Execution commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupMerg(uint8_t id,char *pPara )
{

  esp_StoreType esp;

  #ifdef DEBUG
            uart0_sendStr("executing merg setip\n");
  #endif // DEBUG

  pPara++;
  if (setParamToEsp(pPara,id,&esp)==false){
  #ifdef DEBUG
            uart0_sendStr("failed to parse the command\n");
  #endif // DEBUG
    at_backError;
    return;
  }
    #ifdef DEBUG
    char temp[255];
    os_sprintf(temp, "merg command: ssid-%s passwd-%s cmdid-%d cmdsubid-%d ssidlen-%d passwdlen-%d cwmode-%d cwmux-%d port-%d wpa-%d channel-%d dhcpmode-%d dhcpen-%d servermode-%d timeout-%d\n",
        esp.ssid,
        esp.passwd,
        esp.cmdid,
        esp.cmdsubid,
        esp.ssidlen,
        esp.passwdlen,
        esp.cwmode,
        esp.cwmux,
        esp.port,
        esp.wpa,
        esp.channel,
        esp.dhcp_mode,
        esp.dhcp_enable,
        esp.server_mode,
        esp.timeout);
    uart0_sendStr(temp);
    #endif // DEBUG

    #ifdef DEBUG
            uart0_sendStr("setting mode\n");
    #endif // DEBUG
    //set mode
    if (at_setupCmdCwmodeEsp(esp.cwmode)!=0){
        #ifdef DEBUG
                uart0_sendStr("failed to set mode\n");
        #endif // DEBUG
        at_backError;
        return;
    }
    setupAp(&esp);
    os_delay_us(10000);
    setupServer(&esp);
    at_backOk;
    system_restart();
}

void ICACHE_FLASH_ATTR
setupAp(esp_StoreType *espdata ){

    #ifdef DEBUG
            uart0_sendStr("setting mode\n");
    #endif // DEBUG
    //set mode
    if (at_setupCmdCwmodeEsp(espdata->cwmode)!=0){
        #ifdef DEBUG
                uart0_sendStr("failed to set mode\n");
        #endif // DEBUG
        at_backError;
        return;
    }

    #ifdef DEBUG
            uart0_sendStr("setting dhcp\n");
    #endif // DEBUG
    //set dhcp
    if (at_setupCmdCwdhcpEsp(espdata->dhcp_mode,espdata->dhcp_enable)!=0){
        #ifdef DEBUG
            uart0_sendStr("failed to set dhcp\n");
        #endif // DEBUG
        at_backError;
        return;
    }

    espdata->state=1;
    espdata->saved=1;
    #ifdef DEBUG
            char temp[50];
            os_sprintf(temp,"Saving parameters to memory  size:%d\n",sizeof(esp_StoreType));
            uart0_sendStr(temp);
    #endif // DEBUG
    user_esp_platform_save_param((uint32 *)espdata, sizeof(esp_StoreType));


    //set ssid,passwd
    #ifdef DEBUG
            uart0_sendStr("setting ssi\n");
        #endif // DEBUG
    struct softap_config apConfig;
    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);

    if (espdata->ssidlen>16){
        espdata->ssidlen=16;
    }
    if (espdata->passwdlen>16){
        espdata->passwdlen=16;
    }
    if (espdata->channel>11){
        espdata->channel=1;
    }
    if (espdata->wpa>3){
        espdata->wpa=0;
    }

    if (espdata->ssidlen>0){
        os_memcpy(apConfig.ssid,&espdata->ssid,espdata->ssidlen);
    }else{
        apConfig.ssid[0]='\0';
    }
    if (espdata->passwdlen>0){
        os_memcpy(apConfig.password,&espdata->passwd,espdata->passwdlen);
    }else{
        apConfig.password[0]='\0';
    }
    apConfig.channel=espdata->channel;
    apConfig.authmode=espdata->wpa;

    if (at_setupCmdCwsapEsp(&apConfig,espdata->passwdlen)!=0){
        #ifdef DEBUG
            uart0_sendStr("failed to set ssi\n");
        #endif // DEBUG
        at_backError;
        return;
    }
    #ifdef DEBUG
            uart0_sendStr("ssi set\n");
    #endif // DEBUG

    //system_restart();


}

void ICACHE_FLASH_ATTR
setupServer(esp_StoreType *espdata ){
    //set server mode
        #ifdef DEBUG
            uart0_sendStr("setting server mode\n");
        #endif // DEBUG
    if (at_setupCmdCipmuxEsp(espdata->cwmux)!=0)
    {
        #ifdef DEBUG
            uart0_sendStr("failed to set server mode\n");
        #endif // DEBUG
        at_backError;
        return;
    }
    #ifdef DEBUG
            uart0_sendStr("setting port\n");
        #endif // DEBUG
    if (at_setupCmdCipserverEsp(espdata->server_mode,espdata->port)!=0){
        #ifdef DEBUG
            uart0_sendStr("failed to set port\n");
        #endif // DEBUG
        at_backError;
        return;
    }
    //print the ip
    #ifdef DEBUG
            uart0_sendStr("printing ip as status\n");
    #endif // DEBUG
    at_exeCmdCifsr(CMD_CIFSR);
    //print status
    at_exeCmdCipstatus(CMD_CIPSTATUS);
}
