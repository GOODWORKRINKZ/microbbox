#!/usr/bin/env python3
"""
Генерация самоподписанного SSL сертификата для ESP32 HTTPS сервера
Создает сертификат и приватный ключ, затем конвертирует их в C header файл
"""

import os
import subprocess
import sys
from pathlib import Path

# PlatformIO script environment
Import("env")

def generate_certificate():
    """Генерирует самоподписанный сертификат для ESP32"""
    
    # Проверяем наличие FEATURE_HTTPS
    build_flags = env.ParseFlags(env['BUILD_FLAGS'])
    has_https = any('FEATURE_HTTPS' in flag for flag in build_flags.get('CPPDEFINES', []))
    
    if not has_https:
        print("HTTPS не включен (FEATURE_HTTPS не определен), пропускаем генерацию сертификата")
        return
    
    project_dir = env['PROJECT_DIR']
    include_dir = os.path.join(project_dir, 'include')
    cert_header = os.path.join(include_dir, 'cert.h')
    
    # Проверяем существует ли уже сертификат
    if os.path.exists(cert_header):
        print(f"Сертификат уже существует: {cert_header}")
        return
    
    print("=" * 60)
    print("Генерация самоподписанного SSL сертификата для HTTPS...")
    print("=" * 60)
    
    # Проверяем наличие OpenSSL
    try:
        subprocess.run(['openssl', 'version'], 
                      capture_output=True, 
                      check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("ОШИБКА: OpenSSL не найден!")
        print("Установите OpenSSL для генерации сертификата")
        print("  Ubuntu/Debian: sudo apt-get install openssl")
        print("  macOS: brew install openssl")
        print("  Windows: https://slproweb.com/products/Win32OpenSSL.html")
        sys.exit(1)
    
    # Временные файлы для сертификата и ключа
    temp_dir = os.path.join(project_dir, '.pio', 'build')
    os.makedirs(temp_dir, exist_ok=True)
    
    cert_file = os.path.join(temp_dir, 'server_cert.pem')
    key_file = os.path.join(temp_dir, 'server_key.pem')
    
    # Генерируем приватный ключ RSA 2048 бит
    print("Генерация приватного ключа RSA 2048 бит...")
    subprocess.run([
        'openssl', 'genrsa',
        '-out', key_file,
        '2048'
    ], check=True, capture_output=True)
    
    # Генерируем самоподписанный сертификат на 10 лет
    print("Генерация самоподписанного сертификата...")
    subprocess.run([
        'openssl', 'req',
        '-new', '-x509',
        '-key', key_file,
        '-out', cert_file,
        '-days', '3650',
        '-subj', '/CN=microbbox.local/O=MicroBox/C=RU'
    ], check=True, capture_output=True)
    
    # Читаем сертификат и ключ
    with open(cert_file, 'r') as f:
        cert_pem = f.read()
    
    with open(key_file, 'r') as f:
        key_pem = f.read()
    
    # Создаем C header файл
    print(f"Создание заголовочного файла: {cert_header}")
    
    header_content = """#ifndef CERT_H
#define CERT_H

// Автоматически сгенерированный самоподписанный сертификат
// Для WebXR/VR поддержки на ESP32
// Действителен 10 лет с момента генерации

// PEM сертификат
const char SSL_CERT[] = R"EOF(
{cert})EOF";

// PEM приватный ключ
const char SSL_KEY[] = R"EOF(
{key})EOF";

#endif // CERT_H
""".format(cert=cert_pem, key=key_pem)
    
    with open(cert_header, 'w') as f:
        f.write(header_content)
    
    print("✓ Сертификат успешно сгенерирован")
    print(f"  Сертификат: {cert_file}")
    print(f"  Ключ: {key_file}")
    print(f"  Header: {cert_header}")
    print("=" * 60)
    
    # Удаляем временные файлы
    try:
        os.remove(cert_file)
        os.remove(key_file)
    except:
        pass

# Запускаем генерацию при сборке
generate_certificate()
