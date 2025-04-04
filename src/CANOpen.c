#include "CANOpen.h"
#include "CANOpenOD.h"
#include "CANOpenSDO.h"
typedef enum
{
	INIT,
	RESET_APP,
	RESET_COMM,
	PRE_OPERATIONAL,
	OPERATIONAL,
	STOPPED
} stateNMT_t;

static void CO_init_state_handle(void);
static void CO_reset_app_state_handle(void);
static void CO_reset_comm_state_handle(void);
static stateNMT_t CO_pre_operation_state_handle();
static stateNMT_t CO_operation_state_handle();
static stateNMT_t CO_stopped_state_handle();
static stateNMT_t CO_nmt_handler(stateNMT_t currentState, canFrame_t *canMsg);

bool newCanMsg = true;
canFrame_t canMsg = {0};

/////////////////////////////////////////////////////////////////////
// Функция:
void CO_process(void)
{
	stateNMT_t CANOpenAppState = INIT;
	switch (CANOpenAppState)
	{
	case INIT:
	{
		CO_init_state_handle();
	}
	case RESET_APP:
	{
		CO_reset_app_state_handle();
	}
	case RESET_COMM:
	{
		CO_reset_comm_state_handle();
	}
	case PRE_OPERATIONAL:
	{
		CANOpenAppState = PRE_OPERATIONAL; // Требуется т.к. из-за просачивания состояние может быть другое.
		CANOpenAppState = CO_pre_operation_state_handle();
	}
	break;
	case OPERATIONAL:
	{
		CANOpenAppState = CO_operation_state_handle();
	}
	break;
	case STOPPED:
	{
		CANOpenAppState = CO_stopped_state_handle();
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
static void CO_reset_comm_state_handle(void)
{
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
	// сразу после входа должны отправить сообщение: cansend (интерфейс) NMT_ERR_CTRL_ID#00
	if (newCanMsg)
	{
		switch (canMsg.id)
		{
		case NMT_ID:
		{
			return CO_nmt_handler(PRE_OPERATIONAL, &canMsg);
		}
		break;
		case SDO_ID_REQUEST:
		{
			CO_sdo_handler(&canMsg);
		}
		break;
		}
	}
	return PRE_OPERATIONAL;
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
	if (newCanMsg)
	{
		switch (canMsg.id)
		{
		case NMT_ID:
		{
			return CO_nmt_handler(PRE_OPERATIONAL, &canMsg);
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
		case SDO_ID_REQUEST:
		{
			CO_sdo_handler(&canMsg);
		}
		break;
		}
	}
	return OPERATIONAL;
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
	return STOPPED;
}
/////////////////////////////////////////////////////////////////////
// Функция:
typedef enum
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
			return OPERATIONAL;
			break;
		case CS_NMT_STOPPED:
			return STOPPED;
			break;
		case CS_NMT_PRE_OPERATIONAL:
			return PRE_OPERATIONAL;
			break;
		case CS_NMT_RESET_APP:
			return RESET_APP;
			break;
		case CS_NMT_RESET_COMM:
			return RESET_COMM;
			break;
		default:
			return currentState;
			break;
		}
	}
	else
	{
		return currentState;
	}
}

