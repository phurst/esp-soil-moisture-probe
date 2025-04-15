# esp-wifi-test-connection
Test connection to a specific WIFI network from an ESP32 dev board

# Setup
This project runs on a Seeed Studio XIAO ESP32C6 with no attached peripherals.

Set the target to: esp32c6

> idf.py set-target esp32c6

Run 

> idf.py menuconfig

and set:

  CONFIG_BLINK_LED_GPIO=y
  CONFIG_BLINK_GPIO=8

Use the Windows Device Manager to determine which COM port the SparkFun-ThingPlus-ESP32-C6 is connected to.

# Usage

> idf.py -p COM5 flash monitor

# Operation

The SparkFun-ThingPlus-ESP32-C6repeatedly attempts to make a network connection to the specified WiFi network and shows the result by blinking the LED.

In each cycle you should initially expect to see a couple of RED blinks while the connection is attempted, and then about 5 GREEN blinks if the connection attempt is successful. All RED blinks indicate no connection could be made.
