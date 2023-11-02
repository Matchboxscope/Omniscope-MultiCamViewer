#include <WiFi.h>
#include <WiFiUdp.h>
#include "HTTPClient.h"

const char* ssid = "Blynk";     // Replace with your WiFi SSID
const char* password = "12345678"; // Replace with your WiFi password

unsigned int localPort = 12345;    // local port to listen on
unsigned int replyPort = 4444;
WiFiUDP udp;


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  udp.begin(localPort);
  Serial.print("Now listening at IP ");
  Serial.print(WiFi.localIP());
  Serial.print(", UDP port ");
  Serial.println(localPort);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Read the packet into packetBuffer
    char packetBuffer[255];
    udp.read(packetBuffer, 255);
    String serverIp = String(packetBuffer).substring(0, packetSize);
    Serial.print("Server IP Address: ");
    Serial.println(serverIp);

    // Now send a POST request to the server Now listeni
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("http://" + serverIp + ":"+String(replyPort)+"/setIP"); // Specify the FastAPI server URL
      http.addHeader("Content-Type", "application/json");
      String httpRequestData = "{\"ip\":\"" + WiFi.localIP().toString() + "\"}";
      
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      }
      else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
  }
}
