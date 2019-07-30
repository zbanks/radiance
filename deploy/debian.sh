#!/bin/bash

set -e

if [ -z "$1" ]; then
    echo "You must specify the GIT root directory as the first argument" >&2
    exit 1
fi

if [ -z "$2" ]; then
    echo "You must specify the build directory as the second argument" >&2
    exit 1
fi

if [ -e "$2" ]; then
    echo "Build directory must not exist (it will be created)" >&2
    exit 1
fi

mkdir -p "$2"

SOURCE_DIR=$(readlink -e "$1")
BUILD_DIR=$(readlink -e "$2")

echo "Source dir: $SOURCE_DIR"
echo "Build dir: $BUILD_DIR"

UPSTREAM_VERSION=0.6.1
PKG_VERSION=1

set -x

git -C "$SOURCE_DIR" archive HEAD -o "$BUILD_DIR/radiance.tar.gz"
git -C "$SOURCE_DIR/BTrack" archive HEAD -o "$BUILD_DIR/btrack.tar.gz"
mkdir -p "$BUILD_DIR/radiance_$UPSTREAM_VERSION"
tar -C "$BUILD_DIR/radiance_$UPSTREAM_VERSION" -xf "$BUILD_DIR/radiance.tar.gz"
tar -C "$BUILD_DIR/radiance_$UPSTREAM_VERSION/BTrack" -xf "$BUILD_DIR/btrack.tar.gz"
tar -C "$BUILD_DIR" -zcf "$BUILD_DIR/radiance_$UPSTREAM_VERSION.orig.tar.gz" "radiance_$UPSTREAM_VERSION"
rm "$BUILD_DIR/radiance.tar.gz" "$BUILD_DIR/btrack.tar.gz"
cp -r "$SOURCE_DIR/deploy/debian" "$BUILD_DIR/radiance_$UPSTREAM_VERSION/debian"
cat <<EOF >"$BUILD_DIR/radiance_$UPSTREAM_VERSION/debian/changelog"
radiance ($UPSTREAM_VERSION-$PKG_VERSION) unstable; urgency=medium

  * Automatically packaged for Debian

 -- Eric Van Albert <eric@van.al>  $(date -R)
EOF

(cd "$BUILD_DIR/radiance_$UPSTREAM_VERSION" && debuild -i -us -uc -S)
(cd "$BUILD_DIR/radiance_$UPSTREAM_VERSION" && debuild -i -us -uc -b)
