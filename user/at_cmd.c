/*
 * File	: at_cmd.c
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
#include "at_cmd.h"
#include "user_interface.h"
#include "osapi.h"
#include "at.h"
//#include<stdlib.h>

/** @defgroup AT_BASECMD_Functions
  * @{
  */

/**
  * @brief  Query and localization one commad.
  * @param  cmdLen: received length of command
  * @param  pCmd: point to received command
  * @retval the id of command
  *   @arg -1: failure
  */
static int16_t ICACHE_FLASH_ATTR
at_cmdSearch(uint8_t *pCmd)
{
  int16_t i;
    for(i=1; i<at_cmdNum; i++)
    {
      if (*pCmd==at_fun[i].at_cmdCode){
        return i;
      }
    }

  return -1;
}
//original function
/*
static int16_t ICACHE_FLASH_ATTR
at_cmdSearch(int8_t cmdLen, uint8_t *pCmd)
{
  int16_t i;

  if(cmdLen == 0)
  {
    return 0;
  }
  else if(cmdLen > 0)
  {
    for(i=1; i<at_cmdNum; i++)
    {
      if(cmdLen == at_fun[i].at_cmdLen)
      {
        if(os_memcmp(pCmd, at_fun[i].at_cmdName, cmdLen) == 0) //think add cmp len first
        {
          return i;
        }
      }
    }
  }
  return -1;
}
*/
/**
  * @brief  Get the length of commad.
  * @param  pCmd: point to received command
  * @retval the length of command
  *   @arg -1: failure
  */
  /*
static int8_t ICACHE_FLASH_ATTR
at_getCmdLen(uint8_t *pCmd)
{
  uint8_t n,i;

  n = 0;
  i = CMD_BUFFER_SIZE;

  while(i--)
  {
    if((*pCmd == '\n') || (*pCmd == '=') || (*pCmd == '?') || ((*pCmd >= '0')&&(*pCmd <= '9')))
    {
      return n;
    }
    else
    {
      pCmd++;
      n++;
    }
  }
  return -1;
}
*/

static int8_t ICACHE_FLASH_ATTR
at_checkCmdFormat(uint8_t *pCmd){
    int i=0;
    if (*pCmd!=CANWII_SOH){
        #ifdef DEBUG
        uart0_sendStr("1st not soh\n");
        #endif // DEBUG

        return 1;
    }
    //look for end of command
    for (i=0;i<CMD_BUFFER_SIZE;i++){
        if (*pCmd==CANWII_EOH){

            #ifdef DEBUG
            uart0_sendStr("found valid eoh\n");
            #endif // DEBUG
            break;
        }
        pCmd++;
    }
    //the position should be at least 2 0 to end of command, 1 to cmd, 2 to end of cmd
    if (i>1){
        return 0;
    }
    return 1;
}
/**
  * @brief  Distinguish commad and to execution.
            At this point we expect the following format:
            <CANWII_SOH><COMMAND><PARAM><CANWII_STR_SEP>  ... <CANWII_EOH>
  * @param  pAtRcvData: point to received (command)
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_cmdProcess(uint8_t *pAtRcvData)
{
  char tempStr[32];

  int16_t cmdId;
  int8_t cmdLen;
  uint16_t i;
  //os_sprintf(tempStr,"buffer:%s\n",pAtRcvData);

  if(at_checkCmdFormat(pAtRcvData)==0)
  {
    //point to command code
    pAtRcvData++;
    cmdId = at_cmdSearch(pAtRcvData);
    #ifdef DEBUG
        os_sprintf(tempStr,"valid format, cmd:%d\n",cmdId);
        uart0_sendStr(tempStr);
    #endif // DEBUG

  }
  else
  {

    #ifdef DEBUG
        uart0_sendStr("cmd invalid\n");
    #endif // DEBUG
  	at_backError;
  	return;
  }
  if(cmdId != -1)
  {
//    os_printf("cmd id: %d\n", cmdId);
    //pAtRcvData += cmdLen;
    pAtRcvData++;
    //found end of command. process the command. see at_cmd.h
    if(*pAtRcvData == CANWII_EOH)
    {
      #ifdef DEBUG
        uart0_sendStr("execute command\n");
    #endif // DEBUG

      if(at_fun[cmdId].at_exeCmd)
      {
        at_fun[cmdId].at_exeCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    //found query command. process it. see at_cmd.h
    else if(*pAtRcvData == CMD_QUERY && (pAtRcvData[1] ==CANWII_EOH))
    {

      #ifdef DEBUG
        uart0_sendStr("execute query\n");
    #endif // DEBUG
      if(at_fun[cmdId].at_queryCmd)
      {
        at_fun[cmdId].at_queryCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    //test function. include '=?<EOH>'
    else if((*pAtRcvData == CMD_EQUAL) && (pAtRcvData[1] == CMD_QUERY) && (pAtRcvData[2] == CANWII_EOH))
    {

      #ifdef DEBUG
        uart0_sendStr("execute test\n");
    #endif // DEBUG
      if(at_fun[cmdId].at_testCmd)
      {
        at_fun[cmdId].at_testCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    //function with parameter.
    else if((*pAtRcvData >= '0') && (*pAtRcvData <= '9') || (*pAtRcvData == CMD_EQUAL))
    {
    #ifdef DEBUG
        uart0_sendStr("execute set\n");
    #endif // DEBUG

      if(at_fun[cmdId].at_setupCmd)
      {
        at_fun[cmdId].at_setupCmd(cmdId, pAtRcvData);
      }
      else
      {
        at_backError;
      }
    }
    else
    {
      at_backError;
    }
  }
}
/*original function
void ICACHE_FLASH_ATTR
at_cmdProcess(uint8_t *pAtRcvData)
{
  char tempStr[32];

  int16_t cmdId;
  int8_t cmdLen;
  uint16_t i;

  cmdLen = at_getCmdLen(pAtRcvData);
  if(cmdLen != -1)
  {
    cmdId = at_cmdSearch(cmdLen, pAtRcvData);
  }
  else
  {
  	cmdId = -1;
  }
  if(cmdId != -1)
  {
//    os_printf("cmd id: %d\n", cmdId);
    pAtRcvData += cmdLen;
    if(*pAtRcvData == '\n')
    {
      if(at_fun[cmdId].at_exeCmd)
      {
        at_fun[cmdId].at_exeCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    else if(*pAtRcvData == '?' && (pAtRcvData[1] == '\n'))
    {
      if(at_fun[cmdId].at_queryCmd)
      {
        at_fun[cmdId].at_queryCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    else if((*pAtRcvData == '=') && (pAtRcvData[1] == '?') && (pAtRcvData[2] == '\n'))
    {
      if(at_fun[cmdId].at_testCmd)
      {
        at_fun[cmdId].at_testCmd(cmdId);
      }
      else
      {
        at_backError;
      }
    }
    else if((*pAtRcvData >= '0') && (*pAtRcvData <= '9') || (*pAtRcvData == '='))
    {
      if(at_fun[cmdId].at_setupCmd)
      {
        at_fun[cmdId].at_setupCmd(cmdId, pAtRcvData);
      }
      else
      {
        at_backError;
      }
    }
    else
    {
      at_backError;
    }
  }
  else
  {
  	at_backError;
  }
}
*/
/**
  * @}
  */
