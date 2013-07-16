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

#include <tclap_utils.h>
// using namespace TCLAP; 
#include "libimage/sgilib.h"
#include "libpnmrw/libpnmrw.h"
#include "simple_jpeg.h"
#include "../config/version.h"
#include <boost/format.hpp>

void workproc(void *work);

struct Work {
  // int threadNum, numThreads;
  uint32_t frameNum; 
  smBase *sm; 
  int blockSize[3]; 
  int blockOffset[2]; 
  int lod; 
  string filename; 
  int imageType; 
  int jqual; 
  int verbosity; 
}; 

void errexit(TCLAP::CmdLine &cmd, string msg) {
  cerr << endl << "*** ERROR *** : " << msg  << endl<< endl;
  cmd.getOutput()->usage(cmd); 
  cerr << endl << "*** ERROR *** : " << msg << endl << endl;
  exit(1); 
}

int main(int argc,char **argv)
{
  TCLAP::CmdLine  cmd(str(boost::format("%1% converts movies to images.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 
  TCLAP::ValueArg<int> firstFrame("f", "first", "First frame number",false, 0, "integer", cmd); 
  TCLAP::ValueArg<int> lastFrame("l", "last", "Last frame number",false, -1, "integer", cmd); 
  TCLAP::ValueArg<int> frameStep("s", "step", "Frame step size",false, 1, "integer", cmd); 
  TCLAP::ValueArg<int> mipmap("m", "mipmap", "Which mipmap level to extract from",false, 0, "integer", cmd); 
  TCLAP::ValueArg<int> quality("q", "quality", "Jpeg compression quality, from 0-100",false, 75, "integer", cmd); 
  TCLAP::ValueArg<int> threads("t", "threads", "Number of threads to use",false, 4, "integer", cmd); 

  TCLAP::ValueArg<VectFromString<int> > region("r", "region", "Image pixel subregion to extract",false, VectFromString<int>(), "'Xoffset Yoffset Xsize Ysize'", cmd); 
  region.getValue().expectedElems = 4; 


  vector<string> allowedformats; 
  allowedformats.push_back("tiff"); allowedformats.push_back("TIFF"); 
  allowedformats.push_back("sgi"); allowedformats.push_back("SGI");  
  allowedformats.push_back("pnm"); allowedformats.push_back("PNM");  
  allowedformats.push_back("png"); allowedformats.push_back("PNG");  
  allowedformats.push_back("jpg"); allowedformats.push_back("JPG");  
  allowedformats.push_back("yuv"); allowedformats.push_back("YUV"); 
  TCLAP::ValuesConstraint<string> allowed(allowedformats); 
  TCLAP::ValueArg<string>format("F", "Format", "Format of output files (use if name does not make this clear)", false, "default", &allowed); 
  cmd.add(format); 


  TCLAP::ValueArg<int> verbosity("v", "Verbosity", "Verbosity level",false, 0, "integer", cmd);   

  TCLAP::UnlabeledValueArg<string> moviename("moviename", "Name of the movie",true, "", "path", cmd); 

  TCLAP::UnlabeledValueArg<string> frameTemplate("frameTemplate", "The output frame template or name.  For multiple frames, use %d notation, e.g. frame%04d.png yields names like frame0000.png, frame0001.png, etc.  For a single frame, a template is optional.", true, "", "frame template", cmd); 

  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
	return 1;
  }

  //smBase *sm = NULL; 
  int	originalImageSize[2];
  int blockSize[3] = {0,0,3}; 
  int blockOffset[2] = {0,0}; 
  int argnum; 
  int nThreads=threads.getValue(); 
  int imageType = -1; 
  
  if (region.getValue().valid && region.getValue()[0] != -1) {
    for (int i=0; i<2; i++) blockSize[i] = region.getValue()[i+2]; 
  }

  string suffix = format.getValue();
  if (suffix == "default") {
    string templatestr = frameTemplate.getValue(); 
    string::size_type idx = templatestr.rfind('.'); 
    if (idx == string::npos) {
      cerr << "Error:  Cannot find a file type suffix in your output template.  Please use -form to tell me what to do if you're not going to give a suffix." << endl; 
      exit(1); 
    }
    suffix = templatestr.substr(idx+1,3); 
  } 
  if (suffix == "tif" || suffix == "TIF")  imageType = 0; 
  else if (suffix == "sgi" || suffix == "SGI")  imageType = 1; 
  else if (suffix == "pnm" || suffix == "PNM")  imageType = 2; 
  else if (suffix == "yuv" || suffix == "YUV")  imageType = 3; 
  else if (suffix == "png" || suffix == "PNG")  imageType = 4; 
  else if (suffix == "jpg" || suffix == "JPG" || 
           suffix == "jpe" || suffix == "JPE")  imageType = 5; 
  else  {
    cerr << "Warning:  Cannot deduce format from input files.  Using PNG format but leaving filenames unchanged." << endl; 
    imageType = 4;
  }
  

  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  
  smBase *sm = smBase::openFile(moviename.getValue().c_str(), nThreads);
  if (!sm) {
    fprintf(stderr,"Unable to open the file: %s\n",moviename.getValue().c_str());
    exit(1);
  }
  int lod = mipmap.getValue(); 

  if(sm->getVersion() == 1) {
    lod = 0;
  }
  if(lod >= sm->getNumResolutions()) {
    fprintf(stderr,"Error: Mipmap Level %d Not Available : Choose Levels 0->%d\n",lod,sm->getNumResolutions()-1);
    exit(1);
  }
  /* originalImageSize[2] = 3; unused */
  originalImageSize[0] = sm->getWidth(lod);
  originalImageSize[1] = sm->getHeight(lod);
  
  /* check user inputs for consistency with image size*/
  if (blockSize[0]+blockOffset[0] > originalImageSize[0]) {
    fprintf(stderr, "Error: X size (%d) + X offset (%d) cannot be greater than image width (%d)\n", 
            blockSize[0], blockOffset[0], originalImageSize[0]);
    exit(1);
  }
  if (blockSize[1]+blockOffset[1] > originalImageSize[1]) {
    fprintf(stderr, "Error: Y size (%d) + Y offset (%d) cannot be greater than image height (%d)\n", 
            blockSize[1], blockOffset[1], originalImageSize[1]);
    exit(1);
  }
  
  if (!blockSize[0] && !blockSize[1]){ /* no size given -- use image size minus offsets*/
    blockSize[0] = originalImageSize[0] - blockOffset[0];
    blockSize[1] = originalImageSize[1] - blockOffset[1];
  }
  
  
  if (lastFrame.getValue() < 0) lastFrame.getValue() = sm->getNumFrames() - 1;
  if (lastFrame.getValue() < firstFrame.getValue()) errexit(cmd, "first frame cannot be greater than last"); 
  if (firstFrame.getValue() < 0) errexit(cmd, "first frame cannot be less than zero"); 
  if (lastFrame.getValue() >= sm->getNumFrames()) errexit(cmd, str(boost::format("last frame is %1%, but movie only has %2% frames in it")% lastFrame.getValue() % sm->getNumFrames()));
  
  if (frameStep.getValue() < 1) {
    fprintf(stderr,"Error, frame stepping must be >= 1\n");
    exit(1);
  }
  int numFrames = (lastFrame.getValue() - firstFrame.getValue())/frameStep.getValue() + 1; 
  
  // Bad template name?
  bool useTemplate = false; 
  try {
    string test = str(boost::format(frameTemplate.getValue()) % 1);
    useTemplate = true; 
  } catch(...) {
    if (numFrames > 1) {
      fprintf(stderr,"If you are outputting multiple frames, then the output specification must be a C-style sprintf template\n");
      exit(1);
    }
  }
  
  // here we go...
  pt_pool         thepool;
  pt_pool_t       pool = &thepool;
  pt_pool_init(pool, nThreads, nThreads*2, 0);

  int framenum = firstFrame.getValue(); 
  while (framenum <= lastFrame.getValue()) {
    Work *wrk = new Work; 
    wrk->frameNum = framenum; 
    wrk->sm = sm; 
    memcpy(wrk->blockSize, blockSize, sizeof(blockSize)); 
    memcpy(wrk->blockOffset, blockOffset, sizeof(blockOffset)); 
    wrk->lod = lod; 
    if (useTemplate) {
      wrk->filename = str(boost::format(frameTemplate.getValue()) % framenum);
    } 
    else {
      wrk->filename = frameTemplate.getValue(); 
    } 
    wrk->imageType = imageType; 
    wrk->jqual = quality.getValue(); 
    wrk->verbosity = verbosity.getValue(); 
    pt_pool_add_work(pool, workproc, wrk);
    framenum+= frameStep.getValue();
  }
  
  pt_pool_destroy(pool,1);
  delete sm; 
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
  //unsigned char *img = gBuffers[threadnum]; 
  vector<unsigned char> img(3*work->blockSize[0]*work->blockSize[1], 0); 

  if (work->verbosity) fprintf(stderr, "Thread %d working on frame %d\n",
                          threadnum, work->frameNum); 
  
  if(work->sm->getVersion() > 1) {
    int destRowStride = 0;
    work->sm->getFrameBlock(work->frameNum, &img[0], threadnum, destRowStride,&work->blockSize[0],&work->blockOffset[0],NULL,work->lod);
  }
  else {
    work->sm->getFrame(work->frameNum, &img[0], threadnum);
  }
  
  switch(work->imageType) {
  case 0: {  // TIFF
    unsigned char *p;
    tif = TIFFOpen(work->filename.c_str(),"w");
      if (tif) {
        // Header stuff 
#ifdef Tru64
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32)work->blockSize[0]);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)work->blockSize[1]);
#else
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (unsigned int)work->blockSize[0]);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (unsigned int)work->blockSize[1]);
#endif
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
        TIFFSetField(tif, TIFFTAG_DOCUMENTNAME, work->filename.c_str());
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, "sm2img TIFF image");
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        
        for(y=0;y<work->blockSize[1];y++) {
          p = &img[0] + (work->blockSize[0]*3)*(work->blockSize[1]-y-1);
          TIFFWriteScanline(tif,p,y,0);
        }
        TIFFFlushData(tif);
        TIFFClose(tif);
      }
    }
      break;
    case 1: {  // SGI
      vector<unsigned short>   buf(work->blockSize[0]+1);
      libi = sgiOpen((char*)work->filename.c_str(),SGI_WRITE,SGI_COMP_RLE,1,
                     work->blockSize[0],work->blockSize[1],3);
      if (libi) {
        for(y=0;y<work->blockSize[1];y++) {
          for(j=0;j<3;j++) {
            for(x=0;x<work->blockSize[0];x++) {
              buf[x]=img[(x+(y*work->blockSize[0]))*3+j];
            }
            sgiPutRow(libi,&buf[0],y,j);
          }
        }
        sgiClose(libi);
      }
    }
      break;
    case 2: {  // PNM
      fp = pm_openw((char*)work->filename.c_str());
      if (fp) {
        xel* xrow;
        xel* xp;
        xrow = pnm_allocrow( work->blockSize[0] );
        pnm_writepnminit( fp, work->blockSize[0], work->blockSize[1], 
                          255, PPM_FORMAT, 0 );
        for(y=work->blockSize[1]-1;y>=0;y--) {
          xp = xrow;
          for(x=0;x<work->blockSize[0];x++) {
            int r1,g1,b1;
            r1 = img[(x+(y*work->blockSize[0]))*3+0];
            g1 = img[(x+(y*work->blockSize[0]))*3+1];
            b1 = img[(x+(y*work->blockSize[0]))*3+2];
            PPM_ASSIGN( *xp, r1, g1, b1 );
            xp++;
          }
          pnm_writepnmrow( fp, xrow, work->blockSize[0], 
                           255, PPM_FORMAT, 0 );
        }
        pnm_freerow(xrow);
        pm_closew(fp);
      }
    }
      break;
    case 3: {  // YUV
      int dx = work->blockSize[0] & 0xfffffe;
      int dy = work->blockSize[1] & 0xfffffe;
      unsigned char *buf = (unsigned char *)malloc(
                                                   (unsigned int)(1.6*dx*dy));
      unsigned char *Ybuf = buf;
      unsigned char *Ubuf = Ybuf + (dx*dy);
      unsigned char *Vbuf = Ubuf + (dx*dy)/4;
      
      /* convert RGB to YUV  */
      unsigned char *p = &img[0];
      for(y=0;y<work->blockSize[1];y++) {
        for(x=0;x<work->blockSize[0];x++) {
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
        p = &img[0] + (work->blockSize[1]-y-1)*3*work->blockSize[0];
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
      string yuvname = work->filename + ".Y"; 
      p = buf;
      fp = fopen(yuvname.c_str(),"wb");
      if (fp) {
        fwrite(p,dx*dy,1,fp);
        fclose(fp);
      }
      p += dx*dy;
      yuvname = work->filename + ".U"; 
      fp = fopen(yuvname.c_str(),"wb");
      if (fp) {
        fwrite(p,dx*dy/4,1,fp);
        fclose(fp);
      }
      p += dx*dy/4;
      yuvname = work->filename + ".V"; 
      fp = fopen(yuvname.c_str(),"wb");
      if (fp) {
        fwrite(p,dx*dy/4,1,fp);
        fclose(fp);
      }
      
      free(buf);
    }
      break;
    case 4: {  // PNG
      write_png_file((char*)work->filename.c_str(), &img[0],work->blockSize);
    }
      break;
    case 5: {  // JPEG
      write_jpeg_image((char*)work->filename.c_str(), &img[0],work->blockSize,work->jqual);
    }
      break;
    }
    /*if (work->verbosity) fprintf(stderr, "Thread %d done with frame %d)\n",
                            threadnum, framenum); 
    */
  
  
  //delete img; 
  
  //delete sm;
  
  //exit(0);
}
