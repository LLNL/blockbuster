#!/usr/bin/env bash
set -xv
if [ $(uname) == Linux ]; then 
    rm -rf blockbuster-install && 	mkdir blockbuster-install
    cp -rp ${INSTALL_DIR}/{bin,lib,man,share,doc} README-install.txt blockbuster-install
    sed "s:OLD_INSTALL_DIR=.*:OLD_INSTALL_DIR=${INSTALL_DIR}:" install.sh > blockbuster-install/install.sh
    chmod 755 blockbuster-install/install.sh
    tar -czf blockbuster-install-linux.tgz blockbuster-install
    rm -rf blockbuster-install
    echo "Created blockbuster-install.tgz containing Linux installer in current directory." 
elif [ $(uname) == Darwin ]; then 
    macdeployqt src/blockbuster/blockbuster.app -dmg
    macdeployqt src/blockbuster/sidecar/sidecar.app -dmg
    mv src/blockbuster/blockbuster.dmg src/blockbuster/sidecar/sidecar.dmg ./
    tar -czf blockbuster-install-mac.tgz blockbuster.dmg sidecar.dmg README-install.txt
    echo "Created blockbuster-install.tgz containing blocbuster.dmg and sidecar.dmg in current directory." 
else
    echo "Error: Unrecognized uname results: $(uname).  Nothing done."
    exit 1
fi

exit 0
