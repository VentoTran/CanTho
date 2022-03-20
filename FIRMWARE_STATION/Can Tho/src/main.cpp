#include <Arduino.h>
#include "topic.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "MAX30102.h"
#include "topic.h"
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

typedef enum
{
  WL_STRONG,
  WL_NORMAL,
  WL_POOR,
  WL_UNUSABLE
}WiFiSignalStrength;

uint16_t ADC1_RAW[DATA_LENGTH];
byte peak_loc[5];

double ADC1_val = 0;
int count = 0;
bool measure = false;
bool peak = false;
byte num_peak = 0;

bool is_measure = false;
bool is_MAX_done = false;
bool is_BP_done = false;

const char* PASS = "12345678";
const char* ID;
const char* mqtt_server = "hotrodotquy.vn";

WiFiClient espClient;
PubSubClient mqtt(espClient);
MAXClass MAX;
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

void print_para();
void print_symbol(void);
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

  while (!MAX.begin()) {Serial.println("Cannot begin MAX");}
  Serial.println("MAX Start!");
  MAX.setup(0x1F, 4, 3, 400, 411, 4096);

  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
  mqtt.setKeepAlive(60);

  //Initialize Display
	display.begin();
	// you can change the contrast around to adapt the display for the best viewing!
	display.setContrast(10);
	// Clear the buffer.
	display.clearDisplay();

  pinMode(Valve, OUTPUT);
  pinMode(Motor, OUTPUT);
  pinMode(Buzz, OUTPUT);

  digitalWrite(Valve, LOW);
  digitalWrite(Motor, LOW);
  digitalWrite(Buzz, LOW);

  pinMode(ADC1, INPUT);
  pinMode(BT, INPUT);

  Buzzer(1, 100);

  WiFiConnect();

  Serial.println("");
  delay(1000);
  Buzzer(2, 200);

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

  // Serial.println("Loop");

  if (is_measure == true)
  {
    vTaskResume(xBPTaskHandle);
    vTaskResume(xMAXTaskHandle);
    Buzzer(1, 100);
    is_measure = false;
  }

  if ((is_BP_done == true) && (is_MAX_done == true))
  {
    if ((HeartRate1 == 0) || (HeartRate2 == 0)) {gHeartRate = 0;}
    else {gHeartRate = (uint8_t)((HeartRate1 + HeartRate2) / 2);}

    Buzzer(3, 100);
    display.clearDisplay();
    print_para();
    display.display();
    mqtt.publish(node_human_properties_SpO2_payload, ((String)gSpO2).c_str(), true);
    mqtt.publish(node_human_properties_heartRate_payload , ((String)gHeartRate).c_str(), true);
    mqtt.publish(node_human_properties_bodyTemp_payload, ((String)gBodyTemp).c_str(), true);
    mqtt.publish(node_human_properties_systolic_payload, ((String)gSystolic).c_str(), true);
    mqtt.publish(node_human_properties_diastole_payload, ((String)gDiastole).c_str(), true);
    Buzzer(1, 500);
    is_BP_done = false;
    is_MAX_done = false;
    vTaskResume(xBTTaskHandle);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiConnect();
  }
  
  display.clearDisplay();
  print_para();
	print_symbol();
	display.display();

}

void MQTT_Task(void* parameter)
{
  while(1)
  {
    if (!mqtt.connected())
    {
      MQTTConnect();
    }
    delay(1000);
  }
}

void BP_Task(void* parameter)
{
  while(1)
  {
    vTaskSuspend(xBPTaskHandle);
    digitalWrite(Motor, HIGH);
    digitalWrite(Valve, HIGH);

    bool BP_measuring = false;
    while (BP_measuring == false)
    {
      ADC1_val = (float)analogRead(ADC1)*(3.3/4095);
      // Serial.print("ADC1 = ");
      // Serial.println(ADC1_val);
      if(ADC1_val >= 3.2)
      {
        digitalWrite(Motor, LOW);
        delay(200);
        BP_measuring = true;
      }
    }

    for(count = 0; count < DATA_LENGTH; count++)
    {
      ADC1_RAW[count] = (uint16_t) analogRead(ADC1);
      delay(deltaT);
    }

    // for(count = 0; count < DATA_LENGTH; count++)
    // {
    //   Serial.print(count);
    //   Serial.print("\t");
    //   Serial.println(ADC1_RAW[count]);
    // }

    maxim_find_peaks(peak_loc, &num_peak, ADC1_RAW, DATA_LENGTH, 3000, 20, 10);

    // for(count = 0; count < num_peak; count++)
    // {
    //   Serial.println(peak_loc[count]);
    // }

    uint16_t Calib_ADC1[DATA_LENGTH];

    for(int i = 0; i < DATA_LENGTH; i++)
    {
      Calib_ADC1[i] = (uint16_t)ADC1_RAW[i] + (uint16_t)(5.75 * i);
    }

    Calib_ADC1[0] = ADC1_RAW[0];

    // for(int i = 1; i < DATA_LENGTH; i++)
    // {
    //   Serial.println(Calib_ADC1[i]);
    // }

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

    HeartRate1 = (uint8_t)(60000 / (avrStepLoc * deltaT));

    Serial.println(HeartRate1);

    gSystolic = 0;
    gSystolic = (ADC1_RAW[peak_loc[0]] + 300) * 3.31 * 300 / (4095 * 90.28 * 0.08);
    Serial.println(gSystolic);

    gDiastole = 0;
    gDiastole = (ADC1_RAW[peak_loc[1] - 3] - 500) * 3.31 * 300 / (4095 * 90.28 * 0.08);
    Serial.println(gDiastole);

    if (HeartRate1 <= 40)
    {
      gSystolic = 0;
      gDiastole = 0;
      HeartRate1 = 0;
    }

    is_BP_done = true;
    digitalWrite(Valve, LOW);
  }
}

void MAX_Task(void* parameter)
{
  while(1)
  {
    vTaskSuspend(xMAXTaskHandle);

    while (!MAX.begin()) {Serial.println("Cannot begin MAX");}
    Serial.println("MAX Start!");
    MAX.setup(0x1F, 4, 3, 400, 411, 4096);
    
    if(MAX.measureParameter() == true)
    {
      gBodyTemp = MAX.Body_Temp;
      gSpO2 = MAX.SpO2;
      HeartRate2 = MAX.Heart_Rate;
      is_MAX_done = true;
    }
    else
    {
      gBodyTemp = 0;
      gSpO2 = 0;
      HeartRate2 = 0;
      is_MAX_done = false;
    }
  }
}

void BT_Task(void* parameter)
{
  while(1)
  {
    if ((digitalRead(BT) == HIGH) && (is_measure == false)) 
    {
      delay(200);
      if (digitalRead(BT) == HIGH) 
      {
        Serial.println("Button Pressed!");
        is_measure = true;
        vTaskSuspend(xBTTaskHandle);
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

	// SYS display
	display.setTextColor(BLACK);
	display.setCursor(0,27);
	display.setTextSize(1);
	display.print("SYS: ");
	display.setCursor(26,27);
	display.print(gSystolic);
	display.setCursor(60,27);
	display.print("mmHg");
	delay(50);

	// SYS display
	display.setTextColor(BLACK);
	display.setCursor(0,39);
	display.setTextSize(1);
	display.print("DIA: ");
	display.setCursor(26,39);
	display.print(gDiastole);
	display.setCursor(60,39);
	display.print("mmHg");
	delay(50);
}

void print_symbol()
{
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
	// display.drawBitmap(36, 0,  BAT, 27, 16, BLACK);
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
  else if (rssi < (-70))
  {
    result = WL_POOR;
  }
  else if (rssi < (-35))
  {
    result = WL_NORMAL;
  }
  else if (rssi < 0)
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
    mqtt.publish(device_name, "stroke-medical", true);
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


