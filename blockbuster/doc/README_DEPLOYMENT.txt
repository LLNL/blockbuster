==============================================================
# Releasing the new version to the world. 
There is now a script, finalizeVersion.sh, which you run to create the final version.  This sets the version number, checks in the code, and builds for OCF and SCF.  Good stuff.  

OLD DEPLOYMENT:  
1)  Don't forget to modify config/version.h so that blockbuster is announced properly to the world.  

2)  Create a new SVN tag using svn copy.  Here is what I did to release version 2.6.3 to the world.  Note that "svn copy" does not work from the lab due to some firewall issues.  

svn copy https://blockbuster.svn.sourceforge.net/svnroot/blockbuster/trunk/blockbuster https://blockbuster.svn.sourceforge.net/svnroot/blockbuster/tags/2.6.3a -m 'release 2.6.3a, hopefully' --username wealthychef

3)  Upload the tarball package to sourceforge.  See https://sourceforge.net/apps/trac/sourceforge/wiki/Release%20files%20for%20download
Example: 

rsync -avP blockbuster-v2*.tgz wealthychef@frs.sourceforge.net:uploads

4)  After uploading the file, go to http://sourceforge.net/projects/blockbuster, go to the file release management area, and mark the file release as the default for all platforms.  Currently this takes 15 minutes to take effect after upload.  

