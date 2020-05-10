export interface Edge {
    fromVertex: number;
    toInput: number;
    toVertex: number;
}

export interface ConnectedComponent {
    vertices: number[];
    rootVertex: number;
    internalEdges: number[];
    inputEdges: number[];
    outputEdges: number[];
}

export class GraphData<T> {
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

    upstreamVertexIndex(vertexIndex: number, input: number): number {
        const edge = this.upstreamEdges[vertexIndex][input];
        if (edge === undefined) {
            return undefined;
        } else {
            return this.edges[edge].fromVertex;
        }
    }

    downstreamVertexIndices(vertexIndex: number): number[] {
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

    // Returns a list of indices representing the vertices between (and including)
    // the two given vertices.
    verticesBetween(vertex1: number, vertex2: number): number[] {
        let result = [];
        const traverse = (path: number[], target: number) => {
            const vertex = path[path.length - 1];
            if (vertex == target) {
                result.push(...path);
            } else {
                this.downstreamVertexIndices(vertex).forEach(downstreamVertexIndex => {
                    if (path.indexOf(downstreamVertexIndex) < 0) { // not visited
                        traverse(path.concat([downstreamVertexIndex]), target);
                    }
                });
            }
        }

        // Not sure which node is up/downstream. Traverse only looks downstream, so try both ways.
        traverse([vertex1], vertex2);
        traverse([vertex2], vertex1);

        // Remove duplicates
        result = result.filter((value, index, self) => self.indexOf(value) == index);

        return result;
    }
}

