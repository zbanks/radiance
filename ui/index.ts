"use strict";

import {Model as BackendModel} from "../pkg/index.js";

class Flickable extends HTMLElement {
    outer: HTMLElement; // Outer div (for clipping and mouse events)
    inner: HTMLElement; // Inner div (the one that is transformed)
    offsetX: number; // Current X offset of the surface
    offsetY: number; // Current Y offset of the surface
    dragging: boolean; // Whether or not the surface is being dragged
    mouseDrag: boolean; // Whether or not the surface is being dragged due to the mouse
    touchDrag: boolean; // Whether or not the surface is being dragged due to a touch
    startDragX: number; // X coordinate of the start of the drag
    startDragY: number; // Y coordinate of the start of the drag
    startDragOffsetX: number; // The value of offsetX when the drag starts
    startDragOffsetY: number; // The value of offsetY when the drag starts
    touchId; // The identifier of the touchevent causing the drag
    resizeObserver: ResizeObserver;

    constructor() {
        super();
    }

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: inline-block;
            }
            #outer {
                display: flex;
                width: 100%;
                height: 100%;
                justify-content:center;
                align-items:center;
                overflow: hidden;
            }
            #inner {
                border: 30px solid transparent;
            }
            </style> 
            <div id='outer'>
                <div id='inner'>
                    <slot></slot>
                </div>
            </div>
        `;
        this.inner = shadow.querySelector("#inner");
        this.outer = shadow.querySelector("#outer");
        this.outer.addEventListener('mousedown', this.mouseDown.bind(this));
        document.addEventListener('mouseup', this.mouseUp.bind(this));
        document.addEventListener('mousemove', this.mouseMove.bind(this));
        this.outer.addEventListener('touchstart', this.touchStart.bind(this), { passive: false });
        document.addEventListener('touchend', this.touchEnd.bind(this));
        document.addEventListener('touchmove', this.touchMove.bind(this));

        this.resizeObserver = new ResizeObserver(entries => {
            this.applyTransformation();
        });
        this.resizeObserver.observe(this.outer);
        this.resizeObserver.observe(this.inner);

        this.offsetX = 0;
        this.offsetY = 0;
    }

    applyTransformation() {
        let rangeX = 0.5 * Math.max(0, this.inner.offsetWidth - this.outer.clientWidth);
        let rangeY = 0.5 * Math.max(0, this.inner.offsetHeight - this.outer.clientHeight);
        if (Math.abs(this.offsetX) > rangeX) {
            this.offsetX = rangeX * Math.sign(this.offsetX);
        }
        if (Math.abs(this.offsetY) > rangeY) {
            this.offsetY = rangeY * Math.sign(this.offsetY);
        }
        this.inner.style.transform = `translate(${this.offsetX}px, ${this.offsetY}px)`;
    }

    touchStart(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (touch.target != this.outer && touch.target != this.inner) {
                // Only accept clicks onto this surface, not its child (inner)
                continue;
            }
            if (!this.dragging) {
                this.touchDrag = true;
                this.touchId = touch.identifier
                this.startDrag(touch.pageX, touch.pageY);
                event.preventDefault();
            }
        }
    }

    mouseDown(event: MouseEvent) {
        if (event.target != this.outer && event.target != this.inner) {
            // Only accept clicks onto this surface, not its child (inner)
            return;
        }
        if (!this.dragging && (event.buttons & 1)) {
            this.mouseDrag = true;
            this.startDrag(event.pageX, event.pageY);
            event.preventDefault();
        }
    }

    touchEnd(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (this.dragging && this.touchDrag && this.touchId == touch.identifier) {
                this.touchDrag = false;
                this.endDrag();
            }
        }
    }

    mouseUp(event: MouseEvent) {
        if (this.dragging && this.mouseDrag && !(event.buttons & 1)) {
            this.mouseDrag = false;
            this.endDrag();
            event.preventDefault();
        }
    }

    touchMove(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (this.dragging && this.touchDrag && this.touchId == touch.identifier) {
                this.drag(touch.pageX, touch.pageY);
            }
        }
    }

    mouseMove(event: MouseEvent) {
        if (this.dragging && this.mouseDrag) {
            this.drag(event.pageX, event.pageY);
            event.preventDefault();
        }
    }

    startDrag(ptX: number, ptY: number) {
        this.dragging = true;
        this.startDragX = ptX;
        this.startDragY = ptY;
    }

    endDrag() {
        this.dragging = false;
    }

    drag(ptX: number, ptY: number) {
        this.offsetX = this.startDragOffsetX + ptX - this.startDragX;
        this.offsetY = this.startDragOffsetY + ptY - this.startDragY;
        this.applyTransformation();
    }
}

type UID = number;

class VideoNodeTile extends HTMLElement {
    nInputs: number;
    inputHeights: number[];
    uid: UID;
    x: number;
    y: number;
    graph: Graph;
    preview: VideoNodePreview;
    inner: HTMLElement;

    // Properties relating to drag
    offsetX: number; // Current X offset of the surface
    offsetY: number; // Current Y offset of the surface
    dragging: boolean; // Whether or not the surface is being dragged
    mouseDrag: boolean; // Whether or not the surface is being dragged due to the mouse
    touchDrag: boolean; // Whether or not the surface is being dragged due to a touch
    startDragX: number; // X coordinate of the start of the drag
    startDragY: number; // Y coordinate of the start of the drag
    touchId; // The identifier of the touchevent causing the drag

    constructor() {
        super();
        this.nInputs = 0;
        this.inputHeights = [];
        this.uid = null;

        this.offsetX = 0;
        this.offsetY = 0;
        this.dragging = false;
    }

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: block;
                padding: 15px 0px;
                position: absolute;
                top: 0px;
                left: 0px;
                box-sizing: border-box;
                z-index: 0;
            }

            #inner {
                box-sizing: border-box;
                width: 100%;
                height: 100%;
                background-color: black;
                text-align: center;
                color: white;
                padding: 5px;
                display: flex;
                flex-direction: column;
                justify-content: center;
                align-items: center;
            }

            #outline {
                pointer-events: none;
                position: absolute;
                left: -1px;
                right: -1px;
                top: 14px;
                bottom: 14px;
                border: 2px solid gray;
            }
            </style>
            <div id="inner">
                <slot></slot>
                <div id="outline"></div>
            </div>
        `;

        this.inner = shadow.querySelector("#inner");

        this.inner.addEventListener('mousedown', this.mouseDown.bind(this));
        document.addEventListener('mouseup', this.mouseUp.bind(this));
        document.addEventListener('mousemove', this.mouseMove.bind(this));
        this.inner.addEventListener('touchstart', this.touchStart.bind(this), { passive: false });
        document.addEventListener('touchend', this.touchEnd.bind(this));
        document.addEventListener('touchmove', this.touchMove.bind(this));
    }

    width() {
        // Tile width
        // Override this method to specify width, e.g. as a function of height
        return 110;
    }

    minInputHeight(input: number) {
        // Minimum height of the given input.
        // An input's height may be expanded if upstream blocks are tall.
        // Override this method to specify minimum input heights for your node type.
        // This method will be called at least once, even if there are no inputs, to get the
        // height of the node. So implement it even if your node has zero inputs.
        return 200;
    }

    height() {
        // Total height = sum of inputHeights
        // Do not override this method.
        return this.inputHeights.reduce((a, b) => a + b, 0);
    }

    updateSize() {
        this.style.height = `${this.height()}px`;
        this.style.width = `${this.width()}px`;
    }

    render() {
        if (this.preview) {
            this.preview.render(this);
        }
    }

    updateFromModel(data: ModelVertex) {
        if (data.nInputs != this.nInputs) {
            this.nInputs = data.nInputs;
            this.graph.requestRelayout();
        }
    }

    touchStart(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (touch.target != this.inner && touch.target != this.inner) {
                // Only accept clicks onto this surface, not its child (inner)
                continue;
            }
            if (!this.dragging) {
                this.touchDrag = true;
                this.touchId = touch.identifier
                this.startDrag(touch.pageX, touch.pageY);
                event.preventDefault();
            }
        }
    }

    mouseDown(event: MouseEvent) {
        if (event.target != this.inner && event.target != this.inner) {
            // Only accept clicks onto this surface, not its child (inner)
            return;
        }
        if (!this.dragging && (event.buttons & 1)) {
            this.mouseDrag = true;
            this.startDrag(event.pageX, event.pageY);
            event.preventDefault();
        }
    }

    touchEnd(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (this.dragging && this.touchDrag && this.touchId == touch.identifier) {
                this.touchDrag = false;
                this.endDrag();
            }
        }
    }

    mouseUp(event: MouseEvent) {
        if (this.dragging && this.mouseDrag && !(event.buttons & 1)) {
            this.mouseDrag = false;
            this.endDrag();
            event.preventDefault();
        }
    }

    touchMove(event: TouchEvent) {
        for (let i = 0; i < event.changedTouches.length; i++) {
            let touch = event.changedTouches.item(i);
            if (this.dragging && this.touchDrag && this.touchId == touch.identifier) {
                this.drag(touch.pageX, touch.pageY);
            }
        }
    }

    mouseMove(event: MouseEvent) {
        if (this.dragging && this.mouseDrag) {
            this.drag(event.pageX, event.pageY);
            event.preventDefault();
        }
    }

    startDrag(ptX: number, ptY: number) {
        this.dragging = true;
        this.startDragX = ptX;
        this.startDragY = ptY;
        this.style.zIndex = "10";
    }

    endDrag() {
        this.dragging = false;
        this.offsetX = 0;
        this.offsetY = 0;
        this.updateLocation();
        this.style.zIndex = "0";
    }

    drag(ptX: number, ptY: number) {
        this.offsetX = ptX - this.startDragX;
        this.offsetY = ptY - this.startDragY;
        this.updateLocation();
    }

    updateLocation() {
        this.style.width = `${this.width()}px`;
        this.style.height = `${this.height()}px`;
        this.style.transform = `translate(${this.x + this.offsetX}px, ${this.y + this.offsetY}px)`;
    }
}

class VideoNodePreview extends HTMLElement {
    content: HTMLElement;

    constructor() {
        super();
    }

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: block;
                width: 80%;
                pointer-events: none;
            }
            #square {
                position: relative;
                width: 100%;
                margin: 10px auto;
            }
            #square:after {
                content: "";
                display: block;
                padding-bottom: 100%;
            }
            #content {
                position: absolute;
                width: 100%;
                height: 100%;
                border: 1px solid white;
                background-color: gray;
            }
            </style>
            <div id="square">
                <div id="content">
                </div>
            </div>
        `;
        this.content = shadow.querySelector("#content");
    }

    render(tile: VideoNodeTile) {
        tile.graph.backendModel.paint_node(tile.uid, this.content);
    }
}

class EffectNodeTile extends VideoNodeTile {
    intensitySlider: HTMLInputElement;
    titleDiv: HTMLDivElement;
    intensitySliderBlocked: boolean;

    constructor() {
        super();
        this.nInputs = 1; // XXX Temporary, should be set by GLSL
        this.intensitySliderBlocked = false;
    }

    connectedCallback() {
        super.connectedCallback();

        this.innerHTML = `
            <style>
                hr, div {
                    pointer-events: none;
                }
            </style>
            <div style="font-family: sans-serif;" id="title"></div>
            <hr style="margin: 3px; width: 80%;"></hr>
            <radiance-videonodepreview style="flex: 1 1 auto;" id="preview"></radiance-videonodepreview>
            <input type="range" min="0" max="1" step="0.01" id="intensitySlider"></input>
        `;
        this.preview = this.querySelector("#preview");
        this.intensitySlider = this.querySelector("#intensitySlider");
        this.titleDiv = this.querySelector("#title");
        this.intensitySlider.addEventListener("input", this.intensitySliderChanged.bind(this));
    }

    updateFromModel(data: any) {
        super.updateFromModel(data);
        this.intensitySliderBlocked = true;
        this.intensitySlider.value = data.intensity;
        this.intensitySliderBlocked = false;
        this.titleDiv.textContent = data.name;
    }

    intensitySliderChanged(event: InputEvent) {
        if (this.intensitySliderBlocked) {
            return;
        }
        const newIntensity = parseFloat(this.intensitySlider.value);
        this.graph.mutateModel(this.uid, {"intensity": newIntensity});
    }
}

class MediaNodeTile extends VideoNodeTile {
    constructor() {
        super();
        this.nInputs = 1;
    }

    connectedCallback() {
        super.connectedCallback();

        this.innerHTML = `
            <div style="font-family: sans-serif;" id="title">Media</div>
            <hr style="margin: 3px; width: 80%;"></hr>
            <radiance-videonodepreview style="flex: 1 1 auto;" id="preview"></radiance-videonodepreview>
        `;
        this.preview = this.querySelector("#preview");
    }
}

interface Edge {
    fromVertex: number;
    toInput: number;
    toVertex: number;
}

interface ModelVertex {
    uid: number;
    nInputs: number; // TODO remove me
}

interface Model {
    vertices: ModelVertex[];
    edges: Edge[];
}

class Graph extends HTMLElement {
    // This custom element creates the visual context for displaying connected nodes.

    mutationObserver: MutationObserver;
    tileVertices: VideoNodeTile[];
    tileEdges: Edge[];
    nextUID: number;
    model: Model;
    backendModel: BackendModel;
    relayoutRequested: boolean;

    constructor() {
        super();

        this.nextUID = 0;
        this.tileVertices = [];
        this.tileEdges = [];
        this.relayoutRequested = false;
    }

    connectedCallback() {
        let shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: inline-block;
                pointer-events: none;
            }
            * {
                pointer-events: auto;
            }
            #canvas {
                position: fixed;
                left: 0px;
                top: 0px;
                z-index: 9999;
                pointer-events: none;
                width: 100%;
                height: 100%;
            }
            </style>
            <canvas id="canvas">
            </canvas>
            <slot></slot>
        `;

        const canvas = shadow.querySelector("#canvas");
        //this.appendChild(canvas);
        this.backendModel = new BackendModel(canvas, 512);

        window.requestAnimationFrame(this.render.bind(this));
    }

    addTile(uid: number, state) {
        const type = state.nodeType;
        let tile: VideoNodeTile;
        if (type == "effect") {
            tile = <VideoNodeTile>document.createElement("radiance-effectnodetile");
        } else if (type == "media") {
            tile = <VideoNodeTile>document.createElement("radiance-medianodetile");
        } else {
            tile = <VideoNodeTile>document.createElement("radiance-videonodetile");
        }
        this.appendChild(tile);
        tile.uid = uid;
        tile.graph = this;
        return tile;
    }

    removeTile(tile: VideoNodeTile) {
        this.removeChild(tile);
    }

    // Model looks like:
    // {"vertices": [{"file", "intensity", "type", "uid"},...], "edges": [{from, tovertex, toinput},...]}

    mutateModel(uid: number, newState: object) {
        for (let node of this.model.vertices) {
            if (node.uid == uid) {
                for (let prop in newState) {
                    node[prop] = newState[prop];
                }
                break;
            }
        }
        this.backendModel.set_state(this.model);
        this.modelChanged();
    }

    modelChanged() {
        // Convert the model DAG into a tree

        const origNumTileVertices = this.tileVertices.length;
        const origNumTileEdges = this.tileEdges.length;

        // Precompute some useful things
        let upstreamNodeVertices: number[][] = []; // Parallel to vertices. The upstream vertex index on each input, or null.
        // Even in a DAG, each input should have at most one connection.
        this.model.vertices.forEach((node, index) => {
            upstreamNodeVertices[index] = [];
            for (let i = 0; i < node.nInputs; i++) { // TODO get rid of need for model to contain nInputs
                upstreamNodeVertices[index].push(null);
            }
        });

        let startNodeVertices: number[] = Array.from(this.model.vertices.keys());

        for (let edge of this.model.edges) {
            if (edge.toInput >= upstreamNodeVertices[edge.toVertex].length) {
                throw `Model edge to nonexistant input ${edge.toInput} of vertex ${edge.toVertex}`;
            }
            if (upstreamNodeVertices[edge.toVertex][edge.toInput] !== null) {
                throw `Model vertex ${edge.toVertex} input ${edge.toInput} has multiple upstream vertices`;
            }
            upstreamNodeVertices[edge.toVertex][edge.toInput] = edge.fromVertex;

            let ix = startNodeVertices.indexOf(edge.fromVertex);
            if (ix >= 0) {
                startNodeVertices.splice(ix, 1);
            }
        }

        let startTileVertices = Array.from(this.tileVertices.keys());
        let startTileForUID : {[uid: number]: number} = {};
        let upstreamTileEdges: number[][] = []; // Parallel to vertices. The upstream edge index on each input, or null.

        // Helper function for making sure upstreamTileEdges doesn't get out of date when we add new tiles
        const addUpstreamEntries = (nInputs: number) => {
            upstreamTileEdges.push([]);
            for (let i = 0; i < nInputs; i++) {
                upstreamTileEdges[upstreamTileEdges.length - 1].push(null);
            }
        };

        this.tileVertices.forEach((tile, index) => {
            addUpstreamEntries(tile.nInputs);
        });
        this.tileEdges.forEach((edge, index) => {
            if (edge.toInput >= upstreamTileEdges[edge.toVertex].length) {
                throw `Tile edge to nonexistant input ${edge.toInput} of vertex ${edge.toVertex}`;
            }
            if (upstreamTileEdges[edge.toVertex][edge.toInput] !== null) {
                throw `Tile vertex ${edge.toVertex} input ${edge.toInput} has multiple upstream vertices`;
            }
            upstreamTileEdges[edge.toVertex][edge.toInput] = index;
            let ix = startTileVertices.indexOf(edge.fromVertex);
            if (ix >= 0) {
                startTileVertices.splice(ix, 1);
            }
        });
        startTileVertices.forEach(startTileVertex => {
            startTileForUID[this.tileVertices[startTileVertex].uid] = startTileVertex;
        });

        // Create lists of tile indices to delete, pre-populated with all indices
        let tileVerticesToDelete = Array.from(this.tileVertices.keys());
        let tileEdgesToDelete = Array.from(this.tileEdges.keys());

        // Note: Traversal will have to keep track of tiles as well as nodes.
        // TODO: This algorithm is a little aggressive, and will treat "moves" as deletion + creation

        const traverse = (nodeVertex: number, tileVertex: number) => {
            const tile = this.tileVertices[tileVertex];

            for (let input = 0; input < tile.nInputs; input++) {
                let upstreamNode = upstreamNodeVertices[nodeVertex][input];
                if (upstreamNode !== null) {
                    // Get the upstream node UID for the given input
                    const nodeUID = this.model.vertices[upstreamNode].uid;
                    // See if the connection exists on the tile
                    const upstreamTileEdgeIndex = upstreamTileEdges[tileVertex][input];

                    let upstreamTileVertexIndex = null;
                    let upstreamTile = null;

                    if (upstreamTileEdgeIndex !== null) {
                        upstreamTileVertexIndex = this.tileEdges[upstreamTileEdgeIndex].fromVertex;
                        upstreamTile = this.tileVertices[upstreamTileVertexIndex];
                        const upstreamTileUID = upstreamTile.uid;
                        if (upstreamTileUID == nodeUID) {
                            // If the tile matches, don't delete the edge or node.
                            tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(upstreamTileVertexIndex), 1);
                            tileEdgesToDelete.splice(tileEdgesToDelete.indexOf(this.tileEdges[upstreamTileEdgeIndex].fromVertex), 1);
                        }
                        // No need to specifically request deletion of an edge; simply not preserving it will cause it to be deleted
                    } else {
                        // However, we do need to add a new tile and edge.
                        upstreamTile = this.addTile(nodeUID, this.model.vertices[upstreamNode]); // TODO: Arguments...
                        upstreamTileVertexIndex = this.tileVertices.length;
                        this.tileVertices.push(upstreamTile);
                        addUpstreamEntries(upstreamTile.nInputs);
                        this.tileEdges.push({
                            fromVertex: upstreamTileVertexIndex,
                            toVertex: tileVertex,
                            toInput: input,
                        });
                    }
                    upstreamTile.updateFromModel(this.model.vertices[upstreamNode]);
                    // TODO: update tile properties here from model, such as intensity...
                    traverse(upstreamNode, upstreamTileVertexIndex);
                }
            }
        };

        // For each start node, make a tile or use an existing tile
        startNodeVertices.forEach(startNodeIndex => {
            const uid = this.model.vertices[startNodeIndex].uid;
            let startTileIndex = null;
            if (!(uid in startTileForUID)) {
                // Create tile for start node
                let tile = this.addTile(uid, this.model.vertices[startNodeIndex]);
                tile.updateFromModel(this.model.vertices[startNodeIndex]);
                tile.uid = uid;
                startTileIndex = this.tileVertices.length;
                this.tileVertices.push(tile);
                addUpstreamEntries(tile.nInputs);
                // TODO Update tile properties here from model
            } else {
                startTileIndex = startTileForUID[uid];
                // Don't delete the tile we found
                tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(startTileIndex), 1);
            }
            traverse(startNodeIndex, startTileIndex);
        });

        const changed = (this.tileVertices.length > origNumTileVertices
                      || this.tileEdges.length > origNumTileEdges
                      || tileVerticesToDelete.length > 0
                      || tileEdgesToDelete.length > 0);

        // Compute a mapping of old vertex indices -> new vertex indices, post-deletion
        let vertexTileMapping: number[] = [];
        let newIndex = 0;
        this.tileVertices.forEach((_, oldIndex) => {
            if (tileVerticesToDelete.indexOf(oldIndex) < 0) {
                // Index was not deleted
                vertexTileMapping.push(newIndex);
                newIndex++;
            } else {
                // Index was deleted
                vertexTileMapping.push(null);
            }
        });

        // Perform deletion
        tileVerticesToDelete.forEach(index => {
            this.removeTile(this.tileVertices[index]);
            delete this.tileVertices[index];
        });
        tileEdgesToDelete.forEach(index => {
            delete this.tileEdges[index];
        });
        // Remap indices
        this.tileEdges.forEach(edge => {
            edge.fromVertex = vertexTileMapping[edge.fromVertex];
            edge.toVertex = vertexTileMapping[edge.toVertex];
        });

        if (changed) {
            console.log("New tile vertices:", this.tileVertices);
            console.log("New tile edges:", this.tileEdges);
            console.log("Vertices to remove:", tileVerticesToDelete);
            console.log("Edges to remove:", tileEdgesToDelete);

            this.relayoutGraph();
        }
    }

    relayoutGraph() {
        // This method resizes and repositions all tiles
        // according to the graph structure.

        // Precompute some useful things
        let downstreamTileVertex: number[] = []; // Parallel to vertices. The downstream vertex index, or null.
        let downstreamTileInput: number[] = []; // Parallel to vertices. The downstream input index, or null.
        let upstreamTileVertices: number[][] = []; // Parallel to vertices. The upstream vertex index on each input, or null.
        this.tileVertices.forEach((tile, index) => {
            downstreamTileVertex[index] = null;
            downstreamTileInput[index] = null;
            upstreamTileVertices[index] = [];
            for (let i = 0; i < tile.nInputs; i++) {
                upstreamTileVertices[index].push(null);
            }
        });
        for (let edge of this.tileEdges) {
            if (downstreamTileVertex[edge.fromVertex] !== null) {
                throw `Vertex ${edge.fromVertex} has multiple downstream vertices`;
            }
            downstreamTileVertex[edge.fromVertex] = edge.toVertex;
            downstreamTileInput[edge.fromVertex] = edge.toInput;
            if (edge.toInput >= upstreamTileVertices[edge.toVertex].length) {
                throw `Edge to nonexistant input ${edge.toInput} of vertex ${edge.toVertex}`;
            }
            if (upstreamTileVertices[edge.toVertex][edge.toInput] !== null) {
                throw `Vertex ${edge.toVertex} input ${edge.toInput} has multiple upstream vertices`;
            }
            upstreamTileVertices[edge.toVertex][edge.toInput] = edge.fromVertex;
        }

        // 1. Find root vertices
        // (root vertices have no downstream nodes)
        let roots = [];
        this.tileVertices.forEach((tile, index) => {
            if (downstreamTileVertex[index] === null) {
                roots.push(index);
            }
        });

        // 2. From each root, set downstream nodes to be at least as tall as their upstream nodes.
        const setInputHeightsFwd = (index: number) => {
            let tile = this.tileVertices[index];
            tile.inputHeights = [];
            for (let i = 0; i < tile.nInputs; i++) {
                let height = tile.minInputHeight(i);
                let upstream = upstreamTileVertices[index][i];
                if (upstream !== null) {
                    setInputHeightsFwd(upstream);
                    height = Math.max(height, this.tileVertices[upstream].height());
                }
                tile.inputHeights.push(height);
            }
            if (tile.nInputs == 0) {
                // Push one inputheight even if there are no inputs, to set the overall height
                tile.inputHeights.push(tile.minInputHeight(0));
            }
        };

        for (let index of roots) {
            setInputHeightsFwd(index);
        }

        // 3. From each root, set upstream nodes to be at least as tall as their downstream nodes' inputs.
        const setInputHeightsRev = (index: number) => {
            let tile = this.tileVertices[index];
            let height = tile.height();

            let downstream = downstreamTileVertex[index];
            if (downstream !== null) {
                let downstreamHeight = this.tileVertices[downstream].height();
                let ratio = downstreamHeight / height;
                if (ratio > 1) {
                    // Scale all inputs proportionally to get the node sufficiently tall
                    for (let i = 0; i < tile.nInputs; i++) {
                        tile.inputHeights[i] *= ratio;
                    }
                }
            }
        };

        for (let index of roots) {
            setInputHeightsRev(index);
        }

        // 4. From the heights, calculate the Y-coordinates
        const setYCoordinate = (index: number, y: number) => {
            let tile = this.tileVertices[index];
            tile.y = y;

            for (let i = 0; i < tile.nInputs; i++) {
                let upstream = upstreamTileVertices[index][i];
                if (upstream !== null) {
                    setYCoordinate(upstream, y);
                }
                y += tile.inputHeights[i];
            }
        }

        let y = 0;
        for (let index of roots) {
            setYCoordinate(index, y);
            y += this.tileVertices[index].height();
        }
        let height = y;

        // 5. From the widths, calculate the X-coordinates
        const setXCoordinate = (index: number, x: number) => {
            let tile = this.tileVertices[index];
            tile.x = x;

            x -= tile.width();

            for (let i = 0; i < tile.nInputs; i++) {
                let upstream = upstreamTileVertices[index][i];
                if (upstream !== null) {
                    setXCoordinate(upstream, x);
                }
            }
        }

        for (let index of roots) {
            setXCoordinate(index, -this.tileVertices[index].width());
        }

        // 6. Compute total width & height, and offset all coordinates
        let smallestX = 0;
        this.tileVertices.forEach(tile => {
            if (tile.x < smallestX) {
                smallestX = tile.x;
            }
        });
        this.tileVertices.forEach(tile => {
            tile.x -= smallestX;
        });
        let width = -smallestX;
        this.style.width = `${width}px`;
        this.style.height = `${height}px`;

        for (let uid in this.tileVertices) {
            let vertex = this.tileVertices[uid];
            vertex.updateLocation();
        };
    }

    requestRelayout() {
        this.relayoutRequested = true;
    }

    render(t: number) {
        if (this.relayoutRequested) {
            this.relayoutRequested = false;
            this.relayoutGraph();
        }

        this.backendModel.render(t);
        this.tileVertices.forEach(tile => {
            tile.render();
        });
        window.requestAnimationFrame(this.render.bind(this));
    }
}

customElements.define('radiance-flickable', Flickable);
customElements.define('radiance-videonodetile', VideoNodeTile);
customElements.define('radiance-videonodepreview', VideoNodePreview);
customElements.define('radiance-effectnodetile', EffectNodeTile);
customElements.define('radiance-medianodetile', MediaNodeTile);
customElements.define('radiance-graph', Graph);
