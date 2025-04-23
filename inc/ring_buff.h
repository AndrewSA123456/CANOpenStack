#ifndef __RING_BUFF_H
#define __RING_BUFF_H

#include "globals.h"

#define RING_BUFF_SIZE 22

typedef struct
{
	uint8_t data[RING_BUFF_SIZE];
	uint32_t head;
	uint32_t tail;
} RingBuff;

bool isBuffEmpty(RingBuff *buff);
bool isBuffFull(RingBuff *buff);
bool pushBuff(RingBuff *buff, uint8_t data);
bool popBuff(RingBuff *buff, uint8_t *data);
void clearBuff(RingBuff *buff);

#endif //__RING_BUFF_H
