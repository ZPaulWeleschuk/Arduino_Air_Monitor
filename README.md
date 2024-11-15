Air Monitor


This project uses various sensors and components to monitor and display the air quality in an indoor space. 

The values are tracked over a 24 hour period so you can see how values fluctuate over the course of a day.

This project uses many communication protocols due to the sensors/libraries preferences. So this is a good project if you are interested in learning about communication protocols (UART, SPI, I2C)

Sensors:
BME680 : Indoor Air Quality Index (volatile organic compounds), barometric station pressure

10k @ 25c NTC thermistor  :  temperature

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
|Data|DP8|
|VCC|5V|
|GND|GND|


### TFT Display
|Pins |Connection|
|------:|:---------|
|VCC|5V|
|GND|GND|
|CS| DP53 (3.3V TTL)|
|RESET|DP49 (3.3V TTL)|
|DC|DP48 (3.3V TTL)|
|MOSI| DP51 (3.3V TTL)|
|SCK|DP52 (3.3V TTL)|
|LED| 5V|


### Particle Matter Sensor 7003
|Pins |Connection|
|------:|:---------|
|VCC|5V|
|GND|GND|
|set|not connected |
|Rx|Tx3/DP14 (3.3V TTL)|
|Tx|Rx3/DP15 (3.3V TTL)|
|rst|not connected|


### BME 680
|Pins |Connection|
|------:|:---------|
|VCC|5V|
|GND|GND|
|SCL| I2C Bus -> DP21/SCL |
|SDA| I2C Bus -> DP20/SDA|
|SDO|not connected|
|CS|not connected|


### Real Time Clock DS3231
|Pins |Connection|
|------:|:---------|
|32k|not connected|
|SQW|not connected|
|SCL| I2C Bus -> DP21/SCL |
|SDA| I2C Bus -> DP20/SDA|
|VCC|5V|
|GND|GND|


### Sensair CO2 Sensor S8
|Pins |Connection|
|------:|:---------|
|VCC_out|not connected|
|UART_RxD|DP16/Tx2|
|UART_TxD|DP17/Rx2|
|UART_R/T|not connected|
|bCAL_in|not connected|
|G+|VCC|
|G0|GND|
|Alarm_OC|not connected|
|PWM 1KHz|not connected|


### NTC Thermistor
|Pins |Connection|
|------:|:---------|
|1|5V|
|2|A1 <-> 10k resistor -> GND|





