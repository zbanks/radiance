#!/usr/bin/env python2

import json
import os

def glsl_to_json(directories, output_filename):
    output = {}
    for directory in directories:
        assert os.path.isdir(directory)
        filenames = os.listdir(directory)
        for filename in filenames:
            if not filename.endswith(".glsl"):
                continue
            path = os.path.join(directory, filename)
            if not os.path.isfile(path):
                continue
            with open(path) as f:
                data = f.read()
            output[path] = data

    with open(output_filename, "w") as f:
        f.write("loadGlsl(")
        json.dump(output, f, indent=4)
        f.write(")")

if __name__ == "__main__":
    glsl_to_json(["resources/effects", "resources/glsl"], "glsl.js")
