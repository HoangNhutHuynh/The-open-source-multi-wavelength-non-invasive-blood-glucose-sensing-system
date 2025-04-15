// Set up library
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <HTTPClient.h>
#include "WiFi.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SSID and PASSWORD of the WiFi network
const char *ssid = "PTN_CN_Laser";         // WiFi name
const char *password = "1234567890"; // WiFi password

// Google Script Web_App_URL
String Web_App_URL = "https://script.google.com/macros/s/AKfycbzOECrhmFIItIEDbeVc6Bj1siVXEjju28VnoOc7XId22t88euInd1gnMhPuesonpWQd/exec";

// Set the OLED screen parameters to 128x64
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// SPI interface setup
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 5

// Set up SD
// File myFile;

// Set up connection pins
const int sel = 16;     // Button select
const int save = 17;    // Button save data
const int photo_1 = 12; // Photodiode for REFLECTOR mode
const int photo_2 = 26; // Photodiode for TRANSMITTED mode
const int led_1 = 25;   // led 660nm wavelength
const int led_2 = 33;   // led 940nm wavelength
const int A = 34;       // Data reading port
const int buzzer = 14;  // Buzzer pin
bool connected_wifi;

// Declare variables related to the button
int valSave;
int valSaveold = HIGH;
int valSelect;
int valSelectold = HIGH;
int demSelect = 0; // Set the counter variable
int demSave = 0;   // Set the save variable

// Set up initial calculation variables
const int numberReading = 100;
float a[100];        // The array stores the read values
float sum_a = 0;     // Sum of read values
float a_average = 0; // Average value after reading

// Set up calculation variables after configuring the filter
int renumber = 0;       // Number of valid values ​​after filtering
float sum_renumber = 0; // Sum of valid values ​​after filtering
float value = 0;        // Final average value
float value_glucose = 0;

float px660, px940, pxkh, tq660, tq940, tqkh;                                                 // Updated data variables on Google sheet
float px660_glucose, px940_glucose, pxkh_glucose, tq660_glucose, tq940_glucose, tqkh_glucose; // Updated data variables on Google sheet

void setup()
{
  Serial.begin(115200);
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  Wire.begin(21, 22); // i2C communication setup
  pinMode(sel, INPUT_PULLUP);
  pinMode(save, INPUT_PULLUP);
  pinMode(led_1, OUTPUT);
  pinMode(led_2, OUTPUT);
  pinMode(photo_1, OUTPUT);
  pinMode(photo_2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(led_1, LOW);
  digitalWrite(led_2, LOW);
  digitalWrite(photo_1, LOW);
  digitalWrite(photo_2, LOW);
  digitalWrite(buzzer, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Check the connection of the OLED screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  // Display interface when connecting to wifi
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 24);
  display.println("CONNECTING...");
  display.display();
  delay(2000);
  // int wifi_retry = 0;
  //  Wait for Wifi connection response for 20 seconds
  if (WiFi.status() != WL_CONNECTED)
  {
    connected_wifi = false;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 24);
    display.println("OFFLINE MODEL");
    display.display();
    delay(1000);
  }
  // Check if there is a Wifi connection and display interface
  if (WiFi.status() == WL_CONNECTED)
  {
    connected_wifi = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 14);
    display.println("CONNECTED TO WIFI");
    display.setCursor(20, 34);
    display.println(WiFi.localIP());
    display.display();
    delay(1000);
  }
  // Set up loopback when SD card is not connected
  while (!SD.begin(SD_CS))
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(34, 20);
    display.println("CONNECT TO");
    display.setCursor(43, 35);
    display.println("SD CARD");
    display.display();
    delay(2000);
  }
  //  Set display if SD card is connected
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 24);
  display.println("DESIGN BY VHT");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(25, 24);
  display.println("SD IS CONNECTED");
  display.display();
  delay(2000);
  display.clearDisplay();
  screen();
}
// Set up the main screen interface
void screen()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 10);
  display.println("MODEL");
  display.setCursor(22, 40);
  display.println("GLUCOSE");
  display.display();
}
// Set up pre-measurement display interface
void before()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(46, 20);
  display.print("SAMPLE");
  display.setCursor(19, 35);
  display.print("AFTER 1 SECOND");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(46, 20);
  display.print("SAMPLE");
  display.setCursor(25, 35);
  display.print("IS PROGRESSING");
  display.display();
}
// Set up the display interface after measurement
void after()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 20);
  display.print("FINISHED");
  display.display();
  delay(1000);
}
// Set up buzzer
void buz()
{
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
}
// Set up VOLTAGE display interface
void display_voltage()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("VOLTAGE VALUE");
  display.setCursor(10, 30);
  display.print(px660, 3);
  display.setCursor(50, 30);
  display.print(px940, 3);
  display.setCursor(90, 30);
  display.print(pxkh, 3);
  display.setCursor(10, 50);
  display.print(tq660, 3);
  display.setCursor(50, 50);
  display.print(tq940, 3);
  display.setCursor(90, 50);
  display.print(tqkh, 3);
  display.display();
}
// Set up VOLTAGE data storage on the SD
void save_voltage()
{
  File dataFile = SD.open("/DATA.csv", FILE_WRITE);
  if (dataFile)
  {
    dataFile.seek(dataFile.size());
    dataFile.print(px660, 4);
    dataFile.print(",");
    dataFile.print(px940, 4);
    dataFile.print(",");
    dataFile.print(pxkh, 4);
    dataFile.print(",");
    dataFile.print(tq660, 4);
    dataFile.print(",");
    dataFile.print(tq940, 4);
    dataFile.print(",");
    dataFile.println(tqkh, 4);
    dataFile.close();
  }
}
// Set up GLUCOSE display interface
void display_glucose()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("GLUCOZO VALUE");
  display.setCursor(20, 30);
  display.print(px660_glucose, 0);
  display.setCursor(50, 30);
  display.print(px940_glucose, 0);
  display.setCursor(80, 30);
  display.print(pxkh_glucose, 0);
  display.setCursor(20, 50);
  display.print(tq660_glucose, 0);
  display.setCursor(50, 50);
  display.print(tq940_glucose, 0);
  display.setCursor(80, 50);
  display.print(tqkh_glucose, 0);
  display.display();
  // Set up GLUCOSE data storage on the SD
  File dataFile = SD.open("/DATA.csv", FILE_WRITE);
  if (dataFile)
  {
    dataFile.seek(dataFile.size());
    dataFile.print(px660_glucose, 0);
    dataFile.print(",");
    dataFile.print(px940_glucose, 0);
    dataFile.print(",");
    dataFile.print(pxkh_glucose, 0);
    dataFile.print(",");
    dataFile.print(tq660_glucose, 0);
    dataFile.print(",");
    dataFile.print(tq940_glucose, 0);
    dataFile.print(",");
    dataFile.println(tqkh_glucose, 0);
    dataFile.close();
  }
}
// Setup and measurement function
void setupMeasurement(int led1, int led2, int photo, bool dualLED, float &value, float &value_glucose, float Intercept, float Slope)
{
  // Adjust the variables to 0
  renumber = 0;
  sum_a = 0, a_average = 0, sum_renumber = 0;
  // Correct "dualLED" control will turn on 2 LEDs, otherwise turn on 1 LED
  if (dualLED)
  {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
  }
  else
  {
    digitalWrite(led1, HIGH);
  }
  // Turn on the corresponding photodiode pin
  digitalWrite(photo, HIGH);
  delay(100);
  // Read the data and calculate the average value
  for (int i = 0; i < numberReading; i++)
  {
    a[i] = analogRead(A) * 3.3 / 4095;
    sum_a += a[i];
    delay(2);
  }
  a_average = sum_a / numberReading;
  // Filter unusual values
  for (int i = 0; i < numberReading; i++)
  {
    if (a[i] > 1.03 * a_average || a[i] < 0.97 * a_average)
    {
      a[i] = -1;
    }
  }
  // Recalculate the average value after filtering
  for (int i = 0; i < numberReading; i++)
  {
    if (a[i] != -1)
    {
      a[renumber++] = a[i];
    }
  }
  for (int i = 0; i < renumber; i++)
  {
    sum_renumber += a[i];
  }
  value = sum_renumber / renumber;
  // Print the value `value` to Serial Monitor
  // Serial.print("Value: ");
  // Serial.println(value);
  // Convert to glucose value based on linear regression
  value_glucose = (value - Intercept) / Slope;
  // Turn off LED and photodiode
  if (dualLED)
  {
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
  }
  else
  {
    digitalWrite(led1, LOW);
  }
  digitalWrite(photo, LOW);
}

void loop()
{

  valSelect = digitalRead(sel);
  if (valSelect != valSelectold)
  {
    valSelectold = valSelect;
    if (valSelect != LOW)
    {
      buz();
      demSelect++;
      if (demSelect == 2)
      {
        demSave = 0;
        before();
        // Measure LED reflection at 660nm wavelength
        setupMeasurement(led_1, -1, photo_1, false, value, value_glucose, 0.215, 0.0044);
        px660 = value;
        px660_glucose = value_glucose;
        // Measure LED reflection at 940nm wavelength
        setupMeasurement(led_2, -1, photo_1, false, value, value_glucose, 0.2991, 0.0048);
        px940 = value;
        px940_glucose = value_glucose;
        // Measure LED reflection at wavelengths of 660nm & 940nm
        setupMeasurement(led_1, led_2, photo_1, true, value, value_glucose, 0.5448, 0.0091);
        pxkh = value;
        pxkh_glucose = value_glucose;
        // Measure LED transmittance at 660nm wavelength
        setupMeasurement(led_1, -1, photo_2, false, value, value_glucose, 0.6228, -0.0029);
        tq660 = value;
        tq660_glucose = value_glucose;
        // Measure LED transmittance at 940nm wavelength
        setupMeasurement(led_2, -1, photo_2, false, value, value_glucose, 0.9438, -0.005);
        tq940 = value;
        tq940_glucose = value_glucose;
        // Measure LED transmittance at 660nm & 940nm wavelength
        setupMeasurement(led_1, led_2, photo_2, true, value, value_glucose, 1.598, -0.008);
        tqkh = value;
        tqkh_glucose = value_glucose;
        // If the value is "nan", the result is 0
        if (isnan(px660))
          px660 = 0;
        if (isnan(px940))
          px940 = 0;
        if (isnan(pxkh))
          pxkh = 0;
        if (isnan(tq660))
          tq660 = 0;
        if (isnan(tq940))
          tq940 = 0;
        if (isnan(tqkh))
          tqkh = 0;
        after();
        display_voltage();
        demSelect = 1;
      }
    }
  }
  valSave = digitalRead(save);
  if (valSave != valSaveold)
  {
    valSaveold = valSave;
    if (valSave != LOW)
    {
      buz();
      demSave++;

      if (demSave == 1)
      {
        if (connected_wifi == true)
        {
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(4, 24);
          display.print("SAVING....");
          display.display();
          // Send data to Google Sheets
          String Send_Data_URL = Web_App_URL + "?sts=write";
          Send_Data_URL += "&px660=" + String(px660);
          Send_Data_URL += "&px940=" + String(px940);
          Send_Data_URL += "&pxkh=" + String(pxkh);
          Send_Data_URL += "&tq660=" + String(tq660);
          Send_Data_URL += "&tq940=" + String(tq940);
          Send_Data_URL += "&tqkh=" + String(tqkh);
          Send_Data_URL += "&px660_glucose=" + String(px660_glucose);
          Send_Data_URL += "&px940_glucose=" + String(px940_glucose);
          Send_Data_URL += "&pxkh_glucose=" + String(pxkh_glucose);
          Send_Data_URL += "&tq660_glucose=" + String(tq660_glucose);
          Send_Data_URL += "&tq940_glucose=" + String(tq940_glucose);
          Send_Data_URL += "&tqkh_glucose=" + String(tqkh_glucose);
          Serial.println();
          Serial.println("-------------");
          Serial.println("Send data to Google Spreadsheet...");
          Serial.print("URL:");
          Serial.println(Send_Data_URL);
          HTTPClient http;
          // HTTP GET Request
          http.begin(Send_Data_URL.c_str());
          http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
          // Gets the HTTP status code
          int httpCode = http.GET();
          Serial.print("HTTP Status Code : ");
          Serial.println(httpCode);
          // Getting response from google sheets
          String payload;
          if (httpCode > 0)
          {
            payload = http.getString();
            Serial.println("Payload : " + payload);
          }
          http.end();
        }
        else if (connected_wifi == false)
        {
          // Save Voltage data and display notification screen
          save_voltage();
        }
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(34, 24);
        display.print("SAVED");
        display.display();
        // Set the voltage variables to 0
        px660 = px940 = pxkh = tq660 = tq940 = tqkh = 0;
      }
      // Display Glucose value
      else if (demSave == 2)
      {
        display_glucose();
        px660_glucose = px940_glucose = pxkh_glucose = tq660_glucose = tq940_glucose = tqkh_glucose = 0;
        demSave = 0;
      }
    }
  }
}
