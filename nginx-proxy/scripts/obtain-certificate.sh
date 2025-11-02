#!/bin/bash
# Скрипт для получения SSL сертификата Let's Encrypt для домена

set -e

# Цвета для вывода
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

# Проверка аргументов
if [[ $# -lt 1 ]]; then
    log_error "Использование: $0 <доменное_имя> [email]"
    echo "Пример: $0 robot1.example.com admin@example.com"
    exit 1
fi

DOMAIN_NAME=$1
EMAIL=${2:-""}

# Проверка запуска с правами root
if [[ $EUID -ne 0 ]]; then
   log_error "Этот скрипт должен быть запущен с правами root (sudo)"
   exit 1
fi

log_info "=== Получение SSL сертификата Let's Encrypt ==="
echo
log_info "Домен: $DOMAIN_NAME"
if [[ -n "$EMAIL" ]]; then
    log_info "Email: $EMAIL"
fi
echo

# Проверка существования контейнера certbot
if ! docker ps -a --format '{{.Names}}' | grep -q "^microbbox-certbot$"; then
    log_error "Контейнер microbbox-certbot не найден. Запустите docker-compose up -d"
    exit 1
fi

# Построение команды certbot
CERTBOT_CMD="certbot certonly --webroot --webroot-path=/var/www/certbot"
CERTBOT_CMD="$CERTBOT_CMD --domain $DOMAIN_NAME"
CERTBOT_CMD="$CERTBOT_CMD --non-interactive --agree-tos"

if [[ -n "$EMAIL" ]]; then
    CERTBOT_CMD="$CERTBOT_CMD --email $EMAIL"
else
    CERTBOT_CMD="$CERTBOT_CMD --register-unsafely-without-email"
    log_warning "Email не указан. Сертификат будет получен без регистрации email."
fi

# Проверка доступности домена
log_info "Проверка доступности домена..."
if ! curl -s -o /dev/null -w "%{http_code}" "http://$DOMAIN_NAME/.well-known/acme-challenge/test" | grep -q "404"; then
    log_warning "Домен может быть недоступен. Убедитесь, что:"
    log_warning "1. DNS настроен корректно"
    log_warning "2. Порт 80 открыт и доступен из интернета"
    log_warning "3. nginx запущен и работает"
    echo
    read -p "Продолжить получение сертификата? (y/n): " CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        log_warning "Отменено пользователем"
        exit 0
    fi
fi

# Получение сертификата
log_info "Запрос сертификата у Let's Encrypt..."
log_info "Команда: $CERTBOT_CMD"
echo

if docker exec microbbox-certbot $CERTBOT_CMD; then
    log_info "✓ Сертификат успешно получен!"
    
    # Перезагрузка nginx для применения нового сертификата
    log_info "Перезагрузка nginx..."
    docker exec microbbox-nginx nginx -s reload
    
    echo
    log_info "Информация о сертификате:"
    docker exec microbbox-certbot certbot certificates --domain "$DOMAIN_NAME"
    
    echo
    log_info "✓ Готово! Сертификат установлен и nginx перезагружен."
    log_info "Домен $DOMAIN_NAME теперь доступен по HTTPS"
else
    log_error "Не удалось получить сертификат"
    echo
    log_warning "Возможные причины:"
    echo "  1. Домен не резолвится на IP адрес Raspberry Pi"
    echo "  2. Порт 80 недоступен из интернета"
    echo "  3. nginx не запущен или неправильно настроен"
    echo "  4. Let's Encrypt временно недоступен"
    echo
    log_info "Проверьте логи certbot:"
    echo "  docker logs microbbox-certbot"
    exit 1
fi
