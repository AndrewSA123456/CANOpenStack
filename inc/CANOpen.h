#ifndef __CANOPEN_H
#define __CANOPEN_H

#include "globals.h"

// Структура данных стандартного сообщения CAN
typedef struct
{
	uint16_t id;		// Идентификатор сообщения
	uint8_t len;		// Размер данных в байтах (0-8)
	uint8_t buff[8];		// Буфер
} CAN_MSG;

#endif //__CANOPEN_H
