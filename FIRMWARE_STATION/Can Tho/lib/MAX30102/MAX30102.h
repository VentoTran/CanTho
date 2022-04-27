#ifndef _MAX30102_H_
#define _MAX30102_H_

#include <Arduino.h>
#include "../SparkFun_MAX3010x_Pulse_and_Proximity_Sensor_Library/src/MAX30105.h"
#include "../SparkFun_MAX3010x_Pulse_and_Proximity_Sensor_Library/src/heartRate.h"
#include "../SparkFun_MAX3010x_Pulse_and_Proximity_Sensor_Library/src/spo2_algorithm.h"


#define   MIN_SpO2    (90)
#define   MAX_SpO2    (100)
#define   MIN_HR      (60)
#define   MAX_HR      (120)
#define   MIN_Temp    (30)
#define   MAX_Temp    (45)
#define   DATA_LENGTH (200)
#define   BUFF_LENGTH (3)
#define   LED_POWER   (60)

#define   DONE  (true)
#define   nDONE (false)

class MAXClass: public MAX30105
{
  public:

    MAXClass();

    int32_t SpO2 = 0;
    int32_t Heart_Rate = 0;
    // double Body_Temp = 0.0;

    byte array_count1 = 0;
    byte array_count2 = 0;
    // byte array_count3 = 0;

    long last_beat = 0;

    bool finger_on = false;
    bool newData = false;

    int32_t beat_cache[BUFF_LENGTH];
    int32_t SpO2_cache[BUFF_LENGTH];
    // double Temp_cache[BUFF_LENGTH];

    uint16_t irBuffer[DATA_LENGTH];   //infrared LED sensor data
    uint16_t redBuffer[DATA_LENGTH];  //red LED sensor data

    int8_t validSPO2;       //indicator to show if the SPO2 calculation is valid
    int8_t validHeartRate;  //indicator to show if the heart rate calculation is valid


    bool measureParameter(void);
    
};

#endif  /* _MAX30102_H_ */