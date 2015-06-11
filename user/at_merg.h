#ifndef __AT_MERG_H
#define __AT_MERG_H

#include "at.h"
#include "canwii.h"
#include "at_wifiCmd.h"
#include "at_ipCmd.h"
#include "at_baseCmd.h"
#include "user_interface.h"
#include "osapi.h"
#include <stdlib.h>



void at_setupMerg(uint8_t id,char *pPara );
void setupServer(esp_StoreType *espdata );
void setupAp(esp_StoreType *espdata);

#endif
