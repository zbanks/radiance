#!/bin/sh

SOURCE_DIR=$1
BINARY=$2
APP=$3
TARGZ=$4
QT=$5
RESOURCES=$6

echo "Source dir: $SOURCE_DIR"
echo "Binary: $BINARY"
echo "App: $APP"
echo "TARGZ: $TARGZ"
echo "Qt: $QT"
echo "Resources: $RESOURCES"

if [ $RESOURCES != "resources" ]; then
echo "*** ERROR ***" >&2
echo "For a Linux bundle to work, RADIANCE_SYSTEM_RESOURCES must be set to 'resources/'" >&2
echo "Please recompile with cmake -DRADIANCE_SYSTEM_RESOURCES=resources/" >&2
exit 1
fi

echo "Create AppDir directory..."
mkdir -p "$APP"

echo "Copying executable and desktop file..."
cp -r "$BINARY" "$APP/radiance"
cp -r "$SOURCE_DIR/deploy/radiance.desktop" "$APP/radiance.desktop"
cp -r "$SOURCE_DIR/deploy/icon.png" "$APP/radiance.png"

echo "Copying resources..."
mkdir -p "$APP/resources"
cp -r "$SOURCE_DIR/resources"/* "$APP/resources/"

echo "Removing .qmlc files..."
find "$APP/resources/" -name "*.qmlc" -exec rm \{\} \;

echo "Running linuxdeployqt..."
"linuxdeployqt" "$APP/radiance.desktop" "-bundle-non-qt-libs" "-qmldir=$APP/resources/qml" "-qmake=$QT/bin/qmake"

echo "Running linuxdeployqt again..."
"linuxdeployqt" "$APP/radiance.desktop" "-bundle-non-qt-libs" "-qmldir=$APP/resources/qml" "-qmake=$QT/bin/qmake"

echo "AppDir is done."

echo "Making tarball..."
tar -zcf "$TARGZ" -C "$APP/.." "$(basename "$APP")"
