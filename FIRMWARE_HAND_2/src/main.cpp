#include <Arduino.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <TinyGsmClient.h>
#include "topic.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"



#define   SSTXPin (17)
#define   SSRXPin (16)

#define   SDA     (21)
#define   SCL     (22)

// SoftwareSerial mySerial(SSTXPin, SSRXPin);

#define LED_POWER (60)
#define GPS_TRIGGER_PERIOD  pdMS_TO_TICKS(60000) 
#define CHECKSIGNAL_TRIGGER_PERIOD pdMS_TO_TICKS(5000)
#define MEASURE_TRIGGER_PERIOD pdMS_TO_TICKS(40000)
#define UPDATE_SCREEN_PERIOD pdMS_TO_TICKS(5000)

#define SOS_MODE_TIME_DEBOUNCE (500)
#define TEST_MODE_TIME_DEBOUNCE (3000)
TinyGsm modem(Serial2);
//TinyGsmClient simClient(modem);
WiFiClient espClient;
PubSubClient sim(espClient);
MPU6050 accelgyro;

char timeBuffer[23] = {0};
int gYear = 0, gMonth = 0, gDay = 0, gHour = 0, gMinute = 0, gSecond = 0;
float gTimezone = 0.0;

String userPhone1 = "0901215115";
String userPhone2 = "0914989855";

// float gLatitude = 10.0360307;
// float gLongitude = 105.7815542;

uint8_t signalStrength;


void setup()
{

  Serial.begin(9600);
  Serial2.begin(9600);
  //Wire.begin(SDA, SCL, 100000);
  Wire.begin();

  do
  {
      accelgyro.initialize();
  }
  while (!accelgyro.testConnection());

  EEPROM.begin(512);

  modem.enableGPS();
}

void loop()
{

}

