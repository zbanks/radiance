import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RowLayout {
    id: autoDJ;

    property var model;
    property var context;
    property var registry;

    property var choices: [
        ["cyan", "dwwave", "fire", "heart", "pink", "purple", "wave", "wwave", "yellow", "vu", "oscope"],
        ["edge", "crt", "delace", "depolar", "droste", "flippy", "fractalzoom", "glitch", "halftone", "kaleidoscope", "lathe", "litebrite", "lorenz", "polygon"],
        ["fly", "cube", "tunnel", "warble"],
        ["foh", "pixelate", "posterize", "melt", "life", "sethue", "greenaway", "scramble", "smoky", "speckle", "stripey", "chansep"]
    ];

    function makeVideoNode(index) {
        var choice = choices[index][Math.floor(Math.random() * choices[index].length)];
        var vn = registry.deserialize(context, 
        {
            "file": "effects/" + choice + ".glsl",
            "type": "EffectNode"
        });
        if (index == 1) vn.frequency = 0.5;
        model.addVideoNode(vn);
        return vn;
    }

    function makePlaceholder() {
        console.log("model is" + model + context + registry);
        var vn = registry.deserialize(context, 
        {
            "type": "PlaceholderNode"
        });
        model.addVideoNode(vn);
        return vn;
    }

    function randomIntensity() {
        return 0.4 + Math.random() * 0.4;
    }

    function randomIndex() {
        return Math.floor(Math.random() * choices.length);
    }

    function timer() {
        return Qt.createQmlObject("import QtQuick 2.0; Timer {}", autoDJ);
    }

    function waitShortTime(cb) {
        var t = timer();
        t.interval = 10;
        t.repeat = false;
        t.triggered.connect(function () {
            if (running.checked) cb();
        });
        t.start();
    }

    function waitRandomTime(cb) {
        var t = timer();
        t.interval = Math.floor(1000 + Math.random() * 5000);
        t.repeat = false;
        t.triggered.connect(function () {
            if (running.checked) cb();
        });
        t.start();
    }

    function startAutoDJ() {
        console.log("Start!");

        var head = makePlaceholder();
        var tail = makePlaceholder();
        var vns = [];

        function getNode(i) {
            if (i < 0) return head;
            if (i >= choices.length) return tail;
            return vns[i];
        }

        for (var i=0; i<choices.length; i++) {
            vns.push(makeVideoNode(i));
        }
        for (var i=0; i<choices.length; i++) {
            vns[i].intensity = randomIntensity();
        }
        for (var i=0; i<=choices.length; i++) {
            model.addEdge(getNode(i - 1), getNode(i), 0);
        }
        model.flush();

        function replace(i) {
            var newVN = makeVideoNode(i);
            model.addEdge(vns[i], newVN, 0);
            model.addEdge(newVN, getNode(i+1), 0);
            model.flush();
            function onFadeDone() {
                model.addEdge(getNode(i - 1), newVN, 0);
                model.removeVideoNode(vns[i]);
                vns[i] = newVN;
                model.flush();
                waitRandomTime(function () {replace(randomIndex())});
            }
            fade(vns[i], newVN, vns[i].intensity, randomIntensity(), 0., onFadeDone);
        }

        function fade(from, to, start, end, fraction, cb) {
            if (fraction > 1) fraction = 1;
            from.intensity = start * (1. - fraction);
            to.intensity = end * fraction;
            if (fraction == 1) {
                cb();
            } else {
                waitShortTime(function() {fade(from, to, start, end, fraction + 0.01, cb)});
            }
        }

        waitRandomTime(function() {replace(randomIndex())});
    }

    function stopAutoDJ() {
        console.log("Stop!");
    }

    CheckBox {
        id: running;
        text: "Auto DJ"

        onCheckedChanged: {
            if (checked) {
                startAutoDJ();
            } else {
                stopAutoDJ();
            }
        }
    }
}
