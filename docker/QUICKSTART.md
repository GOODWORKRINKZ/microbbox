# Быстрый старт

## За 5 минут (локальная сеть)

```bash
# 1. Перейдите в папку docker
cd docker/

# 2. Запустите скрипт настройки
./scripts/setup.sh

# 3. Отредактируйте .env файл
nano .env
# Установите:
# - ESP32_IP=192.168.1.100 (IP вашего ESP32)
# - DOMAIN_NAME=robot.local
# - CERT_MODE=selfsigned

# 4. Перезапустите после изменения .env
./scripts/setup.sh

# 5. Сгенерируйте самоподписанный сертификат
./scripts/generate-selfsigned-cert.sh

# 6. Добавьте в /etc/hosts на клиенте
echo "192.168.1.50 robot.local" | sudo tee -a /etc/hosts
# где 192.168.1.50 - IP Docker хоста

# 7. Откройте браузер
https://robot.local/
https://robot.local/stream
```

## За 15 минут (с Let's Encrypt)

Требования:
- Доменное имя с настроенной DNS
- Проброс портов 80 и 443 на Docker хост

```bash
# 1. Перейдите в папку docker
cd docker/

# 2. Запустите скрипт настройки
./scripts/setup.sh

# 3. Отредактируйте .env файл
nano .env
# Установите:
# - ESP32_IP=192.168.1.100
# - DOMAIN_NAME=robot.yourdomain.com
# - LETSENCRYPT_EMAIL=you@example.com
# - CERT_MODE=production

# 4. Перезапустите после изменения .env
./scripts/setup.sh

# 5. Настройте DNS
# Создайте A-запись: robot.yourdomain.com → ваш внешний IP

# 6. Настройте проброс портов на роутере
# 80 → IP Docker хоста
# 443 → IP Docker хоста

# 7. Получите сертификат
./scripts/obtain-certificate.sh

# 8. Откройте браузер
https://robot.yourdomain.com/
https://robot.yourdomain.com/stream
```

## Управление

```bash
# Статус
docker-compose ps

# Логи
docker-compose logs -f

# Перезапуск
docker-compose restart

# Остановка
docker-compose down
```

## Устранение неполадок

```bash
# ESP32 недоступен
ping 192.168.1.100
curl http://192.168.1.100:80/

# Проверка конфигурации nginx
docker exec microbbox-proxy-nginx nginx -t

# Просмотр логов ошибок
tail -f nginx/logs/*-error.log
```

## Подробная документация

См. [README.md](README.md) для полной документации.
