#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqqt_component.h"

#define MQQT_PERIOD_MS 3000

void mqqt_init(void);

/**
 * PUBLIC functions
 */

void mqqt_task_function(
  void* pvParameters
) {
  while (1) {
    printf("\nmqqt\n");
    vTaskDelay(MQQT_PERIOD_MS / portTICK_PERIOD_MS);
  }
}

/**
 * PRIVATE functions
 */

void mqqt_init(void) {
  printf("\nMQQT INIT\n");

  ESP_ERROR_CHECK(esp_netif_init());
}
