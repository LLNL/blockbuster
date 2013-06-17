/*
** $RCSfile: img2sm.C,v $
** $Name:  $
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
**  http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
**  or man llnl_copyright
**
** $Id: img2sm.C,v 1.3 2008/07/08 02:43:20 dbremer Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/
#include <tclap_utils.h>
#include <tclap/Arg.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "version.h"
#include "sm/sm.h"
#include "debugutil.h"
#include "tags.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <map>
typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;


using namespace std; 

#define APPLY_ALL_TAG "Apply To All Movies [y/n]?"
#define USE_TEMPLATE_TAG "Use As Template [y/n]?"

// =====================================================================
void  GetTagsFromFile(string tagfile, map<string,string> &tagvec){ 
  dbprintf(0, "Tagfiles are not yet supported. :-( \n"); 
  exit(1); 
  return; 
}

// =====================================================================
string ResponseSummary(vector<string> &tags, vector<string> &values) {
  string summary = "SUMMARY OF RESPONSES\n";
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
  cout << ResponseSummary(tags, values) << endl; 

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
      cout << ResponseSummary(tags, values) << endl; 
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

// =====================================================================
void usage(TCLAP::CmdLine &cmd, char *argv0, string msg="") {
  cmd.reset(); 
  int argc = 2; 
  char *argv[3] = {argv0, (char*)"-h", NULL}; 
  cerr << endl << msg << endl; 
  cmd.parse(argc, argv);
  return; 
}

// =====================================================================
int main(int argc, char *argv[]) {
 
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.  You must supply at least one of -c, -d, -f, -R, -t, or -n, and at least one of -e or a list of filenames")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", false, "movie name(s)", cmd); 
 
  TCLAP::SwitchArg canonical("c", "canonical", "Enter the canonical metadata for a movie interactively.", cmd); 

  TCLAP::SwitchArg deleteMD("d", "delete-metadata", "Delete all meta data in the file before applying any other tags.  If given alone, then the file will have no metadata when finished.", cmd); 
  
  TCLAP::ValueArg<string> exportTagfile("e", "export-tagfile", "Instead of applying tags to a movie, create a tag file from the current session which can be read with -f to start another smtag session.", false, "", "filename", cmd); 
  
  TCLAP::ValueArg<string> tagfile("f", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::MultiArg<string> taglist("t", "tag", "a name:value pair for a tag being set or added.", false, "tagname:value (value is a string, and may contain spaces)", cmd); 

  TCLAP::ValueArg<int> thumbnail("n", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd); 

  TCLAP::ValueArg<int> thumbres("r", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 

  TCLAP::SwitchArg report("R", "report", "After all operations are complete, list all the tags in the file.", cmd); 
  
  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 

  //------------------------------------------------------------
  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
	return 1;
  }
  if (!movienames.getValue().size() && exportTagfile.getValue() == "" && !report.getValue()) {
    string msg = string("*****************************************************************'\n") + 
      "ERROR: You must supply at least one of -e, -R, or a list of filenames.\n" + 
      "*****************************************************************\n";
    usage(cmd, argv[0], msg); 
    exit(1); 
  }
  if (!canonical.getValue() && !deleteMD.getValue() && tagfile.getValue() == "" && !taglist.getValue().size() && thumbnail.getValue() == -1) {
    string msg = string("*****************************************************************\n") +
      "ERROR: You must supply at least one of -c, -d, -f, -R, -t, or -n.\n" +
      "*****************************************************************\n"; 
    usage(cmd, argv[0], msg); 
    exit(1); 
  }
    
  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 
  
  map<string, string> tagvec; 
  map<string,string> canonicalTags; 
  
  // First, if there is a file, populate the vector from that.
  if (tagfile.getValue() != "") {
    GetTagsFromFile(tagfile.getValue(), tagvec); 
  }
  // Now, add any canonical tags if the user wants to use that interface
  if (canonical.getValue()) {
    // this needs to be here to make sure the -t flags can override the canonical flags
    GetCanonicalTagValuesFromUser(canonicalTags);   
  }
  // Finally, set any tags explicitly from the command line. 
  if (taglist.getValue().size()) {
    for (uint tagnum = 0; tagnum < taglist.getValue().size(); tagnum++) {
      string arg = taglist.getValue()[tagnum]; 
      boost::char_separator<char> sep(":"); 
      tokenizer t(arg, sep);
      tokenizer::iterator pos = t.begin(), endpos = t.end(); 
      vector<string> tokens; 
      
      while (tokens.size() < 2) {
        if (pos == endpos) {
          cerr << "Error in tag format:  must be a tag:value pair, separated by a colon." << endl; 
          exit(1); 
        }
        tokens.push_back(*pos); 
        ++pos; 
      } 
      dbprintf(2, str(boost::format("Adding tag %1% and value %2% to list of tags to apply\n") % tokens[0] % tokens[1]).c_str()); 
      tagvec[tokens[0]] = tokens[1]; 
    }
  }

  // ------------------------------------------------------------------------------------
  if (exportTagfile.getValue() != "") {
    using boost::property_tree::ptree; 
    ptree pt;
    for (map<string,string>::iterator pos = tagvec.begin(); 
         pos != tagvec.end(); ++pos) {
      pt.put(pos->first, pos->second); 
    }    
    write_json(exportTagfile.getValue(), pt); 
  }
  else {
    for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {  
      string moviename = movienames.getValue()[0]; 
      dbprintf(1, str(boost::format("Opening movie file %1%\n")% moviename).c_str()); 
      smBase *sm = smBase::openFile(moviename.c_str(), 1);
      if (!sm) {
        dbprintf(0,"smtag: Error: Unable to open the file: %s\n", moviename.c_str());
        continue;
      }
      dbprintf(5, "Before setting metadata, there are %d metadata items\n", sm->mMetaData.size()); 
      if (deleteMD.getValue()) {
        sm->DeleteMetaData(); 
        cout << "Deleted metadata from " << moviename << endl; 
      }
      
      //--------------------------------------------------
      if (canonical.getValue()) {
        if (canonicalTags[APPLY_ALL_TAG] != "yes") {
          GetCanonicalTagValuesFromUser(canonicalTags);   
        }
        for (map<string,string>::iterator pos = canonicalTags.begin(); 
             pos != canonicalTags.end(); pos++) {
          // in this case, overwrite the tagvec with any new values from the user: 
          tagvec[pos->first] = canonicalTags[pos->first]; 
        }
      }
      
      //--------------------------------------------------
      if (tagvec.size()) {
        map<string,string>::iterator pos = tagvec.begin(), endpos = tagvec.end();
        while (pos != endpos) {        
          if (pos->first != APPLY_ALL_TAG && 
              pos->first != USE_TEMPLATE_TAG) {
            dbprintf(2, str(boost::format("Applying tag %1% and value %2%\n") % pos->first % pos->second).c_str()); 
            sm->SetMetaData(pos->first, pos->second);         
          }
        }
      } // end loop over tagvec
      
      //--------------------------------------------------
      if (thumbnail.getValue() != -1)  {
        int64_t f = thumbnail.getValue(); 
        dbprintf(1, str(boost::format("Setting thumbnail frame to %1%, FWIW.\n")%f).c_str()); 
        sm->SetMetaData("SM__thumbframe", f); 
        if (thumbres.getValue() != -1) {
          int64_t r = thumbres.getValue(); 
          dbprintf(1, str(boost::format("Setting thumbnail resolution to %1%, FWIW.\n")%r).c_str()); 
          sm->SetMetaData("SM__thumbres", r); 
        }        
      }
      
      //--------------------------------------------------
      dbprintf(5, "After setting metadata, there are %d metadata items\n", sm->mMetaData.size()); 
      if (report.getValue()) {
        cout << "After setting metadata, there are " << sm->mMetaData.size() << " metadata items in movie." << endl; 
        for (uint i = 0; i < sm->mMetaData.size(); i++) {
          cout << str(boost::format("Item %1%: tag = \"%2%\", type = \"%3%\", value = \"%4%\"\n")
                      % (i+1)
                      % sm->mMetaData[i].mTag 
                      % sm->mMetaData[i].TypeAsString() 
                      % sm->mMetaData[i].ValueAsString()); 
        }
      }

      // ----------------------------------------------
      if (report.getValue()) {
        vector<string> tags, values; 
        for (map<string,string>::iterator pos = tagvec.begin(); 
             pos != tagvec.end(); ++pos) {
          if (pos->first != APPLY_ALL_TAG && 
              pos->first != USE_TEMPLATE_TAG) {
            tags.push_back(pos->first); 
            values.push_back(pos->second); 
          }
        }
        cout << ResponseSummary(tags, values) << endl; 
      }    
      sm->WriteMetaData(); 
      sm->closeFile(); 
      dbprintf(1, str(boost::format("All flags applied for movie %1%\n") % moviename).c_str()); 
    } // end loop over movienames
  } // end else (if doing movies instead of tagfile export)
  
  if (!movienames.getValue().size() && report.getValue()) {
    if (canonical.getValue()) {
      for (map<string,string>::iterator pos = canonicalTags.begin(); 
           pos != canonicalTags.end(); pos++) {
        if (tagvec[pos->first] == "")
          tagvec[pos->first] = canonicalTags[pos->first]; 
      }
    }    
    vector<string> tags, values; 
    for (map<string,string>::iterator pos = tagvec.begin(); 
         pos != tagvec.end(); ++pos) {
      if (pos->first != APPLY_ALL_TAG && 
          pos->first != USE_TEMPLATE_TAG) {
        tags.push_back(pos->first); 
        values.push_back(pos->second); 
      }
    }
    cout << ResponseSummary(tags, values) << endl; 
  }
  
  return 0;
}
