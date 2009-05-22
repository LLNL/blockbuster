#!/usr/bin/env bash
# This script launches blockbuster proper, unless you happen to be running on sphere12 and there is a file called $HOME/.debugdmx, in which case blockbuster is run under totalview.
testfile=$HOME/.debugdmx
logfile=$(dirname $0)/dmx_debug.log_$(uname -n)
if [ -f $testfile ] && ( [ $(uname -n) == sphere13 ] || [ $(uname -n) == sphere14 ] ); then
    echo FOUND file $testfile on node $(uname -n)
    echo DMX DEBUG ENABLED for node $(uname -n)
    echo sending debug information to $logfile 
    set -vx   
    exec >$logfile 2>&1
    export DISPLAY=sphere10:0
    totalview="totalview"
    tvarg=-a
else
    totalview=
    tvarg=
fi

$totalview $(dirname $0)/blockbuster $tvarg $*
