#!/usr/bin/env bash
set -v 
logfile=$HOME/slave-$(uname -n).out
echo starting slave >$logfile
exec >>$logfile 2>&1

blockbuster  "$@"
