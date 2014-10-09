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

 
  TCLAP::CmdLine  cmd(str(boost::format("%1% concats movies together. ")%argv[0]), ' ', BLOCKBUSTER_VERSION); 
  
  TCLAP::ValueArg<int> verbosity("v", "Verbosity", "Verbosity level",false, 0, "integer", cmd);   
  
  TCLAP::SwitchArg noMetadata("X", "no-metadata", "Do not copy or create metadata in the output movie.  Default: accumulate metadata in the order of the movies given.", cmd, false);

  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.", cmd);
 
  TCLAP::SwitchArg exportTagfile("E", "export-tagfile", "Create a tag file from the current session which can be read by img2sm or smtag.", cmd);

  TCLAP::ValueArg<string> tagfile("G", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd);

  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd);

  TCLAP::ValueArg<string> delimiter("D", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd);

  TCLAP::ValueArg<int> thumbnail("N", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd);

  TCLAP::ValueArg<int> thumbres("R", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd);

  TCLAP::SwitchArg report("", "report", "After all operations are complete, list all the tags in the file.", cmd);

  TCLAP::SwitchArg quiet("q", "quiet", "Do not echo the tags to stdout.  Just return 0 on successful match. ", cmd);

  TCLAP::ValueArg<int> firstFrameArg("f", "first", "First frame number in each source movie",false, 0, "integer", cmd);
  TCLAP::ValueArg<int> lastFrameArg("l", "last", "Last frame number in each source movie",false, -1, "integer", cmd);
  TCLAP::ValueArg<int> frameStepArg("s", "step", "Frame step size in each source movie",false, 1, "integer", cmd);


  TCLAP::SwitchArg stereo("S", "stereo", "output movie is stereo.", cmd, false);
  TCLAP::SwitchArg filter("", "filter", "Enable smoothing filter for image scaling", cmd, false);
  TCLAP::ValueArg<int> 
    threads("t","threads","number of threads to use (default: 8)", false,8,"integer",cmd);

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
  TCLAP::ValuesConstraint<string> *allowed = new TCLAP::ValuesConstraint<string>(allowedcompression);
  if (!allowed)
    errexit(cmd, "Cannot create values constraint for compression\n");

  TCLAP::ValueArg<string>compression("c", "compression", "Compression to use on movie", false, "gz", allowed);
  cmd.add(compression);

  TCLAP::ValueArg<int> jqual("j",  "jqual",  "JPEG quality (0-99, default 75).  Higher is less compressed and higher quality.", false,  75,  "integer (0-99)",  cmd);   
  
  TCLAP::ValueArg<int> mipmaps("", "mipmaps", "Number of levels of detail",false, 1, "integer", cmd);   
  TCLAP::ValueArg<string> tilesizes("",  "tilesizes",  "Pixel size of the tiles within each frame (default: 0 -- no tiling).  Examples: '512' or '512x256'", false,  "0",  "M or MxN",  cmd); 
  //  TCLAP::ValueArg<int> tilesizes("",  "tilesizes",  "Pixel size of the tiles within each frame (default: 0 -- no tiling).  Examples: '512' or '512,256'", false,  0,  "integer",  cmd); 
  
  TCLAP::ValueArg<string> destSize("", "dest-size", "Set output frame dimensions. Default: input size.  Format: XXXxYYY e.g. '300x500'",false, "", "e.g. '300x500'", cmd);   
  TCLAP::ValueArg<string> subregion("", "src-subregion", "Select a rectangle of the input by giving region size and offset. Default: all. Format: 'Xoffset Yoffset Xsize Ysize' e.g. '20 50 300 500'",false, "", "'Xoffset Yoffset Xsize Ysize'", cmd);   
  TCLAP::ValueArg<float> fps("r", "framerate", "Store Frames Per Second (FPS) in movie for playback (float).  Might be ignored.",false, 30, "positive floating point number", cmd);   
  
  // TCLAP::UnlabeledValueArg<string> output("Output-movie", "Name of the movie to create", true, "", "output movie name", cmd); 
  
  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "Name of the input movie(s) to cat, followed by the name of the movie to create. Syntax for input movies is \"file[@first[@last[@step]]]\", allowing the first, last and frame step to be specified for each input .sm file individually. The default is to take all frames in an input file, stepping by 1.  You can also use --first, --last and --step as global defaults, overridden by the @ syntax if present for any movie..",true, "input movie(s)> <output movie", cmd); 
  
  
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
  
  vector<string> movies = movienames.getValue(); 
  if (movies.size() < 2) {
    errexit(cmd, "You must supply at least one input movie and one output movie"); 
  }
  string outputMovie = movies[movies.size()-1]; 
  movies.pop_back(); 

  // ===============================================
  int		iSize[2] = {-1};
  float		fFPS = fps.getValue();
  //  int		i=0,j=0,k=0;
  smBase 		*sm = NULL;
  void 		*buffer = NULL;
  void		*pZoom = NULL;
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
  
  uint count = 0, n = 0;
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
    count += minfo.numframes;
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
  int dst[2] = {-1}; 
  if (destSize.getValue() == "") {
    dst[0] = src[2];
    dst[1] = src[3];
  }
  else {
	bool err = false; 
    std::string item;    
    std::stringstream ss(destSize.getValue());
	char X; 
    try {
      ss >> dst[0] >> X >> dst[1];
    } catch (...) {
	  err = true; 
	}
	if (err || (X!='X' && X!='x') || dst[1] < 1) {
      errexit (cmd, str(boost::format("Improperly formatted dest size: %1%")%destSize.getValue())); 
    }
	smdbprintf(1, "Set size to %d x %d\n", dst[0], dst[1]); 
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
  unsigned int *tsizep = NULL; 
  if(tsizes[0][0]) {
    tsizep = &tsizes[0][0];
  }
  else if (compression.getValue() == "" || compression.getValue() == "gz" || compression.getValue() == "GZ") {
    if (compression.getValue() == "") {
    smdbprintf(1, "No compression given; using gzip compression by default.\n"); 
    }
    sm = new smGZ(outputMovie.c_str(),iSize[0],iSize[1],count,tsizep,nRes, nThreads);
  } else if (compression.getValue() == "raw" || compression.getValue() == "RAW") {
    sm = new smRaw(outputMovie.c_str(),iSize[0],iSize[1],count,tsizep,nRes, nThreads);
  } else if (compression.getValue() == "rle" || compression.getValue() == "RLE") {
    sm = new smRLE(outputMovie.c_str(),iSize[0],iSize[1],count, tsizep,nRes, nThreads);
  } else if (compression.getValue() == "lzo" || compression.getValue() == "LZO") {
    sm = new smLZO(outputMovie.c_str(),iSize[0],iSize[1],count,tsizep,nRes, nThreads);
  } else if (compression.getValue() == "jpeg" || compression.getValue() == "jpg" || compression.getValue() == "JPEG" || compression.getValue() == "JPG") {
    sm = new smJPG(outputMovie.c_str(),iSize[0],iSize[1],count,tsizep,nRes, nThreads);
    ((smJPG *)sm)->setQuality(jqual.getValue());
  } else if (compression.getValue() == "lzma" || compression.getValue() == "LZMA") {
    sm = new smXZ(outputMovie.c_str(),iSize[0],iSize[1],count,tsizep,nRes, nThreads);
  } else {
    errexit(cmd, str(boost::format("Bad encoding type: %1%")%compression.getValue()));
  }
  if (!sm) {
  smdbprintf(0,"Unable to create the file: %s\n", outputMovie.c_str());
    exit(1);
  }
  if (!noMetadata.getValue()) {
    for(uint i=0;i<minfos.size();i++) {
      TagMap tm = minfos[i].sm->GetMetaData();
      sm->SetMetaData(tm, string("Source movie ")+minfos[i].name + ": "); 
    }
    try {
      sm->SetMetaData(commandLine, tagfile.getValue(), canonical.getValue(), delimiter.getValue(), taglist.getValue(), thumbnail.getValue(), thumbres.getValue(), exportTagfile.getValue(), quiet.getValue());
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
        printf("Working on %d of %d (frame %d)\n",fnum,count, fnum);
      }
      if (0 && nThreads == 1 
          /* && (pos->sm->getType() == sm->getType()) && 
             (iScale == 0) && 
             (sm->getNumResolutions() == pos->sm->getNumResolutions())*/
          ) {
        int	size,res;
        for(res=0;res<sm->getNumResolutions();res++) {
          pos->sm->getCompFrame(fnum, 0, NULL, size, res);
          if (buffer_len < size) {
            free(buffer);
            buffer = (void *)malloc(size);
            buffer_len = size;
          }
          pos->sm->getCompFrame(fnum, 0, buffer, size, res);
          sm->writeCompFrame(outframe, buffer, &size, res);
        }
      } else {
        Work *wrk = new Work;
        wrk->insm = pos->sm;
        wrk->sm = sm;
        wrk->inframe = fnum;
        wrk->outframe = outframe;
        wrk->iScale = iScale;
        wrk->iFilter = filter.getValue();
        wrk->dst = dst;
        wrk->src = src;
        pt_pool_add_work(pool, workproc, wrk);
      }
      outframe++;
    }
  }
  pt_pool_destroy(pool,1);
  sm->stopWriteThread(); 
  sm->flushFrames(true); 
  
  sm->closeFile();
  
  if (sm->haveError()) {
    cout << "Got error in movie creation.  No file is created.\n"; 
    sm->deleteFile(); 
    exit(1); 
  } 

  /* clean up */
  // delete sm;
  //   for(i=0;i<nminfos;i++) delete input[i].sm;
  // free(input);
  if (pZoom) free(pZoom);
  if (buffer) free(buffer);
  cout << "smcat successfully created movie " << sm->getName() << endl; 
  exit(0);
}

void workproc(void *arg)
{
	Work *wrk = (Work *)arg;

	int	sizein = 3*wrk->insm->getHeight()*wrk->insm->getWidth();
	int	sizeout = 3*wrk->sm->getHeight()*wrk->sm->getWidth();
	int	size;
	wrk->buffer = new unsigned char[sizein];
  smdbprintf(3, "created new buffer %p for frame %d\n", 
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
      smdbprintf(4, "workproc deleting new buffer %p, replacing with %p for frame %d\n", wrk->buffer, pZoom, wrk->outframe); 
		delete wrk->buffer;
		wrk->buffer = pZoom;
	}

    wrk->sm->compressAndBufferFrame(wrk->outframe,wrk->buffer);

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
