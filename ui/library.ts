import {Graph} from "./graph";

export class Library extends HTMLElement {
    nodeNameInput: HTMLInputElement;
    graph: Graph;

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: flex;
                flex-direction: column;
            }

            #nodeName {
                margin-top: auto;
                width: 100%;
            }
            </style>
            <input type="text" id="nodeName"></input>
        `;
        this.nodeNameInput = shadow.querySelector("#nodeName");
        document.addEventListener('keypress', this.onKeyPress.bind(this));
        this.nodeNameInput.addEventListener('keypress', this.onTextBoxKeyPress.bind(this));
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
        }
    }
}

customElements.define('radiance-library', Library);
