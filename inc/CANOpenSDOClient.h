#ifndef __CANOPEN_SDO_CLIENT_H
#define __CANOPEN_SDO_CLIENT_H

#include "globals.h"
#include "can_hw.h"

void CO_sdo_handler(canFrame_t *canRequestMsg);

#endif //__CANOPEN_SDO_CLIENT_H
