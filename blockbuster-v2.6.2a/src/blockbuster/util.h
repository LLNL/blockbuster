#ifndef BLOCKBUSTER_UTIL_H
#define BLOCKBUSTER_UTIL_H yessirree
#include <string>
QString MakeAbsolutePath(QString path);
// returns an absolute path:
QString ParentDir(const QString path);
std::string GetLocalHostname(void) ;


#endif
