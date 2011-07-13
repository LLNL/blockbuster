#!/usr/bin/env bash
# This script creates a new version, 
# This means: 
#  updates version.h 
#  greps Changelog to make sure there is an entry there.  Updates the Changelog entry. 
#  checks in version.h and Changelog to the trunk, 
#  creates a tarball in the current directory with proper naming scheme.  

# requires a version as an argument.  
#=================================
function errexit() {
    echo "$0: $1"
    exit ${2:-1}
}

#=================================
function testbool () {
    (nonnull "$1" && [ "$1" != false ] && [ "$1" != no ] && [ "$1" != 0 ] && return 0) || return 1
}
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
version=$1
if [ x$version = x ]; then 
    errexit "You need to supply a version number or I'll kill you." 
fi

cd $(dirname $0)
[ -e src/config/version.h ] || errexit "This script needs to be located in the top of the source directory to work right. " 

echo "Setting version in src/config/version.h..." 
sedfiles -e "s/#define BLOCKBUSTER_VERSION.*/#define BLOCKBUSTER_VERSION \"$version -- $(date)\"/" src/config/version.h  || errexit "sedfiles failed"

#======================================================
# Update Changelog
revision=$(svn update | sed 's/At revision \(.*\)\./\1/')
newrevision=$(expr $revision + 2)
echo "Current SVN revision is $revision and new revision will be $newrevision..." 

echo "Checking Changelog to make sure you have done your housekeeping..." 
if ! grep $version doc/Changelog.txt >/dev/null; then 
    errexit "Could not find version $version in Changelog.txt.  Please update the Changelog and I will continue."
fi
sedfiles -e "s/VERSION $version .*/VERSION $version (r$newrevision) $(date)/" doc/Changelog.txt || errexit "Could not place version in doc/Changelog.txt.  Please check the Changelog file for errors."  

#======================================================
# Update Subversion repository
echo "Removing version $version from SVN if it exists..." 
versiondir=https://blockbuster.svn.sourceforge.net/svnroot/blockbuster/tags/blockbuster-v$version
svn rm -m "Removing version $version if it exists..."  $versiondir

echo "Checking in source..."
svn commit -m "Version $version, automatic checkin by finalizeVersion.sh, by user $(whoami)" doc/Changelog.txt src/config/version.h || errexit "svn commit failed"

#======================================================
# Update and install on LC cluster
echo "Creating temp directory to work in for tarball creation..." 
tmpdir=$(pwd)/finalizeVersion-tmp
rm -rf $tmpdir
mkdir $tmpdir || errexit "Could not create tmp directory for tarball"
cd $tmpdir || errexit "Could not cd into new tmp directory!?" 

echo "Creating new version in SVN repo from trunk" 
svn cp -m "Version $version, automatic version creation by finalizeVersion.sh, by user $(whoami)" https://blockbuster.svn.sourceforge.net/svnroot/blockbuster/trunk/blockbuster  $versiondir || errexit "could not create version in svn" 

echo "Checking out the new version from SVN repo..." 
svn co $versiondir || errexit "could not check out versiondir $versiondir from svn repo" 

installdir=/usr/gapps/asciviz/blockbuster/$version
echo "Creating version directory $installdir..."
rm -rf $installdir 
mkdir -p $installdir

echo "Cleaning up install dir from previous attempts..." 
rm -f $installdir/blockbuster-v${version}.tgz

echo "Creating actual tarball..." 
tar -czf ${installdir}/blockbuster-v${version}.tgz blockbuster-v${version}

echo "Cleaning up tempdir..." 
cd $tmpdir/..
rm -rf $tmpdir

echo "Installing software..." 
cd $installdir
rm -rf chaos*
mkdir -p chaos_4_x86_64_ib
ln -s chaos_4_x86_64_ib chaos_4_x86_64
cd chaos_4_x86_64_ib || errexit "Cannot cd to install directory chaos_4_x86_64_ib"
tar -xzf ../blockbuster-v${version}.tgz || errexit "Cannot untar tarball!?" 
cd blockbuster-v${version} || errexit "Cannot cd to blockbuster source directory" 
make || errexit "Could not make software" 

echo "Creating symlink of new version to /usr/gapps/asciviz/blockbuster/test"

cd /usr/gapps/asciviz/blockbuster/ || errexit "Could not cd to blockbuster public area"
rm -f test
ln -s $version test
echo "Done.  Tarball is $installdir/blockbuster-v${version}.tgz.  To use the new version, type \"use asciviz-test\""
exit 0

