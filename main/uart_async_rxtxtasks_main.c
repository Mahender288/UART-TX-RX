/*
 * UART Asynchronous Example for ESP32 DevKit V1
 * Uses separate RX and TX tasks and UART2 (GPIO25 TX, GPIO26 RX)
 *
 * Connect TX (GPIO25) to RX (GPIO26) using a jumper wire.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#define UART_PORT_NUM   UART_NUM_2
#define TXD_PIN         25
#define RXD_PIN         26
#define UART_BAUD_RATE  115200
#define RX_BUF_SIZE     1024
#define TASK_STACK_SIZE 2048

static const char *TAG = "UART_MAIN";

void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver with RX buffer only
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized on TX=%d, RX=%d at %d baud", TXD_PIN, RXD_PIN, UART_BAUD_RATE);
}

int sendData(const char *logName, const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_PORT_NUM, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);

    while (1)
    {
        sendData(TX_TASK_TAG, "Hello Mahender\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2-second delay
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

    while (1)
    {
        ESP_LOGI(RX_TASK_TAG, "Waiting for data...");
        int rxBytes = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);

        if (rxBytes > 0)
        {
            data[rxBytes] = 0; // Null-terminate received data
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
        else
        {
            ESP_LOGI(RX_TASK_TAG, "No data received yet.");
        }
    }

    free(data);
}

void app_main(void)
{
    uart_init();

    // Give some delay to ensure UART is fully initialized before tasks start
    vTaskDelay(pdMS_TO_TICKS(500));

    // Create RX and TX tasks
    xTaskCreate(rx_task, "uart_rx_task", TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL);
}
