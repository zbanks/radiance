import {Context} from "../pkg/index.js";
import {Graph} from "./graph";

export class Library extends HTMLElement {
    nodeNameInput: HTMLInputElement;
    list: HTMLDivElement;
    graph: Graph;
    context: Context;
    library: object;
    bestMatch: string;

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
        this.nodeNameInput.addEventListener('input', this.onTextBoxInput.bind(this));
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

    renderLibrary() {
        const filterText = this.nodeNameInput.value;

        let filteredLibrary;
        if (filterText == "") {
            filteredLibrary = this.library;
            this.bestMatch = undefined;
        } else {
            filteredLibrary = {};
            for (const k in this.library) {
                const item = this.library[k];
                if ("name" in item && item.name.startsWith(filterText)) {
                    filteredLibrary[k] = item;
                }
            }
            this.bestMatch = Object.keys(filteredLibrary)[0];
        }

        const ul = document.createElement("ul");
        this.list.innerHTML = "";
        this.list.appendChild(ul);
        for (const item in filteredLibrary) {
            const li = document.createElement("li");
            li.textContent = filteredLibrary[item].name;
            li.addEventListener("click", event => {
                this.instantiateFromLibrary(item);
                this.done();
            });
            ul.appendChild(li);
        }
    }

    libraryChanged() {
        this.library = this.context.library();
        this.renderLibrary();
    }

    instantiateFromLibrary(item: string) {
        if (this.graph !== undefined) {
            this.graph.addNode(this.library[item]);
        }
    }

    onKeyPress(event: KeyboardEvent) {
        if (event.code == "Semicolon" && event.shiftKey && event.target != this) {
            this.nodeNameInput.focus();
            event.preventDefault();
        }
    }

    onTextBoxInput(event: InputEvent) {
        this.renderLibrary();
    }

    done() {
        this.nodeNameInput.value = "";
        this.renderLibrary();
        this.blur();
    }

    onTextBoxKeyPress(event: KeyboardEvent) {
        if (event.code == "Enter") {
            if (this.bestMatch !== undefined) {
                this.instantiateFromLibrary(this.bestMatch);
            }
            this.done();
        } else if (event.key == "Escape") {
            this.done();
        }
    }
}

customElements.define('radiance-library', Library);
