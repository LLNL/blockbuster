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
**  http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
**  or man llnl_copyright
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
#define SM_VERBOSE 1
// Utility to combine image files into movie
#include <map>
#include <string> 
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <sm/sm.h>

#include "zlib.h"
#include "pngsimple.h"

#include "libimage/sgilib.h"
#include "../libpnmrw/libpnmrw.h"
#include "simple_jpeg.h"
#include "version.h"
#include <tclap_utils.h>
#include "timer.h"

// http://www.highscore.de/boost/process0.5/boost_process/tutorial.html
// #include <boost/process.hpp>

#include "pt/pt.h"
#define int32 int32hack
extern "C" {
#include <tiff.h>
#include <tiffio.h>
}
#undef int32

// =======================================================================
void errexit(string msg) {
  cerr << "ERROR: "  << msg << endl; 
  exit(1);
}


// =======================================================================
void errexit(TCLAP::CmdLine &cmd, string msg) {
  cerr << endl << "*** ERROR *** : " << msg  << endl<< endl;
  cmd.getOutput()->usage(cmd); 
  cerr << endl << "*** ERROR *** : " << msg << endl << endl;
  exit(1); 
}

// =======================================================================
/* applying a filename template to a number */ 
/* string getFilename(string filenameTemplate, int num, bool useTemplate) {
  if (useTemplate)  return str(boost::format(filenameTemplate)%num);
  else return filenameTemplate;
}
*/ 
// =======================================================================
// Prototypes 
int rotate_img(unsigned char *img,int dx,int dy,float rot);
void rotate_dims(float rot,int *dx,int *dy);
void flipx(unsigned char *img,int dx,int dy);
void flipy(unsigned char *img,int dx,int dy);
//void cmdline(char *app);
int check_if_png(char *file_name,int *size);
int read_png_image(char *file_name,int *iSize,unsigned char *buf);

struct Work {
  string filename;
  smBase *sm;
  int frame;
  int filetype; 
  u_char *buffer; // allocate externally to permit copyless buffering
  int iFlipx;
  int iFlipy;
  vector<int> Dims;
  vector<int> Size; 
  bool planar; 
  float rotation;
};

// GLOBALS FOR NOW
//unsigned char *gInputBuf = NULL; 
unsigned short  bitsPerSample, tiffPhoto;
int iMin = -1;
int iMax = -1;
//int   iSize[4] = {-1,-1,0,0};
int iVerb = 0;
//int   iPlanar = 0;
//unsigned short    *rowbuf = NULL;


#ifdef DMALLOC
#include <dmalloc.h>
#endif



// =======================================================================
#define CHECK(v) \
if(v == NULL) \
img2sm_fail_check(__FILE__,__LINE__)

// =======================================================================
void img2sm_fail_check(const char *file,int line)
{
  perror("fail_check");
  fprintf(stderr,"Failed at line %d in file %s\n",line,file);
  exit(1); 
}


// =======================================================================
/*!
  To encapsulate what each worker needs to process its data
*/ 
struct WorkerData {
  int threadNum, numThreads, startFrame, endFrame, frameStep; 
}; 

// =======================================================================
void workerThreadFunction(void *workerData) {
  WorkerData *myData = (WorkerData*)workerData; 
  //FillInputBuffer(myData); 
  return; 
}



void workproc(void *arg);
//=================================================
void FillInputBuffer(Work *wrk) {
  
  FILE      *fp = NULL;
  TIFF      *tiff = NULL;
  sgi_t     *libi = NULL;
  
  // read the file...
  switch(wrk->filetype) {
  case 0: // TIFF
    {
      uint32 *temp;
      unsigned int w, h;
      tiff = TIFFOpen(wrk->filename.c_str(),"r");
      if (!tiff) {
        fprintf(stderr,"Error: Unable to open TIFF: %s\n",wrk->filename.c_str());
        exit(1);
      }
      TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
      TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
      if ((w != wrk->Dims[0]) || (h != wrk->Dims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename.c_str());
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
        temp = (uint32 *)malloc(wrk->Dims[0]*wrk->Size[2]*2);
        CHECK(temp);
        unsigned short *ss = (unsigned short *)temp;
        int x,y;
        float mult = 255.0/(float)(iMax-iMin);
        unsigned char   *p = wrk->buffer;
        unsigned int mm[2] = {200000,0};
        for(y=0;y<wrk->Dims[1];y++) {
          TIFFReadScanline(tiff,(unsigned char *)ss,y,0);
          for(x=0;x<wrk->Dims[0]*wrk->Size[2];x++) {
            if (ss[x] < mm[0]) mm[0] = ss[x];
            if (ss[x] > mm[1]) mm[1] = ss[x];
            if (ss[x] < iMin) ss[x] = iMin;
            if (ss[x] > iMax) ss[x] = iMax;
          }
          for(x=0;x<wrk->Dims[0];x++) {
            if (wrk->Size[2] == 1) {
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
        temp = (uint32 *)malloc(wrk->Dims[0]*wrk->Dims[1]*4);  // Caution deferred free under IRIX
        CHECK(temp);
        TIFFReadRGBAImage(tiff,w,h,temp,0);
        unsigned char   *p = wrk->buffer;
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
      unsigned char *p = wrk->buffer;
      unsigned short *rowbuf = new unsigned short[wrk->Dims[0]];
      CHECK(rowbuf);
      libi = sgiOpen((char*)wrk->filename.c_str(),SGI_READ,0,0,0,0,0);
      if (!libi) {
        fprintf(stderr,"Error: Unable to open SGI: %s\n",wrk->filename.c_str());
        exit(1);
      }
      if ((libi->xsize != wrk->Dims[0]) || 
          (libi->ysize != wrk->Dims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename.c_str());
        exit(1);
      }
      for(unsigned int y=0;y<wrk->Dims[1];y++) {
        int x;
        if (wrk->Size[2] >= 3) {
          sgiGetRow(libi,rowbuf,y,0);
          for(x=0;x<wrk->Dims[0];x++) {
            p[x*3+0] = rowbuf[x];
          }
          sgiGetRow(libi,rowbuf,y,1);
          for(x=0;x<wrk->Dims[0];x++) {
            p[x*3+1] = rowbuf[x];
          }
          sgiGetRow(libi,rowbuf,y,2);
          for(x=0;x<wrk->Dims[0];x++) {
            p[x*3+2] = rowbuf[x];
          }
        } else {
          sgiGetRow(libi,rowbuf,y,0);
          for(x=0;x<wrk->Dims[0];x++) {
            p[x*3+0] = rowbuf[x];
            p[x*3+1] = rowbuf[x];
            p[x*3+2] = rowbuf[x];
          }
        }
        p += 3*wrk->Dims[0];
      }
      sgiClose(libi);
      delete rowbuf; 
    }
    break;
  case 2: // raw apparently means GZ for this!  
    {
      unsigned char *p = wrk->buffer;
      
      gzFile    fpz;
      fpz = gzopen(wrk->filename.c_str(),"r");
      if (!fpz) {
        fprintf(stderr,"Error: Unable to open RAW compressed: %s\n",wrk->filename.c_str());
        exit(1);
      }
      // Header
      if (wrk->Size[3]) {  // I don't believe this ever happens
        void *b = malloc(wrk->Size[3]);
        CHECK(b);
        gzread(fpz,b,wrk->Size[3]);
        free(b);
      }
      // Scan lines
      if (wrk->planar && (wrk->Size[2] == 3)) {
        char *b = (char *)malloc(wrk->Dims[0]*wrk->Dims[1]*3);
        CHECK(b);
        char *p0 = b;
        char *p1 = p0 + wrk->Dims[0]*wrk->Dims[1];
        char *p2 = p1 + wrk->Dims[0]*wrk->Dims[1];
        gzread(fpz,b,wrk->Dims[0]*wrk->Dims[1]*3);
        for(int x=0;x<wrk->Dims[0]*wrk->Dims[1];x++) {
          *p++ = *p0++;
          *p++ = *p1++;
          *p++ = *p2++;
        }
        free(b); 
      } else {
        for(unsigned int y=0;y<wrk->Dims[1];y++) {
          int  x;
          if (wrk->Size[2] == 3) {
            gzread(fpz,p,wrk->Dims[0]*3);
          } else {
            gzread(fpz,p,wrk->Dims[0]);
            for(x=wrk->Dims[0]-1;x>=0;x--) {
              p[x*3+2] = p[x];
              p[x*3+1] = p[x];
              p[x*3+0] = p[x];
            }
          }
          p += 3*wrk->Dims[0];
        }
      }
      // done
      gzclose(fpz);
    }
    break;
  case 3: // PNM
    {  
      int   dx,dy,fmt,f;
      xelval    value;
      
      unsigned char *p = wrk->buffer;
      
      fp = pm_openr((char*)wrk->filename.c_str());
      if (!fp) {
        fprintf(stderr,"Unable to open the file: %s\n",wrk->filename.c_str());
        exit(1);
      }
      if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
        fprintf(stderr,"The file is not in PNM format: %s.\n",wrk->filename.c_str());
        pm_closer(fp);
        exit(1);
      }
      if ((dx != wrk->Dims[0]) || 
          (dy != wrk->Dims[1])) {
        fprintf(stderr,"Error: image size changed: %s\n",wrk->filename.c_str());
        pm_closer(fp);
        exit(1);
      }
      
      if (PNM_FORMAT_TYPE(fmt) == PPM_TYPE) {
        f = 3;
      } else if(PNM_FORMAT_TYPE(fmt) == PGM_TYPE) {
        f = 1;
      } else {
        fprintf(stderr,"Error: file type not supported:%s\n",wrk->filename.c_str());
        pm_closer(fp);
        exit(1);
      }
      if (f != wrk->Size[2]) {
        fprintf(stderr,"Error: file type changed:%s\n",wrk->filename.c_str());
        pm_closer(fp);
        exit(1);
      }
      
      xel*  xrow;
      xel*  xp;
      int   rp,gp,bp;
      
#define NORM(x,mx) ((float)(x)/(float)(mx))*255.0
      
      xrow = pnm_allocrow( dx );
      for(int y=0;y<dy;y++) {
        p = wrk->buffer + (dy-y-1)*dx*3;
        pnm_readpnmrow( fp, xrow, dx, value, fmt );
        if (wrk->Size[2] == 3) {
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
    if (!read_png_image((char*)wrk->filename.c_str(),&wrk->Dims[0],wrk->buffer)) {
      fprintf(stderr,"Unable to read PNG file: %s\n",wrk->filename.c_str());
      exit(1);
    }
    break;
  case 5: // JPEG
    if (!read_jpeg_image((char*)wrk->filename.c_str(),&wrk->Dims[0],wrk->buffer)) {
      fprintf(stderr,"Unable to read JPEG file: %s\n",wrk->filename.c_str());
      exit(1);
    }
    break; 
  }
  return; 
} // end FillInputBuffer()


// =======================================================================
int main(int argc,char **argv)
{
  TCLAP::CmdLine  cmd(str(boost::format("%1% converts a set of images into a streaming movie, optionally setting movie meta data.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 
  
  /*!
    =====================================================
    Metadata arguments
    =====================================================
  */ 
  TCLAP::SwitchArg noMetadata("X", "nometadata", "Do not include any metadata, even if -tag or other is given", cmd, false); 

  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.", cmd); 

  TCLAP::ValueArg<string> exportTagfile("", "export-tagfile", "Instead of applying tags to a movie, create a tag file from the current session which can be read with -f to start another smtag session.", false, "", "filename", cmd); 
  
  TCLAP::ValueArg<string> tagfile("", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd); 

  TCLAP::ValueArg<string> delimiter("D", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd); 

  TCLAP::ValueArg<int> thumbnail("N", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd); 

  TCLAP::ValueArg<int> thumbres("R", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 

  TCLAP::SwitchArg report("L", "list", "After all operations are complete, list all the tags in the file.", cmd); 
  
 

  /*!
    =====================================================
    Frame selection, region of interest, rotation
    =====================================================
  */ 
  TCLAP::ValueArg<int> firstFrameFlag("f", "first", "First frame number",false, 0, "integer", cmd);   
  TCLAP::ValueArg<int> lastFrameFlag("l", "last", "Last frame number",false, -1, "integer", cmd); 
  TCLAP::ValueArg<int> frameStepFlag("s", "step", "Frame step size",false, 1, "integer", cmd); 

  /* WHY IS THIS DISABLED NOW? 
     TCLAP::ValueArg<VectFromString<int> > region("g", "Region", "Image pixel subregion to extract",false, VectFromString<int>(), "'Xoffset Yoffset Xsize Ysize'"); 
  region.getValue().expectedElems = 4; 
  cmd.add(region);
  */ 
  /*!
    =====================================================
    contents, quality
    =====================================================
  */ 
  TCLAP::ValueArg<string> tilesizes("", "tilesizes", "Size of tiles in movie", false, "512", "M for square tiles, MxN for rectangular tiles", cmd); 
  TCLAP::ValueArg<int> mipmaps("m", "mipmaps", "Number of mipmaps (levels of detail) to create",false, 1, "integer", cmd); 
  TCLAP::ValueArg<int> quality("q", "quality", "Jpeg compression quality, from 0-100",false, 75, "integer", cmd); 

  TCLAP::ValueArg<int> buffersize("b", "buffersize", "Number of frames to buffer.  Default is 200.  Using lower values saves memory but may decrease performance, and vice versa.",false, 200, "integer", cmd); 

  TCLAP::ValueArg<float> frameRate("r", "framerate", " Set preferred frame rate.  Default is 30, max is 50.",false, 30, "float", cmd); 
  vector<string> allowedformats; 
  allowedformats.push_back("tiff"); allowedformats.push_back("TIFF"); 
  allowedformats.push_back("sgi"); allowedformats.push_back("SGI");  
  allowedformats.push_back("pnm"); allowedformats.push_back("PNM");  
  allowedformats.push_back("png"); allowedformats.push_back("PNG");  
  allowedformats.push_back("jpg"); allowedformats.push_back("JPG");  
  allowedformats.push_back("yuv"); allowedformats.push_back("YUV"); 
  TCLAP::ValuesConstraint<string> *allowed = new TCLAP::ValuesConstraint<string>(allowedformats); 
  if (!allowed) 
    errexit("Cannot create values constraint for formats\n"); 
  TCLAP::ValueArg<string>format("F", "Format", "Format of output files (use if name does not make this clear)", false, "default", allowed); 
  cmd.add(format); 
  //delete allowed; 

  vector<string> allowedcompression; 
  allowedcompression.push_back("gz"); allowedcompression.push_back("GZ"); 
  allowedcompression.push_back("jpeg"); allowedcompression.push_back("JPEG"); 
  allowedcompression.push_back("jpg"); allowedcompression.push_back("JPG"); 
  allowedcompression.push_back("lzma"); allowedcompression.push_back("LZMA"); 
  allowedcompression.push_back("lzo"); allowedcompression.push_back("LZO"); 
  allowedcompression.push_back("raw"); allowedcompression.push_back("RAW"); 
  allowedcompression.push_back("rle"); allowedcompression.push_back("RLE"); 
  allowed = new TCLAP::ValuesConstraint<string>(allowedcompression); 
  if (!allowed) 
    errexit("Cannot create values constraint for compression\n"); 
  TCLAP::ValueArg<string>compression("c", "compression", "Compression to use on movie", false, "gz", allowed); 
  cmd.add(compression); 
  //delete allowed; 
  TCLAP::ValueArg<float> rotate("", "rotate", " Set rotation of image. ",false, 0, "0,30,60 or 90", cmd); 

  TCLAP::SwitchArg planar("", "planar", "Raw img is planar interleaved (default: pixel interleave)", cmd, false); 
  TCLAP::SwitchArg flipx("", "flipx", "Flip the image over the X axis", cmd, false); 
  TCLAP::SwitchArg flipy("", "flipy", "Flip the image over the Y axis", cmd, false); 

  TCLAP::ValueArg<string> size("", "raw-image-dims", "Specify raw img dims", false, "", "'width:height:depth:header'", cmd); 

  TCLAP::SwitchArg stereo("S", "Stereo", "Specify the output file is L/R stereo.", cmd, false); 
  


  /*!
    =====================================================
    other
    =====================================================
  */ 
  TCLAP::ValueArg<int> threads("t", "threads", "Number of threads to use",false, 4, "integer", cmd); 
  TCLAP::ValueArg<int> verbosity("v", "Verbosity", "Verbosity level",false, 0, "integer", cmd);   

  TCLAP::ValueArg<string> nameTemplateFlag("i", "infile-template", " For a single frame, need not use %d notation.  If this argument is not given, you must supply a list of filenames before the output moviename.", false, "", "input filename template", cmd); 
  
  // Note this is an UnlabeledMultiArg.  If nameTemplate is not given, then there must be at least two files here, the last is the output name, all others are input files.  
  TCLAP::UnlabeledMultiArg<string> filenames("moviename", "Either a list of input filenames or a filename template followed by a moviename.  A filename template is aa C-style string containing C++ template notation for specifying multiple movie frame files, e.g. \"filename%04d.png\" specifies a list of png files with 4 digit 0-padded numbers. Boost-style %1% notation is also supported.", true, "filename", cmd); 

  // save the command line for meta data
  string commandLine; 
  for (int i=0; i<argc; i++) 
    commandLine += (string(argv[i]) + " "); 
  try {
    cmd.parse(argc, argv);
  } catch(std::exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }

  int lastFrame = lastFrameFlag.getValue(), firstFrame = firstFrameFlag.getValue(), frameStep = frameStepFlag.getValue(); 
  
  if (!frameStep) {
    errexit(cmd, "frameStep cannot be 0."); 
  }
  
  if (lastFrame != -1) {
    if ( firstFrame > lastFrame) {
      errexit(cmd, "Last frame must be greater than first frame."); 
    }  
    if (frameStep < 0) {
      uint tmp = lastFrame; 
      lastFrame = firstFrame; 
      firstFrame = tmp; 
    }
  }
  
  //============================================================
  // Identify the input files.  

  string moviename;
  vector<string> inputfiles =  filenames.getValue(); 
  if (inputfiles.size() < 2) {
    errexit(cmd, "You must supply an input file or filename template plus an output movie name.");
  }


  // The movie is the last file in the list of files no matter what 
  moviename = inputfiles.back(); 
  inputfiles.pop_back(); 

  bool haveTemplate = false; 
  if (inputfiles.size() == 1) {
    // See if we have a filename template   
    string nameTemplate = inputfiles[0];
    FILE *fp = NULL; 
    uint filenum = firstFrame; 
    smdbprintf(5, "First frame is %d, last is %d\n",  firstFrame, lastFrame); 
    string filename; 
    while (lastFrame == -1 || filenum <= lastFrame) {
      smdbprintf(5, "Beginning frame number %d\n", filenum); 
      try {
        filename = str(boost::format(nameTemplate)%filenum); 
      } catch (...) {
        smdbprintf(5, str(boost::format("Warning: single input file \"%1%\" given.  This is not a valid filename template, so I'm assuming it's actually a single input file, which is going to make a pretty stupid movie.\n")%nameTemplate).c_str());
        break; 
      } 
      smdbprintf(5, "Opening file %s\n", filename.c_str()); 
      fp = fopen(filename.c_str(),"r");
      if (!fp) {
        if (filenum == firstFrame || lastFrame != -1) {
          errexit(cmd, str(boost::format("Cannot open file \"%1%\", #%2% in sequence for template \"%3%\"")%filename%filenum%nameTemplate));
        }
        else  {
          break; 
        }
      }
      fclose(fp); 
      if (inputfiles[0] == nameTemplate) {
        inputfiles.clear();
      }
      inputfiles.push_back(filename); 
      filenum += frameStep; 
    }
    smdbprintf(1, "Found %d files for template %s.\n", inputfiles.size(), nameTemplate.c_str()); 
    haveTemplate = true; 
    firstFrame = 0; 
    lastFrame = inputfiles.size()-1;
    frameStep = 1; 
  } /* end parsing filenames by name template */ 

  if (lastFrame == -1) {
    lastFrame = inputfiles.size()-1;
  }
  /* If no template, then filter out explicit filenames using --first and --last and --step if given */ 
  if (!haveTemplate) {
    vector<string> filtered;  
    for (uint frame = firstFrame; 
         (frameStep > 0 && frame <= lastFrame)  || 
           (frameStep < 0 && frame >= lastFrame) ; 
         frame += frameStep) {
      if (frame >= inputfiles.size()) {
        errexit(cmd, str(boost::format("Frame %1% in sequence is out of given range (%2%-%3% by step %4%).  Please either don't use --first, --last and --step, or specify such that they fall between 0 and %5%.")%frame%firstFrame%lastFrame%frameStep%(inputfiles.size())));
      }
      string filename = inputfiles[frame]; 
      FILE *fp = fopen(filename.c_str(),"r");
      if (!fp) {
        errexit(cmd, str(boost::format("Cannot open file #%1%, in file list.  Filename: \"%2%\"")%frame%filename));
      }
      fclose(fp); 
      filtered.push_back(filename); 
    }
    inputfiles = filtered;     
  }
  
  // Done identifying input files.
  //============================================================

  
  int imageType = -1;
  string::size_type idx = inputfiles[0].rfind('.'); 
  if (idx == string::npos) {
    cerr << "Error:  Cannot find a file type suffix in your output template.  Please use -form to tell me what to do if you're not going to give a suffix." << endl; 
    exit(1); 
  }
  string suffix = inputfiles[0].substr(idx+1,3); 
  
  if (suffix == "tif" || suffix == "TIF")  imageType = 0; 
  else if (suffix == "sgi" || suffix == "SGI")  imageType = 1; 
  else if (suffix == "pnm" || suffix == "PNM")  imageType = 2; 
  else if (suffix == "yuv" || suffix == "YUV")  imageType = 3; 
  else if (suffix == "png" || suffix == "PNG")  imageType = 4; 
  else if (suffix == "jpg" || suffix == "JPG" || 
           suffix == "jpe" || suffix == "JPE")  imageType = 5; 
  else  {
    cerr << "Warning:  Cannot deduce format from input files.  Using PNG format but leaving inputfiles unchanged." << endl; 
    imageType = 4;
  }

  
  if ((rotate.getValue() != 0) && (rotate.getValue() != 90) && 
      (rotate.getValue() != 180) && (rotate.getValue() != 270)) {
    errexit(cmd, str(boost::format("Invalid rotation: %1%.  Only 0,90,180,270 allowed.\n")% rotate.getValue()));
  }

  sm_setVerbose(verbosity.getValue());  

  if (mipmaps.getValue() < 1 || mipmaps.getValue() > 8) {
    errexit(cmd, str(boost::format("Invalid mipmap level: %1%.  Must be from 1 to 8.\n")% mipmaps.getValue()));
  }
    

  TIFF      *tiff = NULL;
  sgi_t     *libi = NULL;
  unsigned short spp;

  vector<int>   iInDims, iSize(4);
  iSize[0] = iSize[1] = -1; 
  if (size.getValue() != "") {
    string errmsg = str(boost::format("Error in raw-image-dims \"%1%\".  Expected four integers. ")% size.getValue());
    bool success = false; 
    vector<string> nums = Split(size.getValue()); 
    if (nums.size() != 4) {
      errexit(cmd,errmsg); 
    }    
    try {
      for (uint i=0; i<nums.size(); i++) {
        iSize[i] = boost::lexical_cast<int64_t>(nums[i]);
      }     
    } catch (...) {
      errexit(cmd,errmsg); 
    }
  }
  
  smdbprintf(5, "parsing tile sizes \n"); 
  unsigned int  tsizes[8][2];
  //int count = 0;
  int parsed = 0;
  int xsize=0,ysize=0;
  try {
    xsize = boost::lexical_cast<int32_t>(tilesizes.getValue());
    ysize = xsize; 
  }
  catch(boost::bad_lexical_cast &) {
    smdbprintf(5, str(boost::format("Could not make %1% into an integer.  Trying MxN format.")%tilesizes.getValue()).c_str());       
    typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;
    boost::char_separator<char> sep("x"); 
    tokenizer tok(tilesizes.getValue(), sep);
    tokenizer::iterator pos = tok.begin(), endpos = tok.end(); 
    try {
      if (pos != endpos) {
        xsize = boost::lexical_cast<int32_t>(*pos);
        ++pos; 
      } if (pos != endpos) {
        ysize = boost::lexical_cast<int32_t>(*pos);
      } else {
        throw; 
      }
    }
    catch(...) {
      errexit(str(boost::format("Bad format string %1%.")%tilesizes.getValue()));  
    }
  }
  tsizes[0][0] = xsize; 
  tsizes[0][1] = ysize; 

  for (int count = 1; count < mipmaps.getValue(); ++count) {
    tsizes[count][0] = tsizes[count-1][0];
    tsizes[count][1] = tsizes[count-1][1];
  }
  for(int n=0; n < mipmaps.getValue(); n++) {
    smdbprintf(1,"Resolution[%d] Tilesize=[%dx%d]\n",n,tsizes[n][0],tsizes[n][1]);
  }
  

 
  // Check the file type
  string filename = inputfiles[0]; 
  smdbprintf(5, "Checking file type of first file %s\n", filename.c_str()); 
  TIFFErrorHandler prev = TIFFSetErrorHandler(NULL); // suppress error messages
  if (imageType == -1) {
    if ((tiff = TIFFOpen((char*)filename.c_str(),"r"))!=0) {
      TIFFSetErrorHandler(prev); //restore diagnostics -- we want them now. 
      smdbprintf(1,"TIFF input format detected\n");
      imageType = 0;
      TIFFClose(tiff);
    } else if (libi = sgiOpen((char*)filename.c_str(),SGI_READ,0,0,0,0,0)) {
      smdbprintf(1,"SGI input format detected\n");
      imageType = 1;
      sgiClose(libi);
    } else if (check_if_png((char*)filename.c_str(),NULL)) { 
      smdbprintf(1,"PNG input format detected\n");
      imageType = 4; /* PNG */
    } else if (check_if_jpeg((char*)filename.c_str(),NULL)) { 
      smdbprintf(1,"JPEG input format detected\n");
      imageType = 5; /* JPEG */
    } else {
      smdbprintf(1,"Assuming PNG format\n");
      imageType = 4;
    }
  }
  // get the frame size
  if (imageType == 0) {  // TIFF
    tiff = TIFFOpen((char*)filename.c_str(),"r");
    if (!tiff) {
      errexit(str(boost::format("Error: %s is not a TIFF format file.\n")%filename));
    }
    if (!TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&bitsPerSample)) bitsPerSample = 1;
    if (!TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&spp)) spp = 1;
    TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &tiffPhoto);
    TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&(iSize[0]));
    TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&(iSize[1]));
    TIFFClose(tiff);
    if ((bitsPerSample != 8) && (bitsPerSample != 16)) {
      errexit("Only 8 and 16 bits/sample TIFF files allowed\n");
    }
    if ((spp != 1) && (spp != 3)) {
      errexit("Only 1 and 3 sample TIFF files allowed\n");
    }
    iSize[2] = spp;
  } else if (imageType == 1) { // SGI

    libi = sgiOpen((char*)filename.c_str(),SGI_READ,0,0,0,0,0);
    if (!libi) {
      errexit(str(boost::format("Error: %s is not an SGI libimage format file.\n")%filename));
    }
    iSize[0] = libi->xsize;
    iSize[1] = libi->ysize;
    iSize[2] = libi->zsize;
    sgiClose(libi);
  } else if (imageType == 2) { // RAW compressed with GZip -- weird
    if (iSize[0] < 0) 
      errexit("Error: X size cannot be negative for RAW format."); 
    if (iSize[1] < 0)  
      errexit("Error: Y size cannot be negative for RAW format."); 
    if ((iSize[2] != 1) && (iSize[2] != 3)) {
      cerr << "Warning: for RAW format, depth must be either 1 or 3.  Ignoring your values and using 1 instead." << endl;
      iSize[2] = 1; 
    }
    if (iSize[3] < 0) 
      errexit("Error: 'header' value cannot be negative for RAW format."); 
  } else if (imageType == 3) { // PNM
    int     dx,dy,fmt;
    xelval  value;

    FILE *fp = pm_openr((char*)filename.c_str());
    if (!fp) {
      errexit(str(boost::format("Unable to open the file: %s\n")%filename));
    }
    if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
      pm_closer(fp);
      errexit(str(boost::format("%s is not in PNM format\n")%filename));
    }
    pm_closer(fp);
    
    /* set up our params */
    iSize[2] = 1;
    if (PNM_FORMAT_TYPE(fmt) == PBM_TYPE) {
      errexit("Bitmap files are not supported.\n");
    } else if (PNM_FORMAT_TYPE(fmt) == PPM_TYPE) {
      iSize[2] = 3;
    }
    iSize[0] = dx;
    iSize[1] = dy;
  } else if (imageType == 4) { // PNG
    if (!check_if_png((char*)filename.c_str(),&iSize[0])) {
      errexit("Could not open the first PNG image -- it is either corrupt, nonexistent, or not a PNG file\n");
    }
  } else if (imageType == 5) { // JPEG
    if (!check_if_jpeg((char*)filename.c_str(),&iSize[0])) {
      errexit("Could not open the first JPEG image -- it is either corrupt, nonexistent, or not a JPEG file\n"); 
    }
  }
  
  // we may need to swap the dims  
  iInDims = iSize;
  rotate_dims(rotate.getValue(),&iSize[0],&iSize[1]);

  // count the files
  //count = abs((lastFrame-firstFrame.getValue())/frameStep.getValue()) + 1; 

  // Open the sm file...
  smBase::init();
  uint32_t *tsizes_ptr = NULL; 
  tsizes_ptr = &tsizes[0][0]; 
 
  smBase  *sm = NULL;
  if (compression.getValue() == "raw" || compression.getValue() == "RAW") {
    sm = smRaw::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "rle" || compression.getValue() == "RLE") {
    sm = smRLE::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "gz" || compression.getValue() == "GZ") {
    sm = smGZ::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "lzo" || compression.getValue() == "LZO") {
    sm = smLZO::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "jpg" || compression.getValue() == "JPG") {
    sm = smJPG::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
    ((smJPG *)sm)->setQuality(quality.getValue());
  } else if (compression.getValue() == "lzma" || compression.getValue() == "LZMA") {
    sm = smXZ::newFile(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else {
    errexit(str(boost::format("Bad encoding type: %1%")%compression.getValue())); 
  }

  // set the flags...
  sm->setFlags(stereo.getValue() ? SM_FLAGS_STEREO : 0);
    
  sm->setFPS(frameRate.getValue());
  sm->setBufferSize(buffersize.getValue()); 
  /* init the parallel tools */

  int numsteps = inputfiles.size(); 
  int numThreads = threads.getValue();  
  if (numsteps < threads.getValue()) {
    numThreads = numsteps; 
  }
    
  pt_pool         thepool;
  pt_pool_t       pool = &thepool;
  pt_pool_init(pool, threads.getValue(), threads.getValue()*2, 0);
  sm->startWriteThread(); 
    
  // memory buffer for frame input
  smdbprintf(1, "Creating streaming movie file %s from %d files.\n", moviename.c_str(), inputfiles.size());
  
  
  // Walk the input files...
  for(uint frame = 0; frame < inputfiles.size(); frame++) {
 
    Work *wrk = new Work; 
    CHECK(wrk);

    // Get the filename
    wrk->filename = inputfiles[frame];
    wrk->filetype = imageType; 
    // Compress and save (in parallel)
    wrk->Dims = iInDims;
    wrk->Size = iSize; 
    wrk->rotation = rotate.getValue();
    wrk->sm = sm;
    wrk->iFlipx = flipx.getValue();
    wrk->iFlipy = flipy.getValue();
    wrk->frame = frame;
    wrk->planar = planar.getValue(); 
    wrk->buffer = new u_char[iSize[0]*iSize[1]*3]; 
    smdbprintf(1, "Adding work: %s : (%d of %d)\n",
               wrk->filename.c_str(),frame,inputfiles.size());
  
    pt_pool_add_work(pool, workproc, (void *)wrk);
  }
  pt_pool_destroy(pool,1);
  // Done..
  sm->stopWriteThread(); 
  sm->flushFrames(true); 

  if (! noMetadata.getValue()) {
    // populate with reasonable guesses by default:
    TagMap mdmap = SM_MetaData::CanonicalMetaDataAsMap(); 
    mdmap["Movie Create Command"] = SM_MetaData("Movie Create Command", commandLine); 
    mdmap["Movie Create Date"] = SM_MetaData("Movie Create Date", timestamp("%c %Z")); // NEED HOST NAME AND DATE
    char *host = getenv("HOST"); 
    if (host)
      mdmap["Movie Create Host"] = SM_MetaData("Movie Create Host", host); 

    sm->SetMetaData(mdmap); 

    if (tagfile.getValue() != "") {
      if (!sm->ImportMetaData(tagfile.getValue())) {
        errexit (cmd, "Could not export meta data to file."); 
      }
    }
    if (canonical.getValue()) {
      sm->SetMetaData(SM_MetaData::GetCanonicalMetaDataValuesFromUser()); 
    }
      
    SM_MetaData::SetDelimiter(delimiter.getValue()); 
    smdbprintf(2, "Adding command line metadata (%d entries)...\n", taglist.getValue().size()); 
    vector<string>::const_iterator pos = taglist.begin(), endpos = taglist.end(); 
    while (pos != endpos) {
      try {
        sm->SetMetaDataFromDelimitedString(*pos); 
      } catch (...) {
        errexit(str(boost::format("Bad meta data tag string: \"%1%\"\n")%(*pos)));
      }
      ++pos; 
    } 
    if (thumbnail.getValue() != -1)  {
      sm->SetThumbnailFrame(thumbnail.getValue()); 
      if (thumbres.getValue() != -1) {
        sm->SetThumbnailRes(thumbres.getValue()); 
      }
      smdbprintf(2, "Set thumbnail metadata.\n"); 
    }
    //sm->WriteMetaData(); 
    if (exportTagfile.getValue() != "") {      
      if (!sm->ExportMetaData(exportTagfile.getValue())) {
        cerr << "Warning:  could not export metadata to file " << exportTagfile.getValue() << endl; 
      }
      smdbprintf(0, "Exported metadata to file %s\n",exportTagfile.getValue().c_str()); 
    }
  }

  if (report.getValue()) {
    smdbprintf(0, (sm->InfoString(verbosity.getValue())+"\n").c_str()); 
    
  }

  sm->closeFile();
  
  smdbprintf(0, "img2sm successfully created movie %s\n",moviename.c_str()); 
  exit(0);
}
 
void workproc(void *arg)
{
  Work *wrk = (Work *)arg;
  smdbprintf(3, "Thread %d working on %s : frame %d\n",
             pt_pool_threadnum(), wrk->filename.c_str(), wrk->frame);

  FillInputBuffer(wrk); 

  // rotate 
  rotate_img(wrk->buffer,wrk->Dims[0],wrk->Dims[1],wrk->rotation);

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
  unsigned char *junk = (unsigned char*)malloc(dx*dy*3);
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
  unsigned char *junk = (unsigned char*)malloc(dx*dy*3);
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
    int i,j,rowbytes;
    int d = 0;

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

