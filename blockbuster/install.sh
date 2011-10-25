#!/usr/bin/env bash
OLD_INSTALL_DIR=/usr/gapps/asciviz/blockbuster/dev/chaos_4_x86_64_ib/blockbuster/..

function errexit() {
    echo $1
    exit 1
}

echo "Welcome to the binary installer for blockbuster and sidecar.  You can set INSTALL_DIR or give an directory as an argument, or just type one in if you haven't yet done that." 
INSTALL_DIR=${1:-$INSTALL_DIR}
while [ x$INSTALL_DIR == x ]; do
    echo "Please enter the install directory.  New bin, lib, share, include and man directories will be created there and populated appropriately.  Type Control-C to exit without continuing."
    read INSTALL_DIR
done

mkdir -p $INSTALL_DIR

srcdir=$(dirname $0)
for dir in $srcdir/{bin,lib,man,share,doc}; do
    cp -r $dir $INSTALL_DIR  || errexit "Cannot copy $dir to $INSTALL_DIR"
done

set -xv
# make INSTALL_DIR a full pathname
pushd $INSTALL_DIR 
INSTALL_DIR=$(pwd)
popd

if [ $(uname) == Linux ]; then 
    libs=$(ldd $INSTALL_DIR/bin/blockbuster | grep Qt | awk '{print $3}')
elif [ $(uname) == Darwin ]; then 
    libs=$(otool -L $INSTALL_DIR/bin/blockbuster | grep Qt | awk '{print $1}')
fi

cp -f $libs $INSTALL_DIR/lib

echo "Setting rpath for installed executables." 
if [ $(uname) == Linux ]; then    
    rpath=$(chrpath -l $INSTALL_DIR/bin/blockbuster | awk ' {print $2}' | sed 's/RPATH=//' | sed "s:$OLD_INSTALL_DIR:$INSTALL_DIR:" | sed 's~:[^:]*Trolltech[^:]*:~:~')
    chrpath -r $rpath $INSTALL_DIR/bin/blockbuster    
elif [ $(uname) == Darwin ]; then 
    for lib in $libs; do    
        newlib=$INSTALL_DIR/lib/$(basename $lib)
        if ! install_name_tool -change $lib $newlib $INSTALL_DIR/bin/blockbuster; then 
            echo "install_name_tool returned error status; it's probably OK to ignore it." 
        fi
    done
fi 
