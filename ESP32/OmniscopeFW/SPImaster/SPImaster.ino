// Master as a receiver for SPI communication

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "driver/spi_master.h"
#include <SPI.h>
#include "esp_log.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "esp_intr_alloc.h"

// Pins in use
#define GPIO_MOSI 23
#define GPIO_MISO 19
#define GPIO_SCLK 18
#define GPIO_CS 5


// Main application
void setup()
{
  esp_err_t ret;
  int n = 0;
  spi_device_handle_t handle;

  // Configuration for the SPI bus
  spi_bus_config_t buscfg = {
    .mosi_io_num = GPIO_MOSI,
    .miso_io_num = GPIO_MISO,
    .sclk_io_num = GPIO_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
  };

  // Configuration for the SPI device on the other side of the bus
  spi_device_interface_config_t devcfg;
  devcfg.command_bits = 0;
  devcfg.address_bits = 0;
  devcfg.dummy_bits = 0;
  devcfg.clock_speed_hz = 1000000;
  devcfg.duty_cycle_pos = 128; // 50% duty cycle
  devcfg.mode = 0;
  devcfg.spics_io_num = GPIO_CS;
  devcfg.cs_ena_posttrans = 3; // Keep the CS low 3 cycles after transaction
  devcfg.cs_ena_pretrans = 0,
         devcfg.queue_size = 3;


  ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  assert(ret == ESP_OK);

  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &handle);
  assert(ret == ESP_OK);

  WORD_ALIGNED_ATTR char sendbuf[128];
  WORD_ALIGNED_ATTR char recvbuf[128];

  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 64 * 8;
  t.tx_buffer = sendbuf;
  t.rx_buffer = recvbuf;

  while (1) {
    memset(sendbuf, '\0', 128);
    memset(recvbuf, '\0', 128);
    sprintf(sendbuf, "Transaction no. %d", n);

    ESP_LOGI("SPI_MASTER", "Transaction no. %d", n);
    ret = spi_device_transmit(handle, &t);
    ESP_LOGI("SPI_MASTER", "Received: %s\n", recvbuf);
    ESP_LOG_BUFFER_HEX("SPI_MASTER", recvbuf, 64);

    n++;

    vTaskDelay(1000 / portTICK_RATE_MS);
  }

  ret = spi_bus_remove_device(handle);
  assert(ret == ESP_OK);
}

void loop() {}
