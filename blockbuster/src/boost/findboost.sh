#!/usr/bin/env bash
set -xv 

if [ "$NOBOOSTLINK" != "YES" ]; then 
    for prefix in $BOOST_DIR /usr/local/tools /usr/local/ /opt/local /sw; do
        for boost in boost ""; do 
            for infix in -nompi -mpi ""; do 
                for version in -1.55.0 -1.54.0 -1.53.0 -1.49.0 ""; do 
                    dir=$prefix/${boost}${infix}${version}
                    libs=$dir/lib/libboost_{atomic,date_time,filesystem,regex,system,thread,program_options}*
                    if ls $libs >/dev/null 2>&1; then 
                        if [ "$dir" != "$INSTALL_DIR" ]; then
                            rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
                            ln -s $dir/include/boost $INSTALL_DIR/include/boost 
                            for lib in $libs; do 
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
for thing in $INSTALL_DIR/lib/libboost_{atomic,date_time,filesystem,regex,system,thread,program_options}* $INSTALL_DIR/include/boost; do 
    if [ ! -e $thing ]; then
        rm -rf $INSTALL_DIR/include/boost $INSTALL_DIR/lib/libboost_*
        break
    fi
done
INSTALL_DIR=$INSTALL_DIR  make -e build_boost 1>&2 || exit 1
exit 0
