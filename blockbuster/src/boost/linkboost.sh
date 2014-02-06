#!/usr/bin/env bash
set -xv 
BOOST_DIR=$()

# look for src/book/nolinks file

NOBOOSTLINK=${NOBOOSTLINK:-false}
if [ -f ../config/noboostlink ] && [ $(cat ../config/noboostlink) == "YES" ]; then
    NOBOOSTLINK=true
fi

function linkfile() {
    file=$1
    if [ -d $INSTALL_DIR/$file ]; then 
        file="${file}/"
    fi
    if [ $NOBOOSTLINK == "false" ] ; then 
        # make a symlink
        rm -rf $INSTALL_DIR/$file
        link_command='ln -s'
    else
        # remove the file or directory if it's a symlink or not the same file
        if [ -h $INSTALL_DIR/$file ] || diff -q $BOOST_DIR/$file $INSTALL_DIR/$file ; then 
            rm -rf $INSTALL_DIR/$file
        fi
        # update the file or directory
        link_command='rsync -auv'
    fi
    $link_command $BOOST_DIR/$file $INSTALL_DIR/$file 
}


for prefix in $BOOST_DIR /usr/local/tools /usr/local/ /opt/local; do
    for infix in -nompi -mpi ""; do 
        for version in -1.54.0 -1.53.0 -1.49.0 ""; do 
            dir=$prefix/boost${infix}${version}
            if ls $dir/lib/libboost_filesystem.a $dir/lib/libboost_thread.a  $dir/lib/libboost_system.a  $dir/lib/libboost_regex.a $dir/lib/libboost_date_time.a $dir/include/boost/random.hpp>/dev/null 2>&1; then 
                set -xv
                if ! linkfile include/boost; then 
                    echo "could not install boost in $INSTALL_DIR/include"
                    exit 1
                fi
                for lib in $dir/lib/libboost*; do 
                    if ! linkfile lib/$(basename $lib) ; then
                        echo "Could not install file $lib in $INSTALL_DIR/lib"
                        exit 1
                    fi
                done
                echo "boost links placed in $INSTALL_DIR" 
                echo $dir > $INSTALL_DIR/lib/BOOST_DIR
                exit 0
            fi
        done
    done
done
echo "boost not found; Please install and/or set BOOST_DIR (such that \"\$\$BOOST_DIR/include/boost\" exists) to enable me to find it."
exit 1
