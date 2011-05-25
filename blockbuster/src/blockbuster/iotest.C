/*!
  iotest == simulates blockbuster I/O patterns to try to analyze the performance implication of various choices. 
*/ 
#include "../common/timer.h"
#include "errno.h"
#include <iostream> 
using namespace std; 

void usage(const string &prog ) {
  cerr << "usage:  " << prog << " filename " << endl;
  return; 
}

int main(int argc, char *argv[]) {
  // pretend a 40% "compression ratio" and choose "typical sizes"
  uint32_t nframes = 100, tilesize = 2*512/5, frameSize=2*(2000*2000)/5;

  if (argc != 2) {
    cerr << "Error:  need filename to use as \"movie\"" << endl;
    usage(argv[0]); 
    exit(1); 
  }  
  string moviename = argv[1]; 
  
  int fd = fopen(moviename.c_str(), O_RDONLY); 
  if (fd == -1) {
    cerr << "Error: could not open file " << moviename << ", " << strerror(errno) << endl; 
    exit(2); 
  }
  return 0; 
}
