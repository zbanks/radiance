n_strips = 30
n_serial_channels = 1
n_udp_channels = 0
#serial_channels = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'Z', '0']
serial_channels = ['H', 'B', 'J', 'C', 'D', 'E']

# P| H  B  J
# W|
# R| E  D  C

lightboxes = [
        "0.333 1.0,-0.333 -1.0,-0.666 -0.2",
        "-1.0 1.0,-0.333 -1.0,0.0 -0.2",
        "-0.333 1.0,0.333 -1.0,0.666 -0.2",
        "1.0 1.0,0.333 -1.0,0.0 -0.2",
        ]

lbas = {
    0x1e: 0, 0x1a: 0, 0x16: 0, 0x12: 0,
    0x1d: 1, 0x19: 1, 0x15: 1, 0x11: 1,
    0x1c: 2, 0x18: 2, 0x14: 2, 0x10: 2,
    0x1b: 3, 0x17: 3, 0x13: 3, 0x0f: 3,
}

basket_left = "0.0 Y,-1.0 Y"
basket_right = "0.0 Y,1.0 Y"
basket_left_as = {
    0x01: 0,
    0x03: 1,
    0x05: 2,
    0x07: 3,
    0x09: 4,
    0x0B: 5,
    0x0D: 6,
}
basket_right_as = {
    0x02: 0,
    0x04: 1,
    0x06: 2,
    0x08: 3,
    0x0a: 4,
    0x0C: 5,
    0x0E: 6,
}

def address(i):
    return i + 1

def revvl(vl):
    return ",".join(vl.split(",")[::-1])

def vertexlist(i, a):
    if a in lbas:
        return revvl(lightboxes[lbas[a]])
    if a in basket_left_as:
        return basket_left.replace("Y", "%0.3f" % (1.0 - basket_left_as[a] * 2.0 / 6.))
    if a in basket_right_as:
        return basket_right.replace("Y", "%0.3f" % (1.0 - basket_right_as[a] * 2.0 / 6.))

    x = i * 2.0 / (n_strips - 1) - 1.0
    return "{} -1.0,{} 1.0".format(x, x)

def color(i):
    return "#66CC66"

def channel(i):
    return -1
    #return int(i / 6)

def length(i):
    return 150

def generate():
    output = open("resources/output_py.ini", "w")

    n_channels = n_serial_channels + n_udp_channels + len(serial_channels)
    output.write("""
[section_sizes]
n_lux_channels={}
n_lux_strips={}
n_lux_spots=0

[lux]
timeout_ms=5

""".format(n_channels, n_strips))

    n = 0
    for x in serial_channels:
        output.write("""
[lux_channel_{}]
uri=serial:///dev/lux{}
sync=0
""".format(n, x))
        n += 1

    for i in range(n_serial_channels):
        output.write("""
[lux_channel_{}]
uri=serial:///dev/ttyACM{}
sync=0
""".format(n, i))
        n += 1

    for i in range(n_udp_channels):
        output.write("""
[lux_channel_{}]
uri=udp://127.0.0.1:{}
sync=0
""".format(n, i + 1365))
        n += 1

    for i in range(n_strips):
        a = address(i)
        vl = vertexlist(i, a)
        c = color(i)
        l = length(i)
        ch = channel(i)
        output.write("""
[lux_strip_{}]
address={}
ui_name=Strip{}
ui_color={}
channel={}
length={}
max_energy=0.7
oversample=1
quantize=-1
gamma=1.5
vertexlist={}
""".format(i, hex(a), i, c, ch, l, vl))
    
    output.close() 

if __name__ == "__main__":
    generate()
