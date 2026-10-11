/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"      
#include "adc.h"      
#include "gpio.h"  
#include "i2c.h"
#include "portable.h"
#include "oled.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint32_t temp;  
    float volt;   
    uint8_t alarm_test; /* 1 = simulated alarm, not measured over-temperature */
} dat_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ALARM_STARTUP_TEST_ENABLE  1U /* Set to 0 for normal operation. */
#define ALARM_STARTUP_TEST_MS      5000U
#define DISPLAY_INTERVAL_MS       100U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint8_t breath_enable = 0;
volatile uint32_t ctrl_queue_send_fail = 0;
volatile uint32_t disp_queue_send_fail = 0;
volatile uint32_t adc_sample_fail = 0;
/* USER CODE END Variables */
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for ControlTask */
osThreadId_t ControlTaskHandle;
const osThreadAttr_t ControlTask_attributes = {
  .name = "ControlTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};
/* Definitions for BreathTask */
osThreadId_t BreathTaskHandle;
const osThreadAttr_t BreathTask_attributes = {
  .name = "BreathTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for IndicatorTask */
osThreadId_t IndicatorTaskHandle;
const osThreadAttr_t IndicatorTask_attributes = {
  .name = "IndicatorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for CtrlQueue */
osMessageQueueId_t CtrlQueueHandle;
const osMessageQueueAttr_t CtrlQueue_attributes = {
  .name = "CtrlQueue"
};
/* Definitions for DispQueue */
osMessageQueueId_t DispQueueHandle;
const osMessageQueueAttr_t DispQueue_attributes = {
  .name = "DispQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Buzzer_On(void);
void Buzzer_Off(void);
/* USER CODE END FunctionPrototypes */

void StartSensorTask(void *argument);
void StartControlTask(void *argument);
void StartBreathTask(void *argument);
void StartDisplayTask(void *argument);
void StartIndicatorTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

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

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of CtrlQueue */
  CtrlQueueHandle = osMessageQueueNew (16, sizeof(dat_t), &CtrlQueue_attributes);

  /* creation of DispQueue */
  DispQueueHandle = osMessageQueueNew (16, sizeof(dat_t), &DispQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  if ((CtrlQueueHandle == NULL) || (DispQueueHandle == NULL)) {
    Error_Handler();
  }
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of ControlTask */
  ControlTaskHandle = osThreadNew(StartControlTask, NULL, &ControlTask_attributes);

  /* creation of BreathTask */
  BreathTaskHandle = osThreadNew(StartBreathTask, NULL, &BreathTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* creation of IndicatorTask */
  IndicatorTaskHandle = osThreadNew(StartIndicatorTask, NULL, &IndicatorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if ((SensorTaskHandle == NULL) || (ControlTaskHandle == NULL) ||
      (BreathTaskHandle == NULL) || (DisplayTaskHandle == NULL) ||
      (IndicatorTaskHandle == NULL)) {
    Error_Handler();
  }
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief  Function implementing the SensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  const uint32_t test_start = osKernelGetTickCount();
  const uint32_t tick_hz = osKernelGetTickFreq();
  const uint32_t test_ticks = (tick_hz * ALARM_STARTUP_TEST_MS) / 1000U;
  const uint32_t display_ticks = (tick_hz * DISPLAY_INTERVAL_MS) / 1000U;
  uint32_t last_display = test_start - display_ticks;
  dat_t dat = {0}; /* Retain the last valid light sample on ADC errors. */
  /* Infinite loop */
  for(;;)
  {
    //声明必要结构体

    //判断温度
    dat.temp = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET) ? 1 : 0;
    dat.alarm_test = (ALARM_STARTUP_TEST_ENABLE &&
                     ((uint32_t)(osKernelGetTickCount() - test_start) < test_ticks)) ? 1U : 0U;
    
    //判断亮度，ADC转换并计算电压
    if ((HAL_ADC_Start(&hadc1) == HAL_OK) &&
        (HAL_ADC_PollForConversion(&hadc1, 5U) == HAL_OK)) {
        uint32_t dr = HAL_ADC_GetValue(&hadc1);
        dat.volt = dr * 3.3f / 4095.0f;
    } else {
        adc_sample_fail++;
    }
    HAL_ADC_Stop(&hadc1);
    
    //将数据发送到控制和显示队列
    if (osMessageQueuePut(CtrlQueueHandle, &dat, 0, 0) != osOK) {
        ctrl_queue_send_fail++;
    }
    if ((uint32_t)(osKernelGetTickCount() - last_display) >= display_ticks) {
        last_display = osKernelGetTickCount();
        if (osMessageQueuePut(DispQueueHandle, &dat, 0, 0) != osOK) {
            disp_queue_send_fail++;
        }
    }
		
    //延时
    osDelay(10); 
  }
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartControlTask */
/**
* @brief Function implementing the ControlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartControlTask */
void StartControlTask(void *argument)
{
  /* USER CODE BEGIN StartControlTask */

  //接收变量
  dat_t rx_dat;
  Buzzer_Off(); /* Establish a driven inactive level before processing alarms. */
  
  for(;;)
  {
    //从控制队列接收数据（阻塞等待）
    if (osMessageQueueGet(CtrlQueueHandle, &rx_dat, NULL, osWaitForever) == osOK)
    {
        /* ---------- 温度 ---------- */
        if ((rx_dat.temp == 1) || rx_dat.alarm_test) {
            Buzzer_On();
            breath_enable = 1;
        } 
        else {                 // Normal temperature
            Buzzer_Off();
            breath_enable = 0;
        }

        /* ---------- 亮度 ---------- */
        if (rx_dat.volt < 2.0f) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // OFF
        } 
        else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);   // ON
        }
    }
  }
  /* USER CODE END StartControlTask */
}

/* USER CODE BEGIN Header_StartBreathTask */
/**
* @brief Function implementing the BreathTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartBreathTask */
void StartBreathTask(void *argument)
{
  /* USER CODE BEGIN StartBreathTask */
  /* Current project uses an 8 MHz TIM1 clock: 1 kHz PWM, 1000 duty steps. */
  __HAL_TIM_SET_PRESCALER(&htim1, 7U);
  __HAL_TIM_SET_AUTORELOAD(&htim1, 999U);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
  HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);
  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  const float PI = 3.1415926f;
  
  for(;;)
  {
    if (breath_enable) {
        float t = xTaskGetTickCount() * portTICK_PERIOD_MS * 0.001f;
        float duty = 0.5f * sinf(2.0f * PI * t) + 0.5f; // [0.0, 1.0]
        uint32_t ccr = (uint32_t)(duty * (__HAL_TIM_GET_AUTORELOAD(&htim1) + 1));
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
    } 
    else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0); 
    }

    //延时
    osDelay(10); 
  }
  /* USER CODE END StartBreathTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */

  //接收变量
  dat_t rx_dat;
  
  //OLED初始化
  OLED_Init(&hi2c1, 0x3CU);

  for(;;)
  {
    if (osMessageQueueGet(DispQueueHandle, &rx_dat, NULL, osWaitForever) == osOK)
    {
      dat_t latest;
      /* Only display the newest queued sample, not an old backlog. */
      while (osMessageQueueGet(DispQueueHandle, &latest, NULL, 0) == osOK) {
        rx_dat = latest;
      }
      OLED_ShowSurveyScreen(rx_dat.alarm_test, (uint8_t)rx_dat.temp,
                            rx_dat.volt);
    }
  }
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartIndicatorTask */
/**
* @brief Function implementing the IndicatorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartIndicatorTask */
void StartIndicatorTask(void *argument)
{
  /* USER CODE BEGIN StartIndicatorTask */
  for(;;)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    osDelay(500);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    osDelay(500);
  }
  /* USER CODE END StartIndicatorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static uint8_t buzzer_pwm_started = 0;

void Buzzer_On(void)
{
    if (!buzzer_pwm_started) {
        if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) == HAL_OK) {
            buzzer_pwm_started = 1;
        }
    }
    /* Keep the existing tone frequency; use 50% duty instead of 5/1000. */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                          (__HAL_TIM_GET_AUTORELOAD(&htim3) + 1U) / 2U);
}

void Buzzer_Off(void)
{
    /* PWM1 + existing LOW polarity: CCR=0 drives HIGH (PNP module off).
       Do not leave the module input floating by disabling the channel. */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
    if (!buzzer_pwm_started) {
        if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) == HAL_OK) {
            buzzer_pwm_started = 1;
        }
    }
}

/* USER CODE END Application */

