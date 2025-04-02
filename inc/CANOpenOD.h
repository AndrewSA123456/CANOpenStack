#ifndef __CANOPENOD_H
#define __CANOPENOD_H

#include "globals.h"

enum
{
	INTERACTION_SUCCESSFUL = 0x0U,
	UNSUPPORTED_ACCESS_TO_AN_OBJECT = 0x06010000U, // запись в RO или чтение из WO
	INDEX_NOT_EXIST_IN_OD = 0x06020000U,			  // неправильный индекс
	SUBINDEX_NOT_EXIST_IN_OD = 0x06090011U,		  // неправильный субиндекс
	INVALID_VALUE_FOR_PARAMETER = 0x06090030U,
	HARDWARE_ERROR = 0x06060000U // проблема с внешней постоянной памятью
};

void initOD(void);
uint32_t readObjectOD(uint16_t index, uint8_t subindex, uint8_t *buff, uint16_t buffSize);
uint32_t writeObjectOD(uint16_t index, uint8_t subindex, uint8_t *buff, uint16_t buffSize);

#endif //__CANOPENOD_H
