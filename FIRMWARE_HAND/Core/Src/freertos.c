/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "MQTTSim800.h"
#include "mpu6050.h"
#include "math.h"
#include "string.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct
{
  uint8_t SIM;
  uint8_t GPRS;
  uint8_t MQTT;
} SIM800_St;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SIM_R_OK    0
#define SIM_R_nOK   1

#define MPU_OK  0
#define MPU_nOK 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;
extern IWDG_HandleTypeDef hiwdg;
extern SIM800_t SIM800;
extern uint8_t rx_data;
extern SIM800_St SIM800_Status;

MPU6050_t MPU6050;

float gGravity = 0.0;
uint8_t is_fall = 0;
uint8_t is_MPU_OK = 0;

char call[25] = "ATD+84816056426;\r\n";
char SMSStrg[110] = "BENH NHAN DA BI NGA!\nToa do benh nhan: https://www.google.com/maps/search/?api=1&query=10.035911,105.783660\n";
// char InfoBV1[110] = "Benh vien Da khoa Hoan My Cuu Long, SDT: 02923917901\nBenh vien Da khoa S.I.S Can Tho, SDT: 18001115\n";
// char InfoBV2[110] = "Benh vien Da khoa Thanh pho Can Tho, SDT: 02923821235\nBenh vien Da khoa Trung uong Can Tho, SDT: 0901215115\n";
char Loc[25] = "10.035911,105.783660";

/* USER CODE END Variables */
/* Definitions for MQTT */
osThreadId_t MQTTHandle;
const osThreadAttr_t MQTT_attributes = {
  .name = "MQTT",
  .stack_size = 250 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SENSORs */
osThreadId_t SENSORsHandle;
const osThreadAttr_t SENSORs_attributes = {
  .name = "SENSORs",
  .stack_size = 150 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SecTimer */
osTimerId_t SecTimerHandle;
const osTimerAttr_t SecTimer_attributes = {
  .name = "SecTimer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */



/* USER CODE END FunctionPrototypes */

void MQTT_Task(void *argument);
void Sensor_Task(void *argument);
void Callback01(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of SecTimer */
  SecTimerHandle = osTimerNew(Callback01, osTimerPeriodic, NULL, &SecTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of MQTT */
  MQTTHandle = osThreadNew(MQTT_Task, NULL, &MQTT_attributes);

  /* creation of SENSORs */
  SENSORsHandle = osThreadNew(Sensor_Task, NULL, &SENSORs_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_MQTT_Task */
/**
  * @brief  Function implementing the MQTT thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_MQTT_Task */
void MQTT_Task(void *argument)
{
  /* USER CODE BEGIN MQTT_Task */

  SIM800.sim.apn = "m3-world";
  SIM800.sim.apn_user = "mms";
  SIM800.sim.apn_pass = "mms";
  SIM800.mqttServer.host = "hotrodotquy.vn";
  SIM800.mqttServer.port = 1883;
  SIM800.mqttClient.username = "";
  SIM800.mqttClient.clientID = "hm";
  SIM800.mqttClient.pass = "";
  SIM800.mqttClient.keepAliveInterval = 120;

  osDelay(100);

  if (SIM800_SendCommand("AT\r\n", "\r\n", CMD_DELAY) == SIM_R_nOK)
  {
    HAL_GPIO_WritePin(SIM_PWRKEY_GPIO_Port, SIM_PWRKEY_Pin, 1);
    osDelay(2000);
    HAL_GPIO_WritePin(SIM_PWRKEY_GPIO_Port, SIM_PWRKEY_Pin, 0);
  }
  
  osDelay(4000);

  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  MQTT_Init();

  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  SIM800_SendCommand("AT+CMGF=1\r\n", "OK\r\n", 1000);

  SIM800_SendCommand("AT+CFUN=1\r\n", "OK\r\n", 1000);

  // SIM800_SendCommand("AT+CUSD=1,\"*101#\"\r\n", "OK\r\n", 5000);

  // SIM800_SendCommand("AT+CSCS?\r\n", "OK\r\n", 1000);
  SIM800_SendCommand("AT+CSCS=\"GSM\"\r\n", "OK\r\n", 1000);
  // SIM800_SendCommand("AT+CSDH?\r\n", "OK\r\n", 1000);
  SIM800_SendCommand("AT+CSMP=17,167,0,0\r\n", "OK\r\n", 1000);
  

  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  osTimerStart(SecTimerHandle, 1000);

  MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
  uint32_t time = HAL_GetTick();

  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
    if (is_fall == 1)
    {
      if (SIM800_SendCommand("AT+CMGS=\"0855484556\"\r\n", ">", 1000) == SIM_R_OK)
      {
        osDelay(200);
        HAL_UART_Transmit_IT(UART_SIM800, (uint8_t *)SMSStrg, (uint16_t)strlen(SMSStrg));
        osDelay(200);
        huart1.Instance->DR = 0b00011010;
        osDelay(2000);
        MQTT_Pub("mandevices/stroke-medical/human/fall", "1");
        osDelay(2000);
        MQTT_Pub("mandevices/stroke-medical/human/fall/loc", Loc);
        osDelay(2000);
        // HAL_UART_Transmit_IT(UART_SIM800, call, strlen(call));
        SIM800_SendCommand("ATD0855484556;\r\n", "OK\r\n", 2000);
        
        osDelay(30000);
        SIM800_SendCommand("ATH\r\n", "OK\r\n", 1000);
        is_fall = 0;
      }
    }
    if ((HAL_GetTick() - time) >= 30000)
    {
      MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
      time = HAL_GetTick();
    }
    if((SIM800.mqttServer.connect == 0) || (SIM800_Status.SIM == 0) || (SIM800_Status.GPRS == 0) || (SIM800_Status.MQTT == 0))
    {
      MQTT_Init();
      MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
      time = HAL_GetTick();
    }
    // 84914812925
    // SIM800_SendCommand("AT+CGPSINF=0\r\n", "OK\r\n", 10000);
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  }
  /* USER CODE END MQTT_Task */
}

/* USER CODE BEGIN Header_Sensor_Task */
/**
* @brief Function implementing the SENSORs thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Sensor_Task */
void Sensor_Task(void *argument)
{
  /* USER CODE BEGIN Sensor_Task */

  while(MPU6050_Init(&hi2c1) == MPU_nOK);
  // osTimerStart(SecTimerHandle, 1000);
  is_MPU_OK = 1;

  /* Infinite loop */
  for(;;)
  {
    MPU6050_Read_Accel(&hi2c1, &MPU6050);

    gGravity = sqrt((MPU6050.Ax)*(MPU6050.Ax) + (MPU6050.Ay)*(MPU6050.Ay) + (MPU6050.Az)*(MPU6050.Az));

    if ((gGravity < 0.10) && (is_fall == 0) && (gGravity != 0))
    {
      is_fall = 1;
      // vTaskResume(MQTTHandle);
    }
    HAL_IWDG_Refresh(&hiwdg);
    osDelay(10);
  }
  /* USER CODE END Sensor_Task */
}

/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */
  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

