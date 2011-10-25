#!/usr/bin/env bash
set -xv
rm -rf blockbuster-install && 	mkdir blockbuster-install
cp -rp ${INSTALL_DIR}/{bin,lib,man,share,doc} blockbuster-install
sed "s:OLD_INSTALL_DIR=.*:OLD_INSTALL_DIR=${INSTALL_DIR}:" install.sh > blockbuster-install/install.sh
chmod 755 blockbuster-install/install.sh
tar -czf blockbuster-install.tgz blockbuster-install
rm -rf blockbuster-install
