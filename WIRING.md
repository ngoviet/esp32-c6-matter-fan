# ESP32-C6 Smart Fan — Wiring Diagram

## GPIO Mapping (from config.h)

```
ESP32-C6                ECC11 Rotary Encoder
┌─────────────┐         ┌──────────────┐
│             │         │              │
│   GPIO1 ────┼─────────┼── PWM OUT ────► Fan (signal wire)
│   GPIO2 ────┼─────────┼── CLK        │
│   GPIO3 ────┼─────────┼── DT         │
│   GPIO4 ────┼─────────┼── SW (button)│
│             │         └──────────────┘
│   3.3V  ────┼────────── + (encoder power)
│   GND   ────┼────────── GND (encoder)
│             │
└─────────────┘
```
Tóm tắt đấu nối nhanh:

ESP32-C6	Đấu vào
GPIO1 →	MOSFET gate (qua R 330Ω) → quạt
GPIO2 →	Encoder CLK
GPIO3 →	Encoder DT
GPIO4 →	Encoder SW (nút)
3.3V →	VCC encoder
GND →	GND encoder + GND quạt
MOSFET IRLZ44N khuếch đại PWM 3.3V → 12V/24V. Nguồn quạt riêng.

Đấu xong báo tôi code tiếp app_matter.cpp!

## Pin Connection Table

| ESP32-C6 Pin | Connect To | Notes |
|-------------|-----------|-------|
| **GPIO1** | Fan signal wire (PWM) | 100-400Hz, 50% duty cycle |
| **GPIO2** | Encoder CLK | Clock pulse, `ANYEDGE` interrupt |
| **GPIO3** | Encoder DT | Direction detection |
| **GPIO4** | Encoder SW | Button press, `NEGEDGE` interrupt (active low) |
| **3.3V** | Encoder VCC | Power for encoder module |
| **GND** | Encoder GND + Fan GND | Common ground |
| **USB-C** | Computer / 5V power | Power + flash + debug |

## Fan Driver Circuit (3.3V PWM → 12V/24V Fan)

```
ESP32-C6 GPIO1 (3.3V PWM)
        │
        ├── R1 (330Ω) ───┬── D1 (1N4148, protection)
        │                │
        │               G (IRLZ44N MOSFET gate)
        │               S ─── GND
        │               D ─── FAN(-)
        │
        │              FAN(+) ─── 12V/24V POWER SUPPLY (+)
        │              POWER SUPPLY (-) ─── GND (common)
```

**IRLZ44N is a logic-level MOSFET** — turns fully on at 3.3V gate voltage, no driver needed.

## Parts List

| Part | Qty | Purpose |
|------|-----|---------|
| ESP32-C6 dev board | 1 | Main MCU (Matter + Thread) |
| ECC11 Rotary Encoder module | 1 | Speed control knob + push button |
| IRLZ44N MOSFET | 1 | PWM amplification for fan |
| 330Ω resistor | 1 | Gate current limiting |
| 1N4148 diode | 1 | Flyback/ESD protection |
| 12V or 24V PWM-compatible fan | 1 | The fan being controlled |
| 12V/24V DC power supply | 1 | Separate power for fan |

## PWM Parameters

| Parameter | Value |
|-----------|-------|
| Frequency range | 100 Hz (min/off) — 400 Hz (max) |
| Duty cycle | 50% fixed (square wave) |
| Resolution | 13-bit (8192 levels) |
| LEDC Timer | TIMER_0 |
| LEDC Mode | LOW_SPEED_MODE |
| LEDC Channel | CHANNEL_0 |
| Clock source | LEDC_AUTO_CLK |

Formula: `freq = 100 + (speed_percent * 300) / 100`

## Encoder Settings

| Parameter | Value |
|-----------|-------|
| Steps | 0–33 (each step ≈ 3% speed) |
| Resolution | 1 pulse per detent |
| Debounce | Hardware (pull-up) + ISR edge detection |

## Notes

- **Fan GND and ESP32-C6 GND must be connected together** (common ground)
- Keep PWM wire from GPIO1 to MOSFET as short as possible
- If fan draws > 5A, add a heatsink to the IRLZ44N
- The 12V/24V power supply is SEPARATE from the USB power
- 1N4148 diode goes between gate and GND (cathode to gate, anode to GND) for ESD protection
