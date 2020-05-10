import {VideoNodeTile} from "./graph";

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

        this.addEventListener("wheel", this.onScroll.bind(this));
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

    intensitySliderChanged() {
        if (this.intensitySliderBlocked) {
            return;
        }
        const newIntensity = parseFloat(this.intensitySlider.value);
        let state = this.graph.context.nodeState(this.uid, "local"); // XXX backend can do sparse updates?
        state.intensity = newIntensity;
        this.graph.context.setNodeState(this.uid, state);
    }

    onScroll(event: WheelEvent) {
        this.intensitySlider.value = (parseFloat(this.intensitySlider.value) + event.deltaY * -0.0003).toString();
        this.intensitySliderChanged();
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

customElements.define('radiance-effectnodetile', EffectNodeTile);
customElements.define('radiance-medianodetile', MediaNodeTile);
