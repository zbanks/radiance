export class Drawer extends HTMLElement {
    _closed: boolean;
    handle: HTMLElement;

    constructor() {
        super();
        this._closed = false;
    }

    connectedCallback() {
        const shadow = this.attachShadow({mode: 'open'});
        shadow.innerHTML = `
            <style>
            :host {
                display: inline-block;
                overflow: hidden;
                pointer-events: none;
            }

            #outer {
                position: static;
                display: inline-block;
                width: 100%;
                height: 100%;
                will-change: transform;
                transition: transform 0.5s;
                pointer-events: all;
                background-color: #333333EE; /* TODO expose this outside of this element */
            }

            #inner {
                width: 100%;
                height: 100%;
                transition: opacity 0.5s;
                padding: 10px;
                padding-right: 30px;
                box-sizing: border-box;
            }

            #handle {
                position: absolute;
                right: 0px;
                width: 20px;
                top: 0px;
                bottom: 0px;
                background-color: white;
            }

            :host([closed]) #outer {
                transform: translate(calc(-100% + 10px), 0px);
            }
            </style>
            <div id="outer">
                <div id="inner">
                    <slot></slot>
                </div>
                <div id="handle">
            </div>
            </div>
        `;

        this.handle = shadow.querySelector("#handle");
        this.handle.addEventListener('click', this.onClick.bind(this));

        this._closed = this.hasAttribute("closed");

        Array.from(this.children).forEach(child => {
            child.addEventListener("focus", this.checkChildFocus.bind(this));
            child.addEventListener("blur", this.checkChildFocus.bind(this));
        });
    }

    onClick(event: Event) {
        this.closed = !this.closed;
    }

    get closed() {
        return this._closed;
    }

    set closed(value: boolean) {
        if (value && !this._closed) {
            this.setAttribute("closed", "true");
        } else if (!value && this._closed) {
            this.removeAttribute("closed");
        }
        this._closed = value;
    }

    checkChildFocus(event: Event) {
        const focus = Array.from(this.children).reduce((acc, child) => (acc || child == document.activeElement), false);
        this.closed = !focus;
    }
}

customElements.define('radiance-drawer', Drawer);
