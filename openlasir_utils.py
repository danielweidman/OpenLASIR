# This file contains the utility functions for the OpenLASIR protocol, a simple protocol for interactive IR devices.

# Author: Dani Weidman (github.com/danielweidman; dani.pink)


# COLORS:
COLOR_NUM_TO_RGB = {
    0:  (0, 255, 255),      # Cyan
    1:  (255, 0, 255),      # Magenta
    2:  (255, 255, 0),      # Yellow
    3:  (0, 255, 0),        # Green
    4:  (255, 0, 0),        # Red
    5:  (0, 0, 255),        # Blue
    6:  (255, 165, 0),      # Orange
    7:  (255, 255, 255),    # White
}

COLOR_NAME_TO_NUM = {
    "Cyan": 0,
    "Magenta": 1,
    "Yellow": 2,
    "Green": 3,
    "Red": 4,
    "Blue": 5,
    "Orange": 6,
    "White": 7,
}
COLOR_NUM_TO_NAME = {v: k for k, v in COLOR_NAME_TO_NUM.items()}

MODE_NUM_TO_NAME = {
    0: "laser_tag_fire",
    1: "user_presence_announcement",
    2: "base_station_presence_announcement",
    3: "user_to_user_handshake_initiation",
    4: "user_to_user_handshake_response",
    5: "user_to_base_station_handshake_initiation",
    6: "user_to_base_station_handshake_response",
    7: "base_station_to_user_handshake_initiation",
    8: "base_station_to_user_handshake_response",
    9: "color_set_temporary",
    10: "color_set_permanent",
    # ...31: potential future use
}
NAME_TO_MODE_NUM = {v: k for k, v in MODE_NUM_TO_NAME.items()}

def encode_general_packet(block_id, device_id, mode, data) -> (int, int):
    # Generates an "address" and "command" value for the IR transmission
    # Address:  8 bits — bits 0-7: block_id
    # Data:    16 bits — bits 0-7: device_id (8 bits), bits 8-12: mode (5 bits), bits 13-15: color/other data (3 bits)

    if isinstance(mode, str):
        mode = NAME_TO_MODE_NUM.get(mode, None)
        if mode is None:
            raise ValueError(f"Invalid mode name: {mode}")
    elif isinstance(mode, int):
        if not 0 <= mode <= 31:
            raise ValueError(f"Invalid mode number: {mode}")
    else:
        raise ValueError(f"Mode must be a string or integer. Mode was: {mode}, type: {type(mode)}")

    if isinstance(data, str):
        data = COLOR_NAME_TO_NUM.get(data, None)
        if data is None:
            raise ValueError(f"Invalid data name: {data}")
    elif isinstance(data, int):
        if not 0 <= data <= 7:
            raise ValueError(f"Invalid data number: {data}")
    else:
        raise ValueError(f"Data must be a string or integer. Data was: {data}, type: {type(data)}")

    if not 0 <= block_id <= 255:
        raise ValueError(f"block_id must be 0-255, got {block_id}")
    if not 0 <= device_id <= 255:
        raise ValueError(f"device_id must be 0-255, got {device_id}")
    if not 0 <= mode <= 31:
        raise ValueError(f"mode must be 0-31 (5 bits), got {mode}")
    if not 0 <= data <= 7:
        raise ValueError(f"data must be 0-7 (3 bits), got {data}")

    address = block_id
    command = (data << 13) | (mode << 8) | device_id

    return address, command

def encode_laser_tag_fire_packet(block_id, device_id, color) -> (int, int):
    if isinstance(color, str):
        color_num = COLOR_NAME_TO_NUM.get(color, None)
        if color_num is None:
            raise ValueError(f"Invalid color: {color}")
    elif isinstance(color, int):
        if not 0 <= color <= 7:
            raise ValueError(f"Invalid color number: {color}")
        color_num = color
    else:
        raise ValueError("Color must be a string or tuple")

    return encode_general_packet(block_id, device_id, "laser_tag_fire", color_num)

def decode_packet_general(address, command):
    # Decodes a packet into block_id, device_id, mode_name, and data
    # Inverse of encode_general_packet

    block_id = address & 0xFF

    device_id = command & 0xFF             # bits 0-7
    mode = (command >> 8) & 0x1F           # bits 8-12
    data = (command >> 13) & 0x07          # bits 13-15

    mode_name = MODE_NUM_TO_NAME.get(mode, f"unknown_mode_{mode}")

    return block_id, device_id, mode_name, data


def decode_packet_laser_tag_fire(address, command):
    # Decodes a laser tag fire packet into block_id, device_id, and color (name and RGB value)
    # Inverse of encode_laser_tag_fire_packet
    # Raises a ValueError if the packet is not a laser tag fire packet

    block_id = address & 0xFF

    device_id = command & 0xFF             # bits 0-7
    mode = (command >> 8) & 0x1F           # bits 8-12
    data = (command >> 13) & 0x07          # bits 13-15

    if mode != 0:
        raise ValueError(f"Not a laser tag fire packet: mode {mode}")
    
    color_name = COLOR_NUM_TO_NAME.get(data, f"unknown_color_{data}")
    color_rgb = COLOR_NUM_TO_RGB.get(data, None)

    return block_id, device_id, color_name, color_rgb
