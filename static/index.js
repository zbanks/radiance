import { Model } from "radiance";
import "./ui";

var canvas = document.querySelector("#canvas");
var model = new Model(canvas, 512);

// TODO: The model doesn't yet support adding nodes through `model.set_state(...)`
model.append_node("test", 0.7);
model.append_node("oscope", 0.8);
model.append_node("spin", 0.5);
model.append_node("zoomin", 0.3);
model.append_node("rjump", 0.9);
model.append_node("lpf", 0.3);
model.append_node("tunnel", 0.7);
model.append_node("melt", 0.4);
var outputUid = model.append_node("composite", 0.1);

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
model.set_state(state);
console.log(model.state());

var outputDiv = document.querySelector("#output");
let render = function(t) {
    model.render(t);
    model.paint_node(Math.floor(((t / 1000) % 8)) + 101, outputDiv);
    window.requestAnimationFrame(render);
};
window.requestAnimationFrame(render);


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
