#!/usr/bin/env python2

def scale_layout(new_size, inf, outf):
    for line in inf:
        outf.write(line)
        if line.strip().lower() == "[window]":
            break

    old_w = None
    old_h = None
    new_w, new_h = new_size

    for line in inf:
        if "=" in line:
            prop, _eq, val = line.partition("=")
            if prop.strip().lower() == "w":
                old_w = int(val.strip())
                outf.write("w=%d\n" % new_size[0])
                continue
            elif prop.strip().lower() == "h":
                old_h = int(val.strip())
                outf.write("h=%d\n" % new_size[1])
                continue
        outf.write(line)
        if line.strip().startswith("["):
            break

    if old_w is None or old_h is None:
        raise Exception

    scale_w = new_w / float(old_w)
    scale_h = new_h / float(old_h)

    for line in inf:
        if "=" in line:
            prop, _eq, val = line.partition("=")
            prop = prop.strip().lower()
            if prop in {"x", "w", "px"}  or prop.endswith("_x") or prop.endswith("_w") or prop.endswith("_px"):
                val = int(int(val.strip()) * scale_w)
                outf.write("%s=%d\n" % (prop, val))
                continue
            elif prop in {"y", "h", "py"}  or prop.endswith("_y") or prop.endswith("_h") or prop.endswith("_py") or prop.endswith("_size"):
                val = int(int(val.strip()) * scale_h)
                outf.write("%s=%d\n" % (prop, val))
                continue
        outf.write(line)


if __name__ == "__main__":
    import sys
    scale_layout((1920 - 2, 1080 - 2), sys.stdin, sys.stdout)
