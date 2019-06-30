import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.0
import radiance 1.0
import "."

RowLayout {
    id: autoDJ;

    property var context;
    property var t;

    // Setup for working with full 2D video art (e.g. projector, livestream)
    property var choices: [
        // Base: something that has color or form
        ["cyan", "dwwave", "fire", "heart", "pink", "purple", "wave", "wwave", "yellow", "vu", "oscope", "count", "random", "rekt", "gay"],

        // Intensitfy: add distortions and high-pass
        // Always set to 0.5 frequency
        ["edge", "pixelate", "delace", "depolar", "droste", "droste", "flippy", "fractalzoom", "glitch", "halftone", "kaleidoscope", "lathe", "litebrite", "lorenz", "polygon", "uvmapself", "strange", "cga1", "crack", "outline", "projector"],

        // Colorize
        ["sethue", "greenaway", "smoky", "chansep", "threedee", "yuvposter", "yuvrot", "yuvsat", "blowout", "cedge", "gamma", "rainblur", "resat", "rjump", "rsheen", "saturate", "yuvblur", "yuvchansep"],

        // Motion
        // Always set to 0.5 frequency
        ["fly", "tunnel", "warble", "flowing", "flow", "fractalzoom", "life", "scramble", "tesselate", "bespeckle", "bespecklep", "coin", "shake", "solitare", "id", "id", "id"],

        // Low-pass
        ["lpf", "diodelpf", "crt", "posterize", "melt", "life", "smoky", "speckle", "stripey", "rolling", "still", "swipe"],
        ["rekt"],
        ["bespeckle"],
        ["fly"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
        ["rekt"],
    ];
    /*
    // Setup for working with light strips and pixels
    // May need to follow up with an lpf, smooth, no, etc. 
    property var choices: [
        // Base: something with *color*, can be static
        ["cyan", "fire", "fireball", "heart", "purple", "wave", "yellow", "vu", "test", "gay", "rekt", "rekt", "rekt", "smoke", "stripes", "yuvmapid", "uvmapid"],

        // Simple motion
        // Always set to 0.5 frequency
        ["warble", "flowing", "flow", "fractalzoom", "flippy", "scramble", "bespeckle", "bespecklep", "shake", "id"],

        // Colorize
        ["sethue", "greenaway", "yuvposter", "yuvrot", "yuvsat", "blowout", "cedge", "gamma", "rainblur", "resat", "rjump", "rsheen", "saturate", "yuvblur", "yuvchansep"],

        // Simple motion
        // Always set to 0.5 frequency
        ["warble", "flowing", "flow", "fractalzoom", "flippy", "scramble", "bespeckle", "bespecklep", "shake", "id"],

        // Colorize
        ["sethue", "greenaway", "yuvposter", "yuvrot", "yuvsat", "blowout", "cedge", "gamma", "rainblur", "resat", "rjump", "rsheen", "saturate", "yuvblur", "yuvchansep"],

        // Low-pass
        ["lpf", "diodelpf", "posterize", "melt", "speckle", "stripey", "rolling", "still", "swipe", "kmeans"]
    ];
    */

    function makeVideoNode(index) {
        var choice = choices[index][Math.floor(Math.random() * choices[index].length)];
        var vn = registry.deserialize(context, 
        {
            "file": "effects/" + choice + ".glsl",
            "type": "EffectNode"
        });
        if (index == 1 || index == 3) {
            if (Math.random() < 0.1) vn.frequency = 1.0;
            else if (Math.random() < 0.7) vn.frequency = 0.5;
            else vn.frequency = 0.25;
        }
        vn.setFrozenOutput(true);
        vn.setFrozenInput(true);
        vn.setFrozenParameters(true);
        model.addVideoNode(vn);
        return vn;
    }

    function getOrMakePlaceholders(prefix) {
        var head = null;
        var tail = null;
        var headRole = prefix + " Head";
        var tailRole = prefix + " Tail";
        for (var i = 0; i < model.vertices.length; i++) {
            if (model.vertices[i].role == headRole)
                head = model.vertices[i];
            else if (model.vertices[i].role == tailRole)
                tail = model.vertices[i];
        }
        
        if (!head || !tail) {
            if (head) {
                model.removeVideoNode(head);
            }
            if (tail) {
                model.removeVideoNode(tail);
            }
            head = makePlaceholder(headRole);
            tail = makePlaceholder(tailRole);
        }
        head.setFrozenOutput(true);
        tail.setFrozenInput(true);

        var headDescendants = model.qmlDescendants(head);
        if (headDescendants.includes(tail)) {
            var tailAncestors = model.qmlAncestors(tail);
            for (var i = 0; i < headDescendants.length; i++) {
                var node = headDescendants[i];
                if (node != tail && tailAncestors.includes(node)) {
                    model.removeVideoNode(node);
                }
            }
        } else {
            for (var i = 0; i < model.edges.length; i++) {
                var edge = model.edges[i];
                if (edge.fromVertex == head) {
                    model.removeEdge(edge.fromVertex, edge.toVertex, edge.toInput);
                } else if (edge.toVertex == tail) {
                    model.removeEdge(edge.fromVertex, edge.toVertex, edge.toInput);
                }
            }
        }

        model.flush();
        return [head, tail];
    }

    function makePlaceholder(role) {
        var vn = registry.deserialize(context, 
        {
            "type": "PlaceholderNode",
            "role": role,
        });
        model.addVideoNode(vn);
        return vn;
    }

    function randomIntensity() {
        return 0.4 + Math.random() * 0.5;
    }

    function randomIndex() {
        // Usually return 0
        if (Math.random() < 0.4) return 0;
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
        this.t = timer();
        t.interval = 100; // Math.floor(1000 + Math.random() * 5000);
        t.repeat = false;
        t.triggered.connect(function () {
            if (running.checked) cb();
        });
        t.start();
    }

    function startAutoDJ() {
        // https://tc39.github.io/ecma262/#sec-array.prototype.includes
        if (!Array.prototype.includes) {
        Object.defineProperty(Array.prototype, 'includes', { value: function(valueToFind, fromIndex) {
            if (this == null) { throw new TypeError('"this" is null or not defined'); }
            var o = Object(this);
            var len = o.length >>> 0;
            if (len === 0) { return false; }
            var n = fromIndex | 0;
            var k = Math.max(n >= 0 ? n : len - Math.abs(n), 0);
            function sameValueZero(x, y) { return x === y || (typeof x === 'number' && typeof y === 'number' && isNaN(x) && isNaN(y)); }
            while (k < len) { if (sameValueZero(o[k], valueToFind)) { return true; } k++; }
            return false;
            } });
        }

        this.t = null;
        console.log("Start!");
        var placeholders = getOrMakePlaceholders('AutoDJ');
        var head = placeholders[0];
        var tail = placeholders[1];
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
                cb();
        }

        waitRandomTime(function() {
          for (var i = 0; i < choices.length; i++) {
            replace(i);
          }
          });
    }

    function stopAutoDJ() {
        this.t.stop();
        var nodes = model.vertices;
        for (var i = 0; i < nodes.length; i++) {
            nodes[i].setFrozenOutput(false);
            nodes[i].setFrozenInput(false);
            nodes[i].setFrozenParameters(false);
        }
        console.log("Stop!", this.t);
    }

    ColumnLayout {
        anchors.fill: parent
        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

        CheckBox {
            id: running;
            text: "Enable Auto DJ"

            onCheckedChanged: {
                if (checked) {
                    startAutoDJ();
                } else {
                    stopAutoDJ();
                }
            }
        }
    }
}
