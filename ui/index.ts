"use strict";

import {Context} from "../pkg/index.js";

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
        this.startDragOffsetX = this.offsetX;
        this.startDragOffsetY = this.offsetY;
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
    dragCtrl: boolean; // Whether or not shift-key was held at start of drag
    dragShift: boolean; // Whether or not ctrl-key was held at start of drag
    dragSelected: boolean; // Whether or not tile was selected at start of drag
    wasClick: boolean; // Whether or not a drag event should be interpreted as a click instead
    oldWidth: number; // State to avoid unnecessary CSS width changes
    oldHeight: number; // State to avoid unnecessary CSS height changes

    // Properties relating to selection
    _selected: boolean;

    constructor() {
        super();
        this.nInputs = 0;
        this.inputHeights = [];
        this.uid = null;

        this.offsetX = 0;
        this.offsetY = 0;
        this.dragging = false;

        this._selected = false;
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
                outline: none;
                pointer-events: none;
                transition: transform 1s, width 1s, height 1s;
            }

            :host(:focus), :host([dragging]) {
                z-index: 10;
                will-change: transform;
                transition: transform 0s, width 0s, height 0s;
            }

            :host(:focus) #outline {
                border: 2px solid white;
            }

            :host([selected]) #inner {
                background-color: #000066;
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
                pointer-events: all;
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
        this.tabIndex = 0;

        this.addEventListener('keypress', this.keyPress.bind(this));
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

    updateFromState(data: any) {
        if ("nInputs" in data && data.nInputs != this.nInputs) {
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
                this.touchId = touch.identifier;
                this.dragCtrl = event.ctrlKey;
                this.dragShift = event.shiftKey;
                this.dragSelected = this.selected;
                this.graph.ensureSelected(this, this.dragCtrl, this.dragShift);
                this.focus();
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
            this.dragCtrl = event.ctrlKey;
            this.dragShift = event.shiftKey;
            this.dragSelected = this.selected;
            this.graph.ensureSelected(this, this.dragCtrl, this.dragShift);
            this.focus();
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
        this.setAttribute("dragging", "true");
        this.wasClick = true;
    }

    endDrag() {
        this.dragging = false;
        this.offsetX = 0;
        this.offsetY = 0;
        this.removeAttribute("dragging");
        this.updateLocation();
        if (this.wasClick) {
            this.graph.select(this, this.dragSelected, this.dragCtrl, this.dragShift);
        }
    }

    drag(ptX: number, ptY: number) {
        this.wasClick = false;
        this.offsetX = ptX - this.startDragX;
        this.offsetY = ptY - this.startDragY;
        this.updateLocation();
    }

    updateLocation() {
        const newWidth = this.width();
        const newHeight = this.height();
        if (newWidth != this.oldWidth) {
            this.style.width = `${newWidth}px`;
            this.oldWidth = newWidth;
        }
        if (newHeight != this.oldHeight) {
            this.style.height = `${newHeight}px`;
            this.oldHeight = newHeight;
        }
        this.style.transform = `translate(${this.x + this.offsetX}px, ${this.y + this.offsetY}px)`;
    }

    get selected() {
        return this._selected;
    }

    set selected(value: boolean) {
        this._selected = value;
        if (value) {
            this.setAttribute("selected", "true");
        } else {
            this.removeAttribute("selected");
        }
    }

    keyPress(event: KeyboardEvent) {
        if (event.code == "Delete") {
            this.graph.deleteSelected();
        } else if (event.code == "Space") {
            // XXX for debugging
            const selection = Array.from(this.graph.tiles.vertices.keys()).filter((index) => {
                return this.graph.tiles.vertices[index].selected;
            });
            console.log("Connected components:");
            this.graph.tiles.connectedComponents(selection).forEach(component => {
                console.log(component);
            });
        }
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
        if (tile.dragging) {
            tile.graph.context.clearElement(tile.inner);
        }
        tile.graph.context.paintNode(tile.uid, this.content);
    }
}

class EffectNodeTile extends VideoNodeTile {
    intensitySlider: HTMLInputElement;
    titleDiv: HTMLDivElement;
    intensitySliderBlocked: boolean;

    constructor() {
        super();
        this.nInputs = 0;
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

    updateFromState(data: any) {
        super.updateFromState(data);
        if ("intensity" in data) {
            this.intensitySliderBlocked = true;
            this.intensitySlider.value = data.intensity;
            this.intensitySliderBlocked = false;
        }

        if ("name" in data) {
            this.titleDiv.textContent = data.name;
        }
    }

    intensitySliderChanged(event: InputEvent) {
        if (this.intensitySliderBlocked) {
            return;
        }
        const newIntensity = parseFloat(this.intensitySlider.value);
        let state = this.graph.context.nodeState(this.uid, "local");
        state.intensity = newIntensity;
        this.graph.context.setNodeState(this.uid, state);
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

interface ConnectedComponent {
    vertices: number[];
    rootVertex: number;
    internalEdges: number[];
    inputEdges: number[];
    outputEdges: number[];
}

class GraphData<T> {
    vertices: T[]; // Array of graph vertices (passed into constructor)
    edges: Edge[]; // Array of edges (passed into constructor)
    rootVertices: number[]; // Which vertex indices have no outgoing edges
    upstreamEdges: number[][]; // Parallel to vertices. Second index is which input, and the result is the upstream edge index.
    downstreamEdges: number[][]; // Parallel to vertices. Each entry is unsorted list of downstream edge indices.

    constructor(vertices: T[], edges: Edge[]) {
        this.vertices = [];
        this.edges = [];
        this.rootVertices = [];
        this.upstreamEdges = [];
        this.downstreamEdges = [];

        vertices.forEach(vertex => {
            this.addVertex(vertex);
        });
        edges.forEach(edge => {
            this.addEdge(edge);
        });
    }

    upstreamVertexIndex(vertexIndex: number, input: number) {
        const edge = this.upstreamEdges[vertexIndex][input];
        if (edge === undefined) {
            return undefined;
        } else {
            return this.edges[edge].fromVertex;
        }
    }

    downstreamVertexIndices(vertexIndex: number) {
        const edges = this.downstreamEdges[vertexIndex];
        return edges.map(edge => this.edges[edge].toVertex);
    }

    addVertex(vertex: T) {
        const index = this.vertices.length;
        this.vertices.push(vertex);
        this.upstreamEdges.push([]);
        this.downstreamEdges.push([]);
        this.rootVertices.push(index);
        return index;
    }

    addEdge(edge: Edge) {
        const index = this.edges.length;
        this.edges.push(edge);

        if (this.upstreamEdges[edge.toVertex][edge.toInput] !== undefined) {
            throw `Vertex ${edge.toVertex} input ${edge.toInput} has multiple upstream vertices`;
        }
        this.upstreamEdges[edge.toVertex][edge.toInput] = index;
        this.downstreamEdges[edge.fromVertex].push(index);

        let ix = this.rootVertices.indexOf(edge.fromVertex);
        if (ix >= 0) {
            this.rootVertices.splice(ix, 1);
        }
    }

    removeVertices(indices: number[]) {
        // Note: this function does not remove edges,
        // but will set their to/from to undefined.

        // Compute a mapping of old vertex indices -> new vertex indices, post-deletion
        let mapping: number[] = [];
        let newIndex = 0;
        this.vertices.forEach((_, oldIndex) => {
            if (indices.indexOf(oldIndex) < 0) {
                // Index was not deleted
                mapping[oldIndex] = newIndex;
                newIndex++;
            }
        });

        // Perform deletion
        this.vertices = this.vertices.filter((_, index) => indices.indexOf(index) < 0);
        this.rootVertices = this.rootVertices.filter(v => indices.indexOf(v) < 0);
        this.upstreamEdges = this.upstreamEdges.filter((_, index) => indices.indexOf(index) < 0);
        this.downstreamEdges = this.downstreamEdges.filter((_, index) => indices.indexOf(index) < 0);

        // Remap indices
        this.edges.forEach(edge => {
            edge.fromVertex = mapping[edge.fromVertex];
            edge.toVertex = mapping[edge.toVertex];
        });
        this.rootVertices.forEach((v, i) => {
            this.rootVertices[i] = mapping[v];
        });
    }

    removeEdges(indices: number[]) {
        let indices_sorted = indices.slice();
        indices_sorted.sort((a, b) => b - a);
        indices_sorted.forEach(index => {
            this.removeEdge(index);
        });
    }

    removeEdge(index: number) {
        const edge = this.edges[index];
        this.edges.splice(index, 1);

        if (edge.toVertex !== undefined) {
            delete this.upstreamEdges[edge.toVertex][edge.toInput];
        }
        if (edge.fromVertex !== undefined) {
            this.downstreamEdges[edge.fromVertex].splice(this.downstreamEdges[edge.fromVertex].indexOf(index), 1);
        }

        if (edge.fromVertex !== undefined && this.downstreamEdges[edge.fromVertex].length == 0) {
            this.rootVertices.push(edge.fromVertex);
        }

        this.vertices.forEach((_, v) => {
            this.upstreamEdges[v].forEach((e, i) => {
                if (e > index) {
                    this.upstreamEdges[v][i] = e - 1;
                }
            });
            this.downstreamEdges[v].forEach((e, i) => {
                if (e > index) {
                    this.downstreamEdges[v][i] = e - 1;
                }
            });
        });
    }

    // Enumerate the connected components of the subgraph containing the given vertices
    connectedComponents(vertexIndices: number[]): ConnectedComponent[] {
        const componentRootVertices = vertexIndices.filter(vertexIndex => {
            return this.downstreamEdges[vertexIndex].reduce((isRoot, edge) => {
                return isRoot && vertexIndices.indexOf(this.edges[edge].toVertex) < 0;
            }, true);
        });

        return componentRootVertices.map((vertexIndex: number): ConnectedComponent => {
            let connectedComponent: ConnectedComponent = {
                vertices: [],
                rootVertex: vertexIndex,
                internalEdges: [],
                inputEdges: [],
                outputEdges: [],
            };

            // Recursive function to traverse the subgraph and populate connectedComponent
            const traverse = (vertexIndex: number) => {
                // Add this vertex to the included vertices
                connectedComponent.vertices.push(vertexIndex);

                // Add any downstream edges to non-component vertices to the outputEdges list
                const newOutputEdges = this.downstreamEdges[vertexIndex].filter(downstreamEdgeIndex => {
                    return vertexIndices.indexOf(this.edges[downstreamEdgeIndex].toVertex) < 0;
                });
                connectedComponent.outputEdges.push(...newOutputEdges);

                // Add any upstream edges to non-component vertices to the inputEdges list
                const newInputEdges = this.upstreamEdges[vertexIndex].filter(upstreamEdgeIndex => {
                    return vertexIndices.indexOf(this.edges[upstreamEdgeIndex].fromVertex) < 0;
                });
                connectedComponent.inputEdges.push(...newInputEdges);

                // Add and Recurse on upstream edges to component vertices
                const newInternalEdges = this.upstreamEdges[vertexIndex].filter(upstreamEdgeIndex => {
                    return vertexIndices.indexOf(this.edges[upstreamEdgeIndex].fromVertex) >= 0;
                });
                connectedComponent.internalEdges.push(...newInternalEdges);
                let upstreamVertices = newInternalEdges.map(edgeIndex => this.edges[edgeIndex].fromVertex);
                upstreamVertices = upstreamVertices.filter((value, index, self) => self.indexOf(value) == index);
                upstreamVertices.forEach(upstreamVertexIndex => {
                    traverse(upstreamVertexIndex);
                });
            };

            traverse(vertexIndex);
            return connectedComponent;
        });
    }

    // Returns the list of new edges that would need to be added to "bypass"
    // the given connected component.
    bypassEdges(cc: ConnectedComponent): Edge[] {
        let result = [];
        if (cc.inputEdges.length >= 1) {
            cc.outputEdges.forEach(outputEdgeIndex => {
                const outputEdge = this.edges[outputEdgeIndex];
                if (outputEdge.fromVertex == cc.rootVertex) {
                    result.push({
                        fromVertex: this.edges[cc.inputEdges[0]].fromVertex,
                        toVertex: outputEdge.toVertex,
                        toInput: outputEdge.toInput,
                    });
                }
            });
        }
        return result;
    }

}

class Graph extends HTMLElement {
    // This custom element creates the visual context for displaying connected nodes.

    nextUID: number;
    nodes: GraphData<UID>; // Vertices are node UIDs
    tiles: GraphData<VideoNodeTile>; // Vertices are tiles
    context: Context;
    relayoutRequested: boolean;
    oldWidth: number;
    oldHeight: number;

    constructor() {
        super();

        this.nextUID = 0;
        this.relayoutRequested = false;
    }

    connectedCallback() {
        let shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: inline-block;
                pointer-events: none;
                transition: width 1s, height 1s;
            }
            * {
                pointer-events: auto;
            }
            </style>
            <slot></slot>
        `;
        this.tiles = new GraphData([], []);
    }

    attachContext(context: Context) {
        if (this.context !== undefined) {
            throw "Cannot call attachContext more than once!";
        }

        this.context = context;
        this.context.onGraphChanged(this.nodesChanged.bind(this));
        window.requestAnimationFrame(this.render.bind(this));
    }

    addTile(uid: number, state) { // TODO add type to state
        const type = state.nodeType;
        let tile: VideoNodeTile;
        if (type == "EffectNode") {
            tile = <VideoNodeTile>document.createElement("radiance-effectnodetile");
        } else if (type == "MediaNode") {
            tile = <VideoNodeTile>document.createElement("radiance-medianodetile");
        } else {
            tile = <VideoNodeTile>document.createElement("radiance-videonodetile");
        }
        this.appendChild(tile);
        tile.uid = uid;
        tile.graph = this;
        tile.updateFromState(state);
        // XXX needs RefCell wrapper
        //this.context.onNodeChanged(uid, "all", tile.updateFromState.bind(tile));
        return tile;
    }

    removeTile(tile: VideoNodeTile) {
        this.removeChild(tile);
    }

    nodesChanged() {
        let nodes = this.context.state();
        this.nodes = new GraphData(nodes.vertices, nodes.edges);

        // Convert the nodes DAG into a tree

        const origNumTileVertices = this.tiles.vertices.length;

        let tileForUID : {[uid: number]: number} = {};

        // There can be multiple tiles for one UID... this takes the last one. Probably not always the best option.
        this.tiles.vertices.forEach((vertex, index) => {
            tileForUID[vertex.uid] = index;
        });

        // Create lists of tile indices to delete, pre-populated with all indices
        let tileVerticesToDelete: number[] = Array.from(this.tiles.vertices.keys());
        let tileEdgesToDelete: number[] = Array.from(this.tiles.edges.keys());
        let tileEdgesToCreate: Edge[] = [];

        // Note: Traversal will have to keep track of tiles as well as nodes.
        // TODO: This algorithm is a little aggressive, and will treat "moves" as deletion + creation

        const traverse = (nodeVertex: number, tileVertex: number) => {
            const tile = this.tiles.vertices[tileVertex];

            for (let input = 0; input < tile.nInputs; input++) {
                let upstreamNode = this.nodes.upstreamVertexIndex(nodeVertex, input);
                if (upstreamNode !== undefined) {
                    // Get the upstream node UID for the given input
                    const nodeUID = this.nodes.vertices[upstreamNode];
                    // See if the connection exists on the tile
                    const upstreamTileEdgeIndex = this.tiles.upstreamEdges[tileVertex][input];

                    let upstreamTileVertexIndex;
                    let reuse = false;

                    if (upstreamTileEdgeIndex !== undefined) {
                        upstreamTileVertexIndex = this.tiles.edges[upstreamTileEdgeIndex].fromVertex;
                        const upstreamTile = this.tiles.vertices[upstreamTileVertexIndex];
                        const upstreamTileUID = upstreamTile.uid;
                        if (upstreamTileUID == nodeUID) {
                            // If the tile matches, don't delete the edge or node.
                            tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(upstreamTileVertexIndex), 1);
                            tileEdgesToDelete.splice(tileEdgesToDelete.indexOf(upstreamTileEdgeIndex), 1);
                            upstreamTile.updateFromState(this.context.nodeState(this.nodes.vertices[upstreamNode], "all"));
                            reuse = true;
                        }
                        // No need to specifically request deletion of an edge; simply not preserving it will cause it to be deleted
                    }

                    if (!reuse) {
                        // See if there's an unused tile with the correct UID we can re-use.
                        // This is sort of naive and may lead to weird tile-rearranging behavior visually.
                        for (let index of tileVerticesToDelete) {
                            if (this.tiles.vertices[index].uid == nodeUID) {
                                upstreamTileVertexIndex = index;
                                tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(upstreamTileVertexIndex), 1);
                                this.tiles.vertices[upstreamTileVertexIndex].updateFromState(this.context.nodeState(this.nodes.vertices[upstreamNode], "all"));
                                break;
                            }
                        }
                        if (upstreamTileVertexIndex === undefined) {
                            // No suitable node was found. Create a new one.
                            const state = this.context.nodeState(this.nodes.vertices[upstreamNode], "all");
                            const upstreamTile = this.addTile(nodeUID, state);
                            upstreamTileVertexIndex = this.tiles.addVertex(upstreamTile);
                        }
                        tileEdgesToCreate.push({
                            fromVertex: upstreamTileVertexIndex,
                            toVertex: tileVertex,
                            toInput: input,
                        });
                    }
                    traverse(upstreamNode, upstreamTileVertexIndex);
                }
            }
        };

        // For each start node, make a tile or use an existing tile
        this.nodes.rootVertices.forEach(startNodeIndex => {
            const uid = this.nodes.vertices[startNodeIndex];
            let startTileIndex;
            if (!(uid in tileForUID)) {
                // Create tile for start node
                const state = this.context.nodeState(this.nodes.vertices[startNodeIndex], "all");
                let tile = this.addTile(uid, state);
                tile.uid = uid;
                startTileIndex = this.tiles.addVertex(tile);
            } else {
                startTileIndex = tileForUID[uid];
                // Don't delete the tile we found
                tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(startTileIndex), 1);
            }
            traverse(startNodeIndex, startTileIndex);
        });

        const changed = (this.tiles.vertices.length > origNumTileVertices
                      || tileEdgesToCreate.length > 0
                      || tileVerticesToDelete.length > 0
                      || tileEdgesToDelete.length > 0);

        // Delete old tiles

        tileVerticesToDelete.forEach(index => {
            this.removeTile(this.tiles.vertices[index]);
        });

        this.tiles.removeEdges(tileEdgesToDelete);
        tileEdgesToCreate.forEach(edge => {
            this.tiles.addEdge(edge);
        });
        this.tiles.removeVertices(tileVerticesToDelete);

        if (changed) {
            console.log("New tile vertices:", this.tiles.vertices);
            console.log("New tile edges:", this.tiles.edges);
            console.log("Vertices to remove:", tileVerticesToDelete);
            console.log("Edges to remove:", tileEdgesToDelete);

            this.relayoutGraph();
        }
    }

    relayoutGraph() {
        // This method resizes and repositions all tiles
        // according to the graph structure.

        // 1. From each root, set downstream nodes to be at least as tall as their upstream nodes.
        const setInputHeightsFwd = (index: number) => {
            let tile = this.tiles.vertices[index];
            tile.inputHeights = [];
            for (let i = 0; i < tile.nInputs; i++) {
                let height = tile.minInputHeight(i);
                let upstreamVertexIndex = this.tiles.upstreamVertexIndex(index, i);
                if (upstreamVertexIndex !== undefined) {
                    setInputHeightsFwd(upstreamVertexIndex);
                    height = Math.max(height, this.tiles.vertices[upstreamVertexIndex].height());
                }
                tile.inputHeights.push(height);
            }
            if (tile.nInputs == 0) {
                // Push one inputheight even if there are no inputs, to set the overall height
                tile.inputHeights.push(tile.minInputHeight(0));
            }
        };

        for (let index of this.tiles.rootVertices) {
            setInputHeightsFwd(index);
        }

        // 2. From each root, set upstream nodes to be at least as tall as their downstream nodes' inputs.
        const setInputHeightsRev = (index: number) => {
            let tile = this.tiles.vertices[index];
            let height = tile.height();

            let downstreamVertexIndex = this.tiles.downstreamVertexIndices(index)[0];
            if (downstreamVertexIndex !== undefined) {
                let downstreamHeight = this.tiles.vertices[downstreamVertexIndex].height();
                let ratio = downstreamHeight / height;
                if (ratio > 1) {
                    // Scale all inputs proportionally to get the node sufficiently tall
                    for (let i = 0; i < tile.nInputs; i++) {
                        tile.inputHeights[i] *= ratio;
                    }
                }
            }
        };

        for (let index of this.tiles.rootVertices) {
            setInputHeightsRev(index);
        }

        // 3. From the heights, calculate the Y-coordinates
        const setYCoordinate = (index: number, y: number) => {
            let tile = this.tiles.vertices[index];
            tile.y = y;

            for (let i = 0; i < tile.nInputs; i++) {
                let upstreamVertexIndex = this.tiles.upstreamVertexIndex(index, i);
                if (upstreamVertexIndex !== undefined) {
                    setYCoordinate(upstreamVertexIndex, y);
                }
                y += tile.inputHeights[i];
            }
        }

        let y = 0;
        for (let index of this.tiles.rootVertices) {
            setYCoordinate(index, y);
            y += this.tiles.vertices[index].height();
        }
        let height = y;

        // 4. From the widths, calculate the X-coordinates
        const setXCoordinate = (index: number, x: number) => {
            let tile = this.tiles.vertices[index];
            tile.x = x;

            x -= tile.width();

            for (let i = 0; i < tile.nInputs; i++) {
                let upstream = this.tiles.upstreamVertexIndex(index, i);
                if (upstream !== undefined) {
                    setXCoordinate(upstream, x);
                }
            }
        }

        for (let index of this.tiles.rootVertices) {
            setXCoordinate(index, -this.tiles.vertices[index].width());
        }

        // 5. Compute total width & height, and offset all coordinates
        let smallestX = 0;
        this.tiles.vertices.forEach(tile => {
            if (tile.x < smallestX) {
                smallestX = tile.x;
            }
        });
        this.tiles.vertices.forEach(tile => {
            tile.x -= smallestX;
        });
        let width = -smallestX;
        if (this.oldWidth != width) {
            this.style.width = `${width}px`;
            this.oldWidth = width;
        }
        if (this.oldHeight != height) {
            this.style.height = `${height}px`;
            this.oldHeight = height;
        }

        for (let uid in this.tiles.vertices) {
            let vertex = this.tiles.vertices[uid];
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

        this.context.render(t);
        this.tiles.vertices.forEach(tile => {
            tile.render();
        });
        window.requestAnimationFrame(this.render.bind(this));
    }

    ensureSelected(tile: VideoNodeTile, ctrlKey: boolean, shiftKey: boolean) {
        if (!tile.selected) {
            if (!ctrlKey && !shiftKey) {
                this.deselectAll();
            }
            tile.selected = true;
        }
    }

    select(tile: VideoNodeTile, selected: boolean, ctrlKey: boolean, shiftKey: boolean) {
        if (selected) {
            if (ctrlKey) {
                tile.selected = false;
            } else {
                this.deselectAll();
                tile.selected = true;
            }
        } else {
            if (!ctrlKey && !shiftKey) {
                this.deselectAll();
            }
            tile.selected = true;
        }
    }

    deselectAll() {
        this.tiles.vertices.forEach(tile => {
            tile.selected = false;
        });
    }

    addEdge(edge: Edge) {
        this.context.addEdge(this.nodes.vertices[edge.fromVertex], this.nodes.vertices[edge.toVertex], edge.toInput);
    }

    removeEdgeByIndex(index: number) {
        const edge = this.nodes.edges[index];
        this.context.removeEdge(this.nodes.vertices[edge.fromVertex], this.nodes.vertices[edge.toVertex], edge.toInput);
    }

    deleteSelected() {
        let uidsToDelete: number[] = [];
        this.tiles.vertices.forEach(tile => {
            if (tile.selected && uidsToDelete.indexOf(tile.uid) < 0) {
                uidsToDelete.push(tile.uid);
            }
        });

        // Deletion is done in node space. May want to attempt to do it in tile space first.
        const selection = Array.from(this.nodes.vertices.keys()).filter((index) => {
            return uidsToDelete.indexOf(this.nodes.vertices[index]) >= 0;
        });
        const ccs = this.nodes.connectedComponents(selection);

        let bypassEdges: Edge[] = [];
        ccs.forEach(cc => {
            bypassEdges.push(...this.nodes.bypassEdges(cc));
        });

        uidsToDelete.forEach(uid => {
            this.context.removeNode(uid);
        });

        bypassEdges.forEach(edge => {
            this.addEdge(edge);
        });

        this.context.flush();
    }
}

customElements.define('radiance-flickable', Flickable);
customElements.define('radiance-videonodetile', VideoNodeTile);
customElements.define('radiance-videonodepreview', VideoNodePreview);
customElements.define('radiance-effectnodetile', EffectNodeTile);
customElements.define('radiance-medianodetile', MediaNodeTile);
customElements.define('radiance-graph', Graph);
