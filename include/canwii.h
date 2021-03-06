#ifndef CANWII_H_INCLUDED
#define CANWII_H_INCLUDED


#define CANWII_SOH 0x01
#define CANWII_EOH 0x04
#define CANWII_STR_SEP ','
#define CANWII_VERSION 0x01
#define CANWII_ON 0x01
#define CANWII_OFF 0x00
#define CANWII_OK 0x02
#define CANWII_ERR 0x03
#define CANWII_TE_ERR
#define CANWII_TCP 1
#define CANWII_UDP 0
//Merg OPCS
//after this value the esp expect:
//ssid,ssid_passwd,cwmode,cwmux,port,timeout
#define CANWII_CONFIGESP 0x01
#define CANWII_START_TCP_SESSION 0x02
#define CANWII_END_TCP_SESSION 0x03
#define CANWII_START_UDP_SESSION 0x02
#define CANWII_END_UDP_SESSION 0x03
#define CANWII_KEEP_ALIVE 0x04
#define CANWII_DATA_FROM_ESP 0x05
#define CANWII_DATA_TO_ESP 0x06
#define CANWII_END_OF_SESSION 0x07
#define CANWII_SET_BAUD_RATE 0x08
#define CANWII_SET_DHCP 0x09

//Error codes
#define CANWII_ERR_FAIL_CONFIG
#define CANWII_ERR_BUSY


#endif // CANWII_H_INCLUDED

