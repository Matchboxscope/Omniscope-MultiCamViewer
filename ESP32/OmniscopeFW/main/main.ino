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

// ESP32Cam (AiThinker) PIN Map
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define GPIO_HANDSHAKE 2
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14

#define CAM_WIDTH   320
#define CAM_HEIGHT  240

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

// Camera configuration
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_GRAYSCALE,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1
};

// Main application entry point
void setup() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(init_camera());

    xTaskCreate(spi_task, "spi_task", 4096, NULL, 5, NULL);
}

void loop(){
}

static esp_err_t init_camera() {
    esp_err_t err = esp_camera_init(&camera_config);
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
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CAM_WIDTH*CAM_HEIGHT
    };

    // Configure SPI slave interface
    spi_slave_interface_config_t slvcfg;
    slvcfg.mode = 1;
    slvcfg.spics_io_num = GPIO_CS;
    slvcfg.queue_size = 3;
    slvcfg.flags = 0;
    slvcfg.post_setup_cb = my_post_setup_cb;
    slvcfg.post_trans_cb = my_post_trans_cb;
    
    // Handshake line configuration
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1<<GPIO_HANDSHAKE);
    gpio_config(&io_conf);

    // Initialize SPI slave
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, 1);
    assert(ret == ESP_OK);

    uint8_t *camera_frame = (uint8_t *)heap_caps_malloc(CAM_WIDTH*CAM_HEIGHT, MALLOC_CAP_DMA);
    spi_slave_transaction_t t = { 0 };
    uint8_t recvbuf[1]; // Buffer to receive request

    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        t.length = 8; // 8 bits
        t.tx_buffer = NULL;
        t.rx_buffer = recvbuf;
        ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);

        if (ret == ESP_OK && recvbuf[0] == REQ_CAMERA_PIC) {
            // Capture the frame
            camera_fb_t *pic = esp_camera_fb_get();
            if (!pic) {
                ESP_LOGE("camera", "Camera Capture Failed");
                continue;
            }

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
