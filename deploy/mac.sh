#!/bin/sh

SOURCE_DIR=$1
BINARY=$2
APP=$3
DMG=$4
QT=$5

echo "Source dir: $SOURCE_DIR"
echo "Binary: $BINARY"
echo "App: $APP"
echo "DMG: $DMG"
echo "Qt: $QT"

echo "Create bundle directory..."
mkdir -p "$APP/Contents/MacOS"

echo "Copying executable and Info.plist..."
cp -r "$BINARY" "$APP/Contents/MacOS/radiance"
cp -r "$SOURCE_DIR/deploy/Info.plist" "$APP/Contents/Info.plist"

echo "Copying resources..."
mkdir -p "$APP/Contents/Resources"
cp -r "$SOURCE_DIR/resources"/* "$APP/Contents/Resources/"

echo "Removing .qmlc files..."
find "$APP/Contents/Resources/" -name "*.qmlc" -exec rm \{\} \;

echo "Running macdeployqt..."
"$QT/bin/macdeployqt" "$APP" "-qmldir=$APP/Contents/Resources/qml"

INCLUDED_IN_BUNDLE=$(find "$APP/Contents/Frameworks/" -name "*.dylib" -exec basename \{\} \; | awk '{print "-e " $1 }')

function replace_dlybs() {
    DYLIBS=$(otool -L "$1" | grep "/usr/local/Cellar" | awk -F' ' '{print $1 }' | grep $INCLUDED_IN_BUNDLE | grep -v "@executable_path")
    if [ "$DYLIBS" ]; then
	    echo "Replacing dylibs in $file..."
	    for dylib in $DYLIBS; do
		install_name_tool -change "$dylib" "@executable_path/../Frameworks/$(basename "$dylib")" "$1"
	    done;
    fi
}

for file in $(ls "$APP"/Contents/Frameworks/*.dylib); do 
    replace_dlybs "$file"
done

echo "Generating icon set..."
"$SOURCE_DIR/deploy/png2icns.sh" "$SOURCE_DIR/deploy/icon.png"
mv icon.icns "$APP/Contents/Resources/"

echo "Bundle is done."

# Create DMG
echo "Creating DMG..."
mkdir -p tmp
cp -r "$APP" tmp
ln -s /Applications tmp/Applications
hdiutil create -volname "Radiance" -srcfolder tmp -ov -format UDZO "$DMG"
rm -rf tmp
