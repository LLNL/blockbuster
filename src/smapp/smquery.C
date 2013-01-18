#include <tclap_utils.h>
#include <sm/sm.h>
#include <vector>

using namespace std; 

int main(int argc, char *argv[]) {
  string filename; 
  int nThreads = 0; 
  smBase *sm = smBase::openFile(filename.c_str(), nThreads);
  printf("Metadata: (%d entries)\n", sm->mMetaData.size()); 
  vector <SM_MetaData>::iterator pos = sm->mMetaData.begin(), endpos = sm->mMetaData.end(); 
  if (pos == endpos) {
    printf ("No meta data found in movie.\n"); 
  } else {
    while (pos != endpos) {
      printf("%s\n", pos->toString().c_str()); 
      ++pos;
    }
  }
  delete sm;
  
}
