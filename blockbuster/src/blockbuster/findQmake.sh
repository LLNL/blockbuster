#!/usr/bin/env bash
function check_version () {
     if ($1 --version | grep 2.00 && $1 --version | grep 4.1.4)  >/dev/null 2>&1; then
	return 0
    fi
    return 1
}

foundqmake=$(which qmake) >/dev/null 2>&1

for qmake in $HOME/$SYS_TYPE/bin/qmake /usr/local/Trolltech/Qt-4.1.4/bin/qmake /usr/mic/dvsviz/Trolltech/Qt-4.1.4/$SYS_TYPE/bin/qmake $foundqmake; do 
	if check_version $qmake; then
	    echo $qmake
	    exit 0
	fi
done
echo QMAKE_UNDEFINED_FOR_THIS_HOST
exit 1
