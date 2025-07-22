#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "sensor_component.h"

#define SENSOR_LOOP_DELAY_MS      3000
#define ADC1_CHAN0          ADC_CHANNEL_0
#define ADC1_CHAN1          ADC_CHANNEL_1
#define ADC2_CHAN0          ADC_CHANNEL_0
#define ADC_ATTEN           ADC_ATTEN_DB_12

static const char* TAG = "sensor_component";
static bool did_calibration1_chan0 = false;
static adc_cali_handle_t adc1_cali_chan0_handle = NULL;
static adc_oneshot_unit_handle_t adc1_handle;


void sensor_init(void);
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle);
static void adc_calibration_deinit(adc_cali_handle_t handle);

void sensor_task_function(
  void* pvParameters
) {
  sensor_init();
  while (1) {
    int adc_raw[2][10];
    int voltage[2][10];

    printf("\nsensor\n");

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN0, &adc_raw[0][0]));
    printf("\nsensor: ADC%d Channel[%d] Raw Data: %d\n", ADC_UNIT_1 + 1, ADC1_CHAN0, adc_raw[0][0]);
    if (did_calibration1_chan0) {
      ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
      printf("\nsensor: ADC%d Channel[%d] Cali Voltage: %d mV\n", ADC_UNIT_1 + 1, ADC1_CHAN0, voltage[0][0]);
    }

    vTaskDelay(SENSOR_LOOP_DELAY_MS / portTICK_PERIOD_MS);
  }
}

void sensor_init(void) {
  printf("\nsensor init START\n");
  gpio_dump_io_configuration(stdout, (1ULL << 0) | (1ULL << 1) | (1ULL << 2));

  //-------------ADC1 Init---------------//
  // adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  //-------------ADC1 Config---------------//
  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

  //-------------ADC1 Calibration Init---------------//
  did_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, ADC1_CHAN0, ADC_ATTEN, &adc1_cali_chan0_handle);

  printf("\nsensor init DONE\n");
}

void sensor_tear_down(void) {
  if (did_calibration1_chan0) {
    adc_calibration_deinit(adc1_cali_chan0_handle);
  }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    printf("calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    printf("calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  * out_handle = handle;
  if (ret == ESP_OK) {
    printf("Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  printf("deregister %s calibration scheme", "Curve Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  printf("deregister %s calibration scheme", "Line Fitting");
  ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}


