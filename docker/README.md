# Docker композиция для ESP32 HTTPS Proxy

## 📋 Описание

Эта Docker композиция позволяет проксировать интерфейс ESP32-CAM на HTTPS страницу с валидными SSL сертификатами. Композиция может быть запущена на любой системе: от ноутбука до Raspberry Pi.

### Основные возможности

✅ **Проксирование двух портов ESP32 на один HTTPS порт (443)**
- Порт 80 (API) → `https://domain.com/`
- Порт 81 (видеопоток) → `https://domain.com/stream`

✅ **Автоматическая генерация валидных SSL сертификатов**
- Let's Encrypt с автоматическим обновлением
- Самоподписанные сертификаты для локальной сети
- Staging режим для тестирования

✅ **Простая настройка IP адреса ESP32**
- Все настройки в одном файле `.env`
- Автоматическая генерация конфигурации nginx

✅ **Кроссплатформенность**
- Работает на Linux (Ubuntu, Debian, Arch и т.д.)
- Работает на macOS
- Работает на Raspberry Pi (включая Zero)
- Работает на Windows с WSL2

✅ **Обработка недоступности устройства**
- Красивые страницы ошибок
- Автоматическое переподключение
- Health check endpoints

## 🏗️ Архитектура

```
┌─────────────────┐
│   Клиент        │
│  (Браузер/VR)   │
└────────┬────────┘
         │ HTTPS (443)
         ▼
┌─────────────────────────────────┐
│   Docker Host                   │
│  (Ноутбук/Raspberry Pi/Сервер)  │
│                                 │
│  ┌───────────────────────────┐  │
│  │ nginx (Docker)            │  │
│  │ - SSL терминация          │  │
│  │ - Reverse Proxy           │  │
│  │ - Let's Encrypt           │  │
│  └──────────┬────────────────┘  │
│             │                   │
│  ┌──────────▼────────────────┐  │
│  │ certbot (Docker)          │  │
│  │ - Получение сертификатов  │  │
│  │ - Автообновление          │  │
│  └───────────────────────────┘  │
└─────────────┬───────────────────┘
              │ HTTP (80, 81)
              ▼
     ┌────────────────┐
     │  ESP32-CAM     │
     │  192.168.1.100 │
     │  :80 - API     │
     │  :81 - Stream  │
     └────────────────┘
```

## 📦 Требования

### Программное обеспечение
- Docker (версия 20.10 или выше)
- Docker Compose (версия 1.29 или выше, или встроенный `docker compose`)
- Bash (для скриптов настройки)
- OpenSSL (для генерации самоподписанных сертификатов)

### Для Let's Encrypt сертификатов
- Доменное имя с настроенной DNS A-записью
- Порты 80 и 443 доступны из интернета
- Проброс портов на роутере (порт 80 и 443 → IP Docker хоста)

### Для локальной сети
- Локальный DNS сервер (например, на роутере Keenetic)
- или добавление записи в `/etc/hosts` на клиентских устройствах

## 🚀 Быстрый старт

### 1. Клонирование репозитория

```bash
git clone https://github.com/GOODWORKRINKZ/microbbox.git
cd microbbox/docker
```

### 2. Запуск скрипта настройки

```bash
./scripts/setup.sh
```

Скрипт выполнит:
- Проверку наличия Docker и Docker Compose
- Создание файла `.env` из шаблона
- Генерацию конфигурации nginx
- Запуск Docker контейнеров

### 3. Редактирование конфигурации

Откройте файл `.env` и настройте параметры:

```bash
nano .env
```

Обязательные параметры:
```env
# IP адрес вашего ESP32 в локальной сети
ESP32_IP=192.168.1.100

# Доменное имя
DOMAIN_NAME=robot.example.com

# Email для Let's Encrypt уведомлений
LETSENCRYPT_EMAIL=admin@example.com

# Режим сертификата (production, staging, selfsigned)
CERT_MODE=production
```

### 4. Получение SSL сертификата

#### Вариант A: Let's Encrypt (рекомендуется)

Требования:
- Домен должен резолвиться на ваш внешний IP
- Порты 80 и 443 проброшены на этот хост

```bash
./scripts/obtain-certificate.sh
```

#### Вариант B: Самоподписанный сертификат (для локальной сети)

```bash
./scripts/generate-selfsigned-cert.sh
```

⚠️ **Важно**: Самоподписанные сертификаты не подходят для WebXR!

### 5. Проверка работы

Откройте в браузере:
- API: `https://robot.example.com/`
- Видеопоток: `https://robot.example.com/stream`

## ⚙️ Подробная настройка

### Файл .env

Все настройки находятся в файле `.env`:

```env
# ==========================================
# Настройки ESP32 устройства
# ==========================================

# IP адрес ESP32 в локальной сети
ESP32_IP=192.168.1.100

# Порт API ESP32 (обычно 80)
ESP32_API_PORT=80

# Порт видеопотока ESP32 (обычно 81)
ESP32_STREAM_PORT=81

# ==========================================
# Настройки домена и SSL
# ==========================================

# Доменное имя для доступа к устройству
DOMAIN_NAME=robot.example.com

# Email для уведомлений Let's Encrypt
LETSENCRYPT_EMAIL=admin@example.com

# Режим получения сертификатов
# production - валидные сертификаты (лимит 50/неделю)
# staging - тестовые сертификаты (без лимита)
# selfsigned - самоподписанный (локальная сеть)
CERT_MODE=production

# ==========================================
# Дополнительные настройки
# ==========================================

# Часовой пояс
TZ=Europe/Moscow

# Уровень логирования nginx
NGINX_LOG_LEVEL=warn

# Имя устройства (для логов)
DEVICE_NAME=robot1
```

### Настройка DNS

#### Вариант 1: Публичный DNS (для Let's Encrypt)

1. Зайдите в панель управления вашим доменом
2. Создайте A-запись:
   - Имя: `robot` (или `@` для корневого домена)
   - Тип: A
   - Значение: Ваш внешний IP адрес
   - TTL: 3600

3. Проверка:
```bash
nslookup robot.example.com
# Должен вернуть ваш внешний IP
```

#### Вариант 2: Локальный DNS (роутер Keenetic)

1. Зайдите в веб-интерфейс роутера
2. Перейдите в "Интернет-фильтры" → "Серверы имен" → "Локальные доменные имена"
3. Добавьте запись:
   - Имя: `robot.example.com`
   - IP: `192.168.1.50` (IP Docker хоста)

4. Или через telnet/SSH:
```bash
telnet 192.168.1.1
ip name robot.example.com 192.168.1.50
system configuration save
```

#### Вариант 3: Файл hosts (для тестирования)

Linux/macOS:
```bash
sudo nano /etc/hosts
# Добавьте строку:
192.168.1.50 robot.example.com
```

Windows:
```
C:\Windows\System32\drivers\etc\hosts
# Добавьте строку:
192.168.1.50 robot.example.com
```

### Настройка проброса портов

На роутере настройте port forwarding:

| Внешний порт | Внутренний IP | Внутренний порт | Протокол |
|--------------|---------------|-----------------|----------|
| 80           | 192.168.1.50  | 80              | TCP      |
| 443          | 192.168.1.50  | 443             | TCP      |

где `192.168.1.50` - IP адрес Docker хоста (вашего ноутбука/Raspberry Pi)

### Резервирование IP для ESP32

Чтобы IP адрес ESP32 не менялся:

1. Зайдите в настройки роутера
2. Найдите DHCP настройки
3. Добавьте резервирование по MAC адресу:
   - MAC: `xx:xx:xx:xx:xx:xx` (MAC вашего ESP32)
   - IP: `192.168.1.100`

## 🔧 Управление

### Просмотр статуса

```bash
docker-compose ps
```

### Просмотр логов

```bash
# Все логи
docker-compose logs -f

# Только nginx
docker-compose logs -f nginx

# Только certbot
docker-compose logs -f certbot

# Логи nginx на файловой системе
tail -f nginx/logs/robot1-access.log
tail -f nginx/logs/robot1-error.log
```

### Перезапуск

```bash
# Перезапуск всех контейнеров
docker-compose restart

# Перезапуск только nginx
docker-compose restart nginx

# Перезагрузка конфигурации nginx без перезапуска
docker exec microbbox-proxy-nginx nginx -s reload
```

### Остановка

```bash
docker-compose down
```

### Полная очистка

```bash
# Остановка и удаление контейнеров, сетей
docker-compose down

# Удаление volumes (ВНИМАНИЕ: удалит сертификаты!)
docker-compose down -v

# Удаление всех файлов
rm -rf certbot nginx/conf.d nginx/logs
```

## 🔐 Управление сертификатами

### Проверка сертификатов

```bash
docker exec microbbox-proxy-certbot certbot certificates
```

### Ручное обновление сертификатов

```bash
docker exec microbbox-proxy-certbot certbot renew
docker exec microbbox-proxy-nginx nginx -s reload
```

### Удаление сертификата

```bash
docker exec microbbox-proxy-certbot certbot delete --cert-name robot.example.com
```

### Тестирование с staging сертификатами

Staging режим полезен для тестирования, так как не имеет лимитов на количество запросов:

```env
# В .env файле
CERT_MODE=staging
```

Затем:
```bash
./scripts/obtain-certificate.sh
```

⚠️ Staging сертификаты не являются валидными и браузер покажет предупреждение!

## 🌐 Варианты использования

### Сценарий 1: Локальная сеть с самоподписанным сертификатом

**Подходит для**: Разработка, тестирование, локальное использование  
**Не подходит для**: WebXR, публичный доступ

```bash
# В .env
CERT_MODE=selfsigned
ESP32_IP=192.168.1.100
DOMAIN_NAME=robot.local

# Настройка
./scripts/setup.sh
./scripts/generate-selfsigned-cert.sh
```

Добавьте в `/etc/hosts` на клиентских устройствах:
```
192.168.1.50 robot.local
```

### Сценарий 2: Публичный доступ с Let's Encrypt

**Подходит для**: WebXR, удаленный доступ, production  
**Требует**: Доменное имя, проброс портов

```bash
# В .env
CERT_MODE=production
ESP32_IP=192.168.1.100
DOMAIN_NAME=robot.yourdomain.com
LETSENCRYPT_EMAIL=you@example.com

# Настройка DNS
# A-запись: robot.yourdomain.com → ваш внешний IP

# Проброс портов на роутере
# 80 → 192.168.1.50:80
# 443 → 192.168.1.50:443

# Настройка
./scripts/setup.sh
./scripts/obtain-certificate.sh
```

### Сценарий 3: Локальная сеть с локальным DNS

**Подходит для**: Использование в домашней сети без внешнего доступа  
**Требует**: Роутер с DNS сервером (Keenetic, pfSense и т.д.)

```bash
# В .env
CERT_MODE=selfsigned
ESP32_IP=192.168.1.100
DOMAIN_NAME=robot.home

# Настройка локального DNS на роутере
# robot.home → 192.168.1.50

# Настройка
./scripts/setup.sh
./scripts/generate-selfsigned-cert.sh
```

## 📊 Мониторинг и диагностика

### Health Check

nginx имеет endpoint для проверки здоровья:

```bash
curl http://localhost/nginx-health
# Ответ: healthy
```

### Проверка доступности ESP32

```bash
# Ping
ping 192.168.1.100

# Проверка портов
curl -I http://192.168.1.100:80/
curl -I http://192.168.1.100:81/

# Через proxy
curl -I https://robot.example.com/
curl -I https://robot.example.com/stream
```

### Тестирование SSL

```bash
# Проверка сертификата
openssl s_client -connect robot.example.com:443 -servername robot.example.com

# Проверка через браузерный инструмент
curl -vI https://robot.example.com/
```

## 🐛 Устранение неполадок

### Проблема: nginx не запускается

**Симптомы**: Контейнер nginx постоянно перезапускается

**Решение**:
```bash
# Проверка логов
docker-compose logs nginx

# Проверка конфигурации
docker-compose run --rm nginx nginx -t

# Проверка наличия SSL файлов
ls -la certbot/conf/
```

### Проблема: Не удается получить сертификат

**Симптомы**: Ошибка при запуске `obtain-certificate.sh`

**Решение**:
```bash
# 1. Проверка DNS
nslookup robot.example.com

# 2. Проверка доступности порта 80 из интернета
curl -I http://robot.example.com/.well-known/acme-challenge/test

# 3. Проверка логов certbot
docker-compose logs certbot

# 4. Попробуйте staging режим
# В .env установите CERT_MODE=staging
./scripts/obtain-certificate.sh
```

### Проблема: ESP32 недоступен

**Симптомы**: Страница "Устройство недоступно"

**Решение**:
```bash
# 1. Проверка сети
ping 192.168.1.100

# 2. Проверка портов
curl http://192.168.1.100:80/
curl http://192.168.1.100:81/

# 3. Проверка конфигурации
cat nginx/conf.d/*.conf | grep upstream

# 4. Проверка IP в .env
cat .env | grep ESP32_IP
```

### Проблема: Браузер показывает предупреждение о сертификате

**Для самоподписанных сертификатов** - это нормально:
- Chrome/Edge: Нажмите "Дополнительно" → "Продолжить"
- Firefox: Нажмите "Дополнительно" → "Принять риск"

**Для Let's Encrypt сертификатов** - проверьте:
```bash
# Информация о сертификате
docker exec microbbox-proxy-certbot certbot certificates

# Проверка что используется правильный сертификат
openssl s_client -connect robot.example.com:443 -servername robot.example.com | grep "Verify return code"
```

### Проблема: Видеопоток не работает

**Решение**:
```bash
# 1. Проверка прямого доступа к stream
curl -I http://192.168.1.100:81/

# 2. Проверка nginx конфигурации
cat nginx/conf.d/*.conf | grep -A 10 "location /stream"

# 3. Проверка логов
tail -f nginx/logs/robot1-error.log
```

## 📈 Оптимизация для Raspberry Pi

Для Raspberry Pi Zero и других маломощных устройств:

### Ограничение ресурсов Docker

Добавьте в `docker-compose.yml`:

```yaml
services:
  nginx:
    # ... existing config ...
    deploy:
      resources:
        limits:
          cpus: '0.5'
          memory: 256M
        reservations:
          cpus: '0.25'
          memory: 128M
```

### Отключение access логов

В `nginx/nginx.conf`:

```nginx
# Закомментируйте
# access_log /var/log/nginx/access.log main;
access_log off;
```

### Уменьшение worker_connections

В `nginx/nginx.conf`:

```nginx
events {
    worker_connections 512;  # было 1024
}
```

## 🔒 Безопасность

### Рекомендации

1. **Используйте сильные пароли** для ESP32 WiFi
2. **Обновляйте систему** регулярно:
   ```bash
   sudo apt update && sudo apt upgrade -y
   docker-compose pull
   docker-compose up -d
   ```

3. **Настройте firewall**:
   ```bash
   sudo ufw allow 22/tcp   # SSH
   sudo ufw allow 80/tcp   # HTTP
   sudo ufw allow 443/tcp  # HTTPS
   sudo ufw enable
   ```

4. **Ограничьте SSH доступ** только с локальной сети

5. **Мониторьте логи**:
   ```bash
   # Подозрительные запросы
   tail -f nginx/logs/access.log | grep -v "192.168"
   ```

6. **Rate limiting** - уже настроен в nginx.conf

## 📝 Примеры конфигураций

### Несколько ESP32 устройств

Для каждого устройства создайте отдельный `.env` файл и запустите в отдельной директории:

```bash
# Структура
~/microbbox-proxy/
├── robot1/  # Копия docker папки
│   ├── .env (ESP32_IP=192.168.1.100, DOMAIN_NAME=robot1.example.com)
│   └── ...
├── robot2/  # Копия docker папки
│   ├── .env (ESP32_IP=192.168.1.101, DOMAIN_NAME=robot2.example.com)
│   └── ...
```

Или используйте infrastructure/vr-proxy для централизованного управления.

## 🆘 Поддержка

При возникновении проблем:

1. Проверьте [раздел устранения неполадок](#-устранение-неполадок)
2. Просмотрите логи: `docker-compose logs`
3. Создайте Issue в репозитории с описанием проблемы и логами

## 📄 Лицензия

MIT License - см. файл LICENSE в корне репозитория

## 🙏 Благодарности

- nginx - высокопроизводительный веб-сервер и reverse proxy
- Let's Encrypt - бесплатные SSL сертификаты
- Certbot - автоматизация работы с Let's Encrypt
- Docker - контейнеризация приложений
