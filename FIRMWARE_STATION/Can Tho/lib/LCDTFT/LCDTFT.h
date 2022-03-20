#ifndef _LCDTFT_H_
#define _LCDTFT_H_

#include "Adafruit_ST7735.h"
#include "bitmap.h"
#include "MAX30102.h"


class TFTClass: public Adafruit_ST7735
{
    public:

      TFTClass(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst);

      void PageOneDisplay(void);

      void PageTwoDisplay(void);

      byte simSignaldStrength = 0;
      byte wifiSignalStrength = 0;
      byte batteryLevel = 0;

      bool charging = false;

      int day = 21, month = 11, year = 21, hour = 17, minute = 23, second = 13;

      char MedicineName[13] = "Panadol";

      char note1[21] = "Each line can only";
      char note2[21] = "has 20 chars, and";
      char note3[21] = "there are 6 lines.";
      char note4[21] = "So the total chars";
      char note5[21] = "in one note";
      char note6[21] = "is only 120 chars!";

      MAXClass* _MAX;

      void attachClass(MAXClass* MAXIM);

      void setSpO2(int32_t nSpO2);

      void setHR(int32_t nHR);

      void setBodyTemp(double nTemp);

      void setSimSignal(byte nSimSignal);

      void setWifiSignal(byte nWifiSignal);

      void setBatteryLevel(byte nBattLv);

      void printData(void);

};


#endif /* _LCDTFT_H_ */

