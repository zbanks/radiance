#!/bin/sh

SOURCE_DIR=$1
APP=$2
QT=$3

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
