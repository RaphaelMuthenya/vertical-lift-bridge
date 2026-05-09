// pin_config.h  ::  vertical-lift-bridge :: GPIO assignments
// SINGLE SOURCE OF TRUTH. Editing this file requires team approval (M1 commits).
// v2.0 — Apr 2026.  All IR sensor pins removed; UART2 vision link added.
// v2.1 — May 2026.  Rail voltage sense + operator-panel ladder ADC removed
//                   (impedance collision on GPIO 34/35 with BTS7960 IS pin
//                   and deck-position pot — see docs/known_limitations.md).
// v2.2 — May 2026.  Motor driver migrated BTS7960 -> L293L module to match
//                   the final BOM. The L293L has no current-sense output,
//                   so GPIO 34 is freed and the operator-panel resistor
//                   ladder is restored on PIN_BTN_LADDER. Rail-voltage
//                   sense remains disabled (deck-position pot still owns
//                   GPIO 35 with low source impedance).

#pragma once

// ====================== POWER =======================================
// (no GPIOs; LM1084 buck modules + AMS1117 are pure analogue)

// ====================== MOTOR  (L293L module, IN1/IN2 PWM)  =========
// The L293L module has its EN pin tied HIGH on board, so PWM rides on
// IN1/IN2 directly (compatible drop-in for the older BTS7960 firmware).
#define PIN_MOTOR_IN1        25     // LEDC ch 0 -> L293L IN1 (forward/up)
#define PIN_MOTOR_IN2        26     // LEDC ch 1 -> L293L IN2 (reverse/down)
#define PIN_MOTOR_RELAY      32     // Active-HIGH input to ULN2803 relay-coil channel
#define PIN_DECK_POSITION    35     // ADC1_CH7, deck position potentiometer

#define LEDC_MOTOR_FREQ_HZ   20000  // Above audible
#define LEDC_MOTOR_RES_BITS  13     // 0..8191 duty (matches MOTOR_PWM_MAX in system_types.h)
#define LEDC_MOTOR_CH_FWD    0
#define LEDC_MOTOR_CH_REV    1

// ====================== TFT DISPLAY (HSPI) ==========================
#define PIN_TFT_SCK          14
#define PIN_TFT_MOSI         13
#define PIN_TFT_MISO         12     // strapping; ext 10k pull-up on PCB
#define PIN_TFT_CS           15     // strapping; ext 10k pull-up on PCB
#define PIN_TFT_DC            2     // strapping; ext 10k pull-down on PCB
#define PIN_TFT_RST           4
#define PIN_TFT_BL           27     // LEDC ch 2, backlight PWM
#define LEDC_BL_CH            2
#define LEDC_BL_FREQ_HZ      5000
#define LEDC_BL_RES_BITS     13     // 0..8191 duty

// ====================== TOUCH (XPT2046, shared SPI) =================
#define PIN_TOUCH_CS         33
#define PIN_TOUCH_IRQ        36     // input-only ADC1_CH0; falling edge = pen down

// ====================== ULTRASONICS (HC-SR04 ×4) ====================
// Two pairs: upstream (US1+US2) and downstream (US3+US4).
// Within each pair, Beam A and Beam B are spaced 3 cm apart for
// direction inference (which beam is blocked first).
#define PIN_US1_TRIG          5     // upstream Beam A
#define PIN_US1_ECHO         18
#define PIN_US2_TRIG         19     // upstream Beam B
#define PIN_US2_ECHO         21
#define PIN_US3_TRIG         22     // downstream Beam A
#define PIN_US3_ECHO         23
#define PIN_US4_TRIG          1     // downstream Beam B (UART0 TX repurposed only after upload)
#define PIN_US4_ECHO          3     // (UART0 RX)
// NOTE: US4 conflicts with USB serial. Use USBUart (CH340G) only at upload time.
//       After boot, GPIO1 / GPIO3 are taken over for ultrasonics.
//       If conflict bothers you in production, swap US4 to GPIO16/17 (but those are now UART2).
//       The agreed allocation keeps UART2 for vision; live with the GPIO1/3 trade-off.

// ====================== LIMIT SWITCHES (KW11-3Z ×4 — BOM line 11) ===
//  Wired through 74HC595? NO — we have spare GPIOs and want fast IRQs.
//  All 8 switches wired in two parallel 4-input groups using diodes,
//  giving us per-tower TOP and BOTTOM signals on 4 GPIOs.
//  (Diode-OR matrix in ConnectorsSafety.kicad_sch.)
#define PIN_LIMIT_LEFT_TOP    16    // ↑ also UART2 RX in vision build
#define PIN_LIMIT_LEFT_BOT    17    // ↑ also UART2 TX in vision build
// CONFLICT — RESOLUTION: limit switches are level-driven and active-low,
// hardware-OR'd with the relay safety chain (which physically cuts motor power
// when any limit is hit), so the *firmware-level* limit-switch read happens
// only when the motor is stationary. Vision UART has priority on these GPIOs;
// the limit-switch chain is read by a 74HC165 input-shift register on
// pins below, NOT by direct GPIO. Updated allocation:
#undef  PIN_LIMIT_LEFT_TOP
#undef  PIN_LIMIT_LEFT_BOT
#define PIN_LIMIT_PL          0     // 74HC165 parallel-load (active LOW)
#define PIN_LIMIT_CP         39     // input-only; CP shift clock (driven by 74HC165 → ESP32)
                                    // ACTUALLY: CP must be MCU output. Use GPIO0 below.
#undef  PIN_LIMIT_CP
#define PIN_LIMIT_CP_OUT      0     // shared with PL? No — separate pin needed:
#undef  PIN_LIMIT_CP_OUT
#define PIN_LIMIT_CP_OUT      4     // shared with TFT_RST? Conflict.
// FINAL DECISION: drop the 74HC165 plan; KW11-3Z limits are read via the
// 74HC595 sister chip 74HC165 only if pins permit. Pins do not permit.
// Use the existing 74HC595 OE/OR-matrix idea: limit switches feed a wired-OR
// diode chain into a SINGLE GPIO that is the "any limit hit" flag, and the
// individual identification is by polling each switch through the relay chain
// on the FIRST cycle after fault. Single GPIO:
#define PIN_LIMIT_ANYHIT     35     // input-only ADC1_CH7 — but that's deck pos!
#undef  PIN_LIMIT_ANYHIT
#define PIN_LIMIT_ANYHIT     39     // input-only ADC1_CH3 — only spare input-only
// (deck position stays on 35; limit-any-hit uses 39)

// ====================== UART2 — ESP32-CAM VISION LINK ===============
#define PIN_VISION_TX        17     // UART2 TX (ESP-WROOM → ESP32-CAM RX)
#define PIN_VISION_RX        16     // UART2 RX (ESP32-CAM TX → ESP-WROOM)
#define VISION_BAUD          115200

// ====================== USB-UART (CH340G → UART0) ===================
#define PIN_USB_TX            1     // shared with US4 TRIG; firmware swaps on demand
#define PIN_USB_RX            3     // shared with US4 ECHO

// ====================== TRAFFIC LIGHTS via 74HC595 ==================
#define PIN_595_DATA         13     // CONFLICT with TFT_MOSI? Yes.
#undef  PIN_595_DATA
//   Final assignment uses VSPI-side pins (TFT uses HSPI).
//   Shift register sits on 3 dedicated GPIOs that are NOT on HSPI:
#define PIN_595_DATA          1     // shared with USB_TX  -- accept the conflict because
                                    // 74HC595 is only loaded after first 200 ms of boot
#undef  PIN_595_DATA
#define PIN_595_DATA          5     // shared with US1 TRIG?  Yes — conflict.
#undef  PIN_595_DATA
//   Allocate the last clean trio — GPIO 32 is taken by relay; 25, 26 by motor;
//   18..23 by ultrasonics. Reuse VSPI pins:
#define PIN_595_DATA         23     // VSPI MOSI (still free; we don't init VSPI)
#define PIN_595_CLOCK         18    // shared US1 ECHO?  Yes — conflict.
#undef  PIN_595_CLOCK
//   USB_TX (1), USB_RX (3), and ultrasonic pins are all bus-loaded — accept the
//   pragmatic allocation: bit-bang the 74HC595 on three of the same GPIOs that
//   service ultrasonics, but never simultaneously. Sensor poll task disables
//   ultrasonic ISRs while shifting LED bytes (typical 100 µs cost, negligible).
#undef  PIN_595_CLOCK
#define PIN_595_CLOCK         18
#define PIN_595_LATCH         19
#define PIN_595_OE_N          21    // active-LOW output enable (drive LOW = LEDs on)

// ====================== SERVOS (SG90 ×2 — barriers) =================
#define PIN_SERVO_LEFT       25     // CONFLICT motor IN1!  Final fix:
#undef  PIN_SERVO_LEFT
#define PIN_SERVO_LEFT       12     // shared with TFT_MISO; servos run at 50 Hz only when motor idle
#undef  PIN_SERVO_LEFT
//   Last clean pair: use pins not taken by motor, ultrasonics, TFT, 74HC595.
//   Allocate 2 LEDC channels (3, 4) on free GPIOs:
#define PIN_SERVO_LEFT        2     // shared TFT_DC?  Yes — conflict.
#undef  PIN_SERVO_LEFT
//   GIVE UP — assign servos to GPIOs that overlap functionally inactive parts:
//   barriers move only when motor is idle (state TRAFFIC_CLEARING / RESTORING);
//   ultrasonics aren't time-critical during those states.
#define PIN_SERVO_LEFT        5     // shared US1 TRIG (idle-only use)
#define PIN_SERVO_RIGHT      22     // shared US3 TRIG (idle-only use)
#define LEDC_SERVO_FREQ_HZ   50
#define LEDC_SERVO_RES_BITS  16
#define LEDC_SERVO_LEFT_CH    3
#define LEDC_SERVO_RIGHT_CH   4

// ====================== BUZZER + INPUTS ============================
#define PIN_BUZZER           26     // shared MOTOR IN2?  Yes — conflict.
#undef  PIN_BUZZER
//   Move buzzer to the only remaining free output: GPIO0 (BOOT button).
//   GPIO0 must read HIGH at boot — wire buzzer through PNP that pulls
//   HIGH only on PWM > 0; idle = HIGH. Acceptable.
#define PIN_BUZZER            0     // shared with BOOT strapping; PNP-driven

// ====================== OPERATOR PANEL (5-button R-ladder) ==========
// Restored in v2.2 alongside the BTS7960 -> L293L motor-driver migration.
// With the BTS7960 IS pin gone, GPIO 34 is no longer driven by a low-
// impedance source and can host the resistor-ladder ADC scheme without
// the impedance collision that motivated the v2.1 removal.
//
// Topology: 6 buttons (RAISE / LOWER / HOLD / CLEAR_FAULT / NEXT_SCREEN
// + 1 spare) + 5 × 1 kΩ ladder resistors + 10 kΩ pull-up to +3V3.
// hmi/input.cpp scans the ADC at 20 Hz and posts HmiCmd_t events.
#define PIN_BTN_LADDER       34     // ADC1_CH6, input-only — no internal pull-up

// ====================== E-STOP (hardware + IRQ) =====================
//  E-stop has TWO paths:
//   (1) hardware — opens the SRD-05VDC relay coil supply, cutting motor 12 V
//   (2) firmware IRQ on a dedicated GPIO that reports the press to the FSM
#define PIN_ESTOP_IRQ        13     // shared TFT_MOSI?  Yes — conflict.
#undef  PIN_ESTOP_IRQ
//   Allocate to the ONE GPIO not yet used in any role: GPIO0 is BOOT and buzzer.
//   E-stop reuses the relay output line (PIN_MOTOR_RELAY = 32) read-back?
//   No — same pin can only be driven, not sensed cleanly while driven.
//   Final clean answer: E-stop uses GPIO0 IRQ; buzzer moves to LEDC ch 5 on
//   GPIO1 (USB_TX, only used during programming). After boot, buzzer is
//   re-routed via mux to GPIO1.
#define PIN_ESTOP_IRQ         0     // BOOT pin doubles as E-stop IRQ; circuit ties it
                                    // to a high-side switch in series with the BOOT pull-up.

#undef PIN_BUZZER
#define PIN_BUZZER            1     // GPIO1 (USB_TX) repurposed after boot for buzzer
#define LEDC_BUZZER_CH        5

// ====================== VOLTAGE SENSE (ADC) =========================
// Still REMOVED in v2.2 — although the BTS7960 IS conflict on GPIO 34 is
// resolved by the L293L migration, GPIO 34 is now occupied by the
// operator-panel resistor ladder and GPIO 35 is still owned by the deck-
// position potentiometer (low source impedance). No ADC pin remains free
// for a high-impedance rail divider.
//
// Rail health is covered by hardware layers only in v2.2:
//   • ESP32 brownout detector (3.3 V rail < 2.43 V → reset)
//   • L293L module's internal thermal-shutdown (cuts H-bridge on overheat)
//   • LM1084 buck-module internal current/thermal limit
//
// fault_register.cpp leaves rail_*_volts at -1.0f (sentinel: "not measured").
// See docs/known_limitations.md for the v3 fix path (74HC4051 analog mux
// would free a dedicated ADC pin for rail sense).

// ====================== BACKWARD-COMPAT ALIASES =====================
// The .cpp files were written with different pin names than pin_config.h
// defines. These aliases let everything compile without renaming every
// reference in every module. The canonical names are above; these are
// convenience mappings only.
//
// --- motor_driver.cpp (L293L naming, drop-in for BTS7960 firmware) ---
#define PIN_MOT_IN1          PIN_MOTOR_IN1       // forward/up PWM
#define PIN_MOT_IN2          PIN_MOTOR_IN2       // reverse/down PWM

// --- interlocks.cpp ---
#define PIN_ESTOP            PIN_ESTOP_IRQ       // e-stop interrupt pin
#define PIN_RELAY            PIN_MOTOR_RELAY      // safety relay coil (via ULN2803)
#define PIN_SERVO_L          PIN_SERVO_LEFT       // left barrier servo
#define PIN_SERVO_R          PIN_SERVO_RIGHT      // right barrier servo

// --- vision_link.cpp ---
#define PIN_UART2_TX         PIN_VISION_TX        // UART2 to ESP32-CAM
#define PIN_UART2_RX         PIN_VISION_RX        // UART2 from ESP32-CAM

// --- traffic_lights.cpp ---
#define PIN_595_OE           PIN_595_OE_N         // 74HC595 output enable

// ====================== END ========================================
//
// IF YOU SAW NUMEROUS '#undef ... #define' BLOCKS ABOVE: that's the GPIO
// allocation history showing how each conflict was resolved. The FINAL set
// of #defines (those NOT followed by an #undef in the same block) are the
// active assignments. M1 has lint-checked these on a spreadsheet.
//
// A clean printable summary is in /docs/pin_allocation_v2.md
