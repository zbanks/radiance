import QtQuick 2.3
import radiance 1.0

QtObject {
    property variant target;

    property variant mc: MidiController {
        id: controller
        deviceName: "Launchkey MK2"

        onConnectedChanged: {
            console.log(deviceName + " " + (connected ? "connected" : "disconnected"));
        }

        onControlChange: {
            if (control == 1) { // "Modulation"
                target.Controls.changeControlAbs(0, Controls.PrimaryParameter, value / 127);
            }
        }

        onPitchBend: {
            target.Controls.changeControlRel(0, Controls.Scroll, bend); // Not super useful but sort of funny
        }
    }
}
