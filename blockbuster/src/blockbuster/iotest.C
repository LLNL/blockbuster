/*!
  iotest == simulates blockbuster I/O patterns to try to analyze the performance implication of various choices. 
*/ 
#include "../RC_cpp_lib/timer.h"
#include "errno.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream> 
using namespace std; 

void usage(const string &prog ) {
  cerr << "usage:  " << prog << " filename  nframes framebytes ntiles readPercent" << endl;
  return; 
}

int main(int argc, char *argv[]) {
  // pretend a 40% "compression ratio" and choose "typical sizes"
  int err = 0; 
  if (argc != 6) {
    cerr << "Error:  need five arguments" << endl;
    usage(argv[0]); 
    exit(1); 
  }  
  string moviename = argv[1]; 
  uint64_t nframes = atoi(argv[2]),
    framebytes = atoi(argv[3]),
    ntiles = atoi(argv[4]), 
    readpercent = atoi(argv[5]); 
  
  cerr << "running " << argv[0] << ", moviename is " << moviename << ", nframes is " << nframes << ", framebytes is " << framebytes << ", ntiles is "<< ntiles<< ", readpercent is " << readpercent << "." << endl; 
  uint64_t tilebytes= framebytes/ntiles, 
    tilesToReadPerFrame = readpercent * ntiles / 100;
  cerr << "tilebytes = "<< tilebytes << " and tilesToReadPerFrame = " << tilesToReadPerFrame << endl; 

  int fd = open(moviename.c_str(), O_RDONLY); 
  if (fd == -1) {
    cerr << "Error: could not open file " << moviename << ", " << strerror(errno) << endl; 
    exit(2); 
  }
  FILE * file = fdopen(fd, "r"); 
  err = fseek(file, 0, SEEK_END); 
  uint64_t fileLen = ftell(file); 
  cerr << "frameSize = " << framebytes<< " and nframes = " << nframes << ", so we need " << nframes * framebytes/1000000 << " megabytes at least " << endl; 
  cerr << "file " << moviename << " is " << fileLen/(1000*1000) << " megabytes long" << endl; 
  if (fileLen < nframes * framebytes) {
    cerr << "That is too short!" << endl; 
    exit(3); 
  } else {
    cerr << "Good, long enough."<< endl;
  }

  cerr << "Simulating file I/O..." << endl; 
  vector<u_char> inBuffer(framebytes, 0); 
 
  timer Timer; 
  Timer.start(); 
  uint32_t frame =0; 
  uint64_t bytesRead = 0; 
  while (frame < nframes) {
    fseek(file, frame*framebytes, SEEK_SET); 
    uint32_t tilesRemainingToRead = tilesToReadPerFrame;
    uint32_t tilesRemainingToSkip = ntiles-tilesToReadPerFrame;
    uint32_t buffOffset = 0; 
    while (tilesRemainingToRead || tilesRemainingToSkip) {
      if (tilesRemainingToRead) {
        fread(&inBuffer[buffOffset], tilebytes, 1, file);
        bytesRead += tilebytes; 
        tilesRemainingToRead--; 
        buffOffset += tilebytes; 
      } 
      if (tilesRemainingToSkip) {
        fseek(file, tilebytes, SEEK_CUR); 
        tilesRemainingToSkip--;
      }
    }
    ++frame; 
  }
  double elapsed = Timer.elapsed_time(); 
  cerr << "Done reading all frames, time is " << elapsed << ", bytesRead = "<< bytesRead/1000000 <<" MB for " << nframes/elapsed << " FPS and " << bytesRead/1000000/elapsed << " MB/second " <<  endl;
  cerr << "30th byte is " << (int)(inBuffer[30]) << endl; 
  return 0; 
}
