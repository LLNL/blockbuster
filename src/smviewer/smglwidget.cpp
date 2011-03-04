
#include "smglwidget.h"
#include <QtOpenGL>
void SMGLWidget::paintGL(){
  glBitmap(0, 0, 0, 0, 0, 0, NULL);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 640);
  glDrawPixels(640,480,GL_RGB, GL_UNSIGNED_BYTE, mData); 
  return; 
}
void SMGLWidget::resizeGL(int width, int height) {
  paintGL(); 
}
