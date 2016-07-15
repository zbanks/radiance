#!/usr/bin/env python

import socket
import select

def run_lux_udp_dummy(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    sock.bind((host, port))
    last_addr = None

    while True:
        packet, last_addr = sock.recvfrom(1100)
        if len(packet) == 0: # Ping, respond back
            sock.sendto("", 0, last_addr)
        else:
            pass

if __name__ == "__main__":
    run_lux_udp_dummy(host="0.0.0.0", port=1365)
