/**
 ******************************************************************************
 * @file    ev_control.h
 * @brief   Vehicle dynamics + powertrain model: pedals -> torque -> speed,
 *          SOC drain / regen, range estimation, drive-mode scaling.
 ******************************************************************************
 */
#ifndef __EV_CONTROL_H
#define __EV_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct {
    float speed_kmh;      /* current vehicle speed                    */
    float soc_pct;        /* battery state of charge, 0-100           */
    float torque_nm;      /* instantaneous motor torque                */
    float motor_temp_c;   /* motor temperature                         */
    float range_km;       /* estimated remaining range                 */
    float accel_pct;      /* accelerator pedal position, 0-100         */
    float brake_pct;      /* brake pedal position, 0-100                */
    DriveMode_t mode;
    VehicleState_t state;
} EV_State_t;

/* Lifecycle */
void EV_Init(ADC_HandleTypeDef *hadc);
void EV_Update(float dt_s);

/* Accessors */
const EV_State_t *EV_GetState(void);

/* Shell / test overrides (UART shell + deterministic testing) */
void EV_SetSpeedOverride(float kmh);
void EV_ClearSpeedOverride(void);
void EV_SetSOC(float pct);
void EV_SetMotorTemp(float c);
void EV_SetMode(DriveMode_t mode);
void EV_SetState(VehicleState_t state);
void EV_CutTorque(bool cut);   /* called by fault/ADAS engine on CRITICAL alarm */

#endif /* __EV_CONTROL_H */
