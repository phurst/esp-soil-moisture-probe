/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include <wifi_provisioning/manager.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "inter_task_messaging.h"
#include "led_component.h"
#include "sensor_component.h"
#include "wifi_component.h"
#include "nvs_component.h"

#define APP_STARTUP_DELAY_MS   1000

int myInt = 123;

static EventGroupHandle_t appEventGroup;
QueueHandle_t connectionStateMessageQueue;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t ledDisplayTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;

static void wifi_task(void* pvParameters) {
    wifi_task_function(pvParameters);
}

static void sensor_task(void* pvParameters) {
    sensor_task_function(pvParameters);
}

static void led_display_task(void* pvParameters) {
    led_display_task_function(pvParameters);
}

void app_main(void) {
    /* Wait a bit to let the serial port calm down. */
    vTaskDelay(APP_STARTUP_DELAY_MS / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    appEventGroup = xEventGroupCreate();

    if(!get_is_provisioned()) {
        printf("\n+++++++++++++++++++++++++++++++ NOT PROVISIONED ++++++++++++++++++++++++++++++++++++\n");
        return;
    }

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    connectionStateMessageQueue = xQueueCreate(CONNECTION_STATE_MESSAGE_QUEUE_LENGTH, sizeof(ConnectionStateMessage));
    ConnectionStateMessage msg;
    msg.isConnected = false;
    if (xQueueSend(connectionStateMessageQueue, (void*)&msg, 0) != pdPASS) {
        printf("\nERROR - MAIN Can't send to message queue\n");
        return;
    }

    xTaskCreate(&wifi_task, "wifi_task", 8192, NULL, 5, &wifiTaskHandle);

    // xTaskCreate(&led_display_task, "led_display_task", 8192, NULL, 5, &ledDisplayTaskHandle);
    // xTaskCreate(&sensor_task, "sensor_task", 8192, NULL, 5, &sensorTaskHandle);
}
