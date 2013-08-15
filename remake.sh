#!/usr/bin/env bash 
# purpose:  to make sure the blockbuster and sidecar builds are properly up to date, after e.g. removing or adding headers or source files
set -x
echo executing $0 $@
if [ $(uname) == Darwin ]; then 
    rm -rf $INSTALL_DIR/bin/{blockbuster,sidecar}.app src/blockbuster/sidecar/sidecar.app src/blockbuster/blockbuster.app
fi

for dir in src/blockbuster/ src/blockbuster/sidecar/; do 
    pushd $dir; 
    rm -f blockbuster.pro Makefile.qt.include moc_*{cpp,o} *.o ui_*h
    popd
done
export INSTALL_DIR
make $@
make all

exit $?

# END OF SCRIPT 

# ============ OLD STUFF =====================

cleandir=$(dirname $0)/src/blockbuster

if [ x$1 == xsidecar ]; then 
    cleandir=$(dirname $0)/src/blockbuster/sidecar
    shift
fi

pushd $cleandir && make clean
popd

make "$@"
