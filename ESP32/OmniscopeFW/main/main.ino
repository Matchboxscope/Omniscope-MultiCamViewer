//https://gist.github.com/klucsik/711a4f072d7194842840d725090fd0a7
#include <Arduino.h>
#include <Base64.h>
#include <base64.h>
#include <WiFi.h>
#include "HTTPClient.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "Blynk1";
const char *password = "12345678";

#ifdef CAMERA_MODEL_XIAO
#include "esp_camera.h"
#endif
#include "camera_pins.h"

#define BAUDRATE 500000
#ifdef CAMERA_MODEL_XIAO


long startTime = 0;

int lastBlink = 0;
int blinkState = 0;
int blinkPeriod = 500;
int uploadPeriod = 1000;
int lastUpload = 0;
int lastAnnounced = 0;
int announcePeriod = 10000;

// Broadcasting the server IP
WiFiUDP udp;
String serverURL = "http://192.168.2.191:5001";
unsigned int broadcastingPort = 12345; // local port to listen on


uint64_t macAddressToUint64() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    uint64_t macInt = 0;

    for (int i = 0; i < 6; ++i) {
        macInt |= ((uint64_t)mac[i] << (5 - i) * 8);
    }

    return macInt;
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
  // Serial.println("Camera init success!");
}
#endif


String receiveServerURL()
{
  // read the server IP address
  udp.begin(broadcastingPort);
  Serial.print("Now listening at IP ");
  Serial.print(WiFi.localIP());
  Serial.print(", UDP port ");
  Serial.println(broadcastingPort);
  int trialCounter = 0;
  while (1)
  {
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
      // read the packet into packetBufffer
      char packetBuffer[255];
      udp.read(packetBuffer, 255);
      String serverIp = String(packetBuffer).substring(0, packetSize);
      Serial.print("Server IP Address: ");
      Serial.println(serverIp);
      trialCounter += 1;
      return serverIp;
    }

    if (trialCounter > 10)
    {
      ESP.restart();
      return "0";
    }
  }
}


void setup()
{
  startTime = millis();
  Serial.begin(BAUDRATE);
  initCamera();
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");


  // receive the server IP address
  serverURL = receiveServerURL();
  Serial.println("Server IP: " + String(serverURL));


  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}


void loop()
{

  // blink the LED
  if (millis() - lastBlink > blinkPeriod)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    lastBlink = millis();
  }

  if (millis() - lastAnnounced > announcePeriod)
  {
    // Allocate new buffer (size of MAC address + size of image)
    uint64_t macInt = macAddressToUint64();

    // send the MAC address to the server 
    HTTPClient http;
    http.begin(serverURL+"/port"); // HTTP
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header
    int httpCode = http.sendRequest("POST", (uint8_t *)&macInt, sizeof(macInt)); // we simply put the whole image in the post body.
    lastAnnounced = millis();
  }

  if (millis() - lastUpload > uploadPeriod)
  {
    // Send image to server
    camera_fb_t *fb = esp_camera_fb_get();

    HTTPClient http;
    Serial.println("Upload");

    http.begin(serverURL+"/upload"); // HTTP
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header
    int httpCode = http.sendRequest("POST", fb->buf, fb->len); // we simply put the whole image in the post body.

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        Serial.println(payload);
      }
    }
    else
    {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end(); // Free up memory
    esp_camera_fb_return(fb);

    lastUpload = millis();
  }

  // periodically reboot at random times in case serial connection is lost
  if (false and (millis() - startTime > random(30000, 60000)))
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
    if (c == 's')
    {
      esp_sleep_enable_timer_wakeup(500000);
      esp_deep_sleep_start();
    }
    if (c == 't')
    {
      // return compile time
      Serial.println(__DATE__ " " __TIME__);
    }
    if (c == 'c')
    {
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
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
        for (int i = 0; i < 4; i++)
        {
          digitalWrite(LED_BUILTIN, LOW);
          delay(50);
          digitalWrite(LED_BUILTIN, HIGH);
          delay(50);
        }
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