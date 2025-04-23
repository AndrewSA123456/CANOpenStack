#include "ring_buff.h"

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
// Функция: Очистка буфера
void clearBuff(RingBuff *buff)
{
	buff->head = buff->tail;
}
