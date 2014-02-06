#!/usr/bin/env bash
set -xv 

if [ "$NOBOOSTLINK" != "YES" ]; then 
    for prefix in $BOOST_DIR /usr/local/tools /usr/local/ /opt/local; do
        for boost in boost ""; do 
            for infix in -nompi -mpi ""; do 
                for version in -1.54.0 -1.53.0 -1.49.0 ""; do 
                    dir=$prefix/${boost}${infix}${version}
                    if ls $dir/lib/libboost_thread.a $dir/lib/libboost_atomic.a  $dir/lib/libboost_system.a  $dir/lib/libboost_regex.a $dir/lib/libboost_date_time.a $dir/include/boost/random.hpp>/dev/null 2>&1; then 
                        if [ "$dir" != "$INSTALL_DIR" ]; then
                            rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
                            ln -s $dir/include/boost $INSTALL_DIR/include/boost 
                            for lib in $dir/lib/libboost_{atomic,date_time,filesystem,regex,system,thread}*; do 
                                ln -s $lib $INSTALL_DIR/lib/
                            done
                        fi
                        echo $dir
                        exit 0
                    fi
                done
            done
        done
    done
fi
if [ ! -d $INSTALL_DIR/include/boost ] || [ ! -f $INSTALL_DIR/lib/libboost_thread.a ]; then
    rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
fi
export INSTALL_DIR=$INSTALL_DIR
make -e build_boost 1>&2 || exit 1
echo $INSTALL_DIR
exit 0
