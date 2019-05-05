#!/usr/bin/env python3

import logging
import math
import radiance

# This example shows how to interface custom hardware to radiance.

# This basic idea here is to subclasses radiance.LightOutputNode
# and override callback methods in it
# with custom functionality.

class Example(radiance.LightOutputNode):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # This tells Radiance the name of our device, and how big the sampled canvas should be.
        self.description = {
            "name": "Python Example",
            "size": [100, 100]
        }

        # This would request 5 pixels at the corners and center.
        #self.lookup_2d([(0, 0), (0, 1), (1, 0), (1, 1), (0.5, 0.5)])

        # Instead, lets request 120 pixels around the border.
        N = 30
        self.lookup_2d = [(0, i / N) for i in range(N)]
        self.lookup_2d += [(i / N, 1) for i in range(N)]
        self.lookup_2d += [(1, 1 - i / N) for i in range(N)]
        self.lookup_2d += [(1 - i / N, 0) for i in range(N)]

        # If we stopped here, Radiance would visualize this display using the lookup coordinates
        # and show a square.
        # If the physical display looks different, we tell Radiance about it with the
        # "physical coordinates" command.
        # Lets tell Radiance to visualize the points as a circle instead.

        def moveToCircle(x, y):
            l = math.hypot(x - 0.5, y - 0.5)
            return (0.5 * (x - 0.5) / l + 0.5, 0.5 * (y - 0.5) / l + 0.5)
        self.physical_2d = [moveToCircle(x, y) for (x, y) in self.lookup_2d]

        # We can send radiance a PNG file to be used as a background image for visualization.
        # This logo image is not very useful, but perhaps some line-art of your venue would work well.

        #with open("../resources/library/images/logo.png", "rb") as f:
        #    self.geometry_2d = f.read()

        # Ask for frames from Radiance every 20 ms (50 FPS).
        # On flaky connections, set this to zero.
        # Doing so will request frames one-by-one in a synchronous manner,
        # which will avoid network congestion.
        self.period = 20

    # This gets called every time a frame is received.
    def on_frame(self, frame):
        # This is where you would output to LED strips
        # or whatever hardware you have.

        # For now, lets just print out the RGB values for the first few pixels
        # to the console.
        # Print the RGB values for the first few pixels to the console
        print(frame[0:5])

# Turn on logging so we can see debug messages
logging.basicConfig(level=logging.DEBUG)

# Construct our device
device = Example()

# Start it going
device.serve_forever()
