
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "errmsg.h"
#include <QDir>
using namespace std; 

//===================================================
string GetLocalHostname(void) {
  char myHostName[1000]; 
  if (gethostname(myHostName, 1000) != 0) {
    return string("");
  }
  return string(myHostName); 
}

//===================================================
QString MakeAbsolutePath( QString path) {
  return QDir(path).absolutePath(); 
}

//=======================================
QString ParentDir( QString path) {
  path = QDir(path).absolutePath(); 
  while (path.size() > 1 && path.endsWith('/')) {
    path.chop(1);
  }  
  if (path == "/") return path; 
  int lastSlash = path.lastIndexOf("/");   
  return path.remove(lastSlash+1, path.size());//truncate last segment   
}
