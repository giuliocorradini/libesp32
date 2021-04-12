#include "bmp280.h"
#include "driver/i2c.h"

esp_err_t bmp280_read_register(uint8_t addr, uint8_t reg, uint8_t *dst, int n) {

    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    //  Write pointer to register (it'll be read in the next transaction)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1 | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);

    //  Read requested register
    i2c_cmd_link_delete(cmd);
    cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1 | I2C_MASTER_READ), true);
    i2c_master_read(cmd, dst, n, true);
    i2c_master_stop(cmd);

    err |= i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);

    return err;
}

int bmp280_get_id() {
    int id = 0;
    ESP_ERROR_CHECK(bmp280_read_register(BMP280_ADDR, 0xD0, &id, 1));

    return id;
}