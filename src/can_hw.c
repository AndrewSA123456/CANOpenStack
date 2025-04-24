#include "can_hw.h"

#define CAN_INTERFACE_NAME "vcan0"
#define LINUX
// #define STM32

#ifdef LINUX
#include "SocketCAN.h"
#define RECEIVE_TIMEOUT 1000
#endif
#ifdef STM32
#endif

enum
{
	CAN_SUCCESS,
	CAN_ERROR
};

int sockDesc = 0;
/////////////////////////////////////////////////////////////////////
// Функция: инициализация CAN
int initCAN(void)
{
#ifdef LINUX
	sockDesc = openSocketCAN(CAN_INTERFACE_NAME);
	if (sockDesc < 0)
	{
		fprintf(stderr, "Open socket failure\n");
		return CAN_ERROR;
	}
#endif
#ifdef STM32
#endif
	printf("initCAN success\n");
	return CAN_SUCCESS;
}
/////////////////////////////////////////////////////////////////////
// Функция: деинициализация CAN
int deinitCAN(void)
{
#ifdef LINUX
	if (sockDesc)
		closeSocketCAN(sockDesc);
#endif
#ifdef STM32
#endif
	return CAN_SUCCESS;
}
/////////////////////////////////////////////////////////////////////
// Функция: установить фильтр CAN
void setCANfilter(uint32_t canID)
{
#ifdef LINUX
#endif
#ifdef STM32
#endif
}
/////////////////////////////////////////////////////////////////////
// Функция: принять сообщение по CAN
int receiveCAN()
{
	canFrame_t CANframeNew = {0};
#ifdef LINUX
	struct can_frame CANframe = {0};
	if (sockDesc)
	{
		socketCANReceiveNotBlocking(sockDesc, &CANframe, RECEIVE_TIMEOUT);
		memcpy(CANframeNew.data, CANframe.data, CANframe.len);
		CANframeNew.id = CANframe.can_id;
		CANframeNew.len = CANframe.len;
	}
#endif
#ifdef STM32
#endif
	if (CANframeNew.data[0])
	{
		if (pushCANRingBuff(&ReceiveCANRingBuff, CANframeNew))
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/////////////////////////////////////////////////////////////////////
// Функция: передать сообщение по CAN
int transmitCAN(canFrame_t CANframe)
{
#ifdef LINUX
	return socketCANTransmit(sockDesc, CANframe.id, CANframe.len, CANframe.data);
#endif
#ifdef STM32
#endif
}
/////////////////////////////////////////////////////////////////////
// Функция: Проверка, пуст ли буфер
bool isCANRingBuffEmpty(CANRingBuff *buff)
{
	return buff->head == buff->tail;
}
/////////////////////////////////////////////////////////////////////
// Функция: Проверка, полон ли буфер
bool isCANRingBuffFull(CANRingBuff *buff)
{
	return (buff->tail + 1) % CAN_RECEIVE_BUFF_SIZE == buff->head;
}
/////////////////////////////////////////////////////////////////////
// Функция: Добавление элемента в буфер
bool pushCANRingBuff(CANRingBuff *buff, canFrame_t frame)
{
	if (isCANRingBuffFull(buff))
	{
		return false;
	}
	memcpy(&buff->frame[buff->tail], &frame, sizeof(canFrame_t));
	buff->tail = (buff->tail + 1) % CAN_RECEIVE_BUFF_SIZE;
	return true;
}
/////////////////////////////////////////////////////////////////////
// Функция: Извлечение элемента из буфера
bool popCANRingBuff(CANRingBuff *buff, canFrame_t *frame)
{
	if (isCANRingBuffEmpty(buff))
	{
		return false;
	}
	memcpy(frame, &buff->frame[buff->head], sizeof(canFrame_t));
	buff->head = (buff->head + 1) % CAN_RECEIVE_BUFF_SIZE;
	return true;
}
/////////////////////////////////////////////////////////////////////
// Функция: Очистка буфера
void clearCANRingBuff(CANRingBuff *buff)
{
	buff->head = buff->tail;
}
