#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"

#define BUTTON_PIN  0
#define LED_PIN     2
#define TOUCH_PIN   0   //Touch enumeration != GPIO enum

#define TOUCH_LOW_THRESHOLD     300
#define TOUCH_HIGH_THRESHOLD    400

#define HALL_POSITIVE_THRESHOLD 55
#define HALL_NEGATIVE_THRESHOLD 0

//Uncomment and flash to calibrate hall sensor with your magnet
//#define CALIBRATE_HALL

static int time_counter = 0;
static int lap = 0;
static bool counting = false;

void short_press() {
    if(!counting) {
        lap = time_counter;
        counting = true;
    } else {
        int elapsed = time_counter - lap;
        printf("\x1b[32mElapsed time: %02d:%02d:%02d.%02d\x1b[0m\n", elapsed/360000, elapsed/6000 % 60, elapsed/100 % 60, elapsed%100);
    }
    printf("\x1b[33mShort press\x1b[0m\n");
}

void long_press() {
    counting = false;
    int elapsed = time_counter - lap;
    printf("\x1b[33mLong press\x1b[0m\n");
    printf("\x1b[32mElapsed time: %02d:%02d:%02d.%02d\x1b[0m\n", elapsed/360000, elapsed/6000 % 60, elapsed/100 % 60, elapsed%100);
}

bool isButtonPressed() {
    short unsigned tp_value;
    touch_pad_read(TOUCH_PIN, &tp_value);

    int hall_value;
    hall_value = hall_sensor_read();

    return gpio_get_level(BUTTON_PIN) == 0
        || (tp_value > TOUCH_LOW_THRESHOLD && tp_value < TOUCH_HIGH_THRESHOLD)
        || hall_value < HALL_NEGATIVE_THRESHOLD || hall_value > HALL_POSITIVE_THRESHOLD;
}

void button_fsm(void *pvParameters){
    static int stato = 0;

    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(TOUCH_PIN, 0); //Don't use any threshold

    adc1_config_width(ADC_WIDTH_12Bit);

    while(true) {
        switch(stato) {
            case 0: //not pressed
                gpio_set_level(LED_PIN, 0);
                if(isButtonPressed()) {
                    gpio_set_level(LED_PIN, 1);
                    stato = 1;
                }
                break;
            case 1: //pressed
                if(!isButtonPressed()) {
                    stato = 0;
                    short_press();
                } else {
                    stato = 2;
                }
                break;
            case 2: //pressed for more than 500ms
                if(!isButtonPressed()) {
                    stato = 0;
                    short_press();
                } else {
                    stato = 3;
                    long_press();
                }
                break;
            case 3:
                if(!isButtonPressed()) {
                    stato = 0;
                }
                break;
        }

        vTaskDelay((stato == 1 ? 500 : 10) / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void timer_task(void *pvParameters) {
    while(true) {
        if(time_counter%100 == 0) {
            printf("%02d:%02d:%02d\n", time_counter/360000, time_counter/6000 % 60, time_counter/100 % 60);
        }
        time_counter++;
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void calibrate_hall(void *pv) {
    adc1_config_width(ADC_WIDTH_12Bit);

    while(true) {
        int r = hall_sensor_read();
        printf("Hall: %d\n", r);

        vTaskDelay(10);
    }

}

void app_main() {
    #ifndef CALIBRATE_HALL
    xTaskCreate(timer_task, "timer", 2048, NULL, 0, NULL);
    xTaskCreate(button_fsm, "fsm", 2048, NULL, 0, NULL);
    #elif CALIBRATE_HALL
    xTaskCreate(calibrate_hall, "hall", 2048, NULL, 0, NULL);
    #endif
}