#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mqtt_component.h"

#define MQTT_PERIOD_MS 3000
#define EXAMPLE_WIFI_SCAN_METHOD WIFI_FAST_SCAN
#define EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#define NETIF_DESC_STA "netif_sta"

void mqtt_init(void);
void mqtt_connect(void);
void wifi_start(void);
void wifi_stop(void);
esp_err_t wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);
esp_err_t wifi_sta_do_disconnect(void);
bool is_our_netif(const char* prefix, esp_netif_t* netif);

static void handler_on_wifi_connect(
  void* esp_netif,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
);
static void handler_on_wifi_disconnect(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
);
static void handler_on_sta_got_ip(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
);

static esp_netif_t* s_sta_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
static int s_retry_num = 0;

/**
 * PUBLIC functions
 */

void mqtt_task_function(
  void* pvParameters
) {
  mqtt_init();
  while (1) {
    printf("\nmqtt\n");
    vTaskDelay(MQTT_PERIOD_MS / portTICK_PERIOD_MS);
  }
}

/**
 * PRIVATE functions
 */

void mqtt_init(void) {
  printf("\nMQTT INIT\n");

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  // ESP_ERROR_CHECK(example_connect());
  mqtt_connect();
}

void mqtt_connect(void) {
  // From C:\Espressif\frameworks\esp-idf-v5.4.1\examples\common_components\protocol_examples_common\wifi_connect.c
  wifi_start();
  wifi_config_t wifi_config = {
    .sta = {
        .ssid = CONFIG_EXAMPLE_WIFI_SSID,
        .password = CONFIG_EXAMPLE_WIFI_PASSWORD,
        .scan_method = EXAMPLE_WIFI_SCAN_METHOD,
        .sort_method = EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD,
        .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
        .threshold.authmode = EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD,
    },
  };
  esp_err_t err = wifi_sta_do_connect(wifi_config, true);
  if (err != ESP_OK) {
    printf("\nWiFi connect failed! ret:%x\n", err);
  }
  ESP_ERROR_CHECK(wifi_sta_do_connect);

  void wifi_stop(void);
}

void wifi_start(void) {
  // From C:\Espressif\frameworks\esp-idf-v5.4.1\examples\common_components\protocol_examples_common\wifi_connect.c
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
  // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
  esp_netif_config.if_desc = "example_netif_sta";
  esp_netif_config.route_prio = 128;
  s_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
  esp_wifi_set_default_wifi_sta_handlers();

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_stop(void) {
  esp_err_t err = esp_wifi_stop();
  if (err == ESP_ERR_WIFI_NOT_INIT) {
    return;
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_sta_netif));
  esp_netif_destroy(s_sta_netif);
  s_sta_netif = NULL;
}

esp_err_t wifi_sta_do_connect(wifi_config_t wifi_config, bool wait) {
  if (wait) {
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip_addrs == NULL) {
      return ESP_ERR_NO_MEM;
    }
  }
  s_retry_num = 0;
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_sta_got_ip, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect, s_sta_netif));

  printf("Connecting to %s...\n", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_err_t ret = esp_wifi_connect();
  if (ret != ESP_OK) {
    printf("WiFi connect failed! ret:%x\n", ret);
    return ret;
  }
  if (wait) {
    printf("Waiting for IP(s)\n");
    xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
    if (s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY) {
      return ESP_FAIL;
    }
  }
  return ESP_OK;
}

esp_err_t wifi_sta_do_disconnect(void) {
  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect));
  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_sta_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect));
  if (s_semph_get_ip_addrs) {
    vSemaphoreDelete(s_semph_get_ip_addrs);
  }
  return esp_wifi_disconnect();
}

bool is_our_netif(const char* prefix, esp_netif_t* netif) {
  return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void handler_on_wifi_connect(
  void* esp_netif,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
) {
#if CONFIG_EXAMPLE_CONNECT_IPV6
  esp_netif_create_ip6_linklocal(esp_netif);
#endif // CONFIG_EXAMPLE_CONNECT_IPV6
}

static void handler_on_wifi_disconnect(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
) {
  s_retry_num++;
  if (s_retry_num > CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY) {
    printf("WiFi Connect failed %d times, stop reconnect.\n", s_retry_num);
    /* let example_wifi_sta_do_connect() return */
    if (s_semph_get_ip_addrs) {
      xSemaphoreGive(s_semph_get_ip_addrs);
    }
    wifi_sta_do_disconnect();
    return;
  }
  wifi_event_sta_disconnected_t* disconn = event_data;
  if (disconn->reason == WIFI_REASON_ROAMING) {
    printf("station roaming, do nothing\n");
    return;
  }
  printf("Wi-Fi disconnected %d, trying to reconnect...\n", disconn->reason);
  esp_err_t err = esp_wifi_connect();
  if (err == ESP_ERR_WIFI_NOT_STARTED) {
    return;
  }
  ESP_ERROR_CHECK(err);
}

static void handler_on_sta_got_ip(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
) {
  s_retry_num = 0;
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  if (!is_our_netif(NETIF_DESC_STA, event->esp_netif)) {
    return;
  }
  printf("Got IPv4 event: Interface\n");
  if (s_semph_get_ip_addrs) {
    xSemaphoreGive(s_semph_get_ip_addrs);
  } else {
    printf("- IPv4 address:\n");
  }
}
