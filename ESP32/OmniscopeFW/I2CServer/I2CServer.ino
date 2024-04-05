#include <Wire.h>

#define SLAVE_ADDR 0x28 // Address of the I2C slave devices
#define MAX_CHUNK_SIZE 256 // Maximum number of bytes to request in one chunk

size_t frame_size = 0; // Declare frame_size as a global variable


// Replace these with the GPIO numbers you want to use
const int SDA_PIN = 21; // GPIO number for I2C data line (SDA)
const int SCL_PIN = 22; // GPIO number for I2C clock line (SCL)

void setup() {
  Serial.begin(50000); // Start serial communication at 115200 baud
  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C communication using specific pins
  Wire.setClock(800000); 
  // Scan all IP aduint8_tdresses
  Scanner();
}

int iiter = 0;

void loop() {
  // Request frame from camera with ID 1

  // Step 1: Request and read the frame size
  frame_size = requestFrameSize(iiter++);
  if (frame_size == 0) {
    Serial.println("Failed to read frame size or frame size is 0.");
    delay(1000);  // Wait a bit before retrying
    return;
  }

  Serial.print("Frame size: ");
  Serial.println(frame_size);

  
  // Step 2: Request and read the frame data in chunks
  uint8_t* frame_data = new uint8_t[frame_size];
  if (frame_data == nullptr) {
    Serial.println("Failed to allocate memory for frame data.");
    return;
  }

  Serial.println("Reading Frame....");
  bool success = requestFrameData(frame_data, frame_size);
  if (success) {
    Serial.println("Frame data received successfully.");
    // Process frame_data here
  } else {
    Serial.println("Failed to receive complete frame data.");
  }

//  delete[] frame_data;  // Clean up allocated memory


  delay(1000); // Wait a bit before requesting the next frame

}

size_t requestFrameSize(int cameraID) {
  
  Wire.beginTransmission(SLAVE_ADDR); // Begin I2C transmission to the slave device
  Wire.write(cameraID); // Send the camera ID as the request
  Wire.endTransmission(); // End transmission

  
  Wire.requestFrom(SLAVE_ADDR, 2);  // Request 2 bytes for the frame size
  if (Wire.available() == 2) {
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();
    return word(highByte, lowByte);
  }
  return 0;  // Return 0 if failed to read frame size
}

bool requestFrameData(uint8_t* frame_data, size_t frame_size) {
  Serial.println("Requesting Data");
  size_t bytes_received = 0;
  
  while (bytes_received < frame_size) {
    size_t chunk_size = min(static_cast<size_t>(MAX_CHUNK_SIZE), frame_size);
    Wire.requestFrom(SLAVE_ADDR, chunk_size);
    //Serial.print("+++");
    while (Wire.available()) {
      frame_data[bytes_received++] = Wire.read();
      // Print bytes received for debugging
      //Serial.print(frame_data[bytes_received - 1], HEX);
    }    
    //Serial.print("---");
  }
  // print the first 10 bytes of the frame
  for (int i = 0; i < 10; i++) {
    Serial.print(frame_data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println(bytes_received);
  return bytes_received == frame_size;  // Return true if all data was received
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
