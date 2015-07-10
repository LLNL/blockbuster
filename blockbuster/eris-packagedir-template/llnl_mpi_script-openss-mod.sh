#!/usr/bin/env bash
set -x
#exec >$HOME/slurm.out 2>&1
#export DISPLAY=:0
# Example script for running blockbuster in MPI mode on LLNL servers. 
# Modified for use with OpenSS (Speedshop performance tool)
# Rich Cook, LLNL, 2008-10-28
# $1 is the slurm job id from running squeue
export SLURM_JOBID="$1"
# $2 should be something like "vertex[1-8]"
export SLURM_JOBNODES=$2
shift 2

# presumably this will show up in std out somewhere.
echo "$(uname -n): SLURM_JOBID=\"$SLURM_JOBID\" and nodes=\"$SLURM_JOBNODES\" and command is: srun -w \"$SLURM_JOBNODES\"" "$@" >$HOME/slurm.out 2>&1

# Crux: using "srun", run $@ on each node, quote nodes because csh hates brackets: 
use openss
# Use "ossrun" to collect backend data.  This is a currently undocumented way to use OpenSpeedshop
#Before running ossrun, make sure $OPENSS_RAWDATA_DIR is set to a globally avail spot.  I'm using my home directory here, maybe there is a better place?  
mkdir -p ~/OpenSS-rawdata
rm -rf ~/OpenSS-rawdata/*
export OPENSS_RAWDATA_DIR=~/OpenSS-rawdata

srun -w "$SLURM_JOBNODES" ossrun "$@" >>$HOME/slurm.out 2>&1

# Process the openSS data
ossutil $OPENSS_RAWDATA_DIR
