#ifndef _LCDTFT_H_
#define _LCDTFT_H_

#include "Adafruit_ST7735.h"
#include "bitmap.h"
#include "MAX30102.h"
#include "string.h"


#define   PageOne     (1)
#define   PageTwo     (2)

#define   sim0  (0)
#define   sim1  (1)
#define   sim2  (2)
#define   sim3  (3)

#define   wifi0  (0)
#define   wifi1  (1)
#define   wifi2  (2)
#define   wifi3  (3)

#define   batt0  (0)
#define   batt1  (1)
#define   batt2  (2)
#define   batt3  (3)

#define   Charge  (true)
#define   nCharge (false)

class TFTClass: public Adafruit_ST7735
{
    public:

      TFTClass(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst);

      void PageOneDisplay(void);

      void PageTwoDisplay(void);

      void PageThreeDisplay(void);

      bool swScreen = true;
      bool isDisplaySuspended = false;
      byte pageIndex = PageOne;

      MAXClass* _MAX;

      void attachClass(MAXClass* MAXIM);

      void setSimSignal(byte nSimSignal);

      void setWifiSignal(byte nWifiSignal);
      
      void setGPS(bool nGPS);

      void setBatteryLevel(byte nBattLv, bool status);

      void updateHeader(void);

      void printData(void);

      void setNote(char* Note);

      void setMedicineName(char* Name);

      void setPatientName(char* Name);

      void fswScreen(byte page);

      

      byte simSignalStrength = 0;
      byte wifiSignalStrength = 0;
      bool GPS = false;
      byte batteryLevel = 0;

      bool charging = nCharge;
      
      int day = 00, month = 00, year = 00, hour = 00, minute = 00, second = 00;

      char MedicineName[13] = "Panadol";

      char PatientName[20] = "";

      char note1[21] = "Each line can only";
      char note2[21] = "has 20 chars, and";
      char note3[21] = "there are 6 lines.";
      char note4[21] = "So the total chars";
      char note5[21] = "in one note";
      char note6[21] = "is only 120 chars!";

      byte TestResult = 0;

      void clearNote(void);

      void clearName(void);
};


#endif /* _LCDTFT_H_ */

