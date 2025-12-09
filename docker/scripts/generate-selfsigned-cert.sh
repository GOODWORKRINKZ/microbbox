#!/bin/bash
# Скрипт генерации самоподписанного SSL сертификата
# Используется для локальной сети без доступа к интернету
# Использование: ./generate-selfsigned-cert.sh

set -e

# Загрузка переменных окружения
if [ ! -f ../.env ]; then
    echo "❌ Файл .env не найден!"
    echo "Запустите сначала ./setup.sh"
    exit 1
fi

cd ..
source .env

DOMAIN_NAME=${DOMAIN_NAME:-robot.example.com}
DEVICE_NAME=${DEVICE_NAME:-robot1}

echo "========================================="
echo "  Генерация самоподписанного сертификата"
echo "========================================="
echo ""
echo "Домен: $DOMAIN_NAME"
echo "Устройство: $DEVICE_NAME"
echo ""
echo "⚠️  ВАЖНО: Самоподписанные сертификаты не подходят для WebXR!"
echo "   Браузер будет показывать предупреждение о безопасности."
echo "   Для WebXR используйте Let's Encrypt сертификаты."
echo ""

read -p "Продолжить? (y/N): " confirm
if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
    echo "Отменено"
    exit 1
fi

# Создание директорий
CERT_DIR="certbot/conf/live/$DOMAIN_NAME"
mkdir -p "$CERT_DIR"

echo ""
echo "🔐 Генерация приватного ключа и сертификата..."

# Генерация приватного ключа
openssl genrsa -out "$CERT_DIR/privkey.pem" 2048

# Генерация сертификата
openssl req -new -x509 -key "$CERT_DIR/privkey.pem" \
    -out "$CERT_DIR/fullchain.pem" \
    -days 365 \
    -subj "/C=RU/ST=Moscow/L=Moscow/O=MicroRoBox/OU=IoT/CN=$DOMAIN_NAME"

if [ $? -eq 0 ]; then
    echo "✅ Сертификат успешно создан!"
    echo ""
    
    # Создание симлинков для совместимости
    ln -sf fullchain.pem "$CERT_DIR/cert.pem"
    ln -sf fullchain.pem "$CERT_DIR/chain.pem"
    
    # Установка правильных прав
    chmod 644 "$CERT_DIR/fullchain.pem"
    chmod 600 "$CERT_DIR/privkey.pem"
    
    # Создание SSL опций если их нет
    if [ ! -f certbot/conf/options-ssl-nginx.conf ]; then
        echo "📝 Загрузка SSL конфигурации..."
        wget -q -O certbot/conf/options-ssl-nginx.conf \
            https://raw.githubusercontent.com/certbot/certbot/master/certbot-nginx/certbot_nginx/_internal/tls_configs/options-ssl-nginx.conf
    fi
    
    if [ ! -f certbot/conf/ssl-dhparams.pem ]; then
        echo "📝 Загрузка DH параметров..."
        wget -q -O certbot/conf/ssl-dhparams.pem \
            https://raw.githubusercontent.com/certbot/certbot/master/certbot/certbot/ssl-dhparams.pem
    fi
    
    # Определение команды docker compose
    if docker compose version &> /dev/null 2>&1; then
        DOCKER_COMPOSE="docker compose"
    else
        DOCKER_COMPOSE="docker-compose"
    fi
    
    # Перезагрузка nginx
    echo "🔄 Перезагрузка nginx..."
    if docker exec microbbox-proxy-nginx nginx -s reload 2>/dev/null; then
        echo "✅ nginx перезагружен"
    else
        echo "⚠️  nginx еще не запущен, запустите: $DOCKER_COMPOSE up -d"
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
    echo "⚠️  При первом подключении браузер покажет предупреждение"
    echo "   о недоверенном сертификате. Это нормально для"
    echo "   самоподписанных сертификатов."
    echo ""
    echo "Для добавления сертификата в доверенные:"
    echo "  - Chrome/Edge: Зайдите на сайт, нажмите 'Дополнительно' → 'Продолжить'"
    echo "  - Firefox: Зайдите на сайт, нажмите 'Дополнительно' → 'Принять риск'"
    echo "  - Oculus Quest: Не поддерживается для WebXR"
    echo ""
    echo "Срок действия: 365 дней"
    echo "Расположение: $CERT_DIR/"
    echo ""
else
    echo ""
    echo "❌ Ошибка создания сертификата!"
    exit 1
fi
