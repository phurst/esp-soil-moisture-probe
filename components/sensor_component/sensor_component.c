#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "sensor_component.h"

#define SENSOR_LOOP_DELAY_MS      3000

static const char* TAG = "sensor_component";

void sensor_init(void);

void sensor_task_function(
  void* pvParameters
) {
  sensor_init();
  while (1) {
    printf("\nsensor\n");

    vTaskDelay(SENSOR_LOOP_DELAY_MS / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "LOOP AND TRY AGAIN...");
  }
}

void sensor_init(void) {
  printf("\nsensort init START\n");
  // gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);
  gpio_dump_io_configuration(stdout, (1ULL << 0) | (1ULL << 1) | (1ULL << 2));

  printf("\nsensort init DONE\n");
}

