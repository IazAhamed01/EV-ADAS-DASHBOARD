/**
 ******************************************************************************
 * @file    uart_shell.h
 * @brief   USART1 telemetry transmitter + interrupt-driven command shell.
 ******************************************************************************
 */
#ifndef __UART_SHELL_H
#define __UART_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "ev_control.h"
#include "adas.h"

void Shell_Init(UART_HandleTypeDef *huart);

/** Sends the two structured telemetry frames described in the README. */
void Shell_SendTelemetry(const EV_State_t *ev, const ADAS_State_t *adas, uint8_t fault_byte);

/** Call once per main-loop iteration; processes any complete line received. */
void Shell_Poll(void);

/** Must be called from HAL_UART_RxCpltCallback() in main.c / stm32f1xx_it.c */
void Shell_RxCallback(UART_HandleTypeDef *huart);

#endif /* __UART_SHELL_H */
