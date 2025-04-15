#ifndef INTER_TASK_MESSAGING_COMPONENT
#define INTER_TASK_MESSAGING_COMPONENT

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#define CONNECTION_STATE_MESSAGE_QUEUE_LENGTH 10

typedef struct {
  bool isConnected;
} ConnectionStateMessage;

void func(void);

#endif
