#!/usr/bin/env python3

import serial
import socket
import traceback
import fcntl
import os
import select
import time

PACKET_SIZE = 1100
MAX_BUF_LEN = PACKET_SIZE * 10
DEBUG = False

def run_lux_udp(host, port, dev):
    with serial.Serial(dev, baudrate=3000000, xonxoff=False) as ser:
        fl = fcntl.fcntl(ser.fileno(), fcntl.F_GETFL)
        fcntl.fcntl(ser.fileno(), fcntl.F_SETFL, fl | os.O_NONBLOCK)

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
        sock.bind((host, port))
        sock.setblocking(0)
        last_addr = None
        ser_to_sock = []
        sock_to_ser = b""
        incomplete_buffer = b""

        while True:
            inputs, outputs, errors = select.select([sock.fileno(), ser.fileno()], ([sock.fileno()] if len(ser_to_sock) else []) + ([ser.fileno()] if len(sock_to_ser) else []), [])

            if sock.fileno() in inputs:
                while True:
                    try:
                        packet, last_addr = sock.recvfrom(PACKET_SIZE)
                    except BlockingIOError:
                        break
                    if len(packet) == 0: # Ping, respond back
                        ser_to_sock.append((b"", last_addr))
                    else:
                        if len(sock_to_ser) < MAX_BUF_LEN:
                            sock_to_ser += packet
                    if DEBUG:
                        print("UDP>", packet)

            if ser.fileno() in inputs:
                while True:
                    try:
                        packet = os.read(ser.fileno(), PACKET_SIZE)
                    except BlockingIOError:
                        break
                    if len(packet) == 0:
                        break
                    if DEBUG:
                        print("SER>", packet)
                    incomplete_buffer += packet
                    while b'\0' in incomplete_buffer:
                        pkt, sep, incomplete_buffer = incomplete_buffer.partition(b'\0')
                        ser_to_sock.append((pkt + sep, last_addr))

            if sock.fileno() in outputs:
                while len(ser_to_sock):
                    try:
                        pkt, addr = ser_to_sock[0]
                        sock.sendto(pkt, 0, addr)
                        if DEBUG:
                            print("UDP<", pkt)
                        ser_to_sock = ser_to_sock[1:]
                    except BlockingIOError:
                        break

            if ser.fileno() in outputs:
                while len(sock_to_ser):
                    try:
                        l = os.write(ser.fileno(), sock_to_ser)
                        if DEBUG:
                            print("SER<", sock_to_ser[0:l])
                        sock_to_ser = sock_to_ser[l:]
                    except BlockingIOError:
                        break

if __name__ == "__main__":
    while True:
        try:
            run_lux_udp(host="0.0.0.0", port=1365, dev="/dev/ttyACM0")
        except Exception:
            traceback.print_exc()
            time.sleep(5)
        except KeyboardInterrupt:
            break
