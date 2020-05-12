import {Context} from "../pkg/index.js";
import {Graph} from "./graph";

export class Library extends HTMLElement {
    nodeNameInput: HTMLInputElement;
    list: HTMLDivElement;
    graph: Graph;
    context: Context;
    library: object;

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: flex;
                flex-direction: column;
            }

            #list {
                display: flex;
                flex: 1;
                margin: 10px;
                overflow: hidden scroll;
            }

            #nodeName {
                width: 100%;
            }
            </style>
            <div id="list" width></div>
            <input type="text" id="nodeName"></input>
        `;
        this.nodeNameInput = shadow.querySelector("#nodeName");
        this.list = shadow.querySelector("#list");
        document.addEventListener('keypress', this.onKeyPress.bind(this));
        this.nodeNameInput.addEventListener('keydown', this.onTextBoxKeyPress.bind(this));
    }

    attachGraph(graph: Graph) {
        if (this.graph !== undefined) {
            throw "Cannot call attachGraph more than once!";
        }

        this.graph = graph;
        this.context = graph.context;
        this.context.onLibraryChanged(this.libraryChanged.bind(this));
        this.libraryChanged();
    }

    libraryChanged() {
        this.library = this.context.library();
        const ul = document.createElement("ul");
        this.list.innerHTML = "";
        this.list.appendChild(ul);
        for (const item in this.library) {
            const li = document.createElement("li");
            li.textContent = this.library[item].name;
            li.addEventListener("click", event => {
                this.instantiateFromLibrary(event, item);
            });
            ul.appendChild(li);
        }
    }

    instantiateFromLibrary(event: MouseEvent, item: string) {
        this.graph.addNode(this.library[item]);
    }

    onKeyPress(event: KeyboardEvent) {
        if (event.code == "Semicolon" && event.shiftKey && event.target != this) {
            this.nodeNameInput.focus();
            event.preventDefault();
        }
    }

    onTextBoxKeyPress(event: KeyboardEvent) {
        if (event.code == "Enter") {
            let nodeName = this.nodeNameInput.value.trim();
            if (nodeName != "" && this.graph !== undefined) {
                this.graph.addNode({nodeType: "EffectNode", name: nodeName});
            }
            this.nodeNameInput.value = "";
            this.blur();
        } else if (event.key == "Escape") {
            this.nodeNameInput.value = "";
            this.blur();
        }
    }
}

customElements.define('radiance-library', Library);
