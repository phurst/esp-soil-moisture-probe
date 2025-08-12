#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_component.h"

static const char *TAG = "NVS_COMPONENT";

static bool is_nvs_flash_initialized = false;

static nvs_handle_t get_nvs_handle();
static void initialize_nvs_flash(void);

bool get_is_provisioned(void) {
  // Function to get the provisioning state from NVS
  nvs_handle_t my_handle = get_nvs_handle();
  if (my_handle) {
    uint8_t is_provisioned = 0; // Default value
    esp_err_t err = nvs_get_u8(my_handle, "is_provisioned", &is_provisioned);
    nvs_close(my_handle);
    if (err == ESP_OK) {
      return is_provisioned ? true : false;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
      // Key not found, return default value
      return false;
    } else {
      ESP_LOGE(TAG, "Error reading NVS key: %s", esp_err_to_name(err));
      return false;
    }
  }
  return false;
}

void set_is_provisioned(bool newState) {

  // Function to set the provisioning state in NVS
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    nvs_set_u8(my_handle, "is_provisioned", newState ? 1 : 0);
    nvs_commit(my_handle);
    nvs_close(my_handle);
  } else {
    ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
  }
}

static nvs_handle_t get_nvs_handle() {
  nvs_handle_t my_handle;
  initialize_nvs_flash();
  if (is_nvs_flash_initialized) {
    // Open NVS handle
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
      return my_handle;
    }
  }
  return my_handle;
}

static void initialize_nvs_flash(void) {
  if (!is_nvs_flash_initialized) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(err = nvs_flash_init());
    }if (err == ESP_OK) {
      is_nvs_flash_initialized = true;
    } else {
      ESP_LOGE(TAG, "Error initializing NVS: %s", esp_err_to_name(err));
    }
  }
}
