#include "at_cmd.h"
#include "user_interface.h"
#include "osapi.h"
#include "at.h"

/**
  * @brief  Execution commad of get link status.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupMerg(uint8_t id,uint8_t *pPara )
{
  at_uartType tempUart;

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
