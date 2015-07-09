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
#include <libgen.h>

#include <iostream>
#include <fstream>

#define SM_VERBOSE 1
#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"
#include "sm/smJPG.h"
#include "sm/smXZ.h"
#include "../config/version.h"
#include <tclap_utils.h>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "pt/pt.h"

#include "zlib.h"
#include <stdarg.h>

#include "pngsimple.h"
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

int		gVerbosity = 0;


//===============================================
// #definsmdbprintf smdbprintf
/*
  voismdbprintf(int level, const char *fmt, ...) {
  if (gVerbosity < level) return; 
  // cerr <<  DBPRINTF_PREAMBLE; 
  va_list ap;
  va_start(ap, fmt);
  smdbprintf(0,fmt,ap);
  va_end(ap);
  return; 
  }
*/
// Prototypes 
void cmdline(char *app);

static void smoothx(unsigned char *image, int dx, int dy);
static void smoothy(unsigned char *image, int dx, int dy);

void Sample2d(unsigned char *in,int idx,int idy,unsigned char *out,
              int odx,int ody,int s_left,int s_top,
              int s_dx,int s_dy,int filter);

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


struct MovieInfo{
  MovieInfo():sm(NULL), first(0), last(0), step(1), numframes(0) {}
  string name; 
  smBase	*sm;
  int first,last,step;
  int numframes;
};

struct Work {
  smBase *sm;
  smBase *insm;
  string filename; 
  int imageType; 
  int jqual; 
  int verbosity; 
  int inframe;
  int outframe;
  int iScale;
  int iFilter;
  vector<int> dstSize;
  vector<int> dstOffset; 
  int *src;
  unsigned char *buffer;
  unsigned char *compbuffer;
} ;

void WriteOutputImage(Work *wrk) ; 
void workproc(void *arg);

int main(int argc,char **argv)
{

 
  TCLAP::CmdLine  cmd(str(boost::format("%1% concats movies together or outputs their frames. ")%argv[0]), ' ', BLOCKBUSTER_VERSION); 
  
  TCLAP::ValueArg<int> verbosity("v", "Verbosity", "Verbosity level",false, 0, "integer", cmd);   
  
  TCLAP::SwitchArg noMetadata("X", "no-metadata", "Do not copy or create metadata in the output movie.  Default: accumulate metadata in the order of the movies given.", cmd, false);

  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.", cmd);
 
  TCLAP::SwitchArg exportTagfile("E", "export-tagfile", "Create a tag file from the current session which can be read by img2sm or smtag.", cmd);

  TCLAP::ValueArg<string> tagfile("G", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd);

  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd);

  TCLAP::ValueArg<string> delimiter("D", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd);

  TCLAP::ValueArg<int> posterframe("P", "poster-frame", "set poster frame number", false, -1, "frameNum", cmd);

  TCLAP::SwitchArg report("", "report", "After all operations are complete, list all the tags in the file.", cmd);

  TCLAP::SwitchArg quiet("q", "quiet", "Do not echo the tags to stdout.  Just return 0 on successful match. ", cmd);

  TCLAP::ValueArg<float> destScale("", "dest-scale", "Scale the output height and width by the given factor.",false, 1.0, "nonzero floating point number", cmd);   

  TCLAP::ValueArg<string> destSize("", "dest-size", "Set output dimensions.  Note: To maintain aspect ratio, see --dest-scale.  Default: input size.  Format: XXXxYYY e.g. '300x500'",false, "", "e.g. '300x500'", cmd);   

  TCLAP::ValueArg<int> firstFrameArg("f", "first", "First frame number in each source movie",false, 0, "integer", cmd);
  TCLAP::ValueArg<int> lastFrameArg("l", "last", "Last frame number in each source movie",false, -1, "integer", cmd);
  TCLAP::ValueArg<int> frameStepArg("s", "step", "Frame step size in each source movie",false, 1, "integer", cmd);


  TCLAP::SwitchArg stereo("S", "stereo", "output movie is stereo.", cmd, false);
  TCLAP::SwitchArg filter("", "filter", "Enable smoothing filter for image scaling", cmd, false);
  TCLAP::ValueArg<int> 
    threads("t","threads","number of threads to use (default: 8)", false,8,"integer",cmd);

  vector<string> compressionStrings;
  compressionStrings.push_back("gz");
  compressionStrings.push_back("GZ");
  compressionStrings.push_back("jpeg");
  compressionStrings.push_back("JPEG");
  compressionStrings.push_back("jpg");
  compressionStrings.push_back("JPG");
  compressionStrings.push_back("lzma");
  compressionStrings.push_back("LZMA");
  compressionStrings.push_back("lzo");
  compressionStrings.push_back("LZO");
  compressionStrings.push_back("raw");
  compressionStrings.push_back("RAW");
  compressionStrings.push_back("rle");
  compressionStrings.push_back("RLE");
  TCLAP::ValuesConstraint<string> allowedCompression(compressionStrings);

  TCLAP::ValueArg<string>compression("c", "compression", "Compression to use on movie", false, "gz", &allowedCompression);
  cmd.add(compression);

  vector<string> formatStrings; 
  formatStrings.push_back("tiff"); formatStrings.push_back("TIFF"); 
  formatStrings.push_back("tif");  formatStrings.push_back("TIF"); 
  formatStrings.push_back("sgi");  formatStrings.push_back("SGI");  
  formatStrings.push_back("pnm");  formatStrings.push_back("PNM");  
  formatStrings.push_back("png");  formatStrings.push_back("PNG");  
  formatStrings.push_back("jpg");  formatStrings.push_back("JPG");  
  formatStrings.push_back("yuv");  formatStrings.push_back("YUV"); 
  TCLAP::ValuesConstraint<string>  allowedFormats(formatStrings); 
  TCLAP::ValueArg<string>format("F", "Format", "Format of output files (use if name does not make this clear)", false, "default", &allowedFormats); 
  cmd.add(format); 

  TCLAP::ValueArg<int> jqual("j",  "jqual",  "JPEG quality (0-99, default 75).  Higher is less compressed and higher quality.", false,  75,  "integer (0-99)",  cmd);   
  
  TCLAP::ValueArg<int> mipmaps("", "mipmaps", "Number of levels of detail",false, 1, "integer", cmd);   
  TCLAP::ValueArg<string> tilesizes("",  "tilesizes",  "Pixel size of the tiles within each frame (default: 0 -- no tiling).  Examples: '512' or '512x256'", false,  "0",  "M or MxN",  cmd); 
  
  TCLAP::ValueArg<string> subregion("", "src-subregion", "Select a rectangle of the input by giving region size and offset. Default: all. Format: 'Xoffset Yoffset Xsize Ysize' e.g. '20 50 300 500'",false, "", "'Xoffset Yoffset Xsize Ysize'", cmd);   
  TCLAP::ValueArg<float> fps("r", "framerate", "Store Frames Per Second (FPS) in movie for playback (float).  Might be ignored.",false, 30, "positive floating point number", cmd);   
  
  
  TCLAP::UnlabeledMultiArg<string> filenames("filenames", "Name of the input movie(s) to cat, followed by either the name of the movie to create, or the output image name template. For image frame template, use %d notation, e.g. frame%04d.png yields names like frame0000.png, frame0001.png, etc.   For a single image frame, a template is optional.  Syntax for input movies is \"file[@first[@last[@step]]]\", allowing the first, last and frame step to be specified for each input .sm file individually. The default is to take all frames in an input file, stepping by 1.  You can also use --first, --last and --step as global defaults, overridden by the @ syntax if present for any movie..",true, "input movie(s)> <output movie or image name template", cmd); 
  
  
  bool bOutputMovie = true; // if false, write out frames
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
  

  //int		iFilter = 0;
  // int		iStereo = stereo.getValue()? 0 : SM_STEREO;
  int		nThreads = threads.getValue();
  
  int		nRes = mipmaps.getValue();
  if (nRes < 1 || nRes > 8) {
    errexit(cmd, "Resolutions must be between 1 and 8."); 
  }
  
  vector<string> movies = filenames.getValue(); 
  if (movies.size() < 2) {
    errexit(cmd, "You must supply at least one input movie and one output movie or output frame template"); 
  }
  string outputName = movies[movies.size()-1]; 
  movies.pop_back(); 

  // Check to see if we are dealing with a movie or output images.  
  int imageType = -1; 
  string suffix = format.getValue();
  if (suffix == "default") {
    string templatestr = outputName; 
    string::size_type idx = templatestr.rfind('.'); 
    if (idx == string::npos) {
      cerr << "Error:  Cannot find a file type suffix in your output template.  Please use -form to tell me what to do if you're not going to give a suffix." << endl; 
      exit(1); 
    }
    suffix = templatestr.substr(idx+1,3); 
  } 
  if (suffix != "sm") {    
    bOutputMovie = false; 
    if (suffix == "tif" || suffix == "TIF")  imageType = 0; 
    else if (suffix == "sgi" || suffix == "SGI")  imageType = 1; 
    else if (suffix == "pnm" || suffix == "PNM")  imageType = 2; 
    else if (suffix == "yuv" || suffix == "YUV")  imageType = 3; 
    else if (suffix == "png" || suffix == "PNG")  imageType = 4; 
    else if (suffix == "jpg" || suffix == "JPG" || 
             suffix == "jpe" || suffix == "JPE")  imageType = 5; 
    else  {
      cerr << "Warning:  No output format was given, and cannot deduce format from output filename template.  Using PNG format but leaving filenames unchanged." << endl; 
      imageType = 4;
    }
  }    
  // ===============================================
  int		iSize[2] = {-1};
  float		fFPS = fps.getValue();
  //  int		i=0,j=0,k=0;
  void 		*buffer = NULL;
  int		buffer_len = 0;

  vector<MovieInfo> minfos; 

  int		iScale;
  pt_pool		thepool;
  pt_pool_t	pool = &thepool;
  unsigned int    tsizes[8][2];
  int             tiled = 0;

  gVerbosity = verbosity.getValue(); 
  sm_setVerbose(gVerbosity);  
  /*
    nminfos = argc-i;
    input = (inp *)malloc(sizeof(inp)*nminfos);
    if (!input) exit(1);
  */ 
  TagMap mdata; 
  
  uint numFrames = 0, n = 0;
  for(vector<string>::iterator pos = movies.begin();pos != movies.end(); ++pos, ++n) {
    MovieInfo minfo;
    vector<string> tokens; 
    boost::split(tokens, *pos, boost::is_any_of("@")); 
    minfo.name = tokens[0];
    /* handle the @first[@last[@step]] syntax */
    minfo.sm = smBase::openFile(minfo.name.c_str(),O_RDONLY,  nThreads);
    if (minfo.sm == NULL) {
      errexit(cmd, str(boost::format("Unable to open: %1%")%minfo.name));
    }
    if (fFPS == 0.0) fFPS = minfo.sm->getFPS();
    
    minfo.first = firstFrameArg.getValue();
    minfo.last = minfo.sm->getNumFrames()-1;
    if (lastFrameArg.getValue() != -1 && lastFrameArg.getValue() < minfo.last) { 
      minfo.last = lastFrameArg.getValue(); 
    } 
    minfo.step = frameStepArg.getValue();    
    try {
      if (tokens.size() > 1) {
        minfo.first = boost::lexical_cast<int>(tokens[1]); 
      } 
      if (tokens.size() > 2) {
        minfo.last = boost::lexical_cast<int>(tokens[2]); 
      } 
      if (tokens.size() > 3) {
        minfo.step = boost::lexical_cast<int>(tokens[3]); 
      } 
    } catch (...) {
      errexit(cmd, str(boost::format("Bad movie specification string \"%1%\"")%*pos)); 
    }
    if (iSize[0] == -1) {
      iSize[0] = minfo.sm->getWidth();
      iSize[1] = minfo.sm->getHeight();
    } else {
      if ((iSize[0] != minfo.sm->getWidth()) ||
          (iSize[1] != minfo.sm->getHeight())) {
        smdbprintf(0,"Error the file %s has a different framesize\n",minfo.name.c_str());
        exit(1);
      }
    }
    /* first/last/step error checking */
    if ((minfo.first < 0) || 
        (minfo.first >= minfo.sm->getNumFrames())) {
      smdbprintf(0,"Invalid first frame %d for %s\n",
                 minfo.first,minfo.name.c_str());
      exit(1);
    }
    if ((minfo.last < 0) ||
        (minfo.last >= minfo.sm->getNumFrames())) {
      smdbprintf(0,"Invalid last frame %d for %s\n",
                 minfo.last,minfo.name.c_str());
      exit(1);
    }
    if ((minfo.step == 0) || 
        ((minfo.step > 0) && 
         (minfo.last < minfo.first)) ||
        ((minfo.step < 0) && 
         (minfo.last > minfo.first))) {
      smdbprintf(0,"Invalid first/last/step values (%d/%d/%d)\n",
                 minfo.first,minfo.last,minfo.step);
      exit(1);
    }
    
    /* count the frames */
    minfo.numframes = 0;
    if (minfo.step > 0) {
      for(uint k=minfo.first;k<=minfo.last;
          k+=minfo.step) {
        minfo.numframes++;
      }
    } else {
      for(uint k=minfo.first;k>=minfo.last;
          k+=minfo.step) {
        minfo.numframes++;
      }
    }
    
    if (gVerbosity) {
      printf("Minfo: %d - %d of %s - size %d %d\n",
             minfo.first,minfo.last,
             minfo.name.c_str(),iSize[0],iSize[1]);
    }
    numFrames += minfo.numframes;
    minfos.push_back(minfo); 
  }
  
  // Ensure the bounds are good...
  int src[4] = {-1}; 
  if (subregion.getValue() == "") {
    src[0] = 0;
    src[1] = 0;
    src[2] = iSize[0];
    src[3] = iSize[1];
  } 
  else {
    std::string item;    
    std::stringstream ss(subregion.getValue());
    try {
      ss >> src[0] >> src[1] >> src[2] >> src[3];        
    } catch (...) {
      errexit (cmd, str(boost::format("Improperly formatted subregion: %1%")%subregion.getValue())); 
    }
    if (src[0] + src[2] > iSize[0]) {
      smdbprintf(0,"Invalid selection width\n");
      exit(1);
    }
    if (src[1] + src[3] > iSize[1]) {
      smdbprintf(0,"Invalid selection width\n");
      exit(1);
    }
  }
  // if no value, dst is equal to the src
  vector<int> dstSize(2,-1); 
  if (destSize.getValue() == "") {
    dstSize[0] = src[2] * destScale.getValue();
    dstSize[1] = src[3] * destScale.getValue();
  }
  else {
	bool err = false; 
    std::string item;    
    std::stringstream ss(destSize.getValue());
	char X; 
    try {
      ss >> dstSize[0] >> X >> dstSize[1];
    } catch (...) {
	  err = true; 
	}
	if (err || (X!='X' && X!='x') || dstSize[1] < 1) {
      errexit (cmd, str(boost::format("Improperly formatted dest size: %1%")%destSize.getValue())); 
    }
	smdbprintf(1, "Set size to %d x %d\n", dstSize[0], dstSize[1]); 
  }
  
  /* We used to allow tilesizes to be specified in great detail, but it is sufficient now to use same tile size at all mipmaps, since after all the I/O is the same regardless of the level of detail.  
     Formerly, you could say -tilesizes '512x256,256x128,128x64' and have things go the way you wanted them.  Not worth the hassle but worth noting here.  
  */ 
  int xsize=0,ysize=0;
  try {
    xsize = boost::lexical_cast<int32_t>(tilesizes.getValue());
    ysize = xsize;
  } catch(boost::bad_lexical_cast &) {
    smdbprintf(5, str(boost::format("Could not make %1% into an integer.  Trying MxN format.")%tilesizes.getValue()).c_str());
    typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;
    boost::char_separator<char> sep("x");
    tokenizer tok(tilesizes.getValue(), sep);
    tokenizer::iterator pos = tok.begin(), endpos = tok.end();
    try {
      if (pos != endpos) {
        xsize = boost::lexical_cast<int32_t>(*pos);
        ++pos;
      }
      if (pos != endpos) {
        ysize = boost::lexical_cast<int32_t>(*pos);
      } else {
        throw;
      }
    } catch(...) {
      errexit(cmd, str(boost::format("Bad format string %1%.")%tilesizes.getValue()));
    }
  }
  tsizes[0][0] = xsize;
  tsizes[0][1] = ysize;

  for(int n=1; n< nRes; n++) {
    tsizes[n][0] = tsizes[n-1][0];
    tsizes[n][1] = tsizes[n-1][1];
  }

  // Bad template name?  
  bool useTemplate = false; 
  if (!bOutputMovie) {
    try {
      string test = str(boost::format(outputName) % 1);
      useTemplate = true; 
    } catch(...) {
      if (numFrames > 1) {
        smdbprintf(0,"If you are outputting multiple frames, then the output specification must be a C-style sprintf template\n");
        exit(1);
      }
    }
  }
  // ok, here we go...
  iScale = 0;
  if (iSize[0] != dstSize[0]) { iSize[0] = dstSize[0]; iScale = 1; }
  if (iSize[1] != dstSize[1]) { iSize[1] = dstSize[1]; iScale = 1; }
  
  // memory for scaling buffer...
  
  /* ============================================================================== */
  // Open the output movie if we are catting to a movie: 
  smBase 		*sm = NULL;
  if (bOutputMovie) {
    unsigned int *tsizep = NULL; 
    if(tsizes[0][0]) {
      tsizep = &tsizes[0][0];
    }
    else if (compression.getValue() == "" || compression.getValue() == "gz" || compression.getValue() == "GZ") {
      if (compression.getValue() == "") {
        smdbprintf(1, "No compression given; using gzip compression by default.\n"); 
      }
      sm = new smGZ(outputName.c_str(),iSize[0],iSize[1],numFrames,tsizep,nRes, nThreads);
    } else if (compression.getValue() == "raw" || compression.getValue() == "RAW") {
      sm = new smRaw(outputName.c_str(),iSize[0],iSize[1],numFrames,tsizep,nRes, nThreads);
    } else if (compression.getValue() == "rle" || compression.getValue() == "RLE") {
      sm = new smRLE(outputName.c_str(),iSize[0],iSize[1],numFrames, tsizep,nRes, nThreads);
    } else if (compression.getValue() == "lzo" || compression.getValue() == "LZO") {
      sm = new smLZO(outputName.c_str(),iSize[0],iSize[1],numFrames,tsizep,nRes, nThreads);
    } else if (compression.getValue() == "jpeg" || compression.getValue() == "jpg" || compression.getValue() == "JPEG" || compression.getValue() == "JPG") {
      sm = new smJPG(outputName.c_str(),iSize[0],iSize[1],numFrames,tsizep,nRes, nThreads);
      ((smJPG *)sm)->setQuality(jqual.getValue());
    } else if (compression.getValue() == "lzma" || compression.getValue() == "LZMA") {
      sm = new smXZ(outputName.c_str(),iSize[0],iSize[1],numFrames,tsizep,nRes, nThreads);
    } else {
      errexit(cmd, str(boost::format("Bad encoding type: %1%")%compression.getValue()));
    }
    if (!sm) {
      smdbprintf(0,"Unable to create the file: %s\n", outputName.c_str());
      exit(1);
    }
    if (!noMetadata.getValue()) {
      for(uint i=0;i<minfos.size();i++) {
        TagMap tm = minfos[i].sm->GetMetaData();
        sm->SetMetaData(tm, string("Source movie ")+minfos[i].name + ": "); 
      }
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

    /* set any flags */
    if (stereo.getValue()) sm->setStereo(); 
    sm->setFPS(fps.getValue());
    sm->startWriteThread(); 
  }
  // End output movie prep
  /* ============================================================================== */


  /* init the parallel tools */
  pt_pool_init(pool, nThreads, nThreads*2, 0);
  
  /* copy the frames */
  uint outframe = 0; 
  for (vector<MovieInfo>::iterator pos = minfos.begin(); pos != minfos.end(); ++pos) {
    if (gVerbosity) {
      printf("Reading from %s (%d to %d by %d)\n",
             pos->name.c_str(),pos->first,pos->last,
             pos->step);
    }
    for(int fnum = pos->first; fnum <= pos->last; fnum += pos->step) {
      if (gVerbosity) {
        printf("Working on %d of %d (frame %d)\n",fnum,numFrames, fnum);
      }
      Work *wrk = new Work;
      wrk->insm = pos->sm;
      wrk->sm = sm;
      wrk->inframe = fnum;
      wrk->outframe = outframe;
      wrk->iScale = iScale;
      wrk->iFilter = filter.getValue();
      wrk->dstSize = dstSize;
      wrk->src = src;
      if (!bOutputMovie) {
        if (useTemplate) {
          wrk->filename = str(boost::format(outputName) % outframe);
        } 
        else {
          wrk->filename = outputName; 
        } 
        wrk->imageType = imageType; 
        wrk->jqual = jqual.getValue(); 
        wrk->verbosity = verbosity.getValue(); 
      }       
      pt_pool_add_work(pool, workproc, wrk);
      outframe++;
    }
  } 
  pt_pool_destroy(pool,1);
  if (sm) {
    sm->stopWriteThread(); 
    sm->flushFrames(true); 
    
    sm->closeFile();
    
    if (sm->haveError()) {
      cout << "Got error in movie creation.  No file is created.\n"; 
      sm->deleteFile(); 
      exit(1); 
    } 
    cout << "smcat successfully created movie " << sm->getName() << endl; 
  } 
  else {
    cout << "smcat successfully created frames" << endl; 
  }
    
  /* clean up */
  // delete sm;
  //   for(i=0;i<nminfos;i++) delete input[i].sm;
  // free(input);
  if (buffer) free(buffer);
  exit(0);
}

void workproc(void *arg)
{
  Work *work = (Work *)arg;
  int	sizeout = 3*work->dstSize[0]*work->dstSize[1]; 
  unsigned char * img = new unsigned char[sizeout]; 

  int	sizein = 3*work->insm->getHeight()*work->insm->getWidth();
  work->buffer = new unsigned char[sizein];
  work->insm->getFrame(work->inframe, &work->buffer[0], pt_pool_threadnum());
  if (work->iScale) {
    unsigned char *pZoom = new unsigned char[sizeout];
    Sample2d(work->buffer, work->insm->getWidth(),work->insm->getHeight(),
             pZoom, work->dstSize[0],work->dstSize[1],
             work->src[0],work->src[1],
             work->src[2],work->src[3], work->iFilter);
    smdbprintf(4, "workproc resampled work buffer to size %d for frame %d\n", sizeout, work->outframe); 
    delete work->buffer; 
    work->buffer = pZoom;
  }
  if (work->sm) {
    work->sm->compressAndBufferFrame(work->outframe, &work->buffer[0]);
  } else {
    WriteOutputImage(work); 
  }

  delete work;
}

void WriteOutputImage(Work *work) {

  switch(work->imageType) {
  case 0: {  // TIFF
    unsigned char *p;
    TIFF		*tif = TIFFOpen(work->filename.c_str(),"w");
    if (tif) {
      // Header stuff 
#ifdef Tru64
      TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32)work->dstSize[0]);
      TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)work->dstSize[1]);
#else
      TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (unsigned int)work->dstSize[0]);
      TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (unsigned int)work->dstSize[1]);
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
        
      for(int y=0;y<work->dstSize[1];y++) {
        p = &work->buffer[0] + (work->dstSize[0]*3)*(work->dstSize[1]-y-1);
        TIFFWriteScanline(tif,p,y,0);
      }
      TIFFFlushData(tif);
      TIFFClose(tif);
    }
  }
    break;
  case 1: {  // SGI
    vector<unsigned short> buf(work->dstSize[0]+1);
    sgi_t		*libi = sgiOpen((char*)work->filename.c_str(),SGI_WRITE,SGI_COMP_RLE,1,
                                work->dstSize[0],work->dstSize[1],3);
    if (libi) {
      for(int y=0;y<work->dstSize[1];y++) {
        for(int j=0;j<3;j++) {
          for(int x=0;x<work->dstSize[0];x++) {
            buf[x]=work->buffer[(x+(y*work->dstSize[0]))*3+j];
          }
          sgiPutRow(libi,&buf[0],y,j);
        }
      }
      sgiClose(libi);
    }
  }
    break;
  case 2: {  // PNM
    FILE *fp = pm_openw((char*)work->filename.c_str());
    if (fp) {
      xel* xrow;
      xel* xp;
      xrow = pnm_allocrow( work->dstSize[0] );
      pnm_writepnminit( fp, work->dstSize[0], work->dstSize[1], 
                        255, PPM_FORMAT, 0 );
      for(int y=work->dstSize[1]-1;y>=0;y--) {
        xp = xrow;
        for(int x=0;x<work->dstSize[0];x++) {
          int r1,g1,b1;
          r1 = work->buffer[(x+(y*work->dstSize[0]))*3+0];
          g1 = work->buffer[(x+(y*work->dstSize[0]))*3+1];
          b1 = work->buffer[(x+(y*work->dstSize[0]))*3+2];
          PPM_ASSIGN( *xp, r1, g1, b1 );
          xp++;
        }
        pnm_writepnmrow( fp, xrow, work->dstSize[0], 
                         255, PPM_FORMAT, 0 );
      }
      pnm_freerow(xrow);
      pm_closew(fp);
    }
  }
    break;
  case 3: {  // YUV
    int dx = work->dstSize[0] & 0xfffffe;
    int dy = work->dstSize[1] & 0xfffffe;
      
    /* convert RGB to YUV  */
    unsigned char *p = &work->buffer[0];
    for(int y=0;y<work->dstSize[1];y++) {
      for(int x=0;x<work->dstSize[0];x++) {
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
      
    unsigned char * buf = new unsigned char[(int)(1.5*dx*dy) + 1];
    unsigned char *Ybuf = &buf[0];
    unsigned char *Ubuf = Ybuf + (dx*dy);
    unsigned char *Vbuf = Ubuf + (dx*dy)/4;
    /* pull apart into Y,U,V buffers */
    /* down-sample U/V */
    for(int y=0;y<dy;y++) {
      p = &work->buffer[0] + (work->dstSize[1]-y-1)*3*work->dstSize[0];
      for(int x=0;x<dx;x++) {
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
    FILE *fp = fopen(yuvname.c_str(),"wb");
    if (fp) {
      fwrite(&buf[0],dx*dy,1,fp);
      fclose(fp);
    }

    yuvname = work->filename + ".U"; 
    fp = fopen(yuvname.c_str(),"wb");
    if (fp) {
      fwrite(&buf[dx*dy], dx*dy/4, 1, fp);
      fclose(fp);
    }

    yuvname = work->filename + ".V"; 
    fp = fopen(yuvname.c_str(),"wb");
    if (fp) {
      fwrite(&buf[dx*dy/4], dx*dy/4, 1, fp);
      fclose(fp);
    }
  }
    break;
  case 4: {  // PNG
    write_png_file((char*)work->filename.c_str(), &work->buffer[0], &work->dstSize[0]);
  }
    break;
  case 5: {  // JPEG
    write_jpeg_image((char*)work->filename.c_str(), &work->buffer[0], &work->dstSize[0], work->jqual);
  }
    break;
  }
  return; 
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
