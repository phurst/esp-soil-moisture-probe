#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_component.h"
#include "inter_task_messaging.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define FLASHER_PERIOD_MS 300
#define FLASH_PERIOD_MS 150

void configure_led(void);
void do_flash_led(void);

static bool is_connected_flag = false;

/**
 * PUBLIC functions
 */

void led_display_task_function(
    void* pvParameters
) {
    extern QueueHandle_t connectionStateMessageQueue;

    configure_led();
    while (1) {
        UBaseType_t count = uxQueueMessagesWaiting(connectionStateMessageQueue);
        ConnectionStateMessage msg;
        bool gotMsg = false;
        while (count > 0) {
            if (xQueueReceive(connectionStateMessageQueue, &msg, 0) == pdPASS) {
                gotMsg = true;
            }
            count = uxQueueMessagesWaiting(connectionStateMessageQueue);
        }
        if (gotMsg) {
            is_connected_flag = msg.isConnected;
        }
        do_flash_led();
        vTaskDelay(FLASHER_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

/**
 * Would like to use the thread_local_storage here but that prevents this function from being called
 * from a different task.
 * Should probably use messaging to convey the connection state between tasks.
 */
void reset_led_display(void) {
}

/**
 * PRIVATE functions
 */

void configure_led(void) {
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void do_flash_led(void) {
    printf("\ndo_flash_led\n");
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(FLASH_PERIOD_MS / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 0);
}
