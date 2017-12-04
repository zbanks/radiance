var Radiance = (function(window, $, _) {
    'use strict';

    var R = new (function() {
        var R = this;
        R.gls = {};
        R.glslSources = {};
        R.MAX_INTENSITY_INTEGRAL = 1024.;
        R.setupWebGLCanvas = function(name, canvas) {
            var scale = 1.0;
            canvas.width = canvas.clientWidth / scale;
            canvas.height = canvas.clientHeight / scale;

            var gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
            if (!gl) {
                console.error("Unable to initialize webgl");
                throw "Unable to initialize webgl";
            }
            R.gls[name] = gl;

            gl.clearColor(0.0, 0.0, 0.0, 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT);

            var vertex_shader = R.compileGlsl(name, R.glslSources["resources/glsl/plain_vertex.glsl"], gl.VERTEX_SHADER);
            var fragment_shader = R.compileGlsl(name, R.glslSources["resources/glsl/plain_fragment.glsl"], gl.FRAGMENT_SHADER);

            var program = gl.createProgram();
            gl.attachShader(program, vertex_shader);
            gl.attachShader(program, fragment_shader);
            gl.linkProgram(program);
            if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                console.error("unable to link shader: " + gl.getProgramInfoLog(program));
                throw "fixme";
            }

            var vertBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, vertBuffer);

            var verticies = [
                 1.0,  1.0, 1.0, 1.0,
                -1.0,  1.0, 0.0, 1.0,
                 1.0, -1.0, 1.0, 0.0,
                -1.0, -1.0, 0.0, 0.0
            ];
            gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(verticies), gl.STATIC_DRAW);

            gl.renderTextureToCanvas = function(texture) {
                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                gl.clearColor(0.0, 0.0, 0.0, 1.0);
                gl.clear(gl.COLOR_BUFFER_BIT);

                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, texture);

                gl.useProgram(program);
                var textureLoc = gl.getUniformLocation(program, "iTexture");
                gl.uniform1i(textureLoc, 0);

                var vPositionAttribute = gl.getAttribLocation(program, "vPosition");
                gl.enableVertexAttribArray(vPositionAttribute);

                gl.clear(gl.COLOR_BUFFER_BIT);
                gl.bindBuffer(gl.ARRAY_BUFFER, vertBuffer);
                gl.vertexAttribPointer(vPositionAttribute, 4, gl.FLOAT, false, 0, 0);

                gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
            }

            return gl;
        };
        R.compileGlsl = function(context, glslSource, shaderType) {
            var gl = R.gls[context];
            var shader = gl.createShader(shaderType);

            gl.shaderSource(shader, glslSource);
            gl.compileShader(shader);
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                console.log(glslSource);
                console.error("unable to compile shader: " + gl.getShaderInfoLog(shader));
                return null;
            }
            return shader;
        };
        R.loadGlsl = function(glslSources) {
            console.log("Loaded " + _.size(glslSources) + " GLSL scripts");
            R.glslSources = glslSources;
            R.library = {};
            _(glslSources).forEach(function(glslSource, filename) {
                    if (_(filename).startsWith("resources/effects")) {
                        var name = _(_(filename).split("/").last()).split(".").head();
                        R.library[name] = R.parseRadianceEffect(glslSource, name);
                    }
                });
        };
        R.parseRadianceEffect = function(glslSource, name) {
            // Handle #property and #buffershader
            var effect = {
                name: name,
                type: "effect",
                rawSource: glslSource,
                properties: {       // defaults
                    inputCount: 1,
                    description: name,
                },
                bufferShaders: [],
            };

            var headerSource = 
                _(R.glslSources["resources/glsl/effect_header.glsl"])
                    .split("\n")
                    .tail()
                    .join("\n"); // This is a gross hack to strip "#version 150\n" from the header

            var s = headerSource + "\n#line 0\n";
            _(glslSource)
                .split("\n")
                .forEach(function(line, i) {
                    var terms = _(line).trim().split(" ");
                    if (terms[0] == "#property") {
                        effect.properties[terms[1]] = _(terms).drop(2).join(" ");
                    } else if (terms[0] == "#buffershader") {
                        effect.bufferShaders.push(s);
                        s = headerSource + "\n#line " + (i+1);
                    } else {
                        s += line;
                    }
                    s += "\n";
                });
            effect.bufferShaders.push(s);

            return effect;
        };
        R.time = new (function() {
            var msStart = +(new Date());
            this.MAX_BEAT = this.MAX_WALL = 16.;
            this.wallTime = function() {
                var t = ((new Date()) - msStart) / 1000.;
                return t % this.MAX_WALL;
            };
            this.beatTime = function() {
                return ((new Date()) - msStart) / 1000. * (140 / 60);
                return t % this.MAX_BEAT;
            };
        });
        R.Node = function(context, nodeName) {
            var node = this;
            var effect = this.effect = R.library[nodeName];
            if (effect.type != "effect")
                return;

            this.intensity = 0.7;
            this.intensityIntegral = 0.;
            this.lastIntegrateTime = R.time.beatTime();

            var gl = R.gls[context];
            var makeShaderProgram = function(glslSource) {
                var vertex_shader = R.compileGlsl(context, R.glslSources["resources/glsl/plain_vertex.glsl"], gl.VERTEX_SHADER);
                var fragment_shader = R.compileGlsl(context, glslSource, gl.FRAGMENT_SHADER);

                var program = gl.createProgram();
                gl.attachShader(program, vertex_shader);
                gl.attachShader(program, fragment_shader);
                gl.linkProgram(program);
                if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                    console.error("unable to link shader: " + gl.getProgramInfoLog(program));
                    throw "fixme";
                }

                return program;
            };

            var makeTexture = function() {
                var texture = gl.createTexture();
                gl.bindTexture(gl.TEXTURE_2D, texture);
                gl.texImage2D(
                    gl.TEXTURE_2D,
                    0,                      // level
                    gl.RGBA,                // internal format
                    gl.drawingBufferWidth,  // width
                    gl.drawingBufferHeight, // height
                    0,                      // border
                    gl.RGBA,                // format
                    gl.UNSIGNED_BYTE, null);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

                return texture;
            };

            var makeFbo = function() {
                var fbo = {};
                fbo.texture = makeTexture();

                fbo.framebuffer = gl.createFramebuffer();
                gl.bindFramebuffer(gl.FRAMEBUFFER, fbo.framebuffer);
                gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, fbo.texture, 0);

                fbo.bind = function() {
                    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo.framebuffer);
                }
                fbo.release = function() {
                    gl.releaseFramebuffer(gl.FRAMEBUFFER, fbo.framebuffer);
                }

                return fbo;
            };

            var passes = _.map(effect.bufferShaders, function(glslSource) {
                return {
                    program: makeShaderProgram(glslSource),
                    fbo: makeFbo(),
                };
            });
            var extraFbo = makeFbo();

            var vertBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, vertBuffer);

            var verticies = [
                 1.0,  1.0, 1.0, 1.0,
                -1.0,  1.0, 0.0, 1.0,
                 1.0, -1.0, 1.0, 0.0,
                -1.0, -1.0, 0.0, 0.0
            ];
            gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(verticies), gl.STATIC_DRAW);

            var noiseTexture = makeTexture();

            this.update = function() {
                var now = R.time.beatTime();
                var tDiff = (now - this.lastIntegrateTime);
                if (tDiff < 0)
                    tDiff += R.time.MAX_BEAT;
                this.intensityIntegral = (this.intensityIntegral + this.intensity * tDiff) % R.MAX_INTENSITY_INTEGRAL;
                this.lastIntegrateTime = now;
            };

            this.paint = function(inputTextures) {
                
                gl.clearColor(0, 0, 0, 0);
                gl.disable(gl.DEPTH_TEST);
                gl.disable(gl.BLEND);

                // Textures:
                //     iNoise
                //     iChannel[]
                //     iInputs[]
                // fooTex is the index where the texture will be bound (e.g. TEXTURE0)
                // fooTexture is the texture reference that can be bound
                var textureIdx = 0;
                var noiseTex = textureIdx++;
                var inputTexs = _.range(effect.properties.inputCount).map(function() { return textureIdx++; });
                var channelTexs = _.map(passes, function () { return textureIdx++; });

                var outputTexture;
                _(passes).forEachRight(function(pass, i) {
                    extraFbo.bind();
                    var program = pass.program;
                    gl.useProgram(program);

                    //TODO: noise texture
                    gl.activeTexture(gl.TEXTURE0 + noiseTex);
                    gl.bindTexture(gl.TEXTURE_2D, noiseTexture);
                    var iNoiseLoc = gl.getUniformLocation(program, "iNoise");
                    gl.uniform1i(iNoiseLoc, noiseTex);

                    _(inputTextures).forEach(function(inputTexture, i) {
                        if (inputTexture === null)
                            return;
                        gl.activeTexture(gl.TEXTURE0 + inputTexs[i]);
                        gl.bindTexture(gl.TEXTURE_2D, inputTexture);
                        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
                        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
                    });
                    var iInputsLoc = gl.getUniformLocation(program, "iInputs");
                    gl.uniform1iv(iInputsLoc, new Int32Array(inputTexs));

                    _(passes).forEach(function(pp, i) {
                        gl.activeTexture(gl.TEXTURE0 + channelTexs[i]);
                        gl.bindTexture(gl.TEXTURE_2D, pp.fbo.texture);
                        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
                        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
                    });
                    var iChannelsLoc = gl.getUniformLocation(program, "iChannel");
                    gl.uniform1iv(iChannelsLoc, new Int32Array(channelTexs));

                    var iIntensityLoc = gl.getUniformLocation(program, "iIntensity");
                    gl.uniform1f(iIntensityLoc, node.intensity);
                    var iIntensityIntegralLoc = gl.getUniformLocation(program, "iIntensityIntegral");
                    gl.uniform1f(iIntensityIntegralLoc, node.intensityIntegral);
                    var iStepLoc = gl.getUniformLocation(program, "iStep");
                    gl.uniform1f(iStepLoc, 1.0);
                    var iTimeLoc = gl.getUniformLocation(program, "iTime");
                    gl.uniform1f(iTimeLoc, R.time.beatTime());
                    var iFPSLoc = gl.getUniformLocation(program, "iFPS");
                    gl.uniform1f(iFPSLoc, 1.0);
                    var iAudioLoc = gl.getUniformLocation(program, "iAudio");
                    gl.uniform4f(iAudioLoc, 0.3, 0.2, 0.1, 0.2);
                    var iResolutionLoc = gl.getUniformLocation(program, "iResolution");
                    gl.uniform2f(iResolutionLoc, gl.drawingBufferWidth, gl.drawingBufferHeight);

                    var vPositionAttribute = gl.getAttribLocation(program, "vPosition");
                    gl.enableVertexAttribArray(vPositionAttribute);

                    gl.clear(gl.COLOR_BUFFER_BIT);
                    gl.bindBuffer(gl.ARRAY_BUFFER, vertBuffer);
                    gl.vertexAttribPointer(vPositionAttribute, 4, gl.FLOAT, false, 0, 0);

                    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

                    gl.useProgram(null);
                    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                    gl.activeTexture(gl.TEXTURE0); // apparently this is important to reset the scene graph?

                    // Swap
                    outputTexture = extraFbo.texture;
                    var swap = pass.fbo;
                    pass.fbo = extraFbo;
                    extraFbo = swap;

                });
                return outputTexture;
            };
        };
        R.Model = function(context) {
            var model = this;
            // For now, a model is a linear list of nodes with 1 input connected in series
            this.nodes = [];
            this.createAndAppendNode = function(nodeName, intensity) {
                var node = new R.Node(context, nodeName);
                var inputCount = node.effect.properties.inputCount;
                if (inputCount != 1)
                    console.warn("Warn: node has input count > 1", nodeName, inputCount);
                node.intensity = intensity;
                this.nodes.push(node);
                return node;
            };
            this.paint = function() {
                var t = null;
                _(this.nodes).forEach(function(node) {
                    node.update();
                    t = node.paint([t]);
                });
                return t;
            }
        };
        R.View = function(model, $el) {
            var view = this; 
            this.$el = $el;
            this.render = function() {
                var $list = $("<table>");
                $list.append($("<tr>")
                    .append($("<th>").text("#"))
                    .append($("<th>").text("name"))
                    .append($("<th>").text("intensity"))
                    .append($("<th>").text("description"))
                );
                _(model.nodes).forEach(function(node, i) {
                    $list.append($("<tr>")
                        .append($("<td>").text(i + 1))
                        .append($("<td>").text(node.effect.name))
                        .append($("<td>").text(node.intensity))
                        .append($("<td>").text(node.effect.properties.description))
                    );
                });
                this.$el.html($list);
            };
        };
        R.setup = function() {
            var canvas = $(".main-canvas");
            var gl = R.setupWebGLCanvas("main", canvas[0]);
            if (!gl) {
                return;
            }
            window.gl = gl; // XXX for debugging

            var model = R.model = new R.Model("main");
            model.createAndAppendNode("purple", 1.0);
            model.createAndAppendNode("vu", 0.7);
            model.createAndAppendNode("rainbow", 0.4);
            model.createAndAppendNode("test", 0.7);

            var view = R.view = new R.View(model, $(".control"));
            view.render();

            var paint = function(timestamp) {
                var t = model.paint();
                gl.renderTextureToCanvas(t);
                window.requestAnimationFrame(paint);
            };
            window.requestAnimationFrame(paint);
        };
    });
    $(R.setup);
    window.loadGlsl = R.loadGlsl; // glsl.js is a JSON blob wrapped in `loadGlsl(...)`
    return R;
})(window, jQuery, _);
