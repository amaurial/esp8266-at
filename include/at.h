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

#define at_recvTaskPrio        0
#define at_recvTaskQueueLen    64

#define at_procTaskPrio        1
#define at_procTaskQueueLen    1

#define at_backOk        uart0_sendStr("\nOK\n")
#define at_backError     uart0_sendStr("\nERROR\n")
#define at_backTeError   "+CTE ERROR: %d\n"

#define CMD_E 0x0a
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

#define RSP_CONNECTED
#define RSP_DISCONNECTED
#define RSP_OK
#define RSP_TCP_ERROR
#define ERR_IP_ERROR
#define RSP_NOAP
#define RSP_FAIL_CONNECT
#define RSP_MISS_PARAM_ERROR
#define RSP_CLOSED
#define RSP_SENT
#define RSP_DNS_FAIL
#define RSP_NOID_ERROR
#define RSP_LINK_TYPE_ERROR
#define RSP_BUSY_PROCESSING
#define RSP_BUSY_SENDING

#define SOH
#define EOH


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
	char *at_cmdName;
	int8_t at_cmdLen;
  void (*at_testCmd)(uint8_t id);
  void (*at_queryCmd)(uint8_t id);
  void (*at_setupCmd)(uint8_t id, char *pPara);
  void (*at_exeCmd)(uint8_t id);
}at_funcationType;

typedef struct
{
  uint32_t baud;
  uint32_t saved;
}at_uartType;

void at_init(void);
void at_cmdProcess(uint8_t *pAtRcvData);

#endif
