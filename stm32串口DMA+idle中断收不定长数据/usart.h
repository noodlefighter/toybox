/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#define		RXBUFFERSIZE	350
#define		DEBUG_RX_MAX		RXBUFFERSIZE
#define		DEBUG_RX_BUF		USART1_RX_BUF	//Debug接收缓存
#define		DEBUG_RX_COUNT		Usart1RXCount	//Debug接收总数
#define 	DEBUG_RX_FLAG		Usart1RXFlag	//Debug接收完成标志

#define		CMD_RX_MAX			RXBUFFERSIZE
#define		CMD_RX_BUF			USART2_RX_BUF	//Debug接收缓存
#define		CMD_RX_COUNT		Usart2RXCount	//Debug接收总数
#define 	CMD_RX_FLAG			Usart2RXFlag	//Debug接收完成标志
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
extern uint8_t Usart1RxBuffer[RXBUFFERSIZE];
extern uint8_t Usart2RxBuffer[RXBUFFERSIZE];	//DMA接收缓存

extern uint8_t USART1_RX_BUF[350]; 	      //发送缓冲,最大256字节
extern uint8_t USART2_RX_BUF[350]; 	 

extern uint16_t Usart1RXCount;
extern uint8_t Usart1RXFlag;

extern uint16_t Usart2RXCount;
extern uint8_t Usart2RXFlag;
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void u1_printf(char* fmt,...);
void TaskForObserUsart1Rec(void);

void u2_printf(char* fmt,...);


void CMDSendData(uint8_t *buf, uint16_t len);

void ClearDebugBuff(void);
void ClearCmdBuff(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
