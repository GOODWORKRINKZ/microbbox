#!/bin/bash
# Скрипт для отображения статуса всех устройств

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NGINX_CONF_DIR="$SCRIPT_DIR/../nginx/conf.d"

echo "═══════════════════════════════════════════════════════════"
echo "          Статус устройств MicroRoBox"
echo "═══════════════════════════════════════════════════════════"
echo

# Проверка запущен ли nginx
if ! docker ps --format '{{.Names}}' | grep -q "^microbbox-nginx$"; then
    echo -e "${RED}✗ nginx не запущен${NC}"
    echo "Запустите: docker-compose up -d"
    exit 1
fi

echo -e "${GREEN}✓ nginx запущен${NC}"
echo

# Поиск всех конфигурационных файлов
CONF_FILES=("$NGINX_CONF_DIR"/*.conf)

if [ ${#CONF_FILES[@]} -eq 0 ] || [ ! -f "${CONF_FILES[0]}" ]; then
    echo "Нет настроенных устройств"
    echo "Добавьте устройство: sudo ./scripts/add-device.sh"
    exit 0
fi

echo "Настроенные устройства:"
echo "─────────────────────────────────────────────────────────"
printf "%-25s %-20s %-10s %-10s\n" "ДОМЕН" "IP АДРЕС" "СТАТУС" "SSL"
echo "─────────────────────────────────────────────────────────"

for conf_file in "${CONF_FILES[@]}"; do
    if [[ "$conf_file" == *.example ]]; then
        continue
    fi
    
    # Извлечение информации из конфигурации
    DOMAIN=$(grep -m 1 "server_name" "$conf_file" | awk '{print $2}' | sed 's/;//')
    UPSTREAM_LINE=$(grep -m 1 "server.*:.*max_fails" "$conf_file" || echo "")
    
    if [[ -n "$UPSTREAM_LINE" ]]; then
        IP_PORT=$(echo "$UPSTREAM_LINE" | awk '{print $2}')
        IP=$(echo "$IP_PORT" | cut -d':' -f1)
        PORT=$(echo "$IP_PORT" | cut -d':' -f2)
    else
        IP="N/A"
        PORT="N/A"
    fi
    
    # Проверка доступности устройства
    if [[ "$IP" != "N/A" ]]; then
        if timeout 2 bash -c ">/dev/tcp/$IP/$PORT" 2>/dev/null; then
            STATUS="${GREEN}✓ ONLINE${NC}"
        else
            STATUS="${RED}✗ OFFLINE${NC}"
        fi
    else
        STATUS="${YELLOW}? N/A${NC}"
    fi
    
    # Проверка SSL сертификата
    SSL_CHECK=$(docker exec microbbox-certbot certbot certificates 2>/dev/null | grep -A 5 "$DOMAIN" || echo "")
    if [[ -n "$SSL_CHECK" ]]; then
        # Извлечение даты истечения
        EXPIRY=$(echo "$SSL_CHECK" | grep "Expiry Date:" | awk '{print $3}')
        if [[ -n "$EXPIRY" ]]; then
            SSL="${GREEN}✓ $EXPIRY${NC}"
        else
            SSL="${GREEN}✓${NC}"
        fi
    else
        SSL="${RED}✗ Нет${NC}"
    fi
    
    printf "%-25s %-20s %-20s %-20s\n" "$DOMAIN" "$IP:$PORT" "$(echo -e $STATUS)" "$(echo -e $SSL)"
done

echo "─────────────────────────────────────────────────────────"
echo

# Статистика
TOTAL=$(ls -1 "$NGINX_CONF_DIR"/*.conf 2>/dev/null | grep -v ".example" | wc -l)
echo "Всего устройств: $TOTAL"
echo

# Информация о контейнерах
echo "Статус контейнеров:"
docker-compose -f "$SCRIPT_DIR/../docker-compose.yml" ps
