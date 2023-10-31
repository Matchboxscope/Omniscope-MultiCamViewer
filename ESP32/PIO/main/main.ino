#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include <stdio.h>
#include "camera_pins.h"

#define FLASH_PIN 4

const char* ssid = "BenMur"; // Your wifi name like "myWifiNetwork"
const char* password = "MurBen3128"; // Your password to the wifi network like "password123"
const char* websocket_server_host = "192.168.2.191";
const uint16_t websocket_server_port1 = 8004;

int flashlight = 0;

using namespace websockets;
WebsocketsClient client;

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connection Closed");
        ESP.restart();
    }
}

void onMessageCallback(WebsocketsMessage message) {
    String data = message.data();
    int index = data.indexOf("=");
    if(index != -1) {
        String key = data.substring(0, index);
        String value = data.substring(index + 1);

        if(key == "ON_BOARD_LED") {
          if(value.toInt() == 1) {
            flashlight = 1;
            digitalWrite(FLASH_PIN, HIGH);
          } else {
            flashlight = 0;
            digitalWrite(FLASH_PIN, LOW);
          }
        }
        
        Serial.print("Key: ");
        Serial.println(key);
        Serial.print("Value: ");
        Serial.println(value);
    }
}

void initCamera()
{
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
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  /*
     FRAMESIZE_UXGA (1600 x 1200)
    FRAMESIZE_QVGA (320 x 240)
    FRAMESIZE_CIF (352 x 288)
    FRAMESIZE_VGA (640 x 480)
    FRAMESIZE_SVGA (800 x 600)
    FRAMESIZE_XGA (1024 x 768)
    FRAMESIZE_SXGA (1280 x 1024)
  */
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound())
  {
    Serial.println("PSRAM FOUND");
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.frame_size = FRAMESIZE_VGA; // iFRAMESIZE_UXGA;
  }
  else
  {
    log_d("NO PSRAM");
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_VGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    log_e("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  initCamera();

  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  while(!client.connect(websocket_server_host, websocket_server_port1, "/")) { delay(500); }
}

int t0 = 0;

float computeMeanFrame(uint8_t *data, int len)
{
  float mean = 0;
  for(int i = 0; i < len; i++)
  {
    mean += data[i];
  }
  return mean / len;
}

void loop() 
{
  //Serial.println("Framerate : " + String(1000 / (1+ millis() - t0)) + " fps");
  t0 = millis();
  client.poll();
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb)
  {
    esp_camera_fb_return(fb);
    //Serial.println("Camera Capture Failed");
    return;
  }
 // Serial.println("Frame Mean: " + String(computeMeanFrame(fb->buf, fb->len)));

  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
//  client.send("TEST");
}
