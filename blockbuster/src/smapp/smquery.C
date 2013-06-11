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
bool MatchTag(const vector<boost::regex> &tagPatterns, string &tag) {
  for (uint patno=0; patno < tagPatterns.size(); patno++) {
    if (regex_match(tag, tagPatterns[patno])) return true; 
  }
  return false; 
}

//===================================================================
bool MatchValue(const vector<boost::regex> &valuePatterns, SM_MetaData &smdata) {

  if (!valuePatterns.size()) return false;

  string value; 
  if (smdata.mType == METADATA_TYPE_ASCII) {
    value = smdata.mAscii; 
  }
  else if (smdata.mType == METADATA_TYPE_INT64) {
    value = str(boost::format("%1%")%smdata.mInt64); 
  }
  else if (smdata.mType == METADATA_TYPE_DOUBLE) {
    value = str(boost::format("%1%")%smdata.mDouble); 
  }
  else {
    dbprintf(0, str(boost::format("MatchValue() error: unknown meta data type %1%\n")%smdata.mType).c_str()); 
    exit(1); 
  }

  for (uint patno=0; patno < valuePatterns.size(); patno++) {
    if (regex_match(value, valuePatterns[patno])) return true; 
  }
  return false; 
}

//===================================================================
int main(int argc, char *argv[]) {
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::SwitchArg printFilenameFlag("f", "print-filename", "print the names of files matching the patterns.  If this is given without -m, then only matching filenames are printed without the matched text.  Otherwise, the filename is prepended to each match text, e.g., filename: matchtext", cmd); 

  TCLAP::SwitchArg printLongFlag("l", "print-long", "same as -f -m -t. This is the default behavior", cmd); 

  TCLAP::SwitchArg printMatchFlag("m", "print-match", "print the names of found matches", cmd); 

  TCLAP::SwitchArg printMatchTypeFlag("t", "print-match-type", "Also prepend whether match was tag or value (useful if both were searched on).  Ignored if -m or -a is not given.", cmd); 

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
  bool printLong = printLongFlag.getValue(); 
  if (!printFilenameFlag.getValue() && !printMatchFlag.getValue()) {
    printLong = true; 
  }

  vector<boost::regex> tagPatterns, valuePatterns; 
  for (uint patno; patno < tagPatternStrings.getValue().size(); patno++) {
    tagPatterns.push_back(boost::regex(tagPatternStrings.getValue()[patno])); 
  }
  for (uint patno; patno < valuePatternStrings.getValue().size(); patno++) {
    tagPatterns.push_back(boost::regex(valuePatternStrings.getValue()[patno])); 
  }
  
  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 

  for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {
    string filename = movienames.getValue()[fileno]; 
    smBase *sm = smBase::openFile(filename.c_str(), 1);
    printf("Metadata: (%d entries)\n", sm->mMetaData.size()); 
    for (vector <SM_MetaData>::iterator pos = sm->mMetaData.begin();
         pos != sm->mMetaData.end(); pos++) {
      bool tagmatch = MatchTag(tagPatterns, pos->mTag), 
        valuematch = MatchValue(valuePatterns, *pos); 
      
      
      printf("%s\n", pos->toString().c_str()); 
      ++pos;
    }
    
    delete sm;
  }
  
}
