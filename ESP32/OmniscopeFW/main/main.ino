
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
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#define GPIO_MOSI 17
#define GPIO_MISO 16
#define GPIO_SCLK 15
#define GPIO_CS 7

void setup()
{
    int n = 0;
    esp_err_t ret;

    spi_bus_config_t buscfg;
    buscfg.mosi_io_num = GPIO_MOSI;
    buscfg.miso_io_num = GPIO_MISO;
    buscfg.sclk_io_num = GPIO_SCLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    spi_slave_interface_config_t slvcfg;
    slvcfg.mode = 0,
    slvcfg.spics_io_num = GPIO_CS;
    slvcfg.queue_size = 3;
    slvcfg.flags = 0;
    /*
    gpio_set_pull_mode((gpio_num_t) GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t) GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t) GPIO_CS, GPIO_PULLUP_ONLY);
    */
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    //assert(ret == ESP_OK);

    WORD_ALIGNED_ATTR char sendbuf[128];
    WORD_ALIGNED_ATTR char recvbuf[128];

    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 64 * 8;
    t.tx_buffer = sendbuf;
    t.rx_buffer = recvbuf;

    while (1)
    {
        memset(recvbuf, '\0', 128);
        memset(sendbuf, '\0', 128);
        sprintf(sendbuf, "Transaction no. %d", n);

        ESP_LOGI("SPI_SLAVE", "Transaction no. %d. Waiting for spi master", n);
        ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
        ESP_LOGI("SPI_SLAVE", "Received: %s\n", recvbuf);
        ESP_LOG_BUFFER_HEX("SPI_SLAVE", recvbuf, 64);
        // delay for 1 second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        n++;
    }
}

void loop(){}