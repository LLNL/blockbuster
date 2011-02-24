/*
  Proof of concept:  load an image from langer.sm and display it using Qt.
  Internal images in blockbuster are width*height*24 bits (3 bytes).  Very simple.  
*/
#include "../sm/smBase.h"

int main(int argc, char *argv[]){
  // Initialize the SM Library
   smBase::init();
   smBase *smbase = smBase::openFile("/g/g0/rcook/dataAndImages/langer.sm", 1);
  
  // create memory for one frame of langer.sm
  char *image = (char*)calloc(640*480*3, 1); 
  
  return 0; 
}
