import QtQuick 2.3
import radiance 1.0

QtObject {
    property variant target;

    property variant mc: MidiController {
        id: controller
        deviceName: "DJ Control Air"

        property real scrollAccumulator;

        onConnectedChanged: {
            console.log(deviceName + " " + (connected ? "connected" : "disconnected"));
        }

        onControlChange: {
            //console.log(control + " " + value);
            if (control == 48) { // "A" jogwheel
                var N = 4;
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
                target.Controls.changeControlRel(0, Controls.PrimaryParameter, v / 127);
            }
        }
    }
}
