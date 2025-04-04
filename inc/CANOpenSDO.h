#ifndef __CANOPENSDO_H
#define __CANOPENSDO_H

#include "globals.h"

typedef struct
{
	uint32_t id;
	uint8_t len;
	uint8_t data[8];
} canFrame_t;
void CO_sdo_handler(canFrame_t *canMsg);

#endif //__CANOPENSDO_H
