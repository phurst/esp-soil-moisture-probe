# esp-soil-moisture-probe
Test connection to a soil moisture probe

# Setup
This project runs on a Seeed Studio XIAO ESP32C6 with a DFRobot Waterproof Soil Moisture Sensor SKU SENO308
on GPIO A0.

Set the target to: esp32c6

> idf.py set-target esp32c6

Run 

> idf.py menuconfig

and set:

  CONFIG_BLINK_LED_GPIO=y
  CONFIG_BLINK_GPIO=15

Use the Windows Device Manager to determine which COM port the SparkFun-ThingPlus-ESP32-C6 is connected to.

# Usage

> idf.py -p COM5 flash monitor

# Operation

The SparkFun-ThingPlus-ESP32-C6 repeatedly blinks the LED.

