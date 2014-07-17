#!/usr/bin/env bash
if [ x$INSTALL_DIR == x ]; then
    echo "ERROR: INSTALL_DIR not set"
    exit 1
fi
set -x

if [ $(uname) == Darwin ]; then 
    :
    #echo "STOPPING BEFORE $0"
    #exit 1
    for name in blockbuster sidecar; do
        appdir="${INSTALL_DIR}/bin/${name}.app"
        exe="$appdir"/Contents/MacOS/$name
        rm -rf "$appdir"/Contents/PlugIns/*
        mkdir -p "$appdir"/Contents/Frameworks/ 
        for file in "${INSTALL_DIR}"/lib/*dylib; do
            cp -f "$file" "$appdir"/Contents/Frameworks/    
            if [ ! -f "$file" ]; then 
                exit 1
            fi
        done
        pushd "${INSTALL_DIR}"/lib/
        for file in *boost*dylib; do 
            original_path=$(otool -L "$exe" | grep $file | awk '{print $1;}')
            if [ "$original_path" != "" ]; then 
                install_name_tool -change "$original_path" "@executable_path/../Frameworks/$file" "$exe"
            fi
            if [ x$(otool -L $file | grep libboost_system.dylib | awk '{print $1;}') == xlibboost_system.dylib ]; then
                install_name_tool -change "libboost_system.dylib" "${INSTALL_DIR}/lib/libboost_system.dylib" "$file"
            fi
            install_name_tool -id "$appdir"/Contents/Frameworks/$file $file
        done
        
       # This does not work: 
        macdeployqt "$appdir" -dmg 
        # otool -L "$exe"
    done
elif [ $(uname) == Linux ]; then 
	set -v
    for exe in ${INSTALL_DIR}/bin/blockbuster ${INSTALL_DIR}/bin/sidecar; do 
        cp -f $(ldd $exe | grep -e Qt | awk '{print $3}') ${INSTALL_DIR}/lib; 
        #rpath=$(echo '$ORIGIN/../lib:$ORIGIN/../../lib:'$(chrpath -l $exe | awk ' {print $2}' | sed 's/RPATH=//' | sed 's~[^:]*Trolltech[^:]*~:~' | sed "s:$INSTALL_DIR::") |   sed 's~::~:~g' ); 
        rpath='$ORIGIN/../lib':"$(${INSTALL_DIR}/bin/patchelf --print-rpath $exe | sed s~[^:]*qt[^:]*:~:~)"
        # chrpath -r $rpath $exe ; 
        ${INSTALL_DIR}/bin/patchelf --set-rpath $rpath $exe
    done
fi
	
