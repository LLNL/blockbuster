/* 
   Purpose:  provide widgets for the RemoteControl that allow a "lock" to be set such that they cannot be changed by a blockbuster update until some condition is met.  This prevents an "echo" effect that happened when users changed a spin box, for example, but blockbuster was playing and sent an update before the spinbox change got to it, then the change occured and the spinbox changed to what they user set, etc...  
 */ 

#ifndef LOCKABLES_H
#define LOCKABLES_H LOCKABLES_H
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QString>
#include <iostream>
#include <math.h>

using namespace std; 
/* All of these classes simply must have a function Unlock(arg) that takes a single argument so my macros work in RemoteControl.cpp, and they need to store the lock/test value internally, so they can be used from two contexts, that of the RemoteControl and that of sidecar.cpp.
 */ 

class LockableSpinBox: public QSpinBox {
 public:
  LockableSpinBox(QWidget *parent = NULL): QSpinBox(parent), mLockValue(-42) {}
    ~LockableSpinBox() {}
      bool Unlock(int testval) { 
        if ( mLockValue == -42 || testval == mLockValue)  {
          mLockValue = -42; 
          return true; 
        } 
        return false; 
      }
    void Lock(int lockval ) {mLockValue = lockval; }
    int mLockValue; 
}; 

class LockableDoubleSpinBox: public QDoubleSpinBox{
 public:
  LockableDoubleSpinBox(QWidget *parent = NULL): QDoubleSpinBox(parent), mLockValue(-42) {}
    ~LockableDoubleSpinBox(){}
    bool Unlock(double testval) { 
      if ( mLockValue == -42 || fabs(testval - mLockValue) < fabs(mLockValue/1000))  {
        mLockValue = -42; 
         return true; 
      } 
      return false; 
    }
    void Lock(double lockval ) {mLockValue = lockval; }
    double mLockValue; 
    
}; 

class LockableLineEdit: public QLineEdit {
 public:
  LockableLineEdit(QWidget *parent = NULL): QLineEdit(parent), mLockValue("-42") {}
    ~LockableLineEdit(){}
    
    bool Unlock(double testval) {
      return Unlock(QString("%s").arg(testval)); 
    }
    bool Unlock(QString testval) { 
      if ( mLockValue == "-42" || testval == mLockValue)  {
        mLockValue = "-42"; 
        return true; 
      } 
      return false; 
    }
    void Lock(QString lockval ) {mLockValue = lockval; }
    QString mLockValue; 
    
}; 

//=================================================================
// custom double spin box to allow special behavior for zooming 
// ===============================================================
class ZoomSpinBox: public LockableDoubleSpinBox {
 public:
  ZoomSpinBox(QWidget *parent = NULL): LockableDoubleSpinBox(parent) {}
    ~ZoomSpinBox() {}
    void stepBy(int steps) {
      if (!steps ) return; 
      double val = value(); 
      float factor = 1.250; 
      if (steps < 0) {
        factor = 1.0/factor; 
        steps *= -1; 
      }
      while (steps--) {
        val *= factor; 
      } 
      setValue(val); 
      return; 
    }
}; 

#endif
