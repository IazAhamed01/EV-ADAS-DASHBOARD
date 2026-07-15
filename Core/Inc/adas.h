/**
 ******************************************************************************
 * @file    adas.h
 * @brief   Advanced Driver Assistance logic: forward collision warning
 *          (distance + time-to-collision), blind-spot detection, parking
 *          assist proximity scoring, and overall alarm-priority arbitration.
 ******************************************************************************
 */
#ifndef __ADAS_H
#define __ADAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct {
    float front_cm;
    float left_cm;
    float right_cm;

    float ttc_s;              /* time-to-collision, seconds (front sensor) */
    CollisionLevel_t collision;
    uint8_t bsd_flags;        /* bit0 = left occupied, bit1 = right occupied */
    AlarmLevel_t alarm;
    uint8_t parking_score;    /* 0-100, higher = closer to obstacle (any side) */
} ADAS_State_t;

void ADAS_Init(void);

/**
 * @brief Run one ADAS evaluation cycle.
 * @param front_cm/left_cm/right_cm  Latest ultrasonic readings (cm)
 * @param speed_kmh                  Current vehicle speed
 */
void ADAS_Update(float front_cm, float left_cm, float right_cm, float speed_kmh);

const ADAS_State_t *ADAS_GetState(void);

#endif /* __ADAS_H */
