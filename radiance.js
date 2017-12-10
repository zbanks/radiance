var Radiance = (function(window, $, _) {
    'use strict';
    var R = new (function() {
        var R = this;
        R.gls = {};
        R.glslSources = {};
        R.MAX_INTENSITY_INTEGRAL = 1024.;
        R.setupWebGLCanvas = function(name, canvas) {
            canvas.width = canvas.height = 256; // Power of 2
            /*
            var scale = 2.5;
            canvas.width = canvas.clientWidth / scale;
            canvas.height = canvas.clientHeight / scale;
            */

            var gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
            if (!gl) {
                console.error("Unable to initialize webgl");
                throw "Unable to initialize webgl";
            }
            R.gls[name] = gl;

            gl.clearColor(0.0, 0.0, 0.0, 0.0);
            gl.clear(gl.COLOR_BUFFER_BIT);
            gl.disable(gl.BLEND);

            var vertex_shader = R.compileGlsl(name, R.glslSources["resources/glsl/plain_vertex.glsl"], gl.VERTEX_SHADER);
            var fragment_shader = R.compileGlsl(name, R.glslSources["resources/glsl/plain_fragment.glsl"], gl.FRAGMENT_SHADER);

            var program = gl.createProgram();
            gl.attachShader(program, vertex_shader);
            gl.attachShader(program, fragment_shader);
            gl.linkProgram(program);
            if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                console.error("unable to link shader: ", gl.getProgramInfoLog(program));
                throw gl.getProgramInfoLog(program);
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

            // TODO: It's bad form to attach additional properties to the `gl` instance
            // The things below here should be moved into a different object
            gl.renderTextureToCanvas = function(texture) {
                gl.disable(gl.BLEND);
                gl.disable(gl.DEPTH_TEST);
                gl.clearColor(0.0, 0.0, 0.0, 0.0);
                gl.clear(gl.COLOR_BUFFER_BIT);

                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, texture);

                gl.useProgram(program);
                var textureLoc = gl.getUniformLocation(program, "iTexture");
                gl.uniform1i(textureLoc, 0);

                var vPositionAttribute = gl.getAttribLocation(program, "vPosition");
                gl.enableVertexAttribArray(vPositionAttribute);

                gl.bindBuffer(gl.ARRAY_BUFFER, vertBuffer);
                gl.vertexAttribPointer(vPositionAttribute, 4, gl.FLOAT, false, 0, 0);

                gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
                gl.useProgram(null);
                gl.activeTexture(gl.TEXTURE0);
            }

            var blankTextureData = new Uint8Array([0, 0, 0, 0]);
            gl.blankTexture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, gl.blankTexture);
            gl.texImage2D(
                gl.TEXTURE_2D,
                0,                      // level
                gl.RGBA,                // internal format
                1,                      // width
                1,                      // height
                0,                      // border
                gl.RGBA,                // format
                gl.UNSIGNED_BYTE,
                blankTextureData);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);

            var noiseTextureArray = [];
            for (var i = 0; i < gl.drawingBufferWidth * gl.drawingBufferHeight * 4; i++) {
                noiseTextureArray.push(Math.floor(Math.random() * 256));
            }
            var noiseTextureData = new Uint8Array(noiseTextureArray);
            gl.noiseTexture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, gl.noiseTexture);
            gl.texImage2D(
                gl.TEXTURE_2D,
                0,                      // level
                gl.RGBA,                // internal format
                gl.drawingBufferWidth,  // width
                gl.drawingBufferHeight, // height
                0,                      // border
                gl.RGBA,                // format
                gl.UNSIGNED_BYTE,
                noiseTextureData);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

            return gl;
        };
        R.compileGlsl = function(context, glslSource, shaderType) {
            var gl = R.gls[context];
            var shader = gl.createShader(shaderType);

            gl.shaderSource(shader, glslSource);
            gl.compileShader(shader);
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                //console.log(glslSource);
                console.error("unable to compile shader: ", gl.getShaderInfoLog(shader));
                throw gl.getShaderInfoLog(shader);
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

            var s = headerSource + "\n#line 1\n";
            _(glslSource)
                .split("\n")
                .forEach(function(line, i) {
                    var terms = _(line).trim().split(" ");
                    if (terms[0] == "#property") {
                        effect.properties[terms[1]] = _(terms).drop(2).join(" ");
                    } else if (terms[0] == "#buffershader") {
                        effect.bufferShaders.push(s);
                        s = headerSource + "\n#line " + (i+2);
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
        R.audio = new (function() {
            var audio = this;

            this.low = 0.3;
            this.mid = 0.2;
            this.high = 0.1;
            this.level = 0.2;
            this.update = function() {
                // Fake VU
                var t = R.time.beatTime();
                var v = (t % 1.0) * 0.9 + 0.1;
                this.low = v * 0.4;
                this.mid = v * 0.3;
                this.high= v * 0.2;
                this.level= v * 0.2 + 0.2;
            };

            return; // TODO: Don't bother grabbing the mic until we actually do something with it
            navigator.mediaDevices
                .getUserMedia({ audio: true, video: false })
                .then(function(stream) {
                    var audioContext = new AudioContext();
                    var source = audioContext.createMediaStreamSource(stream);

                    var analyser = audioContext.createAnalyser();
                    analyser.fftSize = 1024;
                    var freqArray = new Uint8Array(analyser.frequencyBinCount);
                    source.connect(analyser);
                    var binSize = audioContext.sampleRate / analyser.fftSize; // in Hz
                    console.log("Sample rate:", audioContext.sampleRate, "FFT size:", analyser.fftSize, "Bin size:", binSize);

                    audio.update = function() {
                        analyser.getByteFrequencyData(freqArray);
                        // TODO: Analyze audio
                    }
                });
        });
        R.Node = function(context, nodeName) {
            var node = this;
            var effect = this.effect = R.library[nodeName];
            if (effect.type != "effect")
                return;

            this.intensity = 0.7;
            this.intensityIntegral = 0.;
            this.lastIntegrateTime = R.time.beatTime();

            var gl = this.gl = R.gls[context];
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
                };
                fbo.release = function() {
                    gl.releaseFramebuffer(gl.FRAMEBUFFER, fbo.framebuffer);
                };
                fbo.destroy = function() {
                    gl.deleteTexture(fbo.texture);
                    gl.deleteFramebuffer(fbo.framebuffer);
                };

                return fbo;
            };

            var passes = _.map(effect.bufferShaders, function(glslSource) {
                // TODO: Here would be a decent place to cache the getUniformLocation results
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

            this.update = function() {
                var now = R.time.beatTime();
                var tDiff = (now - this.lastIntegrateTime);
                if (tDiff < 0)
                    tDiff += R.time.MAX_BEAT;
                this.intensityIntegral = (this.intensityIntegral + this.intensity * tDiff) % R.MAX_INTENSITY_INTEGRAL;
                this.lastIntegrateTime = now;
            };

            node.destroyed = false;
            this.destroy = function() {
                node.destroyed = true;
                extraFbo.destroy();
                _(passes).forEach(function(pass) {
                    pass.fbo.destroy();
                    gl.deleteProgram(pass.program);
                    gl.deleteBuffer(vertBuffer);
                });
            };

            this.paint = function(inputTextures) {
                if (node.destroyed) throw "cannot paint destroyed node";

                gl.clearColor(0, 0, 0, 0.0);
                gl.disable(gl.DEPTH_TEST);
                //gl.disable(gl.BLEND);

                // Textures:
                //     iNoise
                //     iChannel[]
                //     iInputs[]
                // fooTex is the index where the texture will be bound (e.g. TEXTURE0)
                // fooTexture is the texture reference that can be bound
                var textureIdx = 0;
                var noiseTex = textureIdx++;
                var inputTexs = _.map(_.range(effect.properties.inputCount), function() { return textureIdx++; });
                var channelTexs = _.map(passes, function () { return textureIdx++; });

                var outputTexture;
                _(passes).forEachRight(function(pass, i) {
                    gl.disable(gl.BLEND);
                    gl.disable(gl.DEPTH_TEST);
                    gl.clearColor(0.0, 0.0, 0.0, 0.0);
                    gl.clear(gl.COLOR_BUFFER_BIT);

                    extraFbo.bind();
                    var program = pass.program;
                    gl.useProgram(program);

                    //TODO: Actually popoulate the noise texture
                    //...although it's actually some pretty good noise by default on my machine
                    gl.activeTexture(gl.TEXTURE0 + noiseTex);
                    gl.bindTexture(gl.TEXTURE_2D, gl.noiseTexture);
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
                    gl.uniform1f(iStepLoc, R.time.wallTime());
                    var iTimeLoc = gl.getUniformLocation(program, "iTime");
                    gl.uniform1f(iTimeLoc, R.time.beatTime());
                    var iFPSLoc = gl.getUniformLocation(program, "iFPS");
                    gl.uniform1f(iFPSLoc, 60.);
                    var iAudioLoc = gl.getUniformLocation(program, "iAudio");
                    gl.uniform4f(iAudioLoc, R.audio.low, R.audio.mid, R.audio.high, R.audio.level);
                    var iResolutionLoc = gl.getUniformLocation(program, "iResolution");
                    gl.uniform2f(iResolutionLoc, gl.drawingBufferWidth, gl.drawingBufferHeight);

                    var vPositionAttribute = gl.getAttribLocation(program, "vPosition");
                    gl.enableVertexAttribArray(vPositionAttribute);

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
            this.verticies = {};

            this.nextVertexId = 1;
            this.makeVertexId = function() { return this.nextVertexId++; }; // 0 is 'unconnected'

            this.createVertex = function(nodeName, intensity) {
                var node = new R.Node(context, nodeName);
                node.intensity = intensity;

                var vertex = {};
                vertex.node = node;
                vertex.id = this.makeVertexId();
                vertex.inEdges = {};
                for (var i = 0; i < node.effect.properties.inputCount; i++) { vertex.inEdges[i] = 0; }
                vertex.texture = null;
                vertex.destroy = function() {
                    _(model.verticies).forEach(function(v) {
                        _(v.inEdges).forEach(function(id, idx) {
                            if (id == vertex.id)
                                v.inEdges[idx] = 0;
                        });
                    });
                    model.verticies.pop(vertex.id); 
                    vertex.node.destroy();
                };
                vertex.connect = function(inputNumber, fromVertex) {
                    vertex.inEdges[inputNumber] = fromVertex.id;
                };
                vertex.createChain = function(_nodeName, _intensity) {
                    var next = model.createVertex(_nodeName, _intensity);
                    next.connect(0, vertex);
                    return next;
                };
                vertex.renderToCanvas = function() {
                    // TODO: this smells bad
                    vertex.node.gl.renderTextureToCanvas(vertex.texture);
                };

                this.verticies[vertex.id] = vertex;
                return vertex;
            };

            this.paint = function() {
                _(this.orderedVerticies()).forEach(function(vertex) {
                    vertex.node.update();
                    var inputTextures = [];
                    _(vertex.inEdges).forEach(function (v) {
                        inputTextures.push(v ? model.verticies[v].texture : vertex.node.gl.blankTexture);
                    });
                    vertex.texture = vertex.node.paint(inputTextures);
                });
            };

            this.orderedVerticies = function() {
                // Toposorted `this.verticies`
                // Based on Kahn's algorithm from Wikipedia, but poorly, for brevity
                var output = [];
                var pending = [];
                var marks = {0: true};
                var isMarked = function(id) { return _(marks).get(id, false); };
                while (true) {
                    _(this.verticies).forEach(function(vertex, id) {
                        if (_(marks).has(id))
                            return;
                        if (!_(vertex.inEdges).every(isMarked))
                            return;
                        pending.push(vertex);
                        marks[id] = false;
                    });

                    if (_(pending).isEmpty())
                        break;

                    var v = pending.pop();
                    output.push(v);
                    marks[v.id] = true;
                }
                if (!_(this.verticies).keys().every(isMarked)) {
                    console.log("Cycle detected!"); // oops
                }
                return output;
            };
        };
        R.View = function(model, $el) {
            var view = this; 
            this.$el = $el;
            this.activeVertex = null;
            this.update = function() {
                var $list = $("<table>");
                $list.append($("<tr>")
                    .append($("<th>").text("#"))
                    .append($("<th>").text("inputs"))
                    .append($("<th>").text("name"))
                    .append($("<th>").text("intensity"))
                    .append($("<th>").text("description"))
                );
                _(model.orderedVerticies()).forEach(function(vertex) {
                    $list.append($("<tr>")
                        .append($("<td>").text(vertex.id))
                        .append($("<td>").text(_(vertex.inEdges).values().join(", ")))
                        .append($("<td>").text(vertex.node.effect.name))
                        .append($("<td>").text(vertex.node.intensity))
                        .append($("<td>").text(vertex.node.effect.properties.description))
                        .mouseover(function() {
                            view.activeVertex = vertex;
                        })
                    );
                    view.activeVertex = vertex;
                });
                this.$el.html($list);
            };
            this.render = function() {
                if (this.activeVertex)
                    this.activeVertex.renderToCanvas();
            };
        };
        R.setup = function() {
            var canvas = $(".main-canvas");
            var gl = R.setupWebGLCanvas("main", canvas[0]);
            if (!gl) {
                return;
            }

            var model = R.model = new R.Model("main");
            model
                .createVertex("purple", 1.0)
                .createChain( "fireball", 0.4)
                .createChain( "vu", 0.7)
                //.createChain( "solitaire", 0.7)
                .createChain( "life", 0.6)
                .createChain( "melt", 0.4)
                ;

            var view = R.view = new R.View(model, $(".control"));
            view.update();

            var paint = function(timestamp) {
                R.audio.update();
                R.model.paint();
                R.view.render();
                window.requestAnimationFrame(paint);
            };
            window.requestAnimationFrame(paint);
        };
        R.testShadersCompile = function() {
            $("body").append($("<hr>"));
            _(R.library).forOwn(function(_value, name) {
                try {
                    var node = new R.Node("main", name);
                } catch (e) {
                    $("body")
                        .append($("<h3>").text(name))
                        .append($("<div>").html(e.replace("\n", "<br>")));
                }
            });
        }
    });
    $(R.setup);
    window.loadGlsl = R.loadGlsl; // glsl.js is a JSON blob wrapped in `loadGlsl(...)`
    return R;
})(window, jQuery, _);
