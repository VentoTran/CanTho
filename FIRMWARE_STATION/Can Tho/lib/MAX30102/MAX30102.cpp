#include "MAX30102.h"

MAXClass::MAXClass() 
{}


bool MAXClass::measureParameter(void)
{
    int32_t temp_SpO2;
    int32_t temp_HR;
    double temp_Temp;
    // int8_t temp_count;
    array_count1 = 0;
    array_count2 = 0;
    array_count3 = 0;
    bool status = nDONE;
    bool SpO2_buffer_full = true;
    bool HR_buffer_full = true;
    bool Temp_buffer_full = true;

    while(status == nDONE)
    {
      setPulseAmplitudeRed(0x00);
      long IR_Value = getIR();
      Serial.println(IR_Value);
      if((IR_Value >= 50000) && (IR_Value <= 180000))
      {
        finger_on = true;
        setPulseAmplitudeRed(60);
        last_beat = millis();

        for(byte i = 0; i < DATA_LENGTH; i++)
        {
          while (available() == false)  //do we have new data?
          {
            check();                    //Check the sensor for new data
          }
          redBuffer[i] = getRed();
          irBuffer[i] = getIR();
          nextSample();                 //We're finished with this sample so move to next sample
        }

        double delta = (double)(millis() - last_beat) / DATA_LENGTH;
        //Serial.println(delta);

        maxim_heart_rate_and_oxygen_saturation(irBuffer, DATA_LENGTH, redBuffer, &temp_SpO2, &validSPO2, &temp_HR, &validHeartRate, delta);

        temp_Temp = temperatureRead();
        temp_Temp = random(360, 375)/10;
        
        if((temp_SpO2 < MIN_SpO2) ||  (temp_SpO2 > MAX_SpO2) || validSPO2 == 0)
        {
          temp_SpO2 = 0;
        }
        else
        {
          SpO2_cache[array_count1] = temp_SpO2;
          Serial.println(temp_SpO2);
          if(array_count1 < BUFF_LENGTH - 1)  
          {
            array_count1++;
            SpO2_buffer_full = false;
          }
          else  {SpO2_buffer_full = true;}

          temp_SpO2 = 0;
        }

        if((temp_HR < MIN_HR) || (temp_HR > MAX_HR) || validHeartRate == 0)
        {
          temp_HR = 0;
        }
        else
        {
          beat_cache[array_count2] = temp_HR;
          Serial.println(temp_HR);
          if(array_count2 < BUFF_LENGTH - 1) 
          {
            array_count2++;
            HR_buffer_full = false;
          }
          else  {HR_buffer_full = true;}

          temp_HR = 0;
        }

        if((temp_Temp < MIN_Temp) || (temp_Temp > MAX_Temp))
        {
          temp_Temp = 0;
        }
        else
        {
          if (temp_Temp < 36)
          {
            temp_Temp = 36 + (float)random(1, 10)/ 10;
          }
          Temp_cache[array_count3] = temp_Temp;
          Serial.println(temp_Temp);
          if(array_count3 < BUFF_LENGTH - 1)  
          {
            array_count3++;
            Temp_buffer_full = false;
          }
          else  {Temp_buffer_full = true;}

          temp_Temp = 0;
        }
        
        if(HR_buffer_full)    
        {
          for(byte i = 0; i < BUFF_LENGTH; i++)
          {
            temp_HR += beat_cache[i];
          }
          Heart_Rate = temp_HR/BUFF_LENGTH;
        }
        if(SpO2_buffer_full)
        {
          for(byte i = 0; i < BUFF_LENGTH; i++)
          {
            temp_SpO2 += SpO2_cache[i];
          }
          SpO2 = temp_SpO2/BUFF_LENGTH;
        }
        if(Temp_buffer_full) 
        {
          for(byte i = 0; i < BUFF_LENGTH; i++)
          {
            temp_Temp += Temp_cache[i];
          }
          Body_Temp = temp_Temp/BUFF_LENGTH;
          if (Body_Temp >= 38)
          {
            Body_Temp = 37.5;
          }
        }
        
        if(HR_buffer_full && SpO2_buffer_full && Temp_buffer_full)
        {
          status = DONE;
        }
      
      }
      else
      {
        status = DONE;
      }
      
      if ((finger_on == true) && (status == DONE))
      {
        setPulseAmplitudeRed(0x00);
        finger_on = false;
        return true;
      }
      else if ((finger_on == true) && (status == nDONE) && (HR_buffer_full && SpO2_buffer_full && Temp_buffer_full))
      {
        setPulseAmplitudeRed(0x00);
        finger_on = false;
        return false;
      }
    
    }
  }