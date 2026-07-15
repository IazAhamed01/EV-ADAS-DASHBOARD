/**
 ******************************************************************************
 * @file    fault.c
 * @brief   Fault bit-field evaluation + status LED + torque-cut arbitration.
 *
 * LED map (see README pin table):
 *   PB8  - Collision LED   (lit on WARNING or CRITICAL collision)
 *   PB9  - Blind-spot Left LED
 *   PB10 - Blind-spot Right LED
 *   PB11 - Fault LED       (lit whenever fault byte != 0)
 ******************************************************************************
 */
#include "fault.h"
#include "ev_control.h"

#define LED_COLLISION_PORT  GPIOB
#define LED_COLLISION_PIN   GPIO_PIN_8
#define LED_BSD_L_PORT      GPIOB
#define LED_BSD_L_PIN       GPIO_PIN_9
#define LED_BSD_R_PORT      GPIOB
#define LED_BSD_R_PIN       GPIO_PIN_10
#define LED_FAULT_PORT      GPIOB
#define LED_FAULT_PIN       GPIO_PIN_11

static uint8_t s_fault_byte = 0;
static bool    s_manual_override = false; /* set by "fault <hex>" shell command */

void Fault_Init(void)
{
    s_fault_byte = 0;
    s_manual_override = false;
}

void Fault_Update(float motor_temp_c, float soc_pct, CollisionLevel_t collision,
                   const ADAS_State_t *adas)
{
    if (!s_manual_override) {
        uint8_t bits = 0;
        if (motor_temp_c > TEMP_FAULT_C)      bits |= FAULT_OT;
        if (soc_pct < SOC_FAULT_PCT)           bits |= FAULT_SOC;
        if (collision == COL_CRITICAL)         bits |= FAULT_COL;
        s_fault_byte = bits;
    }

    if (s_fault_byte != 0) {
        EV_SetState(STATE_FAULT);
    } else if (EV_GetState()->state == STATE_FAULT) {
        /* Fault condition cleared on its own (e.g. obstacle moved away) -
         * auto-recover instead of latching FAULT forever. Manual "fault
         * clear" is still needed to override a manually-injected fault. */
        EV_SetState(STATE_PARKED);
        EV_CutTorque(false);
    }

    /* Torque cut is driven purely by the ADAS alarm level, independent of
     * manual fault injection, so "CRITICAL -> torque cut" always holds. */
    EV_CutTorque(adas->alarm == ALARM_CRITICAL);

    /* ---- Drive status LEDs ------------------------------------------- */
    HAL_GPIO_WritePin(LED_COLLISION_PORT, LED_COLLISION_PIN,
        (adas->collision != COL_NONE) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(LED_BSD_L_PORT, LED_BSD_L_PIN,
        (adas->bsd_flags & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(LED_BSD_R_PORT, LED_BSD_R_PIN,
        (adas->bsd_flags & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(LED_FAULT_PORT, LED_FAULT_PIN,
        (s_fault_byte != 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t Fault_GetByte(void) { return s_fault_byte; }

void Fault_InjectByte(uint8_t bits)
{
    s_manual_override = true;
    s_fault_byte = bits;
}

void Fault_ClearAll(void)
{
    s_manual_override = false;
    s_fault_byte = 0;
    EV_SetState(STATE_PARKED);
    EV_CutTorque(false);
}
