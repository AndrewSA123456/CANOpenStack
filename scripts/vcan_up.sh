#!/bin/bash

# Проверяем, был ли передан аргумент
if [ "$#" -ne 1 ]; then
    echo "Используйте: $0 {up|down}"
    exit 1
fi

ACTION=$1

# Включаем или выключаем интерфейс CAN в зависимости от переданного аргумента
case "$ACTION" in
    up)
        echo "Создание  и включение интерфейса vcan0."
        sudo ip link add dev vcan0 type vcan
        sudo ip link set vcan0 up
        ip link show vcan0
        echo "Интерфейс vcan0 активирован."
        ;;
    down)
        echo "Выключение интерфейса vcan0."
        sudo ip link set vcan0 down
        ip link show vcan0
        echo "Интерфейс vcan0 деактивирован."
        ;;
    *)
        echo "Неверный аргумент: $ACTION. Используйте 'up' или 'down'."
        exit 1
        ;;
esac