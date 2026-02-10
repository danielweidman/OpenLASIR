/*
 * OpenLASIR_Utils.h
 *
 * Utility functions for the OpenLASIR protocol — Arduino / C++ port.
 * Provides packet encoding/decoding, color lookups, and mode name lookups,
 * mirroring the functionality of the MicroPython openlasir_utils.py module.
 *
 * Author: Dani Weidman (github.com/danielweidman; dani.pink)
 *
 * Usage:
 *   #include "OpenLASIR_Utils.h"
 *
 *   uint8_t  address;
 *   uint16_t command;
 *   OpenLASIR_encodeLaserTagFire(0, 42, OPENLASIR_COLOR_RED, address, command);
 *   IrSender.sendOpenLASIR(address, command, 0);
 *
 ************************************************************************************
 * MIT License
 *
 * Copyright (c) 2026 Dani Weidman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************************
 */

#ifndef OPENLASIR_UTILS_H
#define OPENLASIR_UTILS_H

#include <stdint.h>

// ─────────────────────────────────────────────
// Colors (3-bit data field, values 0-7)
// ─────────────────────────────────────────────
#define OPENLASIR_COLOR_CYAN     0
#define OPENLASIR_COLOR_MAGENTA  1
#define OPENLASIR_COLOR_YELLOW   2
#define OPENLASIR_COLOR_GREEN    3
#define OPENLASIR_COLOR_RED      4
#define OPENLASIR_COLOR_BLUE     5
#define OPENLASIR_COLOR_ORANGE   6
#define OPENLASIR_COLOR_WHITE    7

#define OPENLASIR_NUM_COLORS     8

// RGB values for each color (stored in PROGMEM-friendly arrays below)
struct OpenLASIR_RGB {
    uint8_t r, g, b;
};

#ifdef __AVR__
#include <avr/pgmspace.h>
static const OpenLASIR_RGB OPENLASIR_COLOR_RGB[] PROGMEM = {
#else
static const OpenLASIR_RGB OPENLASIR_COLOR_RGB[] = {
#endif
    {  0, 255, 255},   // 0 = Cyan
    {255,   0, 255},   // 1 = Magenta
    {255, 255,   0},   // 2 = Yellow
    {  0, 255,   0},   // 3 = Green
    {255,   0,   0},   // 4 = Red
    {  0,   0, 255},   // 5 = Blue
    {255, 165,   0},   // 6 = Orange
    {255, 255, 255},   // 7 = White
};

// Color names (index matches color number)
static const char *const OPENLASIR_COLOR_NAMES[] = {
    "Cyan", "Magenta", "Yellow", "Green",
    "Red", "Blue", "Orange", "White"
};

// ─────────────────────────────────────────────
// Modes (5-bit field, values 0-31)
// ─────────────────────────────────────────────
#define OPENLASIR_MODE_LASER_TAG_FIRE                          0
#define OPENLASIR_MODE_USER_PRESENCE_ANNOUNCEMENT              1
#define OPENLASIR_MODE_BASE_STATION_PRESENCE_ANNOUNCEMENT      2
#define OPENLASIR_MODE_USER_TO_USER_HANDSHAKE_INITIATION       3
#define OPENLASIR_MODE_USER_TO_USER_HANDSHAKE_RESPONSE         4
#define OPENLASIR_MODE_USER_TO_BASE_STATION_HANDSHAKE_INITIATION  5
#define OPENLASIR_MODE_USER_TO_BASE_STATION_HANDSHAKE_RESPONSE    6
#define OPENLASIR_MODE_BASE_STATION_TO_USER_HANDSHAKE_INITIATION  7
#define OPENLASIR_MODE_BASE_STATION_TO_USER_HANDSHAKE_RESPONSE    8
#define OPENLASIR_MODE_COLOR_SET_TEMPORARY                     9
#define OPENLASIR_MODE_COLOR_SET_PERMANENT                    10
#define OPENLASIR_MODE_GENERAL_INTERACT                       11

#define OPENLASIR_NUM_DEFINED_MODES  12

// Mode names (index matches mode number, up to the defined modes)
static const char *const OPENLASIR_MODE_NAMES[] = {
    "laser_tag_fire",
    "user_presence_announcement",
    "base_station_presence_announcement",
    "user_to_user_handshake_initiation",
    "user_to_user_handshake_response",
    "user_to_base_station_handshake_initiation",
    "user_to_base_station_handshake_response",
    "base_station_to_user_handshake_initiation",
    "base_station_to_user_handshake_response",
    "color_set_temporary",
    "color_set_permanent",
    "general_interact"
};

// ─────────────────────────────────────────────
// Decoded packet structure
// ─────────────────────────────────────────────
struct OpenLASIR_Packet {
    uint8_t  blockId;    // Block ID (from address)
    uint8_t  deviceId;   // Device ID (command bits 0-7)
    uint8_t  mode;       // Mode number (command bits 8-12)
    uint8_t  data;       // Data / color (command bits 13-15)
};

// ─────────────────────────────────────────────
// Encoding functions
// ─────────────────────────────────────────────

/**
 * Encode a general OpenLASIR packet into address + command.
 *
 * @param blockId   Block ID (0-255)
 * @param deviceId  Device ID (0-255)
 * @param mode      Mode number (0-31)
 * @param data      Data / color (0-7)
 * @param[out] address  8-bit address for sendOpenLASIR()
 * @param[out] command  16-bit command for sendOpenLASIR()
 */
inline void OpenLASIR_encodeGeneralPacket(uint8_t blockId, uint8_t deviceId,
        uint8_t mode, uint8_t data,
        uint8_t &address, uint16_t &command) {
    address = blockId;
    command = ((uint16_t)(data & 0x07) << 13)
            | ((uint16_t)(mode & 0x1F) << 8)
            | deviceId;
}

/**
 * Encode a laser tag fire packet.
 *
 * @param blockId   Block ID (0-255)
 * @param deviceId  Device ID (0-255)
 * @param color     Color number (0-7), use OPENLASIR_COLOR_* constants
 * @param[out] address  8-bit address for sendOpenLASIR()
 * @param[out] command  16-bit command for sendOpenLASIR()
 */
inline void OpenLASIR_encodeLaserTagFire(uint8_t blockId, uint8_t deviceId,
        uint8_t color,
        uint8_t &address, uint16_t &command) {
    OpenLASIR_encodeGeneralPacket(blockId, deviceId,
            OPENLASIR_MODE_LASER_TAG_FIRE, color,
            address, command);
}

// ─────────────────────────────────────────────
// Decoding functions
// ─────────────────────────────────────────────

/**
 * Decode a general OpenLASIR packet from address + command.
 *
 * @param address  8-bit address received from decodeOpenLASIR()
 * @param command  16-bit command received from decodeOpenLASIR()
 * @return Decoded packet structure with blockId, deviceId, mode, and data.
 */
inline OpenLASIR_Packet OpenLASIR_decodeGeneralPacket(uint8_t address, uint16_t command) {
    OpenLASIR_Packet pkt;
    pkt.blockId  = address;
    pkt.deviceId = command & 0xFF;           // bits 0-7
    pkt.mode     = (command >> 8) & 0x1F;    // bits 8-12
    pkt.data     = (command >> 13) & 0x07;   // bits 13-15
    return pkt;
}

/**
 * Decode a laser tag fire packet from address + command.
 * Same as decodeGeneralPacket; the data field is the color number.
 * Returns false if the mode is not LASER_TAG_FIRE.
 *
 * @param address  8-bit address received from decodeOpenLASIR()
 * @param command  16-bit command received from decodeOpenLASIR()
 * @param[out] packet  Decoded packet structure
 * @return true if the packet is a laser_tag_fire packet, false otherwise.
 */
inline bool OpenLASIR_decodeLaserTagFire(uint8_t address, uint16_t command,
        OpenLASIR_Packet &packet) {
    packet = OpenLASIR_decodeGeneralPacket(address, command);
    return (packet.mode == OPENLASIR_MODE_LASER_TAG_FIRE);
}

// ─────────────────────────────────────────────
// Name lookup helpers
// ─────────────────────────────────────────────

/**
 * Get the human-readable name for a mode number.
 * @return Mode name string, or "unknown" if the mode is out of range.
 */
inline const char* OpenLASIR_getModeName(uint8_t mode) {
    if (mode < OPENLASIR_NUM_DEFINED_MODES) {
        return OPENLASIR_MODE_NAMES[mode];
    }
    return "unknown";
}

/**
 * Get the human-readable name for a color number.
 * @return Color name string, or "unknown" if the color is out of range.
 */
inline const char* OpenLASIR_getColorName(uint8_t color) {
    if (color < OPENLASIR_NUM_COLORS) {
        return OPENLASIR_COLOR_NAMES[color];
    }
    return "unknown";
}

/**
 * Get the RGB values for a color number.
 * @param color   Color number (0-7)
 * @param[out] r  Red component (0-255)
 * @param[out] g  Green component (0-255)
 * @param[out] b  Blue component (0-255)
 * @return true if color is valid, false otherwise.
 */
inline bool OpenLASIR_getColorRGB(uint8_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
    if (color >= OPENLASIR_NUM_COLORS) {
        return false;
    }
#ifdef __AVR__
    OpenLASIR_RGB rgb;
    memcpy_P(&rgb, &OPENLASIR_COLOR_RGB[color], sizeof(OpenLASIR_RGB));
    r = rgb.r;
    g = rgb.g;
    b = rgb.b;
#else
    r = OPENLASIR_COLOR_RGB[color].r;
    g = OPENLASIR_COLOR_RGB[color].g;
    b = OPENLASIR_COLOR_RGB[color].b;
#endif
    return true;
}

#endif // OPENLASIR_UTILS_H
