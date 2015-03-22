/*
 * File	: at.h
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
#ifndef __AT_H
#define __AT_H

#include "c_types.h"
#include "canwii.h"

#define at_recvTaskPrio        0
#define at_recvTaskQueueLen    64

#define CMD_BUFFER_SIZE 128


#define at_procTaskPrio        1
#define at_procTaskQueueLen    1

#define at_backOk        uart0_sendStr("\nOK\n")
#define at_backError     uart0_sendStr("\nERROR\n")
#define at_backTeError   "+CTE ERROR: %d\n"

#define CMD_AT 0x0a
#define CMD_RST 0x0b
#define CMD_GMR 0x0c
#define CMD_GSLP 0x0d
#define CMD_IPR 0x0e
#define CMD_CWMODE 0x0f
#define CMD_CWJAP 0x10
#define CMD_CWLAP 0x11
#define CMD_CWQAP 0x12
#define CMD_CWSAP 0x13
#define CMD_CWLIF 0x14
#define CMD_CWDHCP 0x15
#define CMD_CIFSR 0x16
#define CMD_CIPSTAMAC 0x17
#define CMD_CIPAPMAC 0x18
#define CMD_CIPSTA 0x19
#define CMD_CIPAP 0x1a
#define CMD_CIPSTATUS 0x1b
#define CMD_CIPSTART 0x1c
#define CMD_CIPCLOSE 0x1d
#define CMD_CIPSEND 0x1e
#define CMD_CIPMUX 0x1f
#define CMD_CIPSERVER 0x20
#define CMD_CIPMODE 0x21
#define CMD_CIPSTO 0x22
#define CMD_CIUPDATE 0x23
#define CMD_CIPING 0x24
#define CMD_CIPAPPUP 0x25
#define CMD_ATE 0x26
#define CMD_MPINFO 0x27

#define CMD_QUERY '?'
#define CMD_EQUAL '='

#define RSP_CONNECTED 0xA1
#define RSP_DISCONNECTED 0xA2
#define RSP_OK 0xA3
#define RSP_TCP_ERROR 0xA4
#define ERR_IP_ERROR 0xA5
#define RSP_NOAP 0xA6
#define RSP_FAIL_CONNECT 0xA7
#define RSP_MISS_PARAM_ERROR 0xA8
#define RSP_CLOSED 0xA9
#define RSP_SENT 0xAA
#define RSP_DNS_FAIL 0xAB
#define RSP_NOID_ERROR 0xAC
#define RSP_LINK_TYPE_ERROR 0xAD
#define RSP_BUSY_PROCESSING 0xAE
#define RSP_BUSY_SENDING 0xAF



typedef enum{
  at_statIdle,
  at_statRecving,
  at_statProcess,
  at_statIpSending,
  at_statIpSended,
  at_statIpTraning
}at_stateType;

typedef enum{
  m_init,
  m_wact,
  m_gotip,
  m_linked,
  m_unlink,
  m_wdact
}at_mdStateType;

typedef struct
{
	char *at_cmdName;//command name
	int8_t at_cmdLen;//command size
	int8_t at_cmdCode;//command hexa code
  void (*at_testCmd)(uint8_t id);//test function
  void (*at_queryCmd)(uint8_t id);//query function
  void (*at_setupCmd)(uint8_t id, char *pPara);//setup function
  void (*at_exeCmd)(uint8_t id);//execute function
}at_funcationType;

typedef struct
{
  uint32_t baud;
  uint32_t saved;
}at_uartType;

void at_init(void);
void at_cmdProcess(uint8_t *pAtRcvData);

#endif
