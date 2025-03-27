#ifndef __SOCKETCAN_H
#define __SOCKETCAN_H
#include "globals.h"

enum
{
	SOCKET_CAN_ERROR = -1,
	SOCKET_CAN_OK = 0,
	SOCKET_CAN_NO_NEW_MSG = 1,
	SOCKET_CAN_NEW_MSG = 2,

};
/////////////////////////////////////////////
// Функция: открыть сокет CAN
int openSocketCAN(const char *interface_name);
/////////////////////////////////////////////
// Функция: закрыть сокет CAN
int closeSocketCAN(int sockDesc);
/////////////////////////////////////////////
// Функция: инициализация фильтров CAN
void setFilterSocketCAN(int sockDesc,
						struct can_filter *rfilter,
						uint16_t rfilter_size);
/////////////////////////////////////////////
// Функция: принять сообщение по CAN
// в блокирующем режиме
int socketCANReceive(int sockDesc,
	struct can_frame *CANframe);
/////////////////////////////////////////////
// Функция: принять сообщение по CAN
// в неблокирующем режиме, используя poll
int socketCANReceiveNotBlocking(int sockDesc,
			   struct can_frame *CANframe,
			   int mStimeout);
/////////////////////////////////////////////
// Функция: передать сообщение по CAN
int socketCANTransmit(int sockDesc,
					  uint32_t message_id,
					  uint8_t message_len,
					  uint8_t *message_data);
#endif //__SOCKETCAN_H