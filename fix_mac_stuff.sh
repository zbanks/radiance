#!/bin/bash

/usr/local/Cellar/qt/5.9.2/bin/macdeployqt radiance.app/ -qmldir=radiance.app/Contents/Resources/qml

function replace_dlybs() {
    DYLIBS=`otool -L $1 | grep "/usr/local/Cellar" | awk -F' ' '{print \$1 }'` 
    for dylib in $DYLIBS; do install_name_tool -change $dylib @executable_path/../Frameworks/`basename $dylib` $1; done;
}

for file in `ls radiance.app/Contents/Frameworks/*.dylib`; do 
    echo replacing dylibs for $file
    replace_dlybs $file
done
