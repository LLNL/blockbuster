#!/usr/bin/env bash
if [ x$INSTALL_DIR == x ]; then
    echo "ERROR: INSTALL_DIR not set"
    exit 1
fi
set -x

if [ $(uname) == Darwin ]; then 
    #echo "STOPPING BEFORE $0"
    #exit 1
    for app in blockbuster.app sidecar.app; do
        rm -rf ${INSTALL_DIR}/bin/${app}/Contents/PlugIns/*
        cp -f ${INSTALL_DIR}/lib/*dylib ${INSTALL_DIR}/bin/$app/Contents/Frameworks/            
        macdeployqt ${INSTALL_DIR}/bin/$app -dmg 
    done
elif [ $(uname) == Linux ]; then 
    for exe in ${INSTALL_DIR}/bin/blockbuster ${INSTALL_DIR}/bin/sidecar; do 
        cp -f $(ldd $exe | grep -e Qt -e mpi | awk '{print $3}') ${INSTALL_DIR}/lib; 
        #rpath=$(echo '$ORIGIN/../lib:$ORIGIN/../../lib:'$(chrpath -l $exe | awk ' {print $2}' | sed 's/RPATH=//' | sed 's~[^:]*Trolltech[^:]*~:~' | sed "s:$INSTALL_DIR::") |   sed 's~::~:~g' ); 
        rpath='$ORIGIN/../lib'
        # chrpath -r $rpath $exe ; 
        ${INSTALL_DIR}/bin/patchelf --set-rpath $rpath $exe
    done
fi
	
