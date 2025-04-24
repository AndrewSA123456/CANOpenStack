#include "can_hw.h"
#include "CANOpen.h"
#include "CANOpenOD.h"
#include "CANOpenSDOClient.h"

typedef enum
{
	NMT_ERROR,
	NMT_INIT,
	NMT_RESET_APP,
	NMT_RESET_COMM,
	NMT_PRE_OPERATIONAL,
	NMT_OPERATIONAL,
	NMT_STOPPED
} stateNMT_t;

static void CO_init_state_handle(void);
static void CO_reset_app_state_handle(void);
static stateNMT_t CO_reset_comm_state_handle(void);
static stateNMT_t CO_pre_operation_state_handle();
static stateNMT_t CO_operation_state_handle();
static stateNMT_t CO_stopped_state_handle();
static stateNMT_t CO_nmt_handler(stateNMT_t currentState, canFrame_t *canMsg);

extern canFrame_t canReceiveMsg;
canFrame_t canTransmitMsg = {0};

/////////////////////////////////////////////////////////////////////
// Функция:
void CO_process(void)
{
	static stateNMT_t CANOpenAppState = NMT_INIT;
	switch (CANOpenAppState)
	{
	case NMT_INIT:
	{
		// printf("NMT state = INIT\n");
		CO_init_state_handle();
	}
	case NMT_RESET_APP:
	{
		// printf("NMT state = RESET_APP\n");
		CO_reset_app_state_handle();
	}
	case NMT_RESET_COMM:
	{
		deinitCAN();
		if (initCAN())
		{
			return;
		}
		// printf("NMT state = RESET_COMM\n");
		CANOpenAppState = CO_reset_comm_state_handle();
	}
	case NMT_PRE_OPERATIONAL:
	{
		// printf("NMT state = PRE_OPERATIONAL\n");
		CANOpenAppState = CO_pre_operation_state_handle();
	}
	break;
	case NMT_OPERATIONAL:
	{
		// printf("NMT state = OPERATIONAL\n");
		CANOpenAppState = CO_operation_state_handle();
	}
	break;
	case NMT_STOPPED:
	{
		// printf("NMT state = STOPPED\n");
		CANOpenAppState = CO_stopped_state_handle();
	}
	break;
	default:
	{
	}
	break;
	}
}
/////////////////////////////////////////////////////////////////////
// Функция: **Инициализация**:
// Это первое под-состояние NMT,
// в которое устройство CANopen переходит
// после включения питания или аппаратного сброса.
// После завершения базовой инициализации
// устройства CANopen оно автоматически переходит
// в под-состояние NMT "сброс приложения".
static void CO_init_state_handle(void)
{
	initOD();
}
/////////////////////////////////////////////////////////////////////
// Функция: **Сброс приложения**:
// В этом под-состоянии NMT параметры
// специфической для производителя области профиля и стандартизированной
// области профиля устройства устанавливаются в значения, соответствующие
// включению питания. После установки значений при включении питания
// устройство автоматически переходит в под-состояние NMT "сброс коммуникации".
static void CO_reset_app_state_handle(void)
{
}
/////////////////////////////////////////////////////////////////////
// Функция: **Сброс коммуникации**:
// В этом под-состоянии NMT параметры
// области коммуникационного профиля устанавливаются в значения,
// соответствующие включению питания. После этого состояние
// инициализации NMT завершается, и устройство CANopen выполняет
// службу NMT "запись при загрузке" и переходит в состояние NMT
// "предоперационный режим".
static stateNMT_t CO_reset_comm_state_handle(void)
{
	// перед выходом должны отправить сообщение: cansend (интерфейс) NMT_ERR_CTRL_ID#00
	canTransmitMsg.id = NMT_ERR_CTRL_ID(NODE_ID);
	canTransmitMsg.len = 1;
	memset(canTransmitMsg.data, 0, sizeof(canTransmitMsg.data));
	if (transmitCAN(canTransmitMsg))
		return NMT_ERROR;
	return NMT_PRE_OPERATIONAL;
}
/////////////////////////////////////////////////////////////////////
// Функция: **Предоперационный режим**:
// В состоянии NMT "предоперационный режим"
// возможна коммуникация через SDO. PDO в этом состоянии не существуют,
// поэтому коммуникация через PDO запрещена. Конфигурация PDO,
// параметров и распределение объектов приложения (PDO mapping)
// могут выполняться приложением конфигурации. Устройство CANopen
// может быть переключено в состояние NMT "операционный режим"
// напрямую путем отправки службы NMT "запуск удаленного узла"
// или с помощью локального управления.
static stateNMT_t CO_pre_operation_state_handle(void)
{
	canFrame_t canReceiveMSG = {0};
	if (!isCANRingBuffEmpty(&ReceiveCANRingBuff))
	{
		popCANRingBuff(&ReceiveCANRingBuff, &canReceiveMSG);
		switch (canReceiveMSG.id)
		{
		case NMT_ID:
		{
			return CO_nmt_handler(NMT_PRE_OPERATIONAL, &canReceiveMSG);
		}
		break;
		case SDO_REQUEST_ID(NODE_ID):
		{
			CO_sdo_handler(&canReceiveMSG);
		}
		break;
		}
	}
	return NMT_PRE_OPERATIONAL;
}
/////////////////////////////////////////////////////////////////////
// Функция: **Операционный режим NMT**:
// В состоянии NMT "операционный режим"
// все коммуникационные объекты активны. Переход в состояние NMT
// "операционный режим" создает все PDO; конструктор использует параметры,
// описанные в словаре объектов. Доступ к словарю объектов через SDO возможен.
// Однако аспекты реализации или автомат состояний приложения
// могут требовать ограничения доступа к определенным объектам
// в состоянии NMT "операционный режим", например, объект может
// содержать приложение, которое нельзя изменять во время выполнения.
static stateNMT_t CO_operation_state_handle(void)
{
	canFrame_t canReceiveMSG = {0};
	if (!isCANRingBuffEmpty(&ReceiveCANRingBuff))
	{
		popCANRingBuff(&ReceiveCANRingBuff, &canReceiveMSG);
		switch (canReceiveMSG.id)
		{
		case NMT_ID:
		{
			return CO_nmt_handler(NMT_OPERATIONAL, &canReceiveMSG);
		}
		break;
		// case SYNC_ID:
		// {
		// 	return
		// }
		// break;
		// case EMCY_ID:
		// {
		// 	return
		// }
		// break;
		// case TIME_STAMP_ID:
		// {
		// 	return
		// }
		// break;
		// case NMT_ERR_CTRL_ID:
		// {
		// 	return
		// }
		// break;
		case SDO_REQUEST_ID(NODE_ID):
		{
			CO_sdo_handler(&canReceiveMSG);
		}
		break;
		}
	}
	return NMT_OPERATIONAL;
}
/////////////////////////////////////////////////////////////////////
// Функция: **Остановленное состояние NMT**:
// Переключение устройства CANopen
// в состояние NMT "остановлено" заставляет его полностью
// прекратить коммуникацию (за исключением охраны узла и
// сердцебиения, если они активны). Кроме того, это состояние NMT
// может использоваться для достижения определенного поведения приложения.
// Определение этого поведения входит в область профилей
// устройств и профилей приложений. Если в этом состоянии
// NMT возникают сообщения EMCY, они остаются в ожидании.
// Наиболее актуальная активная причина EMCY может быть
// передана после того, как устройство CANopen перейдет
// в другое состояние NMT.
static stateNMT_t CO_stopped_state_handle(void)
{
	canFrame_t canReceiveMSG = {0};
	if (!isCANRingBuffEmpty(&ReceiveCANRingBuff))
	{
		popCANRingBuff(&ReceiveCANRingBuff, &canReceiveMSG);
		switch (canReceiveMSG.id)
		{
		case NMT_ID:
		{
			return CO_nmt_handler(NMT_OPERATIONAL, &canReceiveMSG);
		}
		break;
		// case SYNC_ID:
		// {
		// 	return
		// }
		// break;
		// case EMCY_ID:
		// {
		// 	return
		// }
		// break;
		// case TIME_STAMP_ID:
		// {
		// 	return
		// }
		// break;
		// case NMT_ERR_CTRL_ID:
		// {
		// 	return
		// }
		// break;
		case SDO_REQUEST_ID(NODE_ID):
		{
			CO_sdo_handler(&canReceiveMSG);
		}
		break;
		}
	}
	return NMT_STOPPED;
}
/////////////////////////////////////////////////////////////////////
// Функция:
enum
{
	CS_NMT_OPERATIONAL = 0x1,
	CS_NMT_STOPPED = 0x2,
	CS_NMT_PRE_OPERATIONAL = 0x80,
	CS_NMT_RESET_APP = 0x81,
	CS_NMT_RESET_COMM = 0x82
};
static stateNMT_t CO_nmt_handler(stateNMT_t currentState, canFrame_t *canMsg)
{
	// Структура NMT кадра:
	// CAN_ID	|	1byte	|	2byte
	//------------------------------
	// NMT_ID	| 	CS		|	NODE_ID
	//------------------------------
	// NODE_ID = 0 Broadcast
	if (canMsg->data[1] == NODE_ID || canMsg->id == BROADCAST_ID)
	{
		switch (canMsg->data[0])
		{
		case CS_NMT_OPERATIONAL:
		{
			printf("NMT state = NMT_OPERATIONAL\n");
			return NMT_OPERATIONAL;
		}
		break;
		case CS_NMT_STOPPED:
		{
			printf("NMT state = NMT_STOPPED\n");
			return NMT_STOPPED;
		}
		break;
		case CS_NMT_PRE_OPERATIONAL:
		{
			printf("NMT state = NMT_PRE_OPERATIONAL\n");
			return NMT_PRE_OPERATIONAL;
		}
		break;
		case CS_NMT_RESET_APP:
		{
			printf("NMT state = NMT_RESET_APP\n");
			return NMT_RESET_APP;
		}
		break;
		case CS_NMT_RESET_COMM:
		{
			printf("NMT state = NMT_RESET_COMM\n");
			return NMT_RESET_COMM;
		}
		break;
		default:
		{
			return currentState;
		}
		break;
		}
	}
	else
	{
		return currentState;
	}
}
