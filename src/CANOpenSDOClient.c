#include "CANOpen.h"
#include "CANOpenOD.h"
#include "CANOpenSDOClient.h"
#include "ring_buff.h"

/////////////////////////////////////////////////////////////////////
// Функция:
void CO_sdo_handler(canFrame_t *canRequestMsg)
{
	canFrame_t canResponseMSG = {0};
	static RingBuff SDORingBuff = {0};
	uint16_t index = 0;
	uint8_t subindex = 0;
	uint32_t abortCode = 0;

	uint8_t buff[16] = {0};
	uint32_t buffSize = 0;
	// static uint32_t normalLoadDataLeft = 0;

	canResponseMSG.id = SDO_RESPONSE_ID(NODE_ID);
	canResponseMSG.len = 8;

	switch (canRequestMsg->data[0] & SDO_CS_Msk)
	{
	case SDO_CCS_DL_INIT:
	{
		if (canRequestMsg->data[0] & SDO_E_Msk)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Expedited transfer---------------------
			if (canRequestMsg->data[0] & SDO_S_Msk)
			{
				// Expedited transfer with size indicator
				buffSize = (uint16_t)(SDO_N_INIT_SIZE_DATA(canRequestMsg->data[0]));
			}
			else
			{
				// Expedited transfer without size indicator
				buffSize = 4;
			}
			index = (uint16_t)canRequestMsg->data[2] << 8;
			index |= (uint16_t)canRequestMsg->data[1];
			subindex = canRequestMsg->data[3];
			memcpy(buff, &canRequestMsg->data[4], buffSize);
			abortCode = writeObjectOD(index, subindex, buff, buffSize);
		}
		else
		{
			//////////////////////////////////////////////////////////////
			//---------------------Normal transfer init-------------------
			if (canRequestMsg->data[0] & SDO_S_Msk)
			{ // Normal transfer with size indicator
				memcpy(&buffSize, &canRequestMsg->data[4], sizeof(buffSize));
				index = (uint16_t)canRequestMsg->data[2] << 8;
				index |= (uint16_t)canRequestMsg->data[1];
				subindex = canRequestMsg->data[3];
				abortCode = writeObjectOD(index, subindex, NULL, buffSize);
			}
			else
			{ // Normal transfer without size indicator
				// buffSize = 0;
				abortCode = CO_ERR_0504_0002;
			}
		}
		if (abortCode)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Abort transfer responce----------------
			canResponseMSG.data[0] = SDO_CS_ABORT;
			canResponseMSG.data[1] = canRequestMsg->data[1];
			canResponseMSG.data[2] = canRequestMsg->data[2];
			canResponseMSG.data[3] = canRequestMsg->data[3];
			memcpy(&canResponseMSG.data[4], &abortCode, sizeof(abortCode));
		}
		else
		{
			//////////////////////////////////////////////////////////////
			//---------------------Transfer responce----------------------
			clearBuff(&SDORingBuff);
			canResponseMSG.data[0] = SDO_SCS_DL_INIT;
			canResponseMSG.data[1] = canRequestMsg->data[1];
			canResponseMSG.data[2] = canRequestMsg->data[2];
			canResponseMSG.data[3] = canRequestMsg->data[3];
		}
	}
	break;
	case SDO_CCS_DL_SEG:
	{
		if (canRequestMsg->data[0] & SDO_C_Msk)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Normal transfer LAST-------------------
			buffSize = (uint16_t)(SDO_N_SIZE_DATA(canRequestMsg->data[0]));
			for (int i = 1; i < buffSize; i++)
			{
				pushBuff(&SDORingBuff, canRequestMsg->data[i]);
			}
			for (int i = 0; i < 16; i++)
			{
				popBuff(&SDORingBuff, &buff[i]);
			}
			if (isBuffEmpty(&SDORingBuff))
			{
				abortCode = writeObjectOD(index, subindex, buff, 16);
			}
			else
			{
				abortCode = ABORT_SDO_INVALID_SIZE_DATA;
			}
		}
		else
		{
			//////////////////////////////////////////////////////////////
			//---------------------Normal transfer------------------------
			for (int i = 1; i < 8; i++)
			{
				pushBuff(&SDORingBuff, canRequestMsg->data[i]);
			}
			if (isBuffFull(&SDORingBuff))
			{
				for (int i = 0; i < 16; i++)
				{
					popBuff(&SDORingBuff, &buff[i]);
				}
				abortCode = writeObjectOD(index, subindex, buff, 16);
			}
		}
		if (abortCode)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Abort transfer responce----------------
			canResponseMSG.data[0] = SDO_CS_ABORT;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			memcpy(&canResponseMSG.data[4], &abortCode, sizeof(abortCode));
		}
		else
		{
			//////////////////////////////////////////////////////////////
			//---------------------Transfer responce----------------------
			canResponseMSG.data[0] =
				(canRequestMsg->data[0] & SDO_T_Msk) ? SDO_SCS_DL_SEG : SDO_SCS_DL_SEG_TOGGLED;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
		}
	}
	break;
	case SDO_CCS_UL_INIT:
	{
		index = (uint16_t)canRequestMsg->data[2] << 8;
		index |= (uint16_t)canRequestMsg->data[1];
		subindex = canRequestMsg->data[3];
		buffSize = 16;
		abortCode = readObjectOD(index, subindex, buff, &buffSize);
		if (abortCode)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Abort transfer responce----------------
			canResponseMSG.data[0] = SDO_CS_ABORT;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			memcpy(&canResponseMSG.data[4], &abortCode, sizeof(abortCode));
		}
		else if (buffSize < 4)
		{
			//////////////////////////////////////////////////////////////
			//---------------------Expedited transfer---------------------
			canResponseMSG.data[0] = SDO_SCS_UL_INIT;
			canResponseMSG.data[0] |= SDO_E_Msk;
			if (buffSize < 3)
			{
				canResponseMSG.data[0] |= SDO_S_Msk;
				canResponseMSG.data[0] |= (uint8_t)SDO_N_INIT_SIZE_DATA(buffSize);
			}
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			memcpy(&canResponseMSG.data[4], buff, buffSize);
		}
		else
		{
			//////////////////////////////////////////////////////////////
			//---------------------Normal transfer init-------------------
			canResponseMSG.data[0] = SDO_SCS_UL_INIT;
			canResponseMSG.data[1] = (uint8_t)(index & 0xFF);
			canResponseMSG.data[2] = (uint8_t)((index >> 8) & 0xFF);
			canResponseMSG.data[3] = subindex;
			clearBuff(&SDORingBuff);
			for (int i = 0; i < buffSize; i++)
			{
				pushBuff(&SDORingBuff, buff[i]);
			}
			canResponseMSG.data[4] = (uint8_t)buffSize;
		}
	}
	break;
	case SDO_CCS_UL_SEG:
	{
		//////////////////////////////////////////////////////////////
		//---------------------Normal transfer------------------------
		canResponseMSG.data[0] = 0;
		for (int i = 1; i < 8; i++)
		{
			// Смысл следующего кода:
			// Пока данные из буфера извлекаются мы вычисляем сколько байт без данных.
			// В конце это будет либо ноль, либо столько сколько реально без данных.
			// Как только данных больше нет оставшийся пакет мы зануляем.
			if (popBuff(&SDORingBuff, &canResponseMSG.data[i]))
			{
				canResponseMSG.data[0] = (uint8_t)(7 - i);
			}
			else
			{
				canResponseMSG.data[i] = 0;
			}
		}
		// Если canResponseMSG.data[0] содержит какие-то данные то считаем,
		// что это количество байт в пакете без данных, и соответсвенно если байт на пакет не хватило,
		// то это последний пакет в передаче.
		if (canResponseMSG.data[0])
		{
			canResponseMSG.data[0] <<= SDO_N_Pos;
			canResponseMSG.data[0] |= SDO_C_Msk;
		}
		canResponseMSG.data[0] |=
			canRequestMsg->data[0] & SDO_T_Msk ? SDO_SCS_UL_SEG_TOGGLED : SDO_SCS_UL_SEG;
	}
	break;
	}

	printf("id = %X	", canResponseMSG.id);
	printf("len = %X:	", canResponseMSG.len);
	for (int i = 0; i < 8; i++)
	{
		printf("%X	", canResponseMSG.data[i]);
	}
	printf("\n");

	transmitCAN(canResponseMSG);
}
