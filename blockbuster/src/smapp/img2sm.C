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
#include <fstream>

#include "zlib.h"
#include "pngsimple.h"

#include "libimage/sgilib.h"
#include "../libpnmrw/libpnmrw.h"
#include "../libpng/pngsimple.h"
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
  cerr << "ERROR: "  << msg << endl << endl;
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
// Prototypes
int rotate_img(unsigned char *img,int dx,int dy,float rot);
void rotate_dims(float rot,int *dx,int *dy);
void flipx(unsigned char *img,int dx,int dy);
void flipy(unsigned char *img,int dx,int dy);

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
#define CHECK(v)                                \
  if(v == NULL)                                 \
    img2sm_fail_check(__FILE__,__LINE__)

// =======================================================================
void img2sm_fail_check(const char *file,int line) {
  perror("fail_check");
smdbprintf(0,"Failed at line %d in file %s\n",line,file);
  exit(1);
}

// =======================================================================
bool check_if_pnm(string filename) {
  int   dx,dy,fmt,f;
  xelval    value;
  
  FILE *fp = pm_openr((char*)filename.c_str());
  if (!fp) {
    smdbprintf(5, "check_if_pnm: Unable to open the file: %s\n",filename.c_str());
    return false;
  }
  if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
    smdbprintf(5, "check_if_pnm: file \"%s\" is not in PNM format.\n",filename.c_str());
    pm_closer(fp);
    return false; 
  }
  return true; 
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
  case 0: { // TIFF
    uint32 *temp;
    unsigned int w, h;
    tiff = TIFFOpen(wrk->filename.c_str(),"r");
    if (!tiff) {
      wrk->sm->SetError(str(boost::format("Error: Unable to open TIFF: %s")% wrk->filename));
      return; 
    }
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
    if ((w != wrk->Dims[0]) || (h != wrk->Dims[1])) {
      wrk->sm->SetError(str(boost::format("Error: image size changed: %s")%wrk->filename));
      return; 
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
      for(y=0; y<wrk->Dims[1]; y++) {
        TIFFReadScanline(tiff,(unsigned char *)ss,y,0);
        for(x=0; x<wrk->Dims[0]*wrk->Size[2]; x++) {
          if (ss[x] < mm[0]) mm[0] = ss[x];
          if (ss[x] > mm[1]) mm[1] = ss[x];
          if (ss[x] < iMin) ss[x] = iMin;
          if (ss[x] > iMax) ss[x] = iMax;
        }
        for(x=0; x<wrk->Dims[0]; x++) {
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
      for(int x=0; x<w*h; x++) {
        *p++ = TIFFGetR(temp[x]);
        *p++ = TIFFGetG(temp[x]);
        *p++ = TIFFGetB(temp[x]);
      }
    }

    TIFFClose(tiff);
    free(temp);
  }
    break;
  case 1: { // SGI
    unsigned char *p = wrk->buffer;
    unsigned short *rowbuf = new unsigned short[wrk->Dims[0]];
    CHECK(rowbuf);
    libi = sgiOpen((char*)wrk->filename.c_str(),SGI_READ,0,0,0,0,0);
    if (!libi) {
      wrk->sm->SetError(str(boost::format("Error: Unable to open SGI: %s")%wrk->filename));
      return;
    }
    if ((libi->xsize != wrk->Dims[0]) ||
        (libi->ysize != wrk->Dims[1])) {
      wrk->sm->SetError(str(boost::format("Error: image size changed: %s")%wrk->filename));
      return;
    }
    for(unsigned int y=0; y<wrk->Dims[1]; y++) {
      int x;
      if (wrk->Size[2] >= 3) {
        sgiGetRow(libi,rowbuf,y,0);
        for(x=0; x<wrk->Dims[0]; x++) {
          p[x*3+0] = rowbuf[x];
        }
        sgiGetRow(libi,rowbuf,y,1);
        for(x=0; x<wrk->Dims[0]; x++) {
          p[x*3+1] = rowbuf[x];
        }
        sgiGetRow(libi,rowbuf,y,2);
        for(x=0; x<wrk->Dims[0]; x++) {
          p[x*3+2] = rowbuf[x];
        }
      } else {
        sgiGetRow(libi,rowbuf,y,0);
        for(x=0; x<wrk->Dims[0]; x++) {
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
  case 2: { // raw apparently means GZ for this!
    unsigned char *p = wrk->buffer;

    gzFile    fpz;
    fpz = gzopen(wrk->filename.c_str(),"r");
    if (!fpz) {
      wrk->sm->SetError(str(boost::format("Error: Unable to open RAW compressed: %s")%wrk->filename));
      return;
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
      for(int x=0; x<wrk->Dims[0]*wrk->Dims[1]; x++) {
        *p++ = *p0++;
        *p++ = *p1++;
        *p++ = *p2++;
      }
      free(b);
    } else {
      for(unsigned int y=0; y<wrk->Dims[1]; y++) {
        int  x;
        if (wrk->Size[2] == 3) {
          gzread(fpz,p,wrk->Dims[0]*3);
        } else {
          gzread(fpz,p,wrk->Dims[0]);
          for(x=wrk->Dims[0]-1; x>=0; x--) {
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
  case 3: { // PNM
    int   dx,dy,fmt,f;
    xelval    value;

    unsigned char *p = wrk->buffer;

    fp = pm_openr((char*)wrk->filename.c_str());
    if (!fp) {
      wrk->sm->SetError(str(boost::format("Unable to open the file: %s")%wrk->filename));
      return;
    }
    if (pnm_readpnminit(fp, &dx,&dy,&value,&fmt) == -1) {
      wrk->sm->SetError(str(boost::format("The file is not in PNM format: %s.")%wrk->filename));
      pm_closer(fp);
      return;
    }
    if ((dx != wrk->Dims[0]) ||
        (dy != wrk->Dims[1])) {
      wrk->sm->SetError(str(boost::format("Error: image size changed: %s")%wrk->filename));
      pm_closer(fp);
      return;
    }

    if (PNM_FORMAT_TYPE(fmt) == PPM_TYPE) {
      f = 3;
    } else if(PNM_FORMAT_TYPE(fmt) == PGM_TYPE) {
      f = 1;
    } else {
      wrk->sm->SetError(str(boost::format("Error: file type not supported:%s")%wrk->filename));
      pm_closer(fp);
      return;
    }
    if (f != wrk->Size[2]) {
      wrk->sm->SetError(str(boost::format("Error: file type changed:%s")%wrk->filename));
      pm_closer(fp);
      return;
    }

    xel*  xrow;
    xel*  xp;
    int   rp,gp,bp;

#define NORM(x,mx) ((float)(x)/(float)(mx))*255.0

    xrow = pnm_allocrow( dx );
    for(int y=0; y<dy; y++) {
      p = wrk->buffer + (dy-y-1)*dx*3;
      pnm_readpnmrow( fp, xrow, dx, value, fmt );
      if (wrk->Size[2] == 3) {
        xp = xrow;
        for(int x=0; x<dx; x++) {
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
        for(int x=0; x<dx; x++) {
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
    if (!read_png_image((char*)wrk->filename.c_str(),&wrk->Dims[0],wrk->buffer, true)) {
      wrk->sm->SetError(str(boost::format("Unable to read PNG file: %s")%wrk->filename));
      return;
    }
    break;
  case 5: // JPEG
    if (!read_jpeg_image((char*)wrk->filename.c_str(),&wrk->Dims[0],wrk->buffer)) {
      wrk->sm->SetError(str(boost::format("Unable to read JPEG file: %s")%wrk->filename));
      return;
    }
    break;
  }
  return;
} // end FillInputBuffer()


// =======================================================================
// Order the files for stereo using "left/right" labels as needed.  
bool ReorderInputFilesForStereo(vector<string> &inputfiles) {
  string leftstring, rightstring; 
  string::size_type rlstringpos = string::npos; 
  vector<string> leftstrings, rightstrings; 
  leftstrings.push_back("left"); rightstrings.push_back("right"); 
  leftstrings.push_back("Left"); rightstrings.push_back("Right"); 
  leftstrings.push_back("LEFT"); rightstrings.push_back("RIGHT"); 
  for (int i = 0; rlstringpos == -1 && i<leftstrings.size(); i++) {
    rlstringpos = inputfiles[0].find(leftstrings[i]);
    if (rlstringpos  != string::npos) {
      leftstring = leftstrings[i]; 
      rightstring = rightstrings[i]; 
    }
  }
  if (rlstringpos == string::npos) {
    smdbprintf(2, "ReorderInputFilesForStereo Did not find right/left string in first frame of inputs; assuming they are ordered properly.\n");
    return true; 
  }
  vector<string> leftframes, rightframes; 
  smdbprintf(1, "ReorderInputFilesForStereo found the string \"%s\" in the first file name.  We will attempt to place the files sequentially into right and left channels, checking that the name patterns are coherent, and then interleave them.\n", leftstring.c_str()); 
  
  for (uint32_t fileno = 0; fileno < inputfiles.size(); fileno++) {
    string::size_type leftcheck = inputfiles[fileno].find(leftstring), 
      rightcheck = inputfiles[fileno].find(rightstring);
    if (leftcheck == rlstringpos) {
      leftframes.push_back(inputfiles[fileno]); 
    } else if (rightcheck == rlstringpos) {
      rightframes.push_back(inputfiles[fileno]); 
    } else {
      if (leftcheck != string::npos) {
        smdbprintf(0, "Error: ReorderInputFilesForStereo found the string \"%s\" in the first file name, but found it in a different place in filename \"%s\".  \n", leftstring.c_str(), inputfiles[fileno].c_str()); 
        return false; 
      }
      if (rightcheck != string::npos) {
        smdbprintf(0, "Error: ReorderInputFilesForStereo found the string \"%s\" in the first file name, but found it in a different place in filename \"%s\".\n", leftstring.c_str(), inputfiles[fileno].c_str()); 
        return false; 
      }
      smdbprintf(0, "Error: ReorderInputFilesForStereo found the string \"%s\" in the first file name, but did not find any right or left marker in filename \"%s\".\n", leftstring.c_str(), inputfiles[fileno].c_str()); 
      return false; 
    }
  }
  if (leftframes.size() != rightframes.size()) {
    smdbprintf(0, "Error:  ReorderInputFilesForStereo found %d right frames and %d left frames.  This cannot be made into a stereo movie.\n", rightframes.size(), leftframes.size()); 
    return false; 
  }
  inputfiles.clear(); 
  for (uint32_t i = 0; i< leftframes.size(); i++) {
    inputfiles.push_back(leftframes[i]); 
    smdbprintf(2, "ReorderInputFilesForStereo pushing back frame %s\n", leftframes[i].c_str()); 
    inputfiles.push_back(rightframes[i]); 
    smdbprintf(2, "ReorderInputFilesForStereo pushing back frame %s\n", rightframes[i].c_str()); 
  }
  return true; 
}

// =======================================================================
bool FindFirstFile(vector<string> &inputfiles, string &leftTemplate, string &rightTemplate, uint32_t &filenum, int firstFrame, int lastFrame) {
  // Single input file given.  See if it is a filename template
  string nameTemplate = inputfiles[0];
  inputfiles.clear();
  try {
    string filename = str(boost::format(nameTemplate)%filenum);
  } catch (...) {
    // Not a template.  
    return false;
  }
  

  smdbprintf(5, "FindFirstFile(): First frame is %d, last is %d\n",  firstFrame, lastFrame);

  filenum = 0; 
  if (firstFrame != -1) filenum = firstFrame; 
  if (lastFrame == -1) lastFrame = 100;

  string::size_type lrpos = nameTemplate.find("<LR>"); 
  if (lrpos == string::npos) {
    smdbprintf(5, "FindFirstFile(): No <LR> tag found in template.\n", filenum);
    while (filenum <= lastFrame) {
      smdbprintf(5, "FindFirstFile(): Checking frame number %d\n", filenum);
      string filename = str(boost::format(nameTemplate)%filenum);
      FILE *fp = fopen(filename.c_str(),"r");
      if (!fp) {
        if (firstFrame != -1) {
          smdbprintf(0,  str(boost::format("FindFirstFile(): User specified first frame %1% does not exist.\n")%firstFrame).c_str()); 
          return false; 
        }
      } else {
        firstFrame = filenum; 
        leftTemplate = nameTemplate; 
        inputfiles.push_back(filename); 
        return true; 
      }
      filenum++; 
    } 
  }
  else {  
    smdbprintf(3, "FindFirstFile(): <LR> found in template.  We have \"left/right\" patterns to parse.\n"); 
    vector<string> leftstrings, rightstrings; 
    leftstrings.push_back("left"); rightstrings.push_back("right"); 
    leftstrings.push_back("Left"); rightstrings.push_back("Right"); 
    leftstrings.push_back("LEFT"); rightstrings.push_back("RIGHT"); 

    while (filenum <= lastFrame) {
      FILE * fp = NULL; 
      for (int i = 0; i<leftstrings.size(); i++) {
        string leftfilet = boost::replace_all_copy(nameTemplate, "<LR>", leftstrings[i]); 
        string leftfile = str(boost::format(leftfilet)%filenum); 
        string rightfilet = boost::replace_all_copy(nameTemplate, "<LR>", rightstrings[i]); 
        string rightfile = str(boost::format(rightfilet)%filenum); 
        if ((fp = fopen(leftfile.c_str(), "r")) != NULL) {
          smdbprintf(3, "FindFirstFile(): Found first left frame %s.\n", leftfile.c_str()); 
          fclose(fp); 
          if ((fp = fopen(rightfile.c_str(), "r")) != NULL) {
            fclose(fp); 
            smdbprintf(3, "FindFirstFile(): Found first right frame %s.\n", rightfile.c_str()); 
            inputfiles.push_back(leftfile);
            inputfiles.push_back(rightfile); 
            leftTemplate = leftfilet; 
            rightTemplate = rightfilet; 
            firstFrame = filenum; 
            return true; 
          }
        }
      }
      filenum++; 
    }
  }

  if (firstFrame == -1) firstFrame = 0; 
  
  smdbprint(0, str(boost::format("I tried 100 files with input template \"%1%\" starting at \"%2%\" and gave up.  Try using the \"--first\" option if you have an unusual file sequence.")%nameTemplate%firstFrame).c_str());
  return false; 
}

// =======================================================================
int main(int argc,char **argv) {
  TCLAP::CmdLine  cmd(str(boost::format("%1% converts a set of images into a streaming movie, optionally setting movie meta data.")%argv[0]), ' ', BLOCKBUSTER_VERSION);

  /*!
    =====================================================
    Metadata arguments
    =====================================================
  */
  TCLAP::SwitchArg noMetadata("X", "nometadata", "Do not include any metadata, even if -tag or other is given", cmd, false);

  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.", cmd);

  TCLAP::SwitchArg exportTagfile("E", "export-tagfile", "Create a tag file from the current session which can be read by img2sm or smtag.", cmd);

  TCLAP::ValueArg<string> tagfile("G", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd);

  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd);

  TCLAP::ValueArg<string> delimiter("D", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd);

  TCLAP::ValueArg<int> posterframe("P", "poster-frame", "set poster frame number", false, -1, "frameNum", cmd);

  TCLAP::SwitchArg report("", "report", "After all operations are complete, list all the tags in the file.", cmd);

  TCLAP::SwitchArg quiet("q", "quiet", "Do not echo the tags to stdout.  Just return 0 on successful match. ", cmd);



  /*!
    =====================================================
    Frame selection, region of interest, rotation
    =====================================================
  */
  TCLAP::ValueArg<int> firstFrameFlag("f", "first", "First frame number",false, -1, "integer", cmd);
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
  TCLAP::ValueArg<string> tilesizes("",  "tilesizes",  "Pixel size of the tiles within each frame (default: 0 -- no tiling).  Examples: '512' or '512x256'", false,  "0",  "M or MxN",  cmd); 
  TCLAP::ValueArg<int> mipmaps("m", "mipmaps", "Number of mipmaps (levels of detail) to create",false, 1, "integer", cmd);
  TCLAP::ValueArg<int> quality("j", "jqual", "Jpeg compression quality, from 0-100",false, 75, "integer", cmd);

  TCLAP::ValueArg<int> buffersize("b", "buffersize", "Number of frames to buffer.  Default is 200.  Using lower values saves memory but may decrease performance, and vice versa.",false, 200, "integer", cmd);

  TCLAP::ValueArg<float> frameRate("r", "framerate", " Set preferred frame rate (FPS).  Default is 30, max is 50.",false, 30, "float", cmd);
  vector<string> allowedformats;
  allowedformats.push_back("tiff");
  allowedformats.push_back("TIFF");
  allowedformats.push_back("sgi");
  allowedformats.push_back("SGI");
  allowedformats.push_back("raw");
  allowedformats.push_back("RAW");
  allowedformats.push_back("pnm");
  allowedformats.push_back("PNM");
  allowedformats.push_back("png");
  allowedformats.push_back("PNG");
  allowedformats.push_back("jpg");
  allowedformats.push_back("JPG");
  allowedformats.push_back("jpeg");
  allowedformats.push_back("JPEG");
  TCLAP::ValuesConstraint<string> *allowed = new TCLAP::ValuesConstraint<string>(allowedformats);
  if (!allowed)
    errexit("Cannot create values constraint for formats\n");
  TCLAP::ValueArg<string>format("F", "Format", "Format of output files (use if name does not make this clear)", false, "default", allowed);
  cmd.add(format);
  //delete allowed;

  vector<string> allowedcompression;
  allowedcompression.push_back("gz");
  allowedcompression.push_back("GZ");
  allowedcompression.push_back("jpeg");
  allowedcompression.push_back("JPEG");
  allowedcompression.push_back("jpg");
  allowedcompression.push_back("JPG");
  allowedcompression.push_back("lzma");
  allowedcompression.push_back("LZMA");
  allowedcompression.push_back("lzo");
  allowedcompression.push_back("LZO");
  allowedcompression.push_back("raw");
  allowedcompression.push_back("RAW");
  allowedcompression.push_back("rle");
  allowedcompression.push_back("RLE");
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

  TCLAP::SwitchArg noReorder("", "no-stereo-reorder", "Normally, file sequences for movies are assumed to be stereographic left/right frames if they have the words \"left\" and \"right\" in them, and appropriately reordered.  Use this flag to override this behavior.", cmd, false);

  TCLAP::SwitchArg stereo("S", "Stereo", "Specify the output file is L/R stereo.  In this case, the input images will be interpreted first by whether they have any version of the words \"left\" and \"right\" in their names.  If not, then the odd frames will be taken as \"left\" and the even will be \"right.\"  If you are using a filename template, you can use \"<LR>\" as a placeholder for \"left/right\".  Example for VisIt output frames, you might use \"<LR>_stereo-blah%04d.png\"", cmd, false);



  /*!
    =====================================================
    other
    =====================================================
  */
  TCLAP::ValueArg<int> threads("t", "threads", "Number of threads to use (default: 8)",false, 8, "integer", cmd);
  TCLAP::ValueArg<int> verbosity("v", "verbosity", "Verbosity level",false, 0, "integer", cmd);

  // Note this is an UnlabeledMultiArg.  There must be at least two words given here, the last is the output name, all others are input files.
  TCLAP::UnlabeledMultiArg<string> filenames("filenames", "Either a list of input filenames, or a filename template, followed by a moviename.  A filename template is aa C-style string containing C++ template notation for specifying multiple movie frame files, e.g. \"filename%04d.png\" specifies a list of png files with 4 digit 0-padded numbers. Boost-style %1% notation is also supported.", true, "filename", cmd);

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
  
  sm_setVerbose(verbosity.getValue());  

  int lastFrame = lastFrameFlag.getValue(), firstFrame = firstFrameFlag.getValue(), frameStep = frameStepFlag.getValue();

  if (firstFrame == -1) firstFrame = 0;

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
  string origTemplate, leftTemplate, rightTemplate; 
  uint32_t filenum = firstFrame; 
  if (inputfiles.size() == 1) {
    FILE *fp = fopen(inputfiles[0].c_str(),"r");
    if (fp) {
      smdbprintf(5, str(boost::format("Warning: single input file \"%1%\" given.  This is going to make a pretty stupid movie.\n")%inputfiles[0]).c_str());
      fclose(fp); 
    } 
    else { 
      smdbprint(1, str(boost::format("Warning: single input \"%s\" given.  Checking to see if it is a template...\n")%inputfiles[0]).c_str());
      origTemplate = inputfiles[0]; 
      if (!FindFirstFile(inputfiles, leftTemplate, rightTemplate, filenum, firstFrame, lastFrame)) {
        errexit(cmd, "Could not find first file(s).\n"); 
      }
      filenum += frameStep; 
      haveTemplate = true; 

      // Find the rest of the files... 
      while (lastFrame == -1 || filenum <= lastFrame) {
        string leftFilename = str(boost::format(leftTemplate)%filenum), rightFilename;
        smdbprintf(5, "Checking next file %s\n", leftFilename.c_str());
        FILE *fp = fopen(leftFilename.c_str(),"r");
        if (!fp) {
          if (lastFrame == -1) {
            break; // we have found all the frames
          } else {
            // We found the first frame and the user specified last frame but a frame is missing 
            errexit(cmd, str(boost::format("Cannot open left file \"%1%\", #%2% in sequence for template \"%3%\"")%leftFilename%filenum%origTemplate));
          }       
        }
        else {
          fclose(fp);
          smdbprintf(5, "Successfully opened left file %s\n",  leftFilename.c_str()); 
          if (rightTemplate != "") {
            rightFilename  = str(boost::format(rightTemplate)%filenum);        
            smdbprintf(5, "Checking right file %s\n", rightFilename.c_str()); 
            fp = fopen(rightFilename.c_str(),"r");
            if (!fp) {
              errexit(cmd, str(boost::format("Cannot open right file \"%1%\", #%2% in sequence for template \"%3%\"")%rightFilename%filenum%rightTemplate));
            }       
            fclose(fp);
          }
          smdbprintf(2, "Adding left or mono frame %s for timestep %d.\n", leftFilename.c_str(), filenum); 
          inputfiles.push_back(leftFilename);
          if (rightFilename != "") {
            smdbprintf(2, "Adding right stereo frame %s for timestep %d.\n", rightFilename.c_str(), filenum); 
            inputfiles.push_back(rightFilename);
          }
          filenum += frameStep;
        }
      }
      smdbprintf(1, "Found %d files for template %s.\n", inputfiles.size(), origTemplate.c_str());
      firstFrame = 0;
      lastFrame = inputfiles.size()-1;
      frameStep = 1;
    } /* end parsing filenames by name template */
  }

  if (lastFrame == -1) {
    lastFrame = inputfiles.size()-1;
  }
  if (!haveTemplate) {
    smdbprintf(3, "No template found.  Filter out explicit filenames using --first and --last and --step if given\n"); 
    if (stereo.getValue() && !noReorder.getValue()) {
      if (!ReorderInputFilesForStereo(inputfiles)) {
        errexit(cmd, "Could not reorder frames for stereo movie.  This seems suspicious.  If you are sure about your filenames, please use the --no-stereo-reorder flag to override this."); 
      }
    }
        
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
      smdbprintf(3, "Pushing back file %s\n",  filename.c_str()); 
      filtered.push_back(filename);
    }
    inputfiles = filtered;
  }

  // Done identifying input files.
  //============================================================



  if ((rotate.getValue() != 0) && (rotate.getValue() != 90) &&
      (rotate.getValue() != 180) && (rotate.getValue() != 270)) {
    errexit(cmd, str(boost::format("Invalid rotation: %1%.  Only 0,90,180,270 allowed.\n")% rotate.getValue()));
  }


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
  int tile_xsize=0,tile_ysize=0;
  try {
    tile_xsize = boost::lexical_cast<int32_t>(tilesizes.getValue());
    tile_ysize = tile_xsize;
  } catch(boost::bad_lexical_cast &) {
    smdbprintf(5, str(boost::format("Could not make %1% into an integer.  Trying MxN format.")%tilesizes.getValue()).c_str());
    typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;
    boost::char_separator<char> sep("x");
    tokenizer tok(tilesizes.getValue(), sep);
    tokenizer::iterator pos = tok.begin(), endpos = tok.end();
    try {
      if (pos != endpos) {
        tile_xsize = boost::lexical_cast<int32_t>(*pos);
        ++pos;
      }
      if (pos != endpos) {
        tile_ysize = boost::lexical_cast<int32_t>(*pos);
      } else {
        throw;
      }
    } catch(...) {
      errexit(str(boost::format("Bad format string %1%.")%tilesizes.getValue()));
    }
  }
  tsizes[0][0] = tile_xsize;
  tsizes[0][1] = tile_ysize;

  for (int count = 1; count < mipmaps.getValue(); ++count) {
    tsizes[count][0] = tsizes[count-1][0];
    tsizes[count][1] = tsizes[count-1][1];
    smdbprintf(1,"Resolution[%d] Tilesize=[%dx%d]\n", count, 
               tsizes[count][0], tsizes[count][1]);
  } 


  smdbprintf(2, " Checking the file type.\n"); 
  string filename = inputfiles[0];
  int imageType = -1;
  string suffix = format.getValue(); 
  if (suffix == "default") {
    string::size_type idx = inputfiles[0].rfind('.');
    if (idx == string::npos) {
      cerr << "Error:  Cannot find a file type suffix in your output template.  Please use -form to tell me what to do if you're not going to give a suffix." << endl;
      exit(1);
    }
    suffix = inputfiles[0].substr(idx+1,3);
  } 
  
  if (suffix == "tif" || suffix == "TIF")  imageType = 0;
  else if (suffix == "sgi" || suffix == "SGI")  imageType = 1;
  else if (suffix == "raw" || suffix == "RAW")  imageType = 2;
  else if (suffix == "pnm" || suffix == "PNM")  imageType = 3;
  else if (suffix == "png" || suffix == "PNG")  imageType = 4;
  else if (suffix == "jpg" || suffix == "JPG" ||
           suffix.substr(0,3) == "jpe" || 
           suffix.substr(0,3) == "JPE")         imageType = 5;
  else {
    TIFFErrorHandler prev = TIFFSetErrorHandler(NULL); // suppress error messages
    smdbprintf(2, "Cannot figure out file type by filename and you did not use --format.  Checking file type of first file %s\n", filename.c_str());
    if ((tiff = TIFFOpen((char*)filename.c_str(),"r"))!=0) {
      TIFFSetErrorHandler(prev); //restore diagnostics -- we want them now.
      smdbprintf(1,"TIFF input format detected\n");
      imageType = 0;
      suffix = "PNG"; 
      TIFFClose(tiff);
    } else if ((libi = sgiOpen((char*)filename.c_str(),SGI_READ,0,0,0,0,0)) != NULL) {
      smdbprintf(1,"SGI input format detected\n");
      imageType = 1;
      sgiClose(libi);
      suffix = "SGI"; 
    }     
    // NOTE: there is no check_if_raw as the format is completely headerless.
    else if (check_if_pnm(filename)) {
      smdbprintf(1,"PNM input format detected\n");
      imageType = 3; /* PNM */
      suffix = "PNM"; 
    } else if (check_if_png((char*)filename.c_str(),NULL)) {
      smdbprintf(1,"PNG input format detected\n");
      imageType = 4; /* PNG */
      suffix = "PNG"; 
    } else if (check_if_jpeg((char*)filename.c_str(),NULL)) {
      smdbprintf(1,"JPEG input format detected\n");
      imageType = 5; /* JPEG */
      suffix = "JPG"; 
    } else {
      errexit("No file format detected. Please use --format flag to help me out.\n");
    }
  }
  smdbprintf(3, "File type is %s and imageType is %d\n", suffix.c_str(), imageType); 

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

  // Open the sm file...
  uint32_t *tsizes_ptr = NULL;
  if (tsizes[0][0]) {
    tsizes_ptr = &tsizes[0][0];
  }
  smBase  *sm = NULL;
  if (compression.getValue() == "" || compression.getValue() == "gz" || compression.getValue() == "GZ") {
    if (compression.getValue() == "") {
      smdbprintf(1, "No compression given; using gzip compression by default.\n"); 
    }
    sm = new smGZ(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "raw" || compression.getValue() == "RAW") {
    sm = new smRaw(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "rle" || compression.getValue() == "RLE") {
    sm = new smRLE(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "lzo" || compression.getValue() == "LZO") {
    sm = new smLZO(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
  } else if (compression.getValue() == "jpg" || compression.getValue() == "JPG") {
    sm = new smJPG(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
    ((smJPG *)sm)->setQuality(quality.getValue());
  } else if (compression.getValue() == "lzma" || compression.getValue() == "LZMA") {
    sm = new smXZ(moviename.c_str(),iSize[0],iSize[1],inputfiles.size(), tsizes_ptr,mipmaps.getValue());
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
    try {
      sm->SetMetaData(commandLine, tagfile.getValue(), canonical.getValue(), delimiter.getValue(), taglist.getValue(), posterframe.getValue(), exportTagfile.getValue(), quiet.getValue());
    } catch (string msg) {
      errexit(msg); 
    }
  }

  if (report.getValue()) {
    cout << sm->InfoString(verbosity.getValue()) << endl;
    cout << "Tags =============== \n";
    cout << sm->MetaDataAsString() << endl;
  }
  sm->closeFile();

  if (sm->haveError()) {
    cout << "Got error in movie creation.  No file is created.\n"; 
    sm->deleteFile(); 
    exit(1); 
  } 
  else {
    cout << str(boost::format("img2sm successfully created movie %s\n")%moviename);
    exit(0);
  }
}

void workproc(void *arg) {
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

void flipx(unsigned char *img,int dx,int dy) {
  unsigned char *junk = (unsigned char*)malloc(dx*dy*3);
  CHECK(junk);
  memcpy(junk,img,dx*dy*3);

  for(int y=0; y<dy; y++) {
    int yo = dy - y - 1;
    memcpy(img+(yo*dx*3),junk+(y*dx*3),dx*3);
  }
  free(junk);
  return;
}

void flipy(unsigned char *img,int dx,int dy) {
  unsigned char *junk = (unsigned char*)malloc(dx*dy*3);
  CHECK(junk);
  memcpy(junk,img,dx*dy*3);

  for(int y=0; y<dy; y++) {
    int yp = y*(dx*3);
    for(int x=0; x<dx; x++) {
      int xo = dx - x - 1;
      memcpy(img+yp+(xo*3),junk+yp+(x*3),3);
    }
  }
  free(junk);
  return;
}

void rotate_dims(float rot,int *dx,int *dy) {
  if ((rot == 90) || (rot == 270)) {
    int i = *dx;
    *dx = *dy;
    *dy = i;
  }
  return;
}

int rotate_img(unsigned char *img,int dx,int dy,float rot) {
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
    for(j=0; j<dx; j++) {
      pSrc = tmp+(rowbytes*(dy-1))+(3*j);
      for(i=0; i<dy; i++) {
        *pDst++ = pSrc[0];
        *pDst++ = pSrc[1];
        *pDst++ = pSrc[2];
        pSrc -= rowbytes;
      }
    }
    break;
  case 2:  /* 180 */
    for(j=0; j<dy; j++) {
      pSrc = tmp+(rowbytes*(dy-1-j))+(3*(dx-1));
      for(i=0; i<dx; i++) {
        *pDst++ = pSrc[0];
        *pDst++ = pSrc[1];
        *pDst++ = pSrc[2];
        pSrc -= 3;
      }
    }
    break;
  case 1:  /* 90 */
    for(j=0; j<dx; j++) {
      pSrc = tmp+(rowbytes-3)-j*3;
      for(i=0; i<dy; i++) {
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

