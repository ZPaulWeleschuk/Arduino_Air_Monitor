Air Monitor


This project uses various sensors and components to monitor and display the air quality in an indoor space. 

The values are tracked over a 24 hour period so you can see how values fluctuate over the course of a day.

This project uses many communication protocols due to the sensors/libraries preferences. So this is a good project if you are interested in learning about communication protocols (UART, SPI, I2C)

Sensors:
BME680 : Indoor Air Quality Index (volatile organic compounds), barometric station pressure

NTC thermistor  :  temperature

DHT22 : humidity

PMS 7003 : particulate matter

Sensair S8 (004-0-0053) : CO2

Components:
DS3231 : real time clock

TFT : display

Arduino Mega Pro

resistors [1-10k, 5-18k, 5-33k, 2-4.7k]

prototyping board



### DHT22 Humidity Sensor
|Pinout|Connection|
|---------:|:--------|
|Data|DP 8|
|VCC|5V|
|GND|GND|


### TFT Display
|Pins |Voltage|
|------:|:---------|
|VCC|5V|
|GND|GND|
|CS| DP 53, 3.3V TTL|
|RESET|DP 49, 3.3V TTL|
|DC|DP 48, 3.3V TTL|
|MOSI| DP 51, 3.3V TTL|
|SCK|DP 52, 3.3V TTL|
|LED| 5V|

TODO: make tables for pin outs of each sensor