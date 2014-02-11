#!/usr/bin/env bash
# Example script for running blockbuster in MPI mode on LLNL servers. 
# $1 is the slurm job id from running squeue
export SLURM_JOBID=$1
# $2 should be something like "vertex[1-8]"
export nodes=$2
shift 2

# presumably this will show up in std out somewhere.
echo "$(uname -n): SLURM_JOBID=\"$SLURM_JOBID\" and nodes=\"$nodes\" and command is: srun -w $nodes" "$@" 

# Crux: using "srun", run $@ on each node: 
srun -w $nodes "$@" >$HOME/slurm.out 2>&1
