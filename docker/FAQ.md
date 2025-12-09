# Часто задаваемые вопросы (FAQ)

## Общие вопросы

### Зачем нужен этот Docker Proxy?

ESP32 работает только по HTTP без SSL. Для WebXR и безопасного доступа из интернета нужен HTTPS с валидными сертификатами. Docker Proxy:
- Добавляет SSL/TLS шифрование
- Проксирует два порта (80 и 81) на один HTTPS порт (443)
- Автоматически обновляет сертификаты
- Обрабатывает недоступность устройства

### Можно ли обойтись без Docker?

Да, можно настроить nginx напрямую на хосте. Но Docker дает:
- Простую установку на любой системе
- Изоляцию от основной системы
- Легкое обновление и откат
- Портабельность между хостами

### Какие системные требования?

Минимальные:
- 512MB RAM (Raspberry Pi Zero)
- 1GB свободного места
- Linux, macOS, Windows с WSL2

Рекомендуемые:
- 1GB+ RAM (Raspberry Pi 3/4)
- 2GB свободного места
- Debian/Ubuntu Linux

## Настройка и установка

### Как узнать IP адрес ESP32?

Способы:
1. **Serial Monitor** - подключиться по USB и посмотреть вывод при загрузке
2. **Роутер** - зайти в веб-интерфейс роутера, раздел "Подключенные устройства"
3. **Сканирование сети** - использовать `nmap` или `arp-scan`:
   ```bash
   sudo nmap -sn 192.168.1.0/24
   sudo arp-scan --interface=eth0 --localnet
   ```
4. **mDNS** - если поддерживается: `http://XXXXXX.microbbox.local` где XXXXXX - последние 6 символов MAC

### Как зарезервировать IP для ESP32?

На большинстве роутеров:
1. Зайти в веб-интерфейс роутера
2. Найти раздел DHCP или "Подключенные устройства"
3. Найти ESP32 (по MAC адресу или имени)
4. Включить "Статический IP" или "DHCP резервирование"
5. Указать желаемый IP (например, 192.168.1.100)

### Порты 80 и 443 уже заняты, что делать?

**Вариант 1**: Использовать другие порты

Отредактируйте `docker-compose.yml`:
```yaml
ports:
  - "8080:80"   # вместо 80:80
  - "8443:443"  # вместо 443:443
```

Доступ: `https://domain.com:8443/`

**Вариант 2**: Остановить службу, использующую порты

```bash
# Узнать что использует порт
sudo lsof -i :80
sudo lsof -i :443

# Остановить Apache (если установлен)
sudo systemctl stop apache2

# Остановить nginx (если установлен)
sudo systemctl stop nginx
```

### Не получается получить Let's Encrypt сертификат

**Проверки**:

1. **DNS резолвится правильно?**
   ```bash
   nslookup robot.example.com
   # Должен вернуть внешний IP вашей сети
   ```

2. **Порт 80 доступен из интернета?**
   ```bash
   # С внешнего сервера или через онлайн сервис
   curl http://robot.example.com/.well-known/acme-challenge/test
   ```

3. **Проброс портов настроен?**
   - На роутере должен быть port forwarding: 80 → IP Docker хоста

4. **Превышен лимит Let's Encrypt?**
   - 50 сертификатов/неделю на домен
   - Используйте staging режим для тестирования:
     ```bash
     # В .env
     CERT_MODE=staging
     ```

5. **Посмотреть логи certbot**:
   ```bash
   docker-compose logs certbot
   ```

## SSL/HTTPS

### Разница между production, staging и selfsigned?

| Режим | Описание | Валидность | Использование |
|-------|----------|------------|---------------|
| **production** | Let's Encrypt настоящие сертификаты | ✅ Валидные | Production, WebXR |
| **staging** | Let's Encrypt тестовые сертификаты | ❌ Невалидные | Тестирование процесса |
| **selfsigned** | Самоподписанные локальные | ❌ Невалидные | Локальная сеть, разработка |

### Браузер показывает "Небезопасное соединение"

**Для selfsigned сертификатов** - это нормально:

Chrome/Edge:
1. Нажмите "Дополнительно"
2. Нажмите "Продолжить на robot.local (небезопасно)"

Firefox:
1. Нажмите "Дополнительно"
2. Нажмите "Принять риск и продолжить"

**Для Let's Encrypt сертификатов** - проверьте:
```bash
# Статус сертификата
docker exec microbbox-proxy-certbot certbot certificates

# Срок действия
openssl s_client -connect robot.example.com:443 -servername robot.example.com
```

### Как добавить самоподписанный сертификат в доверенные?

**Linux**:
```bash
# Экспорт сертификата
sudo cp certbot/conf/live/robot.local/fullchain.pem /usr/local/share/ca-certificates/robot.crt

# Обновление хранилища
sudo update-ca-certificates
```

**macOS**:
```bash
# Открыть в Keychain Access
open certbot/conf/live/robot.local/fullchain.pem

# Или командой
sudo security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain certbot/conf/live/robot.local/fullchain.pem
```

**Windows**:
1. Открыть `certmgr.msc`
2. Доверенные корневые центры сертификации → Сертификаты
3. Действие → Все задачи → Импорт
4. Выбрать `fullchain.pem`

### Как часто обновляются сертификаты?

- **Let's Encrypt**: Проверка каждые 12 часов, обновление за 30 дней до истечения
- **Срок действия**: 90 дней
- **Автоматически**: Да, certbot контейнер делает это автоматически

### Сертификат истек, что делать?

```bash
# Принудительное обновление
docker exec microbbox-proxy-certbot certbot renew --force-renewal

# Перезагрузка nginx
docker exec microbbox-proxy-nginx nginx -s reload

# Проверка
docker exec microbbox-proxy-certbot certbot certificates
```

## Работа с ESP32

### ESP32 недоступен через proxy

**Проверки**:

1. **ESP32 в сети?**
   ```bash
   ping 192.168.1.100
   ```

2. **Порты отвечают?**
   ```bash
   curl http://192.168.1.100:80/
   curl http://192.168.1.100:81/
   ```

3. **Правильный IP в конфигурации?**
   ```bash
   cat .env | grep ESP32_IP
   ```

4. **nginx запущен?**
   ```bash
   docker-compose ps
   ```

5. **Конфигурация nginx корректна?**
   ```bash
   docker exec microbbox-proxy-nginx nginx -t
   cat nginx/conf.d/*.conf | grep upstream
   ```

### Видеопоток не работает

**Проверки**:

1. **Прямой доступ к stream работает?**
   ```bash
   curl -I http://192.168.1.100:81/
   ```

2. **Через proxy stream доступен?**
   ```bash
   curl -I https://robot.example.com/stream
   ```

3. **Логи nginx показывают ошибки?**
   ```bash
   tail -f nginx/logs/*-error.log
   ```

4. **ESP32 настроен на порт 81?**
   - Проверьте прошивку ESP32
   - В коде должен быть stream сервер на порту 81

### Нужно ли менять код ESP32?

**Минимальные изменения**:

В веб-интерфейсе ESP32 замените:
```javascript
// Было
var streamUrl = "http://" + window.location.hostname + ":81/";

// Стало
var streamUrl = window.location.protocol + "//" + window.location.hostname + "/stream";
```

Это позволит автоматически использовать правильный URL через proxy.

### Можно ли использовать несколько ESP32?

Да, три варианта:

**Вариант 1**: Разные домены на одном nginx  
См. `infrastructure/vr-proxy/` в репозитории

**Вариант 2**: Отдельные Docker стеки  
См. [EXAMPLES.md](EXAMPLES.md) - Пример 4

**Вариант 3**: Разные порты  
```yaml
# Robot 1
ports:
  - "80:80"
  - "443:443"

# Robot 2
ports:
  - "8080:80"
  - "8443:443"
```

## Docker и система

### Как обновить композицию?

```bash
# Остановить контейнеры
docker-compose down

# Обновить образы
docker-compose pull

# Запустить снова
docker-compose up -d
```

### Как посмотреть логи?

```bash
# Все логи в реальном времени
docker-compose logs -f

# Только nginx
docker-compose logs -f nginx

# Последние 100 строк
docker-compose logs --tail=100

# Логи на файловой системе
tail -f nginx/logs/robot1-access.log
tail -f nginx/logs/robot1-error.log
```

### Контейнер постоянно перезапускается

**Причины**:

1. **Ошибка в конфигурации nginx**
   ```bash
   docker logs microbbox-proxy-nginx
   docker-compose run --rm nginx nginx -t
   ```

2. **Нет SSL файлов**
   ```bash
   ls -la certbot/conf/
   ./scripts/generate-selfsigned-cert.sh
   ```

3. **Порты заняты**
   ```bash
   sudo lsof -i :80
   sudo lsof -i :443
   ```

### Как полностью удалить всё?

```bash
# Остановка и удаление контейнеров
docker-compose down

# Удаление volumes
docker-compose down -v

# Удаление образов
docker rmi nginx:alpine certbot/certbot

# Удаление файлов
cd ..
rm -rf docker/
```

### Использование ресурсов слишком высокое

**Оптимизации**:

1. **Ограничить ресурсы в docker-compose.yml**:
   ```yaml
   deploy:
     resources:
       limits:
         cpus: '0.5'
         memory: 256M
   ```

2. **Уменьшить worker_connections в nginx.conf**:
   ```nginx
   worker_connections 512;  # вместо 1024
   ```

3. **Отключить access логи**:
   ```nginx
   access_log off;
   ```

4. **Использовать Docker на SSD** вместо SD карты

## WebXR и VR

### WebXR не работает на Oculus Quest

**Проверки**:

1. **Используется HTTPS?**
   - WebXR требует HTTPS
   - Самоподписанные сертификаты НЕ работают

2. **Сертификат валидный?**
   - Используйте Let's Encrypt (production режим)
   - Проверьте на обычном ПК сначала

3. **Поддерживается WebXR?**
   ```javascript
   if ('xr' in navigator) {
     console.log('WebXR supported');
   }
   ```

4. **Разрешения даны?**
   - Браузер запросит разрешение на WebXR
   - Убедитесь что разрешили доступ

### Лаги в видеопотоке в VR

**Оптимизации**:

1. **Уменьшить качество камеры на ESP32**
   ```cpp
   sensor_t *s = esp_camera_sensor_get();
   s->set_framesize(s, FRAMESIZE_QVGA);  // вместо VGA
   s->set_quality(s, 12);  // 10-63, меньше = лучше качество
   ```

2. **Улучшить WiFi соединение**
   - Использовать 5GHz WiFi если поддерживается
   - Разместить роутер ближе к ESP32
   - Уменьшить помехи

3. **Использовать более мощный Docker хост**
   - Raspberry Pi 4 вместо Zero
   - SSD вместо SD карты

## Сеть и DNS

### Как настроить локальный DNS без роутера?

**Вариант 1**: Hosts файл на каждом устройстве

Linux/macOS:
```bash
sudo nano /etc/hosts
# Добавить:
192.168.1.50 robot.local
```

Windows:
```
C:\Windows\System32\drivers\etc\hosts
# Добавить:
192.168.1.50 robot.local
```

**Вариант 2**: Установить dnsmasq на Docker хост

```bash
sudo apt install dnsmasq
sudo nano /etc/dnsmasq.conf
# Добавить:
address=/robot.local/192.168.1.50
```

### Доступ из внешней сети не работает

**Проверки**:

1. **Внешний IP известен?**
   ```bash
   curl ifconfig.me
   # Или зайти на https://www.whatismyip.com/
   ```

2. **DNS резолвится на внешний IP?**
   ```bash
   nslookup robot.example.com
   # Должен вернуть ваш внешний IP
   ```

3. **Проброс портов настроен?**
   - На роутере: 80, 443 → IP Docker хоста

4. **Firewall не блокирует?**
   ```bash
   sudo ufw status
   sudo ufw allow 80/tcp
   sudo ufw allow 443/tcp
   ```

5. **ISP не блокирует порты?**
   - Некоторые провайдеры блокируют 80/443
   - Попробуйте использовать другие порты (8080, 8443)

### У меня динамический IP, что делать?

Используйте DynDNS сервис:

**Популярные сервисы**:
- [DuckDNS](https://www.duckdns.org/) - бесплатно
- [No-IP](https://www.noip.com/) - бесплатно с ограничениями
- [Dynu](https://www.dynu.com/) - бесплатно
- [FreeDNS](https://freedns.afraid.org/) - бесплатно

**Клиенты для обновления IP**:
- ddclient (Linux)
- Роутеры часто имеют встроенную поддержку DynDNS

См. [EXAMPLES.md](EXAMPLES.md) - Пример 2

## Безопасность

### Насколько безопасно это решение?

**Хорошо**:
- ✅ SSL/TLS шифрование
- ✅ Современные cipher suites
- ✅ HSTS заголовки
- ✅ Автообновление сертификатов
- ✅ Изоляция через Docker

**Рекомендации**:
- Используйте сильный пароль на ESP32
- Обновляйте систему регулярно
- Настройте firewall
- Ограничьте доступ по IP если возможно
- Мониторьте логи

### Нужен ли firewall?

Рекомендуется, особенно для публичного доступа:

```bash
# Установка ufw (Ubuntu)
sudo apt install ufw

# Разрешить SSH
sudo ufw allow 22/tcp

# Разрешить HTTP/HTTPS
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp

# Включить
sudo ufw enable

# Статус
sudo ufw status
```

### Как ограничить доступ по IP?

В nginx конфигурации (`nginx/conf.d/*.conf`):

```nginx
# Разрешить только локальную сеть
location / {
    allow 192.168.1.0/24;
    deny all;
    
    proxy_pass http://esp32_api;
    # ...
}
```

### Как защититься от DDoS?

1. **Rate limiting** (уже настроен в nginx.conf):
   ```nginx
   limit_req_zone $binary_remote_addr zone=mylimit:10m rate=10r/s;
   limit_req zone=mylimit burst=20;
   ```

2. **Cloudflare** (бесплатный план):
   - Защита от DDoS
   - CDN
   - SSL/TLS

3. **fail2ban**:
   ```bash
   sudo apt install fail2ban
   # Настроить для nginx
   ```

## Производительность

### Сколько одновременных подключений поддерживается?

Зависит от ресурсов Docker хоста:

| Платформа | Рекомендуемые подключения |
|-----------|---------------------------|
| Raspberry Pi Zero | 1-3 |
| Raspberry Pi 3 | 5-10 |
| Raspberry Pi 4 | 10-20 |
| VPS (2GB RAM) | 20-50 |
| Выделенный сервер | 50+ |

### Как улучшить производительность?

1. **SSD вместо SD карты** (для RPi)
2. **Больше RAM** (swap для RPi Zero)
3. **Отключить access логи**
4. **Использовать CDN** (Cloudflare)
5. **Оптимизация nginx** (см. ARCHITECTURE.md)

## Поддержка

### Где получить помощь?

1. **Документация**:
   - [README.md](README.md) - основная документация
   - [QUICKSTART.md](QUICKSTART.md) - быстрый старт
   - [EXAMPLES.md](EXAMPLES.md) - примеры использования
   - [ARCHITECTURE.md](ARCHITECTURE.md) - архитектура

2. **Логи**:
   ```bash
   docker-compose logs
   ```

3. **GitHub Issues**:
   - Создайте Issue с описанием проблемы
   - Приложите логи
   - Укажите версию системы

4. **Сообщество**:
   - Telegram группа (если есть)
   - Discord сервер (если есть)

### Как сообщить об ошибке?

1. Проверьте [FAQ](#) и [TROUBLESHOOTING](README.md#-устранение-неполадок)
2. Соберите информацию:
   ```bash
   # Версия Docker
   docker --version
   docker-compose --version
   
   # Система
   uname -a
   cat /etc/os-release
   
   # Логи
   docker-compose logs > logs.txt
   ./scripts/status.sh > status.txt
   ```
3. Создайте Issue на GitHub с:
   - Описанием проблемы
   - Шагами для воспроизведения
   - Ожидаемым и фактическим поведением
   - Логами и выводом status.sh

### Как предложить улучшение?

1. Проверьте существующие Issues
2. Создайте новый Issue с тегом "enhancement"
3. Опишите:
   - Что хотите улучшить
   - Зачем это нужно
   - Как это должно работать
4. Или создайте Pull Request с реализацией
