#include <stdio.h>

#include "driver/gpio.h"

#include "driver/i2c.h"
#include "ssd1306_hal/io.h"
#include "ssd1306.h"

struct locked_menu {
    SAppMenu *menu;
    SemaphoreHandle_t lock;
};

void button_responder_task(void *pvParameters) {

    struct locked_menu *lm = (struct locked_menu *)pvParameters;
    
    gpio_pad_select_gpio(0);
    gpio_set_direction(0, GPIO_MODE_INPUT);

    while(true) {

        if(gpio_get_level(0) == 0) {
            if(xSemaphoreTake(lm->lock, 10)) {
                ssd1306_menuDown(lm->menu);
                xSemaphoreGive(lm->lock);
            }
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void app_main() {
    ssd1306_setFixedFont(ssd1306xled_font6x8);

    ssd1306_platform_i2cConfig_t cfg = {
        .sda = 21,
        .scl = 22
    };
    ssd1306_platform_i2cInit(I2C_NUM_0, 0, &cfg);

    ssd1306_128x64_i2c_init();

    ssd1306_clearScreen();

    SAppMenu menu;
    const char *menuItems[] = {
        "WiFi",
        "Bluetooth",
        "Airplane mode",
        "Other..."
    };
    ssd1306_createMenu(&menu, menuItems, sizeof(menuItems) /sizeof(char *));
    ssd1306_showMenu(&menu);

    SemaphoreHandle_t menuSem = xSemaphoreCreateBinary();
    struct locked_menu task_params = {.menu = &menu, .lock = menuSem};
    xTaskCreate(button_responder_task, "button", 1024, &task_params, 1, NULL);

    while(true) {
        xSemaphoreTake(menuSem, 10);
        ssd1306_showMenu(&menu);
        xSemaphoreGive(menuSem);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}