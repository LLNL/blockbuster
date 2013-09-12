#!/usr/bin/env bash

for prefix in /usr/local/tools /usr/local/ /opt/local; do
    for infix in -nompi -mpi ""; do 
        for version in -1.54.0 -1.53.0 -1.49.0 ""; do 
            dir=$prefix/boost${infix}${version}
            if ls $dir/lib/libboost_thread.a  $dir/lib/libboost_system.a  $dir/lib/libboost_regex.a $dir/lib/libboost_date_time.a $dir/include/boost/random.hpp>/dev/null 2>&1; then 
                set -xv
                rm -rf $INSTALL_DIR/include/boost
                if ! ln -s $dir/include/boost $INSTALL_DIR/include/boost; then
                        echo "could not create symlink to boost in $INSTALL_DIR/include"
                        exit 1
                    fi
                for lib in $dir/lib/libboost*; do 
                    rm -f $INSTALL_DIR/lib/$(basename $lib) 
                    if ! ln -s $lib $INSTALL_DIR/lib/$(basename $lib) ; then
                        echo "could not create symlink to $lib in $INSTALL_DIR"
                        exit 1
                    fi
                done
                echo "boost links placed in $INSTALL_DIR" 
                exit 0
            fi
        done
    done
done
echo "boost not found; need to build from source"
exit 1
