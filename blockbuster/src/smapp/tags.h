#ifndef SM_TAGS_H
#define SM_TAGS_H 1
#include <vector> 
#include <string> 
/*! 
	The idea of canonical tags is used by smquery and smtag to 
    give a common context for movies on LC machines
*/ 
static vector<string> GetCanonicalTagList(void) {
  vector<string> tags; 
  tags.push_back("Title"); 
  tags.push_back("Authors"); 
  tags.push_back("Description"); 
  tags.push_back("Science Type"); 
  tags.push_back("UCRL"); 
  tags.push_back("Code Name"); 
  tags.push_back("Sim Start Time"); 
  tags.push_back("Sim Duration"); 
  tags.push_back("Sim CPUs"); 
  tags.push_back("Sim Cluster"); 
  tags.push_back("Keywords"); 
  tags.push_back("Movie Creator"); 
  tags.push_back("Movie Create Date"); 
  tags.push_back("Movie Create Host"); 
  tags.push_back("Movie Create Command"); 
  return tags; 
}

#endif

