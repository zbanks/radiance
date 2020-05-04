import("./ui")
import("./pkg")

import {Context} from "./pkg/index.js";

let graph = document.querySelector("#graph");
customElements.whenDefined("radiance-graph").then(() => {
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
        {nodeType: "EffectNode", name: "composite", intensity: 0.5},
    ];
    const edges = [
        {fromVertex: 0, toVertex: 2, toInput: 0},
        {fromVertex: 5, toVertex: 1, toInput: 0},
        {fromVertex: 1, toVertex: 9, toInput: 1},
        {fromVertex: 2, toVertex: 3, toInput: 0},
        {fromVertex: 3, toVertex: 4, toInput: 0},
        {fromVertex: 4, toVertex: 5, toInput: 0},
        {fromVertex: 5, toVertex: 6, toInput: 0},
        {fromVertex: 6, toVertex: 7, toInput: 0},
        {fromVertex: 7, toVertex: 8, toInput: 0},
        {fromVertex: 8, toVertex: 9, toInput: 0},
    ];

    const context = new Context(document.querySelector("#canvas"), 512);
    graph.attachContext(context);
    // XXX @zbanks - I added this so I can hack on context from the console
    window["context"] = context;

    const uids = vertices.map(vertex => graph.context.addNode(vertex));
    edges.forEach(edge => {
        graph.context.addEdge(uids[edge.fromVertex], uids[edge.toVertex], edge.toInput);
    });

    graph.context.flush();
    console.log(graph.context.state());

    console.log(graph.model);
    graph.nodesChanged(); // XXX Shouldn't need this
});
