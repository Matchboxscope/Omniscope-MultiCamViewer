#include <Arduino.h>
#include <Base64.h>
#include <base64.h>
#ifdef CAMERA_MODEL_XIAO
#include "esp_camera.h"
#endif
#include "camera_pins.h"

#ifdef CAMERA_MODEL_XIAO
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
  if (!psramFound())
  {
    Serial.println("PSRAM NOT FOUND");
    ESP.restart();
  }
  // Serial.println("PSRAM FOUND");
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_SVGA; // iFRAMESIZE_UXGA;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    log_e("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  //Serial.println("Camera init success!");
}
#endif

long startTime = 0;

void setup()
{
  startTime = millis();
  Serial.begin(2000000);
  initCamera();
}

void loop()
{
  // periodically reboot at random times in case serial connection is lost
  if (millis() - startTime > random(30000, 60000))
  {
    // cut the line for 0.5 second by deepsleep to force a reboot
    esp_sleep_enable_timer_wakeup(500000);
    esp_deep_sleep_start();
  }

  if (Serial.available())
  {
    //
    char c = Serial.read();
    if (c == 'p')
      Serial.println("ping!");
    if (c == 'r')
      ESP.restart();
    if (c == 's'){
      esp_sleep_enable_timer_wakeup(500000);
      esp_deep_sleep_start();
    }
    if (c=='t'){
      // return compile time
      Serial.println(__DATE__ " " __TIME__);
    }
    if (c == 'c')
    {
      startTime = millis();

      // Take picture and read the frame buffer
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.println("FAIL");
      }
      else
      {
        String encoded = base64::encode(fb->buf, fb->len);
        Serial.write(encoded.c_str(), encoded.length());
        Serial.println();
      }
      esp_camera_fb_return(fb);
    }
  }
}

/*
void grabImage()
{

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG)
  {
  }
  else
  {
    String encoded = base64::encode(fb->buf, fb->len);
    Serial.write(encoded.c_str(), encoded.length());
    Serial.println();
  }
  esp_camera_fb_return(fb);
}
*/