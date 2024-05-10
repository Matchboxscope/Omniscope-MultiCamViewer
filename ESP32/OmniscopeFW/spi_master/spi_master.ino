#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"s
#include "freertos/task.h"
#include <SPI.h>

#define PIN_NUM_MISO  19
#define PIN_NUM_MOSI   23
#define PIN_NUM_CLK   18
#define PIN_NUM_CS     5


// Image dimensions and size
#define IMG_WIDTH 320
#define IMG_HEIGHT 240
#define IMG_SIZE IMG_WIDTH * IMG_HEIGHT // Grayscale, 1 byte per pixel

// Command to request a frame from the slave
#define CMD_REQUEST_FRAME 0x02

// SPI transaction settings
SPISettings spiSettings(8000000, MSBFIRST, SPI_MODE0);

void setup() {
  Serial.begin(115200);

  // Configure SPI
  pinMode(SPI_SS, OUTPUT); // SS must be output for SPI master to work
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS);
  SPI.setFrequency(8000000); // Adjust as necessary

  Serial.println("SPI Master Initialized");
}

void loop() {
  delay(1000); // Delay between requests

  // Buffer to store the image data
  uint8_t* frameBuffer = (uint8_t*)malloc(IMG_SIZE);
  if (frameBuffer == nullptr) {
    Serial.println("Failed to allocate memory for frame buffer.");
    return;
  }

  // Request a frame from the slave
  SPI.beginTransaction(spiSettings);
  digitalWrite(SPI_SS, LOW);
  SPI.transfer(CMD_REQUEST_FRAME);
  digitalWrite(SPI_SS, HIGH);
  SPI.endTransaction();

  // Short delay to allow the slave to prepare the data
  delay(50);

  // Read the frame data
  SPI.beginTransaction(spiSettings);
  digitalWrite(SPI_SS, LOW);

  // Depending on the SPI slave implementation, you might need to read in chunks
  // Here, we read the whole frame at once for simplicity
  for (int i = 0; i < IMG_SIZE; i++) {
    frameBuffer[i] = SPI.transfer(0x00); // Send dummy data to clock in the frame data
  }

  digitalWrite(SPI_SS, HIGH);
  SPI.endTransaction();

  // Process the received frame...
  // For demonstration, just print the first few bytes
  Serial.println("Frame received:");
  for (int i = 0; i < 10; i++) {
    Serial.print(frameBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println("...");

  free(frameBuffer); // Free the buffer after processing
}


/*
void setup() {

spi_bus_config_t buscfg = {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096,
};


  // SPI device interface configuration for the master to communicate with the slave
  spi_device_interface_config_t devcfg;
  devcfg.clock_speed_hz = 10 * 1000 * 1000;       // Clock out at 10 MHz
  devcfg.mode = 0;                                // SPI mode 0
  devcfg.spics_io_num = PIN_NUM_CS;               // CS pin
  devcfg.queue_size = 7;                          // Queue up to 7 transactions at a time


  // Initialize the SPI bus  
  esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, ESP_INTR_FLAG_LEVEL1);
  ESP_ERROR_CHECK(ret);

  // Attach the slave device to the SPI bus
  spi_device_handle_t handle;
  ret = spi_bus_add_device(VSPI_HOST, &devcfg, &handle);
  ESP_ERROR_CHECK(ret);

  // This loop continuously receives data from the SPI slave
  while (1) {
    spi_transaction_t transaction = { 0 };
    uint8_t rx_buffer[128] = { 0 }; // Adjust the size as needed
    transaction.length = 8 * sizeof(rx_buffer); // Transaction length in bits
    transaction.rx_buffer = rx_buffer;

    // Queue the transaction
    ret = spi_device_transmit(handle, &transaction); // Transmit!
    ESP_ERROR_CHECK(ret);

    // Process the received data (contained in rx_buffer)
    // For example, just log the received data
    ESP_LOGI("SPI Master", "Received data:");
    for (int i = 0; i < sizeof(rx_buffer); i++) {
      printf("%02X ", rx_buffer[i]);
      if ((i + 1) % 16 == 0) {
        printf("\n");
      }
    }
    printf("\n");

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for a bit before the next transaction
  }
}

void loop() {}
*/
