#!/usr/bin/env bash
# ==========================================
logfile=${0}.log
firsttime=${firsttime:-true}
verbose=false
src_dir=$(cd dirname $0; pwd)

if $firsttime; then
	echo saving output to logfile $logfile
	firsttime=false exec $0 "$@" |& tee $logfile	
	exit 0 
fi

# ==========================================
function runecho () {
	echo running $@
    "$@"
}

# ==========================================
function usage() {
    echo "usage: finalizeVersion.h [options] versionstring"
    echo "This script creates a new version, either with or without committing the changes.   It is designed to be run on rzgpu or rzbeast."
	echo "Example versionstring: \"2.8.6a\"" 
    echo "This means: "
    echo "1.  Updates src/config/version.h"
    echo "2.  Greps Changelog to make sure there is an entry there.  Updates the Changelog entry. "
    echo " OPTIONS: " 
    echo "-c/--commit: Commit changes to all tracked directories and files before proceeding. Default: only commit version-related files."
    echo "-t/--temp: Just update Changelog and version.h as needed."
    echo "-f/--final: updates version.h and Changelog, creates a tarball in the current directory with proper naming scheme.  Installs on auk." 
    echo " -v: set -xv to make this script painfully chatty" 
    echo "NOTE: You must give either --temp or --final."
}


# ==========================================
stagedir=${stagedir:-/nfs/tmp2/rcook/blockbuster/staging}
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
	echo logfile is $logfile
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
    remotehost=rzgpu
	remotesys=chaos_5_x86_64_ib
elif [ "${SYS_TYPE}" == chaos_5_x86_64_ib ] ; then
    remotehost=pw453
	remotesys=chaos_5_x86_64
fi
if [ "$remotehost" == "" ]; then
    errexit "Unknown SYS_TYPE or non-RZ host.  Please run on an RZ cluster"
fi

# =======================================
temp=false
final=false
commit=false
verbose=false
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
        set -xv;
        verbose=true; 
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
echo "Commit files: \"$commitfiles\""
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
if [ -z "$version" ]; then 
    errexit "You need to supply a version number or I'll kill you.  Seriously."
fi

cd $src_dir
[ -e src/config/version.h ] || errexit "This script needs to be located in the top of the source directory to work right. " 

echo "Setting version in src/config/version.h..." 
sedfiles -e "s/#define BLOCKBUSTER_VERSION.*/#define BLOCKBUSTER_VERSION \"$version -- $(date)\"/" src/config/version.h  || errexit "sedfiles failed"

echo "Saving version in src/config/versionstring.txt"
echo $version > src/config/versionstring.txt || errexit "Could not echo string to file!"

tagname=blockbuster-v$version

#======================================================
# Make sure the testing is done for this version. 
# 
if $final; then 
    echo "Have you completed testing of this version?  Please see doc/README_TESTING.txt for the complete procedure.  Type 'h' to see doc/README_TESTING.txt and exit without continuing.  Type 'y' to swear by Zeus that you have followed proper testing."
    
    read answer
    if [ "$answer" == 'h' ]; then 
        cat doc/README_TESTING.txt
        exit 0
    elif [ "$answer" != 'y' ]; then 
        echo "You must either complete testing or lie to Zeus (at the peril of your immortal soul) before using finalizeVersion.sh in 'final' mode.  Your choice."
        exit 1
    fi
fi


#======================================================
# Update Changelog
#
echo "Checking Changelog to make sure you have done your housekeeping..." 
if ! versionfound=$(grep $version doc/Changelog.txt); then 
    errexit "Could not find version $version in Changelog.txt.  Please update the Changelog and I will continue."
fi

sedfiles -e "s/VERSION $version.*/VERSION $version (git tag $tagname) $(date)/" doc/Changelog.txt || errexit "Could not place version in doc/Changelog.txt.  Please check the Changelog file for errors."  

#======================================================
echo "stage files"
runecho git add -u $commitfiles || errexit "git add failed"

#======================================================
# Create a tarball for use in installation etc. 
echo "Creating clean temp directory $tmpdir to work in..." 
rm -rf $tmpdir
mkdir -p $tmpdir || errexit "Could not create tmp directory for tarball"
pushd $tmpdir || errexit "Could not cd into new tmp directory!?" 

builddir=$stagedir/$version
echo "Creating fresh build directory $builddir and installation directory $installdir..."
rm -rf $builddir
mkdir -p  $builddir

popd 


# =============================================================
#======================================================
if ! $temp; then 
    echo "Installing software..." 
    svn_url=https://eris.llnl.gov/svn/lclocal/public/blockbuster/branches/blockbuster-${version}  
    buildconf_url=https://eris.llnl.gov/svn/crlf-build/lclocal.el6/blockbuster-${version}/build.conf
    svn_branch_dir=/g/g0/rcook/current_projects/eris/lclocal/public/blockbuster/branches
    svn_dir=${svn_branch_dir}/blockbuster-${version}
    runecho needinit=false
    if ! runecho svn ls $buildconf_url; then       
        echo "I think we need to initialize the eris repo, since I do not see $buildconf_url.  Am I right? (y/n)"        
        read answer
        if [ "$answer" == "y" ]; then 
            runecho needinit=true
        else
            errexit "I will exit now.  Something is wrong if I think we need to initialize but you disagree.  Please debug this." 
        fi
    fi    
    
    if ! [ -d $svn_dir ]; then 
        pushd $svn_branch_dir
        echo "Directory $(pwd)/blockbuster-${version} not found.  Creating." 
        if ! svn ls $svn_url/package.conf >/dev/null 2>&1; then 
            echo "Version $version does not exist in eris repo; creating..."
            svn mkdir blockbuster-${version}
            cp $src_dir/eris-packagedir-template/* $svn_dir/ || errexit "Cannot update $svn_dir with package contents"
            sed -i'' "s/_vers=.*/_vers=${version}/" package.conf
            svn add *
        else
            svn update --set-depth=infinity blockbuster-${version}
        fi
        popd 
    fi
    
    echo "Updating directory $svn_dir"
    
    git archive --format tar --prefix=blockbuster-v${version}/ HEAD | gzip > ${svn_dir}/blockbuster-v${version}.tgz || errexit "Cannot create blockbuster tarball" 
    echo "Tarball is $svn_dir/blockbuster-v${version}.tgz."
    
    if $verbose; then 
        verboseflag='-v'
    fi
    if $needinit; then
        runecho setTag.sh -i $verboseflag --default=no blockbuster-${version}
    else
        runecho setTag.sh -b -c $verboseflag --default=no  blockbuster-${version} "automatic commit from finalizeVersion.sh" 
    fi
    
    
    auksuccess=true
    scp $svn_dir/blockbuster-v${version}.tgz auk61:/viz/blockbuster/tarballs/
    runecho ssh auk61 "set -xv; rm -rf /viz/blockbuster/${version} && mkdir -p /viz/blockbuster/${version} && tar -C /viz/blockbuster/${version} -xzf /viz/blockbuster/tarballs/blockbuster-v${version}.tgz && pushd /viz/blockbuster/${version}/blockbuster-v${version} && INSTALL_DIR=/viz/blockbuster/${version} make && rm -f /viz/blockbuster/test && ln -s /viz/blockbuster/${version} /viz/blockbuster/test" || auksuccess=false
    
    if ! $auksuccess; then 
        echo "Warning:  build on auk failed"
    else
        echo "Built and installed blockbuster-v${version} and made it the test version on auk" 
    fi
    
fi

#======================================================
# push to remote
if ! testbool $nopush; then
    echo "commiting local changes and pushing source to remote..."
    runecho git commit -m  "Version $version, automatic checkin by finalizeVersion.sh, by user $(whoami)"  || errexit "git commit failed"
    runecho git push origin  || errexit "git push origin failed"
    
    echo "Creating new tag" 
# runecho git tag -d $tagname 2>/dev/null  # ok to fail
    runecho git tag -f $tagname -m "Version $version, automatic checkin by finalizeVersion.sh, by user $(whoami)" || errexit "git tag failed"
    runecho git push origin $tagname || errexit "git push origin  $tagname failed"

    echo "Exporting the new tag tagname=$tagname from git repo..." 
    eval git archive --prefix=$tagname/ $tagname | gzip > ${builddir}/$tagname.tgz  ||  errexit "Could not export archive of tag $tagname from git repo to $tagname.tgz"
    
    echo "Cleaning up tempdir..." 
    rm -rf $tmpdir
    echo "Done.  Source tarball is  ${builddir}/$tagname.tgz"
else
    echo "Done.  No tarball was created because nothing was pushed to the repo.  Use the -f flag to create a source tarball and commit all changes."  
fi

exit 0

