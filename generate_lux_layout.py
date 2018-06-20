#!/usr/bin/env python2.7

import os
import json
import math

output_path = os.path.join(os.path.dirname(__file__), "resources/library/lux", "20_strips.lux.json")
output = {
    "buses": [],
    "devices": [],
}

## Buses

for x in "ABCDEFGHIJYZ":
    output["buses"].append("serial:///dev/lux{}".format(x))

for x in range(10):
    output["buses"].append("serial:///dev/ttyACM{}".format(x))

for x in range(10):
    output["buses"].append("udp:///127.0.0.1:{}".format(x + 1365))

## Devices

N = 42
for i in range(N):

    # Radial
    polygon = [
        [0.5, 0.5],
        [0.5 + 0.5 * math.sin(2. * math.pi * i / float(N)), 0.5 + 0.5 * math.cos(2. * math.pi * i / float(N))],
    ]

    # Stripes
    x = (i % (N/2)) / (N/2. - 1.)
    y = 1 if i >= (N/2) else 0
    polygon = [
        [x, 0.5],
        [x, y]
    ]

    # Shrink towards center by a fraction
    F = 0.9
    polygon = [(0.5 + (x - 0.5) * F, 0.5 + (y - 0.5)) for (x, y) in polygon]

    output["devices"].append({
        "length": 50,
        "lux_id": i + 1,
        "name": "strip_{}".format(i),
        "polygon": polygon,
    })


with open(output_path, "w") as f:
    json.dump(output, f, indent=4)
print "Generated:", output_path
