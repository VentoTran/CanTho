#include "LCDTFT.h"

TFTClass::TFTClass(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst):
            Adafruit_ST7735(cs, dc, mosi, sclk, rst)
            {}


void TFTClass::attachClass(MAXClass* MAXIM)
{
  _MAX = MAXIM;
}

void TFTClass::printData(void)
{
  // if(_MAX->finger_on) {setTextColor(ST7735_GREEN, ST7735_BLACK);}
  // else {setTextColor(ST7735_WHITE, ST7735_BLACK);}

  //---------------------Body Temperature Display----------------------
  setCursor(46, 46);
  printf("%2.1f", _MAX->Body_Temp);
  drawBitmap(94, 46, Degree_Symbol, 12, 16, ST7735_WHITE);
  setCursor(106, 46);
  print("C");

  //-----------------------Heart Rate Display------------------------- 
  setCursor(46, 72);
  printf("%3i", _MAX->Heart_Rate);
  print("BPM");

  //-------------------------SpO2 Value Display------------------------
  setCursor(46, 98);
  printf("%3i", _MAX->SpO2);
  print("%");

}

void TFTClass::setSpO2(int32_t nSpO2)
{
  _MAX->SpO2 = nSpO2;
}

void TFTClass::setHR(int32_t nHR)
{
  _MAX->Heart_Rate = nHR;
}

void TFTClass::setBodyTemp(double nTemp)
{
  _MAX->Body_Temp = nTemp;
}

void TFTClass::setSimSignal(byte nSimSignal)
{
  simSignaldStrength = nSimSignal;
}

void TFTClass::setWifiSignal(byte nWifiSignal)
{
  wifiSignalStrength = nWifiSignal;
}

void TFTClass::setBatteryLevel(byte nBattLv)
{
  batteryLevel = nBattLv;
}

void TFTClass::PageOneDisplay(void)
{
  //-------------------------Signal symbol-----------------------------
  if(simSignaldStrength == 0)
  {
    drawBitmap(0, 0, simSignal0, 14, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(6, 3, 10, 7, ST7735_BLUE);
    drawLine(6, 7, 10, 3, ST7735_BLUE);
  }
  if(simSignaldStrength >= 1)
  {
    drawBitmap(0, 0, simSignal1, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignaldStrength >= 2)
  {
    drawBitmap(0, 0, simSignal2, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignaldStrength == 3)
  {
    drawBitmap(0, 0, simSignal3, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  
  //--------------------------Wifi symbol------------------------------
  if(wifiSignalStrength >= 1)
  {
    drawBitmap(19, 0, wifiSignal1, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength >= 2)
  {
    drawBitmap(19, 0, wifiSignal2, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == 3)
  {
    drawBitmap(19, 0, wifiSignal3, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == 0)
  {
    drawBitmap(19, 0, wifiSignal0, 8, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(22, 3, 26, 7, ST7735_BLUE);
    drawLine(22, 7, 26, 3, ST7735_BLUE);
  }

  //-----------------------------Date----------------------------------
  setTextSize(1);
  setTextColor(ST7735_CYAN);
  setCursor(47, 0);
  printf("%2i", day);
  print("/");
  printf("%2i", month);
  print("/");
  printf("%2i", year);

  //-------------------------Battery symbol----------------------------
  drawRect(116, 0, 12, 8, ST7735_WHITE);
  drawFastVLine(115, 2, 4, ST7735_WHITE);
  if(charging == true)
  {
    if(batteryLevel == 0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
    }
    if(batteryLevel >= 1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(118, 2, 2, 4, ST7735_GREEN);
    }
    if(batteryLevel >= 2)
    {
      drawRect(121, 2, 2, 4, ST7735_GREEN);
    }
    if(batteryLevel == 3)
    {
      drawRect(124, 2, 2, 4, ST7735_GREEN);
    }
  }
  else
  {
    if(batteryLevel == 0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLUE);
    }
    if(batteryLevel >= 1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(118, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel >= 2)
    {
      drawRect(121, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel == 3)
    {
      drawRect(124, 2, 2, 4, ST7735_WHITE);
    }

  }
  
  //------------------------------Time---------------------------------
  setTextSize(2);
  setCursor(16, 16);
  setTextColor(ST7735_YELLOW, ST7735_BLACK);
  printf("%2i", hour);
  print(":");
  printf("%2i", minute);
  print(":");
  printf("%2i", second);
  setTextColor(ST7735_WHITE, ST7735_BLACK);

  //------------------------Body Temperature Logo----------------------
  drawBitmap(14, 46, TemperatureLogo_1, 24, 16, ST7735_WHITE);
  drawRect(14 + 4, 46 + 5, 2, 9, ST7735_BLUE);
  drawRect(14 + 3, 46 + 10, 4, 3, ST7735_BLUE);

  //---------------------Body Temperature Display----------------------
  setCursor(46, 46);
  printf("----");
  drawBitmap(94, 46, Degree_Symbol, 12, 16, ST7735_WHITE);
  setCursor(106, 46);
  print("C");

  //------------------------Heart Rate Symbol-------------------------
  drawBitmap(14, 72, HeartRateLogo_2, 24, 16, ST7735_BLUE);

  //-----------------------Heart Rate Display------------------------- 
  setCursor(46, 72);
  printf("---");
  print("BPM");

  //-------------------------SpO2 Symbol Display-----------------------
  drawBitmap(14, 98, SpO2_Symbol_part1, 24, 16, ST77XX_BLUE);
  drawBitmap(26, 98, SpO2_Symbol_part2, 12, 9, ST7735_WHITE);

  //-------------------------SpO2 Value Display------------------------
  setCursor(46, 98);
  printf("---");
  print("%");

}


void TFTClass::PageTwoDisplay(void)
{
  //-------------------------Signal symbol-----------------------------
  if(simSignaldStrength == 0)
  {
    drawBitmap(0, 0, simSignal0, 14, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(6, 3, 10, 7, ST7735_BLUE);
    drawLine(6, 7, 10, 3, ST7735_BLUE);
  }
  if(simSignaldStrength >= 1)
  {
    drawBitmap(0, 0, simSignal1, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignaldStrength >= 2)
  {
    drawBitmap(0, 0, simSignal2, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignaldStrength == 3)
  {
    drawBitmap(0, 0, simSignal3, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  
  //--------------------------Wifi symbol------------------------------
  if(wifiSignalStrength >= 1)
  {
    drawBitmap(19, 0, wifiSignal1, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength >= 2)
  {
    drawBitmap(19, 0, wifiSignal2, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == 3)
  {
    drawBitmap(19, 0, wifiSignal3, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == 0)
  {
    drawBitmap(19, 0, wifiSignal0, 8, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(22, 3, 26, 7, ST7735_BLUE);
    drawLine(22, 7, 26, 3, ST7735_BLUE);
  }

  //-----------------------------Date----------------------------------
  setTextSize(1);
  setTextColor(ST7735_CYAN);
  setCursor(47, 0);
  printf("%2i", day);
  print("/");
  printf("%2i", month);
  print("/");
  printf("%2i", year);

  //-------------------------Battery symbol----------------------------
  drawRect(116, 0, 12, 8, ST7735_WHITE);
  drawFastVLine(115, 2, 4, ST7735_WHITE);
  if(charging == true)
  {
    if(batteryLevel == 0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
    }
    if(batteryLevel >= 1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(118, 2, 2, 4, ST7735_GREEN);
    }
    if(batteryLevel >= 2)
    {
      drawRect(121, 2, 2, 4, ST7735_GREEN);
    }
    if(batteryLevel == 3)
    {
      drawRect(124, 2, 2, 4, ST7735_GREEN);
    }
  }
  else
  {
    if(batteryLevel == 0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLUE);
    }
    if(batteryLevel >= 1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(118, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel >= 2)
    {
      drawRect(121, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel == 3)
    {
      drawRect(124, 2, 2, 4, ST7735_WHITE);
    }

  }

  //---------------------------- Tittle --------------------------------
  //bitmap
  drawBitmap(28, 15, Warning_part1, 24, 16, ST7735_BLUE);
  drawBitmap(28, 15, Warning_part2, 24, 16, ST7735_CYAN);
  drawBitmap(28, 15, Warning_part3, 24, 16, ST7735_BLUE);
  drawBitmap(52, 15, ClockSymbol, 24, 16, 0x03FF);
  drawBitmap(76, 15, Pill_part1, 24, 16, ST7735_WHITE);
  drawBitmap(76, 15, Pill_part2, 24, 16, ST7735_BLUE);
  drawBitmap(76, 15, Pill_part3, 24, 16, ST7735_RED);
  
  setCursor(17, 36);
  setTextSize(2);
  setTextColor(ST7735_YELLOW, ST7735_BLACK);
  printf("MEDICINE");

  setTextColor(ST77XX_BLUE, ST7735_BLACK);
  setCursor((128 - (int)strlen(MedicineName)*12)/2, 56);
  printf(MedicineName);

  setTextSize(1);
  setCursor(3, 76);
  setTextColor(ST7735_WHITE, ST7735_BLACK);
  printf(note1);
  setCursor(3, 84);
  printf(note2);
  setCursor(3, 92);
  printf(note3);
  setCursor(3, 100);
  printf(note4);
  setCursor(3, 108);
  printf(note5);
  setCursor(3, 116);
  printf(note6);
}

