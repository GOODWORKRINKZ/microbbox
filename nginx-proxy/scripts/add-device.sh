#!/bin/bash
# Скрипт для добавления нового ESP32 устройства в nginx конфигурацию

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Функция для вывода сообщений
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Проверка запуска с правами root
if [[ $EUID -ne 0 ]]; then
   log_error "Этот скрипт должен быть запущен с правами root (sudo)"
   exit 1
fi

# Получаем директорию скрипта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NGINX_CONF_DIR="$SCRIPT_DIR/../nginx/conf.d"
TEMPLATE_FILE="$SCRIPT_DIR/../nginx/templates/device-template.conf"

log_info "=== Добавление нового ESP32 устройства ==="
echo

# Запрос параметров устройства
read -p "Введите имя устройства (например, robot1): " DEVICE_NAME
if [[ -z "$DEVICE_NAME" ]]; then
    log_error "Имя устройства не может быть пустым"
    exit 1
fi

read -p "Введите IP адрес устройства (например, 192.168.1.100): " DEVICE_IP
if [[ -z "$DEVICE_IP" ]]; then
    log_error "IP адрес не может быть пустым"
    exit 1
fi

# Валидация IP адреса
if ! [[ $DEVICE_IP =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
    log_error "Некорректный IP адрес"
    exit 1
fi

read -p "Введите доменное имя (например, robot1.example.com): " DOMAIN_NAME
if [[ -z "$DOMAIN_NAME" ]]; then
    log_error "Доменное имя не может быть пустым"
    exit 1
fi

read -p "Введите порт API (по умолчанию 80): " API_PORT
API_PORT=${API_PORT:-80}

read -p "Введите порт видеопотока (по умолчанию 81): " STREAM_PORT
STREAM_PORT=${STREAM_PORT:-81}

echo
log_info "Параметры устройства:"
echo "  Имя: $DEVICE_NAME"
echo "  IP: $DEVICE_IP"
echo "  Домен: $DOMAIN_NAME"
echo "  Порт API: $API_PORT"
echo "  Порт Stream: $STREAM_PORT"
echo

read -p "Продолжить? (y/n): " CONFIRM
if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
    log_warning "Отменено пользователем"
    exit 0
fi

# Проверка существования конфигурационного файла
CONF_FILE="$NGINX_CONF_DIR/${DOMAIN_NAME}.conf"
if [[ -f "$CONF_FILE" ]]; then
    log_error "Конфигурация для домена $DOMAIN_NAME уже существует: $CONF_FILE"
    exit 1
fi

# Создание конфигурации из шаблона
log_info "Создание конфигурации из шаблона..."
if [[ ! -f "$TEMPLATE_FILE" ]]; then
    log_error "Шаблон не найден: $TEMPLATE_FILE"
    exit 1
fi

# Копирование и замена переменных
sed -e "s/DEVICE_NAME/$DEVICE_NAME/g" \
    -e "s/DEVICE_IP/$DEVICE_IP/g" \
    -e "s/DOMAIN_NAME/$DOMAIN_NAME/g" \
    -e "s/API_PORT/$API_PORT/g" \
    -e "s/STREAM_PORT/$STREAM_PORT/g" \
    "$TEMPLATE_FILE" > "$CONF_FILE"

log_info "Конфигурация создана: $CONF_FILE"

# Проверка конфигурации nginx
log_info "Проверка конфигурации nginx..."
if docker exec microbbox-nginx nginx -t 2>&1 | grep -q "successful"; then
    log_info "Конфигурация nginx корректна"
else
    log_error "Ошибка в конфигурации nginx"
    log_warning "Удаление созданного файла конфигурации..."
    rm -f "$CONF_FILE"
    exit 1
fi

# Перезагрузка nginx
log_info "Перезагрузка nginx..."
docker exec microbbox-nginx nginx -s reload

log_info "✓ Устройство $DEVICE_NAME успешно добавлено!"
echo
log_warning "Не забудьте:"
echo "1. Настроить DNS на роутере для домена $DOMAIN_NAME -> IP Raspberry Pi"
echo "2. Зарезервировать IP адрес $DEVICE_IP для устройства на роутере"
echo "3. Получить SSL сертификат: ./obtain-certificate.sh $DOMAIN_NAME"
echo
