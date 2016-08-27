n_strips = 30
n_serial_channels = 1
n_udp_channels = 0
#serial_channels = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'Z', '0']
serial_channels = ['H', 'B', 'J', 'C', 'D', 'E']

# P| H  B  J
# W|
# R| E  D  C

def address(i):
    return hex(i+1)

def vertexlist(i):
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
