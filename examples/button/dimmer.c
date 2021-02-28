#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

#define BLUELED 2
#define BUTTON  0

#define BUTTON_POLLING_TIME 13
#define DIM_PERIOD  33

int up_period = 0;
int direction = 1;

void read_button_task(void *pvParameter) {
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);

    while(true) {

        int button_level = gpio_get_level(BUTTON);
        if(button_level == 0) {
            up_period += direction;
            if(up_period == DIM_PERIOD) {
                direction = -1;
            } else if (up_period == 0) {
                direction = 1;
            }
        }
        
        vTaskDelay(BUTTON_POLLING_TIME / portTICK_RATE_MS);
    }
}


void blinky_task(void *pvParameter) {
    gpio_pad_select_gpio(BLUELED);
    gpio_set_direction(BLUELED, GPIO_MODE_OUTPUT);

    while(true) {
        gpio_set_level(BLUELED, 0);
        vTaskDelay(up_period / portTICK_RATE_MS);
        gpio_set_level(BLUELED, 1);
        vTaskDelay((DIM_PERIOD - up_period) / portTICK_RATE_MS);
    }
}


void app_main() {
    xTaskCreate(read_button_task, "read_btn", 1024, NULL, 1, NULL);
    xTaskCreate(blinky_task, "blinky", 1024, NULL, 1, NULL);
}