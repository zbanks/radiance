import { Model } from "radiance";

var canvas = document.getElementById("canvas");
var container = document.getElementById("container");
var model = new Model(container, canvas);

var id1 = model.append_node("test", 0.4);
var id2 = model.append_node("spin", 0.5);
var state = model.state();
state.edges.push([id1, id2, 0]);
console.log(state);
model.set_state(state);

console.log(model.state());

