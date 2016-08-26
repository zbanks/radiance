n_strips = 40
n_serial_channels = 10
n_udp_channels = 0

def address(i):
    #return hex(i)
    return "0x12"

def vertexlist(i):
    x = i * 2.0 / (n_strips - 1) - 1.0
    return "{} -1.0,{} 1.0".format(x, x)

def color(i):
    return "#66CC66"

def channel(i):
    return int(i / 6)

def length(i):
    return 150

def generate():
    output = open("resources/output_py.ini", "w")

    n_channels = n_serial_channels + n_udp_channels
    output.write("""
[section_sizes]
n_lux_channels={}
n_lux_strips={}
n_lux_spots=0

[lux]
timeout_ms=30

""".format(n_channels, n_strips))

    for i in range(n_serial_channels):
        output.write("""
[lux_channel_{}]
uri=serial:///dev/ttyACM{}
sync=0
""".format(i, i))

    for i in range(n_udp_channels):
        output.write("""
[lux_channel_{}]
uri=udp://127.0.0.1:{}
sync=0
""".format(i + n_serial_channels, i + 1365))

    for i in range(n_strips):
        a = address(i)
        vl = vertexlist(i)
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
""".format(i, a, i, c, ch, l, vl))
    
    output.close() 

if __name__ == "__main__":
    generate()
