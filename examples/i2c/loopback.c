/* i2c - Loopback
   Adapted from:
   https://github.com/espressif/esp-idf/tree/master/examples
   
   I2C controllers connected in loopback. Read from UART and transmit
   over I2C from slave to master.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "string.h"

static const char *TAG = "i2c-example";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */

#define I2C_SLAVE_SCL_IO 4               /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO 5               /*!< gpio number for i2c slave data */
#define I2C_SLAVE_NUM I2C_NUMBER(CONFIG_I2C_SLAVE_PORT_NUM) /*!< I2C port number for slave dev */
#define I2C_SLAVE_TX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave tx buffer size */
#define I2C_SLAVE_RX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave rx buffer size */

#define I2C_MASTER_SCL_IO 18               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 19               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define ESP_SLAVE_ADDR 0x0f /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */


/**
 * @brief test code to read esp-i2c-slave
 *        We need to fill the buffer of esp slave device, then master can read them out.
 *
 * _______________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------------|--------------------|------|
 *
 */
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Test code to write esp-i2c-slave
 *        Master device write data to slave(both esp32),
 *        the data will be stored in slave buffer.
 *        We can read them out from slave buffer.
 *
 * ___________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------------|------|
 *
 */
static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief i2c slave initialization
 */
static esp_err_t i2c_slave_init(void)
{
    i2c_config_t conf_slave = {
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .mode = I2C_MODE_SLAVE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = ESP_SLAVE_ADDR,
    };
    esp_err_t err = i2c_param_config(I2C_NUM_1, &conf_slave);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_NUM_1, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

/**
 * @brief test function to show buffer
 */
static void disp_buf(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int readline(char *dst, int max) {
    int this_char, i=0;

    do {
        this_char = getchar();
        if(this_char != -1) {
            dst[i++] = this_char;
            putchar(this_char);
        } else {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    } while (this_char != '\n' && i<max);

    return i;
}

#define BLOCKSIZE   8
static void i2c_test_task(void *arg)
{
    char message[256] = "Ciao come stai?";
    uint8_t incoming_msg[256];
    int incoming_len;
    int ret, readchars = strlen(message);

    while (true) {

        printf(">> ");
        readchars = readline(message, 256);

        //  Slave first responds with number of prepared bytes
        ret = i2c_slave_write_buffer(I2C_NUM_1, &readchars, sizeof(int), 1000 / portTICK_RATE_MS);
        if(ret < 0)
            ESP_LOGE("I2C Slave", "Error writing");

        ret = i2c_slave_write_buffer(I2C_NUM_1, (uint8_t *)message, readchars, 1000 / portTICK_RATE_MS);
        if(ret < 0)
            ESP_LOGE("I2C Slave", "Error writing");
        else
            ESP_LOGI("I2C Slave", "Prepared %d bytes in tx buffer", ret);

        ESP_ERROR_CHECK(i2c_master_read_slave(I2C_NUM_0, &incoming_len, sizeof(int)));
        ESP_ERROR_CHECK(i2c_master_read_slave(I2C_NUM_0, incoming_msg, incoming_len));
        ESP_LOGD("I2C Master", "Showing hex dump of read:");
        disp_buf(incoming_msg, incoming_len);
        
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    // Reopen UART in blocking mode
    //ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    //esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    ESP_ERROR_CHECK(i2c_slave_init());
    ESP_ERROR_CHECK(i2c_master_init());
    xTaskCreate(i2c_test_task, "i2c", 4096, NULL, 10, NULL);
}
