#!/bin/bash

# Проверяем, было ли передано достаточно аргументов
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Используйте: $0 {up|down} [скорость]"
    echo "Доступные скорости: 125000 (по умолчанию), 250000, 500000, 1000000"
    exit 1
fi

ACTION=$1
BITRATE=${2:-125000} # По умолчанию 125000, если скорость не указана

# Проверяем, что скорость является допустимым значением
valid_bitrates=(125000 250000 500000 1000000)
valid=false
for rate in "${valid_bitrates[@]}"; do
    if [ "$BITRATE" -eq "$rate" ]; then
        valid=true
        break
    fi
done

if [ "$valid" = false ]; then
    echo "Ошибка: Недопустимая скорость $BITRATE. Используйте одну из: 125000, 250000, 500000, 1000000"
    exit 1
fi

# Включаем или выключаем интерфейс CAN в зависимости от переданного аргумента
case "$ACTION" in
    up)
        echo "Настройка интерфейса can0 с битрейтом $BITRATE и включение."
        sudo ip link set can0 type can bitrate "$BITRATE"
        sudo ip link set can0 up
        ip link show can0
        echo "Интерфейс can0 активирован с битрейтом $BITRATE."
        ;;
    down)
        echo "Выключение интерфейса can0."
        sudo ip link set can0 down
        ip link show can0
        echo "Интерфейс can0 деактивирован."
        ;;
    *)
        echo "Неверный аргумент: $ACTION. Используйте 'up' или 'down'."
        exit 1
        ;;
esac