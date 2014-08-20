#!/usr/bin/env python
from __future__ import print_function
from binascii import hexlify
import struct
import sys
import time
import serial
import threading
import Queue

def bidict(d):
    new_d = dict()
    for (k,v) in d.items():
        new_d[v] = k
        new_d[k] = v
    return new_d

AXIS_MAP = {
    0: "Left-X",
    1: "Left-Y",
    2: "Right-X",
    3: "Right-Y",
    #7: "Left",
    #8: "Up",
    #9: "Right",
    #10: "Down",
    12: "L2",
    13: "R2",
    14: "L1",
    15: "R1",
    #16: "Triangle",
    #17: "Circle",
    #18: "Cross",
    #19: "Square",
}
OFFSETS = {
    12: 32767,
    13: 32767,
    14: 32767,
    15: 32767,
}
current_axes = dict((key, 0) for key in AXIS_MAP)
AXIS_MAP = bidict(AXIS_MAP)

BUTTON_MAP = {
    0: "Select",
    1: "LeftStick",
    2: "RightStick",
    3: "Start",
    4: "Up",
    5: "Right",
    6: "Down",
    7: "Up",
    8: "L2",
    9: "R2",
    10: "L1",
    11: "R1",
    16: "PS",
    12: "Triangle",
    13: "Circle",
    14: "Cross",
    15: "Square",
}
current_buttons = dict((key, 0) for key in BUTTON_MAP)
BUTTON_MAP = bidict(BUTTON_MAP)

DEBUG=False

EVENTS = {
    1: "Button",
    2: "Stick",
    128: "Init",
    129: "InitButton",
    130: "InitStick"
}
EVENTS = bidict(EVENTS)

def get_speed():
    speed = current_axes[AXIS_MAP["Right-Y"]]
    speed = speed * 255
    speed = speed / 32768
    if speed > 255:
        speed = 255
    elif speed < -255:
        speed = -255
    return speed

def process_event(data):
    (timeval, value, event, number) = struct.unpack("<IhBB", data)
    if event in (EVENTS["Stick"], EVENTS["InitStick"]):
        if number in AXIS_MAP:
            if DEBUG:
                print("Time: %u, Value: %u, Type: %s, Number %u" % (timeval, value, EVENTS[event], number))
            if number in OFFSETS:
                value = value + OFFSETS[number]
            current_axes[number] = value
    elif event in (EVENTS["Button"], EVENTS["InitButton"]):
        if number in BUTTON_MAP:
            if DEBUG:
                print("Time: %u, Value: %u, Type: %s, Number %u" % (timeval, value, EVENTS[event], number))
            current_buttons[number] = value

def print_status():
    print("==================")
    for axis in current_axes:
        print("%s = %s" % (AXIS_MAP[axis], current_axes[axis]))
    for axis in current_buttons:
        print("%s = %s" % (BUTTON_MAP[axis], current_buttons[axis]))

def serialthread(f, q):
    while True:
        speed = q.get()
        msg = "%d\n" % speed
        print("msg=%r" % msg)
        if f:
            f.write(msg.encode("ascii"))

def main(*args):
    global DEBUG
    s = None
    if args[0] == "--debug":
        DEBUG=True
        args = args[1:]
    elif args[0].startswith("--serial="):
        OUTPUT=True
        dev = args[0].split("=")[1]
        s = serial.Serial(dev, baudrate=9600)
        args = args[1:]
    dev = args[0]
    print("Device:", dev)
    last = 0
    data = ""
    q = Queue.Queue(maxsize=2)
    thread = threading.Thread(target=serialthread, args=(s, q))
    thread.daemon = True
    thread.start()
    with open(dev, "rb") as f:
        while True:
            data = data + f.read(8)
            print("read %u" % len(data))
            if len(data) >= 8:
                process_event(data[:8])
                data = data[8:]
                speed = get_speed()
                if speed != last:
                    try:
                        q.put(speed)
                        last = speed
                    except Queue.Full:
                        print("Q full!")

if __name__ == '__main__':
    main(*sys.argv[1:])
