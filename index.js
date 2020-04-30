import("./ui")
import("./pkg")

let graph = document.querySelector("#graph");
customElements.whenDefined("radiance-graph").then(() => {
    let graph = document.querySelector("#graph");

    var state = {
        vertices: [
            {nodeType: "media"},
            {nodeType: "effect", name: "resat", intensity: 0.3},
            {nodeType: "effect", name: "oscope", intensity: 0.8},
            {nodeType: "effect", name: "spin", intensity: 0.5},
            {nodeType: "effect", name: "zoomin", intensity: 0.3},
            {nodeType: "effect", name: "rjump", intensity: 0.9},
            {nodeType: "effect", name: "lpf", intensity: 0.3},
            {nodeType: "effect", name: "tunnel", intensity: 0.7},
            {nodeType: "effect", name: "melt", intensity: 0.4},
            {nodeType: "effect", name: "composite", intensity: 0.5},
        ],
        edges: [
            {fromVertex: 0, toVertex: 1, toInput: 0},
            {fromVertex: 1, toVertex: 9, toInput: 1},
            {fromVertex: 2, toVertex: 3, toInput: 0},
            {fromVertex: 3, toVertex: 4, toInput: 0},
            {fromVertex: 4, toVertex: 5, toInput: 0},
            {fromVertex: 5, toVertex: 6, toInput: 0},
            {fromVertex: 6, toVertex: 7, toInput: 0},
            {fromVertex: 7, toVertex: 8, toInput: 0},
            {fromVertex: 8, toVertex: 9, toInput: 0},
        ],
    };
    graph.backendModel.set_state(state);
    console.log(graph.backendModel.state());

    graph.model = graph.backendModel.state();
    console.log(graph.model);
    graph.modelChanged();
});
