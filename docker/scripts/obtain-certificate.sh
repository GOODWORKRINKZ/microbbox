#!/bin/bash
# Скрипт получения SSL сертификата от Let's Encrypt
# Использование: ./obtain-certificate.sh

set -e

# Определение команды docker compose
if docker compose version &> /dev/null 2>&1; then
    DOCKER_COMPOSE="docker compose"
else
    DOCKER_COMPOSE="docker-compose"
fi

# Загрузка переменных окружения
if [ ! -f ../.env ]; then
    echo "❌ Файл .env не найден!"
    echo "Запустите сначала ./setup.sh"
    exit 1
fi

cd ..
source .env

DOMAIN_NAME=${DOMAIN_NAME:-robot.example.com}
LETSENCRYPT_EMAIL=${LETSENCRYPT_EMAIL:-admin@example.com}
CERT_MODE=${CERT_MODE:-production}

echo "========================================="
echo "  Получение SSL сертификата"
echo "========================================="
echo ""
echo "Домен: $DOMAIN_NAME"
echo "Email: $LETSENCRYPT_EMAIL"
echo "Режим: $CERT_MODE"
echo ""

# Проверка что контейнеры запущены
if ! $DOCKER_COMPOSE ps | grep -q "microbbox-proxy-nginx.*Up"; then
    echo "❌ nginx контейнер не запущен!"
    echo "Запустите сначала: $DOCKER_COMPOSE up -d"
    exit 1
fi

# Проверка DNS
echo "🔍 Проверка DNS для $DOMAIN_NAME..."
if ! nslookup $DOMAIN_NAME &> /dev/null; then
    echo "⚠️  Предупреждение: DNS запись для $DOMAIN_NAME не найдена"
    echo "   Убедитесь что домен правильно настроен"
    read -p "Продолжить? (y/N): " confirm
    if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
        echo "Отменено"
        exit 1
    fi
fi

# Проверка доступности порта 80
echo "🔍 Проверка доступности порта 80..."
# Проверяем доступность через nginx health endpoint
if curl -s --connect-timeout 5 http://$DOMAIN_NAME/nginx-health &> /dev/null; then
    echo "✅ Порт 80 доступен (nginx отвечает)"
else
    echo "⚠️  Порт 80 может быть недоступен из интернета"
    echo "   Проверьте проброс портов на роутере"
    echo "   Убедитесь что nginx запущен"
fi

echo ""

# Подготовка аргументов certbot
STAGING_ARG=""
if [ "$CERT_MODE" == "staging" ]; then
    STAGING_ARG="--staging"
    echo "⚠️  Используется staging режим - сертификат не будет валидным!"
    echo "   Для продакшена установите CERT_MODE=production в .env"
    echo ""
fi

# Получение сертификата
echo "🔐 Запрос сертификата от Let's Encrypt..."
echo "Это может занять несколько минут..."
echo ""

docker exec microbbox-proxy-certbot certbot certonly \
    --webroot \
    --webroot-path=/var/www/certbot \
    --email "$LETSENCRYPT_EMAIL" \
    --agree-tos \
    --no-eff-email \
    $STAGING_ARG \
    -d "$DOMAIN_NAME" \
    --non-interactive

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Сертификат успешно получен!"
    echo ""
    
    # Перезагрузка nginx для применения сертификата
    echo "🔄 Перезагрузка nginx..."
    docker exec microbbox-proxy-nginx nginx -s reload
    
    if [ $? -eq 0 ]; then
        echo "✅ nginx перезагружен"
    else
        echo "⚠️  Не удалось перезагрузить nginx"
        echo "   Перезапустите контейнер: $DOCKER_COMPOSE restart nginx"
    fi
    
    echo ""
    echo "========================================="
    echo "  Сертификат установлен!"
    echo "========================================="
    echo ""
    echo "Теперь ваше устройство доступно по HTTPS:"
    echo "  https://$DOMAIN_NAME/"
    echo "  https://$DOMAIN_NAME/stream"
    echo ""
    echo "Сертификат будет автоматически обновляться каждые 12 часов"
    echo ""
    
    # Проверка информации о сертификате
    echo "📋 Информация о сертификате:"
    docker exec microbbox-proxy-certbot certbot certificates
    
else
    echo ""
    echo "❌ Ошибка получения сертификата!"
    echo ""
    echo "Возможные причины:"
    echo "  1. Домен не резолвится на IP вашего хоста"
    echo "  2. Порт 80 недоступен из интернета"
    echo "  3. Превышен лимит запросов Let's Encrypt"
    echo "  4. Неправильный email адрес"
    echo ""
    echo "Проверьте логи для подробностей:"
    echo "  $DOCKER_COMPOSE logs certbot"
    echo ""
    echo "Для тестирования используйте staging режим:"
    echo "  CERT_MODE=staging в .env файле"
    exit 1
fi
