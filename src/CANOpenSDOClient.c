#include "CANOpen.h"
#include "CANOpenOD.h"
#include "CANOpenSDOClient.h"
#include "ring_buff.h"

/////////////////////////////////////////////////////////////////////
// Функция:
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
			memcpy(buff, canRequestMsg->data + 5, buffSize);
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
			clearBuff(&SDORingBuff);
			canResponseMSG.data[0] = SDO_SCS_DL_INIT;
			canResponseMSG.data[1] = canRequestMsg->data[1];
			canResponseMSG.data[2] = canRequestMsg->data[2];
			canResponseMSG.data[3] = canRequestMsg->data[3];
			canResponseMSG.data[4] = 0;
			canResponseMSG.data[5] = 0;
			canResponseMSG.data[6] = 0;
			canResponseMSG.data[7] = 0;
		}
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
				popBuff(&SDORingBuff, &normalLoadBuff[i]);
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
					popBuff(&SDORingBuff, &normalLoadBuff[i]);
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
		index = (uint16_t)canRequestMsg->data[2] << 8;
		index |= (uint16_t)canRequestMsg->data[1];
		subindex = canRequestMsg->data[3];
		buffSize = 16;
		abortCode = readObjectOD(index, subindex, normalLoadBuff, &buffSize);
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
		else if (buffSize < 4)
		{ // expedited transfer
			canResponseMSG.data[0] = SDO_SCS_UL_INIT;
			canResponseMSG.data[0] |= SDO_E_Msk;
			if (buffSize < 3)
			{
				canResponseMSG.data[0] |= SDO_S_Msk;
				canResponseMSG.data[0] |= (((uint8_t)(4 - buffSize)) << SDO_N_INIT_Pos);
			}
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			memset(canResponseMSG.data + 4, 0, 4);
			memcpy(canResponseMSG.data + 4, normalLoadBuff, buffSize);
		}
		else
		{ // initial segment transfer
			canResponseMSG.data[0] = SDO_SCS_UL_INIT;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			clearBuff(&SDORingBuff);
			for (int i = 0; i < buffSize; i++)
			{
				pushBuff(&SDORingBuff, normalLoadBuff[i]);
			}
			memset(canResponseMSG.data + 4, 0, 4);
			canResponseMSG.data[4] = (uint8_t)buffSize;
		}
	}
	break;
	case SDO_CCS_UL_SEG:
	{
		canResponseMSG.data[0] = 0;
		for (int i = 1; i < 8; i++)
		{
			if (popBuff(&SDORingBuff, &canResponseMSG.data[i]))
			{
				canResponseMSG.data[0] = (uint8_t)(7 - i);
			}
			else
			{
				canResponseMSG.data[i] = 0;
			}
		}
		if (canResponseMSG.data[0])
		{
			canResponseMSG.data[0] <<= SDO_N_Pos;
			canResponseMSG.data[0] |= SDO_C_Msk;
		}
		canResponseMSG.data[0] |=
			canRequestMsg->data[0] & SDO_T_Msk ? SDO_SCS_UL_SEG_TOGGLED : SDO_SCS_UL_SEG;
		canResponseMSG.data[1] = canRequestMsg->data[1];
		canResponseMSG.data[2] = canRequestMsg->data[2];
		canResponseMSG.data[3] = canRequestMsg->data[3];
	}
	break;
	default:
	{
	}
	break;
	}
	transmitCAN(canResponseMSG);
}
