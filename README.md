# PET Plastic Recycling to 3D Printer Filament System

An Arduino-based system that turns PET plastic waste into usable 3D printer filament through a custom-built extrusion control system. Independently designed and developed as a solo project, currently in its 3rd hardware iteration.

**Status:** Actively developed · 2024 – Present
**Type:** Solo project

---

## Overview

This project addresses plastic waste by converting PET bottles into functional 3D printer filament, combining precise thermal control, mechanical extrusion, and custom-designed 3D-printed and CAD-modeled components.

The system controls extrusion temperature and filament feed rate in real time, using closed-loop PID control and a microstepping stepper driver for consistent, printable filament output.

## How It Works

- **Temperature control:** PID control loop targeting 245°C, using a thermistor for feedback and a PWM-driven heater for actuation.
- **Extrusion drive:** Microstepping stepper driver (1/2 to 1/16 step resolution, up to 3200 steps/revolution) for smooth, controlled filament extrusion.
- **Motion profile:** Acceleration ramping implemented in firmware to avoid mechanical strain and maintain consistent extrusion speed.
- **Mechanical structure:** Custom-designed body and drivetrain components, modeled in SolidWorks and 3D-printed in-house.

## Repository Structure

```
pet-recycling-filament-system/
├── firmware/
│   └── code.ino                  # Main Arduino firmware (PID control, stepper driving)
├── cad/
│   ├── base-assembly/            # Main structural assembly (SolidWorks + STEP)
│   └── 3d-printed-parts/         # Individual printable components (STEP files)
├── docs/
│   └── Schematic-Diagram_bb.pdf  # Wiring / breadboard schematic
└── README.md
```

## Hardware

| Component            | Role                                      |
|-----------------------|--------------------------------------------|
| Arduino                | Main controller                            |
| Thermistor              | Temperature feedback for PID loop          |
| PWM-driven heater        | Heating element, actuated via PID output   |
| Stepper motor + driver    | Filament extrusion drive (microstepping)   |
| 3D-printed structural parts | Gears, separators, fixatives, pulley system |

Full mechanical design files are available in [`/cad`](./cad), including the base assembly and all individual 3D-printed parts. STEP files can be opened in any CAD software (SolidWorks, Fusion 360, FreeCAD).

## Wiring

See [`docs/Schematic-Diagram_bb.pdf`](./docs/Schematic-Diagram_bb.pdf) for the full breadboard/wiring schematic.

## Development History

This system has been refined across multiple iterations over several years of solo development:
- Ongoing improvements to thermal stability, extrusion consistency, and mechanical durability
- Current version (V3.2 base assembly) reflects the most recent structural redesign

## Future Improvements

- [ ] Closed-loop filament diameter measurement and feedback
- [ ] Spool winding automation
- [ ] Enclosure design for improved thermal consistency

## License

This project is licensed under the MIT License — see [`LICENSE`](./LICENSE) for details.

## Author

**Anouer Chouikh**
Computer Engineering Student — Robotics & IoT
[Portfolio](https://chouikh-anouer.netlify.app/) · [LinkedIn](https://linkedin.com/in/anouer-chouikh-303306220/) · [GitHub](https://github.com/AnouerChouikhgithub)
