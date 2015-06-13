/*
 * File	: at_port.c
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
#include "user_interface.h"
#include "osapi.h"
#include "driver/uart.h"

/** @defgroup AT_PORT_Defines
  * @{
  */
#define at_cmdLenMax 128
#define at_dataLenMax 2048
/**
  * @}
  */

/** @defgroup AT_PORT_Extern_Variables
  * @{
  */
extern uint16_t at_sendLen;
extern uint16_t at_tranLen;
extern os_timer_t at_delayCheck;
extern uint8_t ipDataSendFlag;
/**
  * @}
  */

/** @defgroup AT_PORT_Extern_Functions
  * @{
  */
extern void at_ipDataSending(uint8_t *pAtRcvData);
extern void at_ipDataSendNow(void);
/**
  * @}
  */

os_event_t    at_recvTaskQueue[at_recvTaskQueueLen];
os_event_t    at_procTaskQueue[at_procTaskQueueLen];

BOOL specialAtState = TRUE;
at_stateType  at_state;
uint8_t *pDataLine;
BOOL echoFlag = TRUE;

static uint8_t at_cmdLine[CMD_BUFFER_SIZE];
uint8_t at_dataLine[at_dataLenMax];
/** @defgroup AT_PORT_Functions
  * @{
  */

static void at_procTask(os_event_t *events);
static void at_recvTask(os_event_t *events);

/**
  * @brief  Uart receive task.
            Command format:
            <CANWII_SOH><COMMAND><PARAM><CANWII_STR_SEP>  ... <CANWII_EOH>
  * @param  events: contain the uart receive data
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_recvTask(os_event_t *events)
{
  static uint8_t *pCmdLine;
  uint8_t temp;
  char intChar[10];

  //read byte per byte
  while(READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
  {

    WRITE_PERI_REG(0X60000914, 0x73);

    if(at_state != at_statIpTraning)
    {
      //read the byte from queue
      temp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
      //do the echo function
      if(echoFlag)
      {
        uart_tx_one_char(temp);
      }
    }


    switch(at_state)
    {
    case at_statIdle: //search SOH head

      if(temp==CANWII_SOH)
      {
        #ifdef DEBUG
        uart0_sendStr("start of rec\n");
        #endif // DEBUG
        at_state = at_statRecving;
        //pointer to the buffer
        pCmdLine = at_cmdLine;
        *pCmdLine=temp;
        pCmdLine++;
      }
      else if(temp == CANWII_EOH) //only get end of parameter
      {
        at_backError;
      }
      else{
        at_backError;
      }
      break;

    case at_statRecving: //push receive data to cmd line
      //put in the buffer
      *pCmdLine = temp;
      //look for the end of instruction

      #ifdef DEBUG
        uart0_sendStr("receiving\n");
      #endif // DEBUG
      if(temp == CANWII_EOH)
      {
        #ifdef DEBUG
        uart0_sendStr("found eoh\n");
        #endif // DEBUG

        //change the task priority
        system_os_post(at_procTaskPrio, 0, 0);
        pCmdLine++;
        *pCmdLine = '\0';
        //start to process the received command
        //the other task at_procTask start to process the buffer
        at_state = at_statProcess;
        //TODO:
        //confirm command received?
        if(echoFlag)
        {
          uart_tx_one_char(temp);
          uart0_sendStr("\n");
        }
      }
      //check the array boundary by comparing the ponter address
      else if(pCmdLine >= &at_cmdLine[at_cmdLenMax - 1])
      {
        at_state = at_statIdle;
      }
      //move the buffer pointer
      pCmdLine++;
      break;

    case at_statProcess: //process data
      if(temp == CANWII_EOH)
      {
        uart0_sendStr("\nbusy p...\n");
        //uart_tx_one_char(CANWII_ERR_BUSY);

      }
      break;

    case at_statIpSending:
      *pDataLine = temp;
      if((pDataLine >= &at_dataLine[at_sendLen - 1]) || (pDataLine >= &at_dataLine[at_dataLenMax - 1]))
      {
        system_os_post(at_procTaskPrio, 0, 0);
        at_state = at_statIpSended;
      }
      else
      {
        pDataLine++;
      }

      break;

    case at_statIpSended: //send data
      if(temp == CANWII_EOH)
      {
        uart0_sendStr("busy s...\n");
        //uart_tx_one_char(CANWII_ERR_BUSY);
      }
      break;
    //TODO:
    //what is that?
    case at_statIpTraning:
      os_timer_disarm(&at_delayCheck);

      if(pDataLine > &at_dataLine[at_dataLenMax - 1])
      {
        os_timer_arm(&at_delayCheck, 0, 0);
        os_printf("exceed\n");
        return;
      }
      else if(pDataLine == &at_dataLine[at_dataLenMax - 1])
      {
        temp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
        *pDataLine = temp;
        pDataLine++;
        at_tranLen++;
        os_timer_arm(&at_delayCheck, 0, 0);
        return;
      }
      else
      {
        temp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
        *pDataLine = temp;
        pDataLine++;
        at_tranLen++;
        os_timer_arm(&at_delayCheck, 20, 0);
      }
      break;
    default:
      if(temp ==CANWII_EOH)
      {
      }
      break;
    }
  }
  if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
  {
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
  }
  else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST))
  {
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
  }
  ETS_UART_INTR_ENABLE();
}

/**
  * @brief  Task of process command or txdata.
  * @param  events: no used
  * @retval None
  */
static void ICACHE_FLASH_ATTR
at_procTask(os_event_t *events)
{
  if(at_state == at_statProcess)
  {
    #ifdef DEBUG
        uart0_sendStr("process cmd\n");
    #endif // DEBUG

    at_cmdProcess(at_cmdLine);
    if(specialAtState)
    {
      at_state = at_statIdle;
    }
  }
  else if(at_state == at_statIpSended)
  {

    #ifdef DEBUG
        uart0_sendStr("sending ip data\n");
    #endif // DEBUG
    at_ipDataSending(at_dataLine);
    if(specialAtState)
    {
      at_state = at_statIdle;
    }
  }
  else if(at_state == at_statIpTraning)
  {
     #ifdef DEBUG
        uart0_sendStr("sending ip data now\n");
    #endif // DEBUG

    at_ipDataSendNow();
  }
}


/**
  * @brief  Initializes build two tasks.
  * @param  None
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_init(void)
{
  //set the tasks for the OS
  system_os_task(at_recvTask, at_recvTaskPrio, at_recvTaskQueue, at_recvTaskQueueLen);
  system_os_task(at_procTask, at_procTaskPrio, at_procTaskQueue, at_procTaskQueueLen);
}

/**
  * @}
  */
