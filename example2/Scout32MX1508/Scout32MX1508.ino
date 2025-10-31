 /*
Pilot Hobbies Scout32 - For ESP32
Version 2.2 - Should compile with ESP32 Boards Library 1.0.6+
MX1508 Changes to Pinout:
GPIO12 - IN1 - Left FWD
GPIO13 - IN2 - Left REV
GPIO15 - IN3 - Right FWD
GPIO14 - IN4 - Right REV
*/

// Define Network SSID & Password
// Set ap to 1 to use Scout32 as Standalone Access Point with default IP 192.168.4.1
// Set ap to 0 to connect to a router using DHCP with hostname espressif
bool ap = 1; 
const char* ssid = "Scout007"; //AP Name or Router SSID
const char* hostname = "Scout007"; //For access via Hostname
const char* password = ""; //Password. Leave blank for open network.

//AP Settings
int channel = 1; // Channel for AP Mode
int hidden = 0; // Probably leave at zero
int maxconnection = 1; // Only allow one at a time

// Libraries. If you get errors compiling, please downgrade ESP32 by Espressif.
// Use version 1.0.6 (Tools, Manage Libraries).
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Camera Pin Definitions - Don't heckin' touch.
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Webserver / Controls Function
void startCameraServer();

//Define Servos
const int ServoPinLF = 12; // Left Front Motor Driver
const int ServoPinLR = 13; // Left Rear Motor Driver
const int ServoPinRF = 15; // Right Front Motor Driver
const int ServoPinRR = 14; // Right Rear Motor Driver
void initServo() // Here we setup PWM, and attach it to physical pins.
{
  ledcSetup(3, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  ledcSetup(4, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  ledcSetup(5, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  ledcSetup(2, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  
  ledcAttachPin(ServoPinLF, 3); // Attach PWM 3 to GPIO 12
  ledcAttachPin(ServoPinLR, 4); // Attach PWM 4 to GPIO 13
  ledcAttachPin(ServoPinRR, 5); // Attach PWM 5 to GPIO 15
  ledcAttachPin(ServoPinRF, 2); // Attach PWM 2 to GPIO 14

}

void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Prevent brownouts by silencing them. You probably want to keep this.
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();


// Camera Configuration - Again, don't touch.
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 1);       // -2 to 2
  s->set_saturation(s, 1);     // -2 to 2
  s->set_gainceiling(s, (gainceiling_t)2);  // 0 to 6

  
  // Initialize Servos & LEDs
  initServo();
  ledcSetup(7, 5000, 8);
  ledcAttachPin(4, 7);  //GPIO 4 is LED
  
 if(!ap){
  // Connect to Router
  Serial.println("ssid: " + (String)ssid);
  Serial.println("password: " + (String)password);
  Serial.println("WiFi is Client Scout32");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
 } else {
  // Setup Access Point
  Serial.println("ssid: " + (String)ssid);
  Serial.println("password: " + (String)password);
  Serial.println("WiFi is Standalone Scout32");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid,password,channel,hidden,maxconnection);
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20); //Possible Fix for Interference??
}

  //Flash LED as ready indicator
  for (int i=0;i<5;i++) 
    {
      ledcWrite(7,0);  // flash led
      delay(200);
      ledcWrite(7,10);
      delay(200);    
    }       
  
  //Start Webserver
  startCameraServer();

}

void loop() {
  // put your main code here, to run repeatedly:
  //delay(1000);
  // Serial.printf("RSSi: %ld dBm\n",WiFi.RSSI()); 
}
