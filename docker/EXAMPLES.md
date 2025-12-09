# Примеры использования Docker Proxy для ESP32

## Пример 1: Домашняя сеть с роутером Keenetic

### Сценарий
- Роутер Keenetic с поддержкой локального DNS
- Raspberry Pi 4 как Docker хост
- ESP32 с статическим IP
- Локальный доступ без выхода в интернет

### Настройка

#### 1. Настройка ESP32

Зайдите на ESP32 и настройте статический IP или зарезервируйте IP на роутере:
- IP: `192.168.1.100`
- Gateway: `192.168.1.1`
- DNS: `192.168.1.1`

#### 2. Настройка Raspberry Pi

```bash
# Клонирование репозитория
git clone https://github.com/GOODWORKRINKZ/microbbox.git
cd microbbox/docker

# Настройка .env
cat > .env << 'EOF'
ESP32_IP=192.168.1.100
ESP32_API_PORT=80
ESP32_STREAM_PORT=81
DOMAIN_NAME=robot.home
DEVICE_NAME=myrobot
CERT_MODE=selfsigned
TZ=Europe/Moscow
EOF

# Запуск
./scripts/setup.sh
./scripts/generate-selfsigned-cert.sh
```

#### 3. Настройка DNS на роутере Keenetic

Веб-интерфейс:
1. Открыть `http://192.168.1.1`
2. Интернет-фильтры → Серверы имен → Локальные доменные имена
3. Добавить: `robot.home → 192.168.1.50` (IP Raspberry Pi)

Или через telnet:
```bash
telnet 192.168.1.1
ip name robot.home 192.168.1.50
system configuration save
```

#### 4. Использование

Откройте на любом устройстве в локальной сети:
- `https://robot.home/`
- `https://robot.home/stream`

При первом подключении примите самоподписанный сертификат.

---

## Пример 2: Публичный доступ с динамическим DNS

### Сценарий
- Динамический внешний IP (меняется провайдером)
- Сервис DynDNS (например, No-IP, DuckDNS)
- Ubuntu сервер как Docker хост
- Доступ из интернета с валидным SSL

### Настройка

#### 1. Регистрация на DynDNS сервисе

Например, DuckDNS:
1. Зайти на https://www.duckdns.org/
2. Зарегистрироваться через GitHub/Google
3. Создать домен: `myrobot.duckdns.org`
4. Установить клиент обновления IP

#### 2. Установка DuckDNS клиента

```bash
# Создать директорию
mkdir ~/duckdns
cd ~/duckdns

# Создать скрипт обновления
echo 'url="https://www.duckdns.org/update?domains=myrobot&token=YOUR_TOKEN&ip="' > duck.sh
echo 'echo url="$url" | curl -k -o ~/duckdns/duck.log -K -' >> duck.sh
chmod +x duck.sh

# Добавить в crontab (обновление каждые 5 минут)
crontab -e
# Добавить строку:
# */5 * * * * ~/duckdns/duck.sh >/dev/null 2>&1
```

#### 3. Настройка Docker Proxy

```bash
cd ~/microbbox/docker

cat > .env << 'EOF'
ESP32_IP=192.168.1.100
ESP32_API_PORT=80
ESP32_STREAM_PORT=81
DOMAIN_NAME=myrobot.duckdns.org
LETSENCRYPT_EMAIL=your@email.com
CERT_MODE=production
DEVICE_NAME=myrobot
TZ=Europe/Moscow
EOF

./scripts/setup.sh
```

#### 4. Проброс портов на роутере

| Внешний | Внутренний IP | Внутренний порт |
|---------|---------------|-----------------|
| 80      | 192.168.1.50  | 80              |
| 443     | 192.168.1.50  | 443             |

#### 5. Получение SSL сертификата

```bash
./scripts/obtain-certificate.sh
```

#### 6. Использование

Доступ из любой точки мира:
- `https://myrobot.duckdns.org/`
- `https://myrobot.duckdns.org/stream`

---

## Пример 3: VPS сервер с белым IP

### Сценарий
- VPS с белым статическим IP
- Собственный домен
- ESP32 в домашней сети
- VPN туннель между VPS и домашней сетью

### Настройка

#### 1. Настройка VPN туннеля (WireGuard)

На VPS:
```bash
# Установка WireGuard
sudo apt update
sudo apt install wireguard

# Генерация ключей
wg genkey | tee server_private.key | wg pubkey > server_public.key

# Конфигурация /etc/wireguard/wg0.conf
cat > /etc/wireguard/wg0.conf << 'EOF'
[Interface]
PrivateKey = SERVER_PRIVATE_KEY
Address = 10.0.0.1/24
ListenPort = 51820

[Peer]
PublicKey = CLIENT_PUBLIC_KEY
AllowedIPs = 10.0.0.2/32, 192.168.1.0/24
EOF

# Запуск
sudo systemctl enable wg-quick@wg0
sudo systemctl start wg-quick@wg0
```

На домашнем роутере/сервере:
```bash
# Аналогично настроить WireGuard клиент
# После подключения ESP32 будет доступен через VPN
```

#### 2. Настройка DNS

В панели управления доменом:
- A-запись: `robot.yourdomain.com → IP_VPS`

#### 3. Настройка Docker Proxy на VPS

```bash
cd ~/microbbox/docker

cat > .env << 'EOF'
ESP32_IP=10.0.0.2  # IP ESP32 через VPN
ESP32_API_PORT=80
ESP32_STREAM_PORT=81
DOMAIN_NAME=robot.yourdomain.com
LETSENCRYPT_EMAIL=admin@yourdomain.com
CERT_MODE=production
DEVICE_NAME=robot
TZ=Europe/Moscow
EOF

./scripts/setup.sh
./scripts/obtain-certificate.sh
```

#### 4. Использование

Доступ из любой точки мира:
- `https://robot.yourdomain.com/`
- `https://robot.yourdomain.com/stream`

---

## Пример 4: Несколько ESP32 на одном хосте

### Сценарий
- 3 ESP32 устройства
- Один Docker хост
- Разные домены для каждого робота

### Настройка

#### 1. Резервирование IP для всех ESP32

На роутере:
- ESP32 #1: `192.168.1.100`
- ESP32 #2: `192.168.1.101`
- ESP32 #3: `192.168.1.102`

#### 2. Создание отдельных Docker стеков

```bash
# Робот 1
mkdir -p ~/proxy/robot1
cd ~/proxy/robot1
cp -r ~/microbbox/docker/* .

cat > .env << 'EOF'
ESP32_IP=192.168.1.100
DOMAIN_NAME=robot1.example.com
DEVICE_NAME=robot1
CERT_MODE=production
LETSENCRYPT_EMAIL=admin@example.com
EOF

./scripts/setup.sh

# Робот 2
mkdir -p ~/proxy/robot2
cd ~/proxy/robot2
cp -r ~/microbbox/docker/* .

cat > .env << 'EOF'
ESP32_IP=192.168.1.101
DOMAIN_NAME=robot2.example.com
DEVICE_NAME=robot2
CERT_MODE=production
LETSENCRYPT_EMAIL=admin@example.com
EOF

./scripts/setup.sh

# Робот 3
mkdir -p ~/proxy/robot3
cd ~/proxy/robot3
cp -r ~/microbbox/docker/* .

cat > .env << 'EOF'
ESP32_IP=192.168.1.102
DOMAIN_NAME=robot3.example.com
DEVICE_NAME=robot3
CERT_MODE=production
LETSENCRYPT_EMAIL=admin@example.com
EOF

./scripts/setup.sh
```

#### 3. Получение сертификатов

```bash
cd ~/proxy/robot1 && ./scripts/obtain-certificate.sh
cd ~/proxy/robot2 && ./scripts/obtain-certificate.sh
cd ~/proxy/robot3 && ./scripts/obtain-certificate.sh
```

#### 4. Использование

- Robot 1: `https://robot1.example.com/`
- Robot 2: `https://robot2.example.com/`
- Robot 3: `https://robot3.example.com/`

---

## Пример 5: Разработка на ноутбуке

### Сценарий
- Разработчик с ноутбуком
- ESP32 для тестирования
- Без домена, локальный доступ
- Быстрое развертывание

### Настройка

#### 1. Подключение ESP32 и ноутбука к одной сети

Узнайте IP ESP32 (например, из Serial Monitor или роутера).

#### 2. Быстрая настройка

```bash
cd ~/projects/microbbox/docker

# Минимальная конфигурация
cat > .env << 'EOF'
ESP32_IP=192.168.1.100
DOMAIN_NAME=robot.local
CERT_MODE=selfsigned
DEVICE_NAME=dev-robot
EOF

./scripts/setup.sh
./scripts/generate-selfsigned-cert.sh
```

#### 3. Добавление в /etc/hosts

Linux/macOS:
```bash
sudo sh -c 'echo "127.0.0.1 robot.local" >> /etc/hosts'
```

Windows (PowerShell as Admin):
```powershell
Add-Content C:\Windows\System32\drivers\etc\hosts "127.0.0.1 robot.local"
```

#### 4. Использование

На том же ноутбуке:
- `https://robot.local/`
- `https://robot.local/stream`

---

## Пример 6: WebXR на Oculus Quest

### Сценарий
- Oculus Quest 2 VR гарнитура
- ESP32 робот для управления
- WebXR требует валидный HTTPS
- Доступ из локальной сети

### Настройка

#### 1. Получение домена с валидным SSL

Используйте Пример 2 (DynDNS) или Пример 3 (VPS).

Важно: WebXR **не работает** с самоподписанными сертификатами!

#### 2. Настройка как в Примере 2

```bash
# .env
DOMAIN_NAME=myrobot.duckdns.org
CERT_MODE=production
```

#### 3. Использование на Oculus Quest

1. Включите Oculus Quest
2. Откройте Oculus Browser
3. Перейдите на `https://myrobot.duckdns.org/`
4. Нажмите кнопку "🥽 VR" в интерфейсе
5. Разрешите доступ к WebXR
6. Управляйте роботом с контроллеров!

---

## Пример 7: Raspberry Pi Zero с ограниченными ресурсами

### Сценарий
- Raspberry Pi Zero W (512MB RAM)
- Один ESP32 робот
- Минимальное потребление ресурсов

### Оптимизация

#### 1. Базовая настройка

Следуйте Примеру 1, затем оптимизируйте.

#### 2. Ограничение ресурсов Docker

Отредактируйте `docker-compose.yml`:

```yaml
services:
  nginx:
    # ... existing config ...
    deploy:
      resources:
        limits:
          cpus: '0.5'
          memory: 128M
        reservations:
          memory: 64M
```

#### 3. Оптимизация nginx

Отредактируйте `nginx/nginx.conf`:

```nginx
events {
    worker_connections 256;  # уменьшено с 1024
}

http {
    access_log off;  # отключить access логи
    # ... rest of config
}
```

#### 4. Отключение certbot (если используется selfsigned)

В `docker-compose.yml` закомментируйте certbot сервис.

#### 5. Мониторинг ресурсов

```bash
# Проверка использования памяти
free -h

# Проверка использования Docker
docker stats
```

---

## Пример 8: Автоматическое развертывание с Ansible

### Сценарий
- Множество Raspberry Pi хостов
- Автоматическое развертывание на всех
- Централизованное управление

### Ansible Playbook

```yaml
# deploy-proxy.yml
---
- hosts: raspberry_pi
  become: yes
  vars:
    docker_dir: /home/pi/microbbox-proxy
  
  tasks:
    - name: Install Docker
      shell: curl -fsSL https://get.docker.com | sh
      args:
        creates: /usr/bin/docker
    
    - name: Install Docker Compose
      apt:
        name: docker-compose
        state: present
    
    - name: Clone repository
      git:
        repo: https://github.com/GOODWORKRINKZ/microbbox.git
        dest: "{{ docker_dir }}"
        version: main
    
    - name: Configure .env
      template:
        src: env.j2
        dest: "{{ docker_dir }}/docker/.env"
    
    - name: Run setup script
      command: ./scripts/setup.sh
      args:
        chdir: "{{ docker_dir }}/docker"
    
    - name: Generate self-signed certificate
      command: ./scripts/generate-selfsigned-cert.sh
      args:
        chdir: "{{ docker_dir }}/docker"
      when: cert_mode == "selfsigned"
```

### Inventory

```ini
# hosts.ini
[raspberry_pi]
pi1 ansible_host=192.168.1.51 esp32_ip=192.168.1.100 domain=robot1.home
pi2 ansible_host=192.168.1.52 esp32_ip=192.168.1.101 domain=robot2.home
pi3 ansible_host=192.168.1.53 esp32_ip=192.168.1.102 domain=robot3.home

[raspberry_pi:vars]
ansible_user=pi
cert_mode=selfsigned
```

### Запуск

```bash
ansible-playbook -i hosts.ini deploy-proxy.yml
```

---

## Советы и лучшие практики

### 1. Резервное копирование сертификатов

```bash
# Бэкап сертификатов
tar -czf cert-backup-$(date +%Y%m%d).tar.gz certbot/conf/

# Восстановление
tar -xzf cert-backup-20240101.tar.gz
```

### 2. Мониторинг срока действия сертификатов

```bash
# Добавить в crontab
0 0 * * * docker exec microbbox-proxy-certbot certbot certificates | mail -s "Certificate Status" admin@example.com
```

### 3. Автоматический перезапуск при сбое

```bash
# systemd service для автозапуска
cat > /etc/systemd/system/microbbox-proxy.service << 'EOF'
[Unit]
Description=MicroBox HTTPS Proxy
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/home/pi/microbbox/docker
ExecStart=/usr/bin/docker-compose up -d
ExecStop=/usr/bin/docker-compose down

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl enable microbbox-proxy
sudo systemctl start microbbox-proxy
```

### 4. Логирование в файл

```bash
# Добавить в docker-compose.yml
services:
  nginx:
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```
