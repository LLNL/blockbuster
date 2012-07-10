#!/usr/bin/env bash
# Linux installer for blockbuster and sidecar.  
OLD_INSTALL_DIR=/usr/gapps/asciviz/blockbuster/dev/chaos_4_x86_64_ib/blockbuster/..

function errexit() {
    echo $1
    exit 1
}

function runecho() {
    echo "$@";
    "$@"
}

echo "Welcome to the linux binary installer for blockbuster and sidecar.  You can set INSTALL_DIR or give an directory as an argument, or just type one in if you haven't yet done that." 
INSTALL_DIR=${1:-$INSTALL_DIR}
while [ x$INSTALL_DIR == x ]; do
    echo "Please enter the install directory.  New bin, lib, share, include and man directories will be created there and populated appropriately.  Type Control-C to exit without continuing."
    read INSTALL_DIR
done
echo "Installing... please wait." 
runecho mkdir -p $INSTALL_DIR

srcdir=$(dirname $0)
for dir in $srcdir/{bin,doc,lib,man}; do    
    runecho cp -r $dir $INSTALL_DIR  || errexit "Cannot copy $dir to $INSTALL_DIR"
done

#set -xv
# make INSTALL_DIR a full pathname
#pushd $INSTALL_DIR 
#INSTALL_DIR=$(pwd)
#popd

#libs=$(ldd $INSTALL_DIR/bin/blockbuster | grep Qt | awk '{print $3}')

#cp -f $libs $INSTALL_DIR/lib

#echo "Setting rpath for installed executables to find Qt libraries." 
#for exe in $INSTALL_DIR/bin/blockbuster $INSTALL_DIR/bin/sidecar; do
#    rpath=$(chrpath -l $exe | awk ' {print $2}' | sed 's/RPATH=//' | sed 's~:[^:]*Trolltech[^:]*:~:~' | sed "s:$OLD_INSTALL_DIR:$INSTALL_DIR:" )
#    chrpath -r $rpath $exe 
#done
 
echo "Installation completed.  You might want to add $INSTALL_DIR/bin to your PATH environment variable if you are on Linux."
