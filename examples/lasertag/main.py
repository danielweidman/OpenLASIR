# This file is an example of using the Laser Tag functionality of the OpenLASIR protocol.
# It assumes you have two devices running this code, each with their own IR emitter and receiver, and a button.
# Make sure that the openlasir_utils.py file is in the same directory as this file, along with the ir_rx and ir_tx directories.

from openlasir_utils import encode_laser_tag_fire_packet, decode_packet_laser_tag_fire
from ir_rx.openlasir import OpenLASIR as OpenLASIR_RX
from ir_tx.openlasir import OpenLASIR as OpenLASIR_TX
from machine import Pin
import utime

######################################
# Set these based on your hardware setup:

IR_TX_PIN = 27 # Pin with the IR LED or emitter module
IR_RX_PIN = 12 # Pin with the IR receiver
BUTTON_PIN = 0 # Pin with the button

# Set based on your assigned block ID:
MY_BLOCK_ID = 0 # Block ID, get this see README.md for how to get one assigned (if you will be using around other devices)

# Set different for each device/player:
MY_DEVICE_ID = 0 # Device ID, this should be unique for each device if you want to keep track of hits by specific player or something

# Set to desired color:
MY_COLOR = "Red" # See packet_utils.py for available colors

################
FIRE_RATE_LIMIT_MS = 1000 # milliseconds
DISABLE_TIME_AFTER_TAG = 2000 # milliseconds

RE_ENABLE_AT_TICKS_MS = None
LAST_FIRE_TICKS_MS = 0


def callback_upon_being_tagged(data, addr, ctrl):
    global RE_ENABLE_AT_TICKS_MS, ir_listener
    block_id, device_id, color_name, color_rgb = decode_packet_laser_tag_fire(addr, data)
    print(f'Hit by opponent! Block ID: {block_id}, Device ID: {device_id}, Color Name: {color_name}, Color RGB: {color_rgb}')
    print(f'Should turn to lights {color_name}, ignore hits, and prevent firing for {DISABLE_TIME_AFTER_TAG} milliseconds.')
    ir_listener.close()
    RE_ENABLE_AT_TICKS_MS = utime.ticks_ms() + DISABLE_TIME_AFTER_TAG



ir_listener = OpenLASIR_RX(Pin(IR_RX_PIN, Pin.IN), callback_upon_being_tagged)
ir_transmitter = OpenLASIR_TX(Pin(IR_TX_PIN, Pin.OUT), 38000)
fire_button = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)

print(f'Starting laser tag game. Block ID: {MY_BLOCK_ID}, Device ID: {MY_DEVICE_ID}, Color: {MY_COLOR}')

while True:
    if RE_ENABLE_AT_TICKS_MS is not None and utime.ticks_ms() > RE_ENABLE_AT_TICKS_MS:
        RE_ENABLE_AT_TICKS_MS = None
        ir_listener = OpenLASIR_RX(Pin(IR_RX_PIN, Pin.IN), callback_upon_being_tagged)

    if fire_button.value() == 0:
        # Only allow fire if not disabled due to being tagged, and hasn't fired in last FIRE_RATE_LIMIT_MS milliseconds
        if (utime.ticks_ms() - LAST_FIRE_TICKS_MS) > FIRE_RATE_LIMIT_MS and RE_ENABLE_AT_TICKS_MS is None:
            LAST_FIRE_TICKS_MS = utime.ticks_ms()
            address, command = encode_laser_tag_fire_packet(block_id=MY_BLOCK_ID, device_id=MY_DEVICE_ID, color=MY_COLOR)
            ir_listener.close() # Close the listener to prevent picking up own fire packet
            ir_transmitter.transmit(address, command)
            while ir_transmitter.busy():
                utime.sleep(0.01)
            RE_ENABLE_AT_TICKS_MS = utime.ticks_ms() # Re-enable immediately on next loop iteration
            print(f'Fired laser tag packet: {address}, {command}')
    
    utime.sleep(0.01)
