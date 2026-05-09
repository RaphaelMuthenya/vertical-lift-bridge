# /docs/figures — schematic + layout screenshots

The PNGs in this directory were exported from the **pre-v2.2 KiCad
project** that was deleted on 2026-05-03. Their content (symbol/footprint
choices, sheet names) does **not** match the v2.2 design captured per
`member_guides/M5_Ian_PCB_Power_Safety.md`.

## To regenerate (after the new KiCad project is built)

For each KiCad 10 sub-sheet the new project ships with, do:

1. Open the sub-sheet in the schematic editor.
2. **File → Export → Schematic to PDF** (or PNG via the print dialog).
3. Save into this directory using the v2.2 sheet name. Suggested filenames:
   - `power_supply.png`
   - `esp32_module.png`
   - `usb_programming.png`
   - `motor_driver.png`
   - `shift_reg_lights.png`
   - `connectors_safety.png`
   - `tft_camera.png`

For the PCB layout, use **File → Export → SVG** (or use GerbView's PNG
export of the assembled Gerber set) and save as `pcb_layout_top.png` /
`pcb_layout_bottom.png` / `pcb_3d.png` (the Alt+3 view).

## What's currently here (v2.0 / pre-2026-05-03)

- `ConnectorSafety.png`
- `ESP32_Module.png`
- `MotorDrivers.png`     ← shows BTS7960, not L293L
- `PCB_PowerSupply.png`  ← shows LM2596, not LM1084
- `ShiftRegLEDs.png`
- `USBUart.png`          ← shows CP2102N, not CH340G

These can stay until the v2.2 replacements exist (so the project history
isn't lost), but **do not cite them in the demo report** — they will
mislead the lecturer about which components are actually fitted.
