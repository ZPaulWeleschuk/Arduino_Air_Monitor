using namespace std;
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>
#include <math.h>
#include <Arduino.h>
#include <uRTCLib.h>
#include <DHT.h>

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
//TODO:


//If you arent able to use the designated SPI pins for your board, use the following instead:
//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
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
#define DHTTYPE DHT11   //creates dht object
#define DHT11DataPin 8  //defines arduino pin
DHT dht(DHT11DataPin, DHTTYPE);
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
 const int graphBottomYPos =214;

const int minTemp = 15;
const int maxTemp = 26;

const int minHumidity = 25;
const int maxHumidity = 75;

int x;
int y;


bool valueError = false;

//---------------------------------------------------------------------------------
//time module
uRTCLib rtc(0x68);
int currentMinute;
int previousMinute;

int pixelPos;
int previousPixelPos;
int counter;
//temp variables
float totalTempReadings;
float averageTempReading;
float previousaverageTempReading;

int mapTopTemp;
int mapTopPreviousTemp;

int mapMiddleTemp;
int mapMiddlePreviousTemp; 

//Humidity variables
float totalHumidityReadings;
float averageHumidityReading;
float previousaverageHumidityReading;

int mapTopHumidity;
int mapTopPreviousHumidity;

int mapMiddleHumidity;
int mapMiddlePreviousHumidity ;
//---------------------------------------------------------------------------------




void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(3000);  // wait for console opening
  URTCLIB_WIRE.begin();



  // Comment out below line once you set the date & time and there is a battery in module.
  // Following line sets the RTC with an explicit date & time
  // for example to set January 13 2022 at 12:15 you would call:
  rtc.set(00, 8, 14, 7, 18, 3, 23);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)

  rtc.refresh();
  currentMinute = rtc.minute();
  previousMinute = rtc.minute();

  pixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  previousPixelPos = ((rtc.hour() * 60) + (rtc.minute())) / 8;
  counter = 0;
  totalTempReadings = 0;
  totalHumidityReadings = 0;


  R2 = R1 * (1023.0 / (float)(analogRead(ntcInput)) - 1.0);
  logR2 = log(R2);
  temperature = ((1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15);
  //rounds temperature to one decimal place.
  previousaverageTempReading = round(temperature * 10) / 10.0;
  previousaverageHumidityReading = dht.readHumidity();



  //thermistor
  pinMode(ntcInput, INPUT);  // analog

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
  tft.drawFastVLine(graphXPos - 3, graphMiddleYPos, graphHeight, ST77XX_YELLOW);
  drawScalePoint(15, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, false, true);
  drawScalePoint(16, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(18, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(20, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(22, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(24, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);
  drawScalePoint(26, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_YELLOW, true, true);

  
  //bottom left axis
  // tft.drawFastVLine(graphXPos - 3, graphBottomYPos, graphHeight, ST77XX_ORANGE);
  // drawScalePoint(15, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, false, true);
  // drawScalePoint(16, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);
  // drawScalePoint(18, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);
  // drawScalePoint(20, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);
  // drawScalePoint(22, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);
  // drawScalePoint(24, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);
  // drawScalePoint(26, minTemp, maxTemp, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_ORANGE, true, true);

  //draw right axis (top right humidity)
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphTopYPos, graphHeight, ST77XX_CYAN);
  drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);
  drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, true, false);
  drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos, ST77XX_CYAN, false, false);

//middle right axis
  tft.drawFastVLine(graphXPos + graphWidth + 3, graphMiddleYPos, graphHeight, ST77XX_MAGENTA);
  drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, false, false);
  drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, true, false);
  drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, true, false);
  drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, true, false);
  drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, true, false);
  drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, true, false);
  drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos, ST77XX_MAGENTA, false, false);

  //bottom right axis
  // tft.drawFastVLine(graphXPos + graphWidth + 3, graphBottomYPos, graphHeight, ST77XX_PINK);
  // drawScalePoint(25, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, false, false);
  // drawScalePoint(30, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, true, false);
  // drawScalePoint(40, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, true, false);
  // drawScalePoint(50, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, true, false);
  // drawScalePoint(60, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, true, false);
  // drawScalePoint(70, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, true, false);
  // drawScalePoint(75, minHumidity, maxHumidity, graphHeight + graphBottomYPos, graphBottomYPos, ST77XX_PINK, false, false);


  //draw time axis
  tft.drawFastHLine(graphXPos, graphTopYPos + graphHeight + 1, graphWidth, ST77XX_WHITE);
  tft.drawFastHLine(graphXPos, graphTopYPos + graphHeight + 16, graphWidth, ST77XX_WHITE);
  drawTimeScalePoint(0, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(1, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(2, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(3, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(4, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(5, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(6, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(7, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(8, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(9, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(10, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(11, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(12, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(13, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(14, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(15, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(16, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(17, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(18, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(19, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(20, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(21, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);
  drawTimeScalePoint(22, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(23, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, false, true);
  drawTimeScalePoint(24, 0, 24, graphXPos, graphXPos + graphWidth, ST77XX_WHITE, true, true);


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
    //erases older line
    tft.drawFastVLine(graphXPos + pixelPos + 2, graphTopYPos, graphHeight, ST77XX_WHITE);
    tft.drawRect(graphXPos + pixelPos, graphTopYPos, 2, graphHeight + 1, ST77XX_BLACK);


    tft.drawFastVLine(graphXPos + pixelPos + 2, graphMiddleYPos, graphHeight, ST77XX_WHITE);
    tft.drawRect(graphXPos + pixelPos, graphMiddleYPos, 2, graphHeight + 1, ST77XX_BLACK);

    if (pixelPos == 0) {

      tft.drawFastVLine(graphXPos + graphWidth + 2, graphTopYPos, graphHeight, ST77XX_BLACK);
      tft.drawFastVLine(graphXPos + graphWidth + 1, graphTopYPos, graphHeight, ST77XX_BLACK);


      tft.drawFastVLine(graphXPos + graphWidth + 2, graphMiddleYPos, graphHeight, ST77XX_BLACK);
      tft.drawFastVLine(graphXPos + graphWidth + 1, graphMiddleYPos, graphHeight, ST77XX_BLACK);
    }
    if (previousPixelPos > pixelPos) {
      //this is for the 24 hour role over
      previousPixelPos = 0;
    }


    // Serial.print("Time: ");
    // Serial.print(rtc.hour());
    // Serial.print(":");
    // Serial.println(rtc.minute());

    //get the average reading
    averageTempReading = (float)totalTempReadings / (float)counter;          //we actually dont need to round, we have enough pixels to display halve values
    averageHumidityReading = (float)totalHumidityReadings / (float)counter;  // dont round


    // Serial.print("assinging ");
    // Serial.print(averageTempReading);
    // Serial.print(" to position ");
    // Serial.println(previousPixelPos);


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

    //get values for graphing
    //temp
    mapTopTemp = mapf(averageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
    mapTopPreviousTemp = mapf(previousaverageTempReading, minTemp, maxTemp, graphHeight + graphTopYPos, graphTopYPos);
    //humidity
    mapTopHumidity = mapf(averageHumidityReading, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos);
    mapTopPreviousHumidity = mapf(previousaverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphTopYPos, graphTopYPos);

    //test middle graph //TODO:
    mapMiddleTemp = mapf(averageTempReading, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousTemp = mapf(previousaverageTempReading, minTemp, maxTemp, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddleHumidity = mapf(averageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);
    mapMiddlePreviousHumidity = mapf(previousaverageHumidityReading, minHumidity, maxHumidity, graphHeight + graphMiddleYPos, graphMiddleYPos);


    Serial.print("mapaverageTempReading yPos: ");
    Serial.println(mapTopTemp);
    Serial.print(" @ xPos:");
    Serial.println(graphXPos + pixelPos);

    Serial.print("mapaverageHumidityReading yPos: ");
    Serial.println(mapTopHumidity);
    Serial.print(" @ xPos:");
    Serial.println(graphXPos + pixelPos);



    tft.drawLine(graphXPos + previousPixelPos, mapTopPreviousTemp, graphXPos + pixelPos, mapTopTemp, ST77XX_GREEN);

//TODO:test 
tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousTemp, graphXPos + pixelPos, mapMiddleTemp, ST77XX_YELLOW);

    previousaverageTempReading = averageTempReading;
    totalTempReadings = 0;

    tft.drawLine(graphXPos + previousPixelPos, mapTopPreviousHumidity, graphXPos + pixelPos, mapTopHumidity, ST77XX_CYAN);

//TODO: test
    tft.drawLine(graphXPos + previousPixelPos, mapMiddlePreviousHumidity, graphXPos + pixelPos, mapMiddleHumidity, ST77XX_MAGENTA);

    previousaverageHumidityReading = averageHumidityReading;
    totalHumidityReadings = 0;

    previousPixelPos = pixelPos;
    counter = 0;
  }
  if (currentMinute != previousMinute) {
    Serial.print("Time: ");
    Serial.print(rtc.hour());
    Serial.print(":");
    Serial.println(rtc.minute());
    previousMinute = currentMinute;
    counter++;
    totalTempReadings += temperature;
    totalHumidityReadings += humidity;

    Serial.print("temperature: ");
    Serial.println(temperature);
    Serial.print("counter: ");
    Serial.println(counter);
    Serial.print("totalTempReadings: ");
    Serial.println(totalTempReadings);

    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("counter: ");
    Serial.println(counter);
    Serial.print("totalHumidityReadings: ");
    Serial.println(totalHumidityReadings);
  }


  //---------------------------------
  //live read out of sensors
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
  tft.print("Humidity:");
  tft.print(humidity);
  tft.print("%");


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

//draw the time axis (x-axis)
void drawTimeScalePoint(int timePoint, int xMin, int xMax, int xStart, int xEnd, unsigned int color, bool drawLabel, bool drawBottomScale) {
  x = map(timePoint, xMin, xMax, xStart, xEnd);
  tft.drawFastVLine(x, graphTopYPos + graphHeight + 1, 3, color);
  if (drawBottomScale) {
    tft.drawFastVLine(x, graphTopYPos + graphHeight + 13, 3, color);
  }
  if (drawLabel) {
    tft.setTextColor(color);
    if (timePoint > 9) {  //label contains two digits off set it so its center still
      tft.setCursor(x - 5, graphTopYPos + graphHeight + 5);
    } else {
      tft.setCursor(x - 2, graphTopYPos + graphHeight + 5);
    }
    tft.print(timePoint);
  }
}

//like the map function but accommodates floats
float mapf(float value, int inputMin, int inputMax, int outputMin, int outputMax) {
  Serial.print("mapf is returning: ");
  Serial.println((value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin);
  return (value - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin;
}
