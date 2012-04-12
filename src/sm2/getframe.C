/*
  Test module just to get a frame from disk. 
*/
#include "CImg.h"
#include <iostream>

using namespace cimg_library;
using namespace std;
// 
void getFrame(string filename, CImg<unsigned char> &cimg) {
  // open file
  // read frame
  return; 
}


int main (int argc, char *argv[]){


  // PNG image frame capture from langer movie: 
  string filename = "/Users/cook47/dataAndImages/testm.png";
  CImg<unsigned char> cimg(filename.c_str()); 

  CImgDisplay displayer(cimg, "Testing");
  int64_t w = cimg.width(),
    h = cimg.height(),
    d = cimg.depth(),
    s = cimg.size();     
  int64_t computed = h*w*d * 3; 

  cerr << "Image " << filename << ", displayer w,h,d, computed size, size = (" << w << ", " << h << ", " << d << ", " << computed << ", " << s << ")"<< endl; 
  displayer.wait(5*1000); 

  cerr << "Changing image" << endl; 
  unsigned char *ptr = cimg.data(); 
  memset(ptr, 255, s); 
  displayer.display(cimg); 
  displayer.wait(5*1000); 
  
  //  usleep(15*1000*1000); 
  // OK, now we need to get a single frame from an SM file
  
  return 0; 
}
