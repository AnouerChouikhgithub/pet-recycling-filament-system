# PET Plastic Recycling to 3D Printer Filament System

An Arduino-based system that turns PET plastic waste into usable 3D printer
filament through a custom-built extrusion control system. Independently
designed and developed as a solo project, currently in its 3rd hardware
iteration.

**Status:** Actively developed · 2024 – Present · Iterations V1 → V2 → V3 (V3.2 base assembly).

## Overview

The system controls extrusion temperature and filament feed rate in real time,
using closed-loop PID control and a microstepping stepper driver for consistent,
printable filament output. It is the machine side of the 3awedlou ecosystem:
the backend is the single backbone, and this firmware is the deterministic
controller that the ESP32 gateway will eventually network.

**Phases (as of this writing):**

| Phase | Description | Status |
|---|---|---|
| 1 | Arduino Mega direct control (Serial) | Done |
| 2 | ESP32 + TXS0108E serial gateway → MQTT | **Planned**, wired on bench, bench test pending |
| 3 | MQTT telemetry + command bridge to backend | Done (backend side, see repo README) |

This README documents the machine as it exists today (phase 1) and marks the
ESP32 gateway and the MQTT blade as planned. Hardware facts here come only from
the `.ino` and the files in `firmware/` and `cad/`; anything not present in the
code is marked `TODO(anouer)`.

## Repository tree (current)

```text
pet-recycling-filament-system/
├── firmware/
│   └── arduino-version/
│       └── arduino-version.ino          # Main Arduino firmware (PID control, stepper driving)
├── cad/
│   ├── PET-Recycling-Filament-System-Base - V3.2/
│   │   ├── PET-Recycling-Filament-System-Base 3.2.SLDPRT   # V3.2 base assembly, SolidWorks native
│   │   └── PET-Recycling-Filament-System-Base 3.2.STEP     # V3.2 base assembly, neutral STEP
│   └── 3D Printing/                  # Individual printable components (STEP)
│       ├── 2xBig-Separator.step
│       ├── 3xSmall-Separator.step
│       ├── 4xFixative.step
│       ├── Big-Gear-A.step
│       ├── Big-Gear-B.step
│       ├── Medium-Gear.step
│       ├── Medium-Separator.step
│       ├── Motor-Gear.step
│       ├── Motor-Separator.step
│       ├── Pulley.step
│       ├── Structure-A.step
│       └── Structure-B.step
├── docs/
│   ├── Schematic Diagram.pdf         # Full breadboard / wiring schematic
│   └── Schematic Diagram_bb.pdf      # Breadboard wiring diagram (Fritzing export)
└── README.md
```

> **Known stale paths in earlier READMEs:** the firmware is
> `firmware/arduino-version/arduino-version.ino` (NOT `firmware/code.ino`),
> the base assembly is `cad/PET-Recycling-Filament-System-Base - V3.2/`
> (NOT `cad/base-assembly/`), and the schematic is `docs/Schematic Diagram.pdf`
> (NOT `docs/Schematic-Diagram_bb.pdf`). Keep this tree in mind when editing
> this document.

## Mechanical overview

The machine is a standalone PET extruder feeding a 3D printer filament path.
Base assembly and drivetrain are modeled in SolidWorks (`.SLDPRT`) plus a
neutral `.STEP` for interchange, and the printable cage/drivetrain parts are
provided as `.step` files in `cad/3D Printing/`.

| File / folder | What it is |
|---|---|
| `cad/PET-Recycling-Filament-System-Base - V3.2/` | Main structural assembly (V3.2), SolidWorks + STEP |
| `cad/3D Printing/*.step` | Individual 3D-printed parts: gears, separators, fixatives, pulleys, frame sections |
| `docs/Schematic Diagram.pdf` | Full wiring schematic |
| `firmware/arduino-version/arduino-version.ino` | Firmware source (single file) |

STEP files open in any CAD software (SolidWorks, Fusion 360, FreeCAD).

## Electronics overview

| Component | Role |
|---|---|
| Arduino Mega 2560 (with ATmega2560) | Main controller; keeps deterministic machine control |
| Thermistor (100 kΩ NTC, β=3950, nominal 25 °C @ 100 kΩ) | Temperature feedback for the PID loop, read on A7 |
| PWM-driven heater bridge (LPWM/RPWM) | Heating element actuation from the PID output |
| Stepper driver (TMC2209-class, 1/2–1/16 step) | Filament extrusion drive; microstepping selectable 1/2/4/8/16 via MS1–MS3 |
| Stepper motor (400 steps/rev full step) | Extrusion roller drive |
| Motor enable (R_EN/L_EN), Sleep/reset (SLEEP_RESET_PIN) | Driver enable + burst-mode wake |
| 12 V supply (heater + motor), 5 V (Arduino, thermistor) | Power rails; mains side is isolated from logic |

## Pin map (from `firmware/arduino-version/arduino-version.ino`)

Extracted verbatim from the sketch constants; nothing is inferred.

| Pin (Arduino name) | Constant name | Direction | Purpose | Source (`arduino-version.ino`) |
|---|---|---|---|---|
| 2 | `DIR_PIN` | OUTPUT | Motor direction (HIGH = reverse) | `setup()` / `loop()` |
| 3 | `STEP_PIN` | OUTPUT | Stepper step pulse | `loop()` |
| 4 | `SLEEP_RESET_PIN` | OUTPUT | Driver sleep/wake reset | `sleepDriver()` / `wakeDriver()` |
| 5 | `MS1_PIN` | OUTPUT | Microstepping select bit 1 | `setMicrostepping()` |
| 6 | `MS2_PIN` | OUTPUT | Microstepping select bit 2 | `setMicrostepping()` |
| 7 | `MS3_PIN` | OUTPUT | Microstepping select bit 3 | `setMicrostepping()` |
| 10 | `R_EN` | OUTPUT | H-bridge right side enable | `setHeaterPower()` / `heaterOff()` |
| 11 | `L_EN` | OUTPUT | H-bridge left side enable | `setHeaterPower()` / `heaterOff()` |
| 12 | `LPWM` | OUTPUT | Heater PWM (left H-bridge) | `setHeaterPower()` |
| 13 | `RPWM` | OUTPUT | Heater PWM (right H-bridge) | `setHeaterPower()` |
| A7 | `thermistorPin` | INPUT | Thermistor voltage divider ADC | `readingTemperature()` |

> `TODO(anouer): verify against the updated schematic` — the pin map above was
> read from the `.ino` only; the breadboard schematic (`docs/Schematic Diagram.pdf`)
> has not been re-checked against the latest wired build.

## Firmware overview

### Control loop structure

`loop()` runs a tight, non-blocking cycle:

1. Read the thermistor (`readingTemperature()`).
2. Accelerate the motor: while `motorSpeed > MIN_MOTOR_DELAY`, decrement the
   step delay by `ACCELERATION_STEP` (5 µs per loop).
3. Drive the stepper: toggle `STEP_PIN` with `delayMicroseconds(motorSpeed)`
   and `delayMicroseconds(motorSpeed)` — a square wave whose period halves as
   speed ramps up.
4. Call `pidControl(temperature)`, which runs the PID on `millis()` deltas and
   scales the heater PWM.

The stepper timing loop is **independent** of the PID loop: stepping is
delay-based (not interrupt-driven), and PID runs in the same `loop()` pass.

### PID and units

* Target: `targetTemp = 245.0 °C` (PET extrusion).
* PID constants: `Kp = 5.0`, `Ki = 0.1`, `Kd = 30.0`.
* Output: `constrain(output, 0, 235)` → `setHeaterPower((int)output)` drives
  `LPWM` (left side enabled), `RPWM` holds at 0, both with `analogWrite` PWM.
* Temperature read over a 100 kΩ pull-up: `R = 100 kΩ`, β = 3950, nominal 25 °C,
  10-bit ADC, 5 V reference.

### Timing

* Stepper step time: `motorSpeed` in **microseconds** (starts 1000 µs, decrements
  toward `MIN_MOTOR_DELAY` = 200 µs, i.e. 5 k steps/s flat).
* PID runs on `millis()` deltas; `lastTime` seeded in `setup()`.

### Serial output

The firmware prints to `Serial` (baud 9600):

* `Serial.println("Setup complete. Driver awake, motor starting...")` in `setup()`.
* `Serial.print("Temp stable: ")` / `Serial.println(" °C")` when `|error| < 1.0`.
* `setMicrostepping()` prints the selected `1/n` mode.

No structured JSON, no `deviceAcknowledged`, and no command-response handshake
exist today. Serial is informational only.

### Safety status (as the code is written today)

| Check | Status |
|---|---|
| Over-temperature cutoff (hard >245 °C guard) | **Not implemented** as a hard cutoff. `pidControl` *clips* PID output to `constrain(output, 0, 235)` and turns the heater off if `temp > 330` or `temp == -273.15`; there is no independent over-temperature kill switch. |
| Thermistor fault handling | **Partially present.** `voltage <= 0` returns `-273.15`; `pidControl` then stops the heater. Clock drift / open-circuit is not explicitly guarded. |
| Watchdog | **Not used** in this sketch (no `WDT` enabled). |

> If a remote command path (ESP32/MQTT) is enabled before these guards exist,
> the machine could run past a safe temperature without software protection.
> Treat the firmware as **unprotected** until the remotes are re-verified
> (see the "planned" sections below).

## Planned: network gateway (ESP32 + TXS0108E)

```mermaid
flowchart LR
  Arduino_Mega -->|Serial1 TX/RX (pin 18/19)| TXS0108E
  TXS0108E -->|UART2| ESP32
  ESP32 -->|Wi-Fi| MQTT_Broker
  MQTT_Broker -->|commands| ESP32
  ESP32 -->|UART2| TXS0108E
  TXS0108E -->|Serial1 RX/TX| Arduino_Mega
```

**Status: PLANNED.** The physical link is wired on the bench (Arduino Mega
Serial1 pins 18/19 ↔ TXS0108E bidirectional level shifter ↔ ESP32 UART2), but
**is not yet tested**, and **the protocol is not yet defined**. No pin numbers
below are final — mark confirmed after the bench test.

| Signal | Source | Target | Note |
|---|---|---|---|
| Mega TX1 (pin 18) → TXS B1 → A1 → ESP32 RX | Firmware TX path | ESP32 UART2 RX | `TODO(anouer): confirm board model` |
| ESP32 TX → A2 → B2 → Mega RX1 (pin 19) | Firmware RX path | Mega Serial1 RX | |
| TXS VCCA | ESP32 3V3 | TXS0108E power | |
| TXS VCCB | Mega 5V | TXS0108E power | |
| TXS OE | 3V3 | TXS0108E bidirectional enable | |
| GND | common | TXS0108E / ESP32 / Mega | |
| ESP32 5V / VIN | **temporarily** from Mega 5V | ESP32 power | For bench only — a dedicated 12 V→5 V buck converter is planned |

> **Note:** the `ESP32` is `UART2 (GPIO16/17 on a classic DevKit) -
> TODO(anouer): confirm board model`. Do not treat GPIO16/17 as final until the
> dev kit is confirmed.

## Telemetry / command contract

The firmware today can **really** produce these telemetry fields over the
backend contract (`docs/mqtt-contract.md` in the backend repo):

* `temperature` — thermistor reading (real).
* `targetTemperature` — PID setpoint (real, 245 °C).
* `heaterState` — on/off derived from `analogWrite` duty.
* `motorState` — stepper enabled (real).
* `motorSpeed` — step rate, but **not** wired to firmware output yet
  (`0–10000 RPM` is the backend contract range; the firmware reports the µs
  step delay, which is the inverse).

**Not available today** (per the audit; verify against the code when it is
implemented):

* no fan/cooling channel,
* no filament-diameter sensor,
* no energy meter,
* no RPM output,
* no `recordedAt` (the firmware never timestamps samples),
* no events/beacons.

Telemetry identity comes from the **topic only** — a payload naming another
machine is rejected. Commands are versioned (v1) envelopes with
`commandId` (= `machine_command_audit.id`), `type`, `value`, `issuedAt`,
`expiresAt` (TTL 30 s default).

## Safety and handling

* **245 °C heater:** hot surfaces, fire risk, burns. Keep the machine unplugged
  until wiring is finished and verified.
* **Mains / 12 V:** live mains side separated from logic; use an isolation
  transformer where appropriate. Wear safety glasses when a run is active.
* **Recommendation:** a thermal fuse or cartridge fuse on the heater supply as
  hardware protection independent of software. **This is a recommendation,
  not implemented.**
* **Recommendation:** a dedicated 12 V→5 V buck converter for the ESP32 instead
  of the temporary Mega-5V feed.
* Never modify wiring while power is connected. Let the heater cool before
  probing.

## How to update this document

Revisit exactly these `TODO(anouer)` markers when the firmware and the
schematic change:

1. `TODO(anouer): verify against the updated schematic` — under the pin map table.
2. `TODO(anouer): confirm board model` — under the ESP32 UART2 / GPIO16/17 note.
3. `TODO(anouer): confirm bench test passed / protocol version` — under the
   "Planned: network gateway" section, when the bench test and protocol are
   finalised.
4. `TODO(anouer): update pin map` — if the sketch changes any pin define.
5. `TODO(anouer): telemetry fields` — when `motorSpeed`-like or new telemetry
   leaves the firmware.
6. `TODO(anouer): over-temperature cutoff` — when a hard cutoff is added to the
   firmware (the "Safety status" table must then reflect it).

## Related docs

* Backend README: https://github.com/AnouerChouikhgithub/PET-Recycling-Filament-System-Backend/blob/main/README.md
* MQTT contract (backend): https://github.com/AnouerChouikhgithub/PET-Recycling-Filament-System-Backend/blob/main/docs/mqtt-contract.md
* API contract (backend): https://github.com/AnouerChouikhgithub/PET-Recycling-Filament-System-Backend/blob/main/docs/api-contract.md

## License

TODO(anouer): no LICENSE file found in this repo — add one.
