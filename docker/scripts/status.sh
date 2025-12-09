#!/bin/bash
# Скрипт проверки статуса ESP32 HTTPS Proxy
# Использование: ./status.sh

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Определение команды docker compose
if docker compose version &> /dev/null 2>&1; then
    DOCKER_COMPOSE="docker compose"
else
    DOCKER_COMPOSE="docker-compose"
fi

echo "========================================="
echo "  ESP32 HTTPS Proxy - Статус системы"
echo "========================================="
echo ""

# Загрузка переменных окружения
if [ ! -f ../.env ]; then
    echo -e "${RED}❌ Файл .env не найден!${NC}"
    echo "Запустите сначала ./setup.sh"
    exit 1
fi

cd ..
source .env

ESP32_IP=${ESP32_IP:-192.168.1.100}
ESP32_API_PORT=${ESP32_API_PORT:-80}
ESP32_STREAM_PORT=${ESP32_STREAM_PORT:-81}
DOMAIN_NAME=${DOMAIN_NAME:-robot.example.com}
DEVICE_NAME=${DEVICE_NAME:-robot1}

# Проверка Docker контейнеров
echo -e "${BLUE}📦 Docker Контейнеры:${NC}"
$DOCKER_COMPOSE ps
echo ""

# Проверка статуса nginx
if $DOCKER_COMPOSE ps | grep -q "microbbox-proxy-nginx.*Up"; then
    echo -e "${GREEN}✅ nginx запущен${NC}"
    
    # Проверка конфигурации
    if docker exec microbbox-proxy-nginx nginx -t &> /dev/null; then
        echo -e "${GREEN}✅ Конфигурация nginx корректна${NC}"
    else
        echo -e "${RED}❌ Ошибка в конфигурации nginx${NC}"
        docker exec microbbox-proxy-nginx nginx -t
    fi
else
    echo -e "${RED}❌ nginx не запущен${NC}"
fi
echo ""

# Проверка статуса certbot
if $DOCKER_COMPOSE ps | grep -q "microbbox-proxy-certbot.*Up"; then
    echo -e "${GREEN}✅ certbot запущен${NC}"
else
    echo -e "${RED}❌ certbot не запущен${NC}"
fi
echo ""

# Проверка SSL сертификатов
echo -e "${BLUE}🔐 SSL Сертификаты:${NC}"
if [ -d "certbot/conf/live/$DOMAIN_NAME" ]; then
    echo -e "${GREEN}✅ Сертификат найден для $DOMAIN_NAME${NC}"
    
    # Проверка срока действия
    if docker exec microbbox-proxy-certbot certbot certificates 2>/dev/null | grep -q "$DOMAIN_NAME"; then
        echo ""
        docker exec microbbox-proxy-certbot certbot certificates 2>/dev/null | grep -A 5 "$DOMAIN_NAME"
    fi
else
    echo -e "${YELLOW}⚠️  Сертификат не найден для $DOMAIN_NAME${NC}"
    echo "   Запустите: ./scripts/obtain-certificate.sh"
    echo "   или: ./scripts/generate-selfsigned-cert.sh"
fi
echo ""

# Проверка доступности ESP32
echo -e "${BLUE}🤖 ESP32 Устройство:${NC}"
echo "IP: $ESP32_IP"

# Ping тест
if ping -c 1 -W 2 $ESP32_IP &> /dev/null; then
    echo -e "${GREEN}✅ Устройство отвечает на ping${NC}"
else
    echo -e "${RED}❌ Устройство не отвечает на ping${NC}"
fi

# Проверка API порта
if curl -s --connect-timeout 3 http://$ESP32_IP:$ESP32_API_PORT/ &> /dev/null; then
    echo -e "${GREEN}✅ API порт $ESP32_API_PORT доступен${NC}"
else
    echo -e "${RED}❌ API порт $ESP32_API_PORT недоступен${NC}"
fi

# Проверка Stream порта
if curl -s --connect-timeout 3 http://$ESP32_IP:$ESP32_STREAM_PORT/ &> /dev/null; then
    echo -e "${GREEN}✅ Stream порт $ESP32_STREAM_PORT доступен${NC}"
else
    echo -e "${RED}❌ Stream порт $ESP32_STREAM_PORT недоступен${NC}"
fi
echo ""

# Проверка доступа через proxy
echo -e "${BLUE}🌐 Доступ через Proxy:${NC}"

# Проверка HTTP
if curl -s --connect-timeout 3 http://localhost/ &> /dev/null; then
    echo -e "${GREEN}✅ HTTP (localhost) доступен${NC}"
else
    echo -e "${RED}❌ HTTP (localhost) недоступен${NC}"
fi

# Проверка HTTPS
if [ -d "certbot/conf/live/$DOMAIN_NAME" ]; then
    if curl -sk --connect-timeout 3 https://localhost/ &> /dev/null; then
        echo -e "${GREEN}✅ HTTPS (localhost) доступен${NC}"
    else
        echo -e "${RED}❌ HTTPS (localhost) недоступен${NC}"
    fi
fi
echo ""

# DNS проверка
echo -e "${BLUE}🌍 DNS:${NC}"
# Попробуем использовать dig, если доступен, иначе nslookup
if command -v dig &> /dev/null; then
    DNS_IP=$(dig +short $DOMAIN_NAME 2>/dev/null | head -1)
    if [ ! -z "$DNS_IP" ]; then
        echo -e "${GREEN}✅ DNS резолвится: $DOMAIN_NAME → $DNS_IP${NC}"
    else
        echo -e "${YELLOW}⚠️  DNS запись для $DOMAIN_NAME не найдена${NC}"
        echo "   Добавьте в локальный DNS или /etc/hosts"
    fi
elif nslookup $DOMAIN_NAME &> /dev/null; then
    # Используем более надежный способ парсинга nslookup
    DNS_IP=$(nslookup $DOMAIN_NAME 2>/dev/null | grep -v '#' | grep 'Address:' | tail -1 | awk '{print $2}')
    if [ ! -z "$DNS_IP" ] && [ "$DNS_IP" != "" ]; then
        echo -e "${GREEN}✅ DNS резолвится: $DOMAIN_NAME → $DNS_IP${NC}"
    else
        echo -e "${YELLOW}⚠️  DNS найден, но IP не определен${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  DNS запись для $DOMAIN_NAME не найдена${NC}"
    echo "   Добавьте в локальный DNS или /etc/hosts"
fi
echo ""

# Использование ресурсов
echo -e "${BLUE}💻 Использование ресурсов:${NC}"
docker stats --no-stream microbbox-proxy-nginx microbbox-proxy-certbot 2>/dev/null || echo "Статистика недоступна"
echo ""

# Последние ошибки из логов
echo -e "${BLUE}📋 Последние ошибки (если есть):${NC}"
if [ -f "nginx/logs/${DEVICE_NAME}-error.log" ]; then
    ERROR_COUNT=$(grep -c "error" nginx/logs/${DEVICE_NAME}-error.log 2>/dev/null || echo "0")
    if [ "$ERROR_COUNT" -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Найдено ошибок: $ERROR_COUNT${NC}"
        echo "Последние 5 ошибок:"
        tail -5 nginx/logs/${DEVICE_NAME}-error.log
    else
        echo -e "${GREEN}✅ Ошибок не обнаружено${NC}"
    fi
else
    echo "Файл логов не найден"
fi
echo ""

# Итоговая информация
echo "========================================="
echo -e "${BLUE}📌 Информация для доступа:${NC}"
echo "========================================="
echo ""
if [ -d "certbot/conf/live/$DOMAIN_NAME" ]; then
    echo "🌐 Веб-интерфейс: https://$DOMAIN_NAME/"
    echo "📹 Видеопоток:    https://$DOMAIN_NAME/stream"
else
    echo "🌐 Веб-интерфейс: http://$DOMAIN_NAME/ (HTTP пока нет SSL)"
    echo "📹 Видеопоток:    http://$DOMAIN_NAME/stream (HTTP пока нет SSL)"
fi
echo ""
echo "Логи в реальном времени: $DOCKER_COMPOSE logs -f"
echo "Управление: $DOCKER_COMPOSE {start|stop|restart|ps}"
echo ""
