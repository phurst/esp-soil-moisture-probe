#ifndef LED_COMPONENT
#define LED_COMPONENT

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void reset_led_display(void);

void led_display_task_function(
  void* pvParameters
);

#endif
