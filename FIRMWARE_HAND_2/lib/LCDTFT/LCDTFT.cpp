#include "LCDTFT.h"

TFTClass::TFTClass(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst):
            Adafruit_ST7735(cs, dc, mosi, sclk, rst)
            {}


void TFTClass::attachClass(MAXClass* MAXIM)
{
  _MAX = MAXIM;
}


void TFTClass::updateHeader(void)
{
  //-------------------------Signal symbol-----------------------------
  if(simSignalStrength == sim0)
  {
    drawBitmap(0, 0, simSignal0, 14, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(6, 3, 10, 7, ST7735_BLUE);
    drawLine(6, 7, 10, 3, ST7735_BLUE);
  }
  if(simSignalStrength >= sim1)
  {
    drawBitmap(0, 0, simSignal1, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignalStrength >= sim2)
  {
    drawBitmap(0, 0, simSignal2, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(simSignalStrength == sim3)
  {
    drawBitmap(0, 0, simSignal3, 14, 8, ST7735_WHITE, ST7735_BLACK);
  }
  
  //--------------------------Wifi symbol------------------------------
  if(wifiSignalStrength >= wifi1)
  {
    drawBitmap(19, 0, wifiSignal1, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength >= wifi2)
  {
    drawBitmap(19, 0, wifiSignal2, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == wifi3)
  {
    drawBitmap(19, 0, wifiSignal3, 8, 8, ST7735_WHITE, ST7735_BLACK);
  }
  if(wifiSignalStrength == wifi0)
  {
    drawBitmap(19, 0, wifiSignal0, 8, 8, ST7735_WHITE, ST7735_BLACK);
    drawLine(22, 3, 26, 7, ST7735_BLUE);
    drawLine(22, 7, 26, 3, ST7735_BLUE);
  }

  //-------------------------- GPS Symbol -----------------------------
  drawBitmap(32, 0, GPSSignal0, 8, 8, ST7735_WHITE, ST77XX_BLACK);
  if (GPS == false)
  {
    drawBitmap(32, 0, GPSSignal1, 8, 8, ST7735_BLUE);
  }

  //-----------------------------Date----------------------------------
  //setTextSize(1);
  //setTextColor(ST7735_CYAN);
  // setCursor(47, 0);
  // printf("%2i", day);
  // print("/");
  // printf("%2i", month);
  // print("/");
  // printf("%2i", year);

  //-------------------------Battery symbol----------------------------
  drawRect(116, 0, 12, 8, ST7735_WHITE);
  drawFastVLine(115, 2, 4, ST7735_WHITE);
  if(charging == true)
  {
    if(batteryLevel == batt0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
    }
    if(batteryLevel >= batt1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(124, 2, 2, 4, ST7735_GREEN);
      
    }
    if(batteryLevel >= batt2)
    {
      drawRect(121, 2, 2, 4, ST7735_GREEN);
    }
    if(batteryLevel == batt3)
    {
      drawRect(118, 2, 2, 4, ST7735_GREEN);
    }
  }
  else
  {
    if(batteryLevel == batt0)
    {
      fillRect(118, 2, 8, 4, ST7735_BLUE);
    }
    if(batteryLevel >= batt1)
    {
      fillRect(118, 2, 8, 4, ST7735_BLACK);
      drawRect(124, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel >= batt2)
    {
      drawRect(121, 2, 2, 4, ST7735_WHITE);
    }
    if(batteryLevel == batt3)
    {
      drawRect(118, 2, 2, 4, ST7735_WHITE);
    }
  }

  setTextSize(2);
  setTextColor(ST7735_WHITE);
}

void TFTClass::printData(void)
{
  // if(_MAX->finger_on) {setTextColor(ST7735_GREEN, ST7735_BLACK);}
  // else {setTextColor(ST7735_WHITE, ST7735_BLACK);}

  setTextColor(ST7735_WHITE, ST7735_BLACK);
  setTextSize(1);
  setCursor((128 - (int)strlen(PatientName) * 8) / 2, 16);
  printf(PatientName);
  setTextSize(2);

  //---------------------Body Temperature Display----------------------
  setCursor(46, 46);
  printf("%2.1f", _MAX->Body_Temp);
  //drawBitmap(94, 46, Degree_Symbol, 12, 16, ST7735_WHITE);
  // setCursor(106, 46);
  // print("C");

  //-----------------------Heart Rate Display------------------------- 
  setCursor(46, 72);
  printf("%3i", _MAX->Heart_Rate);
  //print("BPM");

  //-------------------------SpO2 Value Display------------------------
  setCursor(46, 98);
  printf("%3i", _MAX->SpO2);
  //print("%");
}

void TFTClass::setSimSignal(byte nSimSignal)
{
  simSignalStrength = nSimSignal;
  //updateHeader();
}

void TFTClass::setWifiSignal(byte nWifiSignal)
{
  wifiSignalStrength = nWifiSignal;
  //updateHeader();
}

void TFTClass::setGPS(bool nGPS)
{
  GPS = nGPS;
  //updateHeader();
}

void TFTClass::setBatteryLevel(byte nBattLv, bool status)
{
  batteryLevel = nBattLv;
  charging = status;
  updateHeader();
}

void TFTClass::setNote(char* Note)
{
  clearNote();

  int len = (int)strlen(Note);

  int line = (int)(len/20) + 1;

  int count = 0;

  if(line >= 1)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note1[count++] = *Note++;
    }
    note1[count] = '\0';
  }
  if(line >= 2)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note2[count++] = *Note++;
    }
    note2[count] = '\0';
  }
  if(line >= 3)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note3[count++] = *Note++;
    }
    note3[count] = '\0';
  }
  if(line >= 4)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note4[count++] = *Note++;
    }
    note4[count] = '\0';
  }
  if(line >= 5)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note5[count++] = *Note++;
    }
    note5[count] = '\0';
  }
  if(line >= 6)
  {
    count = 0;
    while((*Note != '\0') && (count < 20))
    {
      note6[count++] = *Note++;
    }
    note6[count] = '\0';
  }

}

void TFTClass::setMedicineName(char* Name)
{
  clearName();
  int count = 0;
  while((*Name != '\0') && (count < 12))
  {
    MedicineName[count++] = *Name++;
  }
  MedicineName[count] = '\0';
}

void TFTClass::setPatientName(char* Name)
{
  for(int i = 0; i < 20; i++)
  {
    PatientName[i] = '\0';
  }
  int count = 0;
  while ((*Name != '\0') && (count < 20))
  {
    PatientName[count] = *Name++;
    count++;
  }
  PatientName[count] = '\0';
}

void TFTClass::clearNote(void)
{
  for(int i = 0; i < 20; i++)
  {
    note1[i] = ' ';
    note2[i] = ' ';
    note3[i] = ' ';
    note4[i] = ' ';
    note5[i] = ' ';
    note6[i] = ' ';
  }

  note1[20] = '\0';
  note2[20] = '\0';
  note3[20] = '\0';
  note4[20] = '\0';
  note5[20] = '\0';
  note6[20] = '\0';
}

void TFTClass::clearName(void)
{
  for(int i = 0; i < 12; i++)
  {
    MedicineName[i] = ' ';
  }

  MedicineName[12] = '\0';
}

void TFTClass::fswScreen(byte page)
{
  swScreen = true;
  pageIndex = page;
}


void TFTClass::PageOneDisplay(void)
{
  //------------------------- Clear Screen ----------------------------
  fillScreen(ST7735_BLACK);

  updateHeader();
  
  //------------------------------Time---------------------------------
  setTextSize(2);
  // setCursor(16, 16);
  // setTextColor(ST7735_YELLOW, ST7735_BLACK);
  // printf("%2i", hour);
  // print(":");
  // printf("%2i", minute);
  // print(":");
  // printf("%2i", second);
  setTextColor(ST7735_WHITE, ST7735_BLACK);

  //------------------------Body Temperature Logo----------------------
  drawBitmap(14, 46, TemperatureLogo_1, 24, 16, ST7735_WHITE);
  drawRect(14 + 4, 46 + 5, 2, 9, ST7735_BLUE);
  drawRect(14 + 3, 46 + 10, 4, 3, ST7735_BLUE);

  //---------------------Body Temperature Display----------------------
  setCursor(46, 46);
  //printf("----");
  drawBitmap(94, 46, Degree_Symbol, 12, 16, ST7735_WHITE);
  setCursor(106, 46);
  print("C");

  //------------------------Heart Rate Symbol-------------------------
  drawBitmap(14, 72, HeartRateLogo_2, 24, 16, ST7735_BLUE);

  //-----------------------Heart Rate Display------------------------- 
  setCursor(46, 72);
  //printf("---");
  print("   BPM");

  //-------------------------SpO2 Symbol Display-----------------------
  drawBitmap(14, 98, SpO2_Symbol_part1, 24, 16, ST77XX_BLUE);
  drawBitmap(26, 98, SpO2_Symbol_part2, 12, 9, ST7735_WHITE);

  //-------------------------SpO2 Value Display------------------------
  setCursor(46, 98);
  //printf("---");
  print("   %");


  //----------------------------Display Data---------------------------

  // if(isDisplaySuspended)
  // {
  //   printData();
  //   isDisplaySuspended = false;
  // }

}

void TFTClass::PageTwoDisplay(void)
{
  //------------------------- Clear Screen ----------------------------
  fillScreen(ST7735_BLACK);
  
  //---------------------------- Header -------------------------------
  updateHeader();

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

void TFTClass::PageThreeDisplay(void)
{
  //------------------------- Clear Screen ----------------------------
  fillScreen(ST7735_BLACK);
  
  //---------------------------- Header -------------------------------
  updateHeader();

  //---------------------------- Tittle ------------------------------

  setTextSize(2);
  setTextColor(ST7735_WHITE, ST7735_BLACK);
  setCursor((128 - (int)strlen("TEST PAGE")*12)/2, 30);
  printf("TEST PAGE");

  setTextSize(1);
  setCursor((128 - (int)strlen("Press the")*6)/2, 72);
  printf("Press the");
  setCursor((128 - (int)strlen("button again")*6)/2, 72+12);
  printf("button again");
  setCursor((128 - (int)strlen("to begin!")*6)/2, 72+24);
  printf("to begin!");
  

}