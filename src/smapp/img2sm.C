/*
** $RCSfile: img2sm.C,v $
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
** $Id: img2sm.C,v 1.3 2008/07/08 02:43:20 dbremer Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/
#define DMALLOC 1
#undef DMALLOC

// Utility to combine image files into movie
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sm/smLZO.h"
#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smRaw.h"
#include "sm/smJPG.h"

#include "zlib.h"
#include "pngsimple.h"


#include "libimage/sgilib.h"
#include "../libpnmrw/libpnmrw.h"
#include "simple_jpeg.h"
#include "../config/version.h"

#include "pt/pt.h"
#define int32 int32hack
extern "C" {
#include <tiff.h>
#include <tiffio.h>
}
#undef int32
// Prototypes 
int rotate_img(unsigned char *img,int dx,int dy,float rot);
void rotate_dims(float rot,int *dx,int *dy);
void flipx(unsigned char *img,int dx,int dy);
void flipy(unsigned char *img,int dx,int dy);
void cmdline(char *app);
int check_if_png(char *file_name,int *size);
int read_png_image(char *file_name,int *iSize,unsigned char *buf);

struct Work {
  char filename[2048];
  smBase *sm;
  int frame;
  int filetype; 
  u_char *buffer; // allocate externally to permit copyless buffering
  int iFlipx;
  int iFlipy;
  int *iInDims;
  float	fRot;
};

// GLOBALS FOR NOW
//unsigned char *gInputBuf = NULL; 
int	iInDims[4];
unsigned short	bitsPerSample, tiffPhoto;
int	iMin = -1;
int	iMax = -1;
int	iSize[4] = {-1,-1,0,0};
int	iVerb = 0;
int	iPlanar = 0;
//unsigned short	*rowbuf = NULL;


#ifdef DMALLOC
#include <dmalloc.h>
#endif


#define CHECK(v) \
if(v == NULL) \
img2sm_fail_check(__FILE__,__LINE__)

void img2sm_fail_check(char *file,int line)
{
  perror("fail_check");
  fprintf(stderr,"Failed at line %d in file %s\n",line,file);
  exit(1); 
}

/*!
  To encapsulate what each worker needs to process its data
*/ 
struct WorkerData {
  int threadNum, numThreads, startFrame, endFrame, frameStep; 
}; 

void workerThreadFunction(void *workerData) {
  WorkerData *myData = (WorkerData*)workerData; 
  //FillInputBuffer(myData); 
  return; 
}



void workproc(void *arg);

void cmdline(char *app)
{  
  fprintf(stderr,"%s (%s) usage: %s [options] template output\n",
          basename(app), BLOCKBUSTER_VERSION, basename(app));
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"\t-rle Selects RLE compression\n");
  fprintf(stderr,"\t-gz Selects gzip compression\n");
  fprintf(stderr,"\t-lzo Selects LZO compression\n");
  fprintf(stderr,"\t-jpg Selects JPG compression\n");
  fprintf(stderr,"\t-jqual [qual] Selects JPG quality (default:75)\n");
  fprintf(stderr,"\t-minmax [min] [max]  16bit TIFF min and max values (default: from file)\n");
  fprintf(stderr,"\t-flipx Flip the image over the x axis\n");
  fprintf(stderr,"\t-flipy Flip the image over the y axis\n");
  fprintf(stderr,"\t-rotate [ang] Angle to rotate source (0,90,180,270) (default:0)\n");
  fprintf(stderr,"\t-size [width height depth header] Select raw img dims\n");
  fprintf(stderr,"\t-first [first] First image number.  Default is search from 0.\n");
  fprintf(stderr,"\t-last [last] Last image number. Default is to search.\n");
  fprintf(stderr,"\t-step [step] Image number step.  Default is 1.\n");
  fprintf(stderr,"\t-fps [fps] Set preferred frame rate.  Default is 30, max is 50.  (-FPS is deprecated) \n");
  fprintf(stderr,"\t-buffersize [nt] Number of frames to buffer.  Default is 200.  Using lower values saves memory but may decrease performance, and vice versa.\n");
  fprintf(stderr,"\t-threads [nt] Number of threads to use. Default is 1.\n");
  fprintf(stderr,"\t-mipmaps [n] Number of mipmap levels. Default is 1.\n");
  fprintf(stderr,"\t-v Verbose mode (same as -verbose 1).\n");
  fprintf(stderr,"\t-verbose n Sets verbose level to n.\n");
  fprintf(stderr,"\t-stereo Specify the output file is L/R stereo.\n");
  fprintf(stderr,"\t-form [\"tiff\"|\"raw\"|\"sgi\"|\"pnm\"|\"png\"|\"jpg\"] Selects the input file format\n");
  fprintf(stderr,"\t-planar Raw img is planar interleaved (default: pixel interleave).\n");
  fprintf(stderr,"\t-tilesizes Specify tile sizes per mipmap level. Comma separated.  Non-square tiles can be given as e.g. 128x512. A single value would apply to all mipmaps (default: 512).\n");
  fprintf(stderr,"\t-ignore Allows invalid templates.\n");
  exit(1);
}

//=================================================
void FillInputBuffer(Work *wrk) {
  
  FILE		*fp = NULL;
  TIFF		*tiff = NULL;
  sgi_t		*libi = NULL;
  
  // read the file...
  switch(wrk->filetype) {
  case 0: // TIFF
    {
      uint32 *temp;
      unsigned int w, h;
      tiff = TIFFOpen(wrk->filename,"r");
      if (!tiff) {
        fprintf(stderr,"Error: Unable to open TIFF: %s\n",wrk->filename);
        exit(1);
      }
      TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
      TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
      if ((w != iInDims[0]) || (h != iInDims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename);
        exit(1);
      }
      
      if (bitsPerSample == 16) {
        if (iMin == -1) {
          unsigned short tt;
          TIFFGetFieldDefaulted(tiff, 
                                TIFFTAG_MINSAMPLEVALUE, &tt);
          iMin = tt;
        }
        if (iMax == -1) {
          unsigned short tt;
          TIFFGetFieldDefaulted(tiff, 
                                TIFFTAG_MAXSAMPLEVALUE, &tt);
          iMax = tt;
        }
        temp = (uint32 *)malloc(iInDims[0]*iSize[2]*2);
        CHECK(temp);
        unsigned short *ss = (unsigned short *)temp;
        int x,y;
        float mult = 255.0/(float)(iMax-iMin);
        unsigned char	*p = wrk->buffer;
        unsigned int mm[2] = {200000,0};
        for(y=0;y<iInDims[1];y++) {
          TIFFReadScanline(tiff,(unsigned char *)ss,y,0);
          for(x=0;x<iInDims[0]*iSize[2];x++) {
            if (ss[x] < mm[0]) mm[0] = ss[x];
            if (ss[x] > mm[1]) mm[1] = ss[x];
            if (ss[x] < iMin) ss[x] = iMin;
            if (ss[x] > iMax) ss[x] = iMax;
          }
          for(x=0;x<iInDims[0];x++) {
            if (iSize[2] == 1) {
              unsigned char tt=(unsigned char)((ss[x]-iMin)*mult);
              if (tiffPhoto==PHOTOMETRIC_MINISWHITE) {
                tt=255-tt;
              }
              *p++ = tt;
              *p++ = tt;
              *p++ = tt;
            } else {
              *p++ = (unsigned char)((ss[x*3+0]-iMin)*mult);
              *p++ = (unsigned char)((ss[x*3+1]-iMin)*mult);
              *p++ = (unsigned char)((ss[x*3+2]-iMin)*mult);
            }
          }
        }
        if (iVerb) {
          printf("Image min,max=%d %d\n",mm[0],mm[1]);
        }
      } else {
        temp = (uint32 *)malloc(iInDims[0]*iInDims[1]*4);  // Caution deferred free under IRIX
        CHECK(temp);
        TIFFReadRGBAImage(tiff,w,h,temp,0);
        unsigned char	*p = wrk->buffer;
        for(int x=0;x<w*h;x++) {
          *p++ = TIFFGetR(temp[x]);
          *p++ = TIFFGetG(temp[x]);
          *p++ = TIFFGetB(temp[x]);
        }
      }
      
      TIFFClose(tiff);
      free(temp);
    }
    break;
  case 1: // SGI
    {
      unsigned char	*p = wrk->buffer;
      unsigned short *rowbuf = new unsigned short[iInDims[0]];
      CHECK(rowbuf);
      libi = sgiOpen(wrk->filename,SGI_READ,0,0,0,0,0);
      if (!libi) {
        fprintf(stderr,"Error: Unable to open SGI: %s\n",wrk->filename);
        exit(1);
      }
      if ((libi->xsize != iInDims[0]) || 
          (libi->ysize != iInDims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename);
        exit(1);
      }
      for(unsigned int y=0;y<iInDims[1];y++) {
        int	x;
        if (iSize[2] >= 3) {
          sgiGetRow(libi,rowbuf,y,0);
          for(x=0;x<iInDims[0];x++) {
            p[x*3+0] = rowbuf[x];
          }
          sgiGetRow(libi,rowbuf,y,1);
          for(x=0;x<iInDims[0];x++) {
            p[x*3+1] = rowbuf[x];
          }
          sgiGetRow(libi,rowbuf,y,2);
          for(x=0;x<iInDims[0];x++) {
            p[x*3+2] = rowbuf[x];
          }
        } else {
          sgiGetRow(libi,rowbuf,y,0);
          for(x=0;x<iInDims[0];x++) {
            p[x*3+0] = rowbuf[x];
            p[x*3+1] = rowbuf[x];
            p[x*3+2] = rowbuf[x];
          }
        }
        p += 3*iInDims[0];
      }
      sgiClose(libi);
      delete rowbuf; 
    }
    break;
  case 2: // RAW
    {
      unsigned char	*p = wrk->buffer;
      
      gzFile	fpz;
      fpz = gzopen(wrk->filename,"r");
      if (!fpz) {
        fprintf(stderr,"Error: Unable to open RAW compressed: %s\n",wrk->filename);
        exit(1);
      }
      // Header
      if (iSize[3]) {
        void *b = malloc(iSize[3]);
        CHECK(b);
        gzread(fpz,b,iSize[3]);
        free(b);
      }
      // Scan lines
      if (iPlanar && (iSize[2] == 3)) {
        char *b = (char *)malloc(iInDims[0]*iInDims[1]*3);
        CHECK(b);
        char *p0 = b;
        char *p1 = p0 + iInDims[0]*iInDims[1];
        char *p2 = p1 + iInDims[0]*iInDims[1];
        gzread(fpz,b,iInDims[0]*iInDims[1]*3);
        for(int x=0;x<iInDims[0]*iInDims[1];x++) {
          *p++ = *p0++;
          *p++ = *p1++;
          *p++ = *p2++;
        }
        free(b); 
      } else {
        for(unsigned int y=0;y<iInDims[1];y++) {
          int  x;
          if (iSize[2] == 3) {
            gzread(fpz,p,iInDims[0]*3);
          } else {
            gzread(fpz,p,iInDims[0]);
            for(x=iInDims[0]-1;x>=0;x--) {
              p[x*3+2] = p[x];
              p[x*3+1] = p[x];
              p[x*3+0] = p[x];
            }
          }
          p += 3*iInDims[0];
        }
      }
      // done
      gzclose(fpz);
    }
    break;
  case 3: // PNM
    {  
      int 	dx,dy,fmt,f;
      xelval	value;
      
      unsigned char	*p = wrk->buffer;
      
      fp = pm_openr(wrk->filename);
      if (!fp) {
        fprintf(stderr,"Unable to open the file: %s\n",wrk->filename);
        exit(1);
      }
      if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
        fprintf(stderr,"The file is not in PNM format: %s.\n",wrk->filename);
        pm_closer(fp);
        exit(1);
      }
      if ((dx != iInDims[0]) || 
          (dy != iInDims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename);
        pm_closer(fp);
        exit(1);
      }
      
      if (PNM_FORMAT_TYPE(fmt) == PPM_TYPE) {
        f = 3;
      } else if(PNM_FORMAT_TYPE(fmt) == PGM_TYPE) {
        f = 1;
      } else {
        fprintf(stderr,"Error: file type not supported:%s\n",wrk->filename);
        pm_closer(fp);
        exit(1);
      }
      if (f != iSize[2]) {
        fprintf(stderr,"Error: file type changed:%s\n",wrk->filename);
        pm_closer(fp);
        exit(1);
      }
      
      xel*	xrow;
      xel*	xp;
      int	rp,gp,bp;
      
#define NORM(x,mx) ((float)(x)/(float)(mx))*255.0
      
      xrow = pnm_allocrow( dx );
      for(int y=0;y<dy;y++) {
        p = wrk->buffer + (dy-y-1)*dx*3;
        pnm_readpnmrow( fp, xrow, dx, value, fmt );
        if (iSize[2] == 3) {
          xp = xrow;
          for(int x=0;x<dx;x++) {
            rp = (int)(NORM(PPM_GETR(*xp),value));
            gp = (int)(NORM(PPM_GETG(*xp),value));
            bp = (int)(NORM(PPM_GETB(*xp),value));
            xp++;
            *p++ = rp;
            *p++ = gp;
            *p++ = bp;
          }
        } else {
          xp = xrow;
          for(int x=0;x<dx;x++) {
            rp = (int)(NORM(PNM_GET1(*xp),value));
            xp++;
            *p++ = rp;
            *p++ = rp;
            *p++ = rp;
          }
        }
      }
      pnm_freerow(xrow);
      
      pm_closer(fp);
    }
    break;
  case 4: // PNG
    if (!read_png_image(wrk->filename,iInDims,wrk->buffer)) {
      fprintf(stderr,"Unable to read PNG file: %s\n",wrk->filename);
      exit(1);
    }
    break;
  case 5: // JPEG
    if (!read_jpeg_image(wrk->filename,iInDims,wrk->buffer)) {
      fprintf(stderr,"Unable to read JPEG file: %s\n",wrk->filename);
      exit(1);
    }
    break;
  }
  return; 
} // end FillInputBuffer()

int main(int argc,char **argv)
{
  int	iRLE = 0;
  int	iType = -1;
  char		*sTemplate = NULL;
  char		*sOutput = NULL;
  int	iFlipx = 0;
  int	iFlipy = 0;
  int	iStereo = 0;
  int	iQual = 75;
  int	iIgnore = 0;
  float		fFPS = 30.0;
  float		fRotate = 0.0;
  int           bufferSize = 100;
  int           nThreads = 1;
  int		nRes = 1;

  int	iStart=-1,iEnd=-1,iStep=1;
  int	i,count;
  FILE		*fp = NULL;
  char		filename[1024],filename2[1024];
  char          tsizestr[1024] = "512";
  unsigned int  tsizes[8][2];
  int tiled = 1;
  //unsigned char	*inputBuf = NULL;
  smBase 		*sm = NULL;

  TIFF		*tiff = NULL;
  sgi_t		*libi = NULL;
  unsigned short spp;

  /* parse the command line ... */
  i = 1;
  while ((i<argc) && (argv[i][0] == '-')) {
    if (strcmp(argv[i],"-rle")==0) {
      iRLE = 1;
    } else if (strcmp(argv[i],"-gz")==0) {
      iRLE = 2;
    } else if (strcmp(argv[i],"-lzo")==0) {
      iRLE = 3;
    } else if (strcmp(argv[i],"-jpg")==0) {
      iRLE = 4;
    } else if (strcmp(argv[i],"-planar")==0) {
      iPlanar = 1;
    } else if (strcmp(argv[i],"-v")==0) {
      iVerb = 1;
    } else if (strcmp(argv[i],"-verbose")==0) {
      iVerb = atoi(argv[++i]);
      sm_setVerbose(iVerb); 
    } else if (strcmp(argv[i],"-ignore")==0) {
      iIgnore = 1;
    } else if (strcmp(argv[i],"-flipx")==0) {
      iFlipx = 1;
    } else if (strcmp(argv[i],"-flipy")==0) {
      iFlipy = 1;
    } else if (strcmp(argv[i],"-stereo")==0) {
      iStereo = SM_FLAGS_STEREO;
    } else if ((strcmp(argv[i],"-rotate")==0) && (i+1 < argc))  {
      i++; fRotate = atof(argv[i]);
      if ((fRotate != 0) && (fRotate != 90) && 
          (fRotate != 180) && (fRotate != 270)) {
         fprintf(stderr,"Invalid rotation: %f.  Only 0,90,180,270 allowed.\n",
	 	fRotate);
      }
    } else if ((strcmp(argv[i],"-mipmaps")==0) && (i+1 < argc))  {
      i++; nRes = atoi(argv[i]);
      if (nRes < 1) nRes = 1;
      if (nRes > 8) nRes = 8;
    } else if ((strcmp(argv[i],"-buffersize")==0) && (i+1 < argc))  {
      i++; bufferSize = atoi(argv[i]);
    } else if ((strcmp(argv[i],"-threads")==0) && (i+1 < argc))  {
      i++; nThreads = atoi(argv[i]);
    } else if (strcmp(argv[i],"-form")==0) {
      if ((argc-i) > 1) {
	if (strcmp(argv[i+1],"tiff") == 0) {
	  iType = 0;
	} else if (strcmp(argv[i+1],"sgi") == 0) {
	  iType = 1;
	} else if (strcmp(argv[i+1],"raw") == 0) {
	  iType = 2;
	} else if (strcmp(argv[i+1],"pnm") == 0) {
	  iType = 3;
	  pnm_init2(argv[0]);
	} else if (strcmp(argv[i+1],"png") == 0) {
	  iType = 4;
	} else if (strcmp(argv[i+1],"jpg") == 0) {
	  iType = 5;
	} else {
	  fprintf(stderr,"Invalid format: %s\n",
		  argv[i+1]);
	  exit(1);
	}
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-jqual")==0) {
      if ((argc-i) > 1) {
	iQual = atoi(argv[i+1]);
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if ((strcmp(argv[i],"-FPS")==0) || (strcmp(argv[i],"-fps")==0)) {
      if ((argc-i) > 1) {
	fFPS = atof(argv[i+1]);
	if (fFPS > 50) {
	  fprintf(stderr,"50 FPS max.\n");
	  exit(1);
	}
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-minmax")==0) {
      if ((argc-i) > 2) {
	iMin = atoi(argv[i+1]);
	iMax = atoi(argv[i+2]);
	i += 2;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-first")==0) {
      if ((argc-i) > 1) {
	iStart = atoi(argv[i+1]);
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-last")==0) {
      if ((argc-i) > 1) {
	iEnd = atoi(argv[i+1]);
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-step")==0) {
      if ((argc-i) > 1) {
	iStep = atoi(argv[i+1]);
	i++;
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-size")==0) {
      if ((argc-i) > 4) {
	iSize[0] = atoi(argv[++i]);
	iSize[1] = atoi(argv[++i]);
	iSize[2] = atoi(argv[++i]);
	iSize[3] = atoi(argv[++i]);
      } else {
	cmdline(argv[0]);
      }
    } else if (strcmp(argv[i],"-tilesizes")==0) {
      if ((argc-i) > 1) {
	strcpy(tsizestr,argv[++i]);
	tiled = 1;
      } else {
	cmdline(argv[0]);
      }
    } else {
      fprintf(stderr,"Unknown option: %s\n", argv[i]);
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

    if(parsed && iVerb) {
      for(int n=0; n< nRes; n++) {
	fprintf(stderr,"Resolution[%d] Tilesize=[%dx%d]\n",n,tsizes[n][0],tsizes[n][1]);
      }
    }
  }
  if ((argc - i) != 2) cmdline(argv[0]);

  // get the arguments
  sTemplate = argv[i];
  sOutput = argv[i+1];

  if (iStep == 0) {
    fprintf(stderr,"Invalid Step parameter (%d)\n",iStep);
    exit(1);
  }

  // Bad template name?
  if (!iIgnore) {
    sprintf(filename,sTemplate,0);
    sprintf(filename2,sTemplate,1);
    if (strcmp(filename,filename2) == 0) {
      fprintf(stderr,"Invalid sprintf filename template: %s\n",
              sTemplate);
      exit(1);
    }
  }

  // count the files...
  if (iStart < 0) {
    i = 0;
    while (i < 10000) {
      sprintf(filename,sTemplate,i);
      fp = fopen(filename,"r");
      if (fp) break;
      i += 1;
    }
    if (!fp) {
      fprintf(stderr,"Unable to find initial file: %s\n",
	      sTemplate);
      exit(1);
    }
    fclose(fp);
    iStart = i;
    i += 1;
  } else {
    i = iStart + iStep;
  }
  while (iEnd < 0) {
    sprintf(filename,sTemplate,i);
    fp = fopen(filename,"r");
    if (fp) {
      fclose(fp);
      i += iStep;
    } else {
      iEnd = i - iStep;
    }
  }

  // Check the file type
  sprintf(filename,sTemplate,iStart);
  if (iType == -1) {
    if (tiff = TIFFOpen(filename,"r")) {
      if (iVerb) fprintf(stderr,"TIFF input format detected\n");
      iType = 0;
      TIFFClose(tiff);
    } else if (libi = sgiOpen(filename,SGI_READ,0,0,0,0,0)) {
      if (iVerb) fprintf(stderr,"SGI input format detected\n");
      iType = 1;
      sgiClose(libi);
    } else if (check_if_png(filename,NULL)) { 
      if (iVerb) fprintf(stderr,"PNG input format detected\n");
      iType = 4; /* PNG */
    } else if (check_if_jpeg(filename,NULL)) { 
      if (iVerb) fprintf(stderr,"JPEG input format detected\n");
      iType = 5; /* JPEG */
    } else {
      if (iVerb) fprintf(stderr,"Assuming PNM format\n");
      iType = 2;
    }
  }
  // get the frame size
  if (iType == 0) {  // TIFF
    tiff = TIFFOpen(filename,"r");
    if (!tiff) {
      fprintf(stderr,"Error: %s is not a TIFF format file.\n",filename);
      exit(1);
    }
    if (!TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&bitsPerSample)) bitsPerSample = 1;
    if (!TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&spp)) spp = 1;
    TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &tiffPhoto);
    TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&(iSize[0]));
    TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&(iSize[1]));
    TIFFClose(tiff);
    if ((bitsPerSample != 8) && (bitsPerSample != 16)) {
      fprintf(stderr,"Only 8 and 16 bits/sample TIFF files allowed\n");
      exit(1);
    }
    if ((spp != 1) && (spp != 3)) {
      fprintf(stderr,"Only 1 and 3 sample TIFF files allowed\n");
      exit(1);
    }
    iSize[2] = spp;
  } else if (iType == 1) { // SGI

    libi = sgiOpen(filename,SGI_READ,0,0,0,0,0);
    if (!libi) {
      fprintf(stderr,"Error: %s is not an SGI libimage format file.\n",filename);
      exit(1);
    }
    iSize[0] = libi->xsize;
    iSize[1] = libi->ysize;
    iSize[2] = libi->zsize;
    sgiClose(libi);
  } else if (iType == 2) { // RAW
    if (iSize[0] < 0) cmdline(argv[0]);
    if ((iSize[2] != 1) && (iSize[2] != 3)) cmdline(argv[0]);
    if (iSize[0] < 0) cmdline(argv[0]);
    if (iSize[1] < 0) cmdline(argv[0]);
    if (iSize[3] < 0) cmdline(argv[0]);
  } else if (iType == 3) { // PNM
    int 	dx,dy,fmt;
    xelval	value;

    fp = pm_openr(filename);
    if (!fp) {
      fprintf(stderr,"Unable to open the file: %s\n",filename);
      exit(1);
    }
    if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
      fprintf(stderr,"The file is not in PNM format.\n");
      pm_closer(fp);
      exit(1);
    }
    pm_closer(fp);

    /* set up our params */
    iSize[2] = 1;
    if (PNM_FORMAT_TYPE(fmt) == PBM_TYPE) {
      fprintf(stderr,"Bitmap files are not supported.\n");
      exit(1);
    } else if (PNM_FORMAT_TYPE(fmt) == PPM_TYPE) {
      iSize[2] = 3;
    }
    iSize[0] = dx;
    iSize[1] = dy;
  } else if (iType == 4) { // PNG
    if (!check_if_png(filename,iSize)) {
      fprintf(stderr, "Could not open the first PNG image -- it is either corrupt, nonexistent, or not a PNG file\n"); 
      exit(1); 
    }
  } else if (iType == 5) { // JPEG
    if (!check_if_jpeg(filename,iSize)) {
      fprintf(stderr, "Could not open the first JPEG image -- it is either corrupt, nonexistent, or not a JPEG file\n"); 
      exit(1); 
    }
  }

  // count the files
  count = 0;
  for(i=iStart;;i+=iStep) {
    if ((iStep > 0) && (i>iEnd)) break;
    if ((iStep < 0) && (i<iEnd)) break;
    count++;
  }

  // we may need to swap the dims
  memcpy(iInDims,iSize,sizeof(iSize));
  rotate_dims(fRotate,iSize+0,iSize+1);

  // Open the sm file...
  smBase::init();
  if(tiled) {
    if (iRLE == 1) {
      sm = smRLE::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes);
    } else if (iRLE == 2) {
      sm = smGZ::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes);
    } else if (iRLE == 3) {
      sm = smLZO::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes);
    } else if (iRLE == 4) {
      sm = smJPG::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes);
      ((smJPG *)sm)->setQuality(iQual);
    } else {
      sm = smRaw::newFile(sOutput,iSize[0],iSize[1],count,&tsizes[0][0],nRes);
    }
  }
  else {
    if (iRLE == 1) {
      sm = smRLE::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes);
    } else if (iRLE == 2) {
      sm = smGZ::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes);
    } else if (iRLE == 3) {
      sm = smLZO::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes);
    } else if (iRLE == 4) {
      sm = smJPG::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes);
      ((smJPG *)sm)->setQuality(iQual);
    } else {
      sm = smRaw::newFile(sOutput,iSize[0],iSize[1],count,NULL,nRes);
    }
  }
  // set the flags...
  sm->setFlags(iStereo);
  sm->setFPS(fFPS);
  sm->setBufferSize(bufferSize); 
  /* init the parallel tools */

  
  pt_pool         thepool;
  pt_pool_t       pool = &thepool;
  pt_pool_init(pool, nThreads, nThreads*2, 0);
  sm->startWriteThread(); 
    
  // memory buffer for frame input
  /*gInputBuf = (unsigned char *)malloc(iSize[0]*iSize[1]*3L); 
    CHECK(gInputBuf);*/
  if (iVerb) {
    printf("Creating streaming movie file from...\n");
    printf("Template: %s\n",sTemplate);
    printf("First, Last, Step: %d %d %d\n",iStart,iEnd,iStep);
    printf("%d images of size %d %d\n",count,iSize[0],iSize[1]);
  }
  
  
  // Walk the input files...
  for(i=iStart;;i+=iStep) {

    // terminate??
    if ((iStep > 0) && (i>iEnd)) break;
    if ((iStep < 0) && (i<iEnd)) break;

    Work *wrk = new Work; 
    CHECK(wrk);

    // Get the filename
    sprintf(wrk->filename,sTemplate,i);
    if (iVerb) {
      fprintf(stderr, "Working on: %s : (%d) %d to %d\n",
             wrk->filename,i,iStart,iEnd);
    }
    wrk->filetype = iType; 
    // Compress and save (in parallel)
    wrk->iInDims = iInDims;
    wrk->fRot = fRotate;
    wrk->sm = sm;
    wrk->iFlipx = iFlipx;
    wrk->iFlipy = iFlipy;
    wrk->frame = (i-iStart)/iStep;
    wrk->buffer = new u_char[iSize[0]*iSize[1]*3]; 
    pt_pool_add_work(pool, workproc, (void *)wrk);
  }
  pt_pool_destroy(pool,1);
  sm->stopWriteThread(); 
 
  // Done..
  sm->stopWriteThread(); 
  sm->flushFrames(true); 
  sm->closeFile();
  
  //free(gInputBuf);
  //if (rowbuf) free(rowbuf);

  exit(0);
}

void workproc(void *arg)
{
  Work *wrk = (Work *)arg;
  FillInputBuffer(wrk); 

  // rotate 
  rotate_img(wrk->buffer,wrk->iInDims[0],wrk->iInDims[1],wrk->fRot);

  // flipping...
  if (wrk->iFlipx) flipx(wrk->buffer,wrk->sm->getWidth(),wrk->sm->getHeight());
  if (wrk->iFlipy) flipy(wrk->buffer,wrk->sm->getWidth(),wrk->sm->getHeight());
  
  //wrk->sm->bufferFrame(wrk->frame,wrk->buffer, writeOK);
  wrk->sm->compressAndBufferFrame(wrk->frame,wrk->buffer);

  //free(arg);
  delete wrk; 

#ifdef DMALLOC
   dmalloc_log_stats();
   dmalloc_log_unfreed();
#endif
}

void flipx(unsigned char *img,int dx,int dy)
{
  unsigned char	*junk = (unsigned char*)malloc(dx*dy*3);
  CHECK(junk);
  memcpy(junk,img,dx*dy*3);

  for(int y=0;y<dy;y++) {
    int yo = dy - y - 1;
    memcpy(img+(yo*dx*3),junk+(y*dx*3),dx*3);
  }
  free(junk);
  return;
}

void flipy(unsigned char *img,int dx,int dy)
{
  unsigned char	*junk = (unsigned char*)malloc(dx*dy*3);
  CHECK(junk);
  memcpy(junk,img,dx*dy*3);

  for(int y=0;y<dy;y++) {
    int yp = y*(dx*3);
    for(int x=0;x<dx;x++) {
      int xo = dx - x - 1;
      memcpy(img+yp+(xo*3),junk+yp+(x*3),3);
    }
  }
  free(junk);
  return;
}

void rotate_dims(float rot,int *dx,int *dy)
{
	if ((rot == 90) || (rot == 270)) {
		int i = *dx;
		*dx = *dy;
		*dy = i;
	}
	return;
}

int rotate_img(unsigned char *img,int dx,int dy,float rot)
{
	unsigned char *pSrc,*pDst,*tmp;
	int	i,j,rowbytes;
	int	d = 0;

	// What are we going to do?
	if (rot == 90) {
		d = 1;
	}
	if (rot == 180) {
		d = 2;
	}
	if (rot == 270) {
		d = 3;
	}
	if (d == 0) return(-1);

	// Work from a copy
        tmp = (unsigned char*)malloc(dx*dy*3);
	CHECK(tmp);
	memcpy(tmp,img,dx*dy*3);

	// Do it
	rowbytes = dx*3;
	pDst = img;
	switch(d) {
		case 3:  /* 270 */
			for(j=0;j<dx;j++) {
				pSrc = tmp+(rowbytes*(dy-1))+(3*j);
				for(i=0;i<dy;i++) {
					*pDst++ = pSrc[0];
					*pDst++ = pSrc[1];
					*pDst++ = pSrc[2];
					pSrc -= rowbytes;
				}
			}
			break;
		case 2:  /* 180 */
			for(j=0;j<dy;j++) {
				pSrc = tmp+(rowbytes*(dy-1-j))+(3*(dx-1));
				for(i=0;i<dx;i++) {
					*pDst++ = pSrc[0];
					*pDst++ = pSrc[1];
					*pDst++ = pSrc[2];
					pSrc -= 3;
				}
			}
			break;
		case 1:  /* 90 */
			for(j=0;j<dx;j++) {
				pSrc = tmp+(rowbytes-3)-j*3;
				for(i=0;i<dy;i++) {
					*pDst++ = pSrc[0];
					*pDst++ = pSrc[1];
					*pDst++ = pSrc[2];
					pSrc += rowbytes;
				}
			}
			break;
	}

	free(tmp);
	return(0);
}

