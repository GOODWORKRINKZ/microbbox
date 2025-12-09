# Архитектура Docker Proxy для ESP32

## Обзор

Данное решение позволяет проксировать два порта ESP32 (80 и 81) через один HTTPS порт (443) с валидными SSL сертификатами.

## Схема работы

```
┌─────────────────────────────────────────────────────────────┐
│                         Клиент                              │
│                    (Браузер, VR, API)                       │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            │ HTTPS (443)
                            │ https://domain.com/
                            │ https://domain.com/stream
                            │
┌───────────────────────────▼─────────────────────────────────┐
│                      Docker Host                            │
│              (Ноутбук / Raspberry Pi / Сервер)              │
│                                                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              nginx (Alpine Linux)                      │ │
│  │                                                        │ │
│  │  ┌──────────────────────────────────────────────┐     │ │
│  │  │ Port 443 (HTTPS)                             │     │ │
│  │  │ - SSL терминация                             │     │ │
│  │  │ - Let's Encrypt / Самоподписанный сертификат │     │ │
│  │  └──────────────────────────────────────────────┘     │ │
│  │                                                        │ │
│  │  ┌──────────────────────────────────────────────┐     │ │
│  │  │ Reverse Proxy                                │     │ │
│  │  │                                              │     │ │
│  │  │ Location /       → ESP32:80 (API)           │     │ │
│  │  │ Location /stream → ESP32:81 (Video Stream)  │     │ │
│  │  └──────────────────────────────────────────────┘     │ │
│  │                                                        │ │
│  │  Features:                                             │ │
│  │  - WebSocket поддержка                                │ │
│  │  - Буферизация отключена для streaming               │ │
│  │  - Обработка недоступности устройства                 │ │
│  │  - CORS заголовки для WebXR                           │ │
│  │  - Health check endpoints                             │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              certbot (Certbot/Python)                  │ │
│  │                                                        │ │
│  │  - Получение SSL сертификатов от Let's Encrypt       │ │
│  │  - Автоматическое обновление каждые 12 часов         │ │
│  │  - Проверка через webroot (/.well-known/)            │ │
│  │  - Поддержка staging и production режимов            │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                             │
│  Docker Network: microbbox-proxy-network                    │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            │ HTTP (80, 81)
                            │ Локальная сеть
                            │
┌───────────────────────────▼─────────────────────────────────┐
│                       ESP32-CAM                             │
│                    192.168.1.100                            │
│                                                             │
│  Port 80  → API, веб-интерфейс, управление                 │
│  Port 81  → MJPEG видеопоток с камеры                      │
└─────────────────────────────────────────────────────────────┘
```

## Маппинг портов

### Входящие порты (Docker Host)

- **80 (HTTP)**: Редирект на HTTPS + Let's Encrypt проверка
- **443 (HTTPS)**: Основной доступ с SSL

### Исходящие порты (к ESP32)

- **80**: API и веб-интерфейс ESP32
- **81**: Видеопоток с камеры ESP32

### URL маппинг

| Клиентский запрос | nginx обработка | Целевой upstream |
|-------------------|-----------------|------------------|
| `https://domain.com/` | Proxy pass | `http://ESP32_IP:80/` |
| `https://domain.com/api` | Proxy pass | `http://ESP32_IP:80/api` |
| `https://domain.com/stream` | Proxy pass | `http://ESP32_IP:81/` |

## Компоненты

### 1. nginx (Reverse Proxy)

**Образ**: `nginx:alpine`  
**Роль**: HTTP/HTTPS сервер и reverse proxy

**Функции**:
- SSL терминация (HTTPS → HTTP)
- Reverse proxy для ESP32
- Обработка WebSocket соединений
- Статические страницы ошибок
- Health check endpoints

**Volumes**:
- `./nginx/nginx.conf` → `/etc/nginx/nginx.conf` (основная конфигурация)
- `./nginx/conf.d/` → `/etc/nginx/conf.d/` (конфигурации устройств)
- `./certbot/conf/` → `/etc/letsencrypt/` (SSL сертификаты)
- `./nginx/logs/` → `/var/log/nginx/` (логи)

### 2. certbot (SSL Certificate Management)

**Образ**: `certbot/certbot`  
**Роль**: Управление SSL сертификатами от Let's Encrypt

**Функции**:
- Получение SSL сертификатов (ACME HTTP-01 challenge)
- Автоматическое обновление сертификатов
- Поддержка staging режима для тестирования

**Volumes**:
- `./certbot/conf/` → `/etc/letsencrypt/` (сертификаты и ключи)
- `./certbot/www/` → `/var/www/certbot/` (для ACME challenge)
- `./certbot/logs/` → `/var/log/letsencrypt/` (логи certbot)

## Конфигурационные файлы

### docker-compose.yml

Определяет Docker сервисы, сети и volumes.

**Ключевые настройки**:
- `restart: unless-stopped` - автоматический перезапуск
- `depends_on: certbot` - порядок запуска
- Health checks для мониторинга

### nginx/nginx.conf

Основная конфигурация nginx.

**Ключевые настройки**:
- `worker_processes auto` - автоматическое определение CPU ядер
- `proxy_buffering off` - отключение буферизации для streaming
- Увеличенные таймауты для видеопотоков (300 сек)
- CORS заголовки для WebXR

### nginx/conf.d/[domain].conf

Конфигурация конкретного устройства (генерируется из шаблона).

**Структура**:
- Upstream определения (api и stream)
- HTTP сервер (редирект на HTTPS)
- HTTPS сервер (основная логика)
- Location blocks для маппинга URL
- Error pages для недоступности устройства

### .env

Файл с переменными окружения.

**Основные переменные**:
- `ESP32_IP` - IP адрес ESP32
- `ESP32_API_PORT` - порт API (обычно 80)
- `ESP32_STREAM_PORT` - порт видеопотока (обычно 81)
- `DOMAIN_NAME` - доменное имя
- `CERT_MODE` - режим сертификата (production/staging/selfsigned)

## Процесс получения SSL сертификата

### Let's Encrypt (ACME HTTP-01)

```
1. certbot отправляет запрос к Let's Encrypt API
   ↓
2. Let's Encrypt генерирует challenge token
   ↓
3. certbot создает файл в /var/www/certbot/.well-known/acme-challenge/
   ↓
4. nginx проксирует запросы к /.well-known/acme-challenge/ на certbot volume
   ↓
5. Let's Encrypt проверяет доступность файла по HTTP
   http://domain.com/.well-known/acme-challenge/[token]
   ↓
6. При успешной проверке Let's Encrypt выдает сертификат
   ↓
7. certbot сохраняет сертификат в /etc/letsencrypt/live/[domain]/
   ↓
8. nginx загружает сертификат и начинает обслуживать HTTPS
```

### Автоматическое обновление

- certbot запускается в бесконечном цикле
- Каждые 12 часов проверяет срок действия сертификатов
- Обновляет сертификаты за 30 дней до истечения
- Let's Encrypt сертификаты действительны 90 дней

## Обработка запросов

### HTTPS запрос к API

```
Client → https://domain.com/api/settings
  ↓
nginx:443 (SSL терминация)
  ↓
nginx location / { proxy_pass http://esp32_api; }
  ↓
upstream esp32_api (ESP32_IP:80)
  ↓
ESP32:80/api/settings
  ↓
Response ←
  ↓
nginx (добавление HTTPS заголовков)
  ↓
Client ← HTTPS Response
```

### HTTPS запрос к видеопотоку

```
Client → https://domain.com/stream
  ↓
nginx:443 (SSL терминация)
  ↓
nginx location /stream { proxy_pass http://esp32_stream/; }
  ↓
upstream esp32_stream (ESP32_IP:81)
  ↓
ESP32:81/ (MJPEG stream)
  ↓
Response (multipart/x-mixed-replace) ←
  ↓
nginx (proxy_buffering off, chunked transfer)
  ↓
Client ← HTTPS Live Stream
```

## Режимы работы

### Production (Let's Encrypt)

- Валидные SSL сертификаты
- Требует публичный домен и DNS
- Лимит: 50 сертификатов/неделю на домен
- Подходит для WebXR

### Staging (Let's Encrypt тестовый)

- Невалидные тестовые сертификаты
- Нет лимитов на запросы
- Для отладки процесса получения сертификатов
- Не подходит для production

### Selfsigned (Самоподписанный)

- Локально генерируемые сертификаты
- Не требует интернет и DNS
- Браузеры показывают предупреждение
- Не подходит для WebXR

## Безопасность

### SSL/TLS

- TLS 1.2 и 1.3
- Современные cipher suites
- HSTS (Strict-Transport-Security)
- SSL session cache для производительности

### Заголовки безопасности

- `X-Frame-Options: SAMEORIGIN`
- `X-Content-Type-Options: nosniff`
- `X-XSS-Protection: 1; mode=block`
- `Strict-Transport-Security: max-age=31536000`

### CORS для WebXR

- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, OPTIONS`
- `Permissions-Policy` для WebXR APIs

## Производительность

### Оптимизации nginx

- `sendfile on` - прямая передача файлов
- `tcp_nopush on` - оптимизация TCP пакетов
- `tcp_nodelay on` - минимизация задержек
- `keepalive` connections для upstream
- `gzip` сжатие для текстовых ресурсов

### Для Raspberry Pi

- Ограниченные `worker_connections` (512-1024)
- Минимальное логирование
- Ограничение ресурсов через Docker deploy

## Мониторинг

### Health Checks

- Docker health check для nginx контейнера
- nginx health endpoint: `/nginx-health`
- Проверка каждые 30 секунд

### Логи

- Access logs: все входящие запросы
- Error logs: ошибки и предупреждения
- Certbot logs: процесс получения сертификатов
- Ротация логов через Docker

## Масштабирование

### Несколько ESP32 устройств

**Вариант 1**: Разные домены на одном nginx
```
robot1.example.com → ESP32_1 (192.168.1.100)
robot2.example.com → ESP32_2 (192.168.1.101)
robot3.example.com → ESP32_3 (192.168.1.102)
```

**Вариант 2**: Отдельные Docker стеки
```
~/proxy-robot1/ (порт 8001:80, 8443:443)
~/proxy-robot2/ (порт 8002:80, 8444:443)
```

**Вариант 3**: Централизованное решение
- См. `infrastructure/vr-proxy/` в репозитории
- Поддержка множества устройств
- Единая точка управления

## Отказоустойчивость

### Обработка недоступности ESP32

- `max_fails=3` - максимум 3 неудачные попытки
- `fail_timeout=30s` - считать недоступным на 30 сек
- Кастомные error pages с автообновлением
- Автоматическое восстановление при появлении устройства

### Автоматический перезапуск

- `restart: unless-stopped` в docker-compose
- Docker автоматически перезапускает упавшие контейнеры
- nginx graceful reload при обновлении конфигурации

## Интеграция с ESP32

### Требования к ESP32

- HTTP сервер на порту 80 (API, веб-интерфейс)
- HTTP сервер на порту 81 (MJPEG video stream)
- Стабильное WiFi подключение
- Статический или зарезервированный IP

### Адаптация веб-интерфейса

ESP32 код должен знать о proxy:
```javascript
// Вместо
var streamUrl = "http://" + window.location.hostname + ":81/";

// Использовать
var streamUrl = "https://" + window.location.hostname + "/stream";
```

## Альтернативные решения

### Traefik

Более автоматизированное решение с автодискавери:
- Автоматическое получение Let's Encrypt
- Service discovery через Docker labels
- Dashboard для мониторинга

### Caddy

Простейший веб-сервер с автоматическим HTTPS:
- Автоматический Let's Encrypt
- Простой Caddyfile вместо nginx.conf
- Меньше настроек, но меньше гибкости

### Apache + mod_proxy

Классическое решение:
- mod_ssl для HTTPS
- mod_proxy для reverse proxy
- Больше ресурсов, сложнее конфигурация
