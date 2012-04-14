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
  StreamingMovie sm("/Users/cook47/dataAndImages/langerGZ.sm"); 
  sm.ReadHeader(); 
  cerr << "sm width and height are " << sm.Width(0) << ", "<< sm.Height(0) << endl; 
  CImg<unsigned char> cimg(sm.Width(0), sm.Height(0),1,3); 
  int i=0, j=0; 
  while (j<480) {
    i=0; 
    while (i<640) {
      
      cimg(i,j,0,0) = (i/640.0)*255.0;
      i++;
    }
    j++; 
  }
  CImgDisplay displayer(cimg, "Testing");
  displayer.wait(3*1000); 
  
  //CImg<unsigned char> cimg(640, 480,3); 
  uint32_t framenum = 100, lod = 0; 
  // vector<unsigned char> readbuffer; 
  sm.FetchFrame(framenum, lod, cimg);
  cerr << "after sm fetch frame, cimg has width,height size of " << cimg.width() << ", " << cimg.height() << ", " << cimg.size() << endl; 
  displayer.display(cimg);
  displayer.show(); 
  displayer.wait(3*1000); 

  // PNG image frame capture from langer movie: 
  string filename = "/Users/cook47/dataAndImages/testm.png";
  cimg.load(filename.c_str()); 
  int64_t w = cimg.width(),
    h = cimg.height(),
    d = cimg.depth(),
    s = cimg.size();     
  int64_t computed = h*w*d * 3; 

  cerr << "Image " << filename << ", displayer w,h,d, computed size, size = (" << w << ", " << h << ", " << d << ", " << computed << ", " << s << ")"<< endl; 
  cimg.crop(200,200,600,450); 
  displayer.display(cimg); 
  displayer.wait(3*1000); 

  cerr << "After cropping to 400x250: " << endl; 
  cerr << "Image offset(0,0,0,0) = "<< cimg.offset(0,0,0,0)<< endl; 
  cerr << "Image offset(0,0,0,1) = "<< cimg.offset(0,0,0,1)<< endl; 
  cerr << "Image offset(0,0,0,2) = "<< cimg.offset(0,0,0,2)<< endl; 
  cerr << "Image offset(1,0,0,0) = "<< cimg.offset(1,0,0,0)<< endl; 
  cerr << "Image offset(1,0,0,1) = "<< cimg.offset(1,0,0,1)<< endl; 
  cerr << "Image offset(0,1,0,0) = "<< cimg.offset(0,1,0,0)<< endl; 

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
