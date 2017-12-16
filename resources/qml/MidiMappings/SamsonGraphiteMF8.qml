import QtQuick 2.3
import radiance 1.0

QtObject {
    property variant target;

    property variant lpfTimer: Timer {
        interval: 50; running: true; repeat: true
        onTriggered: mc.scrollAccumulator *= 0.9;
    }

    property variant mc: MidiController {
        id: controller
        deviceName: "SAMSON Graphite MF8"

        property real scrollAccumulator;

        onConnectedChanged: {
            console.log(deviceName + " " + (connected ? "connected" : "disconnected"));
        }

        onNoteOn: {
            console.log("on ", channel, note, velocity);
        }

        onNoteOff: {
            console.log("off", channel, note, velocity);
        }

        onControlChange: {
            console.log("cc ", channel, control, value, 123);
            if (control == 60) { // "A" jogwheel
                var N = 4;
                var v = value;
                if (v > 64) {
                    v = 64 - v;
                }
                v /= Math.sqrt(Math.abs(v));

                scrollAccumulator += v;
                if (scrollAccumulator > N / 2) {
                    target.Controls.changeControlRel(0, Controls.Scroll, 1);
                    scrollAccumulator -= N;
                } else if (scrollAccumulator <= -N / 2) {
                    target.Controls.changeControlRel(0, Controls.Scroll, -1);
                    scrollAccumulator += N;
                }
            } else if (control == 49) { // "B" jogwheel
                var v = value << 25 >> 25;
                target.Controls.changeControlRel(0, Controls.PrimaryParameter, v / 127);
            }
        }
    }
}
