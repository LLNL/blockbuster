#include "tags.h"
#include "debugutil.h"
#include <stdio.h>
#include <map>
#include "version.h"
#include "sm/sm.h"
#include "debugutil.h"
#include "tags.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;

using namespace std; 
// =====================================================================
vector<string> GetCanonicalTagList(void) {
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

// =====================================================================
void  GetTagsFromFile(string tagfile, map<string,string> &tagvec){ 
  dbprintf(0, "Tagfiles are not yet supported. :-( \n"); 
  exit(1); 
  return; 
}

// =====================================================================
void WriteTagsToFile(string filename, map<string, string> &tagvec) {
  using boost::property_tree::ptree; 
  ptree pt;
  for (map<string,string>::iterator pos = tagvec.begin(); 
       pos != tagvec.end(); ++pos) {
    pt.put(pos->first, pos->second); 
  }    
  write_json(filename, pt); 
  return; 
}

// =====================================================================
string TagSummary(map<string,string> &tagvec) {
  vector<string> tags, values; 
  for (map<string,string>::iterator pos = tagvec.begin(); 
       pos != tagvec.end(); ++pos) {
    if (pos->first != APPLY_ALL_TAG && 
        pos->first != USE_TEMPLATE_TAG) {
      tags.push_back(pos->first); 
      values.push_back(pos->second); 
    }
  }
  return TagSummary(tags,values); 
}
// =====================================================================
string TagSummary(vector<string> &tags, vector<string> &values) {
  string summary = "TAG SUMMARY\n";
  for (uint num = 0; num < tags.size(); num++) {
    string value = values[num]; 
    if (value == "") {
      value = "(no response yet)"; 
    }
    summary += str(boost::format("%1$2d) %2$-33s: current value = \"%3%\"\n") % num % tags[num] % values[num]); 
  }
  return summary;   
}

// =====================================================================
void GetCanonicalTagValuesFromUser(map<string,string> &canonicals) {
  vector<string> tags = GetCanonicalTagList(), values; 
  tags.push_back(APPLY_ALL_TAG); 
  tags.push_back(USE_TEMPLATE_TAG); 
  values.resize(tags.size()); 
  values[tags.size()-2] = "no"; 
  values[tags.size()-1] = "no"; 

  // synchronize tags/values and canonicals to start up
  if (!canonicals.size() || canonicals[USE_TEMPLATE_TAG] == "no") {
    for (uint i = 0; i<tags.size(); i++) {
       canonicals[tags[i]] = values[i]; 
    }
  } 
  for (uint i = 0; i<tags.size(); i++) {
    values[i] = canonicals[tags[i]]; 
  }
  

  cout << "You will now be asked to supply values for the " << tags.size() << " 'canonical' tags.  At any time, you can enter 'e' or 'exit' to stop the input for this movie without saving your values, 's' or 'save' to stop the input and save your changes, 'm' or 'menu' to be presented with a menu, a number to choose a different tag to enter." << endl;
  cout << TagSummary(tags, values) << endl; 

  string response; 
  int tagno = 0; 
  while (true) {
    if (response == "e" || response == "exit") {
      cout << "Exiting without saving changes." << endl; 
      return ; 
    }
    else if (response == "s" || response == "save" || tagno == tags.size()) {
      cout << "Exiting and saving." << endl; 
      for (uint i = 0; i<tags.size(); i++) {
        canonicals[tags[i]] = values[i]; 
      }
      return ; 
    } 
    else if (response == "m" || response == "map") {
      cout << TagSummary(tags, values) << endl; 
      response = readline("Please enter a key number from the list (-1 to just continue): ");       
      int rval = -1; 
      if (response != "" && response != "-1") {
        try {
          rval = boost::lexical_cast<short>(response);
          dbprintf(5, "Got good user response %d\n", rval); 
        }
        catch(boost::bad_lexical_cast &) {
          dbprintf(5, "Got bad user response\n"); 
          rval = -1; 
        }
        if (rval < 0 || rval >= tags.size()) {
          cout << "Invalid value. "; 
        } else {
          tagno = rval; 
        }
      }
    }
    else if (response != "") {
      if (boost::regex_match(response, boost::regex("[yY]e*s*"))) response = "yes"; 
      if (boost::regex_match(response, boost::regex("[Nn]o*"))) response = "no"; 
      values[tagno] = response; 
      tagno++; 
    } 
    else if (values[tagno] != "") {
      tagno++; 
    }
    response = readline(str(boost::format("Please enter a value for key %1% (default: \"%2%\"): ")%tags[tagno]%values[tagno]).c_str()); 
  }
  
  return; 
}

