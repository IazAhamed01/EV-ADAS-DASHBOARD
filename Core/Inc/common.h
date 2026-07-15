/**
 ******************************************************************************
 * @file    common.h
 * @brief   Shared types, enums and tunable constants for the EV ADAS project.
 *
 * Target      : STM32F103C8T6 (Blue Pill)
 * Simulator   : PICSimLab
 * Internship  : Emertxe Automotive Embedded Internship, 2026
 ******************************************************************************
 */
#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------------- */
/* Drive modes                                                            */
/* ---------------------------------------------------------------------- */
typedef enum {
    MODE_ECO    = 0,
    MODE_NORMAL = 1,
    MODE_SPORT  = 2
} DriveMode_t;

/* ---------------------------------------------------------------------- */
/* Vehicle state machine                                                  */
/* ---------------------------------------------------------------------- */
typedef enum {
    STATE_PARKED = 0,
    STATE_DRIVE  = 1,
    STATE_FAULT  = 2
} VehicleState_t;

/* ---------------------------------------------------------------------- */
/* Alarm priority (drives buzzer tier + dashboard color)                  */
/* ---------------------------------------------------------------------- */
typedef enum {
    ALARM_NONE     = 0,
    ALARM_ADVISORY = 1,
    ALARM_WARNING  = 2,
    ALARM_CRITICAL = 3
} AlarmLevel_t;

/* ---------------------------------------------------------------------- */
/* Collision severity                                                     */
/* ---------------------------------------------------------------------- */
typedef enum {
    COL_NONE     = 0,
    COL_WARNING  = 1,
    COL_CRITICAL = 2
} CollisionLevel_t;

/* ---------------------------------------------------------------------- */
/* Fault bit-field (byte sent in FLT: field, hex)                         */
/* ---------------------------------------------------------------------- */
#define FAULT_OT   (1U << 0)   /* Motor over-temperature   (> 90 C)       */
#define FAULT_SOC  (1U << 1)   /* Battery critically low   (< 2 %)        */
#define FAULT_COL  (1U << 2)   /* Collision critical       (< 20 cm)      */

/* ---------------------------------------------------------------------- */
/* Scheduling                                                             */
/* ---------------------------------------------------------------------- */
#define CONTROL_PERIOD_MS   100U     /* 10 Hz main control/telemetry loop */

/* ---------------------------------------------------------------------- */
/* Ultrasonic (HC-SR04) timing                                            */
/* ---------------------------------------------------------------------- */
#define ULTRASONIC_TIMEOUT_US   20000U   /* ~3.4 m max range, bails out early */
#define ULTRASONIC_MAX_CM       400.0f
#define SOUND_CM_PER_US         0.0343f  /* speed of sound, cm per microsecond */

/* ---------------------------------------------------------------------- */
/* Thresholds (tunable) - mirror README specification                     */
/* ---------------------------------------------------------------------- */
#define TEMP_FAULT_C            90.0f
#define SOC_FAULT_PCT           2.0f

#define COLLISION_CRITICAL_CM   20.0f
#define COLLISION_WARNING_CM    50.0f
#define TTC_CRITICAL_S          1.5f
#define TTC_WARNING_S           3.0f

#define BSD_RANGE_CM             150.0f   /* left/right car detected inside this range */
#define BSD_MIN_SPEED_KMH        15.0f    /* blind-spot logic is speed-gated           */

#define OVERSPEED_KMH            120.0f
#define PARKING_ASSIST_MAX_KMH   10.0f

#endif /* __COMMON_H */
