#!/usr/bin/env python

import select
import serial
import socket

def run_lux_udp(host, port, dev):
    with serial.Serial(dev, baudrate=3000000, xonxoff=False) as ser:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
        sock.bind((host, port))
        last_addr = None
        serial_buffer = ""

        while True:
            inputs, outputs, errors = select.select([sock.fileno(), ser.fileno()], [], [])
            if sock.fileno() in inputs:
                packet, last_addr = sock.recvfrom(1100)
                #print ">", repr(packet)
                if len(packet) == 0: # Ping, respond back
                    sock.sendto("", 0, last_addr)
                else:
                    ser.write(packet)
            if ser.fileno() in inputs:
                serial_buffer += ser.read()
                while "\0" in serial_buffer:
                    packet, null, serial_buffer = serial_buffer.partition("\0")
                    sock.sendto(packet + null, 0, last_addr)
                    #print "<", repr(packet)

if __name__ == "__main__":
    while True:
        try:
            run_lux_udp(host="0.0.0.0", port=1365, dev="/dev/ttyACM0")
        except Exception as e:
            print e
            select.select([], [], [], 5)
