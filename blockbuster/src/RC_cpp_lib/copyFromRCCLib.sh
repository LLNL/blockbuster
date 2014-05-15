#!/usr/bin/env bash
errexit() {
    echo $1
    exit ${2:-1}
}

runecho () {
    echo "$@"
    "$@"
}

pushd $(dirname $0)

RCCPPLIB=$HOME/current_projects/RC_cpp_lib
if [ ! -d $RCCPPLIB ]; then
    errexit "Cannot find $RCCPPLIB"
fi

for file in *.{C,h}; do
    if [ ! -e  $RCCPPLIB/$file ]; then
        errexit "Invalid assumption:  File $file exists, but $RCCPPLIB/$file does not exist"
    fi
    if ! diff $file $RCCPPLIB/$file >/dev/null 2>&1; then 
        echo "$file needs updating..."
        runecho cp $RCCPPLIB/$file $file 
    else
        echo "$file does not need updating..."
    fi
done

popd
