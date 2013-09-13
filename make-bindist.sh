#!/usr/bin/env bash
set -x
function errexit() {
    echo $1
    exit 1
}
tmpdir=/tmp/$(whoami)
mkdir -p $tmpdir

version="$(cat src/config/versionstring.txt)"

echo yes > src/boost/nolinks

if [ $(uname) == Linux ]; then 
    for buildarg in dmx nodmx; do 
        if [ $buildarg == never ]; then # we do not do DMX any more
            export INSTALL_NAME=linux-dmx-v$version-$(uname -r)
        else
            export INSTALL_NAME=linux-basic-nodmx-v$version-$(uname -r)
        fi
        export INSTALL_DIR=$tmpdir/$INSTALL_NAME
        mkdir -p $INSTALL_DIR
        remake.sh $buildarg || errexit "build failed for $INSTALL_DIR"
        cp -fp bindist-src/README-install.txt bindist-src/install.sh ${INSTALL_DIR}
        tarball=$tmpdir/${INSTALL_NAME}.tgz
        pushd ${INSTALL_DIR}/..
        tar -czf $tarball ${INSTALL_NAME}
        popd
        rm -rf ${INSTALL_DIR}
        mv $tarball .
        echo "Created $(basename ${tarball}) containing Linux installer in current directory." 
    done
elif [ $(uname) == Darwin ]; then 
    # make || errexit "make failed"
    cp $INSTALL_DIR/bin/blockbuster.dmg $INSTALL_DIR/bin/sidecar.dmg ./
    tar -czf blockbuster-install-mac-v$version-$(uname -r).tgz blockbuster.dmg sidecar.dmg bindist-src/README-install.txt || errexit "Cannot tar up the files"    echo "Created blockbuster-install-mac-v$version.tgz containing blocbuster.dmg and sidecar.dmg in current directory." 
else
    errexit "Error: Unrecognized uname results: $(uname).  Nothing done."
fi

rm -f src/boost/nolinks

exit 0
