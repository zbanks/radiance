import QtQuick 2.3
import QtQml.Models 2.2
import radiance 1.0

QtObject {
    property int count;
    property var previous: null;
    property var output: null;

    property var effects;

    function recalculate() {
        var i;
        var prev = previous;
        for(i=0; i<effects.length; i++) {
            //console.log("Effect", i, "is", effects[i]);
            if(effects[i] != null) {
                if(effects[i].previous != prev) {
                    //console.log("Setting", i, "previous to", prev);
                    effects[i].previous = prev;
                }
                prev = effects[i];
            }
        }
        output = prev;
    }

    onPreviousChanged: {
        recalculate();
    }

    function set(index, e) {
        //console.log("Setting", index, "to", e);
        effects[index] = e;
        recalculate();
    }

    onCountChanged: {
        effects = Array(count);
        var i;
        for(i=0; i<effects.length; i++) {
            effects[i] = null;
        }
        recalculate();
    }
} 
