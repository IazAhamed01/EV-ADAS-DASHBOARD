/**
 ******************************************************************************
 * @file    ultrasonic.c
 * @brief   HC-SR04 blocking driver.
 *
 * Uses TIM2 running at 1 MHz (1 tick = 1 us).
 ******************************************************************************/

#include "ultrasonic.h"

#define ULTRASONIC_TIMEOUT_US   25000U

static TIM_HandleTypeDef *s_htim = NULL;

void Ultrasonic_Init(TIM_HandleTypeDef *htim)
{
    s_htim = htim;
}

static uint32_t Ultrasonic_Now(void)
{
    return __HAL_TIM_GET_COUNTER(s_htim);
}

float Ultrasonic_ReadCM(const Ultrasonic_Sensor_t *sensor)
{
    uint32_t start;
    uint32_t rise_tick;
    uint32_t fall_tick;
    uint32_t pulse_us;
    float distance;

    /* --------------------------------------------------------- */
    /* Ensure TRIG is LOW before starting                        */
    /* --------------------------------------------------------- */

    HAL_GPIO_WritePin(sensor->trig_port,
                      sensor->trig_pin,
                      GPIO_PIN_RESET);

    start = Ultrasonic_Now();
    while((uint32_t)(Ultrasonic_Now() - start) < 2);

    /* --------------------------------------------------------- */
    /* Send 10 us trigger pulse                                  */
    /* --------------------------------------------------------- */

    HAL_GPIO_WritePin(sensor->trig_port,
                      sensor->trig_pin,
                      GPIO_PIN_SET);

    start = Ultrasonic_Now();
    while((uint32_t)(Ultrasonic_Now() - start) < 10);

    HAL_GPIO_WritePin(sensor->trig_port,
                      sensor->trig_pin,
                      GPIO_PIN_RESET);

    /* Small settling delay */
    HAL_Delay(1);

    /* --------------------------------------------------------- */
    /* Wait for echo HIGH                                        */
    /* --------------------------------------------------------- */

    start = Ultrasonic_Now();

    while(HAL_GPIO_ReadPin(sensor->echo_port,
                           sensor->echo_pin) == GPIO_PIN_RESET)
    {
        if((uint32_t)(Ultrasonic_Now() - start) > ULTRASONIC_TIMEOUT_US)
        {
            return ULTRASONIC_MAX_CM;
        }
    }

    rise_tick = Ultrasonic_Now();

    /* --------------------------------------------------------- */
    /* Wait for echo LOW                                         */
    /* --------------------------------------------------------- */

    while(HAL_GPIO_ReadPin(sensor->echo_port,
                           sensor->echo_pin) == GPIO_PIN_SET)
    {
        if((uint32_t)(Ultrasonic_Now() - rise_tick) > ULTRASONIC_TIMEOUT_US)
        {
            return ULTRASONIC_MAX_CM;
        }
    }

    fall_tick = Ultrasonic_Now();

    /* --------------------------------------------------------- */
    /* Handle timer overflow                                     */
    /* --------------------------------------------------------- */

    if(fall_tick >= rise_tick)
    {
        pulse_us = fall_tick - rise_tick;
    }
    else
    {
        pulse_us = (0xFFFFU - rise_tick) + fall_tick + 1U;
    }

    /* --------------------------------------------------------- */
    /* Convert pulse width to distance                           */
    /* --------------------------------------------------------- */

    distance = pulse_us / 58.0f;

    /* Reject invalid measurements */

    if(distance < 2.0f)
    {
        return ULTRASONIC_MAX_CM;
    }

    if(distance > ULTRASONIC_MAX_CM)
    {
        return ULTRASONIC_MAX_CM;
    }

    return distance;
}
