#include "CANOpenOD.h"
typedef enum
{
	OD_VAR = 0x7,
	OD_ARRAY = 0x8,
	OD_RECORD = 0x9
} objCode_t;

typedef enum
{
	OD_RW = 0x1,
	OD_WO,
	OD_RO,
	OD_CONST
} accessUsage_t;

typedef enum
{
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

typedef enum
{
	OD_RAM,
	OD_ROM
} placement_t;

typedef struct
{
	uint8_t subindex;
	accessUsage_t access;
	bool mappingPDO;
	dataTypeOD_t dataType;
	uint8_t *data;
} entryOD_t;

typedef struct
{
	uint16_t index;
	objCode_t objCode;
	placement_t placement;
	uint8_t subindexNum;
	entryOD_t *entry;
} objectOD_t;

typedef union
{
	uint8_t arr[4];
	uint32_t var;
}UNSIGNED32;
typedef union
{
	uint8_t arr[2];
	uint16_t var;
}UNSIGNED16;

//-----------------------------------------------------------------
// Object 1000h: Device type
// 	OBJECT DESCRIPTION
// 		Index 1000h
// 			Name: Device type
// 			Object code: VAR
// 			Data type: UNSIGNED32
// 			Category: Mandatory
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Access: ro
// 			PDO mapping: No
// 			Value range: See value definition
// 			Default value: Profile- or manufacturer-specific
//-----------------------------------------------------------------
static UNSIGNED32 deviceType = {0};
static entryOD_t deviceTypeEntry = {
	.subindex = 0,
	.access = OD_RO,
	.mappingPDO = false,
	.dataType = OD_UNSIGNED32,
	.data = deviceType.arr};
static objectOD_t deviceTypeObj = {
	.index = 0x1000,
	.objCode = OD_VAR,
	.placement = OD_RAM,
	.subindexNum = 1,
	.entry = &deviceTypeEntry};
//-----------------------------------------------------------------
// Object 1001h: Error register
// 	OBJECT DESCRIPTION
// 		Index 1001h
// 			Name: Error register
// 			Object code: VAR
// 			Data type: UNSIGNED8
// 			Category: Mandatory
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Access: ro
// 			PDO mapping: Optional
// 			Value range: See value definition
// 			Default value: No
//-----------------------------------------------------------------
static uint8_t errorRegisterData = 0;
static entryOD_t errorRegisterEntry = {
	.subindex = 0,
	.access = OD_RO,
	.mappingPDO = false,
	.dataType = OD_UNSIGNED8,
	.data = &errorRegisterData};
static objectOD_t errorRegisterObj = {
	.index = 0x1001,
	.objCode = OD_VAR,
	.placement = OD_RAM,
	.subindexNum = 1,
	.entry = &errorRegisterEntry};
//-----------------------------------------------------------------
// Object 1008h: Manufacturer device name
// 	OBJECT DESCRIPTION
// 		Index 1008h
// 			Name :Manufacturer device name
// 			Object code: VAR
// 			Data type: VISIBLE_STRING
// 			Category: Optional
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Access: const
// 			PDO mapping: No
// 			Value range: VISIBLE_STRING
// 			Default value: Manufacturer-specific
//-----------------------------------------------------------------
static uint8_t manufacturerDeviceNameData[16] = {0};
static entryOD_t manufacturerDeviceNameEntry = {
	.subindex = 0,
	.access = OD_CONST,
	.mappingPDO = false,
	.dataType = OD_VISIBLE_STRING,
	.data = manufacturerDeviceNameData};
static objectOD_t manufacturerDeviceNameObj = {
	.index = 0x1008,
	.objCode = OD_VAR,
	.placement = OD_RAM,
	.subindexNum = 1,
	.entry = &manufacturerDeviceNameEntry};
//-----------------------------------------------------------------
// Object 1016h: Consumer heartbeat time
// 	OBJECT DESCRIPTION
// 		Index 1016h
// 			Name: Consumer heartbeat time
// 			Object code: ARRAY
// 			Data type: UNSIGNED32
// 			Category: Optional
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Description: Highest sub-index supported
// 			Entry category: Mandatory
// 			Access: const
// 			PDO mapping: No
// 			Value range: 01h to 7Fh
// 			Default: value profile- or manufacturer-specific
// 		Sub-index 01h
// 			Description: Consumer heartbeat time
// 			Entry category: Mandatory
// 			Access: rw
// 			PDO mapping: No
// 			Value range: UNSIGNED32 (Figure 62)
// 			Default value: 0000 0000h
// 		Sub-index 02h to 7Fh
// 			Description: Consumer heartbeat time
// 			Entry category: Optional
// 			Access: rw
// 			PDO mapping: No
// 			Value range: UNSIGNED32 (Figure 62)
// 			Default value: 0000 0000h
//-----------------------------------------------------------------
static uint8_t consumerHeartbeatTimeHighestSubIndex = 1;
static UNSIGNED32 consumerHeartbeatTime = {0};
static entryOD_t consumerHeartbeatTimeEntry[2] = {
	{.subindex = 0,
	 .access = OD_CONST,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED8,
	 .data = &consumerHeartbeatTimeHighestSubIndex},
	{.subindex = 1,
	 .access = OD_RW,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = consumerHeartbeatTime.arr}};
static objectOD_t consumerHeartbeatTimeObj = {
	.index = 0x1016,
	.objCode = OD_ARRAY,
	.placement = OD_RAM,
	.subindexNum = 2,
	.entry = consumerHeartbeatTimeEntry};
//-----------------------------------------------------------------
// Object 1017h: Producer heartbeat time
// 	OBJECT DESCRIPTION
// 		Index 1017h
// 			Name: Producer heartbeat time
// 			Object code: VAR
// 			Data type: UNSIGNED16
// 			Category: Mandatory
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Access: rw
// 			PDO mapping: No
// 			Value range: UNSIGNED16
// 			Default value: 0 or profile-specific
//-----------------------------------------------------------------
static UNSIGNED16 producerHeartbeatTime = {0};
static entryOD_t producerHeartbeatTimeEntry = {
	.subindex = 0,
	.access = OD_RW,
	.mappingPDO = false,
	.dataType = OD_UNSIGNED16,
	.data = producerHeartbeatTime.arr};
static objectOD_t producerHeartbeatTimeObj = {
	.index = 0x1017,
	.objCode = OD_VAR,
	.placement = OD_RAM,
	.subindexNum = 1,
	.entry = &producerHeartbeatTimeEntry};
//-----------------------------------------------------------------
// Object 1018h: Identity object
// 	OBJECT DESCRIPTION
// 		Index 1018h
// 			Name: Identity object
// 			Object code: RECORD
// 			Data type: Identity
// 			Category: Mandatory
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Description: Highest sub-index supported
// 			Entry category: Mandatory
// 			Access: const
// 			PDO mapping: No
// 			Value range: 04h
// 			Default value: profile- or manufacturer-specific
// 		Sub-index 01h
// 			Description: Vendor-ID
// 			Entry category: Mandatory
// 			Access: ro
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 			Default value: Assigned uniquely to manufacturers by CiA
// 		Sub-index 02h
// 			Description: Product code
// 			Entry category: Optional
// 			Access: ro
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 			Default value: Profile- or manufacturer-specific
// 		Sub-index 03h
// 			Description: Revision number
// 			Entry category: Optional
// 			Access: ro
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 			Default value: Profile- or manufacturer-specific
// 		Sub-index 04h
// 			Description: Serial number
// 			Entry category: Optional
// 			Access: ro
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 			Default value: Profile- or manufacturer-specific
//-----------------------------------------------------------------
static uint8_t identityObjectDataHighestSubIndex = 4;
static UNSIGNED32 identityObject_1 = {0};
static UNSIGNED32 identityObject_2 = {0};
static UNSIGNED32 identityObject_3 = {0};
static UNSIGNED32 identityObject_4 = {0};
static entryOD_t identityObjectEntry[5] = {

	{.subindex = 0,
	 .access = OD_CONST,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED8,
	 .data = &identityObjectDataHighestSubIndex},

	{.subindex = 1,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = identityObject_1.arr},

	{.subindex = 2,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = identityObject_2.arr},

	{.subindex = 3,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = identityObject_3.arr},

	{.subindex = 4,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = identityObject_4.arr}};

static objectOD_t identityObjectObj = {
	.index = 0x1018,
	.objCode = OD_RECORD,
	.placement = OD_RAM,
	.subindexNum = 5,
	.entry = identityObjectEntry};
//-----------------------------------------------------------------
// Object 1F80h NMT Startup
// 	OBJECT DESCRIPTION
// 		Index 1F80h
// 			Name: NMT Startup
// 			Object code: VAR
// 			Data type: UNSIGNED32
// 			Category: Optional
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Access rw
// 			PDO mapping No
// 			Value range UNSIGNED32
// 			Default value 0
// // Nodes that autostart (go to operational without waiting for the
// // NMT startup message) should report 0 in this entry.
// The NMT Master provides services for controlling the network behaviour of nodes as defined in
// DS-301. Only one NMT Master can exist in a CANopen Network. Since there may be several
// devices that are able to perform the task of an NMT Master, it is necessary to configure this
// functionality.
// Bit 0
// 	= 0 Device is NOT the NMT Master. The objects of the Network List
// 		have to be ignored. All other bits have to be ignored. Exceptions
// 		for Autostart 0001 000b, 0000010b (see below).
// 	= 1 Device is the NMT Master.
// Bit 1
// 	= 0 Start only explicitly assigned slaves (if Bit 3 = 0).
// 	= 1 After boot-up perform the service NMT Start Remote Node All Nodes (if Bit 3 = 0)
// Bit 2
// 	= 0 Enter myself automatically Operational
// 	= 1 Do not enter myself Operational automatically. Application will
// 		decide, when to enter the state Operational.
// Bit 3
// 	= 0 Allow to start up the slaves (i.e. to send NMT Start Remote Node - command)
// 	= 1 Do not allow to send NMT Start Remote Node -command; the application may start the Slaves
// Bit 4
// 	= 0 On Error Control Event of a mandatory slave treat the slave individually.
// 	= 1 On Error Control Event of a mandatory slave perform NMT Reset
// 		all Nodes (including self). Refer to Bit 6 and 1F81h, Bit 3
// Bit 5
// 	= 0 Do not participate Flying Manager Process
// 	= 1 Participate the Flying Manager Process
// Bit 6
// 	= 0 On Error Control Event of a mandatory slave treat the slave according to Bit 4
// 	= 1 On Error Control Event of a mandatory slave send NMT Stop all
// 		Nodes (including self). Ignore Bit 4.
// Bit 7-31 reserved (0000 0000 0000 0000 0000 0000 0)
//-----------------------------------------------------------------
static UNSIGNED32 nmtStartup = {0};
static entryOD_t nmtStartupEntry = {
	.subindex = 0,
	.access = OD_RW,
	.mappingPDO = false,
	.dataType = OD_UNSIGNED32,
	.data = nmtStartup.arr};
static objectOD_t nmtStartupObj = {
	.index = 0x1F80,
	.objCode = OD_VAR,
	.placement = OD_RAM,
	.subindexNum = 1,
	.entry = &nmtStartupEntry};
//-----------------------------------------------------------------
// Object 1F50h Download Program Data
// 	OBJECT DESCRIPTION
// 		Index 1F50h
// 			Name: Download Program Data
// 			Object code: ARRAY
// 			Data type: DOMAIN
// 			Category: Optional
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Description: Highest sub-index supported
// 			Entry category: Mandatory
// 			Access: const
// 			PDO mapping: No
// 			Value range: UNSIGNED8
// 			Default value: 1
// 		Sub-index 01h
//			Description: Сюда пишется программа
// 			Access: wo
// 			PDO mapping: No
// 			Value range: DOMAIN
//-----------------------------------------------------------------
static uint8_t downloadProgramData = 1;
static entryOD_t downloadProgramEntry[2] = {

	{.subindex = 0,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED8,
	 .data = &downloadProgramData},

	{.subindex = 1,
	 .access = OD_CONST,
	 .mappingPDO = false,
	 .dataType = OD_DOMAIN,
	 .data = NULL}};

static objectOD_t downloadProgramObj = {
	.index = 0x1F50,
	.objCode = OD_ARRAY,
	.placement = OD_RAM,
	.subindexNum = 2,
	.entry = downloadProgramEntry};
//-----------------------------------------------------------------
// Object 1F51h Program Control
// 	OBJECT DESCRIPTION
// 		Index 1F51h
// 			Name: Download Program Data
// 			Object code: ARRAY
// 			Data type: UNSIGNED32
// 			Category: Optional
// 	ENTRY DESCRIPTION
// 		Sub-index 00h
// 			Description: Highest sub-index supported
// 			Entry category: Mandatory
// 			Access: const
// 			PDO mapping: No
// 			Value range: UNSIGNED8
// 			Default value: 5
// 		Sub-index 01h
//			Description: команда
// 			Access: wo
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 		Sub-index 02h
//			Description: адрес векторов прерывания
// 			Access: wo
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 		Sub-index 03h
//			Description: адрес вектора сброса
// 			Access: wo
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 		Sub-index 04h
//			Description: размер прошивки
// 			Access: wo
// 			PDO mapping: No
// 			Value range: UNSIGNED32
// 		Sub-index 05h
//			Description: CRC32
// 			Access: wo
// 			PDO mapping: No
// 			Value range: UNSIGNED32
//-----------------------------------------------------------------
static uint8_t programControlHighestSubIndex = 0;
static UNSIGNED32 programControl_1 = {0};
static UNSIGNED32 programControl_2 = {0};
static UNSIGNED32 programControl_3 = {0};
static UNSIGNED32 programControl_4 = {0};
static UNSIGNED32 programControl_5 = {0};
static entryOD_t programControlEntry[6] = {

	{.subindex = 0,
	 .access = OD_RO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED8,
	 .data = &programControlHighestSubIndex},

	{.subindex = 1,
	 .access = OD_WO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = programControl_1.arr},

	{.subindex = 2,
	 .access = OD_WO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = programControl_2.arr},

	{.subindex = 3,
	 .access = OD_WO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = programControl_3.arr},

	{.subindex = 4,
	 .access = OD_WO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = programControl_4.arr},

	{.subindex = 5,
	 .access = OD_WO,
	 .mappingPDO = false,
	 .dataType = OD_UNSIGNED32,
	 .data = programControl_5.arr}};

static objectOD_t programControlObj = {
	.index = 0x1F51,
	.objCode = OD_ARRAY,
	.placement = OD_RAM,
	.subindexNum = 6,
	.entry = programControlEntry};
//-----------------------------------------------------------------
#define OBJECT_DICTIONARY_SIZE 9
static objectOD_t *objDictionary[OBJECT_DICTIONARY_SIZE];
/////////////////////////////////////////////////////////////////////
// Функция:
void initOD(void)
{
	objDictionary[0] = &deviceTypeObj;
	objDictionary[1] = &deviceTypeObj;
	objDictionary[2] = &errorRegisterObj;
	objDictionary[3] = &manufacturerDeviceNameObj;
	objDictionary[4] = &consumerHeartbeatTimeObj;
	objDictionary[5] = &producerHeartbeatTimeObj;
	objDictionary[6] = &identityObjectObj;
	objDictionary[7] = &nmtStartupObj;
	objDictionary[8] = &programControlObj;
}

enum
{
	WRITE_REQUEST = 1,
	READ_REQUEST
};
/////////////////////////////////////////////////////////////////////
// Функция:
static uint32_t requestControl(uint16_t index,
							   uint8_t subindex,
							   uint16_t buffSize,
							   uint16_t requestType,
							   uint16_t *objNum)
{
	for (uint16_t i = 0; i < OBJECT_DICTIONARY_SIZE; i++)
	{
		if (objDictionary[i]->index == index)
		{
			*objNum = i;
			if (objDictionary[*objNum]->subindexNum >= subindex)
			{
				return SUBINDEX_NOT_EXIST_IN_OD;
			}
			if (objDictionary[*objNum]->entry[subindex].access == OD_WO && requestType == READ_REQUEST ||
				objDictionary[*objNum]->entry[subindex].access == OD_RO && requestType == WRITE_REQUEST ||
				objDictionary[*objNum]->entry[subindex].access == OD_CONST && requestType == WRITE_REQUEST)
			{
				return UNSUPPORTED_ACCESS_TO_AN_OBJECT;
			}
			uint16_t objEntrySize =0;
			switch(objDictionary[*objNum]->entry[subindex].dataType)
			{
				case OD_BOOLEAN:
				case OD_INTEGER8:
				case OD_UNSIGNED8:
				{
					objEntrySize = 1;
				}
				break;
				case OD_INTEGER16:
				case OD_UNSIGNED16:
				{
					objEntrySize = 2;
				}
				break;
				case OD_INTEGER32:
				case OD_UNSIGNED32:
				case OD_REAL32:
				{
					objEntrySize = 4;
				}
				break;
				case OD_VISIBLE_STRING:
				{
					objEntrySize = 16;
				}
				break;
			}
			if(objEntrySize != buffSize)
			{
				return INVALID_VALUE_FOR_PARAMETER;
			}
			return INTERACTION_SUCCESSFUL;
		}
	}
	return INDEX_NOT_EXIST_IN_OD;
}

/////////////////////////////////////////////////////////////////////
// Функция:
uint32_t readObjectOD(uint16_t index, uint8_t subindex, uint8_t *buff, uint16_t buffSize)
{
	uint32_t errCode = INTERACTION_SUCCESSFUL;
	uint16_t objNum = 0;
	errCode = requestControl(index, subindex, buffSize, READ_REQUEST, &objNum);
	if (errCode)
	{
		return errCode;
	}
	memcpy(buff,objDictionary[objNum]->entry->data,buffSize);

	return errCode;
}
/////////////////////////////////////////////////////////////////////
// Функция:
uint32_t writeObjectOD(uint16_t index, uint8_t subindex, uint8_t *buff, uint16_t buffSize)
{
	uint32_t errCode = INTERACTION_SUCCESSFUL;
	uint16_t objNum = 0;
	errCode = requestControl(index, subindex, buffSize, WRITE_REQUEST, &objNum);
	if (errCode)
	{
		return errCode;
	}
	memcpy(objDictionary[objNum]->entry->data,buff,buffSize);

	return errCode;
}