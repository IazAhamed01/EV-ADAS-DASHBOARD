/**
 ******************************************************************************
 * @file    ev_control.c
 * @brief   Simplified but physically-plausible EV powertrain model.
 *
 * Pedals are read from potentiometers on PA0 (accel) / PA1 (brake) via ADC1.
 * PA2 (SOC pot) is sampled ONCE at boot to seed the initial battery level -
 * from then on SOC evolves purely from the simulated energy model, exactly
 * like a real BMS state estimate would. PA3 (motor-temp pot) is sampled
 * continuously so it can be used on the bench / in PICSimLab to inject a
 * temperature fault live.
 *
 * NOTE ON TUNING: constants below (drag, torque scale, Wh/km, etc.) are
 * chosen to give a believable dashboard feel in simulation - they are not
 * derived from a real vehicle dataset.
 ******************************************************************************
 */
#include "ev_control.h"
#include <math.h>

/* ---- ADC channel assignment (must match CubeMX ADC1 regular group) ---- */
#define ADC_CH_ACCEL   0   /* rank 1 -> PA0 */
#define ADC_CH_BRAKE   1   /* rank 2 -> PA1 */
#define ADC_CH_SOC     2   /* rank 3 -> PA2 */
#define ADC_CH_TEMP    3   /* rank 4 -> PA3 */

static ADC_HandleTypeDef *s_hadc;
static EV_State_t s_ev;

static bool  s_speed_override_active = false;
static float s_speed_override_val    = 0.0f;
static bool  s_torque_cut            = false;

/* Per-mode torque scaling (ECO saves energy, SPORT is aggressive) */
static const float MODE_TORQUE_SCALE[3] = {0.65f, 1.0f, 1.35f};
static const float MODE_REGEN_SCALE[3]  = {1.20f, 1.0f, 0.75f};

#define MAX_TORQUE_NM        250.0f
#define VEHICLE_DRAG_COEFF   0.06f     /* speed-proportional aero+rolling drag  */
#define ACCEL_GAIN_KMH_S     9.0f      /* kmh gained per second at 100% pedal   */
#define BRAKE_GAIN_KMH_S     14.0f     /* kmh lost per second at 100% brake     */
#define MAX_SPEED_KMH        180.0f

#define WH_PER_KM_BASE       150.0f    /* baseline consumption                 */
#define BATTERY_CAPACITY_WH  60000.0f  /* 60 kWh pack                           */
#define RANGE_MAX_KM         400.0f    /* range at 100% SOC, WLTP-ish figure    */

/* ------------------------------------------------------------------------ */
static float adc_read_channel(uint32_t rank_channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = rank_channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(s_hadc, &sConfig);

    HAL_ADC_Start(s_hadc);
    if (HAL_ADC_PollForConversion(s_hadc, 5) == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(s_hadc);
        return (raw * 100.0f) / 4095.0f;   /* 12-bit ADC -> 0-100 % */
    }
    return 0.0f;
}

/* ------------------------------------------------------------------------ */
void EV_Init(ADC_HandleTypeDef *hadc)
{
    s_hadc = hadc;
    memset(&s_ev, 0, sizeof(s_ev));

    s_ev.mode  = MODE_NORMAL;
    s_ev.state = STATE_PARKED;

    /* Seed initial SOC once from the SOC potentiometer (PA2) */
    s_ev.soc_pct = adc_read_channel(ADC_CH_SOC);
    if (s_ev.soc_pct < 5.0f) s_ev.soc_pct = 80.0f; /* sane default if pot at 0 */

    s_ev.motor_temp_c = adc_read_channel(ADC_CH_TEMP) * 1.2f; /* map 0-100 -> 0-120C */
    s_ev.range_km = (s_ev.soc_pct / 100.0f) * RANGE_MAX_KM;
}

/* ------------------------------------------------------------------------ */
void EV_Update(float dt_s)
{
    /* ---- 1. Read pedals every cycle -------------------------------- */
    s_ev.accel_pct = adc_read_channel(ADC_CH_ACCEL);
    s_ev.brake_pct = adc_read_channel(ADC_CH_BRAKE);
    s_ev.motor_temp_c = adc_read_channel(ADC_CH_TEMP) * 1.2f;

    if (s_ev.state == STATE_FAULT) {
        /* Vehicle limps to a stop, no propulsion */
        s_ev.accel_pct = 0.0f;
    }

    /* ---- 2. Torque model -------------------------------------------- */
    float mode_scale = MODE_TORQUE_SCALE[s_ev.mode];
    float regen_scale = MODE_REGEN_SCALE[s_ev.mode];

    float drive_torque = (s_ev.accel_pct / 100.0f) * MAX_TORQUE_NM * mode_scale;
    float brake_torque = (s_ev.brake_pct / 100.0f) * MAX_TORQUE_NM * regen_scale;

    if (s_torque_cut) {
        drive_torque = 0.0f;   /* ADAS/fault engine has cut propulsion */
    }

    s_ev.torque_nm = drive_torque - brake_torque;

    /* ---- 3. Speed integration (simple longitudinal model) ------------ */
    if (!s_speed_override_active) {
        float accel_term = (drive_torque / MAX_TORQUE_NM) * ACCEL_GAIN_KMH_S;
        float brake_term = (brake_torque / MAX_TORQUE_NM) * BRAKE_GAIN_KMH_S;
        float drag_term  = s_ev.speed_kmh * VEHICLE_DRAG_COEFF;

        s_ev.speed_kmh += (accel_term - brake_term - drag_term) * dt_s;

        if (s_ev.speed_kmh < 0.0f) s_ev.speed_kmh = 0.0f;
        if (s_ev.speed_kmh > MAX_SPEED_KMH) s_ev.speed_kmh = MAX_SPEED_KMH;
    } else {
        s_ev.speed_kmh = s_speed_override_val;
    }

    s_ev.state = (s_ev.speed_kmh > 0.5f) ? STATE_DRIVE :
                 (s_ev.state == STATE_FAULT ? STATE_FAULT : STATE_PARKED);

    /* ---- 4. Energy / SOC model ---------------------------------------- */
    /* Wh consumed this tick = (base Wh/km + torque-proportional extra) * km driven */
    float km_this_tick = (s_ev.speed_kmh / 3600.0f) * dt_s;
    float wh_per_km = WH_PER_KM_BASE + (drive_torque * 1.4f);
    float wh_used = wh_per_km * km_this_tick;

    /* Regenerative braking returns some energy to the pack */
    float wh_regen = (brake_torque * 0.9f) * km_this_tick;

    float delta_soc_pct = ((wh_regen - wh_used) / BATTERY_CAPACITY_WH) * 100.0f;
    s_ev.soc_pct += delta_soc_pct;

    if (s_ev.soc_pct < 0.0f)   s_ev.soc_pct = 0.0f;
    if (s_ev.soc_pct > 100.0f) s_ev.soc_pct = 100.0f;

    s_ev.range_km = (s_ev.soc_pct / 100.0f) * RANGE_MAX_KM;
}

/* ------------------------------------------------------------------------ */
const EV_State_t *EV_GetState(void) { return &s_ev; }

void EV_SetSpeedOverride(float kmh) { s_speed_override_active = true; s_speed_override_val = kmh; }
void EV_ClearSpeedOverride(void)    { s_speed_override_active = false; }
void EV_SetSOC(float pct)           { s_ev.soc_pct = (pct < 0) ? 0 : (pct > 100 ? 100 : pct); }
void EV_SetMotorTemp(float c)       { s_ev.motor_temp_c = c; }
void EV_SetMode(DriveMode_t mode)   { s_ev.mode = mode; }
void EV_SetState(VehicleState_t st) { s_ev.state = st; }
void EV_CutTorque(bool cut)         { s_torque_cut = cut; }
