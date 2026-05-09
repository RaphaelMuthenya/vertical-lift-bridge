// ============================================================================
// hmi/input.cpp — 5-button resistor-ladder operator panel.
// Owner: M4 Abigael
//
// Five momentary push-buttons share one ADC pin (PIN_BTN_LADDER = GPIO 34)
// through a 5-step series ladder of 1 kΩ resistors with a 10 kΩ pull-up to
// +3V3. Each press sources a distinct fraction of the rail and the band
// detector below maps the reading to an HmiCmd_t. Released = ADC near
// full-scale (≈ 4095). The ladder is deliberately the inverse of the
// touchscreen so a cracked panel still leaves a usable input path.
//
// Topology (per docs/05_electronics_design.md §5.5a):
//
//                +3V3
//                  |
//                R_pu = 10 kΩ
//                  |
//   ADC <----------+----BTN1----R1----BTN2----R2----BTN3----R3----BTN4----R4----BTN5----R5----GND
//                                  (R1..R5 each = 1 kΩ; resistors come from BOM line 18)
//
// Theoretical bands at 12-bit resolution (3.3 V FSR, 11 dB attenuation):
//   no press : ADC ≈ 4095            (V ≈ 3.30 V)
//   BTN1     : ADC ≈ 1365 ± 80       (V ≈ 1.10 V)  -> HMI_CMD_RAISE
//   BTN2     : ADC ≈ 1167 ± 80       (V ≈ 0.94 V)  -> HMI_CMD_LOWER
//   BTN3     : ADC ≈  943 ± 80       (V ≈ 0.76 V)  -> HMI_CMD_HOLD
//   BTN4     : ADC ≈  683 ± 80       (V ≈ 0.55 V)  -> HMI_CMD_CLEAR_FAULT
//   BTN5     : ADC ≈  372 ± 80       (V ≈ 0.30 V)  -> HMI_CMD_NEXT_SCREEN
//
// The implementation uses a debounce-by-stable-N-samples scheme rather than
// an RC filter so that any wiring noise (long ribbon cables in the demo
// rig) is rejected without a hardware change. Edge detection ensures one
// press = one HmiCmd_t event regardless of how long the button is held.
// ============================================================================
#include "input.h"
#include "display.h"           // for HmiCmd_t + hmi_cmd_post()
#include "../pin_config.h"
#include "../system_types.h"
#include <Arduino.h>

#define BTN_NONE          0
#define BTN_RAISE         1
#define BTN_LOWER         2
#define BTN_HOLD          3
#define BTN_CLEAR_FAULT   4
#define BTN_NEXT_SCREEN   5

#define BAND_TOL          120     // ±counts around the centre voltage
#define DEBOUNCE_SAMPLES  3       // stable readings before commit

static uint8_t band_to_btn(uint16_t adc) {
    if (adc > 3500)                         return BTN_NONE;
    if (adc > 1365 - BAND_TOL && adc < 1365 + BAND_TOL) return BTN_RAISE;
    if (adc > 1167 - BAND_TOL && adc < 1167 + BAND_TOL) return BTN_LOWER;
    if (adc >  943 - BAND_TOL && adc <  943 + BAND_TOL) return BTN_HOLD;
    if (adc >  683 - BAND_TOL && adc <  683 + BAND_TOL) return BTN_CLEAR_FAULT;
    if (adc >  372 - BAND_TOL && adc <  372 + BAND_TOL) return BTN_NEXT_SCREEN;
    return BTN_NONE;              // dead-band reading (transition / noise)
}

static HmiCmd_t btn_to_cmd(uint8_t btn) {
    switch (btn) {
    case BTN_RAISE:        return HMI_CMD_RAISE;
    case BTN_LOWER:        return HMI_CMD_LOWER;
    case BTN_HOLD:         return HMI_CMD_HOLD;
    case BTN_CLEAR_FAULT:  return HMI_CMD_CLEAR_FAULT;
    case BTN_NEXT_SCREEN:  return HMI_CMD_NEXT_SCREEN;
    default:               return HMI_CMD_NONE;
    }
}

void input_init(void) {
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BTN_LADDER, ADC_11db);   // 0..3.3 V range
    Serial.println("[input] init OK (5-button R-ladder on GPIO 34)");
}

void input_tick(void) {
    // 4-sample average to suppress single-sample ADC noise.
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) sum += analogRead(PIN_BTN_LADDER);
    uint16_t adc = (uint16_t)(sum >> 2);
    uint8_t  raw = band_to_btn(adc);

    static uint8_t  s_pending     = BTN_NONE;
    static uint8_t  s_pending_n   = 0;
    static uint8_t  s_committed   = BTN_NONE;

    if (raw == s_pending) {
        if (s_pending_n < 0xFF) s_pending_n++;
    } else {
        s_pending   = raw;
        s_pending_n = 1;
    }

    // Commit only after N stable samples and only on a real edge.
    if (s_pending_n >= DEBOUNCE_SAMPLES && s_pending != s_committed) {
        if (s_pending != BTN_NONE) {
            HmiCmd_t cmd = btn_to_cmd(s_pending);
            if (cmd != HMI_CMD_NONE) hmi_cmd_post(cmd);
        }
        s_committed = s_pending;
    }
}
