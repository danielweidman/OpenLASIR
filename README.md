# Open Lasertag And Stuff InfraRed protocol (OpenLASIR)

OpenLASIR is an communication standard for DEFCON-style electronic badges. If anyone actually adopts it, it might allow badge creators to make devices compatible with one another. It defines packet types for a laser tag game, user and base station presence announcement, handshakes between users and base stations, and color setting commands.

The first-party [Laser\* Tag Badge](https://dani.pink/lasertag) for 2026 (and 2025 badges via a firmware update) will support OpenLASIR (as well as a separate proprietary protocol) for the lasertag-related features. If there is interest from other creators, it might also be updated to support the other functionalities described here.

---

## The IR Layer: Modified NEC Extended Protocol

OpenLASIR is based on the [NEC IR protocol](https://www.sbprojects.net/knowledge/ir/nec.php), but with the address and command roles rearranged. This is done in order to prevent interference with other devices.

### Comparison to Standard NEC / NEC Extended

| Feature | Standard NEC | NEC Extended | **OpenLASIR** |
|---|---|---|---|
| Address bits | 8 | 16 | **8** |
| Address error check | 8-bit inverted copy | None (uses both bytes for address) | **8-bit inverted copy** |
| Command bits | 8 | 8 | **16** |
| Command error check | 8-bit inverted copy | 8-bit inverted copy | **None** |
| Total payload | 8 addr + 8 cmd = 16 useful bits | 16 addr + 8 cmd = 24 useful bits | **8 addr + 16 cmd = 24 useful bits** |
| Carrier frequency | 38 kHz | 38 kHz | **38 kHz** |
| Leading mark | 9 ms | 9 ms | **9 ms** |
| Leading space | 4.5 ms | 4.5 ms | **4.5 ms** |
| Bit mark | 562.5 us | 562.5 us | **562.5 us** |
| "1" space | 1.6875 ms | 1.6875 ms | **1.6875 ms** |
| "0" space | 562.5 us | 562.5 us | **562.5 us** |
| Stop bit | 562.5 us mark | 562.5 us mark | **562.5 us mark** |

**The key difference:** Standard NEC puts the error check on the command; NEC Extended removes the address error check to get 16 address bits. OpenLASIR does the opposite; it keeps the address error check (giving 8 validated address bits) and removes the command error check (giving 16 command bits). This is to prevent OpenLASIR packets from causing issues with NEC devices (except in very rare cases of collisions).

### Bit-by-Bit Breakdown


```
  Bits  0-7:   Block ID              (8 bits)  ← Address low byte
  Bits  8-15:  ~Block ID             (8 bits)  ← Address high byte (inverted, for error check)
  Bits 16-23:  Device ID             (8 bits)  ← Command bits 0-7
  Bits 24-28:  Mode                  (5 bits)  ← Command bits 8-12
  Bits 29-31:  Data (color, etc.)    (3 bits)  ← Command bits 13-15
```
---

The included Python sample code converts the packet data into "address" and "command", for the sake of familiarity with the NEC protocol (and libraries that implement it). "Address" actually is the Block ID, and "command" contains Device ID, Mode, and Data (as shown above).

### Packet Types

This table defines specific packet types/modes that are defined by the standard.

**A given device does NOT need to support all packet types in order to be considered compliant with this standard**.

| Mode # | Name | Description |
|--------|------|-------------|
| 0 | `laser_tag_fire` | Laser tag shot. Data = color. |
| 1 | `user_presence_announcement` | User device saying "I'm here." No response expected. |
| 2 | `base_station_presence_announcement` | Fixed base station saying "I'm here." |
| 3 | `user_to_user_handshake_initiation` | User badge initiates handshake with another user badge. |
| 4 | `user_to_user_handshake_response` | Response to a user-to-user handshake initiation. |
| 5 | `user_to_base_station_handshake_initiation` | User badge initiates handshake with a base station. |
| 6 | `user_to_base_station_handshake_response` | Base station responds to a user-initiated handshake. |
| 7 | `base_station_to_user_handshake_initiation` | Base station initiates handshake with a user badge. |
| 8 | `base_station_to_user_handshake_response` | User badge responds to a base-station-initiated handshake. |
| 9 | `color_set_temporary` | Tell a badge to display a color temporarily. |
| 10 | `color_set_permanent` | Tell a badge to display a color and "remember" it according to some device-specific logic. |
| 11 | `general_interact` | Tell a device to execute a general "interact" action. The specific behavior is defined by the receiver. |
| 12-31 | *(reserved)* | Reserved for future use. Let me know if you have some ideas that would fit with the rest of the packet structure. |


### Colors

The 3-bit data field encodes one of 8 colors (used directly for `laser_tag_fire`, `color_set_temporary`, and `color_set_permanent`; meaning may vary for other modes):

| Value | Color | RGB |
|-------|-------|-----|
| 0 | Cyan | (0, 255, 255) |
| 1 | Magenta | (255, 0, 255) |
| 2 | Yellow | (255, 255, 0) |
| 3 | Green | (0, 255, 0) |
| 4 | Red | (255, 0, 0) |
| 5 | Blue | (0, 0, 255) |
| 6 | Orange | (255, 165, 0) |
| 7 | White | (255, 255, 255) |

---

## Packet Types in Detail

### `laser_tag_fire` (Mode 0)

Indicates firing a laser tag shot.

The **data** field carries the shooter's color, so the tagged badge knows what color hit it (so it can display that color momentarily).

The first-party [Laser\* Tag Badge](https://dani.pink/lasertag) will support this protocol. If you just want to make your device compatible with Laser* Tag Badge, this is the only packet type you need to support.

### Presence Announcements (Modes 1-2)

Simple "I exist" broadcasts. No response is expected.

- **`user_presence_announcement` (Mode 1):** A badge saying hi. Example use case: track which devices you've been near, trigger animations in an environment when a user with badge approaches, etc.
- **`base_station_presence_announcement` (Mode 2):** A fixed transmitter announcing itself. Example use case: badges react when entering a zone.

The three data bits can be used to relay some information about the badge or base station's state.

### Handshake Packets (Modes 3-8)

These come in initiation/response pairs. One device sends an initiation, and any device that receives it (and is in a ready state) replies with the corresponding response. This way both devices have confirmation they "saw" each other. 

| Initiation | Response | Use Case |
|---|---|---|
| `user_to_user_handshake_initiation` (3) | `user_to_user_handshake_response` (4) | Example use case: game where you try to meet and talk to as many other people with compatible badge as possible. |
| `user_to_base_station_handshake_initiation` (5) | `user_to_base_station_handshake_response` (6) | Badge checks in at a base station. Both sides can log it. |
| `base_station_to_user_handshake_initiation` (7) | `base_station_to_user_handshake_response` (8) | Same as above, but initiated by base station. |

The three data bits can be used to relay some information about the badge or base station's state.


### Color Set Packets (Modes 9-10)

Tell another badge to change its LED color.

- **`color_set_temporary` (Mode 9):** Request a device to display a color for a short period.
- **`color_set_permanent` (Mode 10):** Request a device to display a color for a longer period/until otherwise ordered.

### `general_interact` (Mode 11)

A general-purpose "interact with this device" packet. It tells the receiver to do so "something", like how an "interact with object" button in a video game. What *something* means depends entirely on the receiving device. This is intentionally open-ended.

The 3-bit data field is available for device-defined use (e.g. to select between different interaction types on the same device).

Must be human-triggered (e.g. a button press).

---

## Base Stations

Some of the packet types refer to the concept of a "base station". This can be any sort of fixed beacon or device that is not intended to belong to an end user. One potential embodiment is a series of beacons placed in different physical locations that announce their presence so that individual badges can react. Or, through the handshake message types, individual badges could interact.

If multiple projects that use base stations as location markers are set up, we may try to produce a public mapping of base station device ID to name/description, so projects can share base stations.

The concept of a base station in OpenLASIR is *not* related to the concept of a LoRa base station for Laser* Tag Badge DS.
---

## Rules and Rate Limits

In order to be compliant with OpenLASIR, please respect these rules and rate limits. IR transmissions are slow and all (for this protocol at least) done at the same 940nm wavelength, so it is easy for devices to interfere with each other if they are constantly transmitting. 

### Laser Tag Rules

1. **Request a Block/Device ID before using one** (or use one of the "free use" IDs -- see the [allocation table](#allocation-table) below).
2. **Normal transmit power** (single standard IR LED, under 1.5W): Fire no more than **once per second**, and no more than **30 times per minute**. Fire must be triggered by a human in some form (e.g. pushing a button).
3. **High transmit power** (high-power COB LED, multiple LEDs, etc.): Fire no more than **once per minute**, and no more than **15 times per hour**. Fire must still be triggered by a human in some form (e.g. pushing a button).
4. **One-off transmit-only devices are allowed.**
5. **Taggable devices** when tagged:, a device should:
   - Indicate the **color** of the shot that hit it (light up, flash, etc.)
   - **Disable itself for 2 at least seconds** before being able to fire again

### Transmit Rate Limits

| Packet Type | Trigger | Normal Power Limit | High Power Limit |
|---|---|---|---|
| `laser_tag_fire` | Manual | 1/sec, 30/min | 1/min, 15/hr |
| `user_presence_announcement` | Manual or automatic | 10/min | 3/min |
| `base_station_presence_announcement` | Manual or automatic | 10/min | 3/min |
| `color_set_temporary` | Manual or automatic | 10/min | 3/min |
| `base_station_to_user_handshake_initiation` | Manual or automatic | 10/min | 3/min |
| `user_to_user_handshake_initiation` | Manual | 1/sec, 30/min | 3/min |
| `user_to_base_station_handshake_initiation` | Manual | 1/sec, 30/min | 3/min |
| `general_interact` | Manual | 1/sec, 30/min | 3/min |
| `*_response` packets | In response to initiation only | -- | -- |

**Response packets** (`user_to_user_handshake_response`, `user_to_base_station_handshake_response`, `base_station_to_user_handshake_response`) should only be sent in direct response to receiving the corresponding initiation packet.

---

## Block IDs and Device IDs

Every OpenLASIR packet contains a **Block ID** (8 bits) and a **Device ID** (8 bits).

- **Block ID** identifies the project, creator, or badge type. There are also some blocks shared between different projects, and one "free-for-all" block.
- **Device ID** identifies a specific device within that block.

A single Block ID provides 256 Device IDs (0-255). Creators producing multiple badges should request a Block ID and assign each device a unique Device ID within it. Creators a small number of devices may instead be assigned individual Device IDs within a shared block.

### How to Get a Block ID or Device ID

To request a Block ID or Device ID:

1. **Open a GitHub Issue** on this repository with a description of your project and the number of devices you plan to produce, OR
2. **Post in the Laser\* Tag Badge Discord** at [dani.pink/discord](https://dani.pink/discord)

### Allocation Table

Assigned Block IDs and Device IDs are tracked in the table below. "Free Use" blocks may be used without requesting an assignment; note that IDs within these blocks are shared and may collide with other users.

| Block ID | Block Name | Devices |
|----------|-----------|---------|
| 0 | Free-for-all! | *Lazy or mysterious anonymous people can use any device ID in this block without reservation, but devices won't be properly identified and laser tag scoring won't count for leaderboard* |
| 1 | One-Offs | 0: TamaBadge<br>1: irBot |
| 2 | One-Offs | *to be assigned* |
| 3 | One-Offs | *to be assigned* |
| 4 | One-Offs | *to be assigned* |
| 5 | One-Offs | *to be assigned* |
| 6-31 | Laser\* Tag Badge (Official) | *various* |
| 32 | Shared / Flipper Zero | *various* |
| 33 | Shared / Misc Tools | *to be assigned* |
| 34 | Shared / Misc Base Stations | *to be assigned* |
| 35-255 | *(Unassigned)* | -- |

As assignments are made, the table above will be updated to project creators can appropriately attribute individual IR interactions to specific projects/devices.

---

## Potential scenarios

**"I want to make by own one-off high power laser blaster/cannon/grenade!"**
- Request a Device ID via GitHub Issue or Discord, so you will show up on the leaderboard (strongly recommended). Otherise you may use the free-for-all block.
- Implement support for the "laser_tag_fire" packet type.
- Follow the rate limits appropriate to your transmit power level.

**"I am making and distributing my own badge and I want it to be able to play laser tag"**
- Request a Block ID (or multiple for >256 devices). Assign each individual badge a unique Device ID within the block(s).
- You do not need to register each individual Device ID if you are issued your own Block ID.
- Implement support for the "laser_tag_fire" packet type.
- Follow the rate limits appropriate to your transmit power level.

**Other use cases:**
- Describe your project and request a Block ID or Device ID as appropriate.
- Implement support for whichever packet types are relevant.
- Modes 11-31 are reserved for future expansion; suggestions are welcome.

---

## Code Examples

### MicroPython

#### Encoding and Decoding Packets

The `micropython/openlasir_utils.py` module handles packet encoding and decoding, separate from the actual reception and tranmission (it can convert between address+command and packet type/meaning/device identification/data)

**Encoding a laser tag fire packet:**

```python
from openlasir_utils import encode_laser_tag_fire_packet

address, command = encode_laser_tag_fire_packet(block_id=0, device_id=42, color="Red")
print(f"Address: {address} (0x{address:02X})")
print(f"Command: {command} (0x{command:04X})")
# Address: 0 (0x00)
# Command: 32810 (0x802A)
#   Device ID 42 = 0x2A in bits 0-7
#   Mode 0 (laser_tag_fire) in bits 8-12
#   Color 4 (Red) = 0b100 in bits 13-15
```

**Decoding a laser tag fire packet:**

```python
from openlasir_utils import decode_packet_laser_tag_fire

block_id, device_id, color_name, color_rgb = decode_packet_laser_tag_fire(address, command)
print(f"Block ID: {block_id}, Device ID: {device_id}")
print(f"Color: {color_name} {color_rgb}")
# Block ID: 0, Device ID: 42
# Color: Red (255, 0, 0)
```

**Encoding and decoding a general packet (any mode):**

```python
from openlasir_utils import encode_general_packet, decode_packet_general

# Encode a user presence announcement with data=2 (Yellow)
address, command = encode_general_packet(
    block_id=34, device_id=7,
    mode="user_presence_announcement", data="Yellow"
)

# Decode
block_id, device_id, mode_name, data = decode_packet_general(address, command)
print(f"Block: {block_id}, Device: {device_id}, Mode: {mode_name}, Data: {data}")
# Block: 34, Device: 7, Mode: user_presence_announcement, Data: 2
```

**Handshake initiation and response:**

```python
from openlasir_utils import encode_general_packet, decode_packet_general

# Device A sends a handshake initiation
address, command = encode_general_packet(
    block_id=6, device_id=100,
    mode="user_to_user_handshake_initiation", data=0
)

# Device B receives and decodes it
block_id, device_id, mode_name, data = decode_packet_general(address, command)
print(f"Received {mode_name} from block {block_id}, device {device_id}")
# Received user_to_user_handshake_initiation from block 6, device 100

# Device B sends the corresponding response
response_addr, response_cmd = encode_general_packet(
    block_id=34, device_id=5,
    mode="user_to_user_handshake_response", data=0
)
```

**Color lookup utilities:**

```python
from openlasir_utils import COLOR_NUM_TO_NAME, COLOR_NUM_TO_RGB, COLOR_NAME_TO_NUM

COLOR_NAME_TO_NUM  # {'Cyan': 0, 'Magenta': 1, 'Yellow': 2, 'Green': 3, 'Red': 4, 'Blue': 5, 'Orange': 6, 'White': 7}
COLOR_NUM_TO_RGB[4]   # (255, 0, 0)
COLOR_NUM_TO_NAME[6]  # 'Orange'
```

#### Using the IR Transmitter and Receiver (MicroPython)

On a MicroPython board (ESP32, RP2040, Pyboard, etc.), the `ir_tx` and `ir_rx` modules handle physical IR transmission and reception. These are based on [micropython_ir](https://github.com/peterhinch/micropython_ir) by Peter Hinch, with new classes created for OpenLASIR.

**Transmitter setup and usage:**

```python
from machine import Pin
from ir_tx.openlasir import OpenLASIR as OpenLASIR_TX
from openlasir_utils import encode_laser_tag_fire_packet

ir_transmitter = OpenLASIR_TX(Pin(27, Pin.OUT), 38000)

address, command = encode_laser_tag_fire_packet(block_id=0, device_id=1, color="Red")
ir_transmitter.transmit(address, command)

while ir_transmitter.busy():
    pass # Or sleep, or skip this loop if you want to execute other logic while transmitting is in progress via PIO
```

**Receiver setup and usage:**

```python
from machine import Pin
from ir_rx.openlasir import OpenLASIR as OpenLASIR_RX
from openlasir_utils import decode_packet_general, decode_packet_laser_tag_fire

def on_receive(data, addr, ctrl):
    block_id, device_id, mode_name, extra = decode_packet_general(addr, data)
    print(f"Received: block={block_id}, device={device_id}, mode={mode_name}, data={extra}")

    if mode_name == "laser_tag_fire":
        _, _, color_name, color_rgb = decode_packet_laser_tag_fire(addr, data)
        print(f"  Tagged with color: {color_name} {color_rgb}")

# The receiver uses pin interrupts; no polling required
ir_listener = OpenLASIR_RX(Pin(12, Pin.IN), on_receive)
```

**Transmit/receive on the same device:** When both the transmitter and receiver are on the same device, close the receiver before transmitting to prevent it from picking up the outgoing signal:

```python
import utime

ir_listener.close()

ir_transmitter.transmit(address, command)
while ir_transmitter.busy():
    utime.sleep(0.01)

# Re-initialize after transmission
ir_listener = OpenLASIR_RX(Pin(12, Pin.IN), on_receive)
```

#### Full Laser Tag Game Example (MicroPython)

The [`examples/micropython/lasertag/main.py`](examples/micropython/lasertag/main.py) file contains a complete minimal laser tag implementation, including:

- Button-triggered firing with rate limiting (1 shot/second)
- Hit detection with attacker color display
- 2-second disable period after being tagged
- Receiver close/reopen around transmissions

Hardware initialization:

```python
from ir_rx.openlasir import OpenLASIR as OpenLASIR_RX
from ir_tx.openlasir import OpenLASIR as OpenLASIR_TX
from machine import Pin

ir_listener = OpenLASIR_RX(Pin(12, Pin.IN), callback_upon_being_tagged)
ir_transmitter = OpenLASIR_TX(Pin(27, Pin.OUT), 38000)
fire_button = Pin(0, Pin.IN, Pin.PULL_UP)
```

To run the example:
1. Copy `openlasir_utils.py`, `ir_rx/`, and `ir_tx/` to your MicroPython board.
2. Set the pin numbers in `main.py` to match your hardware.
3. Set `MY_BLOCK_ID`, `MY_DEVICE_ID`, and `MY_COLOR`.
4. Run on two or more boards.

### Arduino / C++

Arduino support uses [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) with OpenLASIR protocol support, plus a header-only utility library (`arduino/OpenLASIR_Utils.h`) that mirrors the MicroPython `openlasir_utils.py` module.

NOTE: As of 2-10-2026, the [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote)  library has merged in OpenLASIR support, but a new build with it hasn't been released tothe Arduino Library Manager yet. So you may need to download the latestv ersion from the [repo](https://github.com/Arduino-IRremote/Arduino-IRremote)  and install it manually if the latest release is still v4.5.0 at the time of reading.

#### Setup

1. Install the [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library (see NOTE above).
2. Include `OpenLASIR_Utils.h` from the `arduino/` folder in this repository (copy it into your project or Arduino libraries folder).

#### Encoding and Decoding Packets

The `OpenLASIR_Utils.h` header provides encoding, decoding, and lookup functions.

**Encoding a laser tag fire packet:**

```cpp
#include "OpenLASIR_Utils.h"

uint8_t  address;
uint16_t command;
OpenLASIR_encodeLaserTagFire(0, 42, OPENLASIR_COLOR_RED, address, command);
// address = 0x00 (Block ID 0)
// command = 0x802A
//   Device ID 42 = 0x2A in bits 0-7
//   Mode 0 (laser_tag_fire) in bits 8-12
//   Color 4 (Red) = 0b100 in bits 13-15
```

**Decoding a laser tag fire packet:**

```cpp
OpenLASIR_Packet pkt;
if (OpenLASIR_decodeLaserTagFire(address, command, pkt)) {
    Serial.print("Block ID: ");  Serial.println(pkt.blockId);
    Serial.print("Device ID: "); Serial.println(pkt.deviceId);
    Serial.print("Color: ");     Serial.println(OpenLASIR_getColorName(pkt.data));
    // Block ID: 0, Device ID: 42, Color: Red
}
```

**Encoding and decoding a general packet (any mode):**

```cpp
uint8_t  address;
uint16_t command;

// Encode a user presence announcement with data=2 (Yellow)
OpenLASIR_encodeGeneralPacket(34, 7,
    OPENLASIR_MODE_USER_PRESENCE_ANNOUNCEMENT, OPENLASIR_COLOR_YELLOW,
    address, command);

// Decode
OpenLASIR_Packet pkt = OpenLASIR_decodeGeneralPacket(address, command);
Serial.print("Block: ");   Serial.print(pkt.blockId);
Serial.print(" Device: ");  Serial.print(pkt.deviceId);
Serial.print(" Mode: ");    Serial.print(OpenLASIR_getModeName(pkt.mode));
Serial.print(" Data: ");    Serial.println(pkt.data);
// Block: 34, Device: 7, Mode: user_presence_announcement, Data: 2
```

**Color lookup utilities:**

```cpp
OpenLASIR_getColorName(4);          // "Red"
OpenLASIR_getModeName(0);           // "laser_tag_fire"

uint8_t r, g, b;
OpenLASIR_getColorRGB(4, r, g, b);  // r=255, g=0, b=0
```

#### Using the IR Transmitter and Receiver (Arduino)

IR transmission and reception uses the [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library. Enable the OpenLASIR decoder by defining `DECODE_OPENLASIR` before including the library.

**Sending a laser tag fire packet:**

```cpp
#define DECODE_OPENLASIR
#include <IRremote.hpp>
#include "OpenLASIR_Utils.h"

uint8_t  address;
uint16_t command;
OpenLASIR_encodeLaserTagFire(0, 1, OPENLASIR_COLOR_RED, address, command);

IrSender.sendOpenLASIR(address, command, 0);  // 0 repeats
```

**Receiving and decoding:**

```cpp
#define DECODE_OPENLASIR
#include <IRremote.hpp>
#include "OpenLASIR_Utils.h"

void setup() {
    Serial.begin(115200);
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.protocol == OPENLASIR) {
            OpenLASIR_Packet pkt = OpenLASIR_decodeGeneralPacket(
                IrReceiver.decodedIRData.address,
                IrReceiver.decodedIRData.command);

            Serial.print("Block: ");  Serial.print(pkt.blockId);
            Serial.print(" Device: "); Serial.print(pkt.deviceId);
            Serial.print(" Mode: ");   Serial.println(OpenLASIR_getModeName(pkt.mode));

            if (pkt.mode == OPENLASIR_MODE_LASER_TAG_FIRE) {
                Serial.print("  Tagged with color: ");
                Serial.println(OpenLASIR_getColorName(pkt.data));
            }
        }
        IrReceiver.resume();
    }
}
```

**Transmit/receive on the same device:** Stop the receiver before transmitting to prevent it from picking up the outgoing signal, then restart it:

```cpp
IrReceiver.stop();
IrSender.sendOpenLASIR(address, command, 0);
IrReceiver.start();  // Must use start(), not restartAfterSend()
```

#### Full Laser Tag Game Example (Arduino)

The [`examples/arduino/lasertag/lasertag.ino`](examples/arduino/lasertag/lasertag.ino) file contains a complete minimal laser tag implementation for Arduino, including:

- Button-triggered firing with rate limiting (1 shot/second)
- Hit detection with attacker color display
- 2-second disable period after being tagged
- Receiver stop/start around transmissions
- Platform-aware pin defaults for ESP32, RP2040, and AVR

To run the example:
1. Install the [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library.
2. Open `examples/arduino/lasertag/lasertag.ino` in the Arduino IDE.
3. Set the pin numbers to match your hardware.
4. Set `MY_BLOCK_ID`, `MY_DEVICE_ID`, and `MY_COLOR`.
5. Upload to two or more boards and play!

---

## Repository Structure

```
OpenLASIR/
├── README.md                       # This document
├── micropython/
│   ├── openlasir_utils.py          # Packet encoding/decoding (MicroPython)
│   ├── ir_tx/
│   │   ├── __init__.py             # IR transmitter base class (from micropython_ir)
│   │   └── openlasir.py            # OpenLASIR transmitter implementation
│   └── ir_rx/
│       ├── __init__.py             # IR receiver base class (from micropython_ir)
│       └── openlasir.py            # OpenLASIR receiver/decoder implementation
├── arduino/
│   └── OpenLASIR_Utils.h           # Packet encoding/decoding (Arduino/C++)
└── examples/
    ├── micropython/
    │   └── lasertag/
    │       └── main.py             # Minimal laser tag game (MicroPython)
    └── arduino/
        └── lasertag/
            └── lasertag.ino        # Minimal laser tag game (Arduino)
```

---

## Credits and License

**Protocol design and `openlasir_utils.py`:** Dani Weidman ([github.com/danielweidman](https://github.com/danielweidman), [dani.pink](https://dani.pink))

**MicoPython IR transmitter and receiver modules (`ir_tx/`, `ir_rx/`):** Based on [micropython_ir](https://github.com/peterhinch/micropython_ir) by Peter Hinch, modified to implement the OpenLASIR protocol. The original library supports NEC, Sony, Philips RC-5/RC-6, and other IR protocols for MicroPython.

**Arduino IR transmitter and receiver modules:** Uses [Arduino-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library (which now contains native OpenLASIR support).

---

To get involved, interact on GitHub or join the [Laser\* Tag Badge Discord](https://dani.pink/discord).
