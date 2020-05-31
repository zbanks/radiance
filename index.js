import("./ui")
import("./pkg")

import {Context} from "./pkg/index.js";
var crjson = require("crjson");

Promise.all(["radiance-graph", "radiance-library"].map(el => customElements.whenDefined(el))).then(initialize);

let state;
let editState;

// Quick and dirty UUID algorithm from stack overflow
function uuidv4() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
}

let settingState = false;
function stateChanged(model, json) {
    if (!settingState) {
        console.log("change", json);
        window.context.setFullState(json);
        window.context.flush();
    }
    editState = model;
}

function connect() {
    console.log("Connecting...");

    let room = location.hash.substr(1);
    if (room.length < 1) {
        room = "radiance";
    }

    state = new crjson.StateOverWebSocket(uuidv4());

    // Listen for changes so we can update the textbox
    state.addListener(stateChanged);
    settingState = true;
    state.setState([], window.context.fullState());
    settingState = false;
    //state.triggerListeners(stateChanged);

    // Connect to websocket
    const ws = new WebSocket("wss://goodplace.ddns.net/sandbox/ws/" + room);

    ws.addEventListener("error", event => {
        console.error(event);
        state.detachWebSocket();
    });
    ws.addEventListener("close", () => {
        console.error("connection closed");
        state.detachWebSocket();
    });
    ws.addEventListener("open", () => {
        console.info("connection established");

        // Connect to peer
        state.attachWebsocket(ws);
    });
    window.context.onFullStateChanged(s => {
        settingState = true;
        state.setState(editState, s)
        settingState = false;
    });
}

function hashChanged(event) {
}

function initialize() {

    let graph = document.querySelector("#graph");

    const vertices = [
        {nodeType: "EffectNode", name: "test", intensity: 0.7},
        {nodeType: "EffectNode", name: "resat", intensity: 0.3},
        {nodeType: "EffectNode", name: "oscope", intensity: 0.8},
        {nodeType: "EffectNode", name: "spin", intensity: 0.5},
        {nodeType: "EffectNode", name: "zoomin", intensity: 0.3},
        {nodeType: "EffectNode", name: "rjump", intensity: 0.9},
        {nodeType: "EffectNode", name: "lpf", intensity: 0.3},
        {nodeType: "EffectNode", name: "tunnel", intensity: 0.7},
        {nodeType: "EffectNode", name: "melt", intensity: 0.4},
        {nodeType: "EffectNode", name: "composite", intensity: 0.5, nInputs: 2},
        {nodeType: "MediaNode"},
        {nodeType: "OutputNode"},
    ];
    const edges = [
        {fromVertex: 10, toVertex: 0, toInput: 0},
        {fromVertex: 5, toVertex: 1, toInput: 0},
        {fromVertex: 1, toVertex: 9, toInput: 1},
        {fromVertex: 2, toVertex: 3, toInput: 0},
        {fromVertex: 3, toVertex: 4, toInput: 0},
        {fromVertex: 4, toVertex: 5, toInput: 0},
        {fromVertex: 5, toVertex: 6, toInput: 0},
        {fromVertex: 6, toVertex: 7, toInput: 0},
        {fromVertex: 7, toVertex: 8, toInput: 0},
        {fromVertex: 8, toVertex: 9, toInput: 0},
        {fromVertex: 0, toVertex: 2, toInput: 0},
        {fromVertex: 9, toVertex: 11, toInput: 0},
    ];

    const context = new Context(document.querySelector("#canvas"), 512);
    graph.attachContext(context);
    // XXX @zbanks - I added this so I can hack on context from the console
    window["context"] = context;
    connect();

    const uids = vertices.map(vertex => graph.context.addNode(vertex));
    edges.forEach(edge => {
        graph.context.addEdge(uids[edge.fromVertex], uids[edge.toVertex], edge.toInput);
    });

    graph.context.flush();
    console.log(graph.context.state());

    let library = document.querySelector("#library");
    library.attachGraph(graph);

}

