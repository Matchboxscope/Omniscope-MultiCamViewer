#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_camera.h"
#include <esp_err.h>
#include <driver/spi_slave.h>
#include <nvs_flash.h>
#include <driver/spi_slave.h>
#include "camera_pins.h"

#define PIN_NUM_MOSI D10
#define PIN_NUM_MISO D9
#define PIN_NUM_CLK D8
#define PIN_NUM_CS 21
#define CAM_WIDTH 320
#define CAM_HEIGHT 240
#define GPIO_HANDSHAKE 22

// SPI transaction states
#define STATE_WAITING_FOR_REQUEST         2u
#define STATE_SENDING_RESPONSE            3u

// Requests
#define REQ_CAMERA_PIC              0x02u

// Function prototypes
static esp_err_t init_camera();
void spi_task(void *pvParameters);

// SPI Callbacks
void my_post_setup_cb(spi_slave_transaction_t *trans) {
    WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (1<<GPIO_HANDSHAKE));
}

void my_post_trans_cb(spi_slave_transaction_t *trans) {
    WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1<<GPIO_HANDSHAKE));
}


// Main application entry point
void setup() {
    // wait for the serial connection to be established
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(init_camera());

    ESP_LOGI("Starting Code", "Camera Init Succeeded");
    xTaskCreate(spi_task, "spi_task", 4096, NULL, 5, NULL);
}

void loop(){
}

static esp_err_t init_camera() {
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
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.jpeg_quality = 10;
    
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.frame_size = FRAMESIZE_VGA; // iFRAMESIZE_UXGA;


  /*
     FRAMESIZE_UXGA (1600 x 1200)
    FRAMESIZE_QVGA (320 x 240)
    FRAMESIZE_CIF (352 x 288)
    FRAMESIZE_VGA (640 x 480)
    FRAMESIZE_SVGA (800 x 600)
    FRAMESIZE_XGA (1024 x 768)
    FRAMESIZE_SXGA (1280 x 1024)
  */
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE("camera", "Camera Init Failed");
        return err;
    }
    ESP_LOGI("camera", "Camera Init Succeeded");
    return ESP_OK;
}

void spi_task(void *pvParameters) {
    esp_err_t ret;

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CAM_WIDTH*CAM_HEIGHT
    };

    // Configure SPI slave interface
    spi_slave_interface_config_t slvcfg;
    slvcfg.mode = 0;
    slvcfg.spics_io_num = PIN_NUM_CS;
    slvcfg.queue_size = 3;
    slvcfg.flags = 0;
    slvcfg.post_setup_cb = NULL;
    slvcfg.post_trans_cb = NULL;
    
    // Handshake line configuration
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1<<GPIO_HANDSHAKE);
    gpio_config(&io_conf);

    // Initialize SPI slave
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    uint8_t *camera_frame = (uint8_t *)heap_caps_malloc(CAM_WIDTH*CAM_HEIGHT, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
    //uint8_t *camera_frame = (uint8_t *)heap_caps_malloc(CAM_WIDTH*CAM_HEIGHT, MALLOC_CAP_DMA);
    spi_slave_transaction_t t = { 0 };
    // Allocate recvbuf dynamically if it needs to be DMA-capable
    uint8_t *recvbuf = (uint8_t*)heap_caps_malloc(sizeof(uint8_t), MALLOC_CAP_DMA | MALLOC_CAP_32BIT);

    ESP_LOGI("Starting ", "SPI starts");
    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        t.length = 8; // 8 bits
        t.tx_buffer = NULL;
        t.rx_buffer = recvbuf;
        ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE("spi", "Failed to receive request");
            continue;
        }
        else{
            ESP_LOGI("spi", "Received request: %d", recvbuf[0]);
        }
        if (ret == ESP_OK && recvbuf[0] == REQ_CAMERA_PIC) {
            // Capture the frame
            camera_fb_t *pic = esp_camera_fb_get();
            if (!pic) {
                ESP_LOGE("camera", "Camera Capture Failed");
                continue;
            }
            ESP_LOGI("Frame ", "SPI status: ");
            // Prepare for sending the frame over SPI
            memcpy(camera_frame, pic->buf, pic->len);
            esp_camera_fb_return(pic);

            // Send the frame over SPI
            t.length = 8 * CAM_WIDTH*CAM_HEIGHT; // bit length
            t.tx_buffer = camera_frame;
            t.rx_buffer = NULL;
            ret = spi_slave_transmit(SPI1_HOST, &t, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE("spi", "Failed to transmit frame");
            }
        }
    }
}
