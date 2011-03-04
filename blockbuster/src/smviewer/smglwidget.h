
#ifndef SMGLWIDGET_H
#define SMGLWIDGET_H

#include <QGLWidget>


class SMGLWidget: public QGLWidget {
  Q_OBJECT
public:
  SMGLWidget(char *image, QWidget *parent = 0): 
    mData(image){ return; }
  ~SMGLWidget(){ return; }

protected:
  void initializeGL(){}
    void paintGL();
    void resizeGL(int width, int height);
 private: 
    char *mData; 
}; 



#endif
