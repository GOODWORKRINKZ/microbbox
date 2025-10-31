// CameraServer.cpp - Отдельный HTTP сервер для стрима камеры
// Используется ESP-IDF httpd для избежания конфликтов с AsyncWebServer

#include "esp_http_server.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"

// Глобальный httpd сервер для камеры
static httpd_handle_t camera_httpd = NULL;

// Stream encoding
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if(!jpeg_converted){
                    Serial.println("JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        
        if(res == ESP_OK){
            size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        
        if(res != ESP_OK){
            break;
        }
    }
    
    return res;
}

// Публичная функция для запуска камера-сервера
void startCameraStreamServer() {
    Serial.println("Запуск камера-сервера...");
    
    // Конфигурация httpd сервера
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81; // Используем порт 81 для камеры (AsyncWebServer на 80)
    config.ctrl_port = 32769;
    
    // Регистрация URI обработчика для стрима
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    
    // Запуск HTTP сервера
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &stream_uri);
        Serial.println("Камера-сервер запущен на порту 81");
        Serial.println("Стрим доступен: http://[IP]:81/stream");
    } else {
        Serial.println("Ошибка запуска камера-сервера!");
    }
}

void stopCameraStreamServer() {
    if (camera_httpd) {
        httpd_stop(camera_httpd);
        camera_httpd = NULL;
        Serial.println("Камера-сервер остановлен");
    }
}
