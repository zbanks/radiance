import {Context} from "../pkg/index.js";
import {GraphData, Edge, ConnectedComponent} from "./graph_data";

type UID = string;

export interface AdjustEventDetail {
    control: number; // Which control to adjust (0 = primary, 1 = secondary, ...)
    amountRelative: number; // How much to adjust it (relative to current value)
}

export class VideoNodeTile extends HTMLElement {
    nInputs: number;
    inputHeights: number[];
    uid: UID;
    x: number;
    y: number;
    graph: Graph;
    preview: VideoNodePreview;
    inner: HTMLElement;

    // Properties relating to drag
    offsetX: number; // Current X offset of the tile
    offsetY: number; // Current Y offset of the tile
    dragging: boolean; // Whether or not the tile is being dragged
    mouseDrag: boolean; // Whether or not the tile is being dragged due to the mouse
    touchDrag: boolean; // Whether or not the tile is being dragged due to a touch
    startDragX: number; // X coordinate of the start of the drag
    startDragY: number; // Y coordinate of the start of the drag
    touchId; // The identifier of the touchevent causing the drag
    dragCtrl: boolean; // Whether or not shift-key was held at start of drag
    dragShift: boolean; // Whether or not ctrl-key was held at start of drag
    dragSelected: boolean; // Whether or not tile was selected at start of drag
    wasClick: boolean; // Whether or not a drag event should be interpreted as a click instead
    oldWidth: number; // State to avoid unnecessary CSS width changes
    oldHeight: number; // State to avoid unnecessary CSS height changes
    ccDrag: boolean; // Whether or not the tile is being dragged due to being part of a connected component

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
        this.mouseDrag = false;
        this.touchDrag = false;
        this.ccDrag = false;

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
                will-change: transform;
                transition: transform 1s, width 1s, height 1s, opacity 0.5s, z-index 1s;
            }

            :host(:focus) {
                transition: transform 1s, width 1s, height 1s, opacity 0.5s, z-index 0s;
                z-index: 1001;
            }

            :host([dragging]) {
                opacity: 0.5;
                z-index: 1000;
                transition: transform 0s, width 0s, height 0s, opacity 0.5s, z-index 0s;
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
        this.addEventListener('wheel', this.onScroll.bind(this));
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
        if (this.dragging) {
            throw "Cannot start drag while drag in progress";
        }
        this.dragging = true;
        this.startDragX = ptX;
        this.startDragY = ptY;
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
        } else {
            if (!this.ccDrag) {
                this.graph.endDragCC();
            }
        }
    }

    drag(ptX: number, ptY: number) {
        if (this.wasClick) {
            this.wasClick = false;
            this.setAttribute("dragging", "true");
            if (!this.ccDrag) {
                this.graph.startDragCC(this, this.startDragX, this.startDragY);
            }
        }
        if (!this.ccDrag) {
            this.graph.dragCC(ptX, ptY);
        }
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

    onScroll(event: WheelEvent) {
        this.graph.onScroll(event);
        event.preventDefault();
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

class DropTarget extends HTMLElement {
    inner: HTMLElement;
    fromUID: UID;
    toUID: UID;
    toInput: number;
    x: number;
    y: number;
    _active: boolean;
    _dragging: boolean;

    constructor() {
        super();

        this._active = false;
        this._dragging = false;
    }

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: block;
                position: absolute;
                top: 0px;
                left: 0px;
                width: 0px;
                height: 0px;
                box-sizing: border-box;
                z-index: 1;
                outline: none;
                pointer-events: all;
                will-change: opacity;
                transition: opacity 0.2s;
                opacity: 0;
            }

            :host(:not([dragging])) {
                display: none;
            }

            :host([active]) {
                opacity: 1;
            }

            #inner {
                position: absolute;
                width: 50px;
                height: 200px;
                left: -25px;
                top: -100px;
                background-image: radial-gradient(closest-side, yellow, transparent);
            }
            </style>
            <div id="inner">
            </div>
        `;

        this.inner = shadow.querySelector("#inner");
    }

    updateLocation() {
        this.style.transform = `translate(${this.x}px, ${this.y}px)`;
    }

    get active() {
        return this._active;
    }

    set active(value: boolean) {
        if (value && !this._active) {
            this.setAttribute("active", "true");
        } else if (!value && this._active) {
            this.removeAttribute("active");
        }
        this._active = value;
    }

    get dragging() {
        return this._dragging;
    }

    set dragging(value: boolean) {
        if (value && !this._dragging) {
            this.setAttribute("dragging", "true");
        } else if (!value && this._dragging) {
            this.removeAttribute("dragging");
        }
        this._dragging = value;
    }
}

export class Graph extends HTMLElement {
    // This custom element creates the visual context for displaying connected nodes.

    nodes: GraphData<UID>; // Vertices are node UIDs
    tiles: GraphData<VideoNodeTile>; // Vertices are tiles
    context: Context;
    relayoutRequested: boolean;
    oldWidth: number;
    oldHeight: number;
    currentDragCC: ConnectedComponent; // The entity currently being dragged
    dropTargets: DropTarget[];
    lastTile: VideoNodeTile;
    selectUID: UID; // Which tile to select after graph reconciliation

    constructor() {
        super();

        this.relayoutRequested = false;
        this.dropTargets = [];
        this.tiles = new GraphData([], []);
        // A new GraphData is created for the nodes on every nodesChanged.
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
    }

    attachContext(context: Context) {
        if (this.context !== undefined) {
            throw "Cannot call attachContext more than once!";
        }

        this.context = context;
        this.context.onGraphChanged(this.nodesChanged.bind(this));
        this.nodesChanged();
        window.requestAnimationFrame(this.render.bind(this));
    }

    addTile(uid: UID, state) { // TODO add type to state
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
        this.context.onNodeChanged(uid, "all", tile.updateFromState.bind(tile));
        return tile;
    }

    removeTile(tile: VideoNodeTile) {
        this.removeChild(tile);
    }

    addDropTarget(fromUID: UID, toUID: UID, toInput: number) {
        const dropTarget = <DropTarget>document.createElement("radiance-droptarget");
        dropTarget.fromUID = fromUID;
        dropTarget.toUID = toUID;
        dropTarget.toInput = toInput;
        this.appendChild(dropTarget);
        this.dropTargets.push(dropTarget);
        return dropTarget;
    }

    removeDropTargets() {
        this.dropTargets.forEach(dropTarget => {
            this.removeChild(dropTarget);
        });
        this.dropTargets = [];
    }

    nodesChanged() {
        let edge_graph: {[uid: string]: {[index: number]: string}} = this.context.state();

        // Convert graph representation into lists of verticies & edges
        let indexForUID: {[uid: string]: number} = {};
        let vertices: UID[] = [];
        Object.keys(edge_graph).forEach(uid => {
            indexForUID[uid] = vertices.length;
            vertices.push(uid);
        });

        let edges: Edge[] = [];
        Object.entries(edge_graph).forEach(([uid, incoming]) => {
            Object.entries(incoming).forEach(([index, fromUID]) => {
                if (fromUID !== null) {
                    edges.push({
                        fromVertex: indexForUID[fromUID],
                        toVertex: indexForUID[uid],
                        toInput: +index,
                    });
                }
            });
        });

        this.nodes = new GraphData(vertices, edges);

        // Convert the nodes DAG into a tree

        const origNumTileVertices = this.tiles.vertices.length;

        let tileForUID : {[uid: string]: number} = {};

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

                    if (upstreamTileEdgeIndex !== undefined) {
                        const trialUpstreamTileVertexIndex = this.tiles.edges[upstreamTileEdgeIndex].fromVertex;
                        const upstreamTile = this.tiles.vertices[trialUpstreamTileVertexIndex];
                        const upstreamTileUID = upstreamTile.uid;
                        if (upstreamTileUID == nodeUID) {
                            // If the tile matches, don't delete the edge or node.
                            tileVerticesToDelete.splice(tileVerticesToDelete.indexOf(trialUpstreamTileVertexIndex), 1);
                            tileEdgesToDelete.splice(tileEdgesToDelete.indexOf(upstreamTileEdgeIndex), 1);
                            upstreamTile.updateFromState(this.context.nodeState(this.nodes.vertices[upstreamNode], "all"));
                            // Trial was a success; use this tile and edge
                            upstreamTileVertexIndex = trialUpstreamTileVertexIndex;
                        }
                        // No need to specifically request deletion of an edge; simply not preserving it will cause it to be deleted
                    }

                    if (upstreamTileVertexIndex === undefined) {
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
                            if (upstreamTile.uid == this.selectUID) {
                                this.select(upstreamTile, false, false, false);
                                upstreamTile.focus();
                                this.selectUID = undefined;
                            }
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
            this.relayoutDropTargets();
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

    relayoutDropTargets() {
        this.removeDropTargets();

        // Add drop tiles at the inputs of every node
        this.tiles.vertices.forEach((tile, tileIndex) => {
            let offsetHeight = 0;
            for (let i = 0; i < tile.nInputs; i++) {
                const fromIndex = this.tiles.upstreamVertexIndex(tileIndex, i);
                let fromUID = null;
                if (fromIndex !== undefined) {
                    fromUID = this.tiles.vertices[fromIndex].uid;
                }
                const toUID = tile.uid;
                const toInput = i;
                const dt = this.addDropTarget(fromUID, toUID, toInput);
                dt.x = tile.x;
                dt.y = tile.y + offsetHeight + 0.5 * tile.inputHeights[i];
                offsetHeight += tile.inputHeights[i];
            }
        });

        // Add drop tiles to the outputs of every root node
        this.tiles.rootVertices.forEach(tileIndex => {
            const tile = this.tiles.vertices[tileIndex];
            const dt = this.addDropTarget(tile.uid, null, null);
            dt.x = tile.x + tile.width();
            dt.y = tile.y + 0.5 * tile.height();
        });

        this.dropTargets.forEach(dt => {
            dt.updateLocation();
        });
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

    // Called when dragging. Don't want clicks without modifiers to clear selection.
    ensureSelected(tile: VideoNodeTile, ctrlKey: boolean, shiftKey: boolean) {
        if (!tile.selected) {
            if (!ctrlKey && !shiftKey) {
                this.deselectAll();
            } else if (shiftKey) {
                this.selectBetween(this.lastTile, tile);
            }
            tile.selected = true;
        }
    }

    // Called when clicking. Clicks without modifiers should clear selection.
    select(tile: VideoNodeTile, selected: boolean, ctrlKey: boolean, shiftKey: boolean) {
        if (selected) {
            if (shiftKey) {
                this.selectBetween(this.lastTile, tile);
            } else if (ctrlKey) {
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
        this.lastTile = tile;
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
        let uidsToDelete: UID[] = [];
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

    // Called by a tile to indicate that it is being dragged, and all of its connected component should also be dragged
    startDragCC(tile: VideoNodeTile, ptX: number, ptY: number) {
        const tileIndex = this.tiles.vertices.indexOf(tile);
        if (tileIndex < 0) {
            throw "Dragged tile not found in graph";
        }

        const selection = Array.from(this.tiles.vertices.keys()).filter((index) => {
            return this.tiles.vertices[index].selected || index == tileIndex;
        });

        let dragCC;
        for (const cc of this.tiles.connectedComponents(selection)) {
            if (cc.vertices.indexOf(tileIndex) >= 0) {
                dragCC = cc;
                break;
            }
        }
        if (dragCC === undefined) {
            throw "No connected component contains the chosen tile";
        }
        this.currentDragCC = dragCC;

        this.currentDragCC.vertices.forEach(vertex => {
            if (vertex != tileIndex) {
                this.tiles.vertices[vertex].ccDrag = true;
                this.tiles.vertices[vertex].startDrag(ptX, ptY);
            }
        });
        // May want to hold off graph changes until the drag is complete...
    }

    dragCC(ptX: number, ptY: number) {
        this.tiles.vertices.forEach(vertex => {
            if (vertex.ccDrag) {
                vertex.drag(ptX, ptY);
            }
        });
        this.activateDropTarget(ptX, ptY);
    }

    endDragCC() {
        this.tiles.vertices.forEach(vertex => {
            if (vertex.ccDrag) {
                vertex.endDrag();
                vertex.ccDrag = false;
            }
        });
        this.drop();
    }

    activateDropTarget(ptX: number, ptY: number) {
        // The currently lifted tile is usually the topmost element at this point
        // so we scan down the list until we find the first element that is not a tile
        // (which will be a drop target if we are over one)
        const elements = document.elementsFromPoint(ptX, ptY);
        let element;
        for (const e of elements) {
            if (!(e instanceof VideoNodeTile)) {
                element = e;
                break;
            }
        }

        const activeDropTargetIndex = (<Element[]>this.dropTargets).indexOf(element);
        this.dropTargets.forEach((dt, index) => {
            dt.active = (index == activeDropTargetIndex);
            dt.dragging = true;
        });
    }

    drop() {
        let activeDropTarget: DropTarget;
        this.dropTargets.forEach((dt, index) => {
            if (dt.active) {
                activeDropTarget = dt;
            }
            dt.active = false;
            dt.dragging = false;
        });

        if (activeDropTarget === undefined) {
            return; // Tiles were not moved anywhere
        }

        let uidsToMove: UID[] = [];
        this.currentDragCC.vertices.forEach(tileIndex => {
            const uid = this.tiles.vertices[tileIndex].uid;
            if (uidsToMove.indexOf(uid) < 0) {
                uidsToMove.push(uid);
            }
        });

        const selection = Array.from(this.nodes.vertices.keys()).filter((index) => {
            return uidsToMove.indexOf(this.nodes.vertices[index]) >= 0;
        });
        const ccs = this.nodes.connectedComponents(selection);

        if (ccs.length != 1) {
            throw "Conversion of CC in tilespace to nodespace resulted in more than one CC";
        }

        const cc = ccs[0];

        // See if the edge we dropped on is in the CC
        for (let v of cc.vertices) {
            const uid = this.nodes.vertices[v];
            if (activeDropTarget.fromUID == uid || activeDropTarget.toUID == uid) {
                // Can't drop onto an edge that is part of the CC
                console.log("Can't drop onto an edge that is part of the lifted CC");
                return;
            }
        }

        // Remove incoming and outgoing edges
        cc.inputEdges.forEach(edge => {
            this.removeEdgeByIndex(edge);
        });
        cc.outputEdges.forEach(edge => {
            this.removeEdgeByIndex(edge);
        });

        // See if the edge we dropped on is in the graph
        //let dropEI;
        //for (let ei = 0; ei < this.nodes.edges.length; ei++) {
        //    const edge = this.nodes.edges[ei];
        //    const fromUID = this.nodes.vertices[edge.fromVertex];
        //    const toUID = this.nodes.vertices[edge.toVertex];
        //    if (fromUID == activeDropTarget.fromUID && toUID == activeDropTarget.toUID && edge.toInput == activeDropTarget.toInput) {
        //        dropEI = ei;
        //        break;
        //    }
        //}

        // Try to remove the edge we dropped onto (should exist, but might not?? see above code)
        if (activeDropTarget.fromUID !== null && activeDropTarget.toUID !== null && activeDropTarget.toInput !== null) {
            this.context.removeEdge(activeDropTarget.fromUID, activeDropTarget.toUID, activeDropTarget.toInput);
        }

        // Add in edges necessary to bypass the lifted CC
        this.nodes.bypassEdges(cc).forEach(edge => {
            this.addEdge(edge);
        });

        // Add edge from model into CC
        if (activeDropTarget.fromUID !== null) {
            // We do this to the topmost disconnected input on the CC
            let leafNode = cc.rootVertex;
            while (true) {
                if (this.context.nodeState(this.nodes.vertices[leafNode], "all").nInputs < 1) {
                    // The topmost sequence of nodes ends with a zero-input node. Give up.
                    break; // TODO: do a more elaborate DFS to find a valid input.
                }
                const next = this.nodes.upstreamVertexIndex(leafNode, 0);
                if (next === undefined || cc.vertices.indexOf(next) < 0) {
                    const edge = this.nodes.edges[cc.inputEdges[0]];
                    this.context.addEdge(activeDropTarget.fromUID, this.nodes.vertices[leafNode], 0);
                    break;
                }
                leafNode = next;
            }
        }

        // Add edge from CC back to model
        if (activeDropTarget.toUID !== null && activeDropTarget.toInput !== null) {
            const fromUID = this.nodes.vertices[cc.rootVertex];
            this.context.addEdge(fromUID, activeDropTarget.toUID, activeDropTarget.toInput);
        }

        this.context.flush();
    }

    selectBetween(tile1: VideoNodeTile, tile2: VideoNodeTile) {
        const index1 = this.tiles.vertices.indexOf(tile1);
        const index2 = this.tiles.vertices.indexOf(tile2);
        console.log("Select between", index1, index2);
        if (index1 >= 0 && index2 >= 0) {
            this.tiles.verticesBetween(index1, index2).forEach(tileIndex => {
                this.tiles.vertices[tileIndex].selected = true;
            });
        } else {
            // One or both of the tiles is not present.
            // Select the other one (no harm on calling this on deleted tiles)
            tile1.selected = true;
            tile2.selected = true;
        }
    }

    addNode(nodeState: object) {
        const uid = this.context.addNode(nodeState);
        if (uid === undefined) {
            console.log("Error adding node");
            return;
        }

        // Insert after last clicked tile
        let nodeVertexIndex = -1;
        if (this.lastTile !== undefined) {
            nodeVertexIndex = this.nodes.vertices.indexOf(this.lastTile.uid);
        }
        if (nodeVertexIndex >= 0) {
            const edgesToRemove = this.nodes.downstreamEdges[nodeVertexIndex];
            const edgesToAdd = edgesToRemove.map(edgeIndex => {
                return {
                    fromUID: uid,
                    toUID: this.nodes.vertices[this.nodes.edges[edgeIndex].toVertex],
                    toInput: this.nodes.edges[edgeIndex].toInput,
                };
            });
            edgesToAdd.push({
                fromUID: this.nodes.vertices[nodeVertexIndex],
                toUID: uid,
                toInput: 0,
            });

            edgesToRemove.forEach(edgeIndex => {
                this.removeEdgeByIndex(edgeIndex);
            });
            edgesToAdd.forEach(edge => {
                this.context.addEdge(edge.fromUID, edge.toUID, edge.toInput);
            });
        } // Last interacted tile was deleted, if < 0. Add as orphan in that case.

        this.selectUID = uid;
        this.context.flush();
    }

    adjustSelected(detail: AdjustEventDetail) {
        const newEvent = new CustomEvent("adjust", {detail: detail})
        this.tiles.vertices.forEach(tile => {
            if (tile.selected) {
                tile.dispatchEvent(newEvent);
            }
        });
    }

    onScroll(event: WheelEvent) {
        this.adjustSelected({control: 0, amountRelative: event.deltaY * -0.0003});
    }
}

customElements.define('radiance-videonodetile', VideoNodeTile);
customElements.define('radiance-videonodepreview', VideoNodePreview);
customElements.define('radiance-graph', Graph);
customElements.define('radiance-droptarget', DropTarget);
