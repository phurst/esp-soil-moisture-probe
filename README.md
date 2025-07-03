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

The sensor component should use code from the One-shot read example.
A sender component (TBD) should use a network to dispatch samples and track the health of the network connection.
The LED component should provide an indication of the health of the system.
  Is it reading samples from the sensor?
  Are the samples being sent somewhere successfully?

