/*
  Test module just to get a frame from disk. 
*/
#include "CImg.h"
#include <iostream>
#include "StreamingMovie.h"
#include "timer.h"
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
  cerr << "sm width and height are " << sm.Width() << ", "<< sm.Height() << endl; 
  CImg<unsigned char> cimg(sm.Width(), sm.Height(),1,3); 
  CImgDisplay displayer;
  /*  int i=0, j=0; 
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
  */

  
  // play a movie
  //CImg<unsigned char> cimg(640, 480,3); 
  uint32_t framenum = 0, numframes = sm.NumFrames(); 
  if (numframes < 0) {
    cerr << "Error:  bad movie -- numframes is < 0" << endl;
    exit(1); 
  }


  // play a movie with optimal buffering to see what that does for us
#define BUFFER_MOVIE 1
  timer bufferTimer, chunkTimer; 
  if (BUFFER_MOVIE) {
    vector<CImg<unsigned char> > cimglist; 
    while (framenum < numframes ) {
      sm.FetchFrame(framenum++, cimg); 
      cimglist.push_back(cimg); 
    }
    CImgDisplay bufdisplayer(cimglist[0], "blah"); 
    // display all frames: 
    framenum = 0;
    bufferTimer.start(); 
    chunkTimer.start(); 
    float buft = 0; 
    while (framenum < numframes ) {      
      bufdisplayer.display(cimglist[framenum].get_crop(200,200,600,450)); 
      framenum++; 
      if (framenum % 50 == 0) {
	buft = chunkTimer.total_time(); 
	cerr << "displayed "<< 50 << " frames in " << buft << " seconds for FPS of " << 50/buft << endl; 
	chunkTimer.restart(); 
      }
    }
    buft = bufferTimer.total_time(); 
    cerr << "displayed "<<numframes << " frames in " << buft << " seconds for FPS of " << numframes/buft << endl; 
  } 
#define PLAY_MOVIE 0
  if (PLAY_MOVIE) {
    // unbuffered version: 
    timer theTimer; 
    theTimer.start(); 
    sm.FetchFrame(framenum, cimg);
    displayer.display(cimg);
    cerr << "after sm fetch frame, cimg has width,height size of " << cimg.width() << ", " << cimg.height() << ", " << cimg.size() << endl; 
    // vector<unsigned char> readbuffer; 
    while (framenum < numframes ) {
      sm.FetchFrame(framenum++,  cimg); 
      displayer.display(cimg);     
    } 
    theTimer.stop(); 
    float t = theTimer.total_time(); 
    cerr << "displayed "<<numframes << " frames in " << t << " seconds for FPS of " << numframes/t << endl; 
    
    // displayer.display(cimg);
    // displayer.show(); 
    displayer.wait(3*1000); 
  }


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
