#!/usr/bin/env bash
set -xv 

function testdir () {
    echo
    dir=$1
    libs=$(ls $dir/lib/libboost_{atomic,date_time,filesystem,regex,system,thread,program_options}*)
    if [ "$libs" != "" ] && ls $libs >/dev/null 2>&1; then 
        if [ "$dir" != "$INSTALL_DIR" ]; then
            rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
            ln -s $dir/include/boost $INSTALL_DIR/include/boost 
            for lib in $dir/lib/libboost_*; do 
                ln -s $lib $INSTALL_DIR/lib/
            done
        fi
        allgood=true
        for thing in $INSTALL_DIR/lib/libboost_{atomic,date_time,filesystem,regex,system,thread,program_options}* $INSTALL_DIR/include/boost; do 
            if [ ! -e $thing ]; then
                rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
                echo "Something is missing -- keep trying"
                allgood=false
                break
            fi
        done
        if $allgood; then 
            echo "Successfully linked boost"
            echo $dir >BOOSTDIR
            exit 0
        fi
    fi
    return
}
    
for version in -1.55.0 -1.54.0 -1.53.0 -1.49.0; do 
    if [ -d /usr/local/tools/boost-nompi${version} ]; then 
        testdir /usr/local/tools/boost-nompi${version}
    fi
done

for prefix in /usr/local ; do
    testdir $prefix
done


echo "We have exhausted all possibilities... could not find a good system boost"
exit 1
