# Docker Proxy для ESP32 - Краткая справка

## Что это?

Docker композиция для проксирования ESP32 интерфейса через HTTPS с валидными SSL сертификатами.

**Основные возможности:**
- Проксирование портов 80 и 81 ESP32 на один HTTPS порт (443)
- Автоматическая генерация SSL сертификатов (Let's Encrypt или самоподписанные)
- Простая настройка через .env файл
- Работает на любой системе: ноутбук, Raspberry Pi, сервер
- WebXR совместимость

## Структура

```
docker/
├── README.md                      # Полная документация
├── QUICKSTART.md                  # Быстрый старт за 5 минут
├── ARCHITECTURE.md                # Подробное описание архитектуры
├── EXAMPLES.md                    # Примеры использования
├── FAQ.md                         # Часто задаваемые вопросы
├── docker-compose.yml             # Docker Compose конфигурация
├── .env.example                   # Шаблон конфигурации
├── .gitignore                     # Git ignore правила
├── nginx/
│   ├── nginx.conf                 # Основная конфигурация nginx
│   ├── device-template.conf       # Шаблон для устройства
│   ├── conf.d/                    # Сгенерированные конфигурации
│   └── logs/                      # Логи nginx
├── certbot/
│   ├── conf/                      # SSL сертификаты
│   ├── www/                       # ACME challenge файлы
│   └── logs/                      # Логи certbot
└── scripts/
    ├── setup.sh                   # Автоматическая настройка
    ├── obtain-certificate.sh      # Получение Let's Encrypt сертификата
    ├── generate-selfsigned-cert.sh # Генерация самоподписанного сертификата
    ├── status.sh                  # Проверка статуса системы
    └── cleanup.sh                 # Остановка и очистка
```

## Быстрый старт

### Минимальная настройка (5 минут)

```bash
# 1. Перейти в папку docker
cd docker/

# 2. Запустить скрипт настройки
./scripts/setup.sh

# 3. Отредактировать .env
nano .env
# Указать ESP32_IP и DOMAIN_NAME

# 4. Сгенерировать самоподписанный сертификат
./scripts/generate-selfsigned-cert.sh

# 5. Открыть в браузере
https://robot.local/
```

### С Let's Encrypt (15 минут)

Требует: доменное имя, проброс портов 80/443

```bash
# 1-3. Как выше, но в .env указать:
# CERT_MODE=production
# DOMAIN_NAME=robot.yourdomain.com

# 4. Получить сертификат
./scripts/obtain-certificate.sh

# 5. Открыть в браузере
https://robot.yourdomain.com/
```

## Основные команды

```bash
# Статус системы
./scripts/status.sh

# Просмотр логов
docker-compose logs -f

# Перезапуск
docker-compose restart

# Остановка
docker-compose down

# Полная очистка
./scripts/cleanup.sh --full
```

## URL маппинг

| Клиентский запрос | Проксируется на |
|-------------------|-----------------|
| `https://domain.com/` | `http://ESP32_IP:80/` (API) |
| `https://domain.com/stream` | `http://ESP32_IP:81/` (видео) |

## Режимы сертификатов

| Режим | Описание | Использование |
|-------|----------|---------------|
| `production` | Let's Encrypt валидные сертификаты | Production, WebXR |
| `staging` | Let's Encrypt тестовые | Тестирование |
| `selfsigned` | Самоподписанные | Локальная сеть |

## Требования

- Docker 20.10+
- Docker Compose 1.29+
- 512MB+ RAM
- 1GB+ свободного места

Для Let's Encrypt:
- Доменное имя
- Проброс портов 80/443
- DNS настроен на внешний IP

## Документация

- [README.md](README.md) - Полная документация
- [QUICKSTART.md](QUICKSTART.md) - Быстрый старт
- [ARCHITECTURE.md](ARCHITECTURE.md) - Архитектура решения
- [EXAMPLES.md](EXAMPLES.md) - Примеры для разных сценариев
- [FAQ.md](FAQ.md) - Часто задаваемые вопросы

## Поддержка

Создайте Issue на GitHub с описанием проблемы и логами.

## Лицензия

MIT License
