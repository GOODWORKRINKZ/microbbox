#!/bin/bash
# Скрипт остановки и очистки Docker композиции
# Использование: ./cleanup.sh [--full]

set -e

# Определение команды docker compose
if docker compose version &> /dev/null 2>&1; then
    DOCKER_COMPOSE="docker compose"
else
    DOCKER_COMPOSE="docker-compose"
fi

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================="
echo "  Остановка ESP32 HTTPS Proxy"
echo "========================================="
echo ""

cd ..

# Проверка параметров
FULL_CLEANUP=false
if [ "$1" == "--full" ]; then
    FULL_CLEANUP=true
fi

# Остановка контейнеров
echo -e "${BLUE}📦 Остановка Docker контейнеров...${NC}"
if $DOCKER_COMPOSE ps | grep -q "Up"; then
    $DOCKER_COMPOSE down
    echo -e "${GREEN}✅ Контейнеры остановлены${NC}"
else
    echo -e "${YELLOW}⚠️  Контейнеры уже остановлены${NC}"
fi
echo ""

# Полная очистка если указан флаг --full
if [ "$FULL_CLEANUP" = true ]; then
    echo -e "${YELLOW}⚠️  ВНИМАНИЕ: Выполняется полная очистка!${NC}"
    echo ""
    
    # Подтверждение
    read -p "Это удалит сертификаты, логи и конфигурацию. Продолжить? (y/N): " confirm
    if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
        echo "Отменено"
        exit 0
    fi
    
    echo ""
    echo -e "${BLUE}🗑️  Удаление volumes...${NC}"
    $DOCKER_COMPOSE down -v
    echo -e "${GREEN}✅ Volumes удалены${NC}"
    echo ""
    
    echo -e "${BLUE}🗑️  Удаление сертификатов...${NC}"
    if [ -d "certbot/conf" ]; then
        rm -rf certbot/conf/*
        echo -e "${GREEN}✅ Сертификаты удалены${NC}"
    else
        echo -e "${YELLOW}⚠️  Директория certbot/conf не найдена${NC}"
    fi
    echo ""
    
    echo -e "${BLUE}🗑️  Удаление логов...${NC}"
    if [ -d "nginx/logs" ]; then
        rm -rf nginx/logs/*
        echo -e "${GREEN}✅ Логи удалены${NC}"
    else
        echo -e "${YELLOW}⚠️  Директория nginx/logs не найдена${NC}"
    fi
    if [ -d "certbot/logs" ]; then
        rm -rf certbot/logs/*
        echo -e "${GREEN}✅ Логи certbot удалены${NC}"
    fi
    echo ""
    
    echo -e "${BLUE}🗑️  Удаление сгенерированной конфигурации...${NC}"
    if [ -d "nginx/conf.d" ]; then
        rm -rf nginx/conf.d/*.conf
        echo -e "${GREEN}✅ Конфигурация nginx удалена${NC}"
    else
        echo -e "${YELLOW}⚠️  Директория nginx/conf.d не найдена${NC}"
    fi
    echo ""
    
    # Опционально удалить .env
    if [ -f ".env" ]; then
        read -p "Удалить файл .env? (y/N): " confirm_env
        if [ "$confirm_env" == "y" ] || [ "$confirm_env" == "Y" ]; then
            rm .env
            echo -e "${GREEN}✅ Файл .env удален${NC}"
        else
            echo -e "${YELLOW}⚠️  Файл .env сохранен${NC}"
        fi
    fi
    echo ""
    
    echo -e "${GREEN}✅ Полная очистка завершена${NC}"
else
    echo -e "${BLUE}ℹ️  Для полной очистки (удаление сертификатов, логов, конфигурации) используйте:${NC}"
    echo -e "   ${YELLOW}./scripts/cleanup.sh --full${NC}"
fi

echo ""
echo "========================================="
echo -e "${GREEN}  Остановка завершена!${NC}"
echo "========================================="
echo ""
echo "Для запуска снова:"
echo "  $DOCKER_COMPOSE up -d"
echo ""
echo "Для полной переустановки:"
echo "  ./scripts/setup.sh"
echo ""
