/*
 * lasertag.ino
 *
 * OpenLASIR Laser Tag example for Arduino.
 * Port of the MicroPython example (OpenLASIR/examples/micropython/lasertag/main.py).
 *
 * This example demonstrates sending and receiving laser tag fire packets
 * using the OpenLASIR protocol (a modified NEC protocol) via the
 * Arduino-IRremote library.
 *
 * Two or more devices running this sketch can play laser tag:
 *   - Press the fire button to shoot (sends an IR laser tag packet)
 *   - When hit, the device prints the attacker's color and is disabled
 *     for DISABLE_TIME_AFTER_TAG_MS milliseconds
 *   - Rate limiting prevents firing faster than once per FIRE_RATE_LIMIT_MS
 *
 * Hardware needed:
 *   - IR LED or emitter module (connected to IR_SEND_PIN)
 *   - IR receiver module (connected to IR_RECEIVE_PIN)
 *   - Push button (connected to FIRE_BUTTON_PIN, active LOW with pull-up)
 *
 * Setup:
 *   1. Set the pin numbers below to match your wiring
 *   2. Set MY_BLOCK_ID and MY_DEVICE_ID (see OpenLASIR README for allocation)
 *   3. Set MY_COLOR to your desired color
 *   4. Upload to two or more boards and play*! 
 *   (*all it does it print the hits to the serial monitor, so not that exciting...)
 *
 ************************************************************************************
 * MIT License — see OpenLASIR repository for full license text.
 ************************************************************************************
 */

#include <Arduino.h>

// ── Protocol selection ──────────────────────────────────────────────────────────
// Only enable the OpenLASIR decoder (and optionally the universal distance decoder
// for debugging). This keeps the binary small and avoids false decodes from NEC.
#define DECODE_OPENLASIR
// #define DECODE_DISTANCE_WIDTH  // Uncomment for debugging unknown signals

// ── Pin definitions ─────────────────────────────────────────────────────────────
// Change these to match your hardware. Defaults below are for a typical ESP32 setup.
// If you are using an Arduino Uno / Nano, common choices are:
//   IR_RECEIVE_PIN 2, IR_SEND_PIN 3, FIRE_BUTTON_PIN 4
#if defined(ESP32)
#define IR_RECEIVE_PIN   15
#define IR_SEND_PIN       4
#define FIRE_BUTTON_PIN   0   // BOOT button on many ESP32 dev boards
#elif defined(ARDUINO_ARCH_RP2040)
#define IR_RECEIVE_PIN   15
#define IR_SEND_PIN      16
#define FIRE_BUTTON_PIN  14
#else
// AVR / default
#define IR_RECEIVE_PIN    2
#define IR_SEND_PIN       3
#define FIRE_BUTTON_PIN   4
#endif

// Arduino-IRremote library with OpenLASIR support. As of 2-10-2026, the
// https://github.com/Arduino-IRremote/Arduino-IRremote library has merged
// in OpenLASIR support, but a new build with it hasn't been released to
// the Arduino Library Manager yet. So you may need to download the latest
// version from the GitHub repository and install it manually if the latest
// release is still v4.5.0.
#include <IRremote.hpp>

// Path is relative — adjust if your folder layout differs.
// If you installed OpenLASIR_Utils.h to your Arduino libraries 
// folder, or you have it in the same folder as this sketch, you can
// change this to: #include <OpenLASIR_Utils.h>
#include "../../../arduino/OpenLASIR_Utils.h"

// ── Player / device configuration ───────────────────────────────────────────────
// Block ID: identifies your project/badge type (see OpenLASIR README for allocation).
// Use 0 (free-for-all) if you don't have an assigned block.
#define MY_BLOCK_ID    0

// Device ID: unique per device within your block (0-255).
#define MY_DEVICE_ID   0

// Your color: one of the OPENLASIR_COLOR_* constants.
// This is the color that will be sent in your fire packets so opponents know
// which color hit them.
#define MY_COLOR       OPENLASIR_COLOR_RED

// ── Game parameters ─────────────────────────────────────────────────────────────
#define FIRE_RATE_LIMIT_MS        1000   // Minimum milliseconds between shots
#define DISABLE_TIME_AFTER_TAG_MS 2000   // Milliseconds disabled after being hit

// ── Global state ────────────────────────────────────────────────────────────────
static unsigned long lastFireTimeMs    = 0;
static unsigned long reEnableAtMs      = 0;
static bool          isDisabled        = false;

// Pre-encoded fire packet (computed once in setup)
static uint8_t  fireAddress;
static uint16_t fireCommand;

// ─────────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) \
    || defined(USBCON) || defined(SERIALUSB_PID) || defined(ARDUINO_ARCH_RP2040) \
    || defined(ARDUINO_attiny3217)
    delay(4000); // Wait for Serial monitor on boards with native USB
#endif

    Serial.println(F("──────────────────────────────────────"));
    Serial.println(F(" OpenLASIR Laser Tag Example"));
    Serial.println(F("──────────────────────────────────────"));

    // ── Fire button ──
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);

    // ── IR receiver ──
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    Serial.print(F("IR receiver on pin "));
    Serial.println(IR_RECEIVE_PIN);

    // ── IR sender ──
    // IrSender uses IR_SEND_PIN by default (defined above)
    Serial.print(F("IR sender on pin "));
    Serial.println(IR_SEND_PIN);

    // ── Pre-encode the fire packet ──
    OpenLASIR_encodeLaserTagFire(MY_BLOCK_ID, MY_DEVICE_ID, MY_COLOR,
            fireAddress, fireCommand);

    Serial.print(F("Block ID: "));
    Serial.print(MY_BLOCK_ID);
    Serial.print(F("  Device ID: "));
    Serial.print(MY_DEVICE_ID);
    Serial.print(F("  Color: "));
    Serial.println(OpenLASIR_getColorName(MY_COLOR));

    Serial.print(F("Fire packet -> address=0x"));
    Serial.print(fireAddress, HEX);
    Serial.print(F(" command=0x"));
    Serial.println(fireCommand, HEX);

    Serial.println(F("Ready! Press the fire button to shoot."));
    Serial.println();
}

void loop() {
    unsigned long now = millis();

    // ── Re-enable after being tagged ────────────────────────────────────────────
    if (isDisabled && now >= reEnableAtMs) {
        isDisabled = false;
        IrReceiver.start(); // Resume receiving
        Serial.println(F(">> Re-enabled after tag cooldown."));
        Serial.println();
    }

    // ── Check for incoming IR (hits) ────────────────────────────────────────────
    if (!isDisabled && IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol == OPENLASIR) {
            uint8_t  rxAddr = IrReceiver.decodedIRData.address;
            uint16_t rxCmd  = IrReceiver.decodedIRData.command;

            OpenLASIR_Packet pkt;
            if (OpenLASIR_decodeLaserTagFire(rxAddr, rxCmd, pkt)) {
                // We got hit!
                uint8_t r, g, b;
                OpenLASIR_getColorRGB(pkt.data, r, g, b);

                Serial.print(F("!! HIT by Block "));
                Serial.print(pkt.blockId);
                Serial.print(F(" Device "));
                Serial.print(pkt.deviceId);
                Serial.print(F(" — Color: "));
                Serial.print(OpenLASIR_getColorName(pkt.data));
                Serial.print(F(" ("));
                Serial.print(r); Serial.print(F(", "));
                Serial.print(g); Serial.print(F(", "));
                Serial.print(b); Serial.println(F(")"));

                Serial.print(F("   Disabled for "));
                Serial.print(DISABLE_TIME_AFTER_TAG_MS);
                Serial.println(F(" ms"));

                // Disable receiving & firing for the cooldown period
                isDisabled  = true;
                reEnableAtMs = now + DISABLE_TIME_AFTER_TAG_MS;
                IrReceiver.stop(); // Stop the receiver during cooldown

                // TODO: Set your LEDs / NeoPixels to the attacker's color here!
                // e.g. setLEDColor(r, g, b);
            } else {
                // Non-laser-tag OpenLASIR packet — print for debugging
                Serial.print(F("Received OpenLASIR mode="));
                Serial.print(OpenLASIR_getModeName(pkt.mode));
                Serial.print(F(" from Block "));
                Serial.print(pkt.blockId);
                Serial.print(F(" Device "));
                Serial.println(pkt.deviceId);
            }
        }
        IrReceiver.resume(); // Ready for the next frame
    }

    // ── Fire button ─────────────────────────────────────────────────────────────
    if (digitalRead(FIRE_BUTTON_PIN) == LOW) {
        // Only fire if not disabled and rate limit has elapsed
        if (!isDisabled && (now - lastFireTimeMs) >= FIRE_RATE_LIMIT_MS) {
            lastFireTimeMs = now;

            // Stop receiver so we don't pick up our own transmission
            IrReceiver.stop();

            Serial.print(F("<< FIRE!  addr=0x"));
            Serial.print(fireAddress, HEX);
            Serial.print(F(" cmd=0x"));
            Serial.println(fireCommand, HEX);
            Serial.flush(); // Flush before sending to avoid serial interrupts during IR

            IrSender.sendOpenLASIR(fireAddress, fireCommand, 0);

            // Restart receiver after send.
            // We must use start() here, not restartAfterSend(), because we
            // explicitly stop()-ed the receiver above.  restartAfterSend() is
            // a NOP on platforms where sending doesn't use the receive timer,
            // which would leave the receiver permanently stopped.
            IrReceiver.start();
        }
    }

    delay(10); // Small loop delay to debounce and save power
}
