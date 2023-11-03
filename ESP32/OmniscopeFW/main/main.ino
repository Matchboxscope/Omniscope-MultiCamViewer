#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include <stdio.h>
#include "camera_pins.h"
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

#define FLASH_PIN 4

const char *ssid = "BenMur";         // Your wifi name like "myWifiNetwork"
const char *password = "MurBen3128"; // Your password to the wifi network like "password123"

// default values; will be updated
String websocket_server_host = "192.168.2.191";
uint16_t websocket_server_port = 8004;

// Broadcasting the server IP
unsigned int broadcastingPort = 12345; // local port to listen on
unsigned int replyPort = 8000;         // local port to listen on
WiFiUDP udp;
AsyncWebServer server(80);
Preferences preferences;
bool isSendImage = false;

int flashlight = 0;

using namespace websockets;
WebsocketsClient client;

String receiveServerPort()
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

bool sendPortToServer(String serverIP, int uniquePort, int replyPort)
{
  // Now send a POST request to the server
  if (WiFi.status() == WL_CONNECTED)
  {
    /*
    curl -X 'POST' \
  'http://192.168.2.192:8000/setIPPort' \
  -H 'accept: application/json' \
  -H 'Content-Type: application/json' \
  -d '{"ip": "192.168.1.1", "port":8001}'
  */
    HTTPClient http;
    http.begin("http://" + serverIP + ":" + String(replyPort) + "/setIPPort"); // Specify the FastAPI server URL
    http.addHeader("Content-Type", "application/json");
    String httpRequestData = "{\"ip\":\"" + WiFi.localIP().toString() + "\",\"port\":" + String(uniquePort) + "}";
    Serial.println(httpRequestData);
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
      return true;
    }
    else
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
      return false;
    }
    http.end();
  }
  return false;
}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connection Opened");
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connection Closed");
    ESP.restart();
  }
}

void onMessageCallback(WebsocketsMessage message)
{
  String data = message.data();
  int index = data.indexOf("=");
  if (index != -1)
  {
    String key = data.substring(0, index);
    String value = data.substring(index + 1);

    if (key == "ON_BOARD_LED")
    {
      if (value.toInt() == 1)
      {
        flashlight = 1;
        digitalWrite(FLASH_PIN, HIGH);
      }
      else
      {
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

uint32_t createUniqueID()
{
  uint64_t mac = ESP.getEfuseMac();  // Get MAC address
  uint32_t upper = mac >> 32;        // Get upper 16 bits
  uint32_t lower = mac & 0xFFFFFFFF; // Get lower 32 bits
  uint32_t uid = upper ^ lower;      // XOR upper and lower parts to get a 32-bit result
  return uid % 1000;
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

  // connect to the Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  // compute the unique PORT
  uint16_t uniqueID = createUniqueID();
  uint16_t uniquePort = 8000 + uniqueID;

  // receive the server IP address
  websocket_server_host = receiveServerPort();
  Serial.println("Server IP: " + String(websocket_server_host));
  // init the camera
  initCamera();

  // Start Arduino OTA
  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA.begin(); // Port defaults to 3232

  // setup server to control the microscope
  initServer();

  // reply with the unique camera port/ID
  sendPortToServer(websocket_server_host, uniquePort, replyPort);

  // register callbacks and start the websocket client for frame transfer
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  websocket_server_port = uniquePort;
  int nConnectionTrial=0;
  while (!client.connect(websocket_server_host, websocket_server_port, "/"))
  {
    nConnectionTrial++;
    delay(500);
    if (nConnectionTrial>10)
      ESP.restart();
  }
}

int t0 = 0;

float computeMeanFrame(uint8_t *data, int len)
{
  float mean = 0;
  for (int i = 0; i < len; i++)
  {
    mean += data[i];
  }
  return mean / len;
}

int frameID = 0;
void loop()
{
  ArduinoOTA.handle();
  
  // Serial.println("Framerate : " + String(1000 / (1+ millis() - t0)) + " fps");
  t0 = millis();
  client.poll();
  if (isSendImage)
  {
    return;
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    esp_camera_fb_return(fb);
    // Serial.println("Camera Capture Failed");
    return;
  }
  else{
    Serial.printf("Frame ID %i", frameID);
    Serial.println("");
  }
  // Serial.println("Frame Mean: " + String(computeMeanFrame(fb->buf, fb->len)));

  client.sendBinary((const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  //  client.send("TEST");
  frameID ++;
}

void initServer()
{

  // Setup Web server
  log_d("Staring Server");
  // Modify /setServer to handle HTTP_GET
  server.on("/setServer", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String message;

    // Check if query parameter server exists
    if (request->hasParam("server")) {
      String serverIP = request->getParam("server")->value();
      log_d("Server IP: %s", serverIP.c_str());

      preferences.begin("network", false);
      preferences.putString("wshost", serverIP);
      preferences.end();
      message = "{\"success\":\"server IP updated\"}";
      request->send(200, "application/json", message);

      // Restart the ESP
      ESP.restart();

    } else {
      message = "{\"error\":\"No server IP provided\"}";
    }

    request->send(200, "application/json", message); });

  server.on("/setUniqueID", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String message;
    Serial.println("setUniqueID");

    // Check if query parameter uid exists
    if (request->hasParam("uid")) {
      String uidStr = request->getParam("uid")->value(); // Get the uid as a String
      int uid = uidStr.toInt();                         // Convert the String to an integer
      Serial.println(uid);

      preferences.begin("network", false);
      preferences.putInt("uid", uid);
      preferences.end();

      message = "{\"success\":\"Unique ID updated\"}";
      request->send(200, "application/json", message);

      // Restart the ESP
      ESP.restart();

    } else {
      message = "{\"error\":\"No Unique ID provided\"}";
      request->send(200, "application/json", message);
    } });
  server.on("restart", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String message = "{\"id\":\"" + String(1) + "\"}";
    request->send(200, "application/json", message);
    ESP.restart(); });
  server.on("/getId", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    uint32_t uniqueId = createUniqueID();

    // Adding compile date and time
    String compileDate = __DATE__; // Compiler's date
    String compileTime = __TIME__; // Compiler's time

    String message = "{\"id\":\"" + String(uniqueId) + "\", \"compileDate\":\"" + compileDate + "\", \"compileTime\":\"" + compileTime + "\"}";
    request->send(200, "application/json", message); });
  server.on("/getId", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    uint32_t uniqueId = createUniqueID();
    String message = "{\"id\":\"" + String(uniqueId) + "\"}";
    request->send(200, "application/json", message); });
  // Capture Image Handler
  server.on("/getImage", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              isSendImage = true;

              camera_config_t config;

              /*
                 FRAMESIZE_UXGA (1600 x 1200)
                FRAMESIZE_QVGA (320 x 240)
                FRAMESIZE_CIF (352 x 288)
                FRAMESIZE_VGA (640 x 480)
                FRAMESIZE_SVGA (800 x 600)
                FRAMESIZE_XGA (1024 x 768)
                FRAMESIZE_SXGA (1280 x 1024)
              */
              log_d("Setting frame size to UXGA");
              sensor_t *s = esp_camera_sensor_get();
              s->set_framesize(s, FRAMESIZE_SVGA); // FRAMESIZE_QVGA);
              // digest the settings => warmup camera
              camera_fb_t *fb = NULL;
              for (int iDummyFrame = 0; iDummyFrame < 5; iDummyFrame++)
              {
                fb = esp_camera_fb_get();
                if (!fb){
                  log_e("Camera frame error", false);
                   request->send(500, "text/plain", "Camera capture failed");
                return;
              
                }
                  
                esp_camera_fb_return(fb);
              }

              fb = esp_camera_fb_get();
              if (!fb)
              {
                log_e("Camera capture failed");
                request->send(500, "text/plain", "Camera capture failed");
                return;
              }
              log_d("Camera capture OK, sending out image via HTTP");
              AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", fb->buf, fb->len);
              response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
              request->send(response);

              esp_camera_fb_return(fb);

              log_d("Setting frame size to QVGA");
              // revert to default settings
              s->set_framesize(s, FRAMESIZE_QVGA); // FRAMESIZE_QVGA);
              // digest the settings => warmup camera
              for (int iDummyFrame = 0; iDummyFrame < 2; iDummyFrame++)
              {
                fb = esp_camera_fb_get();
                if (!fb)
                  log_e("Camera frame error", false);
                esp_camera_fb_return(fb);
              }
              isSendImage = false; });
  server.on("/resetESP", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String message = "{\"id\":\"" + String(1) + "\"}";
    request->send(200, "application/json", message);
    preferences.begin("network", false);
    preferences.clear();
    preferences.end();
    ESP.restart(); });
  server.begin();
}
