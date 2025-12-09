#!/bin/bash
# Скрипт настройки Docker композиции для ESP32 HTTPS proxy
# Использование: ./setup.sh

set -e

echo "========================================="
echo "  Настройка ESP32 HTTPS Proxy"
echo "========================================="
echo ""

# Проверка наличия Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker не установлен!"
    echo "Установите Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

# Проверка наличия Docker Compose
if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
    echo "❌ Docker Compose не установлен!"
    echo "Установите Docker Compose: https://docs.docker.com/compose/install/"
    exit 1
fi

# Определение команды docker compose
if docker compose version &> /dev/null; then
    DOCKER_COMPOSE="docker compose"
else
    DOCKER_COMPOSE="docker-compose"
fi

echo "✅ Docker установлен"
echo "✅ Docker Compose установлен"
echo ""

# Проверка существования .env файла
if [ ! -f .env ]; then
    echo "📝 Создание файла конфигурации .env из шаблона..."
    cp .env.example .env
    echo "✅ Файл .env создан"
    echo ""
    echo "⚠️  ВАЖНО: Отредактируйте файл .env перед продолжением!"
    echo "   Необходимо указать:"
    echo "   - ESP32_IP: IP адрес вашего ESP32 устройства"
    echo "   - DOMAIN_NAME: доменное имя для доступа"
    echo "   - LETSENCRYPT_EMAIL: email для уведомлений"
    echo ""
    read -p "Нажмите Enter после редактирования .env файла..."
fi

# Загрузка переменных окружения
source .env

# Проверка обязательных параметров
if [ -z "$ESP32_IP" ] || [ "$ESP32_IP" == "192.168.1.100" ]; then
    echo "⚠️  Предупреждение: ESP32_IP не настроен или использует значение по умолчанию"
    read -p "IP адрес ESP32 [192.168.1.100]: " user_ip
    if [ ! -z "$user_ip" ]; then
        ESP32_IP=$user_ip
        sed -i "s/ESP32_IP=.*/ESP32_IP=$ESP32_IP/" .env
    fi
fi

if [ -z "$DOMAIN_NAME" ] || [ "$DOMAIN_NAME" == "robot.example.com" ]; then
    echo "⚠️  Предупреждение: DOMAIN_NAME не настроен или использует значение по умолчанию"
    read -p "Доменное имя (например, robot.example.com): " user_domain
    if [ ! -z "$user_domain" ]; then
        DOMAIN_NAME=$user_domain
        sed -i "s/DOMAIN_NAME=.*/DOMAIN_NAME=$DOMAIN_NAME/" .env
    fi
fi

# Установка значений по умолчанию если не указаны
ESP32_API_PORT=${ESP32_API_PORT:-80}
ESP32_STREAM_PORT=${ESP32_STREAM_PORT:-81}
DEVICE_NAME=${DEVICE_NAME:-robot1}
CERT_MODE=${CERT_MODE:-production}

echo ""
echo "Конфигурация:"
echo "  ESP32 IP: $ESP32_IP"
echo "  API порт: $ESP32_API_PORT"
echo "  Stream порт: $ESP32_STREAM_PORT"
echo "  Доменное имя: $DOMAIN_NAME"
echo "  Имя устройства: $DEVICE_NAME"
echo "  Режим сертификата: $CERT_MODE"
echo ""

# Создание конфигурации nginx из шаблона
echo "📝 Создание конфигурации nginx..."
cp nginx/device-template.conf nginx/conf.d/${DOMAIN_NAME}.conf

# Замена переменных в конфигурации
sed -i "s/ESP32_IP/$ESP32_IP/g" nginx/conf.d/${DOMAIN_NAME}.conf
sed -i "s/ESP32_API_PORT/$ESP32_API_PORT/g" nginx/conf.d/${DOMAIN_NAME}.conf
sed -i "s/ESP32_STREAM_PORT/$ESP32_STREAM_PORT/g" nginx/conf.d/${DOMAIN_NAME}.conf
sed -i "s/DOMAIN_NAME/$DOMAIN_NAME/g" nginx/conf.d/${DOMAIN_NAME}.conf
sed -i "s/DEVICE_NAME/$DEVICE_NAME/g" nginx/conf.d/${DOMAIN_NAME}.conf

echo "✅ Конфигурация nginx создана: nginx/conf.d/${DOMAIN_NAME}.conf"
echo ""

# Создание необходимых директорий
echo "📁 Создание директорий..."
mkdir -p certbot/{conf,www,logs} nginx/logs
echo "✅ Директории созданы"
echo ""

# Проверка доступности ESP32
echo "🔍 Проверка доступности ESP32..."
if ping -c 1 -W 2 $ESP32_IP &> /dev/null; then
    echo "✅ ESP32 доступен по адресу $ESP32_IP"
    
    # Проверка портов
    if curl -s --connect-timeout 5 http://$ESP32_IP:$ESP32_API_PORT/ &> /dev/null; then
        echo "✅ API порт $ESP32_API_PORT доступен"
    else
        echo "⚠️  API порт $ESP32_API_PORT недоступен"
    fi
    
    if curl -s --connect-timeout 5 http://$ESP32_IP:$ESP32_STREAM_PORT/ &> /dev/null; then
        echo "✅ Stream порт $ESP32_STREAM_PORT доступен"
    else
        echo "⚠️  Stream порт $ESP32_STREAM_PORT недоступен"
    fi
else
    echo "⚠️  ESP32 недоступен по адресу $ESP32_IP"
    echo "   Убедитесь что устройство включено и подключено к сети"
fi
echo ""

# Запуск контейнеров
echo "🚀 Запуск Docker контейнеров..."
$DOCKER_COMPOSE up -d

# Ожидание запуска
echo "⏳ Ожидание запуска контейнеров..."
sleep 5

# Проверка статуса
echo ""
echo "📊 Статус контейнеров:"
$DOCKER_COMPOSE ps

echo ""
echo "========================================="
echo "  Настройка завершена!"
echo "========================================="
echo ""
echo "📋 Следующие шаги:"
echo ""

if [ "$CERT_MODE" == "selfsigned" ]; then
    echo "1. Генерация самоподписанного сертификата:"
    echo "   ./scripts/generate-selfsigned-cert.sh"
    echo ""
elif [ "$CERT_MODE" == "production" ] || [ "$CERT_MODE" == "staging" ]; then
    echo "1. Настройте DNS запись для домена $DOMAIN_NAME"
    echo "   A-запись должна указывать на внешний IP вашей сети"
    echo ""
    echo "2. Настройте проброс портов на роутере:"
    echo "   80 -> IP этого хоста"
    echo "   443 -> IP этого хоста"
    echo ""
    echo "3. Получите SSL сертификат:"
    echo "   ./scripts/obtain-certificate.sh"
    echo ""
fi

echo "Доступ к устройству:"
if [ "$CERT_MODE" == "selfsigned" ] || [ -d "certbot/conf/live/$DOMAIN_NAME" ]; then
    echo "  HTTPS: https://$DOMAIN_NAME/"
    echo "  Видеопоток: https://$DOMAIN_NAME/stream"
else
    echo "  HTTP: http://$DOMAIN_NAME/ (доступен до получения сертификата)"
    echo "  После получения сертификата будет автоматический редирект на HTTPS"
fi
echo ""
echo "Управление:"
echo "  Просмотр логов: $DOCKER_COMPOSE logs -f"
echo "  Остановка: $DOCKER_COMPOSE down"
echo "  Перезапуск: $DOCKER_COMPOSE restart"
echo ""
