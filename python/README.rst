Outputting to lights from Radiance
##################################

`Radiance <https://radiance.video>`_ provides a simple way to output to light displays.
A python library is provided for easy implementation on your device or middleware.
All you have to do is instantiate a LightOutputNode and point it to your device.

Example device
**************

To try it out, run it in the background. Then, open Radiance and create a new ``LightOutputNode`` pointing at ``localhost``.
Add a full-screen pattern such as ``purple`` before the ``LightOutputNode`` to fill the display with color.
You should see a circle of lights that correspond to the edges of the frame.

If you restart the server and want to reconnect, simply select the ``LightOutputNode`` and hit ``R``.
