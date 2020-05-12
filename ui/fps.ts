export class FPSCounter extends HTMLElement {
    shadow: ShadowRoot
    timestamps: number[]
    historyLength: number;

    constructor() {
        super();
        this.timestamps = [];
        this.historyLength = 15;
    }

    connectedCallback() {
        this.shadow = this.attachShadow({mode: 'open'});
        this.shadow.textContent = "FPS: ???";
        window.requestAnimationFrame(this.onFrame.bind(this));
    }

    onFrame(timestamp: number) {
        this.timestamps.push(timestamp);
        if (this.timestamps.length > this.historyLength) {
            this.timestamps = this.timestamps.slice(-this.historyLength);
        }
        const oldest = this.timestamps[0];
        const nFrames = this.timestamps.length - 1;
        if (nFrames > 0) {
            const fps = Math.round(nFrames / (0.001 * (timestamp - oldest)));
            this.shadow.textContent = `FPS: ${fps}`;
        }
        window.requestAnimationFrame(this.onFrame.bind(this));
    }
}

customElements.define('radiance-fps-counter', FPSCounter);
