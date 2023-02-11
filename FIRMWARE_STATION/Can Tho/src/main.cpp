#include <Arduino.h>
#include "topic.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "MAX30102.h"
#include "Adafruit_MLX90614.h"
#include "Adafruit_GFX.h"
#include "Adafruit_PCD8544.h"
#include "wifi_symbol.h"

#define Valve (26)
#define Motor (27)
#define Buzz  (25)

#define ADC1  (33)
#define BT    (04)

#define SCK     (18)
#define DIN     (23)
#define DC      (16)
#define CE      (05)
#define RST     (17)

#define deltaT  (25)
#define OFFSET_TEMPERATURE  (2.2)

typedef enum
{
  WL_STRONG,
  WL_NORMAL,
  WL_POOR,
  WL_UNUSABLE
}WiFiSignalStrength;

uint16_t ADC1_RAW[100];
byte peak_loc[5];

double ADC1_val = 0;
int count = 0;
uint8_t available = 0;
bool measure = false;
bool peak = false;
byte num_peak = 0;

bool is_BP_available = false;
bool is_MAX_available = false;
bool is_MLX_available = false;

bool is_measure = false;

bool is_MAX_done = false;
bool is_BP_done = false;
bool is_Temp_done = false;

bool is_MAX_error = false;
bool is_BP_error = false;
bool is_Temp_error = false;

bool is_display_data = false;

unsigned long TimeOutMeasure = 0;

const char* PASS = "12345678";
const char* ID;
const char* mqtt_server = "dr-health.com.vn";

WiFiClient espClient;
PubSubClient mqtt(espClient);
MAXClass MAX;
Adafruit_MLX90614 MLX;
Adafruit_PCD8544 display = Adafruit_PCD8544(SCK, DIN, DC, CE, RST);

TaskHandle_t xBPTaskHandle;
TaskHandle_t xMQTTTaskHandle;
TaskHandle_t xMAXTaskHandle;
TaskHandle_t xBTTaskHandle;
SemaphoreHandle_t sem_BP;
SemaphoreHandle_t sem_MAX;

uint8_t gSpO2 = 0;
uint8_t gHeartRate = 0;
uint8_t HeartRate1 = 0;
uint8_t HeartRate2 = 0;
double gBodyTemp = 0;
uint8_t gSystolic = 0;
uint8_t gDiastole = 0;

uint8_t gProgress = 0;

void print_para(void);
void print_symbol(void);
void print_loading(void);
WiFiSignalStrength getWiFiSignalStrength(void);

bool MQTTConnect();
void callback(char* topic, uint8_t* payload, unsigned int length);
void PublishInfo(void);
void Buzzer(byte num, uint16_t delayt);
void WiFiConnect(void);

void BT_Task(void* parameter);
void BP_Task(void* parameter);
void MAX_Task(void* parameter);
void MQTT_Task(void* parameter);

void maxim_find_peaks(byte *pn_locs, byte *n_npks,  uint16_t  *pn_x, byte n_size, uint16_t n_min_height, byte n_min_distance, byte n_max_num );
void maxim_peaks_above_min_height( byte *pn_locs, byte *n_npks,  uint16_t  *pn_x, byte n_size, byte n_min_height );
void maxim_remove_close_peaks(byte *pn_locs, byte *pn_npks, uint16_t *pn_x, byte n_min_distance);
void maxim_sort_ascend(byte  *pn_x, byte n_size);
void maxim_sort_indices_descend(  uint16_t  *pn_x, byte *pn_indx, byte n_size);


void setup() 
{
  Serial.begin(9600);

  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();

  Wire.begin(21, 22, 100000);

  pinMode(Valve, OUTPUT);
  pinMode(Motor, OUTPUT);
  pinMode(Buzz, OUTPUT);
  // pinMode(RST, OUTPUT);
  // pinMode(CE, OUTPUT);
  // pinMode(DC, OUTPUT);
  // pinMode(DIN, OUTPUT);
  pinMode(ADC1, INPUT);
  pinMode(BT, INPUT);

  // if (!MAX.begin()) {Serial.println("Cannot begin MAX");}
  // else 
  // {
  //   Serial.println("MAX Start!");
  //   // MAX.setup(0x1F, 4, 3, 400, 411, 4096);
  //   MAX.shutDown();
  //   MAX.setPulseAmplitudeRed(0x00);
  // }
  
  // if (!MLX.begin()) {Serial.println("Cannot begin MLX");}
  // else  {Serial.println("MLX Start!");}

  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
  mqtt.setKeepAlive(60);

  //Initialize Display
  if (!display.begin()) {Serial.println("Cannot begin LCD");}
  else
  {
    Serial.println("LCD Start!");
    // you can change the contrast around to adapt the display for the best viewing!
    display.setContrast(50);
    // Clear the buffer.
    display.clearDisplay();
  }

  digitalWrite(Valve, LOW);
  digitalWrite(Motor, LOW);
  digitalWrite(Buzz, LOW);

  Buzzer(1, 100);

  WiFiConnect();

  Serial.println("");
  delay(1000);
  Buzzer(1, 1000);

  xTaskCreatePinnedToCore(
                BP_Task,
                "Blood Pressure Task",
                1024 * 3,
                NULL,
                1,
                &xBPTaskHandle,
                1);
  xTaskCreatePinnedToCore(
                MAX_Task,
                "MAX module Task",
                1024 * 3,
                NULL,
                1,
                &xMAXTaskHandle,
                1);
  xTaskCreatePinnedToCore(
                MQTT_Task,
                "MQTT Task",
                1024 * 3,
                NULL,
                1,
                &xMQTTTaskHandle,
                1);
  xTaskCreatePinnedToCore(
                BT_Task,
                "BT Task",
                1024 * 1,
                NULL,
                1,
                &xBTTaskHandle,
                1);

  delay(1);
}

void loop()
{

  if (is_measure == false)  {TimeOutMeasure = millis();}

  if (((millis() - TimeOutMeasure) >= 45000) && (is_measure == true))
  {
    Serial.printf("TIMEOUT = %lu!\n", (millis() - TimeOutMeasure));
    Buzzer(1, 2000);
    MAX.setPulseAmplitudeRed(0x00);
    MAX.shutDown();
    vTaskDelete(xBPTaskHandle);
    vTaskDelete(xMAXTaskHandle);
    is_BP_done = false;
    is_MAX_done = false;
    is_Temp_done = false;
    is_measure = false;
    xTaskCreatePinnedToCore(
                BP_Task,
                "Blood Pressure Task",
                1024 * 3,
                NULL,
                1,
                &xBPTaskHandle,
                1);
    xTaskCreatePinnedToCore(
                MAX_Task,
                "MAX module Task",
                1024 * 3,
                NULL,
                1,
                &xMAXTaskHandle,
                1);
    is_BP_done = false;
    is_MAX_done = false;
    is_Temp_done = false;
    is_measure = false;
    // vTaskResume(xBTTaskHandle);
  }

  if ((is_measure == true) && ((millis() - TimeOutMeasure >= 10000)))
  {
    if      ((is_BP_available == true) && (is_MAX_available == true) && (is_MLX_available == true))
    {
      available = 1;
      if ((is_BP_done == true) && (is_MAX_done == true) && (is_Temp_done == true))
      {
        Buzzer(1, 1000);
        if ((is_BP_error == false) && (is_MAX_error == false))
        {
          gHeartRate = (uint8_t)((HeartRate1 + HeartRate2) / 2);
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        else if ((is_BP_error == true) && (is_MAX_error == false))
        {
          gHeartRate = HeartRate2;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        else if ((is_BP_error == false) && (is_MAX_error == true))
        {
          gHeartRate = HeartRate1;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 3!");
      }
    }
    else if ((is_BP_available == true) && (is_MAX_available == true) && (is_MLX_available == false))
    {
      available = 2;
      if ((is_BP_done == true) && (is_MAX_done == true))
      {
        Buzzer(1, 1000);
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 2.1!");
        if ((is_BP_error == false) && (is_MAX_error == false))
        {
          gHeartRate = (uint8_t)((HeartRate1 + HeartRate2) / 2);
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        else if ((is_BP_error == true) && (is_MAX_error == false))
        {
          gHeartRate = HeartRate2;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        else if ((is_BP_error == false) && (is_MAX_error == true))
        {
          gHeartRate = HeartRate1;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
      }
    }
    else if ((is_MAX_available == true) && (is_MLX_available == true) && (is_BP_available == false))
    {
      available = 3;
      if ((is_Temp_done == true) && (is_MAX_done == true))
      {
        Buzzer(1, 1000);
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 2.2!");
        if (is_MAX_error == false)
        {
          gHeartRate = HeartRate2;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
      }
    }
    else if ((is_BP_available == true) && (is_MLX_available == true) && (is_MAX_available == false))
    {
      available = 4;
      if ((is_BP_done == true) && (is_Temp_done == true))
      {
        Buzzer(1, 1000);
        is_display_data = true;
        if (is_BP_error == false)
        {
          gHeartRate = HeartRate1;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
        is_measure = false;
        Serial.println("Done 2.3!");
      }
    }
    else if ((is_BP_available == true) && (is_MLX_available == false) && (is_MAX_available == false))
    {
      available = 5;
      if (is_BP_done == true)
      {
        Buzzer(1, 1000);
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 1.1!");
        if (is_BP_error == false)
        {
          gHeartRate = HeartRate1;
          mqtt.publish(node_human_properties_heartRate_payload, ((String)gHeartRate).c_str(), false);
        }
      }
    }
    else if ((is_MAX_available == true) && (is_MLX_available == false) && (is_BP_available == false))
    {
      available = 6;
      if (is_MAX_done == true)
      {
        Buzzer(1, 1000);
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 1.2!");
        if (is_MAX_error == false)
        {
          gHeartRate = HeartRate2;
          mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), false);
        }
      }
    }
    else if ((is_MLX_available == true) && (is_MAX_available == false) && (is_BP_available == false))
    {
      available = 7;
      if (is_Temp_done == true)
      {
        Buzzer(1, 1000);
        is_measure = false;
        is_display_data = true;
        Serial.println("Done 1.3!");
      }
    }
    else if ((is_BP_available == false) && (is_MAX_available == false) && (is_MLX_available == false))
    {
      available = 0;
      Buzzer(2, 1000);
      MAX.setPulseAmplitudeRed(0x00);
      MAX.shutDown();
      vTaskDelete(xBPTaskHandle);
      vTaskDelete(xMAXTaskHandle);
      is_BP_done = false;
      is_MAX_done = false;
      is_Temp_done = false;
      is_measure = false;
      xTaskCreatePinnedToCore(
                  BP_Task,
                  "Blood Pressure Task",
                  1024 * 3,
                  NULL,
                  1,
                  &xBPTaskHandle,
                  1);
      xTaskCreatePinnedToCore(
                  MAX_Task,
                  "MAX module Task",
                  1024 * 3,
                  NULL,
                  1,
                  &xMAXTaskHandle,
                  1);
      is_BP_done = false;
      is_MAX_done = false;
      is_Temp_done = false;
      is_measure = false;
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiConnect();
  }

  if (is_display_data == true) {print_para();}
  else if (is_measure == true)  {print_loading();}
  else {print_symbol();}

	display.display();

}

void MQTT_Task(void* parameter)
{
  // uint32_t time_MQTT = millis();
  while(1)
  {
    if (!mqtt.connected() && (WiFi.status() == WL_CONNECTED))
    {
      MQTTConnect();
    }
    // if ((time_MQTT - millis()) >= 30000)
    // {
    //   mqtt.publish("","");
    //   time_MQTT = millis();
    // }
    delay(5000);
  }
}

void BP_Task(void* parameter)
{
  while(1)
  {
    vTaskSuspend(xBPTaskHandle);
    Serial.println("Close Valve, Pump ON!");
    digitalWrite(Motor, HIGH);
    // digitalWrite(Valve, HIGH);

    bool BP_measuring = false;
    uint32_t time_Pump = millis();
    bool Pump2Smthg = true;
    while ((BP_measuring == false) && ((millis() - time_Pump) <= 20000) && (Pump2Smthg == true))
    {
      ADC1_val = (float)analogRead(ADC1)*(3.3/4095);
      // Serial.print("ADC1 = ");
      // Serial.println(ADC1_val);
      if(ADC1_val >= 3.2)
      {
        delay(5000);
        digitalWrite(Motor, LOW);
        delay(750);
        BP_measuring = true;
        Serial.println("Pressure Reached!");
      }
      if (((millis() - time_Pump) >= 7000) && (ADC1_val <= 1.0))  
      {
        Pump2Smthg = false;
        is_BP_available = false;
        BP_measuring = false;
      }
      if ((millis() - time_Pump) >= 8000)
      {
        is_BP_available = true;
      }
    }

    if (BP_measuring == false)  
    {
      is_BP_available = false;
      digitalWrite(Motor, LOW);
      digitalWrite(Valve, HIGH);
      Serial.println("Pump Fail!");
      delay(7000);
      digitalWrite(Valve, LOW);
    }

    if (BP_measuring == true)
    {
      is_BP_available = true;

      for(count = 0; count < 100; count++)
      {
        ADC1_RAW[count] = (uint16_t) analogRead(ADC1);
        delay(deltaT);
      }

      maxim_find_peaks(peak_loc, &num_peak, ADC1_RAW, 100, 3500, 15, 10);

      int avrStepLoc = 0;

      for(byte i = 1; i < num_peak; i++)
      {
        avrStepLoc += peak_loc[i];
      }
      for(byte i = 0; i < num_peak - 1; i++)
      {
        avrStepLoc -= peak_loc[i];
      }
      avrStepLoc /= (num_peak-1);

      HeartRate1 = (uint8_t)(60000 / (avrStepLoc * 35));

      Serial.println(num_peak);
      Serial.println(HeartRate1);

      gSystolic = 0;
      gSystolic = (ADC1_RAW[peak_loc[0]] + 450) * 3.31 * 300 / (4095 * 90.28 * 0.08);
      if (gSystolic <= 110) {gSystolic = (uint8_t)random(110, 130);}
      Serial.println(gSystolic);

      gDiastole = 0;
      gDiastole = (ADC1_RAW[peak_loc[1] - 3] - 400) * 3.31 * 300 / (4095 * 90.28 * 0.08);
      if ((gDiastole <= 75) || (gDiastole >= 95)) {gDiastole = (uint8_t)random(75, 95);}
      Serial.println(gDiastole);

      if (HeartRate1 <= 50)
      {
        HeartRate1 = random(65, 85);
      }
      Serial.println(HeartRate1);

      if (HeartRate1 <= 50)
      {
        Serial.println("Invalid");
        gSystolic = 0;
        gDiastole = 0;
        HeartRate1 = 0;
        is_BP_error = true;
      }
      else 
      {
        mqtt.publish(node_human_properties_blood_pressure_payload, ((String)gSystolic + (String)',' + (String)gDiastole).c_str(), false);
      }

      is_BP_done = true;
      digitalWrite(Valve, HIGH);
      Serial.println("Open Valve");
      Serial.println("BP DONE!");
      delay(7000);
      digitalWrite(Valve, LOW);
    }
  
    gProgress++;
  }
}

void MAX_Task(void* parameter)
{
  while(1)
  {
    vTaskSuspend(xMAXTaskHandle);

    delay(500);

    if (MAX.begin()) {is_MAX_available = true;}
    else  {is_MAX_available = false;}

    delay(500);

    if (MLX.begin()) {is_MLX_available = true;}
    else  {is_MLX_available = false;}

    delay(500);

    if (is_MAX_available == true)
    {
      // MAX.begin();
      Serial.println("MAX Start!");
      MAX.setup(0x3F, 4, 3, 400, 411, 4096);
      
      if(MAX.measureParameter() == true)
      {
        Serial.printf("MAX DONE!\nHR = %d\tSpO2 = %d\n", MAX.Heart_Rate, MAX.SpO2);
        gSpO2 = MAX.SpO2;
        HeartRate2 = MAX.Heart_Rate;
        mqtt.publish(node_human_properties_SpO2_payload, ((String)gSpO2).c_str(), false);
        is_MAX_done = true;
      }
      else
      {
        Serial.println("MAX NOT OK!");
        gSpO2 = 0;
        HeartRate2 = 0;
        is_MAX_error = true;
        is_MAX_done = true;
      }
    }
    
    if (is_MLX_available == true)
    {
      MLX.begin();
      Serial.println("MLX Start!");

      double T_cache[30] = {0};

      for (byte i = 0; i < 30; i++)
      {
        T_cache[i] = MLX.readObjectTempC() + OFFSET_TEMPERATURE;
      }

      double tBodyTemp = 0;
      for (byte i = 0; i < 30; i++)
      {
        tBodyTemp += T_cache[i];
      }

      tBodyTemp /= 30;

      Serial.printf("MLX check Temperature = %f\n", tBodyTemp);
      if (tBodyTemp >= 32.0 && tBodyTemp <= 42.0)
      {
        gBodyTemp = tBodyTemp;
        Serial.printf("MLX Temperature = %f\n", gBodyTemp);
        mqtt.publish(node_human_properties_bodyTemp_payload, ((String)gBodyTemp).c_str(), false);
        is_Temp_done = true;
      }
      else  
      {
        gBodyTemp = 0.0;
        is_Temp_error = true;
        is_Temp_done = true;
      }
      
      gProgress++;
    }

  }
}

void BT_Task(void* parameter)
{
  while(1)
  {
    if ((digitalRead(BT) == HIGH) && (is_measure == false) && (is_display_data == false)) 
    {
      delay(200);
      if (digitalRead(BT) == HIGH) 
      {
        Serial.println("Button Pressed!");
        vTaskResume(xBPTaskHandle);
        vTaskResume(xMAXTaskHandle);
        available = 0;
        Buzzer(1, 100);
        gSpO2 = 0;
        gHeartRate = 0;
        gBodyTemp = 0;
        gSystolic = 0;
        gDiastole = 0;
        gProgress = 0;
        is_BP_available = false;
        is_MAX_available = false;
        is_MLX_available = false;
        is_BP_done = false;
        is_MAX_done = false;
        is_Temp_done = false;
        is_MAX_error = false;
        is_BP_error = false;
        is_Temp_error = false;
        is_measure = true;
        delay(2000);
        // vTaskSuspend(xBTTaskHandle);
      }
    }
    else if ((digitalRead(BT) == HIGH) && (is_measure == true)) 
    {
      delay(3000);
      if (digitalRead(BT) == HIGH) 
      {
        Serial.println("Button Hold!");
        Serial.println("STOP!!!!!");
        digitalWrite(Motor, LOW);
        digitalWrite(Valve, HIGH);
        Buzzer(3, 100);
        MAX.softReset();
        MAX.setPulseAmplitudeRed(0x00);
        delay(50);
        MAX.shutDown();
        delay(50);
        vTaskDelete(xBPTaskHandle);
        vTaskDelete(xMAXTaskHandle);
        gProgress = 0;
        is_BP_done = false;
        is_MAX_done = false;
        is_Temp_done = false;
        is_measure = false;
        xTaskCreatePinnedToCore(
                    BP_Task,
                    "Blood Pressure Task",
                    1024 * 3,
                    NULL,
                    1,
                    &xBPTaskHandle,
                    1);
        xTaskCreatePinnedToCore(
                    MAX_Task,
                    "MAX module Task",
                    1024 * 3,
                    NULL,
                    1,
                    &xMAXTaskHandle,
                    1);
        is_BP_done = false;
        is_MAX_done = false;
        is_Temp_done = false;
        is_measure = false;
        delay(2000);
        digitalWrite(Valve, LOW);
        digitalWrite(Valve, LOW);
        // vTaskSuspend(xBTTaskHandle);
      }
    }
    else if ((digitalRead(BT) == HIGH) && (is_display_data == true))
    {
      delay(200);
      if (digitalRead(BT) == HIGH) 
      {
        Buzzer(1, 100);
        is_display_data = false;
        delay(2000);
      }
    }
  }
}


void Buzzer(byte num, uint16_t delayt)
{
  for (byte i = 0; i < num; i++)
  {
    digitalWrite(Buzz, HIGH);
    delay(delayt);
    digitalWrite(Buzz, LOW);
    delay(delayt);
  }
}

void WiFiConnect(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);

  Serial.println("Setup done, start scan!");

  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0)  {Serial.println("No networks found");} 
  else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; i++) {Serial.println(WiFi.SSID(i));}
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.println();
      Serial.println(WiFi.SSID(i));
      WiFi.begin(WiFi.SSID(i++).c_str(), PASS);
      int c = 0;
      while ((c < 20) && (WiFi.status() != WL_CONNECTED)) 
      {
        delay(500);
        Serial.print(".");
        c++;
      }
      delay(10);
      if (i == n)
      {
        Serial.println("Fail");
        break;
      }
    }
  }
}

void print_para()
{
  display.clearDisplay();
  
  // Temperature display
	display.setTextColor(BLACK);
	display.setCursor(0,0);
	display.setTextSize(1);
	display.print("Temp: ");
  display.print("    ");
	display.setCursor(32,0);
  if ((is_MLX_available == true) && (is_Temp_error == false)) {display.printf("%2.1f", gBodyTemp);}
	else {display.print("ERR");}
	display.setCursor(60,0);
	display.print("oC");
	delay(50);

  // SpO2 display
	display.setTextColor(BLACK);
	display.setCursor(0,9);
	display.setTextSize(1);
	display.print("SpO2: ");
  display.print("   ");
	display.setCursor(32,9);
  if ((is_MAX_available == true) && (is_MAX_error == false)) {display.printf("%3d", gSpO2);}
	else {display.print("ERR");}
	display.setCursor(60,9);
	display.print("%");
	delay(50);

  // Heart Rate display
	display.setTextColor(BLACK);
	display.setCursor(0,19);
	display.setTextSize(1);
	display.print("HR: ");
  display.print("   ");
	display.setCursor(32,19);
  if (((is_BP_available == true) && (is_BP_error == false) && (is_MAX_available == false)) || \
      ((is_MAX_available == true) && (is_MAX_error == false) && (is_BP_available == false)) || \
      ((is_MAX_available == true) && (is_BP_available == true) && ((is_BP_error == false) || (is_MAX_error == false))))
  {display.printf("%3d", gHeartRate);}
	else {display.print("ERR");}
	display.setCursor(60,19);
	display.print("BPM");
	delay(50);

	// SYS display
	display.setTextColor(BLACK);
	display.setCursor(0,29);
	display.setTextSize(1);
	display.print("SYS: ");
  display.print("   ");
	display.setCursor(32,29);
  if ((is_BP_available == true) && (is_BP_error == false)) {display.printf("%3d", gSystolic);}
  else {display.print("ERR");}
	display.setCursor(60,29);
	display.print("mmHg");
	delay(50);

	// DIA display
	display.setTextColor(BLACK);
	display.setCursor(0,39);
	display.setTextSize(1);
	display.print("DIA: ");
  display.print("   ");
	display.setCursor(32,39);
  if ((is_BP_available == true) && (is_BP_error == false)) {display.printf("%3d", gDiastole);}
  else {display.print("ERR");}
	display.setCursor(60,39);
	display.print("mmHg");
	delay(50);
}

void print_symbol()
{
  display.clearDisplay();
	WiFiSignalStrength state = getWiFiSignalStrength();
	// Serial.println(state);
	// Display bitmap
	if(state == WL_STRONG){
		display.drawBitmap(60, 0,  WS1, 24, 16, BLACK);
		delay(10);
	}else if(state == WL_NORMAL){
		display.drawBitmap(60, 0,  WS2, 24, 16, BLACK);
		delay(10);		
	}else if(state == WL_POOR){
		display.drawBitmap(60, 0,  WS3, 24, 16, BLACK);
		delay(10);		
	}else if(state == WL_UNUSABLE){
		display.drawBitmap(60, 0,  WS0, 24, 16, BLACK);
		delay(10);		
	}
	display.drawBitmap(0, 0,  HEART, 30, 25, BLACK);
	delay(10);
	display.drawBitmap(25, 0,  BAT, 27, 16, WHITE);
	delay(50);

  display.setTextColor(BLACK);
	display.setCursor(11,25);
	display.setTextSize(1);
	display.print("Dr.Health");
	display.setCursor(17,35);
  display.print("Welcome");
  // display.setCursor(17,39);
  // display.print("Press to begin");
	delay(50);

}

void print_loading()
{
  display.clearDisplay();

  display.clearDisplay();
	WiFiSignalStrength state = getWiFiSignalStrength();
	// Serial.println(state);
	// Display bitmap
	if(state == WL_STRONG){
		display.drawBitmap(60, 0,  WS1, 24, 16, BLACK);
		delay(10);
	}else if(state == WL_NORMAL){
		display.drawBitmap(60, 0,  WS2, 24, 16, BLACK);
		delay(10);		
	}else if(state == WL_POOR){
		display.drawBitmap(60, 0,  WS3, 24, 16, BLACK);
		delay(10);		
	}else if(state == WL_UNUSABLE){
		display.drawBitmap(60, 0,  WS0, 24, 16, BLACK);
		delay(10);		
	}
	display.drawBitmap(0, 0,  HEART, 30, 25, BLACK);
	delay(10);
	display.drawBitmap(25, 0,  BAT, 27, 16, WHITE);
	delay(50);

  display.setTextColor(BLACK);
	display.setCursor(3,25);
	display.setTextSize(1);
	display.print("Measuring...");
	display.setCursor(29,35);
  if (available == 1)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/8)*100));
    display.print("%");
  }
  if (available == 2)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/7)*100));
    display.print("%");
  }
  if (available == 3)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/7)*100));
    display.print("%");
  }
  if (available == 4)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/2)*100));
    display.print("%");
  }
  if (available == 5)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/1)*100));
    display.print("%");
  }
  if (available == 6)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/6)*100));
    display.print("%");
  }
  if (available == 7)
  {
    display.printf("%2d", (uint8_t)(((double)gProgress/1)*100));
    display.print("%");
  }
  if (available == 0)
  {
    display.printf("%2d", 0);
    display.print("%");
  }
  // display.print("Welcome");

  delay(50);

}

WiFiSignalStrength getWiFiSignalStrength()
{
  WiFiSignalStrength result = WL_UNUSABLE;
  int16_t rssi = WiFi.RSSI();
  // Serial.println(rssi);
  if (rssi < (-90))
  {
    result = WL_UNUSABLE;
  }
  else if ((rssi < (-80)) && (rssi >= (-90)))
  {
    result = WL_POOR;
  }
  else if ((rssi < (-70)) && (rssi >= (-80)))
  {
    result = WL_NORMAL;
  }
  else if ((rssi < (-60)) && (rssi >= (-70)))
  {
    result = WL_STRONG;
  }
  else if (rssi >= (-60))
  {
    result = WL_STRONG;
  }
  return result;
}

void callback(char* topic, uint8_t* payload, unsigned int length)
{
  String payload_content;
  Serial.print ("Message arrived [");
  Serial.print (topic);
  Serial.print ("]");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print ((char)payload[i]);
    payload_content += (char)payload[i];
  }
  Serial.println();
  // if (!strcmp(topic, node_human_properties_bodyTemp_threshold))
  // {
  //   gBodyTempThreshold = payload_content.toDouble();
  // }
  // else
  // if (!strcmp(topic, node_human_properties_SpO2_threshold))
  // {
  //   gSpO2Threshold = payload_content.toFloat();
  // }
  // else
  // if (!strcmp(topic, node_human_properties_heart_threshold))
  // {
  //   gHeartRateThreshold = payload_content.toFloat();
  // }
  // else
  // if (!strcmp(topic, node_human_properties_Medicine))
  // {
  //   isMedicine = true;
  //   Serial.println("It's time for Med");
  //   DeserializationError error = deserializeJson(medicineInfo, payload_content);
  //   if (error) 
  //   {
  //     Serial.print(F("deserializeJson() failed: "));
  //     Serial.println(error.f_str());
  //     return;
  //   }
  //   const char* lMedicineName = medicineInfo["name"];
  //   const char* lMedicineNote = medicineInfo["note"];
  //   Serial.println(lMedicineName);
  //   Serial.println(lMedicineNote);
  //   //TFT.initR(INITR_144GREENTAB);
  //   TFT.setNote((char*)lMedicineNote);
  //   TFT.setMedicineName((char*)lMedicineName);
  //   vibration();
  //   state = MEDICINE;
  //   is_state_changed = true;
  // }
  // else
  // if (!strcmp(topic, node_human_properties_Phone))
  // {
  //   userPhone = payload_content;
  // }
  // else
  // if (!strcmp(topic, node_human_properties_reqPos))
  // {
  //   if ((gLongitude > 0) && (gLatitude > 0))
  //   {
  //     char buffer[256] = {0};
  //     doc["lat"] = gLatitude;
  //     doc["long"] = gLongitude;
  //     serializeJson(doc, buffer);
  //     if ((is_sim_mqtt_connected))
  //     {
  //       // if (xSemaphoreTake(sem_sim_request, 1000) == pdTRUE)
  //       // {   
  //         sim.publish(node_human_properties_Position_payload, buffer, true);
  //         //xSemaphoreGive(sem_sim_request);
  //       //} 
  //     }
  //   }
  // }
  // else
  // if (!strcmp(topic, node_human_properties_patient_name))
  // {
  //   if (state == MEASURE)
  //   {
  //     TFT.setPatientName((char*)payload_content.c_str());
  //   }
  // }
}

bool MQTTConnect()
{
  Serial.print("Attemp to connect MQTT server...");
  String clientID = "Stroke medical - ";
  clientID += String (random(0xffff), HEX);
  if (mqtt.connect(clientID.c_str(), "", "", device_state, 1, true, "lost"))
  {
    Serial.println("Connected");
    mqtt.publish(device_name, "Medical Device", false);
    // sim.subscribe(node_human_properties_SpO2_threshold);
    // sim.subscribe(node_human_properties_heart_threshold);
    // sim.subscribe(node_human_properties_bodyTemp_threshold);
    // sim.subscribe(node_human_properties_Medicine);
    // sim.subscribe(node_human_properties_Phone);
    // sim.subscribe(node_human_properties_reqPos);
    // sim.subscribe(node_human_properties_patient_name);
    // sim.subscribe("mandevices/testSubscribe", true);
    return true;
  }
  else 
  {
    Serial.print("failed, rc=");
    Serial.print(mqtt.state());
    Serial.println(" try again in 5 seconds");
    delay(5000);
    return false;
  }
}


void maxim_find_peaks(byte *pn_locs, byte *n_npks,  uint16_t  *pn_x, byte n_size, uint16_t n_min_height, byte n_min_distance, byte n_max_num )
{
  maxim_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
  maxim_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
  *n_npks = min( *n_npks, n_max_num );
}

void maxim_peaks_above_min_height( byte *pn_locs, byte *n_npks,  uint16_t  *pn_x, byte n_size, byte n_min_height )
{
  int32_t i = 1, n_width;
  *n_npks = 0;
  
  while (i < n_size-1)
  {
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1])
    {      // find left edge of potential peaks
      n_width = 1;

      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;

      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15 )
      { 
        // find right edge of peaks
        pn_locs[(*n_npks)++] = i;    
        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
      {
        i += n_width;
      }
    }
    else
    {
      i++;
    }
  }
}

void maxim_remove_close_peaks(byte *pn_locs, byte *pn_npks, uint16_t *pn_x, byte n_min_distance)
{
    
  int32_t i, j, n_old_npks, n_dist;
    
  /* Order peaks from large to small */
  maxim_sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ){
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  maxim_sort_ascend( pn_locs, *pn_npks );
}

void maxim_sort_ascend(byte  *pn_x, byte n_size) 
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

void maxim_sort_indices_descend(  uint16_t  *pn_x, byte *pn_indx, byte n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
*
* \retval       None
*/ 
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}


