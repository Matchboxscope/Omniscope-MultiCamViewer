#include <Wire.h>

#define SLAVE_ADDR 0x28 // Address of the I2C slave devices
#define MAX_CHUNK_SIZE 32 // Maximum number of bytes to request in one chunk

// Replace these with the GPIO numbers you want to use
const int SDA_PIN = 21; // GPIO number for I2C data line (SDA)
const int SCL_PIN = 22; // GPIO number for I2C clock line (SCL)

void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud
  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C communication using specific pins
  // Scan all IP aduint8_tdresses 
  Scanner();
}

void loop() {
  // Request frame from camera with ID 1
  
  requestFrame(1);
  delay(1000); // Wait a bit before requesting the next frame
  
  }

void requestFrame(uint8_t cameraID) {
  Serial.print("Requesting frame from camera ID ");
  Serial.println(cameraID);

  Wire.beginTransmission(SLAVE_ADDR); // Begin I2C transmission to the slave device
  Wire.write(cameraID); // Send the camera ID as the request
  Wire.endTransmission(); // End transmission

  // parse frame size
  uint8_t frame_size = 0;
  Wire.requestFrom(SLAVE_ADDR, sizeof(frame_size));
  if (Wire.available() == sizeof(frame_size)) {
    uint8_t* frame_size_ptr = (uint8_t*)&frame_size;
    for (size_t i = 0; i < sizeof(frame_size); ++i) {
      frame_size_ptr[i] = Wire.read();
      Serial.println(frame_size_ptr[i]);
    }
    Serial.print("Frame size: ");
    Serial.println(frame_size);
  } else {
    Serial.println("Failed to read frame size.");
  }
  

/*
  // read the data 
  uint32_t bytesRead = 0;
  while (bytesRead < frame_size) {
    uint8_t bytesToRequest = MAX_CHUNK_SIZE;
    if (frame_size - bytesRead < MAX_CHUNK_SIZE) {
      bytesToRequest = frame_size - bytesRead;
    }

    Wire.requestFrom(SLAVE_ADDR, bytesToRequest);
    while (Wire.available()) {
      char c = Wire.read();
      Serial.print(c); // Or process/store the received byte
      bytesRead++;
    }
  }
  */
  /*
  // Begin to read a response (for demonstration, let's assume a fixed size of data, e.g., 32 bytes)
  Wire.requestFrom(SLAVE_ADDR, 32); // Request 32 bytes from the slave
  while (Wire.available()) { // Slave may send less than requested
    char c = Wire.read(); // Receive a byte as character
    Serial.print(c); // Print the byte
  }
  Serial.println(); // New line after printing all data
*/
}



void Scanner()
{
  Serial.println ();
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;

  Wire.begin();
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i);          // Begin I2C transmission Address (i)
    if (Wire.endTransmission () == 0)  // Receive 0 = success (ACK response) 
    {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);     // PCF8574 7 bit address
      Serial.println (")");
      count++;
    }
  }
  Serial.print ("Found ");      
  Serial.print (count, DEC);        // numbers of devices
  Serial.println (" device(s).");
}
