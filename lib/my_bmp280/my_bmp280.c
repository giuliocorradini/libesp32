#include "my_bmp280.h"
#include "driver/i2c.h"

esp_err_t bmp280_read_register(uint8_t reg, uint8_t *dst, int n) {

    esp_err_t err;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    //  Write pointer to register (it'll be read in the next transaction)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMP280_ADDR << 1 | I2C_MASTER_WRITE), true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);

    //  Read requested register
    i2c_cmd_link_delete(cmd);
    cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BMP280_ADDR << 1 | I2C_MASTER_READ), true);
    i2c_master_read(cmd, dst, n, true);
    i2c_master_stop(cmd);

    err |= i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);

    return err;
}

int bmp280_get_id() {
    int id = 0;
    ESP_ERROR_CHECK(bmp280_read_register(0xD0, &id, 1));

    return id;
}

esp_err_t bmp280_get_compensation_params(struct bmp280_compensation_params *p) {
    esp_err_t err = ESP_OK;

    uint8_t base_addr = 0x88;

    for(uint8_t i = 0; i < 24; i++) {
        err |= bmp280_read_register(base_addr + i, &p + i, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    return err;
}