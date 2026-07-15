/**
 ******************************************************************************
 * @file    uart_shell.c
 * @brief   USART1 @ 115200 8N1 telemetry TX + interrupt-driven ASCII command
 *          shell RX (see README "UART Shell Commands" table).
 ******************************************************************************
 */
#include "uart_shell.h"
#include "fault.h"
#include <stdlib.h>

static UART_HandleTypeDef *s_huart;

/* ---- Interrupt RX ring buffer ------------------------------------------ */
#define RX_LINE_MAX 64
static uint8_t  s_rx_byte;                 /* single byte landing zone for HAL_UART_Receive_IT */
static char     s_rx_line[RX_LINE_MAX];
static uint8_t  s_rx_index = 0;
static volatile bool s_line_ready = false;

void Shell_Init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    s_rx_index = 0;
    s_line_ready = false;
    HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);

    const char *banner = "\r\n=== EV ADAS Dashboard - UART Shell Ready ===\r\n"
                         "Commands: speed/soc/temp/mode/fault/status\r\n";
    HAL_UART_Transmit(s_huart, (uint8_t *)banner, strlen(banner), 100);
}

/* Called from HAL_UART_RxCpltCallback (see stm32f1xx_it.c / main.c) */
void Shell_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != s_huart->Instance) return;

    char c = (char)s_rx_byte;

    if (c == '\r' || c == '\n') {
        if (s_rx_index > 0) {
            s_rx_line[s_rx_index] = '\0';
            s_line_ready = true;
        }
    } else if (s_rx_index < (RX_LINE_MAX - 1)) {
        s_rx_line[s_rx_index++] = c;
    }

    /* re-arm interrupt RX for next byte */
    HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);
}

/* ------------------------------------------------------------------------ */
static void reply(const char *msg)
{
    HAL_UART_Transmit(s_huart, (uint8_t *)msg, strlen(msg), 100);
}

static void handle_command(char *line)
{
    char cmd[16] = {0};
    char arg[32] = {0};
    int n = sscanf(line, "%15s %31s", cmd, arg);

    if (n >= 1 && strcmp(cmd, "speed") == 0 && n == 2) {
        EV_SetSpeedOverride((float)atof(arg));
        reply("OK: speed override set\r\n");

    } else if (n >= 1 && strcmp(cmd, "soc") == 0 && n == 2) {
        EV_SetSOC((float)atof(arg));
        reply("OK: soc set\r\n");

    } else if (n >= 1 && strcmp(cmd, "temp") == 0 && n == 2) {
        EV_SetMotorTemp((float)atof(arg));
        reply("OK: motor temp set\r\n");

    } else if (n >= 1 && strcmp(cmd, "mode") == 0 && n == 2) {
        int m = atoi(arg);
        if (m >= 0 && m <= 2) {
            EV_SetMode((DriveMode_t)m);
            reply("OK: mode set\r\n");
        } else {
            reply("ERR: mode must be 0(ECO)/1(NORMAL)/2(SPORT)\r\n");
        }

    } else if (n >= 1 && strcmp(cmd, "fault") == 0 && n == 2) {
        if (strcmp(arg, "clear") == 0) {
            Fault_ClearAll();
            reply("OK: faults cleared\r\n");
        } else {
            uint8_t bits = (uint8_t)strtol(arg, NULL, 16);
            Fault_InjectByte(bits);
            reply("OK: fault injected\r\n");
        }

    } else if (n >= 1 && strcmp(cmd, "status") == 0) {
        char buf[160];
        const EV_State_t *ev = EV_GetState();
        snprintf(buf, sizeof(buf),
            "STATE:%d MODE:%d SPD:%.1f SOC:%.1f TRQ:%.1f TMP:%.1f RNG:%.0f\r\n",
            ev->state, ev->mode, ev->speed_kmh, ev->soc_pct,
            ev->torque_nm, ev->motor_temp_c, ev->range_km);
        reply(buf);

    } else {
        reply("ERR: unknown command\r\n");
    }
}

void Shell_Poll(void)
{
    if (s_line_ready) {
        handle_command(s_rx_line);
        s_rx_index = 0;
        s_line_ready = false;
    }
}

/* ------------------------------------------------------------------------ */
void Shell_SendTelemetry(const EV_State_t *ev, const ADAS_State_t *adas, uint8_t fault_byte)
{
    char frame1[96];
    char frame2[96];

    snprintf(frame1, sizeof(frame1),
        "SPD:%.2f SOC:%.1f TRQ:%.0f TMP:%.1f RNG:%.0f ACC:%.0f BRK:%.0f\r\n",
        ev->speed_kmh, ev->soc_pct, ev->torque_nm, ev->motor_temp_c,
        ev->range_km, ev->accel_pct, ev->brake_pct);

    snprintf(frame2, sizeof(frame2),
        "F:%.2f L:%.2f R:%.2f TTC:%.1fs COL:%d BSD:%d%d ALM:%d FLT:%02X\r\n",
        adas->front_cm, adas->left_cm, adas->right_cm, adas->ttc_s,
        adas->collision,
        (adas->bsd_flags & 0x01) ? 1 : 0,
        (adas->bsd_flags & 0x02) ? 1 : 0,
        adas->alarm, fault_byte);

    HAL_UART_Transmit(s_huart, (uint8_t *)frame1, strlen(frame1), 50);
    HAL_UART_Transmit(s_huart, (uint8_t *)frame2, strlen(frame2), 50);
}
