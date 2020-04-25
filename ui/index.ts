"use strict";

class Graph extends HTMLElement {
    constructor() {
        super();
        let shadow = this.attachShadow({mode: 'open'});
        let content = document.createElement('div');
        content.textContent = 'Hello, radiance';
        shadow.appendChild(content);
    }
}

customElements.define('radiance-graph', Graph);
