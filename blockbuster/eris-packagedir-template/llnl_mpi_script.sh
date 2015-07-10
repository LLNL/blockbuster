#!/usr/bin/env bash
set -x
#exec >$HOME/slurm.out 2>&1
#export DISPLAY=:0
# Example script for running blockbuster in MPI mode on LLNL servers. 
# $1 is the slurm job id from running squeue
export SLURM_JOBID="$1"
# $2 should be something like "vertex[1-8]"
export SLURM_JOBNODES=$2
shift 2
logfile=$HOME/.blockbuster/mpiscript.out
exec  >$logfile 2>&1
# presumably this will show up in std out somewhere.
#echo "$(uname -n): SLURM_JOBID=\"$SLURM_JOBID\" and nodes=\"$SLURM_JOBNODES\" and command is: srun -w \"$SLURM_JOBNODES\"" "$@" >$logfile 2>&1

# Crux: using "srun", run $@ on each node, quote nodes because csh hates brackets: 
# srun --jobid $SLURM_JOBID -w "$SLURM_JOBNODES" "$@" -display :0 >$HOME/.blockbuster/slurm.out 2>&1
srun --jobid $SLURM_JOBID -w "$SLURM_JOBNODES" "$@" -display :0 
