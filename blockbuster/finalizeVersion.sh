#!/usr/bin/env bash

function usage() {
    echo "usage: finalizeVersion.h [options] versionstring"
    echo "This script creates a new version, either with or without committing the changes.   It is designed to be run on rzgpu or rzbeast."
    echo "This means: "
    echo "updates version.h "
    echo "greps Changelog to make sure there is an entry there.  Updates the Changelog entry. "
    echo " OPTIONS: " 
    echo "-c/--commit: Commit changes to all directories and before proceeding. Default: no."
    echo "-t/--temp: Just update Changelog and version.h as needed."
    echo "-f/--final: updates version.h and Changelog, creates a tarball in the current directory with proper naming scheme. " 
    echo " -v: set -xv" 
    echo "NOTE: You must give either --temp or --final."
}


stagedir=${stagedir:-/nfs/tmp2/rcook/blockbuster}
tmpdir=$stagedir/install-tmp/finalizeVersion-tmp
#=================================
# test if a string has nonzero length
function nonnull () { 
    if [ x"$1" == x ] ; then 
	return 1
    fi
    return 0
}
#=================================
function isnull() {
    if [ x"$1" == x ]; then 
	return 0; 
fi
    return 1
}
#=================================
function testbool () {
    (nonnull "$1" && [ "$1" != false ] && [ "$1" != no ] && [ "$1" != 0 ] && return 0) || return 1
}

# requires a version as an argument.  
#=================================
function errexit() {
    echo 
    echo '*******************************************************'
    echo "ERROR"
    echo "ERROR:  $1"
    echo "ERROR"
    echo '*******************************************************'
    echo 
    rm -rf $tmpdir
    exit ${2:-1}
}

#=================================
export PATH=/usr/local/tools/qt-4.8.4/bin:$PATH
if [ -f /usr/local/tools/dotkit/init.sh ]; then
    . /usr/local/tools/dotkit/init.sh
    if use qt; then 
        echo "Warning: used dotkit to set Qt."
    else
        echo "Warning: Could not set Qt dotkit."
    fi
fi
echo  "qmake is " $(which qmake)
#========================================
if [ "${SYS_TYPE}" == chaos_5_x86_64 ] ; then
    # we are on rzbeast or equivalent
    remotehost=rzgpu
elif [ "${SYS_TYPE}" == chaos_5_x86_64_ib ] ; then
    remotehost=rzbeast
fi
if [ "$remotehost" == "" ]; then
    errexit "Unknown SYS_TYPE or non-RZ host.  Please run on an RZ cluster"
fi

# =======================================
temp=false
final=false
commit=false
# must give either -temp or -final: 
for arg in "$@"; do 
    if [ "$arg" == --commit ]  ||  [ "$arg" == -c ] ; then
        commit=true
    elif [ "$arg" == --temp ]  ||  [ "$arg" == -t ] ; then
        temp=true
    elif [ "$arg" == --final ]  ||  [ "$arg" == -f ] ; then
        final=true
    elif [ "$arg" == --help ] ||  [ "$arg" == -h ] ; then
        usage
        exit 0
    elif [ "$arg" == --verbose ]  ||  [ "$arg" == -v ] ; then
        set -xv
    elif [ "${arg:0:1}" == '-' ]; then
        usage
        errexit "Bad argument: $arg"
    else
       version="$arg"
    fi 
done

#=================================
if ! testbool "$temp" && ! testbool "$final"; then
    usage
    errexit "You must give either -t/--temp or -f/--final as an argument"
fi
nopush=true
if [ "$final" == true ]; then 
    nopush=false
fi
#=================================
commitfiles="doc/Changelog.txt src/config/version.h  src/config/versionstring.txt"
if testbool "$commit"; then 
    commitfiles=.
fi
#=================================
function sedfilesusage() {
    echo "usage: sedfiles [opts] (expression | -e expression1 -e expression2 ...) files"
    echo "OPTIONS:"
    echo '--nobackups:  live dangerously'
    return
}

#=================================
function sedfiles () {
    if isnull $1 || [ "$1" == -h ]; then 
        sedfilesusage 
        return 1;
    fi
    while [ ${1:0:1} == - ]; do 
	case "$1" in 
	    --nobackups) nobackups=true; shift;;
	    *) break;;
	esac
    done
    #set -vx
    expressions=()
    expnum=0    
    while [ x"$1" == x-e ] ; do	
	expressions[expnum]="$1"
	let expnum++
	shift
	expressions[expnum]="$1"
	let expnum++
	shift
    done
    if [ x"${expressions[0]}" == x ] ; then 
	expressions[expnum]="$1"
	shift
    fi
    for file in "$@"; do
	[ -e "$file" ] || returnerr "file \"$file\" does not exist"
	if ! testbool $nobackups; then
	    if ! cp "$file" "$file.bak" ; then 
            echo "cannot copy \"$file\" to \"$file.bak\", aborting"
            return 1
        fi
	fi
	if ! sed "${expressions[@]}" "$file" > "$file.new"; then
        echo "sed ${expressions[@]} \"$file\" failed, original is unchanged" 
        return 1
    fi
	if ! mv "$file.new" "$file"; then 
        echo "Cannot move \"$file.new\" to \"$file\", changes are in \"$file.new\""
        return 1
    fi
    done
    return 0
    #set +vx
}


#=================================


#======================================================
# Update version.h
if [ x$version = x ]; then 
    errexit "You need to supply a version number or I'll kill you.  Seriously."
fi

cd $(dirname $0)
[ -e src/config/version.h ] || errexit "This script needs to be located in the top of the source directory to work right. " 

echo "Setting version in src/config/version.h..." 
sedfiles -e "s/#define BLOCKBUSTER_VERSION.*/#define BLOCKBUSTER_VERSION \"$version -- $(date)\"/" src/config/version.h  || errexit "sedfiles failed"

echo "Saving version in src/config/versionstring.txt"
echo $version > src/config/versionstring.txt || errexit "Could not echo string to file!"

tagname=blockbuster-v$version

#======================================================
# Update Changelog
echo "Checking Changelog to make sure you have done your housekeeping..." 
if ! grep $version doc/Changelog.txt >/dev/null; then 
    errexit "Could not find version $version in Changelog.txt.  Please update the Changelog and I will continue."
fi

sedfiles -e "s/VERSION $version.*/VERSION $version (git tag $tagname) $(date)/" doc/Changelog.txt || errexit "Could not place version in doc/Changelog.txt.  Please check the Changelog file for errors."  

#======================================================
# stage files 
git add $commitfiles || errexit "git add failed"

if testbool $nopush; then
    echo "Version updated.  No git push will be performed"
    exit 0
fi

#======================================================
# push to remote
echo "commiting local changes and pushing source to remote..."
git commit -m  "Version $version, automatic checkin by finalizeVersion.sh, by user $(whoami)"  || errexit "git commit failed"
git push origin  || errexit "git push origin failed"

echo "Creating new tag" 
git tag -d $tagname 2>/dev/null  # ok to fail
git tag -a $tagname -m "Version $version, automatic checkin by finalizeVersion.sh, by user $(whoami)" || errexit "git tag failed"
git push origin $tagname || errexit "git push origin  $tagname failed"

#======================================================
# Update and install on LC cluster
echo "Creating clean temp directory $tmpdir to work in..." 
rm -rf $tmpdir
mkdir -p $tmpdir || errexit "Could not create tmp directory for tarball"
pushd $tmpdir || errexit "Could not cd into new tmp directory!?" 


builddir=$stagedir/$version
installdir=/usr/gapps/asciviz/blockbuster/$version
echo "Creating fresh build directory $builddir and installation directory $installdir..."
rm -rf $installdir $builddir
mkdir -p $installdir $builddir

echo "Exporting the new tag from git repo..." 
git archive --prefix=$tagname/ $tagname | gzip > ${builddir}/$tagname.tgz || errexit "Could not export archive of tag $tagname from git repo to $tagname.tgz"

popd
echo "Cleaning up tempdir..." 
rm -rf $tmpdir


# =============================================================
echo "Installing software..." 

echo '#!/usr/bin/env bash'"
. $HOME/.profile
export PATH=/usr/local/tools/qt-4.8.4/bin:\$PATH

function errexit() {
    echo 
    echo '*******************************************************'
    echo \"ERROR\"
    echo \"ERROR:  \$1\"
    echo \"ERROR\"
    echo '*******************************************************'
    echo 
    rm -rf $tmpdir
    exit ${2:-1}
}

rm -rf $installdir/\$SYS_TYPE $builddir/\$SYS_TYPE
mkdir -p $builddir/\$SYS_TYPE
pushd $builddir/\$SYS_TYPE || errexit \"Cannot cd to install directory $SYS_TYPE\"

tar -xzf $builddir/blockbuster-v${version}.tgz || errexit \"Cannot untar tarball\" 

pushd blockbuster-v${version} || errexit \"Cannot cd to blockbuster source directory\" 

INSTALL_DIR=$installdir/\$SYS_TYPE make || errexit \"Could not make software\" 

popd

popd
echo INSTALLATION FINISHED
" > ${builddir}/installer.sh
chmod 700 ${builddir}/installer.sh
${builddir}/installer.sh || errexit "installer failed on localhost"

ssh $remotehost "${builddir}/installer.sh" || errexit "installer failed on remotehost"

echo "Creating symlink of new version to /usr/gapps/asciviz/blockbuster/test"

pushd /usr/gapps/asciviz/blockbuster/ || errexit "Could not cd to blockbuster public area"
rm -f test
ln -s $version test
popd
echo "Done.  Tarball is $builddir/blockbuster-v${version}.tgz.  To use the new version, type \"use asciviz-test\""
exit 0

