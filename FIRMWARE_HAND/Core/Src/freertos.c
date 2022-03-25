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
#include "stdio.h"

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
uint8_t gScore = 0;

uint8_t is_test = 0;
uint8_t is_done = 0;
uint8_t is_fall = 0;
uint8_t is_MPU_OK = 0;

// char call[25] = "ATD+84816056426;\r\n";
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

  //  Khoi tao cac tham so cho ket noi qua giao thuc MQTT
  SIM800.sim.apn = "m3-world";
  SIM800.sim.apn_user = "mms";
  SIM800.sim.apn_pass = "mms";
  SIM800.mqttServer.host = "hotrodotquy.vn";
  SIM800.mqttServer.port = 1883;
  SIM800.mqttClient.username = "";
  SIM800.mqttClient.clientID = "hm";
  SIM800.mqttClient.pass = "";
  SIM800.mqttClient.keepAliveInterval = 120;

  // Chuan bi nghe phan hoi tu module SIM
  osDelay(100);
  HAL_UART_Receive_IT(UART_SIM800, &rx_data, 1);


  //Kiem tra module SIM hoat dong chua? Neu chua thi bat module SIM
  if (SIM800_SendCommand("AT\r\n", "OK\r\n", CMD_DELAY) == SIM_R_nOK)
  {
    HAL_GPIO_WritePin(SIM_PWRKEY_GPIO_Port, SIM_PWRKEY_Pin, 1);
    osDelay(2000);
    HAL_GPIO_WritePin(SIM_PWRKEY_GPIO_Port, SIM_PWRKEY_Pin, 0);
  }
  
  osDelay(4000);
  // Reset CLK timeout
  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  // Bat dau ket noi MQTT
  MQTT_Init();

  // Reset CLK timeout
  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  // 4 lenh cai dat chuc nang cho module SIM
  SIM800_SendCommand("AT+CMGF=1\r\n", "OK\r\n", 1000);

  SIM800_SendCommand("AT+CFUN=1\r\n", "OK\r\n", 1000);

  // SIM800_SendCommand("AT+CUSD=1,\"*101#\"\r\n", "OK\r\n", 5000);

  // SIM800_SendCommand("AT+CSCS?\r\n", "OK\r\n", 1000);
  SIM800_SendCommand("AT+CSCS=\"GSM\"\r\n", "OK\r\n", 1000);
  // SIM800_SendCommand("AT+CSDH?\r\n", "OK\r\n", 1000);
  SIM800_SendCommand("AT+CSMP=17,167,0,0\r\n", "OK\r\n", 1000);
  
  // Reset CLK timeout
  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);

  // Khoi tao timer 1s Reset CLK tu dong
  osTimerStart(SecTimerHandle, 1000);
  
  // Ban tin ket noi trang thai dau tien
  MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
  osDelay(2000);
  uint32_t time = HAL_GetTick();

  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
    if (is_fall == 1)
    {
      // BENH NHAN DA NGA, GUI TIN NHAN
      if (SIM800_SendCommand("AT+CMGS=\"0855484556\"\r\n", ">", 1000) == SIM_R_OK)
      {
        osDelay(200);
        HAL_UART_Transmit_IT(UART_SIM800, (uint8_t *)SMSStrg, (uint16_t)strlen(SMSStrg));
        osDelay(200);
        huart1.Instance->DR = 0b00011010;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        osDelay(5000);
        // GUI BAN TIN MQTT BENH NHAN NGA
        MQTT_Pub("mandevices/stroke-medical/human/fall", "1");
        osDelay(2000);
        MQTT_Pub("mandevices/stroke-medical/human/fall/loc", Loc);
        osDelay(2000);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        // HAL_UART_Transmit_IT(UART_SIM800, call, strlen(call));
        // GOI DIEN THOAI
        SIM800_SendCommand("ATD0855484556;\r\n", "OK\r\n", 2000);
        osDelay(30000);
        SIM800_SendCommand("ATH\r\n", "OK\r\n", 1000);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        time = HAL_GetTick();
        is_fall = 0;
      }
    }
    if ((HAL_GetTick() - time) >= 30000)
    {
      // ban tin ping moi 30s
      MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
      time = HAL_GetTick();
    }
    if ((SIM800.mqttServer.connect == 0) || (SIM800_Status.SIM == 0) || (SIM800_Status.GPRS == 0) || (SIM800_Status.MQTT == 0))
    {
      // ket noi lai MQTT
      MQTT_Init();
      MQTT_Pub("mandevices/stroke-medical/human/fall", "0");
      time = HAL_GetTick();
    }
    if (is_done == 1)
    {
      // Da test xong, truyen diem test len Server
      is_done = 0;
      char str[2];
      sprintf(str, "%d\0", gScore);
      MQTT_Pub("mandevices/stroke-medical/human/test/score", str);
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      time = HAL_GetTick();
    }
    // SIM800_SendCommand("AT+CGPSINF=0\r\n", "OK\r\n", 10000);
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    // 0855484556
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

  osDelay(1000);
  // Khoi tao cam bien mpu6050
  while(MPU6050_Init(&hi2c1) == MPU_nOK);
  // osTimerStart(SecTimerHandle, 1000);
  is_MPU_OK = 1;
  float data[20];
  /* Infinite loop */
  for(;;)
  {
    // READ SENSOR
    MPU6050_Read_Accel(&hi2c1, &MPU6050);

    // CALCULATE GRAVITY
    gGravity = sqrt((MPU6050.Ax)*(MPU6050.Ax) + (MPU6050.Ay)*(MPU6050.Ay) + (MPU6050.Az)*(MPU6050.Az));

    if (gGravity == 0)
    {
      // Gravity = 0 la cam bien k chay, smthg went wrong
      is_MPU_OK = 0;
    }
    if ((gGravity < 0.20) && (is_fall == 0) && (gGravity != 0))
    {
      // CHECK NGA, BENH NHAN DA NGA
      is_fall = 1;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      // vTaskResume(MQTTHandle);
    }
    if (is_test == 1)
    {
      // TEST
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      while(MPU6050_Init(&hi2c1) == MPU_nOK);
      is_test = 0;
      for (uint8_t i = 0; i < 20; i++)
      {
        MPU6050_Read_Accel(&hi2c1, &MPU6050);
        data[i] = MPU6050.Az;
        osDelay(500);
      }
      uint8_t lowestAngelPos = 0;
      float error = 0;
      for(uint8_t i = 1; i < 20; i++)
      {
        error = data[lowestAngelPos] - data[i];
        if (error >= 0.03) 
        {
          lowestAngelPos = i;
        }
      }
      if (data[lowestAngelPos] > 0.9)
      {
        gScore = 0;
      }
      else
      {
        if (((data[lowestAngelPos] < 0.6) && (lowestAngelPos <= 10)) || (data[lowestAngelPos] < 0.4))
        {
          gScore = 3;
        }
        else if (lowestAngelPos < 10)
        {
          gScore = 1;
        }
        else
        {
          gScore = 2;
        }
      }
      uint8_t dead = 1;
      for(uint8_t i = 0; i < 20; i++)
      {
        if(data[i] > 0.25)
        {
          dead = 0;
        }
      }
      if(dead)
      {
        gScore = 4;
      }
      is_done = 1;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }

    // RESET CLK
    HAL_IWDG_Refresh(&hiwdg);
    osDelay(20);
  }
  /* USER CODE END Sensor_Task */
}


/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */
  // TIMER 1s RESET CLK
  if (is_MPU_OK == 1)     HAL_IWDG_Refresh(&hiwdg);
  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if ((is_test == 0) && (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == 0))   
  {
    for (int i = 0; i < 100000; i++);
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == 0)
    {
      // BUTTON PRESSED, RAISE FLAG
      is_test = 1;
    }
  }
}
/* USER CODE END Application */

