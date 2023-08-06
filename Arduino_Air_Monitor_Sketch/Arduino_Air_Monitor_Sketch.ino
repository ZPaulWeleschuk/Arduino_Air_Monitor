using namespace std;
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <Arduino.h>
#include <uRTCLib.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include "PMS.h"
#include "bsec.h"


//---------------------------------------------------------------------------------
//screencode
//2.4 inch TFT display example code
//The code is forked from the ‘graphicstest’ example code that comes
//with the Adafruit ST77XX library. However that code does not
//mention the 2.4” TFT display, so I reorganized the code specifically
//for the 2.4 inch display.


/*
┌────────────────┐
│Board │SPI Pin #│
├──────┼─────────┤
│UNO   │MOSI: 11 │
│      │MISO: 12 │
│      │SCK: 13  │
│      │CS: 10   │
├──────┼─────────┤
│NANO  │MOSI: 11 │
│      │MISO: 12 │
│      │SCK: 13  │
│      │CS: 10   │
├──────┼─────────┤
│MEGA  │MOSI: 51 │
│      │MISO: 50 │
│      │SCK: 52  │
│      │CS: 53   │
└──────┴─────────┘
note: the MISO pin is not used in this sketch
*/

#define TFT_DC 48    //can be any pin
#define TFT_CS 53    //refer to chart
#define TFT_MOSI 51  //refer to chart
#define TFT_SCLK 52  //refer to chart
#define TFT_RST 49   //can be any pin
//also ensure that VCC and LED are connected to a 5V source, and GND is connected.

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//color (RGB565)
#define RED 0xFAEC
#define BLUE 0xD6BF
#define PURPLE 0xFEBF
//TODO: not sure if built in magenta is better or if the purple is better
#define LIME 0x87F0

//If you arent able to use the designated SPI pins for your board, use the following instead:
//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//---------------------------------------------------------------------------------
//BME680
//running on I2C protocol
float bme_IAQ;
float bme_pressure; //note: sensor measures station pressure in pascals
String output; //TODO:not sure if we should keep this

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);

// Create an object of the class Bsec
Bsec BME;

//graph variables
// IAQ=50 corresponds to typical good air and IAQ=200 indicates typical polluted air.
const int minIaq = 0;
const int maxIaq = 200;

const int minPressure =86;//kpa
const int  maxPressure = 91;//kpa





//---------------------------------------------------------------------------------
//theremistor code
// pins for ntc thermistor
#define ntcInput A1  // analog read pin connects to rail with thermistor and resistor
//5v power goes to  thermistor only rail
// GND pin connects to resistor only rail

// thermistor variables
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float temperature;


//---------------------------------------------------------------------------------
//Humidity Sensor
#define DHTTYPE DHT22   //creates dht object
#define DHT22DataPin 8  //defines arduino pin
DHT_Unified dht(DHT22DataPin, DHTTYPE);
float humidity;
sensors_event_t event;

//---------------------------------------------------------------------------------
//PMS 7003
//particulate matter sensor
PMS pms(Serial1);
PMS::DATA data;
//note data is returned as unsigned 16 bit int
//TODO: use the proper data type
int pm25;
int pm10;

//ug/m3
int minPm25 = 0;
int maxPm25 = 60;

int minPm10 = 0;
int maxPm10 = 100;

//---------------------------------------------------------------------------------
//graphcode
// Define variables for line graph
//TODO: figure out if there is extra pixels and utalize them
const int displayWidth = 240;   //x
const int displayHeight = 320;  //y

const int graphWidth = 180;  //px
const int graphHeight = 70;  //px

const int graphXPos = 30;
const int graphTopYPos = 15;

//offsets for middle and bottom graphs
const int graphMiddleYPos = 107;
const int graphBottomYPos = 214;

//TODO:move these to their respective sections
const int minTemp = 15;
const int maxTemp = 26;

const int minHumidity = 25;
const int maxHumidity = 75;


int x;
int y;


bool valueError = false;

//---------------------------------------------------------------------------------
//time module
//uRTCLib rtc(0x68);
// set up for I2C
uRTCLib rtc;

int currentMinute;
int previousMinute;

int pixelPos;
int previousPixelPos;
int counter;

//TODO:perhaps move these some where else, doesnt make sense its with the RTC section
//temp variables
float totalTempReadings;
float averageTempReading;
float previousaverageTempReading;

// float bmeTotalTempReadings;
// float bmeAverageTempReading;
// float bmePreviousaverageTempReading;

float totalIAQReadings;
float averageIAQReading;
float previousAverageIAQReading;
int mapMiddleIAQ;
int mapMiddlePreviousIAQ;

float totalPressureReadings;
float averagePressureReading;
float previousAveragePressureReading;
int mapMiddlePressure;
int mapMiddlePreviousPressure;

float totalPM25Readings;//TODO:double check is we double cap "PM" or "Pm", anyways keep it consistent
float averagePM25Reading;
float previousAveragePM25Reading;
int mapBottomPM25;
int mapBottomPreviousPM25;

float totalPM10Readings;
float averagePM10Reading;
float previousAveragePM10Reading;
int mapBottomPM10;
int mapBottomPreviousPM10;




//TODO: do these really need to be global variables?
int mapTopTemp;
int mapTopPreviousTemp;

// int mapBmeTopTemp;
// int mapBmeTopPreviousTemp;

int mapMiddleTemp;
int mapMiddlePreviousTemp;

//Humidity variables
float totalHumidityReadings;
float averageHumidityReading;
float previousaverageHumidityReading;



int mapTopHumidity;
int mapTopPreviousHumidity;



int mapMiddleHumidity;
int mapMiddlePreviousHumidity;


//---------------------------------------------------------------------------------




void setup() {
  Serial.begin(115200);   //for serial monitor
   Serial1.begin(9600);  //for PMS
  delay(3000);  // wait for console opening
  Serial.println("Setup Begin");

  dht.begin();

  URTCLIB_WIRE.begin();
rtc.set_rtc_address(0x68);
rtc.set_model(URTCLIB_MODEL_DS3231);
  // Following line sets the RTC with an explicit date & time
  // for example to set January 13 2022 at 12:15 you would call://TODO:
  // Comment out below line once there is a battery in module, and you set the date & time
  //rtc.set(20, 26, 18, 2, 15, 5, 23);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)


//PMS7003
if (pms.read(data)){
  previousAveragePM25Reading = data.PM_AE_UG_2_5;
  previousAveragePM10Reading = data.PM_AE_UG_10_0;
}else{
  Serial.println("ERROR: pms not reading");
}

  //BME680
 pinMode(LED_BUILTIN, OUTPUT); //onboard led
  BME.begin(BME68X_I2C_ADDR_HIGH, Wire);
    checkIaqSensorStatus();
      // mouse over these to see description
      //TODO:we can probably remove some of these as we are only interested in iaq and pressure
  bsec_virtual_sensor_t sensorList[13] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE
  };
  BME.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
  if (BME.run()){
    previousAverageIAQReading = BME.iaq;
    previousAveragePressureReading = BME.pressure/1000;
  }


  //thermistor
  pinMode(ntcInput, INPUT);  // analog

  

  rtc.refresh();
  currentMinute = rtc.minute();
  previousMinute = rtc.minute();

  pixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  previousPixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  //TODO: could we just initialize these vairiables at zero;
  counter = 0;
  totalTempReadings = 0;
  totalHumidityReadings = 0;
  totalIAQReadings = 0;
  totalPressureReadings = 0;
  totalPM25Readings = 0;
  totalPM10Readings = 0;

//temperature
  R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
  logR2 = log(R2);
  temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
  //rounds temperature to one decimal place.
  previousaverageTempReading = round(temperature * 10) / 10.0;


 // Get humidity event
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
 previousaverageHumidityReading = event.relative_humidity;
  }



  //display
  tft.init(240, 320);


  //make the display show the proper colors
  tft.invertDisplay(0);

  // Clear screen and draw graph axes
  tft.fillScreen(ST77XX_BLACK);
  //-----------------------
  //draw temp axis (top left axis)
  tft.drawFastVLine(graphXPos - 3, graphTopYPos, graphHeight, ST77XX_GREEN);
  drawScalePoint(15, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, false, true);
  drawScalePoint(16, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);
  drawScalePoint(18, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);
  drawScalePoint(20, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);
  drawScalePoint(22, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);
  drawScalePoint(24, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);
  drawScalePoint(26, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true);

  //middle left axis
  //TODO:we might need to do something about 3 digit numbers being to far to the right, on the left side
   tft.drawFastVLine(graphXPos - 3, graphMiddleYPos, graphHeight, ST77XX_YELLOW);
  drawScalePoint(0, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, false, true);
  drawScalePoint(30, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(60, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(90, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(120, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(150, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(170, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(200, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);


   tft.drawFastVLine(graphXPos + graphWidth + 3, graphMiddleYPos, graphHeight, PURPLE);
   drawScalePoint(86, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);//TODO:theses are not working for some reason, displaying wrong number
   drawScalePoint(87, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
   drawScalePoint(88, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
   drawScalePoint(89, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
   drawScalePoint(90, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
   drawScalePoint(91, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);


  //bottom left axis
  //tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, BLUE);
   drawDualScalePoint(0, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(10, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(20, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(30, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(40, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(50, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(60, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);

  //test Double axis
  tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, ST77XX_CYAN);
   drawDualScalePoint(0, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(20, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(40, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(60, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(80, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(100, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   




  //top right axis
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphTopYPos, graphHeight, ST77XX_CYAN);
  drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);
  drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);


//TODO: figure out typical co2 ranges and build scale
  //bottom right axis
  // tft.drawFastVLine(graphXPos + graphWidth + 3, graphBottomYPos, graphHeight, RED);
  // drawScalePoint(25, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, false, false);
  // drawScalePoint(100, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  //   drawScalePoint(175, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  // drawScalePoint(250, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  // drawScalePoint(325, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  // drawScalePoint(400, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  // drawScalePoint(475, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  // drawScalePoint(500, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, false, false);


  //draw time axis
  tft.drawFastHLine(graphXPos, graphTopYPos + graphHeight + 1, graphWidth, ST77XX_WHITE);
  tft.drawFastHLine(graphXPos, graphTopYPos + graphHeight + 16, graphWidth, ST77XX_WHITE);
  drawTimeScalePoint(0, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(1, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(2, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(3, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(4, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(5, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(6, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(7, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(8, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(9, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(10, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(11, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(12, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(13, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(14, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(15, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(16, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(17, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(18, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(19, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(20, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(21, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);
  drawTimeScalePoint(22, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(23, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, false, true);
  drawTimeScalePoint(24, 0, 24, graphXPos, graphXPos + graphWidth, graphTopYPos, ST77XX_WHITE, true, true);


  //draw bottom axis
  tft.drawFastHLine(graphXPos, graphBottomYPos + graphHeight + 1, graphWidth, ST77XX_WHITE);
  drawTimeScalePoint(0, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(1, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(2, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(3, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(4, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(5, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(6, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(7, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(8, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(9, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(10, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(11, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(12, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(13, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(14, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(15, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(16, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(17, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(18, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(19, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(20, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(21, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);
  drawTimeScalePoint(22, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(23, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, false, false);
  drawTimeScalePoint(24, 0, 24, graphXPos, graphXPos + graphWidth, graphBottomYPos, ST77XX_WHITE, true, false);

  Serial.println("Setup Complete");
}

void loop() {
  // put your main code here, to run repeatedly:

  //Temp
  R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
  logR2 = log(R2);
  temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
  //rounds temperature to one decimal place.
  temperature = round(temperature * 10) / 10.0;


//BME
  if (BME.run()) { 
  bme_IAQ = BME.iaq;
  bme_pressure = BME.pressure/1000;
  }else {
       checkIaqSensorStatus();
  }

//PMS
//TODO:check wiring. it was working fine but then i think i bumped the bread board and now its not reading. but it was working
// if (pms.read(data)){
//   pm25 = data.PM_AE_UG_2_5;
//   pm10 = data.PM_AE_UG_10_0;
// }else {
//   Serial.println("error: unable to read pms");
// }


  //Humidity
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
 humidity = event.relative_humidity;
  }





  // Scale value to fit graph
  //https://www.youtube.com/watch?v=j0o5EGeShME
  /*
    map(
      input value,
      from input value low,
      from input value high,
      to output value low,
      to output value high)
    */


  rtc.refresh();
  pixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  currentMinute = rtc.minute();

  //moving time bar that pans left to right with time and erases the old data
  if (pixelPos != previousPixelPos) {
    //erases older lines top graph
    tft.drawFastVLine(graphXPos + pixelPos + 2, graphTopYPos, graphHeight, ST77XX_WHITE);
    tft.drawRect(graphXPos + pixelPos, graphTopYPos, 2, graphHeight + 1, ST77XX_BLACK);
    //erases older lines middle graph
    tft.drawFastVLine(graphXPos + pixelPos + 2, graphMiddleYPos, graphHeight, ST77XX_WHITE);
    tft.drawRect(graphXPos + pixelPos, graphMiddleYPos, 2, graphHeight + 1, ST77XX_BLACK);
    //erase older lines for bottom graph
        tft.drawFastVLine(graphXPos + pixelPos + 2, graphBottomYPos, graphHeight, ST77XX_WHITE);
    tft.drawRect(graphXPos + pixelPos, graphBottomYPos, 2, graphHeight + 1, ST77XX_BLACK);


    if (pixelPos == 0) {
      //erases the white line scrub line from right of graph
      //top
      tft.drawFastVLine(graphXPos + graphWidth + 2, graphTopYPos, graphHeight, ST77XX_BLACK);
      tft.drawFastVLine(graphXPos + graphWidth + 1, graphTopYPos, graphHeight, ST77XX_BLACK);
//middle
      tft.drawFastVLine(graphXPos + graphWidth + 2, graphMiddleYPos, graphHeight, ST77XX_BLACK);
      tft.drawFastVLine(graphXPos + graphWidth + 1, graphMiddleYPos, graphHeight, ST77XX_BLACK);
//bottom
      tft.drawFastVLine(graphXPos + graphWidth + 2, graphBottomYPos, graphHeight, ST77XX_BLACK);
      tft.drawFastVLine(graphXPos + graphWidth + 1, graphBottomYPos, graphHeight, ST77XX_BLACK);
    }
    if (previousPixelPos > pixelPos) {
      //this is for the 24 hour role over
      previousPixelPos = 0;
    }



    //get the average reading
    averageTempReading = (float)totalTempReadings / (float)counter;          
    averageHumidityReading = (float)totalHumidityReadings / (float)counter;  
averageIAQReading = (float)totalIAQReadings / (float)counter;
averagePressureReading = (float) totalPressureReadings / (float) counter;
averagePM25Reading = (float) totalPM25Readings / (float) counter;
averagePM10Reading = (float) totalPM10Readings / (float) counter;






    //safty limit on  temp value
    if (averageTempReading > maxTemp) {
      averageTempReading = maxTemp;
    } else if (averageTempReading < minTemp) {
      averageTempReading = minTemp;
    }
    //safty limit on  Humidity value
    if (averageHumidityReading > maxHumidity) {
      averageHumidityReading = maxHumidity;
    } else if (averageHumidityReading < minHumidity) {
      averageHumidityReading = minHumidity;
    }

    //TODO:safety limit for iaq and pressure, pm



    //get values for graphing
    //temp for top gragh
    mapTopTemp = mapf(averageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
    mapTopPreviousTemp = mapf(previousaverageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
      //draw top graph temp
    tft.drawLine(graphXPos + previousPixelPos, mapTopPreviousTemp, graphXPos + pixelPos, mapTopTemp, ST77XX_GREEN);
    previousaverageTempReading = averageTempReading;
    totalTempReadings = 0;

//map and draw humidity on top graph
    mapTopHumidity = mapf(averageHumidityReading, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos);
    mapTopPreviousHumidity = mapf(previousaverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapTopPreviousHumidity, graphXPos + pixelPos, mapTopHumidity, ST77XX_CYAN);
              previousaverageHumidityReading = averageHumidityReading;
    totalHumidityReadings = 0;

//map and draw IAQ on middle graph
    mapMiddleIAQ = mapf(averageIAQReading, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousIAQ = mapf(previousAverageIAQReading, minIaq, maxIaq, graphHeight + graphMiddleYPos, graphMiddleYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousIAQ, graphXPos+ pixelPos, mapMiddleIAQ,ST77XX_YELLOW );
    previousAverageIAQReading = averageIAQReading;
    totalIAQReadings = 0;

//map and draw pressure on middle graph
        mapMiddlePressure = mapf(averagePressureReading, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousPressure = mapf(previousAveragePressureReading, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousPressure, graphXPos+ pixelPos, mapMiddlePressure,PURPLE);
    previousAveragePressureReading = averagePressureReading;
    totalPressureReadings = 0;

//map and draw pm 2.5 on bottom graph
mapBottomPM25 = mapf(averagePM25Reading, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos);
mapBottomPreviousPM25 = mapf(previousAveragePM25Reading, minPm25, maxPm25, graphHeight + graphBottomYPos, graphBottomYPos);
tft.drawLine(graphXPos + previousPixelPos, mapBottomPreviousPM25, graphXPos + pixelPos, mapBottomPM25, BLUE);
previousAveragePM25Reading = averagePM25Reading;
totalPM25Readings = 0;

//map and draw pm 10 on bottom graph
mapBottomPM10 = mapf(averagePM10Reading, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos);
mapBottomPreviousPM10 = mapf(previousAveragePM10Reading, minPm10, maxPm10, graphHeight + graphBottomYPos, graphBottomYPos);
tft.drawLine(graphXPos + previousPixelPos, mapBottomPreviousPM10, graphXPos + pixelPos, mapBottomPM10, BLUE);
previousAveragePM10Reading = averagePM10Reading;
totalPM10Readings = 0;


//bottom graph
//voc
// mapVoc = mapf(averageVoc, minVoc, maxVoc, graphHeight+ graphBottomYPos, graphBottomYPos);
// mapPreviousVoc = mapf(averagePreviousVoc, minVoc, maxVoc, graphHeight+ graphBottomYPos, graphBottomYPos);
// tft.drawLine(graphXPos+previousPixelPos, mapPreviousVoc, graphXPos +pixelPos, mapVoc,RED );
// averagePreviousVoc = averageVoc;
// totalVoc = 0;






    previousPixelPos = pixelPos;
    counter = 0;
  }

//add to value if its not time to draw
  if (currentMinute != previousMinute) {
    previousMinute = currentMinute;
    counter++;
    totalTempReadings += temperature;
    totalHumidityReadings += humidity;
    totalIAQReadings += bme_IAQ;
    totalPressureReadings +=bme_pressure;
    totalPM25Readings += pm25;
    totalPM10Readings += pm10;
  }



      //---------------------------------


  //---------------------------------
  //live read out of sensors
  //TOP
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(5, 0);

  tft.print("Temp:");
  tft.print(temperature);
  tft.print("c");

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(85, 0);
  tft.print(rtc.hour());
  tft.print(":");
  tft.print(rtc.minute());
  tft.print(":");
  tft.print(rtc.second());
  tft.println("   ");//TODO:???i cant remember what this is for

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(145, 0);
  tft.print("Humidity:");
  tft.print(humidity);
  tft.print("%");


  //MIDDLE
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(5, 180);

  tft.print("IAQ:");
  tft.print(bme_IAQ);
  

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(85, 180);
  tft.print(rtc.hour());
  tft.print(":");
  tft.print(rtc.minute());
  tft.print(":");
  tft.print(rtc.second());
  tft.println("   ");//TODO:??

  tft.setTextColor(PURPLE, ST77XX_BLACK);
  tft.setCursor(145, 180);
  tft.print("Pressure:");
  tft.print(bme_pressure);
  tft.print("");//TODO:

  //BOTTOM
  //TODO:test spacing for pm2.5 pm10 and co2
  tft.setTextSize(1);
  tft.setTextColor(BLUE, ST77XX_BLACK);
  tft.setCursor(5, 190);

  tft.print("PM2.5:");
  tft.print(pm25);
  //TODO:testing remove
  //   Serial.print("PM 2.5 (ug/m3): ");
  // Serial.println(pm25);



  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(75, 190);

  tft.print("PM10:");
  tft.print(pm10);
      // Serial.print("PM 10.0  (ug/m3): ");
      //   Serial.println(pm10);




  tft.setTextColor(RED, ST77XX_BLACK);
  tft.setCursor(145, 190);
  tft.print("CO2:");
  tft.print("200");
  


  //Serial.println("Loop Complete");
  delay(1000);  //add a one second delay
}

//draw the y-axis
void drawScalePoint(int scalePoint, int yMin, int yMax, int yStart, int yEnd, unsigned int color, bool drawLabel, bool onLeft) {
  y = map(scalePoint, yMin, yMax, yStart, yEnd);
    if (onLeft) {
      tft.drawFastHLine(22, y, 6, color);
    } else {
      tft.drawFastHLine(213, y, 6, color);
    }
    if (drawLabel == true) {
      tft.setTextColor(color);
      if (onLeft) {
        tft.setCursor(10, y - 3);  //+3 as text starts at top left and text is ?? px tall and we need to raise it higher
      } else {
        tft.setCursor(220, y - 3);
      }

      tft.print(scalePoint);
    }
}

///draw two axis on the left side
///isLeftSide is the left side of the double axis
void drawDualScalePoint(int scalePoint, int yMin, int yMax, int yStart, int yEnd, unsigned int color, bool drawLabel, bool isLeftSide ) {
  //
  y = map(scalePoint, yMin, yMax, yStart, yEnd);
if(!isLeftSide){
       tft.drawFastHLine(25, y, 2, color);
}
    if (drawLabel == true) {
      if (isLeftSide){
      tft.setTextColor(color);
        tft.setCursor(0, y - 3);  //+3 as text starts at top left and text is ?? px tall and we need to raise it higher
      tft.print(scalePoint);
    }else{
            tft.setTextColor(color);
        tft.setCursor(13, y - 3);  //+3 as text starts at top left and text is ?? px tall and we need to raise it higher
      tft.print(scalePoint);
    }}
}


//draw the time axis (x-axis)
void drawTimeScalePoint(int timePoint, int xMin, int xMax, int xStart, int xEnd, int yPos, unsigned int color, bool drawLabel, bool drawBottomScale) {
  x = map(timePoint, xMin, xMax, xStart, xEnd);
  tft.drawFastVLine(x, yPos + graphHeight + 1, 3, color);
  if (drawBottomScale) {
    tft.drawFastVLine(x, yPos + graphHeight + 13, 3, color);
  }
  if (drawLabel) {
    tft.setTextColor(color);
    if (timePoint > 9) {  //label contains two digits off set it so its center still
      tft.setCursor(x - 5, yPos + graphHeight + 5);
    } else {
      tft.setCursor(x - 2, yPos + graphHeight + 5);
    }
    tft.print(timePoint);
  }
}

//like the map function but accommodates floats
float mapf(float value, int inputMin, int inputMax, int outputMin, int outputMax) {

  return (value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin;
}

// Helper function for bme
void checkIaqSensorStatus(void)
{
  if (BME.bsecStatus != BSEC_OK) {
    if (BME.bsecStatus < BSEC_OK) {
      String output = "BSEC error code : " + String(BME.bsecStatus);
      Serial.println(output);//TODO:combine this and the print statement?
      for (;;)//TODO:for loop runs forever
        errLeds(); /* Halt in case of failure */
    } else {
      String output = "BSEC warning code : " + String(BME.bsecStatus);//TODO:combine this and the print statement?
      Serial.println(output);
    }
  }


  if (BME.bme68xStatus != BME68X_OK) {
    if (BME.bme68xStatus < BME68X_OK) {
     output = "BME68X error code : " + String(BME.bme68xStatus);
      //-2 is communication failure
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME68X warning code : " + String(BME.bme68xStatus);
      Serial.println(output);
    }
  }
}

// Helper function for bme
//TODO: hmmm might be a good way to determine error states . . .
//TODO: may have to do some this in other places as it crashe more often then i would like
void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

