#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "esp_netif.h"
#include "wifi_component.h"

#define WIFI_LOOP_DELAY_MS                  5000
#define EXAMPLE_PROV_SEC2_USERNAME          "wifiprov"
#define EXAMPLE_PROV_SEC2_PWD               "abcd1234"

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

static void wifi_init(void);

static void event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
);

static void get_device_service_name(
  char* service_name,
  size_t max
);

void wifi_task_function(
  void* pvParameters
) {
  // /* Wait a bit to let the serial port calm down. */
  // vTaskDelay(WIFI_STARTUP_DELAY_MS / portTICK_PERIOD_MS);
  wifi_init();
  while (1) {

    printf("\n++++++++++++++++++++++++ WIFI +++++++++++++++++++++++++++++\n");

    vTaskDelay(WIFI_LOOP_DELAY_MS / portTICK_PERIOD_MS);
  }
}

static void wifi_init(void) {
  printf("\n++++++++++++++++++++++++ WIFI INIT +++++++++++++++++++++++++++++\n");

  /* Initialize TCP/IP */
  ESP_ERROR_CHECK(esp_netif_init());

  /* Initialize the event loop */
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_event_group = xEventGroupCreate();

  /* Register our event handler for Wi-Fi, IP and Provisioning related events */
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
  /* Initialize Wi-Fi including netif with default config */
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  /* Configuration for the provisioning manager */
  wifi_prov_mgr_config_t config = {
    /* What is the Provisioning Scheme that we want ?
     * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
    .scheme = wifi_prov_scheme_ble,

    /* Any default scheme specific event handler that you would
     * like to choose. Since our example application requires
     * neither BT nor BLE, we can choose to release the associated
     * memory once provisioning is complete, or not needed
     * (in case when device is already provisioned). Choosing
     * appropriate scheme specific event handler allows the manager
     * to take care of this automatically. This can be set to
     * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
  };

  /* Initialize provisioning manager with the
 * configuration parameters set above */
  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

  bool provisioned = false;
  /* Let's find out if the device is provisioned */
  ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
  /* If device is not yet provisioned start provisioning service */
  if (!provisioned) {
    printf("\n+++++++++++++++++++++++++++++++ Starting provisioning ++++++++++++++++++++++++++++++++++++\n");
    /* What is the Device Service Name that we want
     * This translates to :
     *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
     *     - device name when scheme is wifi_prov_scheme_ble
     */
    char service_name[12];
    get_device_service_name(service_name, sizeof(service_name));

    wifi_prov_security_t security = WIFI_PROV_SECURITY_2;

    /* This pop field represents the password that will be used to generate salt and verifier.
     * The field is present here in order to generate the QR code containing password.
     * In production this password field shall not be stored on the device */
    const char* username = EXAMPLE_PROV_SEC2_USERNAME;
    const char* pop = EXAMPLE_PROV_SEC2_PWD;


  } else {
    printf("\n+++++++++++++++++++++++++++++++ ALREADY PROVISIONED ++++++++++++++++++++++++++++++++++++\n");
  }
  printf("\n++++++++++++++++++++++++ WIFI INIT DONE +++++++++++++++++++++++++++++\n");
}

/* Event handler for catching system events */
static void event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
) {
  if (event_base == WIFI_PROV_EVENT) {
    printf("\n++++++++++++++++++++++++ WIFI EVENT +++++++++++++++++++++++++++++ %ld \n", event_id);
  }
}

static void get_device_service_name(
  char* service_name,
  size_t max
) {
  uint8_t eth_mac[6];
  const char* ssid_prefix = "PROV_";
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
  snprintf(service_name, max, "%s%02X%02X%02X",
    ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}
