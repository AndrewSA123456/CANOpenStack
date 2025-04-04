#ifndef __CANOPEN_H
#define __CANOPEN_H

#include "globals.h"


#define BROADCAST_ID 0
#define NODE_ID  4 // 1..127
#define NMT_ID 0
#define SYNC_ID 0x080
#define EMCY_ID (0x080 + NODE_ID)
#define TIME_STAMP_ID (0x100)
#define TPDO1_ID (0x180 + NODE_ID)
#define RPDO1_ID (0x200 + NODE_ID)
#define TPDO2_ID (0x280 + NODE_ID)
#define RPDO2_ID (0x300 + NODE_ID)
#define TPDO3_ID (0x380 + NODE_ID)
#define RPDO3_ID (0x400 + NODE_ID)
#define TPDO4_ID (0x480 + NODE_ID)
#define RPDO4_ID (0x500 + NODE_ID)
#define SDO_ID_REQUEST (0x600 + NODE_ID)
#define SDO_ID_RESPONSE (0x580 + NODE_ID)
#define NMT_ERR_CTRL_ID (0x700 + NODE_ID) //HEARTBEAT

void CO_process(void);

#endif //__CANOPEN_H
