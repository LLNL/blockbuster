/*
  Proof of concept:  load an image from langer.sm and display it using Qt.
  Internal images in blockbuster are width*height*24 bits (3 bytes).  Very simple.  
*/
#include "../sm/smBase.h"
#include "smglwidget.h"
#include <QImage>
#include <QApplication>
int main(int argc, char *argv[]){
  QApplication app(argc, argv); 
  // Initialize the SM Library
  smBase::init();
  smBase *smbase = smBase::openFile("/g/g0/rcook/dataAndImages/langer.sm", 1);
  int size[2]={640,480}, pos[2]={0,0}, step[2]={1,1}; 
  // create memory for one frame of langer.sm
  char *imagep = (char*)calloc(640*480*3, sizeof(char)); 
  smbase->getFrameBlock(50, imagep, 0, 1920, size, pos, step, 0);
  printf("image[1] = %d\n", (int)(imagep[1])); 

  //  The Qt way: 
  SMGLWidget w(imagep); 
  w.setWindowFlags( Qt::FramelessWindowHint);
  //w.show(); 
  w.showFullScreen();
  w.updateGL();
  app.exec(); 
  
  
  return 0; 
}

