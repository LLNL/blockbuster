#ifndef SM_TAGS_H
#define SM_TAGS_H 1
#include <vector> 
#include <string> 
#include <map>
using std::map; 
using std::string;
using std::vector; 


#define APPLY_ALL_TAG "Apply To All Movies [y/n]?"
#define USE_TEMPLATE_TAG "Use As Template [y/n]?"

/*! 
	The idea of canonical tags is used by smquery and smtag to 
    give a common context for movies on LC machines
*/ 
vector<string> GetCanonicalTagList(void);
void GetCanonicalTagValuesFromUser(map<string,string> &canonicals);
void  GetTagsFromFile(string tagfile, map<string,string> &tagvec);
void  WriteTagsToFile(string tagfile, map<string,string> &tagvec);
string TagSummary(vector<string> &tags, vector<string> &values);

#endif

