import { Model } from "radiance";
import "./ui";

var canvas = document.getElementById("canvas");
var container = document.getElementById("container");
var model = new Model(container, canvas);

model.append_node("test", 0.7);
model.append_node("oscope", 0.4);
model.append_node("spin", 0.2);
model.append_node("zoomin", 0.3);
model.append_node("rjump", 0.9);
model.append_node("lpf", 0.3);
model.append_node("tunnel", 0.3);
model.append_node("melt", 0.4);
model.append_node("composite", 0.9);

var state = model.state();
state.edges = [
    {fromVertex: 0, toVertex: 8, toInput: 1},
    {fromVertex: 1, toVertex: 2, toInput: 0},
    {fromVertex: 2, toVertex: 3, toInput: 0},
    {fromVertex: 3, toVertex: 4, toInput: 0},
    {fromVertex: 4, toVertex: 5, toInput: 0},
    {fromVertex: 5, toVertex: 6, toInput: 0},
    {fromVertex: 6, toVertex: 7, toInput: 0},
    {fromVertex: 7, toVertex: 8, toInput: 0},
];

console.log(state);
model.set_state(state);

console.log(model.state());

let graph = document.querySelector("#graph");
customElements.whenDefined("radiance-graph").then(() => {
    let graph = document.querySelector("#graph");
    console.log(typeof graph);
    /*
    graph.model = {
        vertices: [
            {uid: 100, nInputs: 1},
            {uid: 200, nInputs: 1},
            {uid: 300, nInputs: 1},
            {uid: 400, nInputs: 1},
            {uid: 500, nInputs: 2},
            {uid: 600, nInputs: 1},
            {uid: 700, nInputs: 1},
        ],
        edges: [
            {fromVertex: 0, toVertex: 1, toInput: 0},
            {fromVertex: 1, toVertex: 2, toInput: 0},
            {fromVertex: 2, toVertex: 4, toInput: 0},
            {fromVertex: 3, toVertex: 4, toInput: 1},
            {fromVertex: 4, toVertex: 5, toInput: 0},
            {fromVertex: 5, toVertex: 6, toInput: 0},
        ],
    };
    */
    graph.model = model.state();
    console.log(graph.model);
    graph.modelChanged();
});
