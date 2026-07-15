#ifndef ULTRASONIC_H
#define ULTRASONIC_H
#define ULTRASONIC_MAX_CM 400.0f

#include "stm32f1xx_hal.h"

typedef struct {
    GPIO_TypeDef *trig_port;
    uint16_t      trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t      echo_pin;
} Ultrasonic_Sensor_t;

void  Ultrasonic_Init(TIM_HandleTypeDef *htim);
float Ultrasonic_ReadCM(const Ultrasonic_Sensor_t *sensor);

#endif
