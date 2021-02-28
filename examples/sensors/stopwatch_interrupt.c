/*
 *  Stopwatch - Interrupt version
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"

#define BUTTON_PIN  0
#define LED_PIN     2
#define TOUCH_PIN   0   //Touch enumeration != GPIO enum (= GPIO4)

#define TOUCH_LOW_THRESHOLD     50
#define TOUCH_HIGH_THRESHOLD    450

#define HALL_POSITIVE_THRESHOLD 55
#define HALL_NEGATIVE_THRESHOLD 0

//Uncomment and flash to calibrate hall sensor with your magnet
//#define CALIBRATE_HALL

//Uncomment and flash to calibrate touch pin
//#define CALIBRATE_TOUCH

static int time_counter = 0;
static int lap = 0;
static bool counting = false;

enum {
    GPIO_NOTIFICATION   =   (1<<0),
    TOUCH_NOTIFICATION  =   (1<<1),
    HALL_NOTIFICATION   =   (1<<2),
    MAX_NOTIFICATION
};

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

void button_check_isr(void *args) {
    EventGroupHandle_t e = (EventGroupHandle_t)args;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    int button_state = gpio_get_level(BUTTON_PIN);

    if(button_state == 0)
        xEventGroupSetBitsFromISR(e, GPIO_NOTIFICATION, &xHigherPriorityTaskWoken);
    else
        xEventGroupClearBitsFromISR(e, GPIO_NOTIFICATION);
}

void touch_check_isr(void *args) {
    //Only works for short press
    EventGroupHandle_t e = (EventGroupHandle_t)args;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xEventGroupSetBitsFromISR(e, TOUCH_NOTIFICATION, &xHigherPriorityTaskWoken);
}

void hall_task(void *args) {
    EventGroupHandle_t e = (EventGroupHandle_t)args;

    while(true) {
        int value = hall_sensor_read();
        if(value < HALL_NEGATIVE_THRESHOLD || value > HALL_POSITIVE_THRESHOLD) {
            xEventGroupSetBits(e, HALL_NOTIFICATION);
        } else {
            xEventGroupClearBits(e, HALL_NOTIFICATION);
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

bool isButtonPressed(EventGroupHandle_t e) {
    EventBits_t pressed = xEventGroupWaitBits(e, 0x07, pdFALSE, pdFALSE, 1);
    if(pressed & TOUCH_NOTIFICATION)
        xEventGroupClearBits(e, TOUCH_NOTIFICATION);

    return (bool)pressed;
}

void button_fsm(void *pvParameters){
    //EventGroup for touch, hall and gpio
    EventGroupHandle_t e = xEventGroupCreate();

    //Configure GPIO interrupt
    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    gpio_isr_handler_add(BUTTON_PIN, button_check_isr, (void*)e);


    //Configure touch pad interrupt
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(TOUCH_PIN, TOUCH_HIGH_THRESHOLD);

    touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
    touch_pad_isr_register(touch_check_isr, (void*)e);
    touch_pad_intr_enable();

    adc1_config_width(ADC_WIDTH_12Bit);
    xTaskCreate(hall_task, "hall", 1024, e, 1, NULL);

    static int stato = 0;
    bool button_pressed = false;

    while(true) {
        switch(stato) {
            case 0: //not pressed
                gpio_set_level(LED_PIN, 0);
                if(button_pressed) {
                    gpio_set_level(LED_PIN, 1);
                    stato = 1;
                }
                break;
            case 1: //pressed
                if(!button_pressed) {
                    stato = 0;
                    short_press();
                } else {
                    stato = 2;
                }
                break;
            case 2: //pressed for more than 500ms
                if(!button_pressed) {
                    stato = 0;
                    short_press();
                } else {
                    stato = 3;
                }
                break;
            case 3:
                if(!button_pressed) {
                    long_press();
                    stato = 0;
                }
                break;
        }

        vTaskDelay((stato == 1 ? 500 : 10) / portTICK_RATE_MS);
        button_pressed = isButtonPressed(e);
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

void calibrate_touch(void *pv) {
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(TOUCH_PIN, 0); //Don't use any threshold

    short unsigned touch_value;

    while(true) {
        touch_pad_read(TOUCH_PIN, &touch_value);
        printf("%d\n", touch_value);

        vTaskDelay(50);
    }
}

void app_main() {
    #ifndef CALIBRATE_HALL

    #ifndef CALIBRATE_TOUCH
    xTaskCreate(timer_task, "timer", 2048, NULL, 0, NULL);
    xTaskCreate(button_fsm, "fsm", 2048, NULL, 0, NULL);
    #else //CALIBRATE_TOUCH
    xTaskCreate(calibrate_touch, "touch", 2048, NULL, 0, NULL);
    #endif

    #else //CALIBRATE_HALL
    xTaskCreate(calibrate_hall, "hall", 2048, NULL, 0, NULL);
    #endif
}
