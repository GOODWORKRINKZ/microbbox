#!/bin/bash
# Скрипт для удаления устройства из конфигурации

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

if [[ $EUID -ne 0 ]]; then
   log_error "Этот скрипт должен быть запущен с правами root (sudo)"
   exit 1
fi

if [[ $# -lt 1 ]]; then
    log_error "Использование: $0 <доменное_имя>"
    echo "Пример: $0 robot1.example.com"
    exit 1
fi

DOMAIN_NAME=$1
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NGINX_CONF_DIR="$SCRIPT_DIR/../nginx/conf.d"
CONF_FILE="$NGINX_CONF_DIR/${DOMAIN_NAME}.conf"

log_info "=== Удаление устройства ==="
echo

if [[ ! -f "$CONF_FILE" ]]; then
    log_error "Конфигурация для домена $DOMAIN_NAME не найдена: $CONF_FILE"
    exit 1
fi

log_warning "Будет удалена конфигурация: $CONF_FILE"
read -p "Продолжить? (y/n): " CONFIRM
if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
    log_warning "Отменено пользователем"
    exit 0
fi

# Удаление конфигурационного файла
rm -f "$CONF_FILE"
log_info "Конфигурация удалена"

# Проверка конфигурации nginx
log_info "Проверка конфигурации nginx..."
if docker exec microbbox-nginx nginx -t 2>&1 | grep -q "successful"; then
    log_info "Конфигурация nginx корректна"
else
    log_error "Ошибка в конфигурации nginx"
    exit 1
fi

# Перезагрузка nginx
log_info "Перезагрузка nginx..."
docker exec microbbox-nginx nginx -s reload

log_info "✓ Устройство успешно удалено!"
echo
log_warning "Не забудьте:"
echo "1. Удалить DNS запись на роутере для $DOMAIN_NAME"
echo "2. Удалить резервирование IP для устройства"
echo "3. При необходимости удалить SSL сертификат:"
echo "   docker exec microbbox-certbot certbot delete --cert-name $DOMAIN_NAME"
