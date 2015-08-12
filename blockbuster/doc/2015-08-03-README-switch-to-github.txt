There are two big things to migrate:  source, and issues. 

# =============================================================
SOURCE MIGRATION (GIT): 

# To switch to github, I had to deal with the fact that github does not allow files greater than 100MB in the repo.  

# So I did this: 
git clone --mirror  ssh://wealthychef@git.code.sf.net/p/blockbuster/git  blockbuster-no-big-files

java -jar bfg-1.12.3.jar --strip-blobs-bigger-than 90M blockbuster-no-big-files

# I could not push the result to github because you can only push from a working directory, so had to commit to sourceforge first.  When I tried to commit the resu lto sourceforge, I ran into sourceforge's "no non-fast-forward commits" policy, which I dealt with by editing the config file on the server per  http://pete.akeo.ie/2011/02/denying-non-fast-forward-and.html

rcook@rzgpu2 (blockbuster.old (master)): ssh -t 'wealthychef,blockbuster@shell.sourceforge.net' create
[wealthychef@shell-24003 ~]$ sf-help --scm
Your SCM repositories are here:

    /home/git/p/blockbuster
    /home/svn/p/blockbuster
[wealthychef@shell-24003 ~]$ cd  /home/git/p/blockbuster
[wealthychef@shell-24003 blockbuster]$ ls git.git/
HEAD  branches  config  description  hooks  info  objects  refs
[wealthychef@shell-24003 blockbuster]$ cd git.git/    
[wealthychef@shell-24003 git.git]$ vi config 
< now comment this out: 
  # denyNonFastforwards = true > 

# Now I could push the changes to sourceforge. 
# After that, I cloned the sourceforge repo to a working directory:
git clone --mirror  ssh://wealthychef@git.code.sf.net/p/blockbuster/git blockbuster.sourceforge-2015-08-03-LT100MB-working
cd blockbuster.sourceforge-2015-08-03-LT100MB-working
# Now add github as a remote, pull from github to sync them up, then push to github: 
git remote add github git@github.com:wealthychef1/blockbuster.git
git pull --all # get from sourceforge and github everything
git push --all github

# Now we can work normally from github: 
git clone  git@github.com:wealthychef1/blockbuster.git

# =============================================================
ISSUE MIGRATION (GIT): 
I simply used the tool from 
https://github.com/cmungall/gosf2github

1.  First, exported all items from sourceforge from https://sourceforge.net/p/blockbuster/admin/export, then ran
scp wealthychef@web.sourceforge.net:/home/project-exports/blockbuster/blockbuster-backup-2015-07-30-181947.zip .

2.  Created users_sf2gh.json by hand, with a single mapping from "wealthychef" to "wealthychef1": 

{
  "wealthychef": "wealthychef1"
}


3.  Created collaborators file blockbuster-collaborators.json per the instructions: 

 curl -k -H "Authorization: token (MY_OATH_TOKEN_HERE)" https://api.github.com/repos/wealthychef1/blockbuster/collaborators > blockbuster-collaborators.json 


4.  Dry Run: 
./gosf2github.pl --dry-run --repo wealthychef1/blockbuster --token (MY_OATH_TOKEN_HERE) --usermap users_sf2gh.json --assignee wealthychef1 --collaborators blockbuster-collaborators.json blockbuster-backup-2015-07-30-181947/bugs.json  

Looks good.  Had to hack the perl script to put -k in the curl line to avoid our certificate problem here, but otherwise straightforward.  

4.  Ultimate command to execute (no --dry-run)
./gosf2github.pl  --repo wealthychef1/blockbuster --token (MY_OATH_TOKEN_HERE) --usermap users_sf2gh.json --assignee wealthychef1 --collaborators blockbuster-collaborators.json blockbuster-backup-2015-07-30-181947/bugs.json  


Works like a charm!  
