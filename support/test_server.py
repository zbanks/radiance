#!/usr/bin/env python3

import socket
import json
import struct
import time


class RadianceOutputDevice:
    def __init__(self, port=9001):
        self.port = port

    def listen(self):
        self.serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serversocket.bind(("", self.port))
        self.serversocket.listen(1)

    def accept(self):
        (clientsocket, address) = self.serversocket.accept()
        self.clientsocket = clientsocket
        self.buffer = b""

    def send_packet(self, d):
        length_bytes = struct.pack("<I", len(d))
        #print("sending", len(length_bytes + d), "bytes")
        self.clientsocket.send(length_bytes + d)

    def send_description(self, description):
        self.send_packet(bytes((0,)) + bytes(json.dumps(description), encoding="utf8"))

    def send_get_frame(self, frame_period_ms):
        self.send_packet(bytes((1,)) + struct.pack("<I", frame_period_ms))

    def send_lookup_2d(self, locations):
        locations_flat = [item for sublist in locations for item in sublist]
        self.send_packet(bytes((3,)) + struct.pack("<{}f".format(len(locations_flat)), *locations_flat))

    def send_physical_2d(self, locations):
        locations_flat = [item for sublist in locations for item in sublist]
        self.send_packet(bytes((4,)) + struct.pack("<{}f".format(len(locations_flat)), *locations_flat))

    def send_geometry_2d(self, fn):
        with open(fn, "rb") as f:
            self.send_packet(bytes((5,)) + f.read())

    def recv_packet(self):
        while True:
            result = self.clientsocket.recv(4096)
            if not result:
                break
            self.buffer += result
            if len(self.buffer) > 4:
                (packet_length,) = struct.unpack("<I", self.buffer[0:4])
            if len(self.buffer) - 4 >= packet_length:
                packet = self.buffer[0:packet_length + 4]
                self.buffer = self.buffer[packet_length + 4:]
                return packet

    def parse_frame(self, packet):
        if packet[4] != 2:
            return
        return [(packet[i], packet[i+1], packet[i+2], packet[i+3]) for i in range(5, len(packet), 4)]

d = RadianceOutputDevice()
d.listen()

while True:
    print("Waiting for a connection on port {}".format(d.port))
    d.accept()

    print("Connected! Sending description and framerate")
    d.send_description({"name": "Python test server", "size": [100,100]})
    #d.send_lookup_2d([(0, 0), (0, 1), (1, 0), (1, 1), (0.5, 0.5)])
    pts = [(0, i / 100) for i in range(100)]
    pts += [(i / 100, 0) for i in range(100)]
    pts += [(1, 1 - i / 100) for i in range(100)]
    pts += [(1 - i / 100, 1) for i in range(100)]
    d.send_lookup_2d(pts)

    import math
    def moveToCircle(x, y):
        l = math.hypot(x - 0.5, y - 0.5)
        return (0.5 * (x - 0.5) / l + 0.5, 0.5 * (y - 0.5) / l + 0.5)
    d.send_physical_2d([moveToCircle(x, y) for (x, y) in pts])
    #d.send_geometry_2d("../resources/library/images/logo.png")
    d.send_get_frame(10)

    while True:
        packet = d.recv_packet()
        if not packet:
            break
        #print(d.parse_frame(packet))
