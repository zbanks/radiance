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
        deviceName: "DJControl Compact"

        property real scrollAccumulator;

        onConnectedChanged: {
            console.log(deviceName + " " + (connected ? "connected" : "disconnected"));
        }

        onControlChange: {
            //console.log(control + " " + value);
            if (control == 48) { // "A" jogwheel
                var N = 12;
                var v = value << 25 >> 25;
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
                target.Controls.changeControlRel(0, Controls.PrimaryParameter, v / 400);
            } else if (control == 59) { // Left Medium
                target.Controls.changeControlAbs(0, Controls.Parameter1, value / 127);
            } else if (control == 63) { // Right Medium
                target.Controls.changeControlAbs(0, Controls.Parameter2, value / 127);
            } else if (control == 60) { // Left Bass
                target.Controls.changeControlAbs(0, Controls.Parameter3, value / 127);
            } else if (control == 64) { // Right Bass
                target.Controls.changeControlAbs(0, Controls.Parameter4, value / 127);
            } else if (control == 54) { // Crossfader
                target.Controls.changeControlAbs(0, Controls.Parameter0, value / 127);
            }
        }

        onNoteOn: {
            if (velocity == 0) return;
            console.log(note + " " + velocity);
            // Connect the left "Loop" buttons to speed settings
            if (note == 25) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 0.)
            } else if (note == 26) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 0.25)
            } else if (note == 27) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 0.5)
            } else if (note == 28) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 1.)
            } else if (note == 9) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 0.)
            } else if (note == 10) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 1.)
            } else if (note == 11) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 2.)
            } else if (note == 12) {
                target.Controls.changeControlAbs(0, Controls.Frequency, 4.)
            }
        }
    }
}
