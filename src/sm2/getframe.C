/*
  Test module just to get a frame from disk. 
*/
#include "CImg.h"
#include <iostream>
#include "StreamingMovie.h"
using namespace cimg_library;
using namespace std;
// 
void getFrame(string filename, CImg<unsigned char> &cimg) {
  // open file
  // read frame
  return; 
}


int main (int argc, char *argv[]){

  // First we need to get a single frame from an SM file
  smSetVerbose(4); 
  StreamingMovie sm("/Users/cook47/dataAndImages/langer.sm"); 
  sm.ReadHeader(); 
  CImg<unsigned char> cimg; 
  uint32_t framenum = 100, lod = 0; 
  vector<unsigned char> readbuffer; 
  sm.FetchFrame(framenum, lod, cimg, readbuffer);

  // PNG image frame capture from langer movie: 
  string filename = "/Users/cook47/dataAndImages/testm.png";
  cimg.load(filename.c_str()); 
  int64_t w = cimg.width(),
    h = cimg.height(),
    d = cimg.depth(),
    s = cimg.size();     
  int64_t computed = h*w*d * 3; 
  cimg.crop(200,200,600,450); 
  CImgDisplay displayer(cimg, "Testing");
  cerr << "Image " << filename << ", displayer w,h,d, computed size, size = (" << w << ", " << h << ", " << d << ", " << computed << ", " << s << ")"<< endl; 
  displayer.wait(3*1000); 

  cerr << "Resizing to new size"<< endl; 
  displayer.resize(w,h); 

  displayer.wait(3*1000); 

  cerr << "Changing image" << endl; 
  unsigned char *ptr = cimg.data(); 
  memset(ptr, 255, cimg.size()); 
  displayer.display(cimg); 
  displayer.wait(5*1000); 
  
  //  usleep(15*1000*1000); 
  return 0; 
}
