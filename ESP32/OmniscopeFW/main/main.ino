// https://github.com/hideakitai/ESP32DMASPI/blob/main/examples/transfer_big_data_in_the_background/transfer_big_data_in_the_background_slave/transfer_big_data_in_the_background_slave.ino
#include <ESP32DMASPISlave.h>
#include "helper.h"
#define PIN_NUM_MOSI D10
#define PIN_NUM_MISO D9
#define PIN_NUM_CLK D8
#define PIN_NUM_CS 21
#include "esp_camera.h"
#include "camera_pins.h"
//  slave.begin(HSPI, PIN_NUM_CLK, PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CS);  // alternative pins (CS, CLK, MISO, MOSI

#include "helper.h"

ESP32DMASPI::Slave slave;

static constexpr size_t BUFFER_SIZE = 256; // should be multiple of 4
static constexpr size_t QUEUE_SIZE = 1;
uint8_t *dma_tx_buf;
uint8_t *dma_rx_buf;

void setup()
{
    Serial.begin(115200);
    initCamera();
    delay(2000);

    // to use DMA buffer, use these methods to allocate buffer
    dma_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
    dma_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);

    slave.setDataMode(SPI_MODE0);          // default: SPI_MODE0
    slave.setMaxTransferSize(BUFFER_SIZE); // default: 4092 bytes
    slave.setQueueSize(QUEUE_SIZE);        // default: 1

    // begin() after setting
    slave.begin(HSPI, PIN_NUM_CLK, PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CS); // alternative pins (CS, CLK, MISO, MOSI

    Serial.println("start spi slave");
}

void loop()
{

    // get image from camera
    camera_fb_t *fb = esp_camera_fb_get();
    size_t original_size = fb->len;
    Serial.println("Original frame size: " + String(original_size));
    Serial.println(esp_get_free_heap_size());
    
    // Calculate the size of the new buffer
    size_t padded_size = (original_size + 3) & ~3; // Round up to multiple of 4

    // Allocate a new buffer
    uint8_t *padded_data = new uint8_t[padded_size];

    // Copy the original data
    memcpy(padded_data, fb->buf, original_size);
    esp_camera_fb_return(fb);

    // Pad the rest with zeros
    if (padded_size > original_size)
    {
        memset(padded_data + original_size, 0, padded_size - original_size);
    }

    Serial.println("Padded frame size: " + String(padded_size));
    Serial.println(esp_get_free_heap_size());
    // send image to master -> MUST BE MUTIPLE OF 4!!!!
    // initialize tx/rx buffers
    if (slave.hasTransactionsCompletedAndAllResultsHandled())
    {
        // initialize tx/rx buffers
        Serial.println("initialize tx/rx buffers");
        initializeBuffers(dma_tx_buf, dma_rx_buf, padded_size, 0);

        // queue transaction and trigger it right now
        Serial.println("execute transaction in the background");
        Serial.println(esp_get_free_heap_size());
        slave.queue(padded_data, dma_rx_buf, padded_size);
        slave.trigger();

        Serial.println("wait for the completion of the queued transactions...");
        Serial.println(esp_get_free_heap_size());
    }

    // if all transactions are completed and all results are ready, handle results
    if (slave.hasTransactionsCompletedAndAllResultsReady(padded_size))
    {
        // process received data from slave
        Serial.println("all queued transactions completed. start verifying received data from slave");

        // get the oldeest transfer result
        size_t received_bytes = slave.numBytesReceived();

        Serial.println("received_bytes: " + String(received_bytes));
        // verify and dump difference with received data
        // NOTE: we need only 1st results (received_bytes[0])
        if (verifyAndDumpDifference("slave", dma_tx_buf, BUFFER_SIZE, "master", dma_rx_buf, received_bytes))
        {
            Serial.println("successfully received expected data from master");
        }
        else
        {
            Serial.println("Unexpected difference found between master/slave data");
        }
    }
    delay(200);
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
