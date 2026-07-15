/**
 ******************************************************************************
 * @file    fault.h
 * @brief   Bit-field fault register (acts like a mini OBD-II DTC store) and
 *          status-LED driver.
 ******************************************************************************
 */
#ifndef __FAULT_H
#define __FAULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "adas.h"

void Fault_Init(void);

/**
 * @brief Evaluate fault conditions from the latest EV + ADAS state and
 *        drive the status LEDs (PB8-PB11). Also cuts propulsion torque
 *        via EV_CutTorque() when alarm == ALARM_CRITICAL.
 */
void Fault_Update(float motor_temp_c, float soc_pct, CollisionLevel_t collision,
                   const ADAS_State_t *adas);

uint8_t Fault_GetByte(void);
void    Fault_InjectByte(uint8_t bits);
void    Fault_ClearAll(void);

#endif /* __FAULT_H */
