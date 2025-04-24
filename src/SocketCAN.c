#include "SocketCAN.h"
/////////////////////////////////////////////////////////////////////
// Функция: открыть сокет CAN
int openSocketCAN(const char *interface_name)
{
	int sockDesc = 0;
	struct sockaddr_can addr = {0};
	struct ifreq ifr = {0};

	// Создаем сокет CAN
	if ((sockDesc = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		perror("Socket");
		return SOCKET_CAN_ERROR;
	}

	// Указываем имя интерфейса CAN
	strcpy(ifr.ifr_name, interface_name);
	// Получаем индекс интерфейса
	if (ioctl(sockDesc, SIOCGIFINDEX, &ifr) < 0)
	{
		perror("IOCTL");
		close(sockDesc);
		return SOCKET_CAN_ERROR;
	}

	// Заполняем структуру адреса
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	// Привязываем сокет к интерфейсу CAN
	if (bind(sockDesc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("Bind");
		close(sockDesc);
		return SOCKET_CAN_ERROR;
	}
	return sockDesc;
}
/////////////////////////////////////////////////////////////////////
// Функция: закрыть сокет CAN
int closeSocketCAN(int sockDesc)
{
	if (close(sockDesc) < 0)
	{
		perror("Close");
		return SOCKET_CAN_ERROR;
	}
	return SOCKET_CAN_OK;
}
/////////////////////////////////////////////////////////////////////
// Функция: инициализация фильтров CAN
void setFilterSocketCAN(int sockDesc, struct can_filter *rfilter, uint16_t rfilter_size)
{
	// Устанавливаем фильтры CAN
	for (int i = 0; i < rfilter_size; i++)
	{
		rfilter[i].can_id = 0x550;
		rfilter[i].can_mask = 0xFF0;
	}
	// Применяем фильтры к сокету
	setsockopt(sockDesc, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(struct can_filter) * rfilter_size);
}
/////////////////////////////////////////////////////////////////////
// Функция: принять сообщение по CAN в неблокирующем режиме, используя poll
int socketCANReceiveNotBlocking(int sockDesc, struct can_frame *CANframe, int mStimeout)
{
	// Настройка структуры pollfd
	struct pollfd sockfds[1];
	sockfds[0].fd = sockDesc;	// Сокет для мониторинга
	sockfds[0].events = POLLIN; // Интересующее событие: данные для чтения
	int RetPoll = 0;
	RetPoll = poll(sockfds, 1, mStimeout);
	if (RetPoll < 0)
	{
		perror("poll");
		return SOCKET_CAN_ERROR;
	}
	else if (RetPoll == 0)
	{
		return SOCKET_CAN_NO_NEW_MSG;
	}
	else
	{
		if (sockfds[0].revents & POLLIN)
		{
			sockfds[0].revents = 0;
			if (socketCANReceive(sockDesc, CANframe) == SOCKET_CAN_ERROR)
			{
				
				perror("socketCANReceive failed");
				return SOCKET_CAN_ERROR;
			}
			else
			{
				return SOCKET_CAN_NEW_MSG;
			}
		}
	}
	return SOCKET_CAN_ERROR;
}
/////////////////////////////////////////////////////////////////////
// Функция: принять сообщение по CAN в блокирующем режиме
int socketCANReceive(int sockDesc, struct can_frame *CANframe)
{
	if (read(sockDesc, CANframe, sizeof(struct can_frame)) < 0)
	{
		perror("Read");
		return SOCKET_CAN_ERROR;
	}
	return SOCKET_CAN_OK;
}
/////////////////////////////////////////////////////////////////////
// Функция: передать сообщение по CAN
int socketCANTransmit(int sockDesc, uint32_t message_id, uint8_t message_len, uint8_t *message_data)
{
	struct can_frame frame = {0};

	// Заполняем структуру кадра CAN
	frame.can_id = message_id;
	frame.len = message_len;

	// Копируем данные сообщения в кадр CAN
	memcpy(frame.data, message_data, CAN_MAX_DLEN);

	// Отправляем кадр CAN
	if (write(sockDesc, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
	{
		perror("Write");
		close(sockDesc);
		return SOCKET_CAN_ERROR;
	}

	return SOCKET_CAN_OK;
}
/////////////////////////////////////////////////////////////////////
// Функция: передать сообщение по CAN на вход принимаем сразу CAN кадр
int socketCANTransmitFrame(int sockDesc, struct can_frame frame)
{
	// Отправляем кадр CAN
	if (write(sockDesc, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
	{
		perror("Write");
		close(sockDesc);
		return SOCKET_CAN_ERROR;
	}

	return SOCKET_CAN_OK;
}
