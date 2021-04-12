#include "i2c.h"
#include "esp_err.h"
#include "driver/i2c.h"

void i2c_configure() {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 22,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_1, &config));
    i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 128, 128, 0);
}