# Outputting to lights from Radiance
Radiance provides a simple way to output to light displays.
If you implement this protocol on your device or middleware,
then all you have to do is instantiate a LightOutputNode and point it to your device.
This document describes the communication protocol between your device and Radiance.

## Example device
The file [test_server.py](support/test_server.py) implements a simple device.
You should use it as a reference for implementing your own devices.

To try it out, run it in the background. Then, open Radiance and create a new `LightOutputNode` pointing at `localhost`.
Add a full-screen pattern such as `purple` before the `LightOutputNode` to fill the display with color.
You should see a circle of lights that correspond to the edges of the frame.

If you restart the server and want to reconnect, simply select the `LightOutputNode` and hit `R`.

## Nuts and bolts
* Your device should bind a TCP port. Radiance will connect.
* Radiance defaults to port 11647 if no port is specified.
* All messages are asynchronous and may be sent at any time.
* Unknown messages should be ignored.
* All values are little-endian unless otherwise noted.
* If an unrecoverable error occurs, the connection should be terminated.
* No state is stored between connections, so you will have to re-issue setup messages after a disconnect.

## Message format
<table><tr>
<td>Length (4 bytes)</td>
<td>Command (1 byte)</td>
<td>Data (length - 1 bytes)</td>
</tr></table>

## Messages

**Important note:** Only commands 0-5 are implemented right now. Commands 6-9 will be implemented in the future.

<table>
<tr>
<th>byte</th>
<th>Command</th>
<th>Sender</th>
<th>Data</th>
<th>Description</th>
<th>Default if not sent</th>
</tr>
<tr>
<td>0</td>
<td>Description</td>
<td>Device</td>
<td>JSON</td>
<td>Device returns a high level description of itself in JSON format. See below</td>
<td>See below</td>
</tr>
<tr>
<td>1</td>
<td>Get frame</td>
<td>Device</td>
<td>uint32</td>
<td>Ask Radiance for a frame. Data is a uint32 that sets the frame period in milliseconds. Request 0 ms for a single frame or to stop a previous request.</td>
<td>No frames will be sent</td>
</tr>
<tr>
<td>2</td>
<td>Frame</td>
<td>Radiance</td>
<td>RGBA data (32 bits total)</td>
<td>Radiance returns a pixel color in RGBA format for every location requested.</td>
<td>N/A</td>
</tr>
<tr>
<td>3</td>
<td>Lookup coordinates 2D</td>
<td>Device</td>
<td>Array of {float u, float v}</td>
<td>A list of pixel coordinates in uv space to lookup and return in "frame" messages. Must be sent before "get frame"</td>
<td>No pixels will be returned in a frame unless a "lookup coordinates 2D" or "lookup coordinates 3D" command is sent.</td>
</tr>
<tr>
<td>4</td>
<td>Physical coordinates 2D</td>
<td>Device</td>
<td>Array of {float u, float v}</td>
<td>A list of physical coordinates (in UV space.) Used purely for visualization in radiance.</td>
<td>Radiance will visualize pixel locations using the lookup coordinates.</td>
</tr>
<tr>
<td>5</td>
<td>Geometry 2D</td>
<td>Device</td>
<td>PNG image data</td>
<td>A PNG image that will be used as a background for visualization in radiance.</td>
<td>Radiance will visualize pixel locations against a transparent background.</td>
</tr>
<tr>
<td>6</td>
<td>Lookup coordinates 3D</td>
<td>Device</td>
<td>Array of {float t, float u, float v}</td>
<td>A list of pixel coordinates in tuv space to lookup and return in “frame” messages. Must be sent before “get frame”</td>
<td>No pixels will be returned in a frame unless a “lookup coordinates 2D” or “lookup coordinates 3D” command is sent.</td>
</tr>
<tr>
<td>7</td>
<td>Physical coordinates 3D</td>
<td>Device</td>
<td>Array of {float x, float y, float z}</td>
<td>A list of physical coordinates (in the STL’s space.) Used purely for visualization in radiance. If not sent, radiance will visualize pixel locations using the 3D lookup coordinates.</td>
<td>Radiance will visualize pixel locations using the lookup coordinates.</td>
</tr>
<tr>
<td>8</td>
<td>Geometry 3D</td>
<td>Device</td>
<td>STL data</td>
<td>A STL file that will be used as a background for visualization in radiance.</td>
<td>Radiance will visualize pixel locations against a transparent background.</td>
</tr>
<tr>
<td>9</td>
<td>tuv map</td>
<td>Device</td>
<td>GLSL shader program</td>
<td>A file containing one or more shader programs that convert tuv coordinates to uv coordinates for sampling.</td>
<td>Radiance will use a set of presets.</td>
</tr>
</table>

### Description keys

<table>
<tr>
<th>Key name</th>
<th>Value</th>
<th>Description</th>
<th>Default if not set</th>
<th>Notes</th>
</tr>
<tr>
<td>name</td>
<td>string</td>
<td>What to call this device</td>
<td>unnamed</td>
<td></td>
</tr>
<tr>
<td>size</td>
<td>[width, height] or single number for width & height</td>
<td>How big the canvas should be on which the points are sampled</td>
<td>300x300</td>
<td>all UV values are [0, 1] despite aspect ratio</td>
</tr>
</table>

### Typical conversation

A minimal example:
* **Radiance**: connects to your device on port 11647
* **Device**: sends `lookup coordinates 2D` command with 5 pixel coordinates in UV space
* **Device**: sends `get frame` command with `0` ms period
* **Radiance**: sends `frame` command with 5 pixel colors
* **Device**: sends `get frame` command with `0` ms period
* **Radiance**: sends `frame` command with 5 pixel colors
* etc.

A more fully featured example:
* **Radiance**: connects to your device on port 11647
* **Device**: sends `lookup coordinates 2D` command with pixel coordinates in UV space indicating how they should be sampled
* **Device**: sends `physical coordinates 2D` command with pixel coordinates in UV space indicating how they should be visualized in Radiance
* **Device**: sends `geometry 2D` command with a PNG image to serve as a background in Radiance
* **Device**: sends `get frame` command with `10` ms period
* **Radiance**: sends `frame` command with pixel colors
* 10 ms later, **Radiance**: sends `frame` command with pixel colors
* 10 ms later, **Radiance**: sends `frame` command with pixel colors
* etc.

Note that you may update the lookup coordinates, the physical coordinates, and the background image at any time.
This is useful for outputting to interactive displays.
