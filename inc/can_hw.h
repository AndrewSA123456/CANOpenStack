#ifndef __CAN_HW_H
#define __CAN_HW_H

#include "globals.h"

typedef struct
{
	uint32_t id;
	uint8_t len;
	uint8_t data[8];
} canFrame_t;
enum
{
	CAN_SUCCESS,
	CAN_ERROR
};
/////////////////////////////////////////////////////////////////////
// Функция: инициализация CAN
int initCAN(void);
/////////////////////////////////////////////////////////////////////
// Функция: деинициализация CAN
int deinitCAN(void);
/////////////////////////////////////////////
// Функция: установить фильтр CAN
void setCANfilter(uint32_t canID);
/////////////////////////////////////////////
// Функция: принять сообщение по CAN
int receiveCAN();
/////////////////////////////////////////////
// Функция: передать сообщение по CAN
int transmitCAN(canFrame_t CANframe);

#define CAN_RECEIVE_BUFF_SIZE 22

typedef struct
{
	canFrame_t frame[CAN_RECEIVE_BUFF_SIZE];
	uint32_t head;
	uint32_t tail;
	#ifdef LINUX
	GMutex mutex;
	#endif
} CANRingBuff;

extern CANRingBuff ReceiveCANRingBuff;

bool isCANRingBuffEmpty(CANRingBuff *buff);
bool isCANRingBuffFull(CANRingBuff *buff);
bool pushCANRingBuff(CANRingBuff *buff, canFrame_t frame);
bool popCANRingBuff(CANRingBuff *buff, canFrame_t *frame);
void clearCANRingBuff(CANRingBuff *buff);
#endif //__CAN_HW_H
