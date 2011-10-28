#!/usr/bin/env bash
set -x
function errexit() {
    echo $1
    exit 1
}

if [ $(uname) == Linux ]; then 
    for buildarg in all nodmx; do 
        if [ $buildarg == all ]; then
            export INSTALL_DIR=linux-dmx
        else
            export INSTALL_DIR=linux-basic-nodmx 
        fi
        INSTALL_DIR=$(pwd)/$INSTALL_DIR remake.sh $buildarg || errexit "build failed for $INSTALL_DIR"
        echo "Setting rpath for installed executables to find Qt libraries." 
        for exe in ${INSTALL_DIR}/bin/blockbuster ${INSTALL_DIR}/bin/sidecar; do 
            rpath='${ORIGIN}/../lib:'$(chrpath -l $exe | awk ' {print $2}' | sed 's/RPATH=//' | sed 's~[^:]*Trolltech[^:]*~:~' | sed "s:$INSTALL_DIR::")
            rpath=$(echo $rpath | sed 's~::~:~g' )
            chrpath -r $rpath $exe 
        done
        cp -fp bindist-src/README-install.txt bindist-src/install.sh ${INSTALL_DIR}
        tar -czf ${INSTALL_DIR}.tgz ${INSTALL_DIR}    
        rm -rf ${INSTALL_DIR}
        echo "Created ${INSTALL_DIR}.tgz containing Linux installer in current directory." 
    done
elif [ $(uname) == Darwin ]; then 
    macdeployqt src/blockbuster/blockbuster.app -dmg
    macdeployqt src/blockbuster/sidecar/sidecar.app -dmg
    mv src/blockbuster/blockbuster.dmg src/blockbuster/sidecar/sidecar.dmg ./
    tar -czf blockbuster-install-mac.tgz blockbuster.dmg sidecar.dmg README-install.txt
    echo "Created blockbuster-install.tgz containing blocbuster.dmg and sidecar.dmg in current directory." 
else
    errexit "Error: Unrecognized uname results: $(uname).  Nothing done."
fi

exit 0
