# Устранение неполадок

## Диагностические команды

Используйте эти команды для быстрой диагностики проблем:

```bash
# Проверка статуса всех устройств
./scripts/status.sh

# Проверка работы контейнеров
docker-compose ps
docker-compose logs

# Проверка конфигурации nginx
docker exec microbbox-nginx nginx -t

# Проверка сертификатов
docker exec microbbox-certbot certbot certificates

# Проверка доступности устройства
ping 192.168.1.100
curl -I http://192.168.1.100
curl -I https://robot1.example.com

# Проверка DNS
nslookup robot1.example.com
dig robot1.example.com

# Проверка портов
telnet 192.168.1.100 80
nc -zv 192.168.1.100 80

# Просмотр логов конкретного устройства
tail -f nginx/logs/robot1-access.log
tail -f nginx/logs/robot1-error.log

# Мониторинг ресурсов
htop
docker stats
```

## Сценарии проблем

### 1. Контейнеры не запускаются

**Симптомы:**
- `docker-compose ps` показывает статус "Exit" или "Restarting"
- Контейнеры постоянно перезапускаются

**Диагностика:**
```bash
# Просмотр логов
docker-compose logs

# Проверка конкретного контейнера
docker logs microbbox-nginx
docker logs microbbox-certbot

# Проверка ресурсов
df -h  # Проверка места на диске
free -h  # Проверка памяти
```

**Решения:**

1. **Недостаточно места на диске:**
   ```bash
   # Очистка неиспользуемых образов и контейнеров
   docker system prune -a
   
   # Очистка старых логов
   rm -f nginx/logs/*.log.gz
   truncate -s 0 nginx/logs/*.log
   ```

2. **Недостаточно памяти:**
   ```bash
   # Добавление swap файла (для Pi Zero)
   sudo dd if=/dev/zero of=/swapfile bs=1M count=1024
   sudo chmod 600 /swapfile
   sudo mkswap /swapfile
   sudo swapon /swapfile
   
   # Автоматическое монтирование при загрузке
   echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
   ```

3. **Ошибка в конфигурации nginx:**
   ```bash
   # Проверка синтаксиса
   docker exec microbbox-nginx nginx -t
   
   # Если контейнер не запускается, проверьте конфигурацию вручную
   cat nginx/nginx.conf
   cat nginx/conf.d/*.conf
   ```

### 2. Let's Encrypt не выдает сертификат

**Симптомы:**
- Ошибка при выполнении `obtain-certificate.sh`
- "Failed authorization procedure" в логах certbot
- "Connection refused" или "404 Not Found"

**Диагностика:**
```bash
# Проверка логов certbot
docker logs microbbox-certbot

# Проверка доступности порта 80 из интернета
curl -I http://YOUR_PUBLIC_IP/.well-known/acme-challenge/test

# Проверка DNS
dig robot1.example.com
nslookup robot1.example.com 8.8.8.8

# Проверка проброса портов
# С другого устройства вне локальной сети:
curl -I http://YOUR_PUBLIC_IP
```

**Решения:**

1. **DNS не настроен:**
   ```bash
   # Убедитесь, что A-запись указывает на ваш публичный IP
   dig robot1.example.com +short
   # Должен вернуть ваш публичный IP
   
   # Узнать свой публичный IP
   curl ifconfig.me
   ```

2. **Порт 80 недоступен:**
   - Проверьте настройки port forwarding на роутере
   - Убедитесь, что порт не блокируется провайдером
   - Проверьте firewall на Raspberry Pi:
   ```bash
   sudo ufw status
   sudo ufw allow 80/tcp
   sudo ufw allow 443/tcp
   ```

3. **nginx не отдает challenge:**
   ```bash
   # Проверка конфигурации
   docker exec microbbox-nginx cat /etc/nginx/conf.d/robot1.example.com.conf | grep acme-challenge
   
   # Должна быть секция:
   # location /.well-known/acme-challenge/ {
   #     root /var/www/certbot;
   # }
   
   # Проверка доступности директории
   docker exec microbbox-certbot ls -la /var/www/certbot/.well-known/acme-challenge/
   ```

4. **Лимиты Let's Encrypt:**
   ```bash
   # Используйте staging режим для тестирования
   docker exec microbbox-certbot certbot certonly \
     --webroot \
     --webroot-path=/var/www/certbot \
     --staging \
     --email admin@example.com \
     --agree-tos \
     --domain robot1.example.com
   ```

### 3. Устройство недоступно через proxy

**Симптомы:**
- Страница "Устройство недоступно" при обращении к домену
- 502 Bad Gateway или 504 Gateway Timeout
- Прямой доступ к ESP32 работает, но через proxy нет

**Диагностика:**
```bash
# Проверка доступности ESP32 напрямую
ping 192.168.1.100
curl -v http://192.168.1.100

# Проверка портов
nc -zv 192.168.1.100 80
nc -zv 192.168.1.100 81

# Проверка nginx логов
tail -f nginx/logs/robot1-error.log

# Проверка nginx конфигурации
docker exec microbbox-nginx nginx -t
cat nginx/conf.d/robot1.example.com.conf | grep "server.*:"
```

**Решения:**

1. **Неправильный IP адрес в конфигурации:**
   ```bash
   # Проверьте IP в конфигурации
   cat nginx/conf.d/robot1.example.com.conf | grep upstream -A 1
   
   # Должно быть:
   # upstream robot1_api {
   #     server 192.168.1.100:80 ...
   
   # Если неправильно, исправьте и перезагрузите
   nano nginx/conf.d/robot1.example.com.conf
   docker exec microbbox-nginx nginx -s reload
   ```

2. **ESP32 получил другой IP:**
   ```bash
   # Найдите текущий IP ESP32 на роутере или через Serial Monitor
   
   # Обновите конфигурацию
   nano nginx/conf.d/robot1.example.com.conf
   # Замените старый IP на новый
   
   # Или зарезервируйте IP на роутере (рекомендуется)
   ```

3. **Firewall блокирует соединения:**
   ```bash
   # Проверка firewall на Raspberry Pi
   sudo iptables -L -n
   
   # Разрешение соединений с ESP32
   sudo iptables -A INPUT -s 192.168.1.100 -j ACCEPT
   ```

4. **nginx не может резолвить upstream:**
   ```bash
   # Проверка DNS в контейнере
   docker exec microbbox-nginx nslookup 192.168.1.100
   docker exec microbbox-nginx ping -c 1 192.168.1.100
   
   # Если не работает, проверьте сетевой режим контейнера
   docker network inspect microbbox-network
   ```

### 4. DNS не работает в локальной сети

**Симптомы:**
- `nslookup robot1.example.com` не возвращает IP
- Браузер показывает "Сайт не найден"
- Прямой доступ по IP работает

**Диагностика:**
```bash
# Проверка DNS на клиенте
nslookup robot1.example.com
dig robot1.example.com

# Проверка какой DNS используется
cat /etc/resolv.conf  # Linux/macOS
ipconfig /all  # Windows

# Проверка настроек роутера
telnet my.keenetic.net
show ip name
```

**Решения:**

1. **DNS записи не настроены на роутере:**
   ```bash
   # Через telnet к роутеру
   telnet my.keenetic.net
   ip name robot1.example.com 192.168.1.50
   system configuration save
   exit
   ```

2. **Клиент не использует роутер как DNS:**
   - Windows: Панель управления → Сеть → Свойства адаптера → TCP/IPv4
   - Linux: Проверьте `/etc/resolv.conf` или настройки NetworkManager
   - macOS: Системные настройки → Сеть → Дополнительно → DNS

3. **DNS кэш устарел:**
   ```bash
   # Windows
   ipconfig /flushdns
   
   # macOS
   sudo dscacheutil -flushcache
   sudo killall -HUP mDNSResponder
   
   # Linux (systemd)
   sudo systemd-resolve --flush-caches
   
   # Linux (nscd)
   sudo service nscd restart
   ```

4. **DNS-прокси не запущен на роутере:**
   - Откройте веб-интерфейс роутера
   - Управление → Компоненты → Установите "DNS-прокси"
   - Перезагрузите роутер

### 5. WebXR не работает

**Симптомы:**
- Кнопка VR недоступна или не работает
- Ошибка "WebXR не поддерживается" в консоли
- VR режим не активируется

**Диагностика:**
```bash
# Проверка SSL сертификата
openssl s_client -connect robot1.example.com:443 -servername robot1.example.com

# В браузере VR гарнитуры
# Откройте DevTools (если доступно)
# Проверьте консоль на ошибки
```

**Решения:**

1. **Используется HTTP вместо HTTPS:**
   - Убедитесь, что открываете `https://` а не `http://`
   - Проверьте редирект с HTTP на HTTPS в конфигурации

2. **Сертификат невалиден или самоподписанный:**
   ```bash
   # Проверка сертификата
   docker exec microbbox-certbot certbot certificates
   
   # Переполучение сертификата если нужно
   sudo ./scripts/obtain-certificate.sh robot1.example.com
   ```

3. **Браузер не поддерживает WebXR:**
   - На Quest используйте Oculus Browser (рекомендуется)
   - Или установите Firefox Reality / Wolvic
   - Проверьте что браузер обновлен до последней версии

4. **Permissions-Policy заголовок неправильный:**
   ```bash
   # Проверьте что в nginx конфигурации есть:
   cat nginx/conf.d/robot1.example.com.conf | grep Permissions-Policy
   
   # Должно быть:
   # add_header Permissions-Policy "accelerometer=*, camera=*, gyroscope=*, magnetometer=*, microphone=*, xr-spatial-tracking=*" always;
   ```

### 6. Медленная работа или высокая задержка

**Симптомы:**
- Видеопоток отстает
- Управление откликается с задержкой
- Высокая загрузка CPU на Raspberry Pi

**Диагностика:**
```bash
# Мониторинг ресурсов
htop
docker stats

# Проверка сетевой задержки
ping 192.168.1.100
ping 192.168.1.50

# Проверка пропускной способности
iperf3 -s  # На Raspberry Pi
iperf3 -c 192.168.1.50  # С другого устройства

# Анализ трафика
sudo tcpdump -i wlan0 -n host 192.168.1.100
```

**Решения:**

1. **Raspberry Pi Zero перегружен:**
   - Используйте Raspberry Pi 3 или 4 для лучшей производительности
   - Уменьшите количество одновременно работающих устройств
   - Отключите ненужные сервисы

2. **Слабый WiFi сигнал:**
   - Переместите устройства ближе к роутеру
   - Используйте WiFi 5GHz вместо 2.4GHz
   - Подключите Raspberry Pi через Ethernet

3. **Высокое разрешение камеры:**
   - Уменьшите разрешение в настройках ESP32
   - Снизьте FPS видеопотока
   - Используйте меньшее качество JPEG

4. **Неоптимальная конфигурация nginx:**
   ```nginx
   # Проверьте что используются оптимальные настройки
   proxy_buffering off;  # Для live streaming
   sendfile on;          # Для производительности
   tcp_nodelay on;       # Для низкой задержки
   ```

### 7. Сертификат истек или скоро истечет

**Симптомы:**
- Браузер показывает предупреждение о сертификате
- "Your connection is not private" в Chrome
- ERR_CERT_DATE_INVALID

**Диагностика:**
```bash
# Проверка даты истечения
docker exec microbbox-certbot certbot certificates

# Проверка обновления сертификатов
docker logs microbbox-certbot | grep -i renew
```

**Решения:**

1. **Ручное обновление:**
   ```bash
   # Принудительное обновление
   docker exec microbbox-certbot certbot renew --force-renewal
   
   # Перезагрузка nginx
   docker exec microbbox-nginx nginx -s reload
   ```

2. **Автоматическое обновление не работает:**
   ```bash
   # Проверка что certbot контейнер запущен
   docker ps | grep certbot
   
   # Перезапуск если нужно
   docker-compose restart certbot
   
   # Проверка логов на ошибки
   docker logs microbbox-certbot
   ```

3. **Проблемы с обновлением:**
   ```bash
   # Удаление старого сертификата
   docker exec microbbox-certbot certbot delete --cert-name robot1.example.com
   
   # Получение нового
   sudo ./scripts/obtain-certificate.sh robot1.example.com
   ```

## Полезные команды для отладки

```bash
# Полная перезагрузка системы
docker-compose down
docker-compose up -d

# Очистка и перезапуск
docker-compose down -v  # Удалит volumes!
rm -rf certbot/conf certbot/www
docker-compose up -d

# Просмотр всех логов в реальном времени
docker-compose logs -f

# Интерактивная оболочка в контейнере
docker exec -it microbbox-nginx sh
docker exec -it microbbox-certbot sh

# Проверка сети
docker network inspect microbbox-network

# Мониторинг соединений
netstat -tupln | grep nginx
ss -tupln | grep nginx

# Тест производительности
ab -n 100 -c 10 https://robot1.example.com/
wrk -t2 -c10 -d30s https://robot1.example.com/
```

## Когда обращаться за помощью

Если после всех проверок проблема не решена:

1. Соберите диагностическую информацию:
   ```bash
   # Создайте файл с диагностикой
   ./scripts/status.sh > diagnostics.txt
   docker-compose logs >> diagnostics.txt
   docker ps >> diagnostics.txt
   ```

2. Создайте Issue на GitHub с:
   - Описанием проблемы
   - Шагами для воспроизведения
   - Файлом диагностики
   - Версиями ПО (Docker, ОС, роутер)

3. Проверьте существующие Issues - возможно проблема уже решена

## Профилактика проблем

Рекомендации для стабильной работы:

1. **Регулярные обновления:**
   ```bash
   # Обновление образов Docker
   docker-compose pull
   docker-compose up -d
   
   # Обновление системы
   sudo apt update && sudo apt upgrade -y
   ```

2. **Мониторинг места на диске:**
   ```bash
   # Ротация логов
   find nginx/logs -name "*.log" -mtime +7 -delete
   
   # Или настройте logrotate (см. FAQ)
   ```

3. **Резервное копирование:**
   ```bash
   # Бэкап конфигураций
   tar -czf backup-$(date +%Y%m%d).tar.gz \
     nginx/conf.d/*.conf \
     docker-compose.yml
   ```

4. **Мониторинг сертификатов:**
   ```bash
   # Еженедельная проверка
   docker exec microbbox-certbot certbot certificates
   ```

5. **Проверка здоровья системы:**
   ```bash
   # Создайте cron job для периодической проверки
   */5 * * * * /home/pi/microbbox/nginx-proxy/scripts/status.sh > /tmp/status.log 2>&1
   ```
