#!/usr/bin/env bash
# Rich Cook, LLNL, 2008-10-28
shopt -s extglob

function contains ()   { 
   [ "${1/$2}" != "$1" ]
}

runecho() {
    echo "$@"
    "$@"
}
IMG_TRACK=${IMG_TRACK:-/usr/local/tools/imgtrack-1.0/bin/imgtrack}
if [ -f $IMG_TRACK ]; then 
# optionally trap signals:
    trap "$IMG_TRACK"' ABORT blockbuster-VERSION $0 "$@"' 1 2 9
# make the first log entry:
    $IMG_TRACK BEGIN blockbuster-VERSION $0 "$@"
fi

mkdir -p ~/.blockbuster
threadarg=""
screenlockarg=""
if [ $(uname -n) == rzbeast ] && contains "$0" "bin/blockbuster"; then 
    screenlockarg="--no-screensaver"
fi

if [ $(whoami) == "rcook" ] && [ $(uname -n) == stagg1 ]; then
    export DISPLAY=stagg0:0   
    debugger=$(which totalview)
fi
    
export SIDECAR_GLOBAL_HOST_PROFILE=/usr/gapps/asciviz/blockbuster/LLNL-site/hostProfiles.cnf
tvarg=
SS_experiment=
CONSOLE=${CONSOLE:-true}
GPROF=false
help=false
#exec >$HOME/.blockbuster/wrapperout.txt
stop=false
function script_usage() {
    echo '*******************************************************'
    echo "You are using the LLNL blockbuster and smtool wrapper script. "
    echo "This script understands the following options; all others are passed through to $(basename $0) in the appropriate version directory."
    echo
    echo "--debug: turn on -xv bash verbosity in wrapper script"
    echo "--dev:  run the development version of $(basename $0)"
    echo "--dmx:  try to detect DMX display, not just use DISPLAY"
    echo "--gdb:  run blockbuster using gdb -- you will have to supply the arguments within gdb for runtime, e.g. 'run args'"
    echo "--gprof:  run blockbuster using gprof-preload for thread profiling (requires that you compiled with gprof)"
    echo "--helgrind:  run blockbuster using helgrind"
    echo "--speedshop experiment:  run blockbuster using speedshop using the given experiment type"
    echo "--strace :  run blockbuster using strace -ttt"
    echo "--strace-arg args:  run blockbuster using strace with args given (use quotes for multiple args).  Suggestion: -ttt -e trace=network (see DeveloperNotes.txt for some ideas, along with manpage)"
    echo "--totalview:  run blockbuster using totalview"
    echo '*******************************************************'
    echo
}

while true; do 
    argbase="${1##+(-)}"  # strips all leading '-' chars from arg
                          # e.g., changes "--this-thing" to "this-thing"
    if [[ "$argbase" == "debug" ]]; then
        set -xv         
    elif [[ "$argbase" == "dev" ]]; then 
        exe=/usr/gapps/asciviz/blockbuster/dev/${SYS_TYPE}/bin/$(basename $0)
    elif [[ "$argbase" == "dmx" ]]; then
        CONSOLE=false
    elif [[ "$argbase" == "gdb" ]] ; then
        debugger=gdb
    elif [[ "$argbase"  == "gprof" ]] ; then
        GPROF=true
    elif [[ "$argbase" == "helgrind" ]]; then
        debugger="valgrind-3.3.0 --tool=helgrind"
    elif [[ "$argbase" == "h" ]]; then
        # shift # do not shift, let blockbuster or sidecar see -help
        script_usage      
        CONSOLE=true
        break
    elif [[ "$argbase" == "speedshop" ]] ; then
        debugger="openss -offline -f"
        SS_experiment="$2"
        shift
    elif [[ "$argbase" == "strace" ]]; then
        # see developer notes 
        debugger="strace -ttt"
    elif [[ "$argbase" == "strace-args" ]]; then
        debugger="strace $2"
        shift 
    elif [[ "$argbase" == "totalview" ]]; then
        debugger=$(which totalview)
        tvarg="-a"
    else
        break
    fi
    shift 
done

exe=${exe:-"PREFIX/bin/$(basename ${0%-VERSION}).real"}

if "$GPROF"; then
    export LD_PRELOAD=PREFIX/lib/gprof-helper.so
fi

# see if we can detect SLURM; set variables if so 
myhost=$(uname -n | sed 's/[0-9]*//g')
#echo '$0 is' $0
# only check for blockbuster right now, sidecar to follow...
#if echo $0 | grep -e "bin/blockbuster" -e "bin/sidecar"; then 
if [ x$CONSOLE == xfalse ] && [ $(basename $0) == "blockbuster" ]; then 
    echo "blockbuster detected.  Checking for SLURM..."
    mpiarg=
    HAVE_SLURM=false
    SLURM_JOBLINE=$(squeue | grep $(whoami) | grep $myhost | grep dmx)
    if [ ! -z  "$SLURM_JOBLINE" ] ; then 
        SLURM_JOBID=$(echo $SLURM_JOBLINE | awk '{print $1}' )
        SLURM_JOBNODES="$(echo $SLURM_JOBLINE | awk '{print $8}' )"
        DMX_DISPLAY=$(echo $SLURM_JOBLINE | awk '{print $3}' | sed 's/dmx//') 
        if [ -z $SLURM_JOBID ] || [ -z $SLURM_JOBNODES ]; then
            echo "$0: You have a SLURM batch job on this host, but I could not determine SLURM_JOBID and SLURM_JOBNODES-- please contact Rich Cook at 3-9605 about this error."
            exit 1
        fi
        echo "found SLURM_JOBID $SLURM_JOBID and SLURM_JOBNODES $SLURM_JOBNODES"    
        export BLOCKBUSTER_MPI_SCRIPT=${BLOCKBUSTER_MPI_SCRIPT:-"/usr/gapps/asciviz/blockbuster/bin/llnl_mpi_script.sh"}
        export BLOCKBUSTER_MPI_SCRIPT_ARGS=${BLOCKBUSTER_MPI_SCRIPT_ARGS:-"$SLURM_JOBID '$SLURM_JOBNODES'"}
        if echo "$@" | grep -v -- -mpi >/dev/null; then
            mpiarg=-mpi
        fi
        HAVE_SLURM=true
    else
        echo "SLURM is not running, so we CANNOT USE the -mpi flag."
    fi
    
    # Now see if we can detect DMX running, which might not be SLURM-based
    if [ $HAVE_SLURM == false ] &&  ls ~/.TelepathLite/TelepathLite_dmx_config.$myhost.*>/dev/null 2>&1; then 
        echo "Found TelepathLite config file.  Parsing... "
        DMX_DISPLAY_FROM_CONFIG=$(grep DMX_DISPLAY ~/.TelepathLite/TelepathLite_dmx_config.$myhost.* | awk '{print $3}')
        if [ -z "$DMX_DISPLAY_FROM_CONFIG" ]; then 
            echo "Could not get DMX_DISPLAY from config file."
        else 
            if ! ls /tmp/.X${DMX_DISPLAY_FROM_CONFIG}-lock >/dev/null 2>&1; then
                echo "Cannot find DMX server lock file on this host."
            else
                echo "have lockfile and dmx display from config file.  Checking for backend X servers..."
                numbackends=$(grep 'Venue size' ~/.TelepathLite/TelepathLite_dmx_config.$myhost.* | sed -e 's/(//g' -e 's/,//g' | awk '{print $5}')
                echo "Expecting $numbackends X servers..."
                numdetected=$((pdsh -a ps -ef) | grep xinit | grep -v grep | grep -v srun | grep -v bash | grep sleep | grep -v pdsh | grep $(whoami) | grep -v tcsh | wc | awk '{print $1}')
                echo "Detected $numdetected X servers..."
                if [ x"$numbackends" == x"$numdetected" ]; then 
                    DMX_DISPLAY=":"$DMX_DISPLAY_FROM_CONFIG                
                    echo "DMX is running.  DMX_DISPLAY is $DMX_DISPLAY"
                fi
            fi
        fi
    fi # checking for non-SLURM DMX
    # If we're going to run blockbuster without debugger and have DMX, set DISPLAY for the user to DMX_DISPLAY
    dmxarg=
    if [ ! -z $DMX_DISPLAY  ] &&  [ "${0/blockbuster/}" != "$0" ]; then 
        echo "Blockbuster with DMX:"
        if  [ -z "$debugger" ]; then 
            echo "setting DISPLAY to DMX_DISPLAY $DMX_DISPLAY"
            export DISPLAY=$DMX_DISPLAY
        fi
        if echo "$@" | grep -v dmx >/dev/null; then
            dmxarg=" -r dmx"
        fi
    fi # setting DISPLAY 
    
    # setup sidecar default profile for DMX on rzthriller: 
    if [ x$DMX_DISPLAY != x ]; then 
        if  uname -n | grep rzthriller6 2>&1  ; then 
            export SIDECAR_DEFAULT_PROFILE=${SIDECAR_DEFAULT_PROFILE:-"rzthriller-dmx"}
        fi
    fi
fi # sidecar or blockbuster are running... 
 
mkdir -p ~/.blockbuster/$(uname -n)

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:PREFIX/lib

if [ "${debugger:0:6}" == openss ]; then 
    if ! use openss; then 
        echo "Error: cannot find OpenSpeedshop dotkit module:"
        exit 1
    fi
fi

#echo executing $debugger PREFIX/bin/$(basename $0) $tvarg $threadarg $screenlockarg $mpiarg $dmxarg "$@"  $SS_experiment 
$debugger $exe $tvarg $threadarg $screenlockarg $mpiarg $dmxarg "$@"  $SS_experiment 

_value=$?

if [ -f $IMG_TRACK ]; then 
#make the final entry in the log:
    $IMG_TRACK END blockbuster-links $0 "$@"
fi

exit $_value



