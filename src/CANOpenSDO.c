#include "CANOpenSDO.h"
#include "CANOpenOD.h"

#define RING_BUFF_SIZE 21
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
/////////////////////////////////////////////////////////////////////
// Функция: Проверка, пуст ли буфер
bool isBuffEmpty(RingBuff *buff)
{
	return buff->head == buff->tail;
}
/////////////////////////////////////////////////////////////////////
// Функция: Проверка, полон ли буфер
bool isBuffFull(RingBuff *buff)
{
	return (buff->tail + 1) % RING_BUFF_SIZE == buff->head;
}
/////////////////////////////////////////////////////////////////////
// Функция: Добавление элемента в буфер
bool pushBuff(RingBuff *buff, uint8_t data)
{
	if (isBuffFull(buff))
	{
		return false;
	}
	buff->data[buff->tail] = data;
	buff->tail = (buff->tail + 1) % RING_BUFF_SIZE;
	return true;
}
/////////////////////////////////////////////////////////////////////
// Функция: Извлечение элемента из буфера
bool popBuff(RingBuff *buff, uint8_t *data)
{
	if (isBuffEmpty(buff))
	{
		return false;
	}
	*data = buff->data[buff->head];
	buff->head = (buff->head + 1) % RING_BUFF_SIZE;
	return true;
}
/////////////////////////////////////////////////////////////////////
// Функция:
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
#define SDO_N_INIT_Msk ((uint8_t)(0x3 << SDO_S_Pos))

#define SDO_N_Pos (1)
#define SDO_N_Msk ((uint8_t)(0x7 << SDO_S_Pos))

#define SDO_T_Pos (4)
#define SDO_T_Msk ((uint8_t)(0x1 << SDO_S_Pos))

#define SDO_C_Pos (0)
#define SDO_C_Msk ((uint8_t)(0x1 << SDO_C_Pos))

#define ABORT_SDO_INVALID_SIZE_DATA (0x06070010U)

canFrame_t canResponseMSG = {0};
void CO_sdo_handler(canFrame_t *canRequestMsg)
{
	static uint16_t index = 0;
	static uint8_t subindex = 0;
	uint8_t buff[8] = {0};
	uint16_t buffSize = 0;
	uint32_t abortCode;
	static uint32_t normalLoadPacketNum = 0;
	static uint32_t normalLoadDataLeft = 0;
	uint8_t normalLoadBuff[16] = {0};
	static RingBuff SDORingBuff = {0};
	switch (canRequestMsg->data[0] & SDO_CS_Msk)
	{
	case SDO_CCS_DL_INIT:
	{
		index = 0;
		subindex = 0;
		if (canRequestMsg->data[0] & SDO_E_Msk)
		{
			if (canRequestMsg->data[0] & SDO_S_Msk)
			{ // Expedited transfer with size indicator
				buffSize = (uint16_t)(4 - ((canRequestMsg->data[0] & SDO_N_INIT_Msk) >> SDO_N_INIT_Pos));
			}
			else
			{ // Expedited transfer without size indicator
				buffSize = 4;
			}
			index = (uint16_t)canRequestMsg->data[2] << 8;
			index |= (uint16_t)canRequestMsg->data[1];
			subindex = canRequestMsg->data[3];
			memcpy(buff, canRequestMsg->data[4], buffSize);
			abortCode = writeObjectOD(index, subindex, buff, buffSize);
		}
		else
		{
			// Normal transfer init
			if (canRequestMsg->data[0] & SDO_S_Msk)
			{ // Expedited transfer with size indicator
				normalLoadDataLeft = (uint32_t)canRequestMsg->data[4];
				normalLoadDataLeft |= ((uint32_t)canRequestMsg->data[5] << 8);
				normalLoadDataLeft |= ((uint32_t)canRequestMsg->data[6] << 16);
				normalLoadDataLeft |= ((uint32_t)canRequestMsg->data[7] << 24);
			}
			else
			{ // Expedited transfer without size indicator
				normalLoadDataLeft = 0;
			}
			index = (uint16_t)canRequestMsg->data[2] << 8;
			index |= (uint16_t)canRequestMsg->data[1];
			subindex = canRequestMsg->data[3];
			abortCode = writeObjectOD(index, subindex, NULL, 0);
		}
		if (abortCode)
		{
			canResponseMSG.data[0] = SDO_CS_ABORT;
			canResponseMSG.data[1] = canRequestMsg->data[1];
			canResponseMSG.data[2] = canRequestMsg->data[2];
			canResponseMSG.data[3] = canRequestMsg->data[3];
			canResponseMSG.data[4] = (uint8_t)(abortCode);
			canResponseMSG.data[5] = (uint8_t)((abortCode >> 8) & 0xFF);
			canResponseMSG.data[6] = (uint8_t)((abortCode >> 16) & 0xFF);
			canResponseMSG.data[7] = (uint8_t)((abortCode >> 24) & 0xFF);
		}
		else
		{
			canResponseMSG.data[0] = SDO_SCS_DL_INIT;
			canResponseMSG.data[1] = canRequestMsg->data[1];
			canResponseMSG.data[2] = canRequestMsg->data[2];
			canResponseMSG.data[3] = canRequestMsg->data[3];
			canResponseMSG.data[4] = 0;
			canResponseMSG.data[5] = 0;
			canResponseMSG.data[6] = 0;
			canResponseMSG.data[7] = 0;
		}
		// отправить
	}
	break;
	case SDO_CCS_DL_SEG:
	{
		if (canRequestMsg->data[0] & SDO_C_Msk)
		{ // флаг последнего пакета установлен
			buffSize = (uint16_t)(7 - ((canRequestMsg->data[0] & SDO_N_Msk) >> SDO_N_Pos));
			for (int i = 1; i < buffSize; i++)
			{
				pushBuff(&SDORingBuff, canRequestMsg->data[i]);
			}
			for (int i = 0; i < 16; i++)
			{
				popBuff(&SDORingBuff, buff);
				normalLoadBuff[i] = buff[0];
			}
			if (isBuffEmpty(&SDORingBuff))
			{
				abortCode = writeObjectOD(index, subindex, normalLoadBuff, 16);
			}
			else
			{
				abortCode = ABORT_SDO_INVALID_SIZE_DATA;
			}
		}
		else
		{
			for (int i = 1; i < 8; i++)
			{
				pushBuff(&SDORingBuff, canRequestMsg->data[i]);
			}
			if (isBuffFull(&SDORingBuff))
			{
				for (int i = 0; i < 16; i++)
				{
					popBuff(&SDORingBuff, buff);
					normalLoadBuff[i] = buff[0];
				}
				abortCode = writeObjectOD(index, subindex, normalLoadBuff, 16);
			}
		}
		if (abortCode)
		{
			canResponseMSG.data[0] = SDO_CS_ABORT;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			canResponseMSG.data[4] = (uint8_t)(abortCode);
			canResponseMSG.data[5] = (uint8_t)((abortCode >> 8) & 0xFF);
			canResponseMSG.data[6] = (uint8_t)((abortCode >> 16) & 0xFF);
			canResponseMSG.data[7] = (uint8_t)((abortCode >> 24) & 0xFF);
		}
		else
		{
			canResponseMSG.data[0] =
				(canRequestMsg->data[0] & SDO_T_Msk) ? SDO_SCS_DL_SEG : SDO_SCS_DL_SEG_TOGGLED;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			canResponseMSG.data[4] = 0;
			canResponseMSG.data[5] = 0;
			canResponseMSG.data[6] = 0;
			canResponseMSG.data[7] = 0;
		}
	}
	break;
	case SDO_CCS_UL_INIT:
	{
		index = 0;
		subindex = 0;
	}
	break;
	case SDO_CCS_UL_SEG:
	{
	}
	break;
	default:
	{
	}
	break;
	}
}
