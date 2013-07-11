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

  TCLAP::SwitchArg canonical("c", "canonical", "List all canonical tags for each movie", cmd); 

  TCLAP::SwitchArg filenameOnly("f", "only-filename", "Only print the filename of the matching movie(s).", cmd); 

  TCLAP::SwitchArg getinfoFlag("i", "movie-info", "Get non-metadata info for movie, such as compression type, number of frames, etc.", cmd); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", true, "movie name(s)", cmd); 

  TCLAP::SwitchArg matchAllFlag("A", "match-all", "Same as -T '.*' -V '.*', matches everything everywhere.", cmd); 

  TCLAP::SwitchArg list("l", "list", "Lists all tags in movie(s) with their values.  Equivalent to -T '.*' -s.  This is the default behavior", cmd); 

  TCLAP::MultiArg<string> tagPatternStrings("T", "Tag", "Regexp pattern to match the tag name being queried", false, "regexp", cmd); 

  TCLAP::MultiArg<string> valuePatternStrings("V", "Value", "Regexp pattern to match the name the tag being queried", false, "regexp", cmd); 

  TCLAP::SwitchArg thumbnailInfo("n", "thumbnail-info", "get number of thumbnail and resolution", cmd); 

  TCLAP::SwitchArg singleLineFlag("s", "summary", "Summarize: For each movie, report each match as tag, type and value on a line together.", cmd); 

  TCLAP::SwitchArg extractThumb("e", "extract-thumbnail", "extract thumbnail frame (not working yet)", cmd); 

  TCLAP::ValueArg<string> exportTagfile("E", "export-tagfile", "Extract a tag file from the movie which can be read with smtag.", false, "", "filename", cmd); 
  TCLAP::SwitchArg quietExport("q", "quiet-export", "when exporting a tagfile, do not echo the tags to stdout", cmd); 


  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 


  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
    usage(cmd, argv[0]); 
	exit(1);
  }
  
  bool matchAll = matchAllFlag.getValue(), singleLine = singleLineFlag.getValue(); 

  if (!canonical.getValue() && !thumbnailInfo.getValue() && !extractThumb.getValue() && !tagPatternStrings.getValue().size() && !valuePatternStrings.getValue().size() && !matchAll) {
    matchAll = true; 
    singleLine = true; 
  }

  // handle "sminfo" and --info persona 
  bool getinfo = false; 
  if (strstr(argv[0],"sminfo") || getinfoFlag.getValue()) {
    getinfo = true;     
  }   

  vector<boost::regex> tagPatterns, valuePatterns; 
  TagMap canonicalTags;
  if (matchAll) {
    tagPatterns.push_back(boost::regex(".*")); 
    valuePatterns.push_back(boost::regex(".*")); 
  } 
  else if (!canonical.getValue()) {
    vector<string> patternStrings = tagPatternStrings.getValue(); 
    for (uint patno = 0; patno < patternStrings.size(); patno++) {
      tagPatterns.push_back(boost::regex(patternStrings[patno])); 
    }
    patternStrings = valuePatternStrings.getValue(); 
    for (uint patno = 0; patno < patternStrings.size(); patno++) {
      valuePatterns.push_back(boost::regex(patternStrings[patno])); 
    }
  }  

  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 

  for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {
    if (canonical.getValue()) {
      canonicalTags = SM_MetaData::CanonicalMetaDataAsMap(false); 
    }
    string filename = movienames.getValue()[fileno]; 
    smBase *sm = smBase::openFile(filename.c_str(), 1);
    if (!sm) {
      dbprintf(0, "\n***********************************************\n"); 
      dbprintf(0, "ERROR: could not open file %s.  Did you make a mistake in your arguments?\n", filename.c_str()); 
      dbprintf(0, "***********************************************\n"); 
      usage(cmd, argv[0]); 
      exit(1); 
    }

    // Movie info case... (both sminfo and sm2img file)
    if (getinfo) {  
      smdbprintf(0, (sm->InfoString(verbosity.getValue())+"\n").c_str()); 
    }
    if (exportTagfile.getValue() != "") {
      TagMap moviedata = sm->GetMetaData(); 
      SM_MetaData::WriteMetaDataToFile(exportTagfile.getValue(), moviedata);
      if (quietExport.getValue()) {
        continue; 
      }
    }
      
    dbprintf(3, "Metadata for %s: (%d entries)\n", filename.c_str(), sm->mMetaData.size()); 
    int32_t thumbnum = -1, thumbres = -1;
    int numMatches = 0; 
     // for long list format:
    vector<string> tagMatches, valueMatches, valueTypes, matchTypes;
    uint longestTagMatch = 0, longestValueType = 5; 
    for (map <string,SM_MetaData>::iterator pos = sm->mMetaData.begin();
         pos != sm->mMetaData.end(); pos++) {
      string mdtag = pos->first, mdvalue = pos->second.ValueAsString(), 
        mdtype = pos->second.TypeAsString(); 
      bool tagmatch = MatchesAPattern(tagPatterns, mdtag), 
        valuematch = MatchesAPattern(valuePatterns, mdvalue);

      if (canonicalTags.find(mdtag) != canonicalTags.end()) {
        canonicalTags[mdtag].Set(mdtag,mdtype,mdvalue);                   
      }

      if (tagmatch || valuematch) numMatches ++; 

      if (filenameOnly.getValue()) {
        if (tagmatch || valuematch) {
          cout << filename << endl; 
          break; 
        }
      }
      else {
        if (singleLine && (tagmatch || valuematch)) {
          string matchtype; 
          if (matchAll) {
            matchtype = str(boost::format("Got Item: ")); 
          } else if (tagmatch && valuematch) {
            matchtype = "Both Match"; 
          } else if (tagmatch) {
            matchtype = "Tag Match";
          } else if (valuematch) {
            matchtype = "Value Match";
          }
           if (mdtype.size() > longestValueType) 
            longestValueType = mdtype.size(); 
          if (mdtag.size() > longestTagMatch) 
            longestTagMatch = mdtag.size(); 
          tagMatches.push_back(str(boost::format("\"%s\"")%mdtag)); 
          valueMatches.push_back(mdvalue); 
          valueTypes.push_back(mdtype); 
          matchTypes.push_back(matchtype); 
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
      if (thumbnailInfo.getValue()) {
        if (mdtag == "SM__thumbframe") {
          thumbnum = pos->second.mInt64; 
          dbprintf(5, "Found thumbnail frame %d\n", thumbnum); 
        }
        else if (mdtag == "SM__thumbres") {
          thumbres = pos->second.mInt64; 
          dbprintf(5, "Found thumbnail res %d\n", thumbres); 
        }
      }
    }
    if (thumbnailInfo.getValue()) {
      string res = "UNSET", frame = "UNSET"; 
      if (thumbnum != -1) frame = str(boost::format("%1%")%thumbnum); 
      if (thumbres != -1) res = str(boost::format("%1%")%thumbres); 
      cout << str(boost::format("%1%: thumbnail frame: %2%, res: %3%\n") % filename % frame % res) << endl; 
    }
    if (canonical.getValue()) {
      dbprintf(0, "Canonical tags for movie %s:\n", filename.c_str()); 
      vector<SM_MetaData> cmdata = SM_MetaData::CanonicalMetaData(false);
      for (uint i = 0; i<cmdata.size()-1; i++) {
        SM_MetaData * smdp = &canonicalTags[cmdata[i].mTag];
        dbprintf(0, str(boost::format("%1%: (%2%) %3%:\n") % (smdp->mTag) % (smdp->TypeAsString()) % (smdp->ValueAsString())).c_str());
      }
    } 
    if (singleLine || getinfo) {
      if (!getinfo) {
        dbprintf(0, "Matched tags for movie %s:\n", filename.c_str()); 
      } else {
        dbprintf(0, "Tags --------------------------------------\n", filename.c_str()); 
      }
      for (uint i=0; i< tagMatches.size(); i++) {
        SM_MetaData md(tagMatches[i], valueTypes[i], valueMatches[i]); 
        cout << md.toShortString(matchTypes[i], longestValueType, longestTagMatch+2) << endl;
      }      
    }
    dbprintf(1, str(boost::format("Finished with movie %1%") % filename).c_str()); 
    delete sm;
  }
  
}
