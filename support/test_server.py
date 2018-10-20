#!/usr/bin/env python3

import socket
import json
import struct
import time

PORT = 9001

serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
serversocket.bind(("", PORT))
serversocket.listen(1)

while True:
    print("Waiting for a connection on port {}".format(PORT))
    (clientsocket, address) = serversocket.accept()
    print("Connected! Sending a packet...")

    def send_packet(d):
        length_bytes = struct.pack("<I", len(d))
        clientsocket.send(length_bytes + d)

    def send_description(description):
        send_packet(bytes((0,)) + bytes(json.dumps(description), encoding="utf8"))

    send_description({"name": "Python test server"})

    while True:
        result = clientsocket.recv(4096)
        if not result:
            break
