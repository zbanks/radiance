import socket
import json
import struct
import time
import logging

class LightOutputNode:
    def __init__(self, port=11647, host=""):
        self.host = host
        self.port = port
        self.description = None
        self.lookup_2d = None
        self.physical_2d = None
        self.geometry_2d = None
        self.period = None

    def listen(self):
        self.serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serversocket.bind((self.host, self.port))
        self.serversocket.listen(1)

    def accept(self):
        (clientsocket, address) = self.serversocket.accept()
        self.clientsocket = clientsocket
        self.address = address
        self.buffer = b""

    def send_packet(self, d):
        print(d)
        length_bytes = struct.pack("<I", len(d))
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

    def send_geometry_2d(self, image_data):
        self.send_packet(bytes((5,)) + image_data)

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

    def send_defaults(self):
        if self.description is not None:
            self.send_description(self.description)

        if self.lookup_2d is not None:
            self.send_lookup_2d(self.lookup_2d)

        if self.physical_2d is not None:
            self.send_physical_2d(self.physical_2d)

        if self.geometry_2d is not None:
            self.send_geometry_2d(self.geometry_2d)

    def handle_packet(self, packet):
        frame = self.parse_frame(packet)
        if frame is not None:
            self.on_frame(frame)
            if self.period == 0:
                self.send_get_frame(self.period) # Ask for another packet

    def serve_forever(self):
        logger = logging.getLogger(__name__)
        self.listen()
        self.on_start()
        while True:
            logger.debug("Waiting for a connection on {}:{}".format(self.host, self.port))
            self.accept()
            logger.debug("Connected to {}".format(self.address[0]))
            self.on_connect()
            if self.period is not None:
                self.send_get_frame(self.period)

            while True:
                packet = self.recv_packet()
                if not packet:
                    break
                self.handle_packet(packet)

            logger.debug("Disconnected from {}".format(self.address[0]))
            self.on_disconnect()

    # These functions can be overridden to implement your own server functionality
    def on_start(self):
        pass

    def on_connect(self):
        self.send_defaults()

    def on_frame(self, frame):
        pass

    def on_disconnect(self):
        pass
