# Примеры использования

Этот документ содержит практические примеры настройки nginx proxy для различных сценариев использования MicroRoBox.

## Сценарий 1: Один робот для личного использования

### Требования
- 1 ESP32-CAM устройство
- Домашняя сеть WiFi
- Raspberry Pi Zero
- Домен (например, myrobot.example.com)

### Настройка

```bash
# 1. Запуск nginx и certbot
cd ~/microbbox/infrastructure/vr-proxy
docker-compose up -d

# 2. Резервирование IP на роутере для ESP32
# Web: 192.168.1.1 → Устройства → MICROBBOX-XXXXXX → IP: 192.168.1.100

# 3. Настройка DNS на роутере
telnet my.keenetic.net
ip name myrobot.example.com 192.168.1.50
system configuration save

# 4. Добавление устройства в nginx
sudo ./scripts/add-device.sh
# Имя: myrobot
# IP: 192.168.1.100
# Домен: myrobot.example.com

# 5. Проброс портов (для Let's Encrypt)
# Web роутера: 80 → 192.168.1.50:80, 443 → 192.168.1.50:443

# 6. Настройка A-записи в DNS домена
# myrobot.example.com → YOUR_PUBLIC_IP

# 7. Получение SSL сертификата
sudo ./scripts/obtain-certificate.sh myrobot.example.com you@email.com

# 8. Проверка
curl -I https://myrobot.example.com
```

### Использование
- Локальная сеть: `https://myrobot.example.com`
- Через интернет: `https://myrobot.example.com`
- VR в Oculus Quest: открыть в Oculus Browser → кнопка 🥽 VR

---

## Сценарий 2: Несколько роботов в образовательном учреждении

### Требования
- 5 ESP32-CAM устройств
- Raspberry Pi 4 (для лучшей производительности)
- Домен с поддоменами (robot1-5.school.edu)
- Локальная сеть

### Настройка

```bash
# 1. Резервирование IP адресов
# Роутер → DHCP:
# robot1: 192.168.1.101
# robot2: 192.168.1.102
# robot3: 192.168.1.103
# robot4: 192.168.1.104
# robot5: 192.168.1.105

# 2. Настройка DNS (все указывают на Raspberry Pi)
telnet my.keenetic.net
ip name robot1.school.edu 192.168.1.50
ip name robot2.school.edu 192.168.1.50
ip name robot3.school.edu 192.168.1.50
ip name robot4.school.edu 192.168.1.50
ip name robot5.school.edu 192.168.1.50
system configuration save

# 3. Массовое добавление устройств (скрипт)
for i in {1..5}; do
  sudo ./scripts/add-device.sh <<EOF
robot$i
192.168.1.10$i
robot$i.school.edu
80
81
y
EOF
done

# 4. Получение сертификатов для всех
for i in {1..5}; do
  sudo ./scripts/obtain-certificate.sh robot$i.school.edu admin@school.edu
done

# 5. Проверка статуса всех устройств
./scripts/status.sh
```

### Организация
- Каждому классу/группе выделить свой робот
- Использовать QR коды для быстрого доступа
- Создать инструкции для учеников

**Пример QR кода:**
```
URL: https://robot1.school.edu
Описание: Робот команды "Альфа"
```

---

## Сценарий 3: Демонстрация на выставке

### Требования
- 3 ESP32-CAM устройства
- Raspberry Pi 3B+
- Портативный роутер с 4G
- Временный домен

### Особенности
- Мобильный интернет через 4G роутер
- Локальная сеть для демонстрации
- Возможность удаленного доступа

### Настройка

```bash
# 1. Настройка портативной сети
# SSID: MicroRoBox-Demo
# Password: Demo2024!

# 2. Статические IP для стабильности
# Raspberry Pi: 192.168.43.1
# Robot1: 192.168.43.101
# Robot2: 192.168.43.102
# Robot3: 192.168.43.103

# 3. Локальный DNS
telnet 192.168.43.1
ip name demo1.local 192.168.43.1
ip name demo2.local 192.168.43.1
ip name demo3.local 192.168.43.1

# 4. Добавление устройств
sudo ./scripts/add-device.sh
# demo1, 192.168.43.101, demo1.local, 80, 81

sudo ./scripts/add-device.sh
# demo2, 192.168.43.102, demo2.local, 80, 81

sudo ./scripts/add-device.sh
# demo3, 192.168.43.103, demo3.local, 80, 81

# 5. Для выставки используем самоподписанные сертификаты
# (если нет возможности получить Let's Encrypt)
docker exec microbbox-nginx openssl req -x509 -nodes -days 7 \
  -newkey rsa:2048 \
  -keyout /etc/ssl/private/demo.key \
  -out /etc/ssl/certs/demo.crt \
  -subj "/CN=demo.local"
```

### Презентация
- Подключите VR гарнитуры к WiFi сети MicroRoBox-Demo
- Раздайте карточки с URL и инструкциями
- Обеспечьте техническую поддержку

---

## Сценарий 4: Исследовательская лаборатория

### Требования
- 10+ ESP32-CAM устройств
- Высокопроизводительный сервер (не Raspberry Pi)
- Корпоративная сеть
- Мониторинг и логирование

### Настройка

```bash
# 1. Использование расширенной конфигурации с мониторингом
docker-compose -f docker-compose.yml \
               -f docker-compose.monitoring.yml \
               --profile monitoring up -d

# 2. Организация по группам
# Группа A: 192.168.10.101-110 (lab-a-01 до lab-a-10)
# Группа B: 192.168.10.111-120 (lab-b-01 до lab-b-10)

# 3. Массовое добавление с помощью скрипта
#!/bin/bash
for group in a b; do
  start=$((group == 'a' ? 1 : 11))
  for i in {1..10}; do
    num=$(printf "%02d" $i)
    ip=$((100 + start + i - 1))
    sudo ./scripts/add-device.sh <<EOF
lab-$group-$num
192.168.10.$ip
lab-$group-$num.research.local
80
81
y
EOF
  done
done

# 4. Настройка Grafana для мониторинга
# http://SERVER_IP:3000
# Логин: admin / admin

# 5. Настройка алертов для недоступных устройств
```

### Дополнительные возможности

**nginx конфигурация с Basic Auth:**
```nginx
# Добавить в nginx/conf.d/lab-a-01.research.local.conf
location / {
    auth_basic "Research Lab Access";
    auth_basic_user_file /etc/nginx/.htpasswd;
    proxy_pass http://lab_a_01_api;
    # ...
}
```

**Создание пользователей:**
```bash
# На сервере
htpasswd -c nginx/.htpasswd researcher1
htpasswd nginx/.htpasswd researcher2
docker exec microbbox-nginx nginx -s reload
```

---

## Сценарий 5: Соревнования роботов

### Требования
- Переменное количество устройств (участники приносят свои)
- Быстрая регистрация новых роботов
- Изолированные сети для команд
- Прямой доступ судей

### Настройка

```bash
# 1. Создание сетевых сегментов на роутере
# Team1: 192.168.1.0/24
# Team2: 192.168.2.0/24
# Judges: 192.168.0.0/24

# 2. Raspberry Pi в судейской сети
# IP: 192.168.0.50

# 3. Скрипт быстрой регистрации команды
#!/bin/bash
# register-team.sh

TEAM_NAME=$1
TEAM_IP=$2

cat > nginx/conf.d/${TEAM_NAME}.conf <<EOF
upstream ${TEAM_NAME}_api {
    server ${TEAM_IP}:80 max_fails=3 fail_timeout=30s;
}

server {
    listen 80;
    server_name ${TEAM_NAME}.competition.local;
    
    location / {
        proxy_pass http://${TEAM_NAME}_api;
        proxy_set_header Host \$host;
        # ... остальные настройки
    }
}
EOF

docker exec microbbox-nginx nginx -s reload
echo "Team $TEAM_NAME registered at http://${TEAM_NAME}.competition.local"

# 4. Использование
sudo ./register-team.sh team-alpha 192.168.1.101
sudo ./register-team.sh team-beta 192.168.2.101
```

### Судейская панель

Создайте страницу с ссылками на все команды:
```html
<!-- judges-panel.html -->
<!DOCTYPE html>
<html>
<head>
    <title>Судейская панель</title>
    <style>
        .team { display: inline-block; margin: 10px; }
        iframe { border: 2px solid #333; }
    </style>
</head>
<body>
    <h1>Соревнование роботов - Судейская панель</h1>
    <div class="team">
        <h3>Team Alpha</h3>
        <iframe src="https://team-alpha.competition.local" width="400" height="300"></iframe>
    </div>
    <div class="team">
        <h3>Team Beta</h3>
        <iframe src="https://team-beta.competition.local" width="400" height="300"></iframe>
    </div>
    <!-- ... -->
</body>
</html>
```

---

## Сценарий 6: Удаленное управление через интернет

### Требования
- Доступ из любой точки мира
- Защита от несанкционированного доступа
- Ограничение по IP или паролю

### Настройка безопасности

```bash
# 1. Создание пользователей для Basic Auth
htpasswd -c nginx/.htpasswd owner
htpasswd nginx/.htpasswd friend1
htpasswd nginx/.htpasswd friend2

# 2. Модификация конфигурации nginx
nano nginx/conf.d/myrobot.example.com.conf
```

```nginx
# Добавить в server блок
# Ограничение доступа только с определенных IP
location / {
    # Разрешить с домашнего IP
    allow 203.0.113.0/24;
    # Разрешить с рабочего IP
    allow 198.51.100.5;
    # Остальным требовать пароль
    satisfy any;
    
    auth_basic "Robot Access";
    auth_basic_user_file /etc/nginx/.htpasswd;
    
    proxy_pass http://myrobot_api;
    # ...
}

# Видеопоток всегда требует авторизацию
location /stream {
    auth_basic "Robot Stream";
    auth_basic_user_file /etc/nginx/.htpasswd;
    proxy_pass http://myrobot_stream/;
    # ...
}
```

```bash
# 3. Перезагрузка nginx
docker exec microbbox-nginx nginx -s reload
```

### Дополнительная защита

**Rate limiting:**
```nginx
http {
    limit_req_zone $binary_remote_addr zone=robotlimit:10m rate=10r/s;
    
    server {
        location / {
            limit_req zone=robotlimit burst=20;
            # ...
        }
    }
}
```

**Логирование всех доступов:**
```nginx
access_log /var/log/nginx/myrobot-access.log;
error_log /var/log/nginx/myrobot-error.log;
```

---

## Сценарий 7: Тестирование и разработка

### Требования
- Быстрое добавление/удаление устройств
- Тестовые сертификаты (staging)
- Легкий откат изменений

### Настройка

```bash
# 1. Использование отдельной конфигурации для разработки
cp docker-compose.yml docker-compose.dev.yml

# 2. Изменение портов (чтобы не конфликтовать с production)
nano docker-compose.dev.yml
# Изменить порты: 8080:80 и 8443:443

# 3. Запуск dev окружения
docker-compose -f docker-compose.dev.yml up -d

# 4. Использование staging Let's Encrypt
docker exec microbbox-certbot certbot certonly \
  --webroot \
  --webroot-path=/var/www/certbot \
  --staging \
  --email dev@example.com \
  --agree-tos \
  --domain test-robot.example.com

# 5. Быстрое тестирование конфигураций
# Создайте test-config.conf
nano nginx/conf.d/test-robot.conf

# Проверка без перезагрузки
docker exec microbbox-nginx nginx -t

# Если OK, перезагрузка
docker exec microbbox-nginx nginx -s reload

# Если ошибка, откат
git checkout nginx/conf.d/test-robot.conf
```

### Автоматизация тестирования

```bash
#!/bin/bash
# test-device.sh - Тестирование конфигурации устройства

DEVICE_NAME=$1
DEVICE_IP=$2

echo "Testing $DEVICE_NAME at $DEVICE_IP..."

# 1. Ping тест
if ping -c 1 $DEVICE_IP > /dev/null 2>&1; then
    echo "✓ Device is reachable"
else
    echo "✗ Device is not reachable"
    exit 1
fi

# 2. HTTP тест
if curl -f -s http://$DEVICE_IP > /dev/null; then
    echo "✓ HTTP port 80 is accessible"
else
    echo "✗ HTTP port 80 is not accessible"
    exit 1
fi

# 3. Stream тест
if curl -f -s http://$DEVICE_IP:81 > /dev/null; then
    echo "✓ Stream port 81 is accessible"
else
    echo "✗ Stream port 81 is not accessible"
    exit 1
fi

echo "All tests passed for $DEVICE_NAME!"
```

---

## Сценарий 8: Облачное развертывание

### Требования
- VPS или облачный сервер вместо Raspberry Pi
- Публичный IP адрес
- Больше устройств и производительности

### Настройка на VPS

```bash
# 1. Подготовка сервера (Ubuntu 22.04)
ssh root@YOUR_VPS_IP

apt update && apt upgrade -y
apt install docker.io docker-compose git -y

# 2. Клонирование репозитория
git clone https://github.com/GOODWORKRINKZ/microbbox.git
cd microbbox/infrastructure/vr-proxy

# 3. Настройка firewall
ufw allow 22/tcp   # SSH
ufw allow 80/tcp   # HTTP
ufw allow 443/tcp  # HTTPS
ufw enable

# 4. Создание VPN туннеля к локальным устройствам
# Вариант А: WireGuard
apt install wireguard -y
# Настройка WireGuard сервера

# Вариант Б: OpenVPN
apt install openvpn -y
# Настройка OpenVPN сервера

# 5. Настройка nginx для проксирования через VPN
# ESP32 будут подключаться к VPN и получать IP в подсети VPN
# Например: 10.8.0.2, 10.8.0.3, и т.д.

sudo ./scripts/add-device.sh
# Имя: robot1
# IP: 10.8.0.2  # VPN IP
# Домен: robot1.example.com

# 6. Получение сертификатов
sudo ./scripts/obtain-certificate.sh robot1.example.com admin@example.com

# 7. Запуск
docker-compose up -d
```

### Преимущества облачного развертывания
- ✅ Статический IP адрес
- ✅ Высокая пропускная способность
- ✅ Лучшая производительность
- ✅ Легкое масштабирование
- ✅ Резервное копирование

### Недостатки
- ❌ Ежемесячная оплата
- ❌ Требуется VPN для доступа к локальным устройствам
- ❌ Возможна большая задержка

---

## Полезные команды для всех сценариев

```bash
# Быстрая проверка всех устройств
for host in robot{1..5}.example.com; do
  echo -n "$host: "
  curl -s -o /dev/null -w "%{http_code}" http://$host
  echo
done

# Массовое обновление сертификатов
for domain in $(docker exec microbbox-certbot certbot certificates | grep "Certificate Name:" | awk '{print $3}'); do
  docker exec microbbox-certbot certbot renew --cert-name $domain --force-renewal
done

# Бэкап всех конфигураций
tar -czf backup-$(date +%Y%m%d-%H%M%S).tar.gz \
  nginx/conf.d/*.conf \
  docker-compose.yml \
  certbot/conf/

# Быстрое добавление устройства в одну команду
echo -e "robot6\n192.168.1.106\nrobot6.example.com\n80\n81\ny" | sudo ./scripts/add-device.sh

# Мониторинг всех логов в реальном времени
tail -f nginx/logs/*-access.log

# Поиск ошибок во всех логах
grep -i error nginx/logs/*.log
```

---

Эти примеры покрывают большинство сценариев использования. Адаптируйте их под свои нужды!
