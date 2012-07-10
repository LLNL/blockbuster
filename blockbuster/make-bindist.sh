#!/usr/bin/env bash
set -x
function errexit() {
    echo $1
    exit 1
}
version="$(cat src/config/versionstring.txt)"
if [ $(uname) == Linux ]; then 
    for buildarg in all nodmx; do 
        if [ $buildarg == all ]; then
            export INSTALL_DIR=linux-dmx-v$version-$(uname -r)
        else
            export INSTALL_DIR=linux-basic-nodmx-v$version-$(uname -r)
        fi
        INSTALL_DIR=$(pwd)/$INSTALL_DIR remake.sh $buildarg || errexit "build failed for $INSTALL_DIR"
        cp -fp bindist-src/README-install.txt bindist-src/install.sh ${INSTALL_DIR}
        tar -czf ${INSTALL_DIR}.tgz ${INSTALL_DIR}    
        rm -rf ${INSTALL_DIR}
        echo "Created ${INSTALL_DIR}.tgz containing Linux installer in current directory." 
    done
elif [ $(uname) == Darwin ]; then 
    # make || errexit "make failed"
    cp $INSTALL_DIR/bin/blockbuster.dmg $INSTALL_DIR/bin/sidecar.dmg ./
    tar -czf blockbuster-install-mac-v$version-$(uname -r).tgz blockbuster.dmg sidecar.dmg bindist-src/README-install.txt || errexit "Cannot tar up the files" 
    echo "Created blockbuster-install-mac-v$version.tgz containing blocbuster.dmg and sidecar.dmg in current directory." 
else
    errexit "Error: Unrecognized uname results: $(uname).  Nothing done."
fi

exit 0
