#include "at_merg.h"

/**
  * @brief  Execution commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupMerg(uint8_t id,char *pPara )
{
  char temp[255];
  esp_StoreType esp;

  pPara++;
  if (setParamToEsp(pPara,id,&esp)==false){
    at_backError;
    return;
  }
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
    at_backOk;
    return;
    /*
    //set mode
    if (at_setupCmdCwmodeEsp(esp.cwmode)!=0){
        at_backError;
        return;
    }

    //set dhcp
    if (at_setupCmdCwdhcpEsp(esp.dhcp_mode,esp.dhcp_enable)!=0){
        at_backError;
        return;
    }
    //set ssid,passwd
    struct softap_config apConfig;
    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);
    if (esp.ssidlen>0){
        os_memcpy(apConfig.ssid,&esp.ssid,esp.ssidlen);
    }else{
        apConfig.ssid[0]='\0';
    }
    if (esp.passwdlen>0){
        os_memcpy(apConfig.password,&esp.passwd,esp.passwdlen);
    }else{
        apConfig.password[0]='\0';
    }
    apConfig.channel=esp.channel;
    apConfig.authmode=esp.wpa;

    if (at_setupCmdCwsapEsp(&apConfig,esp.passwdlen)!=0){
        at_backError;
        return;
    }
    //set server mode
    if (at_setupCmdCipmuxEsp(esp.cwmux)!=0)
    {
        at_backError;
        return;
    }

    if (at_setupCmdCipserverEsp(esp.server_mode,esp.port)!=0){
        at_backError;
        return;
    }
    */
}
