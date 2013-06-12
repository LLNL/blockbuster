#include <tclap_utils.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <sm/sm.h>
#include <vector>
#include "version.h"
#include "debugutil.h"

using namespace std; 

//===================================================================
void usage(TCLAP::CmdLine  &cmd, char *prog) {
  char *helpargs[3] = {prog, (char*)"--help", NULL}; 
  int helpargc = 2; 
  cmd.reset(); 
  cmd.parse(helpargc, helpargs); 
  return; 
}

//===================================================================
string MetaDataValueAsString(SM_MetaData &smdata) {
  string value ; 
  if (smdata.mType == METADATA_TYPE_ASCII) {
    return smdata.mAscii; 
  }
  else if (smdata.mType == METADATA_TYPE_INT64) {
    return str(boost::format("%1%")%smdata.mInt64); 
  }
  else if (smdata.mType == METADATA_TYPE_DOUBLE) {
    return str(boost::format("%1%")%smdata.mDouble); 
  }

  dbprintf(0, str(boost::format("MatchValue() error: unknown meta data type %1%\n")%smdata.mType).c_str()); 
  exit(1); 

  return value; 
}

//===================================================================
bool MatchesAPattern(const vector<boost::regex> &patterns, string &s) { 
  dbprintf(5, "MatchesAPattern: %d patterns to check\n", patterns.size()); 

  for (uint patno=0; patno < patterns.size(); patno++) {
    dbprintf(5, "MatchesAPattern: Comparing string \"%s\" to pattern \"%s\"\n",
             s.c_str(), patterns[patno].str().c_str()); 
    if (regex_match(s, patterns[patno])) {
      dbprintf(5, "\n *** Found match. *** \n"); 
      return true; 
    }
  }
  dbprintf(5, "Found no matches for string \"%s\".\n", s.c_str()); 
  return false; 
}


//===================================================================
int main(int argc, char *argv[]) {
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::SwitchArg filenameOnly("f", "only-filename", "Only print the filename of the matching movie(s).", cmd); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", true, "movie name(s)", cmd); 

  TCLAP::SwitchArg matchAll("A", "match-all", "Same as -T '.*' -V '.*', matches everything everywhere.", cmd); 

  TCLAP::MultiArg<string> tagPatternStrings("T", "Tag", "Regexp pattern to match the tag name being queried", false, "regexp", cmd); 

  TCLAP::MultiArg<string> valuePatternStrings("V", "Value", "Regexp pattern to match the name the tag being queried", false, "regexp", cmd); 

  TCLAP::SwitchArg thumbnailNum("n", "thumbnail-info", "get number of thumbnail and resolution", cmd); 

  TCLAP::SwitchArg extractThumb("e", "extract-thumbnail", "extract thumbnail frame", cmd); 

  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 


  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
	return 1;
  }
  
  if (!thumbnailNum.getValue() && !extractThumb.getValue() && !tagPatternStrings.getValue().size() && !valuePatternStrings.getValue().size()) {
    cerr << "ERROR: You must provide either the --Tag (-T), --Value (-V), --thumbnail-info (-n), or --extract-thumbnail (-e) option." << endl; 
    usage(cmd, argv[0]); 
    exit(1); 
  }

  vector<boost::regex> tagPatterns, valuePatterns; 
  vector<string> patternStrings = tagPatternStrings.getValue(); 
  for (uint patno = 0; patno < patternStrings.size(); patno++) {
    tagPatterns.push_back(boost::regex(patternStrings[patno])); 
  }
  patternStrings = valuePatternStrings.getValue(); 
  for (uint patno = 0; patno < patternStrings.size(); patno++) {
    valuePatterns.push_back(boost::regex(patternStrings[patno])); 
  }
  
  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 

  for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {
    string filename = movienames.getValue()[fileno]; 
    smBase *sm = smBase::openFile(filename.c_str(), 1);
    dbprintf(3, "Metadata for %s: (%d entries)\n", filename.c_str(), sm->mMetaData.size()); 
    for (vector <SM_MetaData>::iterator pos = sm->mMetaData.begin();
         pos != sm->mMetaData.end(); pos++) {
      string mdtag = pos->mTag, mdvalue = MetaDataValueAsString(*pos); 
      bool tagmatch = MatchesAPattern(tagPatterns, mdtag), 
        valuematch = MatchesAPattern(valuePatterns, mdvalue); 
      if (filenameOnly.getValue()) {

        if (tagmatch || valuematch) {
          cout << filename << endl; 
          continue; 
        }
      }
      else {
        if (tagmatch) {
          cout << str(boost::format("%1%: Tag Match: %2%") % filename % mdtag) << endl;
        }  
        if (valuematch) {
          cout << str(boost::format("%1%: Value Match: %2%") % filename % mdvalue) << endl;
        }
      }
    }
    dbprintf(1, str(boost::format("Finished checking %1%") % filename).c_str()); 
    delete sm;
  }
  
}
