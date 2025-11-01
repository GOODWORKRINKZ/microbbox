# Safe OTA Update Mode

## Overview

Safe OTA Mode is a firmware update mechanism designed to prevent watchdog timeouts and memory issues during Over-The-Air (OTA) updates on ESP32-CAM devices.

## The Problem

Standard OTA updates on ESP32-CAM devices face several challenges:

1. **Limited RAM**: ESP32 with camera uses significant memory (~50KB for camera buffers)
2. **Resource Competition**: Web server, camera stream, and OTA download compete for resources
3. **Watchdog Timeouts**: SSL handshakes and HTTPS redirects can take 10-15+ seconds, triggering watchdog
4. **async_tcp Task Starvation**: Heavy operations prevent the async_tcp task from resetting watchdog

## The Solution

Instead of downloading firmware while all services are running, the device:

1. **Saves the download URL** to EEPROM
2. **Reboots immediately** to free all resources
3. **Boots in "safe mode"** with minimal services
4. **Downloads and installs** firmware with full memory available
5. **Reboots normally** after completion

## How It Works

### User Flow

```
User clicks "Download Update"
    ↓
Frontend sends POST to /api/update/download
    ↓
Backend saves URL to EEPROM
    ↓
Backend sets ota.pending = true
    ↓
Backend sends response: {"rebooting": true}
    ↓
Backend reboots immediately
    ↓
Frontend shows "Rebooting..." message
    ↓
Device boots and checks OTA flag
    ↓
[OTA FLAG IS TRUE]
    ↓
Device boots in SAFE MODE
    ↓
Safe Mode: Initialize WiFi only
    ↓
Safe Mode: Create minimal web server
    ↓
Safe Mode: Download firmware from URL
    ↓
Safe Mode: Install firmware
    ↓
Safe Mode: Clear OTA flag
    ↓
Device reboots normally
    ↓
Frontend reconnects and shows success
```

### Code Flow

#### 1. User Initiates Update (Frontend)

```javascript
// In resources/script.js
async downloadAndInstallUpdate() {
    const formData = new FormData();
    formData.append('url', this.updateDownloadUrl);
    
    const response = await fetch('/api/update/download', {
        method: 'POST',
        body: formData
    });
    
    const data = await response.json();
    if (data.rebooting) {
        // Handle reboot and reconnection
        await this.waitForReboot();
        await this.reconnectToDevice();
        this.pollUpdateStatus();
    }
}
```

#### 2. Backend Saves and Reboots (C++)

```cpp
// In src/FirmwareUpdate.cpp
void FirmwareUpdate::handleDownloadAndInstall(AsyncWebServerRequest *request) {
    String url = request->getParam("url", true)->value();
    
    // Save URL to EEPROM
    preferences.begin("ota", false);
    preferences.putString("url", url);
    preferences.end();
    
    // Set OTA pending flag
    FirmwareUpdate::setOTAPending(true);
    
    // Send response and reboot
    request->send(200, "application/json", 
        "{\"status\":\"ok\",\"rebooting\":true}");
    delay(1000);
    ESP.restart();
}
```

#### 3. Boot-Time Check (C++)

```cpp
// In src/main.cpp
void setup() {
    // ... motor safety init ...
    Serial.begin(115200);
    
    // CHECK FOR PENDING OTA
    if (FirmwareUpdate::isOTAPending()) {
        Serial.println("OTA PENDING - SAFE MODE");
        robot = new MicroBoxRobot();
        robot->initSafeModeForOTA();  // Will reboot after OTA
        return;
    }
    
    // NORMAL BOOT
    robot = new MicroBoxRobot();
    robot->init();  // Full initialization
}
```

#### 4. Safe Mode Initialization (C++)

```cpp
// In src/MicroBoxRobot.cpp
bool MicroBoxRobot::initSafeModeForOTA() {
    // SKIP: Camera (saves ~50KB RAM + LEDC channels)
    // SKIP: Web server and camera stream (saves ~30KB RAM)
    // SKIP: Motors, NeoPixel, Buzzer (saves ~10KB RAM)
    
    // Initialize ONLY what's needed
    wifiSettings = new WiFiSettings();
    wifiSettings->init();
    firmwareUpdate = new FirmwareUpdate();
    
    // Connect to WiFi (use saved settings)
    if (wifiSettings->getMode() == WiFiMode::CLIENT) {
        connectToSavedWiFi();
    } else {
        startWiFiAP();
    }
    
    // Minimal web server (status page only)
    server = new AsyncWebServer(WIFI_PORT);
    firmwareUpdate->init(server);
    server->begin();
    
    // Read URL from EEPROM
    Preferences prefs;
    prefs.begin("ota", true);
    String url = prefs.getString("url", "");
    prefs.end();
    
    // Download and install (now we have resources!)
    bool success = firmwareUpdate->downloadAndInstallFirmware(url);
    
    // Clear flag and reboot to normal mode
    FirmwareUpdate::clearOTAPending();
    delay(2000);
    ESP.restart();
    
    return success;
}
```

## EEPROM Storage

The implementation uses ESP32's Preferences library (EEPROM abstraction) with two namespaces:

### `ota` Namespace
- **Key: `pending`** (bool) - Indicates if OTA update is pending
- **Key: `url`** (String) - Firmware download URL

### `firmware` Namespace
- **Key: `autoUpdate`** (bool) - Auto-update enabled
- **Key: `dontOffer`** (bool) - Don't offer updates

## Resource Comparison

### Normal Boot
```
Camera:        ~50KB RAM + LEDC 0-1
Web Server:    ~20KB RAM
Camera Stream: ~10KB RAM
Motors:        ~5KB RAM + LEDC 2-5
NeoPixel:      ~3KB RAM + LEDC 7
Buzzer:        ~2KB RAM + LEDC 6
------------------------------
Total:         ~90KB RAM + 8 LEDC channels
Available:     ~230KB RAM for OTA
```

### Safe Mode Boot
```
WiFi:          ~10KB RAM
Minimal Web:   ~5KB RAM
FirmwareUpdate:~5KB RAM
------------------------------
Total:         ~20KB RAM + 0 LEDC channels
Available:     ~300KB RAM for OTA
```

**Benefit: +70KB more RAM available for OTA download/install!**

## Failure Recovery

The system is designed to be fail-safe:

1. **If OTA download fails**: Flag is cleared, device reboots normally
2. **If OTA install fails**: Flag is cleared, device reboots normally
3. **If device loses power**: On next boot, flag is still set, safe mode retries
4. **If flag is stuck**: User can manually clear by uploading firmware via web UI

## Testing Checklist

- [ ] Normal boot without OTA pending works
- [ ] OTA request triggers reboot
- [ ] Device boots in safe mode when OTA pending
- [ ] Firmware downloads successfully in safe mode
- [ ] Firmware installs successfully
- [ ] Device clears flag and boots normally after OTA
- [ ] Failed OTA clears flag and recovers
- [ ] Frontend reconnects after reboot
- [ ] Frontend shows progress correctly
- [ ] User can still manually upload firmware if auto-OTA fails

## Debugging

Enable debug output to see the OTA flow:

```cpp
#define DEBUG 1
#define CORE_DEBUG_LEVEL 4
```

Watch for these log messages:

1. **Normal Boot**: `"НОРМАЛЬНАЯ ЗАГРУЗКА"`
2. **Safe Mode**: `"ОБНАРУЖЕНО ОЖИДАЮЩЕЕ OTA ОБНОВЛЕНИЕ"`
3. **Safe Mode Init**: `"БЕЗОПАСНЫЙ РЕЖИМ ДЛЯ OTA ОБНОВЛЕНИЯ"`
4. **Download Start**: `"Скачивание прошивки с: [URL]"`
5. **Success**: `"✓ Обновление успешно завершено!"`
6. **Failure**: `"✗ Ошибка обновления прошивки"`

## Known Limitations

1. **Requires WiFi**: Device must maintain WiFi connection through reboot
2. **Static IP recommended**: DHCP may assign different IP after reboot
3. **No progress during safe mode**: Web UI shows reconnection status only
4. **Two reboots**: One to enter safe mode, one to exit it

## Future Improvements

- [ ] Add progress reporting during safe mode download
- [ ] Implement retry logic with exponential backoff
- [ ] Add firmware signature verification
- [ ] Support downloading to SPIFFS first, then installing
- [ ] Add rollback capability if new firmware crashes
