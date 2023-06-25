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
#include "Adafruit_BME680.h"
//TODO:bosch apparently has a library for the bme that can calc voc
//should check it out

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
//TODO: convert to bsec library and use i2c protocol with pull down resistors
//TODO: i2c has to share the same bus as RTC
//BME680
#define BME_SCK 52
#define BME_MISO 50
#define BME_MOSI 51
#define BME_CS 44

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme(BME_CS); // hardware SPI

float bme_temp;
float bme_humidity;
float bme_voc;
float bme_hpa; //note: 1 hPa == 1 mbar

//TODO:ok so i dont think there is a way to calibrate the temp with the adafruit lib, so we will have to try a different lib

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
DHT dht(DHT22DataPin, DHTTYPE);
float humidity;



//---------------------------------------------------------------------------------
//graphcode
// Define variables for line graph
const int displayWidth = 240;   //x
const int displayHeight = 320;  //y

const int graphWidth = 180;  //px
const int graphHeight = 70;  //px

const int graphXPos = 30;
const int graphTopYPos = 15;

//offsets for middle and bottom graphs
const int graphMiddleYPos = 107;
const int graphBottomYPos = 214;

const int minTemp = 15;
const int maxTemp = 26;

const int minHumidity = 25;
const int maxHumidity = 75;

const int minVoc = 25;
const int maxVoc = 500;

//TODO:max/min voc, hpa

int x;
int y;


bool valueError = false;

//---------------------------------------------------------------------------------
//time module
//uRTCLib rtc(0x68);
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

float bmeTotalTempReadings;
float bmeAverageTempReading;
float bmePreviousaverageTempReading;

//deltaTemp difference between bme and ntc
float dAvgTemp;
float dPrevAvgTemp;
int mapDTemp;
int mapDPreviousTemp;

//TODO: do these really need to be global variables?
int mapTopTemp;
int mapTopPreviousTemp;

int mapBmeTopTemp;
int mapBmeTopPreviousTemp;

int mapMiddleTemp;
int mapMiddlePreviousTemp;

//Humidity variables
float totalHumidityReadings;
float averageHumidityReading;
float previousaverageHumidityReading;

float bmeTotalHumidityReadings;
float bmeAverageHumidityReading;
float bmePreviousaverageHumidityReading;

int mapTopHumidity;
int mapTopPreviousHumidity;

int mapBmeMiddleHumidity;
int mapBmeMiddlePreviousHumidity;

int mapMiddleHumidity;
int mapMiddlePreviousHumidity;

//VOC variables
float totalVoc;
float averageVoc;
float averagePreviousVoc;
int mapVoc;
int mapPreviousVoc;

//---------------------------------------------------------------------------------




void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(3000);  // wait for console opening

  URTCLIB_WIRE.begin();
rtc.set_rtc_address(0x68);
rtc.set_model(URTCLIB_MODEL_DS3231);
  // Following line sets the RTC with an explicit date & time
  // for example to set January 13 2022 at 12:15 you would call://TODO:
  // Comment out below line once there is a battery in module, and you set the date & time
  //rtc.set(20, 26, 18, 2, 15, 5, 23);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)



  //BME680
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
    // bme Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms


  //thermistor
  pinMode(ntcInput, INPUT);  // analog

  

  rtc.refresh();
  currentMinute = rtc.minute();
  previousMinute = rtc.minute();

  pixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  previousPixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  counter = 0;
  totalTempReadings = 0;
  bmeTotalTempReadings = 0;
  totalHumidityReadings = 0;
  bmeTotalHumidityReadings = 0;


  R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
  logR2 = log(R2);
  temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
  //rounds temperature to one decimal place.
  previousaverageTempReading = round(temperature * 10) / 10.0;
  previousaverageHumidityReading = dht.readHumidity();
    bmePreviousaverageTempReading = bme.readTemperature();
  bmePreviousaverageHumidityReading = bme.readHumidity();

  dPrevAvgTemp = bmePreviousaverageTempReading-previousaverageTempReading;


  averagePreviousVoc = bme.gas_resistance / 1000.0;


  //display
  tft.init(240, 320);
  Serial.println(F("Initialized"));

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
  // tft.drawFastVLine(graphXPos - 3, graphMiddleYPos, graphHeight, ST77XX_YELLOW);
  // drawScalePoint(15, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, false, true);
  // drawScalePoint(16, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  // drawScalePoint(18, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  // drawScalePoint(20, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  // drawScalePoint(22, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  // drawScalePoint(24, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  // drawScalePoint(26, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);


  //bottom left axis
  //tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, BLUE);
   drawDualScalePoint(15, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, false, true);
   drawDualScalePoint(16, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(18, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(20, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(22, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(24, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);
   drawDualScalePoint(26, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, BLUE, true, true);

  //test Double axis
  tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, ST77XX_CYAN);
   drawDualScalePoint(15, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, false, false);
   drawDualScalePoint(16, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(18, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(20, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(22, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(24, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);
   drawDualScalePoint(26, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_CYAN, true, false);

  //draw right axis (top right humidity)
  // tft.drawFastVLine(graphXPos + graphWidth + 3, graphTopYPos, graphHeight, ST77XX_CYAN);
  // drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);
  // drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  // drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  // drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  // drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  // drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  // drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);

//delta temp scale top
    tft.drawFastVLine(graphXPos + graphWidth + 3, graphTopYPos, graphHeight, ST77XX_RED);
  drawScalePoint(4, 4, 7, graphHeight + graphTopYPos, graphTopYPos, ST77XX_RED, true, false);
  drawScalePoint(7, 4, 7, graphHeight + graphTopYPos, graphTopYPos, ST77XX_RED, true, false);


  //middle right axis
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphMiddleYPos, graphHeight, PURPLE);
  drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, false, false);
  drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
  drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
  drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
  drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
  drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, true, false);
  drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, PURPLE, false, false);

  //bottom right axis
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphBottomYPos, graphHeight, RED);
  drawScalePoint(25, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, false, false);
  drawScalePoint(100, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
    drawScalePoint(175, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  drawScalePoint(250, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  drawScalePoint(325, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  drawScalePoint(400, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  drawScalePoint(475, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, true, false);
  drawScalePoint(500, minVoc, maxVoc, graphHeight + graphBottomYPos, graphBottomYPos, RED, false, false);


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

  Serial.println("starting monitoring");
}

void loop() {
  // put your main code here, to run repeatedly:

  //Temp
  R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
  logR2 = log(R2);
  temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
  //rounds temperature to one decimal place.
  temperature = round(temperature * 10) / 10.0;

  bme_temp = bme.readTemperature();
  bme_humidity = bme.readHumidity();

  bme_voc = bme.gas_resistance / 1000.0;

  //Humidity
  humidity = dht.readHumidity();
  while (isnan(humidity)) {
    Serial.println('failed to read humidity!');
    delay(1000);
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

    bmeAverageTempReading = (float)bmeTotalTempReadings/ (float)counter;
    bmeAverageHumidityReading = (float)bmeTotalHumidityReadings/ (float)counter;

//delta temp
    dAvgTemp = bmeAverageTempReading-averageTempReading;

    averageVoc = (float)totalVoc / (float)counter;




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

    //TODO:safety limit for bme temp or humidity
    //TODO:safetly limit on voc


    //get values for graphing
    //temp for top gragh
    mapTopTemp = mapf(averageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
    mapTopPreviousTemp = mapf(previousaverageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);

   //BME temp for top graph
    mapBmeTopTemp = mapf(bmeAverageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
    mapBmeTopPreviousTemp = mapf(bmePreviousaverageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);

//delta map
mapDTemp = mapf(dAvgTemp, 4, 7, graphHeight + graphTopYPos, graphTopYPos);
mapDPreviousTemp = mapf(dPrevAvgTemp, 4, 7, graphHeight + graphTopYPos, graphTopYPos);

  //draw top graph temp
    tft.drawLine(graphXPos + previousPixelPos, mapTopPreviousTemp, graphXPos + pixelPos, mapTopTemp, ST77XX_GREEN);
    previousaverageTempReading = averageTempReading;
    totalTempReadings = 0;
//draw bme temp graph
    tft.drawLine(graphXPos + previousPixelPos, mapBmeTopPreviousTemp, graphXPos + pixelPos, mapBmeTopTemp, ST77XX_CYAN);
    bmePreviousaverageTempReading = bmeAverageTempReading;
    bmeTotalTempReadings = 0;

    //draw delta temp
        tft.drawLine(graphXPos + previousPixelPos, mapDPreviousTemp, graphXPos + pixelPos, mapDTemp, ST77XX_RED);
    dPrevAvgTemp = dAvgTemp;
  




    //humidity for middle graph
    mapMiddleHumidity = mapf(averageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousHumidity = mapf(previousaverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);

//bme humidity for middle graph
  mapBmeMiddleHumidity = mapf(bmeAverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapBmeMiddlePreviousHumidity = mapf(bmePreviousaverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);

        //draw humdity graph middle
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousHumidity, graphXPos + pixelPos, mapMiddleHumidity, PURPLE);
              previousaverageHumidityReading = averageHumidityReading;
    totalHumidityReadings = 0;

    //draw bme humidity graph middle
      tft.drawLine(graphXPos + previousPixelPos, mapBmeMiddlePreviousHumidity, graphXPos + pixelPos, mapBmeMiddleHumidity, ST77XX_YELLOW);

        bmePreviousaverageHumidityReading = bmeAverageHumidityReading;
    bmeTotalHumidityReadings = 0;





//bottom graph
//voc
mapVoc = mapf(averageVoc, minVoc, maxVoc, graphHeight+ graphBottomYPos, graphBottomYPos);
mapPreviousVoc = mapf(averagePreviousVoc, minVoc, maxVoc, graphHeight+ graphBottomYPos, graphBottomYPos);
tft.drawLine(graphXPos+previousPixelPos, mapPreviousVoc, graphXPos +pixelPos, mapVoc,RED );
averagePreviousVoc = averageVoc;
totalVoc = 0;







    previousPixelPos = pixelPos;
    counter = 0;
  }
  if (currentMinute != previousMinute) {
    // Serial.print("Time: ");
    // Serial.print(rtc.hour());
    // Serial.print(":");
    // Serial.println(rtc.minute());
    previousMinute = currentMinute;
    counter++;
    totalTempReadings += temperature;
    bmeTotalTempReadings += bme_temp;

    totalHumidityReadings += humidity;
    bmeTotalHumidityReadings += bme_humidity;
    totalVoc+=bme_voc;

    // Serial.print("temperature: ");
    // Serial.println(temperature);
    // Serial.print("counter: ");
    // Serial.println(counter);
    // Serial.print("totalTempReadings: ");
    // Serial.println(totalTempReadings);

    // Serial.print("Humidity: ");
    // Serial.println(humidity);
    // Serial.print("counter: ");
    // Serial.println(counter);
    // Serial.print("totalHumidityReadings: ");
    // Serial.println(totalHumidityReadings);
  }

    //---------------------------------
      if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
// testing values from peripherals
//uncomment if want serial
// Serial.print("NTC temperature: ");
// Serial.println(temperature);

// Serial.print("DH11 Humidity: ");
// Serial.println(humidity);
// Serial.print("DH11 Temperature: ");
// Serial.println(dht.readTemperature());

// Serial.print("RTC DS3231 Temperature: ");
// Serial.println(rtc.temp()  / 100);


//   Serial.print("BME Temperature = ");
//   Serial.print(bme.temperature);
//   Serial.println(" *C");
//   Serial.print("BME Pressure = ");
//   Serial.print(bme.pressure / 100.0);
//   Serial.println(" hPa");
//   Serial.print("BME Humidity = ");
//   Serial.print(bme.humidity);
//   Serial.println(" %");
//   Serial.print("BME Gas = ");
//   Serial.print(bme.gas_resistance / 1000.0);
//   Serial.println(" KOhms");
//   Serial.print("BME Approx. Altitude = ");
//   Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
//   Serial.println(" m");
//   Serial.println(' ');


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
  tft.println("   ");

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(145, 0);
  tft.print("BME T:");
  tft.print(bme_temp);
  tft.print("c");

  //difference between bme and ntc
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(215,0);
  tft.print(bme_temp-temperature);


  //MIDDLE
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(5, 180);

  tft.print("BME H:");
  tft.print(bme_humidity);
  tft.print("%");

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(85, 180);
  tft.print(rtc.hour());
  tft.print(":");
  tft.print(rtc.minute());
  tft.print(":");
  tft.print(rtc.second());
  tft.println("   ");

  tft.setTextColor(PURPLE, ST77XX_BLACK);
  tft.setCursor(145, 180);
  tft.print("Humidity:");
  tft.print(humidity);
  tft.print("%");

  //BOTTOM
  tft.setTextSize(1);
  tft.setTextColor(BLUE, ST77XX_BLACK);
  tft.setCursor(5, 190);

  tft.print("VOC:");
  tft.print(bme_voc);
  //tft.print("");


  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(75, 190);

  tft.print("Pressure:");
  tft.print(bme.pressure / 100.0);
  //tft.print("");



  // tft.setTextColor(RED, ST77XX_BLACK);
  // tft.setCursor(145, 190);
  // tft.print(":");
  // tft.print(humidity);
  // tft.print("%");



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
  //Serial.print("mapf is returning: ");
  //Serial.println((value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin);
  return (value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin;
}
