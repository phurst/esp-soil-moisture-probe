#include <esp_wifi.h>
#include "device_service_name.h"

void get_device_service_name(
  char* service_name,
  size_t max
) {
  uint8_t eth_mac[6];
  const char* ssid_prefix = "PROV_";
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
  snprintf(service_name, max, "%s%02X%02X%02X",
    ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}
