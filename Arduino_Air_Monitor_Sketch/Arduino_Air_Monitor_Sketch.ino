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
/*
/ note: I couldn't get the bsec.h library to work with IAQ for some reason
/ so I am using adafruit library instead. If you get it bsec.h library working
/ please let me know.  
*/
#include "Adafruit_BME680.h"
#include <CRCx.h>  //https://github.com/hideakitai/CRCx

// UNCOMMENT FOR DEBUG MODE
//#define DEBUG

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

//graphcode
// Define variables for line graph
//TODO: figure out if there is extra pixels and utalize them
const int displayWidth = 240;   //x
const int displayHeight = 320;  //y

const int graphWidth = 180;  //px
const int graphHeight = 70;  //px

const int graphXPos = 30;

//offsets for graphs
const int graphTopYPos = 15;
const int graphMiddleYPos = 107;
const int graphBottomYPos = 214;

int x;
float y;

int pixelPos;
int previousPixelPos;

//---------------------------------------------------------------------------------
//BME680
//running on I2C protocol
float bme_IAQ;
float bme_pressure;  //note: sensor measures station pressure in pascals.
float offset;        //offset has been calculated by taking the difference in the station pressure reading from
//the BME680 and the barometric pressure from the nearest METAR weather monitoring station
String output;  //TODO:not sure if we should keep this

// Helper functions declarations
void errLeds(void);

Adafruit_BME680 bme;  // I2C

float totalIAQReadings;
float averageIAQReading;
float previousAverageIAQReading;
int mapBottomIAQ;
int mapBottomPreviousIAQ;

float totalPressureReadings;
float averagePressureReading;
float previousAveragePressureReading;
int mapMiddlePressure;
int mapMiddlePreviousPressure;

//graph variables
// using gas resistance. This is a relative scale and resistance will increase slowly over time.
// With low resistance indicating the VOC.
//TODO: remane iaq to gasRes
const int minIaq = 0;
const int maxIaq = 1600;

const int minPressure = 86;  //kpa
const int maxPressure = 91;  //kpa

uint8_t lastBmeReading = 0;
const byte bmeReadingInterval = 4;


//---------------------------------------------------------------------------------
//theremistor code
// pins for ntc thermistor
#define ntcInput A3  // analog read pin connects to rail with thermistor and resistor
//5v power goes to  thermistor only rail
// GND pin connects to resistor only rail

// thermistor variables
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float temperature;

const int minTemp = 15;
const int maxTemp = 26;

uint8_t lastTempReading = 0;
const byte tempReadingInterval = 1;

//temp variables
float totalTempReadings;
float averageTempReading;
float previousaverageTempReading;

int mapTopTemp;
int mapTopPreviousTemp;


//---------------------------------------------------------------------------------
//Humidity Sensor
#define DHTTYPE DHT22   //creates dht object
#define DHT22DataPin 8  //defines arduino pin
DHT_Unified dht(DHT22DataPin, DHTTYPE);
float humidity;
sensors_event_t event;

//Humidity variables
float totalHumidityReadings;
float averageHumidityReading;
float previousaverageHumidityReading;

uint8_t lastHumidityReading = 0;
byte humidityReadingInterval = 2;

int mapTopHumidity;
int mapTopPreviousHumidity;

const int minHumidity = 25;
const int maxHumidity = 75;
//---------------------------------------------------------------------------------
//PMS 7003
//particulate matter sensor
//PMS pms(Serial1);
PMS pms(Serial3);
PMS::DATA data;
//note data is returned as unsigned 16 bit int
//TODO: use the proper data type
int pm25;
int pm10;

//ug/m3
int minPm25 = 1;
int maxPm25 = 1000;

int minPm10 = 1;
int maxPm10 = 1000;

float totalPM25Readings;  //TODO:double check is we double cap "PM" or "Pm", anyways keep it consistent
float averagePM25Reading;
float previousAveragePM25Reading;
int mapMiddlePM25;
int mapMiddlePreviousPM25;


float totalPM10Readings;
float averagePM10Reading;
float previousAveragePM10Reading;
int mapMiddlePM10;
int mapMiddlePreviousPM10;

uint8_t lastPmsReading = 0;
const byte pmsReadingInterval = 3;


//---------------------------------------------------------------------------------
//SenseAir S8
//alot of the code for the SenseAir S8 was pulled from Daniel Bernal LibreCO2 project

#define co2sensor Serial2

const byte MB_PKT_7 = 7;  //MODBUS Packet Size
const byte MB_PKT_8 = 8;  //MODBUS Packet Size

// SenseAir S8 MODBUS commands
const byte cmdReSA[MB_PKT_8] = { 0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5 };  // SenseAir Read CO2
const byte cmdOFFs[MB_PKT_8] = { 0xFE, 0x06, 0x00, 0x1F, 0x00, 0x00, 0xAC, 0x03 };  // SenseAir Close ABC
static byte response[MB_PKT_8] = { 0 };

byte VALalti = 21;  //TODO:calgary is 1045m elevation, as 1045/50 = 20.9
//we might not even need this if we pull hpa form the bme sensor
byte ConnRetry = 0;
int CO2 = 0;
int CO2value;
unsigned int crc_cmd;
float CO2cor;
float hpa;

uint8_t lastS8Reading = 0;
const byte S8ReadingInterval = 4;

float totalCO2Readings;
float averageCO2Reading;
float previousAverageCO2Reading;
int mapBottomCO2;
int mapBottomPreviousCO2;

int minCO2 = 400;
int maxCO2 = 2000;


//---------------------------------------------------------------------------------
//time module
uRTCLib rtc;

int currentMinute;
int previousMinute;

int counter;

//---------------------------------------------------------------------------------


void setup() {
  Serial.begin(115200);  //for serial monitor
  Serial2.begin(9600);   // for CO2 sensor
  //delay(1000);  // wait for console opening
  Serial.println("Setup Begin");

  dht.begin();
  CO2iniSenseAir();
  Wire.begin();
  Wire.setClock(100000);
  delay(200);
  URTCLIB_WIRE.begin();
  rtc.set_rtc_address(0x68);
  rtc.set_model(URTCLIB_MODEL_DS3231);
  // Following line sets the RTC with an explicit date & time
  // for example to set January 13 2023 at 12:15 you would call:
  //rtc.set(0, 15, 12, 6, 13, 1, 23);
  // Comment out below line once there is a battery in module, and you set the date & time
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)
  //rtc.set(0,22,21,2,12,2,24);//comment out the rtc.set line and re-upload sketch to not overwrite the date/time each time the arduino starts up.

  //real time clock
  rtc.refresh();
  currentMinute = rtc.minute();
  previousMinute = rtc.minute();

  //set the pixel position based on the time, note the /8 is due to the resolution of the screen.
  //ie: 1440 minutes in a day divided by the the pixel width of the graph (180 px) => 8 minutes per pixel.
  //TODO: fix hard coding
  pixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  previousPixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;

  counter = 0;
  totalTempReadings = 0;
  totalHumidityReadings = 0;
  totalIAQReadings = 0;
  totalPressureReadings = 0;
  totalPM25Readings = 0;
  totalPM10Readings = 0;
  totalCO2Readings = 0;


  //display
  tft.init(240, 320);
  tft.setRotation(2);
  //make the display show the proper colors
  tft.invertDisplay(0);
  // Clear screen and draw graph axes
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(5, 0);
  tft.print("initializing");


  //display start up sequence
  Serial.print(F("Preheat: "));
  for (int i = 30; i > -1; i--) {  // Preheat from 0 to 30
    delay(1000);
    //display initialization count down
    //sensors need to warm up to operating temp (mainly S8)
    delay(1000);
    tft.setCursor(5, 25);
    tft.print("Start up in:");
    tft.print(i);
    tft.print("  ");
    //display set time
    tft.setCursor(5, 50);
    rtc.refresh();
    tft.print(rtc.hour());
    tft.print(":");
    tft.print(rtc.minute());
    tft.print(":");
    tft.print(rtc.second());
    tft.println("  ");
    i--;
  }

  Serial.print(F("Start measurements compensated by Altitude: "));
  Serial.print(VALalti * 50);
  Serial.println(" m");
  delay(5000);

  CO2 = 0;
  CO2value = co2SenseAir();
  hPaCalculation();
  CO2cor = float(CO2value) + (0.016 * ((1013 - float(hpa)) / 10) * (float(CO2value) - 400));  //adjusts for altitude above sea level
  previousAverageCO2Reading = round(CO2cor);
  lastS8Reading = rtc.second();




  //BME680
  pinMode(LED_BUILTIN, OUTPUT);  //onboard led
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
      ;
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms


  previousAverageIAQReading = bme.gas_resistance / 1000.0;
  previousAveragePressureReading = bme.pressure / 1000.0;



  //thermistor
  pinMode(ntcInput, INPUT);  // analog


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
  } else {
    previousaverageHumidityReading = event.relative_humidity;
  }

  Serial3.begin(9600);  //for pms sensor
  //PMS7003
  if (pms.read(data)) {
    previousAveragePM25Reading = data.PM_AE_UG_2_5;
    previousAveragePM10Reading = data.PM_AE_UG_10_0;
  } else {
    Serial.println("ERROR: pms not reading in setup");
  }


  //-----------------------
  tft.setTextSize(1);
  tft.fillScreen(ST77XX_BLACK);
  //draw temperature axis (top left axis)
  tft.drawFastVLine(graphXPos - 3, graphTopYPos, graphHeight, ST77XX_GREEN);
  drawScalePoint(15, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, false, true, false);
  drawScalePoint(16, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);
  drawScalePoint(18, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);
  drawScalePoint(20, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);
  drawScalePoint(22, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);
  drawScalePoint(24, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);
  drawScalePoint(26, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos, ST77XX_GREEN, true, true, false);

  //draw humidity axis (top right axis)
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphTopYPos, graphHeight, ST77XX_CYAN);
  drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false, false);
  drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false, false);
  drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false, false);
  drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false, false);
  drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false, false);
  drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false, false);
  drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false, false);

  //draw PM2.5/PM10 axis on log graph (middle left  axis)
  tft.drawFastVLine(graphXPos - 3, graphMiddleYPos, graphHeight, BLUE);
  drawScalePoint(1, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(2, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(5, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(10, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(20, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(50, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(100, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(200, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(500, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);
  drawScalePoint(1000, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos, BLUE, true, true, true);


  //draw pressure axis (middle right axis)
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphMiddleYPos, graphHeight, PURPLE);
  drawScalePoint(86, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, false, false, false);
  drawScalePoint(87, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false, false);
  drawScalePoint(88, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false, false);
  drawScalePoint(89, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false, false);
  drawScalePoint(90, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false, false);
  drawScalePoint(91, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false, false);

  //draw gas resistance axis (bottim left axis)
  tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, RED);
  drawScalePoint(0, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, false, true, false);
  drawScalePoint(200, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(400, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(600, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(800, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(1000, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(1200, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(1400, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);
  drawScalePoint(1600, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, true, false);

  //draw CO2 axis (bottom right axis)
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphBottomYPos, graphHeight, ST77XX_YELLOW);
  drawScalePoint(400, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(600, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(800, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(1000, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(1200, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(1400, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(1600, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(1800, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);
  drawScalePoint(2000, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_YELLOW, true, false, false);


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


  //draw lower time axis
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
#ifdef DEBUG
  Serial.println("Setup Complete");
#endif
}

void loop() {
  //Temp
  if ((rtc.second() > tempReadingInterval) && (abs(rtc.second() - lastTempReading) > tempReadingInterval)) {
    R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
    logR2 = log(R2);
    temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
    //rounds temperature to one decimal place.
    temperature = round(temperature * 10) / 10.0;
    lastTempReading = rtc.second();
  }

  //BME 680
  //hmm i wonder if I need to await the bsec.h iaq read . . .
  //TODO: In the future switch the libraries and try it
  if ((rtc.second() > bmeReadingInterval) && (abs(rtc.second() - lastBmeReading) > bmeReadingInterval)) {
    if (bme.performReading()) {
#ifdef DEBUG
      Serial.print("Temp: ");
      Serial.print(bme.temperature);
      Serial.print("  Pressure: ");
      Serial.print(bme.pressure);
      Serial.print("  Gas_res:");
      Serial.println(bme.gas_resistance);
#endif
      bme_IAQ = bme.gas_resistance / 1000.0;
      bme_pressure = bme.pressure / 1000.0;
    } else {
      Serial.println("Failed to perform reading ");
    }
    lastBmeReading = rtc.second();
  }

  //PMS
  if ((rtc.second() > pmsReadingInterval) && (abs(rtc.second() - lastPmsReading) > pmsReadingInterval)) {
    if (pms.readUntil(data)) {
      pm25 = data.PM_AE_UG_2_5;
      pm10 = data.PM_AE_UG_10_0;
#ifdef DEBUG
      Serial.println("reading PMS");
      Serial.print("succesful read pms 2.5:");
      Serial.print(pm25);
      Serial.print("  pm10:");
      Serial.println(pm10);
#endif

    } else {
      Serial.println("error: unable to read pms");
    }
    lastPmsReading = rtc.second();
  }

  //S8
  if ((rtc.second() > S8ReadingInterval) && (abs(rtc.second() - lastS8Reading) > S8ReadingInterval)) {
    //S8 can only take a reading once every 4 seconds
    CO2value = co2SenseAir();
#ifdef DEBUG
    Serial.print("co2value:");
    Serial.println(CO2value);
#endif
    hPaCalculation();
    CO2cor = float(CO2value) + (0.016 * ((1013 - float(hpa)) / 10) * (float(CO2value) - 400));  // Increment of 1.6% for every hPa of difference at sea level
    previousAverageCO2Reading = round(CO2cor);
    lastS8Reading = rtc.second();
  }


  //Humidity
  if ((rtc.second() > humidityReadingInterval) && (abs(rtc.second() - lastHumidityReading) > humidityReadingInterval)) {
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    } else {
      humidity = event.relative_humidity;
#ifdef DEBUG
      Serial.print("humidity:");
      Serial.println(humidity);
#endif
    }
    lastHumidityReading = rtc.second();
  }



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
    averagePressureReading = (float)totalPressureReadings / (float)counter;
    averagePM25Reading = (float)totalPM25Readings / (float)counter;
    averagePM10Reading = (float)totalPM10Readings / (float)counter;
    averageCO2Reading = (float)totalCO2Readings / (float)counter;



    //TODO: change all these to call the constraint(x,a, b) function instead of this

    //overflow/underflow limit on  temp value
    if (averageTempReading > maxTemp) {
      averageTempReading = maxTemp;
    } else if (averageTempReading < minTemp) {
      averageTempReading = minTemp;
    }
    //overflow/underflow limit on  Humidity value
    if (averageHumidityReading > maxHumidity) {
      averageHumidityReading = maxHumidity;
    } else if (averageHumidityReading < minHumidity) {
      averageHumidityReading = minHumidity;
    }
    //overflow/underflow limit on  pressure value
    if (averagePressureReading > maxPressure) {
      averagePressureReading = maxPressure;
    } else if (averagePressureReading < minPressure) {
      averagePressureReading = minPressure;
    }
    //overflow/underflow limit on  pm2.5 value
    if (averagePM25Reading > maxPm25) {
      averagePM25Reading = maxPm25;
    } else if (averagePM25Reading < minPm25) {
      averagePM25Reading = minPm25;
    }
    //overflow/underflow limit on  pm10 value
    if (averagePM10Reading > maxPm10) {
      averagePM10Reading = maxPm10;
    } else if (averagePM10Reading < minPm10) {
      averagePM10Reading = minPm10;
    }
    //overflow/underflow limit on  gas resistance value
    if (averageIAQReading > maxIaq) {
      averageIAQReading = maxIaq;
    } else if (averageIAQReading < minIaq) {
      averageIAQReading = minIaq;
    }
    //overflow/underflow limit on  CO2 value
    if (averageCO2Reading > maxCO2) {
      averageCO2Reading = maxCO2;
    } else if (averageCO2Reading < minCO2) {
      averageCO2Reading = minCO2;
    }


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


    //map and draw pressure on middle graph
    mapMiddlePressure = mapf(averagePressureReading, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousPressure = mapf(previousAveragePressureReading, minPressure, maxPressure, graphHeight + graphMiddleYPos, graphMiddleYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousPressure, graphXPos + pixelPos, mapMiddlePressure, PURPLE);
    previousAveragePressureReading = averagePressureReading;
    totalPressureReadings = 0;

    //map and draw pm 2.5 on bottom graph
    mapMiddlePM25 = mapLog(averagePM25Reading, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousPM25 = mapLog(previousAveragePM25Reading, minPm25, maxPm25, graphHeight + graphMiddleYPos, graphMiddleYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousPM25, graphXPos + pixelPos, mapMiddlePM25, BLUE);
    previousAveragePM25Reading = averagePM25Reading;
    totalPM25Readings = 0;

    //map and draw pm 10 on Middle graph
    mapMiddlePM10 = mapLog(averagePM10Reading, minPm10, maxPm10, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousPM10 = mapLog(previousAveragePM10Reading, minPm10, maxPm10, graphHeight + graphMiddleYPos, graphMiddleYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousPM10, graphXPos + pixelPos, mapMiddlePM10, ST77XX_CYAN);
    previousAveragePM10Reading = averagePM10Reading;
    totalPM10Readings = 0;

    //map and draw IAQ on bottom graph
    mapBottomIAQ = mapf(averageIAQReading, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos);
    mapBottomPreviousIAQ = mapf(previousAverageIAQReading, minIaq, maxIaq, graphHeight + graphBottomYPos, graphBottomYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapBottomPreviousIAQ, graphXPos + pixelPos, mapBottomIAQ, RED);
    previousAverageIAQReading = averageIAQReading;
    totalIAQReadings = 0;

    //map co2 on bottom graph
    mapBottomCO2 = mapf(averageCO2Reading, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos);
    mapBottomPreviousCO2 = mapf(previousAverageCO2Reading, minCO2, maxCO2, graphHeight + graphBottomYPos, graphBottomYPos);
    tft.drawLine(graphXPos + previousPixelPos, mapBottomPreviousCO2, graphXPos + pixelPos, mapBottomCO2, ST77XX_YELLOW);
    previousAverageCO2Reading = averageCO2Reading;
    totalCO2Readings = 0;





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
    totalPressureReadings += bme_pressure;
    totalPM25Readings += pm25;
    totalPM10Readings += pm10;
    totalCO2Readings += CO2cor;
  }



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
  tft.println("  ");

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(145, 0);
  tft.print("Humidity:");
  tft.print(humidity);
  tft.print("%");


  //MIDDLE
  tft.setTextSize(1);
  tft.setTextColor(BLUE, ST77XX_BLACK);
  tft.setCursor(5, 180);

  tft.print("PM2.5:");
  tft.print(pm25);
  tft.println("    ");


  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(85, 180);
  tft.print(rtc.hour());
  tft.print(":");
  tft.print(rtc.minute());
  tft.print(":");
  tft.print(rtc.second());
  tft.println("  ");

  tft.setTextColor(PURPLE, ST77XX_BLACK);
  tft.setCursor(145, 180);
  tft.print("Pressure:");
  tft.print(bme_pressure);


  //BOTTOM
  tft.setTextSize(1);
  tft.setTextColor(RED, ST77XX_BLACK);
  tft.setCursor(72, 190);

  tft.print("GasRes:");
  tft.print(round(bme_IAQ));

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(5, 190);

  tft.print("PM10:");
  tft.print(pm10);
  tft.println("    ");


  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(145, 190);
  tft.print("CO2:");
  tft.print(CO2cor);
}



//draw the y-axis
void drawScalePoint(int scalePoint, int yMin, int yMax, int yStart, int yEnd, unsigned int color, bool drawLabel, bool onLeft, bool isLogGraph) {
  if (isLogGraph) {
    y = mapLog(scalePoint, yMin, yMax, yStart, yEnd);
  } else {
    y = map(scalePoint, yMin, yMax, yStart, yEnd);
  }

  if (onLeft) {
    tft.drawFastHLine(22, y, 6, color);
  } else {
    tft.drawFastHLine(213, y, 6, color);
  }
  if (drawLabel == true) {
    tft.setTextColor(color);
    if (onLeft) {
      if (scalePoint > 99) {
        //if number has 3 digits, set cursor furthere over to the left
        tft.setCursor(5, y - 3);
      } else {
        tft.setCursor(10, y - 3);  //-3 as text starts at top left and text is ?? px tall and we need to raise it higher
      }
    } else {
      tft.setCursor(220, y - 3);
    }

    if (scalePoint > 999) {
      //1000=> 1.0
      tft.print(String(float(scalePoint) / 1000, 1));
    } else {
      tft.print(scalePoint);
    }
  }
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

//remaps a number from a linear scale to a logarithic scale
float mapLog(float value, float yMin, float yMax, float yStart, float yEnd) {
  // showing work
  // a + b * log10(yMin) = yStart
  // a + b * log10(yMax) = yEnd
  // (a + b * log10(yMax)) - (a + b * log10(yMin)) = yEnd - yStart
  // b * (log10(yMax) - log10(yMin)) = yEnd - yStart

  int a;
  int b;

  if (value == 0) {
    //Log10(0) is undefined, therefore we need to set to 1
    value = 1;
  }

  b = (yEnd - yStart) / (log10(yMax) - log10(yMin));
  a = yStart - b * log10(yMin);

  float scaledValue = a + b * log10(value);
  return round(scaledValue);
}

//like the map function but accommodates floats
float mapf(float value, int inputMin, int inputMax, int outputMin, int outputMax) {

  return round((value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin);
}


//TODO: hmmm might be a good way to determine error states . . .
//TODO: may have to do some this in other places as it crashe more often then i would like
void errLeds(void) {
  Serial.println("error: BME error, blink led");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}


//Routine Bad connection for S8
void BadConn() {
  Serial.println("Air sensor not detected. Please check wiring... Try# " + String(ConnRetry));
  delay(2500);
  ConnRetry++;
}

void Done() {
  Serial.println(F("done"));
}

void Failed() {
  Serial.println(F("failed"));
}

//Done or failed revision routine for S8
void CheckResponse(uint8_t* a, uint8_t* b, uint8_t len_array_cmp) {
  bool check_match = false;
  for (int n = 0; n < len_array_cmp; n++) {
    if (a[n] != b[n]) {
      check_match = false;
      break;
    } else
      check_match = true;
  }

  if (check_match) {
    Done();
  } else {
    Failed();
  }
}


// Calculate of Atmospheric pressure for S8
//return barometric pressure in hectopascal
void hPaCalculation() {
  //estimate barometric pressure from station pressure
  hpa = (bme_pressure * 10.0) + 128.1;  //note 128.1 is the average difference between station and barometric pressure for Calgary (hpa), averaged over 1000 days
}

// SenseAir S8 routines
// Initialice SenseAir S8
void CO2iniSenseAir() {
  //Deactivate Automatic Self-Calibration
  Serial2.write(cmdOFFs, MB_PKT_8);
  Serial2.readBytes(response, MB_PKT_8);
  Serial.print(F("Deactivate Automatic Self-Calibration SenseAir S8: "));
  CheckResponse(cmdOFFs, response, MB_PKT_8);
  delay(2000);

  while (co2SenseAir() == 0 && (ConnRetry < 5)) {
    BadConn();
  }
  Serial.println(F(" Air sensor detected AirSense S8 Modbus"));
  Serial.println(F("SenseAir read OK"));
  delay(4000);
}

// CO2 lecture SenseAir S8
int co2SenseAir() {
  static byte responseSA[MB_PKT_7] = { 0 };
  memset(responseSA, 0, MB_PKT_7);
  Serial2.write(cmdReSA, MB_PKT_8);
  Serial2.readBytes(responseSA, MB_PKT_7);
  CO2 = (256 * responseSA[3]) + responseSA[4];



  if (CO2 != 0) {
    crc_cmd = crcx::crc16(responseSA, 5);
    if (responseSA[5] == lowByte(crc_cmd) && responseSA[6] == highByte(crc_cmd)) {
      return CO2;
    } else {
      while (co2sensor.available() > 0)
        char t = co2sensor.read();  // Clear serial input buffer;
      Serial.println(F("FAIL"));
      return 0;
    }
  } else {

    Serial.println("FAIL");

    return 0;
  }
}
