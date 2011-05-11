/*
** $RCSfile: smcat.C,v $
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
** $Id: smcat.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


// Utility to combine movie files into a movie file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"
#include "sm/smJPG.h"

#include "pt/pt.h"

#include "zlib.h"
#include <stdarg.h>

int		gVerbosity = 0;


//===============================================
void dbprintf(int level, const char *fmt, ...) {
  if (gVerbosity < level) return; 
  // cerr <<  DBPRINTF_PREAMBLE; 
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  return; 
}

// Prototypes 
void cmdline(char *app);

static void smoothx(unsigned char *image, int dx, int dy);
static void smoothy(unsigned char *image, int dx, int dy);

void Sample2d(unsigned char *in,int idx,int idy,unsigned char *out,
        int odx,int ody,int s_left,int s_top,
        int s_dx,int s_dy,int filter);


/* code... */
void cmdline(char *app)
{
	fprintf(stderr,"(%s) usage: %s [options] outfile infile1 [infile2...]\n",
		__DATE__,app);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"\t-v Verbose mode (sets verbosity to 1).\n");
	fprintf(stderr,"\t-verbose n Set verbosity to n.\n");
	fprintf(stderr,"\t-threads [nt] Number of threads to use. Default: 1.\n");
	fprintf(stderr,"\t-mipmaps [n] Number of mipmap levels. Default: 1\n");
	fprintf(stderr,"\t-tilesizes Specify tile sizes per mipmap level. Comma separated.  Non-square tiles can be specified as e.g. 128x256. (default: 512).\n");
	fprintf(stderr,"\t-rle Select RLE compresssion.\n");
	fprintf(stderr,"\t-gz Select gzip compresssion.\n");
	fprintf(stderr,"\t-lzo Select LZO compresssion.\n");
	fprintf(stderr,"\t-jpg Select JPG compresssion.\n");
	fprintf(stderr,"\t-jqual [qual] Selects JPG quality. Default: 75\n");
	fprintf(stderr,"\t-FPS [fps] Set preferred frame rate.  Default is 30.\n");
	fprintf(stderr,"\t-stereo Specify the output file is stereo.\n");
	fprintf(stderr,"\t-dst [dx] [dy] Select the output size. Default: source size.\n");
	fprintf(stderr,"\t-src [x] [y] [dx] [dy] Select a rectangle of the input. Default: all.\n");
	fprintf(stderr,"\t-filter Enable smoothing filter for image scaling.\n");
	fprintf(stderr,"\n\tNote: inputfile syntax \"file[@first[@last[@step]]]\"\n");
	fprintf(stderr,"\tAllowing the first, last and frame step to be specified\"\n");
	fprintf(stderr,"\tfor each input .sm file individually. The default is to\n");
	fprintf(stderr,"\ttake all frames in an input file, stepping by 1.\n");
	exit(1);
}

typedef struct {
	char name[1024];
	smBase	*sm;
	int first,last,step;
	int num;
} inp;

struct Work {
	smBase *sm;
	smBase *insm;
	int inframe;
	int outframe;
	int iScale;
	int iFilter;
	int *dst;
	int *src;
  unsigned char *buffer;
  unsigned char *compbuffer;
} ;

void workproc(void *arg);

int main(int argc,char **argv)
{
	int		iRLE = 0;
	char		*sOutput = NULL;
	int		iFilter = 0;
	int		iStereo = 0;
	int		iSize[2];
	int		dst[2] = {-1,-1};
	int		src[4] = {-1,-1,-1,-1};
	int		iQual = 75;
	float		fFPS = 0.0;
	int		nThreads = 1;
	int		nRes = 1;

	int		i,j,k,count;
	smBase 		*sm = NULL;
	void 		*buffer = NULL;
	void		*pZoom = NULL;
	int		buffer_len = 0;
	int		ninputs;
	inp		*input;
	int		iScale;
	pt_pool		thepool;
	pt_pool_t	pool = &thepool;

	char	        tstr[1024],tstr2[1024];
	char            tsizestr[1024] = "512";
	unsigned int    tsizes[8][2];
	int             tiled = 0;

	/* parse the command line ... */
	i = 1; 
	while ((i<argc) && (argv[i][0] == '-')) {
		if (strcmp(argv[i],"-v")==0) {
			gVerbosity = 1;
            //sm_setVerbose(5); 
		}  else if (strcmp(argv[i],"-verbose")==0) {
          gVerbosity = atoi(argv[++i]);
          sm_setVerbose(gVerbosity); 
        } else if (strcmp(argv[i],"-stereo")==0) {
			iStereo = SM_FLAGS_STEREO;
		} else if (strcmp(argv[i],"-filter")==0) {
			iFilter = 1;
		} else if (strcmp(argv[i],"-rle")==0) {
			iRLE = 1;
		} else if (strcmp(argv[i],"-gz")==0) {
			iRLE = 2;
		} else if (strcmp(argv[i],"-lzo")==0) {
			iRLE = 3;
		} else if (strcmp(argv[i],"-jpg")==0) {
			iRLE = 4;
		} else if ((strcmp(argv[i],"-threads")==0) && (i+1 < argc))  {
			i++; nThreads = atoi(argv[i]);
		} else if ((strcmp(argv[i],"-mipmaps")==0) && (i+1 < argc))  {
			i++; nRes = atoi(argv[i]);
			if (nRes < 1) nRes = 1;
			if (nRes > 8) nRes = 8;
		} else if ((strcmp(argv[i],"-jqual")==0) && (i+1 < argc))  {
			i++; iQual = atoi(argv[i]);
		} else if ((strcmp(argv[i],"-FPS")==0) && (i+1 < argc))  {
			i++; fFPS = atof(argv[i]);
		} else if ((strcmp(argv[i],"-src")==0) && (i+4 < argc))  {
			i++; src[0] = atoi(argv[i]);
			i++; src[1] = atoi(argv[i]);
			i++; src[2] = atoi(argv[i]);
			i++; src[3] = atoi(argv[i]);
			if (src[0] < 0) cmdline(argv[0]);
			if (src[1] < 0) cmdline(argv[0]);
			if (src[2] <= 0) cmdline(argv[0]);
			if (src[3] <= 0) cmdline(argv[0]);
		} else if ((strcmp(argv[i],"-dst")==0) && (i+2 < argc))  {
			i++; dst[0] = atoi(argv[i]);
			i++; dst[1] = atoi(argv[i]);
			if (dst[0] <= 0) cmdline(argv[0]);
			if (dst[1] <= 0) cmdline(argv[0]);
		} else if (strcmp(argv[i],"-tilesizes")==0) {
		  if ((argc-i) > 1) {
		    strcpy(tsizestr,argv[++i]);
		    tiled = 1;
		  } else {
		    cmdline(argv[0]);
		  }
		} else {
			fprintf(stderr,"Unknown option: %s\n\n",argv[i]);
			cmdline(argv[0]);
		}
		i++;
	}

	// parse tile sizes 
	{
	  char *tok;
	  char *str;
   
	  int count = 0;
	  int parsed = 0;
	  int xsize,ysize;

	  str = &tsizestr[0];
   
	  tok = strtok(str,(const char *)",");
	  while(tok != NULL) {
	    parsed=1;

	    xsize = ysize = 0;
	    if(strchr(tok,'x')) {
	      sscanf(tok,"%dx%d",&xsize,&ysize);
	      if((xsize > 0)&&(ysize > 0)){
		tsizes[count][0] = xsize;
		tsizes[count][1] = ysize;
		count++;
	      }
	    }
	    else {
	      sscanf(tok,"%d",&xsize);
	      if(xsize > 0) {
		ysize = xsize;
		tsizes[count][0] = xsize;
		tsizes[count][1] = ysize;
		count++;
	      }
	    }
     
	    tok = strtok((char*)NULL,(const char *)",");
     
	  }
	  while( (count < nRes) && parsed) {
	    if(count == 0)
	      break;
	    tsizes[count][0] = tsizes[count-1][0];
	    tsizes[count][1] = tsizes[count-1][1];
	    count++; 
	  }

	  if(parsed && gVerbosity) {
	    for(int n=0; n< nRes; n++) {
	      fprintf(stderr,"Resolution[%d] Tilesize=[%dx%d]\n",n,tsizes[n][0],tsizes[n][1]);
	    }
	  }
	}

	// input filenames
	if ((argc - i) < 2) cmdline(argv[0]);
	sOutput = argv[i++];

	smBase::init();

	ninputs = argc-i;
	input = (inp *)malloc(sizeof(inp)*ninputs);
	if (!input) exit(1);

	count = 0;
	for(j=i;j<argc;j++) {
		char	*p;
		int	has_at = 0;

		strcpy(input[j-i].name,argv[j]);
		p = strchr(input[j-i].name,'@');
		if (p) {
			*p = '\0';
			has_at = 1;
		}
	        input[j-i].sm = smBase::openFile(input[j-i].name,nThreads);
		if (input[j-i].sm == NULL) {
			fprintf(stderr,"Unable to open: %s\n",
				input[j-i].name);
			exit(1);
		}
		if (fFPS == 0.0) fFPS = input[j-i].sm->getFPS();

		/* cook up defaults */
		input[j-i].first = 0;
		input[j-i].last = input[j-i].sm->getNumFrames()-1;
		input[j-i].step = 1;
		input[j-i].num = input[j-i].last + 1;
		if (j == i) {
			iSize[0] = input[j-i].sm->getWidth();
			iSize[1] = input[j-i].sm->getHeight();
		} else {
			if ((iSize[0] != input[j-i].sm->getWidth()) ||
			    (iSize[1] != input[j-i].sm->getHeight())) {
				fprintf(stderr,"Error the file %s has a different framesize\n",input[j-i].name);
				exit(1);
			}
		}

		/* handle the @first[@last[@step]] syntax */
		if (has_at) {
			p++;
			input[j-i].first = atoi(p);
			p = strchr(p,'@');
			if (p) {
				p++;
				input[j-i].last = atoi(p);
				p = strchr(p,'@');
				if (p) {
					p++;
					input[j-i].step = atoi(p);
				}
			}
		}

		/* first/last/step error checking */
		if ((input[j-i].first < 0) || 
                    (input[j-i].first >= input[j-i].sm->getNumFrames())) {
			fprintf(stderr,"Invalid first frame %d for %s\n",
				input[j-i].first,input[j-i].name);
			exit(1);
		}
		if ((input[j-i].last < 0) ||
                    (input[j-i].last >= input[j-i].sm->getNumFrames())) {
			fprintf(stderr,"Invalid last frame %d for %s\n",
				input[j-i].last,input[j-i].name);
			exit(1);
		}
		if ((input[j-i].step == 0) || 
		    ((input[j-i].step > 0) && 
                     (input[j-i].last < input[j-i].first)) ||
		    ((input[j-i].step < 0) && 
                     (input[j-i].last > input[j-i].first))) {
			fprintf(stderr,"Invalid first/last/step values (%d/%d/%d)\n",
			    input[j-i].first,input[j-i].last,input[j-i].step);
			exit(1);
		}

		/* count the files */
		input[j-i].num = 0;
		if (input[j-i].step > 0) {
			for(k=input[j-i].first;k<=input[j-i].last;
				k+=input[j-i].step) {
				input[j-i].num++;
			}
		} else {
			for(k=input[j-i].first;k>=input[j-i].last;
				k+=input[j-i].step) {
				input[j-i].num++;
			}
		}

		if (gVerbosity) {
			printf("Input: %d - %d of %s - size %d %d\n",
				input[j-i].first,input[j-i].last,
				input[j-i].name,iSize[0],iSize[1]);
		}
		count += input[j-i].num;
	}

	// Ensure the bounds are good...
	if (src[0] < 0) {
		src[0] = 0;
		src[1] = 0;
		src[2] = iSize[0];
		src[3] = iSize[1];
	} else {
		if (src[0] + src[2] > iSize[0]) {
			fprintf(stderr,"Invalid selection width\n");
			exit(1);
		}
		if (src[1] + src[3] > iSize[1]) {
			fprintf(stderr,"Invalid selection width\n");
			exit(1);
		}
	}
	// if no value, dst is equal to the src
	if (dst[0] < 0) {
		dst[0] = src[2];
		dst[1] = src[3];
	}
	// ok, here we go...
	iScale = 0;
	if (iSize[0] != dst[0]) { iSize[0] = dst[0]; iScale = 1; }
	if (iSize[1] != dst[1]) { iSize[1] = dst[1]; iScale = 1; }

	// memory for scaling buffer...
	if (iScale) {
		pZoom = (void *)malloc(iSize[0]*iSize[1]*3);
		if (!pZoom) exit(1);
	}

	// Open the output file...
	if(tiled) {
	  if (iRLE == 1) {
	    sm = smRLE::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes, nThreads);
	  } else if (iRLE == 2) {
	    sm = smGZ::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes, nThreads);
	  } else if (iRLE == 3) {
	    sm = smLZO::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes, nThreads);
	  } else if (iRLE == 4) {
	    sm = smJPG::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes, nThreads);
	    ((smJPG *)sm)->setQuality(iQual);
	  } else {
	    sm = smRaw::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes, nThreads);
	  }
	  if (!sm) {
	    fprintf(stderr,"Unable to create the file: %s\n",
		    sOutput);
	    exit(1);
	  }
	}
	else {
	  if (iRLE == 1) {
	    sm = smRLE::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes, nThreads);
	  } else if (iRLE == 2) {
	    sm = smGZ::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes, nThreads);
	  } else if (iRLE == 3) {
	    sm = smLZO::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes, nThreads);
	  } else if (iRLE == 4) {
	    sm = smJPG::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes, nThreads);
	    ((smJPG *)sm)->setQuality(iQual);
	  } else {
	    sm = smRaw::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes, nThreads);
	  }
	  if (!sm) {
	    fprintf(stderr,"Unable to create the file: %s\n",
		    sOutput);
	    exit(1);
	  }
	}

	/* set any flags */
	sm->setFlags(iStereo);
	sm->setFPS(fFPS);
    sm->startWriteThread(); 

	/* init the parallel tools */
#ifdef irix
	pthread_setconcurrency(nThreads*2);
#endif
	pt_pool_init(pool, nThreads, nThreads*2, 0);

	/* copy the frames */
	i = 0;
	for(j=0;j<ninputs;j++) {
		int	x;
		if (gVerbosity) {
			printf("Reading from %s (%d to %d by %d)\n",
			   input[j].name,input[j].first,input[j].last,
				input[j].step);
		}
		k = input[j].first;
		for(x=0;x<input[j].num;x++) {
			if (gVerbosity) {
				printf("Working on %d of %d\n",i+1,count);
			}
			if (nThreads == 1 && (input[j].sm->getType() == sm->getType()) && 
					(iScale == 0) && 
		(sm->getNumResolutions() == input[j].sm->getNumResolutions())) {
				int	size,res;
				for(res=0;res<sm->getNumResolutions();res++) {
                  input[j].sm->getCompFrame(k,0,NULL,size,res);
				    if (buffer_len < size) {
					free(buffer);
					buffer = (void *)malloc(size);
					buffer_len = size;
				    }
				    input[j].sm->getCompFrame(k,0,buffer,size,res);
				    sm->writeCompFrame(i,buffer,&size,res);
				}
				i++;
			} else {
				Work *wrk = new Work;
				wrk->insm = input[j].sm;
				wrk->sm = sm;
				wrk->inframe = k;
				wrk->outframe = i++;
				wrk->iScale = iScale;
				wrk->iFilter = iFilter;
				wrk->dst = dst;
				wrk->src = src;
				pt_pool_add_work(pool, workproc, wrk);
			}
			k += input[j].step;
		}
	}
	pt_pool_destroy(pool,1);
    sm->stopWriteThread(); 
    sm->flushFrames(true); 
	sm->closeFile();

	/* clean up */
	delete sm;
	for(i=0;i<ninputs;i++) delete input[i].sm;
	free(input);
	if (pZoom) free(pZoom);
	if (buffer) free(buffer);

	exit(0);
}

void workproc(void *arg)
{
	Work *wrk = (Work *)arg;

	int	sizein = 3*wrk->insm->getHeight()*wrk->insm->getWidth();
	int	sizeout = 3*wrk->sm->getHeight()*wrk->sm->getWidth();
	int	size;
	wrk->buffer = new unsigned char[sizein];
    dbprintf(4, "created new buffer %p for frame %d\n", 
             wrk->buffer, wrk->outframe); 
	wrk->insm->getFrame(wrk->inframe,wrk->buffer, pt_pool_threadnum());
	if (wrk->iScale) {
		unsigned char *pZoom = new unsigned char[sizeout];
		Sample2d(wrk->buffer,
			 wrk->insm->getWidth(),
			 wrk->insm->getHeight(),
			 pZoom,
			 wrk->dst[0],wrk->dst[1],wrk->src[0],wrk->src[1],
			 wrk->src[2],wrk->src[3],wrk->iFilter);
        dbprintf(4, "workproc deleting new buffer %p, replacing with %p for frame %d\n", wrk->buffer, pZoom, wrk->outframe); 
		delete wrk->buffer;
		wrk->buffer = pZoom;
	}
    //bool writeOK = (pt_pool_threadnum() == 0) ;
	//wrk->sm->bufferFrame(wrk->outframe,wrk->buffer, writeOK);
    /*   while (!wrk->sm->bufferReady(wrk->outframe))  {
      // buffer cannot take this frame yet, needs to get written and swapped
      usleep (1000); 
    }
    */
    //wrk->sm->bufferFrame(wrk->frame,wrk->buffer, writeOK);
    wrk->sm->compressAndBufferFrame(wrk->outframe,wrk->buffer);
	// do not delete wrk->buffer; sm will delete when finished
	delete wrk;
}

static void smoothx(unsigned char *image, int dx, int dy)
{
        register int x,y;
	int	p1[3],p2[3],p3[3];

/* smooth along X scanlines using 242 kernel */
        for(y=0;y<dy;y++) {
                p1[0] = image[(y*dx)*3+0];
                p1[1] = image[(y*dx)*3+1];
                p1[2] = image[(y*dx)*3+2];

                p2[0] = image[((y*dx)+1)*3+0];
                p2[1] = image[((y*dx)+1)*3+1];
                p2[2] = image[((y*dx)+1)*3+2];

                for(x=1;x<dx-1;x++) {
                        p3[0] = image[((y*dx)+x+1)*3+0];
                        p3[1] = image[((y*dx)+x+1)*3+1];
                        p3[2] = image[((y*dx)+x+1)*3+2];

                        image[((y*dx)+x)*3+0]=((p1[0]*2)+(p2[0]*4)+(p3[0]*2))/8;
                        image[((y*dx)+x)*3+1]=((p1[1]*2)+(p2[1]*4)+(p3[1]*2))/8;
                        image[((y*dx)+x)*3+2]=((p1[2]*2)+(p2[2]*4)+(p3[2]*2))/8;

                        p1[0] = p2[0];
                        p1[1] = p2[1];
                        p1[2] = p2[2];

                        p2[0] = p3[0];
                        p2[1] = p3[1];
                        p2[2] = p3[2];
                }
        }
        return;
}

static void smoothy(unsigned char *image, int dx, int dy)
{
        register int x,y;
	int	p1[3],p2[3],p3[3];

/* smooth along Y scanlines using 242 kernel */
        for(x=0;x<dx;x++) {
                p1[0] = image[x*3+0];
                p1[1] = image[x*3+1];
                p1[2] = image[x*3+2];

                p2[0] = image[(x+dx)*3+0];
                p2[1] = image[(x+dx)*3+1];
                p2[2] = image[(x+dx)*3+2];

                for(y=1;y<dy-1;y++) {
                        p3[0] = image[((y*dx)+x+dx)*3+0];
                        p3[1] = image[((y*dx)+x+dx)*3+1];
                        p3[2] = image[((y*dx)+x+dx)*3+2];

                        image[((y*dx)+x)*3+0]=((p1[0]*2)+(p2[0]*4)+(p3[0]*2))/8;
                        image[((y*dx)+x)*3+1]=((p1[1]*2)+(p2[1]*4)+(p3[1]*2))/8;
                        image[((y*dx)+x)*3+2]=((p1[2]*2)+(p2[2]*4)+(p3[2]*2))/8;

                        p1[0] = p2[0];
                        p1[1] = p2[1];
                        p1[2] = p2[2];

                        p2[0] = p3[0];
                        p2[1] = p3[1];
                        p2[2] = p3[2];
                }
        }
        return;
}

void Sample2d(unsigned char *in,int idx,int idy,
        unsigned char *out,int odx,int ody,
        int s_left,int s_top,int s_dx,int s_dy,int filter)
{
        register double xinc,yinc,xp,yp;
        register int x,y;
        register int i,j;
        register int ptr;

        xinc = (double)s_dx / (double)odx;
        yinc = (double)s_dy / (double)ody;

/* prefilter if decimating */
        if (filter) {
                if (xinc > 1.0) smoothx(in,idx,idy);
                if (yinc > 1.0) smoothy(in,idx,idy);
        }
/* resample */
        ptr = 0;
        yp = s_top;
        for(y=0; y < ody; y++) {  /* over all scan lines in output image */
                j = (int)yp;
                xp = s_left;
                for(x=0; x < odx; x++) {  /* over all pixel in each scanline of
output */
                        i = (int)xp;
			i = (i+(j*idx))*3;
                        out[ptr++] = in[i++];
                        out[ptr++] = in[i++];
                        out[ptr++] = in[i++];
                        xp += xinc;
                }
                yp += yinc;
        }
/* postfilter if magnifing */
        if (filter) {
                if (xinc < 1.0) smoothx(out,odx,ody);
                if (yinc < 1.0) smoothy(out,odx,ody);
        }
        return;
}  
