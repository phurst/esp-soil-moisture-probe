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
#include "salt_and_verifier.h"
#include "device_service_name.h"
#include "qr_code.h"

#define WIFI_LOOP_DELAY_MS      1000
#define PROV_TRANSPORT_BLE      "ble"

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifiEventGroup;

static void wifi_init(void);

static void event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data
);

void wifi_task_function(
  void* pvParameters
) {
  wifi_init();
  while (true) {
    vTaskDelay(WIFI_LOOP_DELAY_MS / portTICK_PERIOD_MS);
    printf("\n++++++++++++++++++++++++ WIFI LOOP +++++++++++++++++++++++++++++\n");
  }
}

static void wifi_init(void) {
  printf("\n++++++++++++++++++++++++ WIFI INIT +++++++++++++++++++++++++++++\n");

  /* Initialize TCP/IP */
  ESP_ERROR_CHECK(esp_netif_init());

  /* Initialize the event loop */
  // ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifiEventGroup = xEventGroupCreate();

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

    wifi_prov_security2_params_t sec_params = get_prov_security2_params();

    /* What is the service key (could be NULL)
     * This translates to :
     *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
     *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
     *     - simply ignored when scheme is wifi_prov_scheme_ble
     */
    const char* service_key = NULL;

#ifdef CONFIG_EXAMPLE_PROV_TRANSPORT_BLE
    /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
     * set a custom 128 bit UUID which will be included in the BLE advertisement
     * and will correspond to the primary GATT service that provides provisioning
     * endpoints as GATT characteristics. Each GATT characteristic will be
     * formed using the primary service UUID as base, with different auto assigned
     * 12th and 13th bytes (assume counting starts from 0th byte). The client side
     * applications must identify the endpoints by reading the User Characteristic
     * Description descriptor (0x2901) for each characteristic, which contains the
     * endpoint name of the characteristic */
    uint8_t custom_service_uuid[] = {
      /* LSB <---------------------------------------
       * ---------------------------------------> MSB */
      0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
      0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
    };

    /* If your build fails with linker errors at this point, then you may have
     * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
     * the sdkconfig.defaults in the example project) */
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

    /* Start provisioning service */
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void*)&sec_params, service_name, service_key));

    wifi_prov_print_qr(service_name, get_username(), get_pwd(), PROV_TRANSPORT_BLE);

#endif /* CONFIG_EXAMPLE_PROV_TRANSPORT_BLE */

  } else {
    printf("\n+++++++++++++++++++++++++++++++ ALREADY PROVISIONED ++++++++++++++++++++++++++++++++++++\n");
  }
  // printf("\n++++++++++++++++++++++++ WIFI INIT DONE +++++++++++++++++++++++++++++\n");
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
    switch (event_id) {
    case WIFI_PROV_START:
      printf("\n++++++++++++++++++++++++ Provisioning started +++++++++++++++++++++++++++++ \n");
      break;
    case WIFI_PROV_CRED_RECV: {
      wifi_sta_config_t* wifi_sta_cfg = (wifi_sta_config_t*)event_data;
      printf("\n++++++++++++++++++++++++Received Wi-Fi credentials"
        "\n\tSSID     : %s\n\tPassword : %s\n",
        (const char*)wifi_sta_cfg->ssid,
        (const char*)wifi_sta_cfg->password);
      break;
    }
    case WIFI_PROV_CRED_FAIL: {
      wifi_prov_sta_fail_reason_t* reason = (wifi_prov_sta_fail_reason_t*)event_data;
      printf("\n++++++++++++++++++++++++Provisioning failed!\n\tReason : %s"
        "\n\tPlease reset to factory and retry provisioning\n",
        (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
        "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
      /* Reset the state machine on provisioning failure.
       * This is enabled by the CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE configuration.
       * It allows the provisioning manager to retry the provisioning process
       * based on the number of attempts specified in wifi_conn_attempts. After attempting
       * the maximum number of retries, the provisioning manager will reset the state machine
       * and the provisioning process will be terminated.
       */
      wifi_prov_mgr_reset_sm_state_on_failure();
#endif
      break;
    }
    case WIFI_PROV_CRED_SUCCESS:
      printf("\n++++++++++++++++++++++++ Provisioning successful +++++++++++++++++++++++++++++ \n");
      break;
    case WIFI_PROV_END:
      /* De-initialize manager once provisioning is finished */
      printf("\n++++++++++++++++++++++++ Provisioning end +++++++++++++++++++++++++++++ \n");
      wifi_prov_mgr_deinit();
      break;
    default:
      break;
    }
  } else if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
      case WIFI_EVENT_STA_DISCONNECTED:
        printf("\n++++++++++++++++++++++++ Disconnected. Connecting to the AP again... +++++++++++++++++++++++++++++ \n");
        esp_wifi_connect();
        break;
      default:
        break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    printf("\n++++++++++++++++++++++++ Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
    /* Signal main application to continue execution */
    xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_EVENT);
  } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
    switch (event_id) {
    case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
      printf("\n++++++++++++++++++++++++BLE transport: Connected!\n");
      break;
    case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
      printf("\n++++++++++++++++++++++++BLE transport: Disconnected!\n");
      break;
    default:
      break;
    }
  } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
    switch (event_id) {
    case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
      printf("\n++++++++++++++++++++++++Secured session established!\n");
      break;
    case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
      printf("\n++++++++++++++++++++++++Received invalid security parameters for establishing secure session!\n");
      break;
    case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
      printf("\n++++++++++++++++++++++++Received incorrect username and/or PoP for establishing secure session!\n");
      break;
    default:
      break;
    }
  }
}

