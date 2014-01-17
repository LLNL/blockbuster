/*
** $RCSfile: smevo.C,v $
** $Name:  $
**
** ASCI Visualization Project 
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
** 	http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smevo.C,v 1.1 2007/06/13 18:59:35 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


// Utility to convert sm movies to evo movies and vice versa
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>

#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"
#include "sm/smJPG.h"

#include "zlib.h"
#include "../config/version.h"

#include "evomovie.h"

// Prototypes 
void cmdline(char *app);

void cmdline(char *app)
{
	smdbprintf(0,"%s (%s) usage: %s [options] evofile smfile | smfile evofile\n",
		basename(app), BLOCKBUSTER_VERSION, basename(app));
	smdbprintf(0,"Options:\n");
	smdbprintf(0,"\t-v Verbose mode.\n");
	smdbprintf(0,"\t-rle Selects RLE sm compression\n");
	smdbprintf(0,"\t-gz Selects gzip sm compression\n");
	smdbprintf(0,"\t-lzo Selects LZO sm compression\n");
	smdbprintf(0,"\t-jpg Selects JPG sm compression\n");
	smdbprintf(0,"\t-jpeg Selects jpeg evo compression\n");
	smdbprintf(0,"\t-stereo Treat the sm file as stereo\n");
	exit(1);
}

int main(int argc,char **argv)
{
	char 		*infile,*outfile;
	int	iVerb = 0;
	int	iSMComp = 0;
	int	iEVOComp = 0;
	int	iStereo = 0;
	int	i,isize[2],nFrames;

	smBase 		*sm;
	unsigned char	*img;
	EVOMoviePtrType evo;
	EVOImagePtrType evo_img;

	/* parse the command line ... */
	i = 1;
	while ((i<argc) && (argv[i][0] == '-')) {
		if (strcmp(argv[i],"-v")==0) {
			iVerb = 1;
		} else if (strcmp(argv[i],"-stereo")==0) {
			iStereo = 1;
		} else if (strcmp(argv[i],"-rle")==0) {
			iSMComp = 1;
		} else if (strcmp(argv[i],"-gz")==0) {
			iSMComp = 2;
		} else if (strcmp(argv[i],"-lzo")==0) {
			iSMComp = 3;
		} else if (strcmp(argv[i],"-jpg")==0) {
			iSMComp = 4;
		} else if (strcmp(argv[i],"-jpeg")==0) {
			iEVOComp = 1;
		} else {
			cmdline(argv[0]);
		}
		i++;
	}

	if (argc-i != 2) cmdline(argv[0]);

	infile = argv[i++];
	outfile = argv[i++];

/* convert to evo */
	sm = smBase::openFile(infile, O_RDONLY, 1);
	if (sm) { 
		isize[0] = sm->getWidth();
		isize[1] = sm->getHeight();
		nFrames = sm->getNumFrames();
		if (sm->getFlags() & SM_FLAGS_STEREO) iStereo = 1;

		if (iEVOComp) {
			evo = evo_movie_new_file(outfile,iStereo,isize[0],
				isize[1],nFrames/(iStereo+1),MOVIEJPEG,75);
		} else {
			evo = evo_movie_new_file(outfile,iStereo,isize[0],
				isize[1],nFrames/(iStereo+1),MOVIERLE,0);
		}

		evo_img = evo_movie_alloc_image(isize[0],isize[1]);
		if (!evo_img) {
	smdbprintf(0,"Insufficient memory\n");
			exit(1);
		}
		img = evo_img->pixels;

		for(i=0;i<nFrames;i++) {
			if (iVerb) printf("Working on %d of %d\n",i,nFrames);
			sm->getFrame(i, img, 0);
			evo_movie_add_image(evo, evo_img);
		}

		evo_movie_free_image(evo_img);

		evo_movie_close_file(evo);
		sm->closeFile();
		delete sm;

		exit(0);
	}

/* comvert to sm */
	evo=evo_movie_open_file(infile);
	if (evo) {
		isize[0] = evo->xdim;
		isize[1] = evo->ydim;
		nFrames = evo->nFrames;
		if (iStereo) nFrames *= 2;

	        if (iSMComp == 1) {
     	  	        sm = new smRLE(outfile,isize[0],isize[1],nFrames);
   		} else if (iSMComp == 2) {
  	              	sm = new smGZ(outfile,isize[0],isize[1],nFrames);
 	        } else if (iSMComp == 3) {
	                sm = new smLZO(outfile,isize[0],isize[1],nFrames);
 	        } else if (iSMComp == 4) {
	                sm = new smJPG(outfile,isize[0],isize[1],nFrames);
      	        } else {
     	       		sm = new smRaw(outfile,isize[0],isize[1],nFrames);
    	        }
		if (iStereo) sm->setFlags(SM_FLAGS_STEREO);

		evo_img = evo_movie_alloc_image(isize[0],isize[1]);
		if (!evo_img) {
	smdbprintf(0,"Insufficient memory\n");
			exit(1);
		}
		img = evo_img->pixels;

		/* order is lefteye,righteye, */
		for(i=0;i<nFrames;i+=(iStereo+1)) {
			if (iVerb) printf("Working on %d of %d\n",i,nFrames);
			evo_movie_get_image(evo,evo_img,i/(iStereo+1),
		       		LEFTEYE);
			sm->compressAndWriteFrame(i,img);
			/* if the sm is to be stereo, pick the other eye
			 * if you can do that */
			if (iStereo) {
				evo_movie_get_image(evo,evo_img,i/(iStereo+1),
					evo->stereo ? RIGHTEYE : LEFTEYE);
				sm->compressAndWriteFrame(i+1,img);
			}
		}

		evo_movie_free_image(evo_img);

		evo_movie_close_file(evo);
		sm->closeFile();
		delete sm;

		exit(0);

	}

	smdbprintf(0,"Input file %s was not in sm or evo format\n", infile);
	exit(1);
}
