/*
  ESP-NOW Remote Sensor - Receiver (Multiple Version)
  esp-now-rcv.ino
  Receives Temperature & Humidity data from other ESP32 via ESP-NOW

  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include required libraries
#include <WiFi.h>
#include <esp_now.h>

// Define data structure
typedef struct struct_message
{
    uint8_t photoBytes[160]; // ESP Now supports max of 250 bytes at each packet
} struct_message;

struct_message incomingReadings;
struct_message incomingData;

uint8_t r_color = 50;
uint8_t g_color = 50;
uint8_t b_color = 50;
unsigned int pixelCounter = 0;
int rowCounter = 0;
unsigned int zeroCounter = 0;

// Callback function
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.println("!");

    pixelCounter = 0;
    if (zeroCounter > 100)
    {
        zeroCounter = 0;
        rowCounter = 0;
        pixelCounter = 0;
    }

    for (uint8_t i = 0; i < 160; i++)
    {
        if (incomingReadings.photoBytes[i] == 0)
        {
            zeroCounter++;
        }

        if (rowCounter >= 159)
        {
            rowCounter = 0;
            pixelCounter = 0;
        }
        uint16_t RGB565 = (((incomingReadings.photoBytes[i] & 0b11111000) << 8) + ((incomingReadings.photoBytes[i] & 0b11111100) << 3) + (incomingReadings.photoBytes[i] >> 3));
        Serial.println(RGB565);
        pixelCounter += 1;
    }
    rowCounter++;
    Serial.println("");
    Serial.println("?");

}


void setup()
{
    // Set up Serial Monitor
    Serial.begin(115200);

    // Start ESP32 in Station mode
    WiFi.mode(WIFI_STA);

    delay(3000);
    // Print MAC Address to Serial monitor
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Initalize ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register callback function
    esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}