#include "Arduino.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "AccelStepper.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Neopixel
#define NUMPIXELS 128
#define NEOPIXEL_PIN GPIO_NUM_5

// Focusmotor connected to the WEMOS / CNC Shield v3  -  Stepper X
#define STEPPER_MOTOR_SPEED 20000
#define STEPPER_MOTOR_DIR GPIO_NUM_4
#define STEPPER_MOTOR_STEP GPIO_NUM_2
#define STEPPER_MOTOR_ENABLE GPIO_NUM_15

// Illumination related
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

const char *mSSID = "omniscope";
const char *mPW = "omniscope"; // Change this to your WiFi password

// Stepper-related
AccelStepper motor(1, STEPPER_MOTOR_STEP, STEPPER_MOTOR_DIR);

// default values; will be updated
String websocket_server_host = "192.168.0.116";
uint16_t websocket_server_port = 8004;

// Broadcasting the server IP
unsigned int broadcastingPort = 12345; // local port to listen on
unsigned int replyPort = 8000;         // local port to listen on

using namespace websockets;
WebsocketsClient client;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up");


  // in setup()
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // illuimation-related
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(100);
  setNeopixel(100);
  delay(100);
  setNeopixel(0);

  // motor-related
  pinMode(STEPPER_MOTOR_DIR, OUTPUT);
  pinMode(STEPPER_MOTOR_ENABLE, OUTPUT);
  setMotorActive(true);
  motor.setMaxSpeed(STEPPER_MOTOR_SPEED);
  motor.setAcceleration(10000);
  setMotorActive(false);
  motor.setCurrentPosition(0);
  if(0){
  moveFocusRelative(1000, true);
  moveFocusRelative(-1000, true);
}
//  scanWifi();
  Serial.println("Connecting to: ");
  Serial.println(mSSID);
  WiFi.begin(mSSID, mPW);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(500);
  }
  uint16_t uniqueID = -1;
  uint16_t uniquePort = 8000 + uniqueID;

  // receive the server IP address
  websocket_server_host = receiveServerPort();
  Serial.println("Server IP: " + String(websocket_server_host));

  // reply with the unique camera port/ID
  sendPortToServer(websocket_server_host, uniquePort, replyPort);

  // register callbacks and start the websocket client for frame transfer
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  websocket_server_port = uniquePort;
  int nConnectionTrial = 0;
  while (!client.connect(websocket_server_host, websocket_server_port, "/"))
  {
    nConnectionTrial++;
    delay(500);
    if (nConnectionTrial > 10)
      ESP.restart();
  }

  setMotorActive(true);
}
int t0 = 0;
int frameID = 0;
void loop()
{

  client.poll();
}

WiFiUDP udp;

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

    //  curl -X 'POST' \
    //  'http://192.168.2.192:8000/setIPPort' \
    //  -H 'accept: application/json' \
    //  -H 'Content-Type: application/json' \
    //  -d '{"ip": "192.168.1.1", "port":8001}'

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

    if (key == "MOVE_FOCUS")
    {
      int focusVal = value.toInt();
      moveFocusRelative(focusVal, false);
    }
    else if (key == "ILLUMINATION")
    {
      int illuminationVal = value.toInt();
      setNeopixel(illuminationVal);
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

float computeMeanFrame(uint8_t *data, int len)
{
  float mean = 0;
  for (int i = 0; i < len; i++)
  {
    mean += data[i];
  }
  return mean / len;
}

// Neopixel Control
void setNeopixel(int newVal)
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(newVal, newVal, newVal));
  }
  log_d("Setting Neopixel to %d", newVal);
  pixels.show(); // Send the updated pixel colors to the hardware.
}

void setMotorActive(bool isActive)
{
  // low means active
  digitalWrite(STEPPER_MOTOR_ENABLE, !isActive);
}

// move focus using accelstepper
void moveFocusRelative(int steps, bool handleEnable)
{
  log_d("Moving focus %d steps, currentposition %d", steps, motor.currentPosition());
  // a very bad idea probably, but otherwise we may have concurancy with the loop function
  if (handleEnable)
    setMotorActive(true);
  // log_i("Moving focus %d steps, currentposition %d", motor.currentPosition() + steps, motor.currentPosition());

  // run motor to new position with relative movement
  motor.setSpeed(STEPPER_MOTOR_SPEED);
  motor.runToNewPosition(motor.currentPosition() + steps);
  if (handleEnable)
    setMotorActive(false);
}

int getCurrentMotorPos()
{
  return motor.currentPosition();
}

void setSpeed(int speed)
{
  motor.setSpeed(speed);
}

void scanWifi()
{
  // Scan for Wi-Fi networks
  int networkCount = WiFi.scanNetworks();

  if (networkCount == 0)
  {
    Serial.println("No Wi-Fi networks found");
  }
  else
  {
    Serial.print("Found ");
    Serial.print(networkCount);
    Serial.println(" Wi-Fi networks:");

    for (int i = 0; i < networkCount; ++i)
    {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      int encryptionType = WiFi.encryptionType(i);

      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(ssid);
      Serial.print(" Signal strength: ");
      Serial.print(rssi);
      Serial.println(" dBm");
    }
  }
}
