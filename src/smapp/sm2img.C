/*
** $RCSfile: sm2img.C,v $
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

** $Id: sm2img.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


// Utility to combine image files into movie
// Also used as sminfo.  
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>

#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"

#include "zlib.h"
#include "pngsimple.h"
#include "pt/pt.h"

//define int32 int32hack
extern "C" {
#include <tiff.h>
#include <tiffio.h>
}
//undef int32

#include "libimage/sgilib.h"
#include "libpnmrw/libpnmrw.h"
#include "simple_jpeg.h"
#include "../config/version.h"

void workproc(void *work);
int gBlocksize[3] = {0,0,3}, gBlockOffset[2]= {0,0}; 
int gMipmap=0; 
char gNameTemplate[4096]; 
char gSmFilename[4096]; 
vector<unsigned char *>gBuffers;
int	gVerbosity = 0;
int	gType = -1;
int	gFirstFrame = 0;
int	gLastFrame = -1;
int	gFrameStep = 1;
int		gQuality = 75;
smBase *gSm; 

struct Work {
  // int threadNum, numThreads;
  uint32_t frameNum; 
}; 

void cmdline(char *app,int binfo)
{
    if (binfo) {
      fprintf(stderr,"%s (%s) usage: %s smfile\n",
              basename(app), BLOCKBUSTER_VERSION, basename(app));
    } else {      
      fprintf(stderr,"%s (%s) usage: %s [options] smfile [outputtemplate]\n",
              basename(app), BLOCKBUSTER_VERSION, basename(app));
      fprintf(stderr,"Options:\n");
      fprintf(stderr,"\t-v Verbose mode. Equivalent to -verbose 1\n");
      fprintf(stderr, "\t-verbose n Set verbosity to n.\n"); 
      fprintf(stderr,"\t-ignore Ignore invalid output templates. Default:check.\n");
      fprintf(stderr,"\t-first num Select first frame number to extract. Default:0.\n");
      fprintf(stderr,"\t-last num Select last frame number to extract. Default:last frame.\n");
      fprintf(stderr,"\t-step num Select frame number step factor. Default:1.\n");
      fprintf(stderr,"\t-quality num Select JPEG output quality (0-100). Default: 75\n");
      fprintf(stderr,"\t-threads num Use num threads for work. Default: 1\n");
      fprintf(stderr,"\t-form [\"tiff\"|\"sgi\"|\"pnm\"|\"png\"|\"jpg\"|\"YUV\"] Output file format (default:use template suffix or png)\n");
      fprintf(stderr,"\t-region offsetX offsetY sizeX sizeY -- Output will be the given subregion of the input.  Default: offsets 0 0, original size .  \n");
      fprintf(stderr,"\t-mipmap Extract frame from mipmap level. Default: 0\n");
      fprintf(stderr,"\tNote: without an output template, movie stats will be displayed.\n");
    }
    exit(1);
}

int main(int argc,char **argv)
{
  //smBase *sm = NULL; 
  int	iIgnore = 0;
  int	originalImageSize[2];
  int		bIsInfo = 0;
  //int	i,j,x,y;
  int argnum; 
  char		nametemplate[1024],nametemplate2[1024];
  int nThreads=1; 
  bool getinfo = false; 

  if (strstr(argv[0],"sminfo")) {
    getinfo = true; 
    bIsInfo = 1;
    argnum = 1;
    while ((argnum<argc) && (argv[argnum][0] == '-')) {
      if (strcmp(argv[argnum],"-v")==0) {
        gVerbosity = 1;
      } else {
        cmdline(argv[0],bIsInfo);
      }
      argnum++;
    }
    //if ((argc - argnum) != 1) cmdline(argv[0],bIsInfo);
    
  } else {
    
	/* parse the command line ... */
	argnum = 1;
	while ((argnum<argc) && (argv[argnum][0] == '-')) {
      if (strcmp(argv[argnum],"-v")==0) {
        gVerbosity = 1;
      } else if (strcmp(argv[argnum],"-verbose")==0) {
        gVerbosity = atoi(argv[argnum+1]);
        argnum++; 
      } else if (strcmp(argv[argnum],"-ignore")==0) {
        iIgnore = 1;
      } else if (strcmp(argv[argnum],"-quality")==0) {
        if ((argc-argnum) > 1) {
          gQuality = atoi(argv[argnum+1]);
        } else {
          cmdline(argv[0],bIsInfo);
        }
        argnum++;
      } else if (strcmp(argv[argnum],"-first")==0) {
        if ((argc-argnum) > 1) {
          gFirstFrame = atoi(argv[argnum+1]);
        } else {
          cmdline(argv[0],bIsInfo);
        }
        argnum++;
      } else if (strcmp(argv[argnum],"-last")==0) {
        if ((argc-argnum) > 1) {
          gLastFrame = atoi(argv[argnum+1]);
        } else {
          cmdline(argv[0],bIsInfo);
        }
        argnum++;
      } else if (strcmp(argv[argnum],"-step")==0) {
        if ((argc-argnum) > 1) {
          gFrameStep = atoi(argv[argnum+1]);
        } else { 
          cmdline(argv[0],bIsInfo);
        }
        argnum++;
      } else if (strcmp(argv[argnum],"-threads")==0) {
        if ((argc-argnum) > 1) {
          nThreads = atoi(argv[argnum+1]);
        } else {
          cmdline(argv[0],bIsInfo);
        }
        argnum++;
      } else if (strcmp(argv[argnum],"-region")==0) {
        if ((argc-argnum) > 4) {
          gBlockOffset[0] = atoi(argv[++argnum]);
          gBlockOffset[1] = atoi(argv[++argnum]);
          gBlocksize[0] = atoi(argv[++argnum]);
          gBlocksize[1] = atoi(argv[++argnum]);
          if (gBlocksize[0] <= 0 || gBlocksize[1] <= 0){
            fprintf(stderr, "Error: image size must be greater than zero.\n");
            exit(1);
          }
          if (gBlockOffset[0] < 0 || gBlockOffset[1] < 0){
            fprintf(stderr, "Error: image offsets must be nonnegative.\n");
            exit(1);
          }
        } else {
          cmdline(argv[0],bIsInfo);
        }
      } else if (strcmp(argv[argnum],"-mipmap")==0) {
        if ((argc-argnum) > 1) {
          gMipmap = atoi(argv[++argnum]);
		  
        } else {
          cmdline(argv[0],bIsInfo);
        }
      } else if (strcmp(argv[argnum],"-form")==0) {
        if ((argc-argnum) > 1) {
          if (strcmp(argv[argnum+1],"tiff") == 0) {
            gType = 0;
          } else if (strcmp(argv[argnum+1],"sgi") == 0) {
            gType = 1;
          } else if (strcmp(argv[argnum+1],"pnm") == 0) {
            gType = 2;
          } else if (strcmp(argv[argnum+1],"YUV") == 0) {
            gType = 3;
          } else if (strcmp(argv[argnum+1],"png") == 0) {
            gType = 4;
          } else if (strcmp(argv[argnum+1],"jpg") == 0) {
            gType = 5;
          } else {
            fprintf(stderr,"Invalid format: %s\n",
                    argv[argnum+1]);
            exit(1);
          }
          argnum++;
        } else {
          cmdline(argv[0],bIsInfo);
        }
      } else {
        fprintf(stderr,"Unknown option: %s\n\n",argv[argnum]);
        cmdline(argv[0],bIsInfo);
      }
      argnum++;
	}
    
  }
    

  smBase::init();
  sm_setVerbose(gVerbosity);  

  if ((argc - argnum) == 1) {
    getinfo = true; // even for sm2img, a single arg is a request for info
  }
  // Movie info case... (both sminfo and sm2img file)
  if (getinfo) {
    while (argc-argnum) {
      strncpy(gSmFilename, argv[argnum++], 4095); 
      
      gSm = smBase::openFile(gSmFilename, nThreads);
      if (!gSm) {
        fprintf(stderr,"Unable to open the file: %s\n",gSmFilename);
        exit(1);
      }
      
      printf("-----------------------------------------\n"); 
      printf("File: %s\n",gSmFilename);
      printf("Streaming movie version: %d\n",gSm->getVersion());
      if (gSm->getType() == 1) {   // smRLE::typeID
        printf("Format: RLE compressed\n");
      } else if (gSm->getType() == 2) {   // smGZ::typeID
        printf("Format: gzip compressed\n");
      } else if (gSm->getType() == 4) {   // smJPG::typeID
        printf("Format: JPG compressed\n");
      } else if (gSm->getType() == 3) {   // smLZO::typeID
        printf("Format: LZO compressed\n");
      } else if (gSm->getType() == 0) {   // smRaw::typeID
        printf("Format: RAW uncompressed\n");
      } else {
        printf("Format: unknown\n");
      }
      printf("Size: %d %d\n",gSm->getWidth(),gSm->getHeight());
      printf("Frames: %d\n",gSm->getNumFrames());
      printf("FPS: %0.2f\n",gSm->getFPS());
      double len = 0;
      double len_u = 0;
      int frame, res;
      for(res=0;res<gSm->getNumResolutions();res++) {
        for(frame=0;frame<gSm->getNumFrames();frame++) {
          len += (double)(gSm->getCompFrameSize(frame,res));
          len_u += (double)(gSm->getWidth(res)*gSm->getHeight(res)*3);
        }
      }
      printf("Compression ratio: %0.4f%% (%0.0f compressed, %0.0f uncompressed)\n",(len/len_u)*100.0, len, len_u);
      printf("Number of resolutions: %d\n",gSm->getNumResolutions());
      for(res=0;res<gSm->getNumResolutions();res++) {
        printf("    Level: %d : size %d x %d : tile %d x %d\n",
               res,gSm->getWidth(res),gSm->getHeight(res),
               gSm->getTileWidth(res),gSm->getTileHeight(res));
        
      }
      printf("Flags: ");
      if (gSm->getFlags() & SM_FLAGS_STEREO) printf("Stereo ");
      printf("\n");
      
      if (gVerbosity) {
        printf("Frame\tOffset\tLength\n");
        int res = 0; 
        for (res=0; res < gSm->getNumResolutions(); res++) {
          for(frame=0;frame<gSm->getNumFrames();frame++) {
            gSm->printFrameDetails(stdout,frame, res);
          }
        }
      }

      delete gSm;
    }
    exit(0);
  }
  
  if ((argc - argnum) != 2) cmdline(argv[0],bIsInfo);
  
  // get the arguments
  strncpy(gSmFilename, argv[argnum], 4095); 
  strncpy(gNameTemplate, argv[argnum+1], 2047); 
  
  if (!iIgnore) {
    // Bad template name?
    sprintf(nametemplate,gNameTemplate,0);
    sprintf(nametemplate2,gNameTemplate,1);
    if (strcmp(gNameTemplate,nametemplate2) == 0) {
      fprintf(stderr,"Output specification is not a sprintf template (see -ignore)\n");
      exit(1);
    }
  }
  
  gSm = smBase::openFile(gSmFilename, nThreads);
  if (!gSm) {
    fprintf(stderr,"Unable to open the file: %s\n",gSmFilename);
    exit(1);
  }
  
  if(gSm->getVersion() == 1) {
    gMipmap = 0;
  }
  if(gMipmap >= gSm->getNumResolutions()) {
    fprintf(stderr,"Error: Mipmap Level %d Not Available : Choose Levels 0->%d\n",gMipmap,gSm->getNumResolutions()-1);
    exit(1);
  }
  /* originalImageSize[2] = 3; unused */
  originalImageSize[0] = gSm->getWidth(gMipmap);
  originalImageSize[1] = gSm->getHeight(gMipmap);
  
  /* check user inputs for consistency with image size*/
  if (gBlocksize[0]+gBlockOffset[0] > originalImageSize[0]) {
    fprintf(stderr, "Error: X size (%d) + X offset (%d) cannot be greater than image width (%d)\n", 
            gBlocksize[0], gBlockOffset[0], originalImageSize[0]);
    exit(1);
  }
  if (gBlocksize[1]+gBlockOffset[1] > originalImageSize[1]) {
    fprintf(stderr, "Error: Y size (%d) + Y offset (%d) cannot be greater than image height (%d)\n", 
            gBlocksize[1], gBlockOffset[1], originalImageSize[1]);
    exit(1);
  }
  
  if (!gBlocksize[0] && !gBlocksize[1]){ /* no size given -- use image size minus offsets*/
    gBlocksize[0] = originalImageSize[0] - gBlockOffset[0];
    gBlocksize[1] = originalImageSize[1] - gBlockOffset[1];
  }
  
  
  if (gLastFrame < 0) gLastFrame = gSm->getNumFrames() - 1;
  if (gLastFrame < gFirstFrame) cmdline(argv[0],bIsInfo);
  if (gFirstFrame < 0) cmdline(argv[0],bIsInfo);
  if (gLastFrame >= gSm->getNumFrames())  cmdline(argv[0],bIsInfo);
  
  if (gFrameStep < 1) {
    fprintf(stderr,"Error, frame stepping must be >= 1\n");
    exit(1);
  }
  
  // here we go...
  pt_pool         thepool;
  pt_pool_t       pool = &thepool;
  pt_pool_init(pool, nThreads, nThreads*2, 0);

  /*  prepare some reusable buffers */
  int threadnum = 0; 
  while (threadnum<nThreads) {
    gBuffers.push_back(new unsigned char [3*gBlocksize[0]*gBlocksize[1]]);
    threadnum ++; 
  }
  /*
    Work *wrk = new Work; 
    wrk->threadNum = threadnum; 
    wrk->numThreads = nThreads; 
    pt_pool_add_work(pool, workproc, wrk);
    threadnum++; 
    }
  */ 
  int framenum = gFirstFrame; 
  while (framenum <= gLastFrame) {
    Work *wrk = new Work; 
    wrk->frameNum = framenum; 
    pt_pool_add_work(pool, workproc, wrk);
    framenum+= gFrameStep;
  }
  
  pt_pool_destroy(pool,1);
  delete gSm; 
  if (!strstr(argv[0],"sminfo")) {
    cerr << "Successful completion" << endl; 
  }
  return 0; 
}


void workproc(void *vp) {
  TIFF		*tif = NULL;
  sgi_t		*libi = NULL;
  FILE		*fp = NULL;
  int x,y,  j; 
  int framenum; 
  Work *work = (Work*)vp; 
  int threadnum = pt_pool_threadnum(); 
  unsigned char *img = gBuffers[threadnum]; 

  /*  smBase *sm = smBase::openFile(gSmFilename, work->numThreads);
	if (!sm) {
		fprintf(stderr,"Unable to open the file: %s\n",gSmFilename);
		exit(1);
	}
  */
  char nameTemplate[2048]; 
  /*  int framestep = gFrameStep*work->numThreads;
  int firstframe = gFirstFrame + work->threadNum*gFrameStep; 
  fprintf(stderr, "Thread %d of %d: firstframe = %d, framestep = %d, verbosity = %d\n", 
          work->threadNum, work->numThreads,
          firstframe, framestep, gVerbosity); 
  for(framenum =  firstframe;
      framenum <= gLastFrame;
      framenum += framestep ) {	
  */ 
  if (gVerbosity) fprintf(stderr, "Thread %d working on frame %d)\n",
                          threadnum, work->frameNum); 
  
  if(gSm->getVersion() > 1) {
    int destRowStride = 0;
    gSm->getFrameBlock(work->frameNum, img, threadnum, destRowStride,&gBlocksize[0],&gBlockOffset[0],NULL,gMipmap);
  }
  else {
    gSm->getFrame(work->frameNum, img, threadnum);
  }

  if (gType == -1) {
    string templatestr = gNameTemplate; 
    string::size_type idx = templatestr.rfind('.'); 
    if (idx == string::npos) {
      cerr << "Error:  Cannot find a file type suffix in your output template.  Please use -form to tell me what to do if you're not going to give a suffix." << endl; 
      exit(1); 
    }
    string suffix = templatestr.substr(idx+1,3); 
    if (suffix == "tif" || suffix == "TIF")  gType = 0; 
    else if (suffix == "sgi" || suffix == "SGI")  gType = 1; 
    else if (suffix == "pnm" || suffix == "PNM")  gType = 2; 
    else if (suffix == "yuv" || suffix == "YUV")  gType = 3; 
    else if (suffix == "png" || suffix == "PNG")  gType = 4; 
    else if (suffix == "jpg" || suffix == "JPG" || suffix == "jpe" || suffix == "JPE")  gType = 5; 
    else  {
      cerr << "Warning:  Cannot deduce format from input files.  Using PNG format but leaving filenames unchanged." << endl; 
      gType = 4;
    }
  }
  sprintf(nameTemplate,gNameTemplate,work->frameNum);
  switch(gType) {
  case 0: {  // TIFF
    unsigned char *p;
      tif = TIFFOpen(nameTemplate,"w");
      if (tif) {
        // Header stuff 
#ifdef Tru64
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32)gBlocksize[0]);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)gBlocksize[1]);
#else
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (unsigned int)gBlocksize[0]);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (unsigned int)gBlocksize[1]);
#endif
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
        TIFFSetField(tif, TIFFTAG_DOCUMENTNAME, nameTemplate);
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, "sm2img TIFF image");
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        
        for(y=0;y<gBlocksize[1];y++) {
          p = img + (gBlocksize[0]*3)*(gBlocksize[1]-y-1);
          TIFFWriteScanline(tif,p,y,0);
        }
        TIFFFlushData(tif);
        TIFFClose(tif);
      }
    }
      break;
    case 1: {  // SGI
      unsigned short   buf[8192];
      libi = sgiOpen(nameTemplate,SGI_WRITE,SGI_COMP_RLE,1,
                     gBlocksize[0],gBlocksize[1],3);
      if (libi) {
        for(y=0;y<gBlocksize[1];y++) {
          for(j=0;j<3;j++) {
            for(x=0;x<gBlocksize[0];x++) {
              buf[x]=img[(x+(y*gBlocksize[0]))*3+j];
            }
            sgiPutRow(libi,buf,y,j);
          }
        }
				    sgiClose(libi);
      }
    }
      break;
    case 2: {  // PNM
      fp = pm_openw(nameTemplate);
      if (fp) {
        xel* xrow;
        xel* xp;
        xrow = pnm_allocrow( gBlocksize[0] );
        pnm_writepnminit( fp, gBlocksize[0], gBlocksize[1], 
                          255, PPM_FORMAT, 0 );
        for(y=gBlocksize[1]-1;y>=0;y--) {
          xp = xrow;
          for(x=0;x<gBlocksize[0];x++) {
            int r1,g1,b1;
            r1 = img[(x+(y*gBlocksize[0]))*3+0];
            g1 = img[(x+(y*gBlocksize[0]))*3+1];
            b1 = img[(x+(y*gBlocksize[0]))*3+2];
            PPM_ASSIGN( *xp, r1, g1, b1 );
            xp++;
          }
          pnm_writepnmrow( fp, xrow, gBlocksize[0], 
                           255, PPM_FORMAT, 0 );
        }
        pnm_freerow(xrow);
        pm_closew(fp);
      }
    }
      break;
    case 3: {  // YUV
      int dx = gBlocksize[0] & 0xfffffe;
      int dy = gBlocksize[1] & 0xfffffe;
      unsigned char *buf = (unsigned char *)malloc(
                                                   (unsigned int)(1.6*dx*dy));
      unsigned char *Ybuf = buf;
      unsigned char *Ubuf = Ybuf + (dx*dy);
      unsigned char *Vbuf = Ubuf + (dx*dy)/4;
      
      /* convert RGB to YUV  */
      unsigned char *p = img;
      for(y=0;y<gBlocksize[1];y++) {
        for(x=0;x<gBlocksize[0];x++) {
          float rd=p[0];
          float gd=p[1];
          float bd=p[2];
          float yd= 0.2990*rd+0.5870*gd+0.1140*bd;
          float ud=-0.1687*rd-0.3313*gd+0.5000*bd;
          float vd= 0.5000*rd-0.4187*gd-0.0813*bd;
          int   Y=(int)floor(yd+0.5);
          int   U=(int)floor(ud+128.5);
          int   V=(int)floor(vd+128.5);
          if (Y<0)   Y=0;
          if (Y>255) Y=255;
          if (U<0)   U=0;
          if (U>255) U=255;
          if (V<0)   V=0;
          if (V>255) V=255;
          *p++ = Y;
          *p++ = U;
          *p++ = V;
        }
      }
      
      /* pull apart into Y,U,V buffers */
      /* down-sample U/V */
      for(y=0;y<dy;y++) {
        p = img + (gBlocksize[1]-y-1)*3*gBlocksize[0];
        for(x=0;x<dx;x++) {
          *Ybuf++ = *p++;
          if ((x&1) || (y&1)) {
            p += 2;
          } else {
            *Ubuf++ = *p++;
            *Vbuf++ = *p++;
          }
        }
      }
      
      /* write the 3 files */
      char	yuvnametemplate[4096];
      p = buf;
      sprintf(yuvnametemplate,"%s.Y",nameTemplate);
      fp = fopen(yuvnametemplate,"wb");
      if (fp) {
        fwrite(p,dx*dy,1,fp);
        fclose(fp);
      }
      p += dx*dy;
      sprintf(yuvnametemplate,"%s.U",nameTemplate);
      fp = fopen(yuvnametemplate,"wb");
      if (fp) {
        fwrite(p,dx*dy/4,1,fp);
        fclose(fp);
      }
      p += dx*dy/4;
      sprintf(yuvnametemplate,"%s.V",nameTemplate);
      fp = fopen(yuvnametemplate,"wb");
      if (fp) {
        fwrite(p,dx*dy/4,1,fp);
        fclose(fp);
      }
      
      free(buf);
    }
      break;
    case 4: {  // PNG
      write_png_file(nameTemplate, img,gBlocksize);
    }
      break;
    case 5: {  // JPEG
      write_jpeg_image(nameTemplate, img,gBlocksize,gQuality);
    }
      break;
    }
    /*if (gVerbosity) fprintf(stderr, "Thread %d done with frame %d)\n",
                            threadnum, framenum); 
    */
  
  
  //delete img; 
  
  //delete sm;
  
  //exit(0);
}
