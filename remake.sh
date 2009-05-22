#!/usr/bin/env bash 
# purpose:  to make sure the blockbuster and sidecar builds are properly up to date, after e.g. removing or adding headers or source files
set -x
rm -f src/blockbuster/blockbuster.pro src/blockbuster/sidecar/Makefile.qt.include

make $@

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
