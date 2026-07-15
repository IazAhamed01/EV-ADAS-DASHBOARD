/**
 ******************************************************************************
 * @file    adas.c
 * @brief   Collision avoidance, blind-spot detection, parking assist and
 *          alarm-priority arbitration.
 ******************************************************************************
 */

#include "adas.h"
#include <string.h>

#define NO_OBSTACLE_CM 380.0f

static ADAS_State_t s_adas;

void ADAS_Init(void)
{
    memset(&s_adas, 0, sizeof(s_adas));

    s_adas.front_cm = ULTRASONIC_MAX_CM;
    s_adas.left_cm  = ULTRASONIC_MAX_CM;
    s_adas.right_cm = ULTRASONIC_MAX_CM;

    s_adas.ttc_s = 99.9f;
}

/*-----------------------------------------------------------*/
/* Collision Evaluation                                      */
/*-----------------------------------------------------------*/
static CollisionLevel_t evaluate_collision(float front_cm, float ttc_s)
{
    /* No obstacle detected */
    if(front_cm >= (ULTRASONIC_MAX_CM - 1))
        return COL_NONE;

    /* Critical */
    if(front_cm < COLLISION_CRITICAL_CM)
        return COL_CRITICAL;

    if(ttc_s < TTC_CRITICAL_S)
        return COL_CRITICAL;

    /* Warning */
    if(front_cm < COLLISION_WARNING_CM)
        return COL_WARNING;

    if(ttc_s < TTC_WARNING_S)
        return COL_WARNING;

    return COL_NONE;
}

/*-----------------------------------------------------------*/
/* Blind Spot Detection                                      */
/*-----------------------------------------------------------*/
static uint8_t evaluate_bsd(float left_cm, float right_cm, float speed_kmh)
{
    uint8_t flags = 0;

    if(speed_kmh >= BSD_MIN_SPEED_KMH)
    {
        if(left_cm < BSD_RANGE_CM && left_cm > 5.0f)
            flags |= 0x01;

        if(right_cm < BSD_RANGE_CM && right_cm > 5.0f)
            flags |= 0x02;
    }

    return flags;
}

/*-----------------------------------------------------------*/
/* Parking Assist                                             */
/*-----------------------------------------------------------*/
static uint8_t evaluate_parking_score(float front_cm,
                                      float left_cm,
                                      float right_cm)
{
    float closest = front_cm;

    if(left_cm < closest)
        closest = left_cm;

    if(right_cm < closest)
        closest = right_cm;

    if(closest >= ULTRASONIC_MAX_CM)
        return 0;

    float score = 100.0f * (1.0f - (closest / ULTRASONIC_MAX_CM));

    if(score < 0.0f)
        score = 0.0f;

    if(score > 100.0f)
        score = 100.0f;

    return (uint8_t)(score + 0.5f);
}

/*-----------------------------------------------------------*/
/* Alarm Arbitration                                          */
/*-----------------------------------------------------------*/
static AlarmLevel_t arbitrate_alarm(CollisionLevel_t col,
                                    uint8_t bsd,
                                    float speed_kmh)
{
    if(col == COL_CRITICAL)
        return ALARM_CRITICAL;

    if(col == COL_WARNING)
        return ALARM_WARNING;

    if(bsd || speed_kmh > OVERSPEED_KMH)
        return ALARM_ADVISORY;

    return ALARM_NONE;
}

/*-----------------------------------------------------------*/
/* Main Update                                                */
/*-----------------------------------------------------------*/
void ADAS_Update(float front_cm,
                 float left_cm,
                 float right_cm,
                 float speed_kmh)
{
    /* Clamp ultrasonic values */

    if(front_cm < 0)
        front_cm = ULTRASONIC_MAX_CM;

    if(left_cm < 0)
        left_cm = ULTRASONIC_MAX_CM;

    if(right_cm < 0)
        right_cm = ULTRASONIC_MAX_CM;

    if(front_cm > ULTRASONIC_MAX_CM)
        front_cm = ULTRASONIC_MAX_CM;

    if(left_cm > ULTRASONIC_MAX_CM)
        left_cm = ULTRASONIC_MAX_CM;

    if(right_cm > ULTRASONIC_MAX_CM)
        right_cm = ULTRASONIC_MAX_CM;

    /*
     * filters the ultrasonic readings
     */

    if(front_cm < 5.0f)
        front_cm = ULTRASONIC_MAX_CM;

    if(left_cm < 5.0f)
        left_cm = ULTRASONIC_MAX_CM;

    if(right_cm < 5.0f)
        right_cm = ULTRASONIC_MAX_CM;

    s_adas.front_cm = front_cm;
    s_adas.left_cm  = left_cm;
    s_adas.right_cm = right_cm;

    /*-------------------------------------------------------*/
    /* TTC Calculation                                       */
    /*-------------------------------------------------------*/

    float speed_cm_s = speed_kmh * 27.7778f;

    if((front_cm >= NO_OBSTACLE_CM) || (speed_kmh < 1.0f))
    {
        s_adas.ttc_s = 99.9f;
    }
    else
    {
        float new_ttc = front_cm / speed_cm_s;

        if(new_ttc > 99.9f)
            new_ttc = 99.9f;

        s_adas.ttc_s =
            0.8f * s_adas.ttc_s +
            0.2f * new_ttc;
    }
    /*-------------------------------------------------------*/
    /* Collision                                              */
    /*-------------------------------------------------------*/

    CollisionLevel_t new_col =
        evaluate_collision(front_cm, s_adas.ttc_s);

    if(new_col > s_adas.collision)
    {
        s_adas.collision = new_col;
    }
    else if(new_col < s_adas.collision)
    {
        if(front_cm >= NO_OBSTACLE_CM)
        {
            s_adas.collision = COL_NONE;
        }
        else if(front_cm > (COLLISION_WARNING_CM + 10.0f))
        {
            s_adas.collision = new_col;
        }
    }

    /*-------------------------------------------------------*/
    /* Blind Spot                                             */
    /*-------------------------------------------------------*/

    s_adas.bsd_flags =
        evaluate_bsd(left_cm,
                     right_cm,
                     speed_kmh);

    /*-------------------------------------------------------*/
    /* Parking Assist                                         */
    /*-------------------------------------------------------*/

    if(speed_kmh <= PARKING_ASSIST_MAX_KMH)
    {
        s_adas.parking_score =
            evaluate_parking_score(front_cm,
                                   left_cm,
                                   right_cm);
    }
    else
    {
        s_adas.parking_score = 0;
    }

    /*-------------------------------------------------------*/
    /* Alarm                                                  */
    /*-------------------------------------------------------*/

    s_adas.alarm =
        arbitrate_alarm(s_adas.collision,
                        s_adas.bsd_flags,
                        speed_kmh);
}

/*-----------------------------------------------------------*/

const ADAS_State_t *ADAS_GetState(void)
{
    return &s_adas;
}
