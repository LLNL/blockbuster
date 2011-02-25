/*
  Proof of concept:  load an image from langer.sm and display it using Qt.
  Internal images in blockbuster are width*height*24 bits (3 bytes).  Very simple.  
*/
#include "../sm/smBase.h"

int main(int argc, char *argv[]){
  // Initialize the SM Library
  smBase::init();
  smBase *smbase = smBase::openFile("/g/g0/rcook/dataAndImages/langer.sm", 1);
  int size[2]={640,480}, pos[2]={0,0}, step[2]={1,1}; 
  // create memory for one frame of langer.sm
  char *image = (char*)calloc(640*480*3, sizeof(char)); 
  smbase->getFrameBlock(50, image, 0, 1920, size, pos, step, 0);
  printf("image[1] = %d\n", (int)(image[1])); 
  return 0; 
}
