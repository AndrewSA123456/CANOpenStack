#ifndef __CANOPEN_H
#define __CANOPEN_H

#include "globals.h"
#include "can_hw.h"

typedef enum
{
	OD_INVALID,
	OD_BOOLEAN = 0x1,
	OD_INTEGER8 = 0x2,
	OD_INTEGER16 = 0x3,
	OD_INTEGER32 = 0x4,
	OD_UNSIGNED8 = 0x5,
	OD_UNSIGNED16 = 0x6,
	OD_UNSIGNED32 = 0x7,
	OD_REAL32 = 0x8,
	OD_VISIBLE_STRING = 0x9, // массив байт ограничим 16 байтами, таблица ASCII не нужна, заранее предопределенный массив
	OD_DOMAIN = 0xF			 // кольцевой буфер 21 байт, только для приема прошивки, доступен только в режиме bootloader
} dataTypeOD_t;

#define WAIT_COUNT 1000
#define BROADCAST_ID 0
#define NODE_ID 5 // 1..127
#define NMT_ID 0
#define SYNC_ID 0x080
#define EMCY_ID(nodeID) ((uint32_t)(0x080 + nodeID))
#define TIME_STAMP_ID (0x100)
#define TPDO1_ID(nodeID) ((uint32_t)(0x180 + nodeID))
#define RPDO1_ID(nodeID) ((uint32_t)(0x200 + nodeID))
#define TPDO2_ID(nodeID) ((uint32_t)(0x280 + nodeID))
#define RPDO2_ID(nodeID) ((uint32_t)(0x300 + nodeID))
#define TPDO3_ID(nodeID) ((uint32_t)(0x380 + nodeID))
#define RPDO3_ID(nodeID) ((uint32_t)(0x400 + nodeID))
#define TPDO4_ID(nodeID) ((uint32_t)(0x480 + nodeID))
#define RPDO4_ID(nodeID) ((uint32_t)(0x500 + nodeID))
#define SDO_REQUEST_ID(nodeID) ((uint32_t)(0x600 + nodeID))
#define SDO_RESPONSE_ID(nodeID) ((uint32_t)(0x580 + nodeID))
#define NMT_ERR_CTRL_ID(nodeID) ((uint32_t)(0x700 + nodeID)) // HEARTBEAT
#define SDO_REQUEST_ID(nodeID) ((uint32_t)(0x600 + nodeID))

#define SDO_CS_Pos (5)
#define SDO_CS_Msk ((uint8_t)(0x7 << SDO_CS_Pos))

#define SDO_CS_ABORT ((uint8_t)0x4 << SDO_CS_Pos)

#define SDO_CCS_DL_INIT ((uint8_t)(0x1 << SDO_CS_Pos))
#define SDO_CCS_DL_SEG ((uint8_t)(0x0 << SDO_CS_Pos))
#define SDO_CCS_UL_INIT ((uint8_t)(0x2 << SDO_CS_Pos))
#define SDO_CCS_UL_SEG ((uint8_t)(0x3 << SDO_CS_Pos))

#define SDO_SCS_DL_INIT ((uint8_t)(0x3 << SDO_CS_Pos))
#define SDO_SCS_DL_SEG ((uint8_t)(0x20))
#define SDO_SCS_DL_SEG_TOGGLED ((uint8_t)(0x30))
#define SDO_SCS_UL_INIT ((uint8_t)(0x2 << SDO_CS_Pos))
#define SDO_SCS_UL_SEG ((uint8_t)(0x0))
#define SDO_SCS_UL_SEG_TOGGLED ((uint8_t)(0x10))

#define SDO_E_Pos (1)
#define SDO_E_Msk ((uint8_t)(0x1 << SDO_E_Pos))

#define SDO_S_Pos (0)
#define SDO_S_Msk ((uint8_t)(0x1 << SDO_S_Pos))

#define SDO_N_INIT_Pos (2)
#define SDO_N_INIT_Msk ((uint8_t)(0x3 << SDO_N_INIT_Pos))
#define SDO_N_INIT_SIZE_DATA(byte) (4 - ((byte & SDO_N_INIT_Msk) >> SDO_N_INIT_Pos))

#define SDO_N_Pos (1)
#define SDO_N_Msk ((uint8_t)(0x7 << SDO_N_Pos))
#define SDO_N_SIZE_DATA(byte) (7 - ((byte & SDO_N_Msk) >> SDO_N_Pos))

#define SDO_T_Pos (4)
#define SDO_T_Msk ((uint8_t)(0x1 << SDO_T_Pos))

#define SDO_C_Pos (0)
#define SDO_C_Msk ((uint8_t)(0x1 << SDO_C_Pos))

#define ABORT_SDO_INVALID_SIZE_DATA (0x06070010U)

typedef enum
{
	// 050x errors
	CO_ERR_0503_0000 = 0x05030000, // Toggle bit not alternated
	CO_ERR_0504_0000 = 0x05040000, // SDO protocol timed out
	CO_ERR_0504_0001 = 0x05040001, // Client/server command specifier not valid or unknown
	CO_ERR_0504_0002 = 0x05040002, // Invalid block size (block mode only)
	CO_ERR_0504_0003 = 0x05040003, // Invalid sequence number (block mode only)
	CO_ERR_0504_0004 = 0x05040004, // CRC error (block mode only)
	CO_ERR_0504_0005 = 0x05040005, // Out of memory

	// 0601 errors - Access errors
	CO_ERR_0601_0000 = 0x06010000, // Unsupported access to an object
	CO_ERR_0601_0001 = 0x06010001, // Attempt to read a write only object
	CO_ERR_0601_0002 = 0x06010002, // Attempt to write a read only object

	// 0602 errors
	CO_ERR_0602_0000 = 0x06020000, // Object does not exist in the object dictionary

	// 0604 errors - PDO mapping errors
	CO_ERR_0604_0041 = 0x06040041, // Object cannot be mapped to the PDO
	CO_ERR_0604_0042 = 0x06040042, // The number and length of the objects to be mapped would exceed PDO length
	CO_ERR_0604_0043 = 0x06040043, // General parameter incompatibility reason
	CO_ERR_0604_0047 = 0x06040047, // General internal incompatibility in the device

	// 0606 errors
	CO_ERR_0606_0000 = 0x06060000, // Access failed due to an hardware error

	// 0607 errors - Data type mismatch
	CO_ERR_0607_0010 = 0x06070010, // Data type does not match, length of service parameter does not match
	CO_ERR_0607_0012 = 0x06070012, // Data type does not match, length of service parameter too high
	CO_ERR_0607_0013 = 0x06070013, // Data type does not match, length of service parameter too low

	// 0609 errors - Parameter errors
	CO_ERR_0609_0011 = 0x06090011, // Sub-index does not exist
	CO_ERR_0609_0030 = 0x06090030, // Invalid value for parameter (download only)
	CO_ERR_0609_0031 = 0x06090031, // Value of parameter written too high (download only)
	CO_ERR_0609_0032 = 0x06090032, // Value of parameter written too low (download only)
	CO_ERR_0609_0036 = 0x06090036, // Maximum value is less than minimum value

	// 060A errors
	CO_ERR_060A_0023 = 0x060A0023, // Resource not available: SDO connection

	// 0800 errors - General errors
	CO_ERR_0800_0000 = 0x08000000, // General error
	CO_ERR_0800_0020 = 0x08000020, // Data cannot be transferred or stored to the application
	CO_ERR_0800_0021 = 0x08000021, // Data cannot be transferred or stored to the application because of local control
	CO_ERR_0800_0022 = 0x08000022, // Data cannot be transferred or stored to the application because of the present device state
	CO_ERR_0800_0023 = 0x08000023, // Object dictionary dynamic generation fails or no object dictionary is present
	CO_ERR_0800_0024 = 0x08000024  // No data available
} CO_ErrorCodes;

void CO_process(void);

#endif //__CANOPEN_H
