#include "main.h"
#include "CANOpen.h"
#include "can_hw.h"

bool newCanMsg = false;
CANRingBuff ReceiveCANRingBuff = {0};

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "ru_RU.UTF-8");
	while (1)
	{
		CO_process();
		// Только для тестов
		// в реальном приложении в кольцевой буфер принятые сообщения попадают
		// либо из прерываний либо из выделенного потока приема сообщений
		// мьютекс для потока заложен
		receiveCAN();
	}

	return EXIT_SUCCESS;
}