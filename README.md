# ⚡ EV ADAS Dashboard

Real-Time Electric Vehicle Telemetry & Driver-Assistance System, built on the
**STM32F103C8T6 (Blue Pill)**, simulated in **PICSimLab**, with a live
**Python Matplotlib** dashboard styled like a real EV instrument cluster.


## 📌 Overview

This project simulates a real EV's control unit and its ADAS (Advanced
Driver Assistance System) safety features. It reads pedal and ultrasonic
sensor inputs on an STM32 microcontroller, computes real-time vehicle
physics (speed, SOC, torque, range), runs collision-avoidance and
blind-spot logic, manages fault detection with auto-recovery, and streams
all telemetry over UART to a live Python dashboard.

## 🧰 Tools & Technologies

| Tool | Purpose |
|---|---|
| STM32CubeIDE | Firmware development (C, HAL) |
| PICSimLab | STM32 Blue Pill hardware simulation |
| STM32F103C8T6 (Blue Pill) | Microcontroller — acts as the Vehicle Control Unit |
| HC-SR04 Ultrasonic ×3 | Front / Left / Right obstacle distance sensing |
| UART (USART1 @ 115200 bps) | Telemetry link to PC |
| com0com | Null-modem emulator, bridges the simulated UART to a real Windows COM port |
| Python 3 + pyserial | Serial data capture |
| Matplotlib + NumPy | Live animated dashboard (10 fps) |

## 🏗️ System Architecture
Sensors (Potentiometers + HC-SR04)
│
▼
STM32 ADC / Ultrasonic EXTI Interrupt Capture (100 ms cycle)
│
▼
EV Control Engine   +   ADAS Engine
│                    │
└────────► Fault / Alarm Evaluation
│
▼
UART (115200 bps)
│
▼
Python Matplotlib Dashboard

| Real Industry Component | Project Equivalent |
|---|---|
| Pressure / wheel-speed / proximity sensors | Potentiometers (ADC) + PICSimLab simulation |
| ECU / Vehicle Control Unit | STM32 Blue Pill (F103C8T6) |
| CAN / LIN Bus | UART @ 115200 bps |
| Instrument cluster / HMI | Python Matplotlib Dashboard |
| OBD-II / DTC reader | UART Shell + fault bit-field register |
| Multi-tone alarm buzzer | Tiered alarm system (NONE → ADVISORY → WARNING → CRITICAL) |

## ⚙️ Features

- Real-time speed, SOC, torque, and range monitoring
- Physics-based inertia model with regenerative braking
- ECO / NORMAL / SPORT drive modes with torque scaling
- Forward Collision Warning (distance + speed-gated Time-To-Collision)
- Blind Spot Detection (left/right, speed-gated)
- Parking Assist (proximity scoring below 10 km/h)
- Bit-field fault register with 4-level alarm priority (NONE / ADVISORY / WARNING / CRITICAL)
- Fault auto-recovery — transient faults clear automatically once the condition resolves
- Interrupt-driven (EXTI) ultrasonic edge capture for reliable timing under simulation
- UART shell for live parameter injection & deterministic testing
- Live Python dashboard: speedometer, SOC bar, speed history, ADAS bird's-eye view, metrics panel

## 🔌 Hardware / Pin Mapping

| Pin | Component | Notes |
|---|---|---|
| PA0 | Accelerator pedal (ADC) | 0–100% |
| PA1 | Brake pedal (ADC) | 0–100% |
| PA2 | SOC potentiometer (ADC) | Seeds initial battery % |
| PA3 | Motor temperature (ADC) | 0–120 °C |
| PA9 / PA10 | USART1 TX/RX | 115200 bps telemetry + shell |
| PB0 / PB1 | Front HC-SR04 TRIG/ECHO | Forward collision sensing |
| PB2 / PB3 | Left HC-SR04 TRIG/ECHO | Blind spot (left) |
| PB4 / PB5 | Right HC-SR04 TRIG/ECHO | Blind spot (right) |
| PB8–PB11 | Status LEDs | Collision / BSD-L / BSD-R / Fault |

## 📡 UART Telemetry Protocol

Two structured ASCII frames sent every 100 ms:
SPD:72.5 SOC:79.3 TRQ:75 TMP:27.1 RNG:2600 ACC:50 BRK:0
F:40 L:400 R:400 TTC:2.1s COL:1 BSD:10 ALM:2 FLT:04

| Field | Meaning |
|---|---|
| SPD | Vehicle speed (km/h) |
| SOC | State of Charge (%) |
| TRQ | Motor torque (Nm) |
| TMP | Motor temperature (°C) |
| RNG | Estimated range (km) |
| F / L / R | Ultrasonic distance, front/left/right (cm) |
| TTC | Time-to-collision (s), only evaluated above 10 km/h |
| COL | Collision level (0=none, 1=warning, 2=critical) |
| BSD | Blind spot flags (left, right) |
| ALM | Alarm priority (0=none → 3=critical) |
| FLT | Fault byte (hex bit-field) |

## 🚨 Fault & Alarm System

| Bit | Fault | Trigger |
|---|---|---|
| 0 | FAULT_OT | Motor temp > 90 °C |
| 1 | FAULT_SOC | Battery SOC < 2% |
| 2 | FAULT_COL | Collision critical (< 20 cm) |

| Alarm Level | Condition |
|---|---|
| NONE | Normal operation |
| ADVISORY | Blind spot detected or overspeed (> 120 km/h) |
| WARNING | Collision < 50 cm OR TTC < 3 s (above 10 km/h) |
| CRITICAL | Collision < 20 cm OR TTC < 1.5 s (above 10 km/h) — motor torque cut, dashboard flashes red |

Faults auto-clear once the condition resolves (e.g. an obstacle moves away), restoring normal operation automatically — no manual reset needed for transient events.

## 🖥️ UART Shell Commands

| Command | Effect |
|---|---|
| `speed <km/h>` | Inject vehicle speed |
| `soc <0-100>` | Set battery SOC |
| `temp <°C>` | Set motor temperature |
| `mode <0-2>` | ECO / NORMAL / SPORT |
| `fault <hex>` | Manually inject fault bits |
| `fault clear` | Clear faults, return to PARKED |
| `status` | Print full system state |

## 📂 Project Structure
ev_dash/
├── Core/
│   ├── Src/
│   │   ├── main.c
│   │   ├── ev_control.c
│   │   ├── adas.c
│   │   ├── ultrasonic.c
│   │   ├── fault.c
│   │   └── uart_shell.c
│   └── Inc/
│       ├── common.h
│       ├── ev_control.h
│       ├── adas.h
│       ├── ultrasonic.h
│       ├── fault.h
│       └── uart_shell.h
├── python/
│   └── dashboard.py
└── README.md

## ▶️ Running in Simulation

1. **STM32CubeIDE** — create an STM32F103C8Tx project, configure ADC1 (PA0-3), USART1 (PA9/10, 115200 8N1), TIM2 (free-running 1MHz), and GPIO per the pin table above. Drop in the `Core/Src` and `Core/Inc` files, build to get a `.bin`.
2. **PICSimLab** — load the Blue Pill board, load the compiled `.bin`, wire up potentiometers and HC-SR04 spare parts per the pin table, add a UART spare part wired to PA9/PA10.
3. **com0com** (Windows) — create a virtual null-modem port pair; assign one side to PICSimLab's UART part, the other to your terminal/dashboard.
4. **Python dashboard**:
```bash
   pip install matplotlib numpy pyserial
   python dashboard.py --port COM<x>
   # or, with no hardware/simulator running:
   python dashboard.py --demo
```

## 🧪 Notes & Known Limitations

- Vehicle physics (drag, Wh/km, torque curve) are tuned for a believable **simulation feel**, not derived from a real vehicle's datasheet.
- Ultrasonic sensing uses EXTI interrupt edge-capture rather than software GPIO polling, since QEMU (which PICSimLab runs on) doesn't guarantee consistent instruction-level timing for busy-wait loops.
- TTC-based collision severity is gated to speeds above 10 km/h, since the raw distance/speed formula is overly sensitive at low speed / short range.

## 📄 License

MIT
