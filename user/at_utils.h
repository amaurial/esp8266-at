#ifndef __AT_UTILS_H
#define __AT_UTILS_H


#include "at.h"
#include <stdlib.h>
#include "osapi.h"
#include "c_types.h"

uint8_t at_dataStrCpyWithDelim(void *pDest, const void *pSrc, int8_t maxLen,char delim);
bool setParamToEsp(char *param,uint8_t cmdid,esp_StoreType *espdata);
struct_MSGType generalMSG;
void sendGeneralMsg(struct_MSGType msgtype);
void logMessage(char *msg);

#endif
