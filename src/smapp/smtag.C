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
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "version.h"
#include "sm/smBase.h"
#include "debugutil.h"

typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;

using namespace std; 

void  GetTagsFromFile(string tagfile, vector<vector<string> > &tagvec){ 
  return; 
}

void GetCanonicalTagValuesFromUser(vector<vector<string> > &tagvec) {

  return; 
}
int main(int argc, char *argv[]) {
 
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::SwitchArg canonical("c", "canonical", "Enter the canonical metadata for a movie interactively." , cmd); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", true, "movie name(s)", cmd); 

  TCLAP::ValueArg<string> tagfile("f", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::MultiArg<string> taglist("t", "tag", "a name:value pair for a tag being set or added.", false, "tagname:value (value may contain spaces)", cmd); 

  TCLAP::ValueArg<int> thumbnail("n", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd); 

  TCLAP::ValueArg<int> thumbres("r", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 
  
  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 

  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
	return 1;
  }

  if (!canonical.getValue() && !taglist.getValue().size() && tagfile.getValue() == "" && thumbnail.getValue() == -1) {
    cerr << "ERROR: You must provide either the --tag (-t), --tagfile (-f), --thumbnail (-n), or --canonical (-c) option." << endl; 
    exit(1); 
  }

  smBase::init();
  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 

  
  vector<vector<string> >tagvec; 
  // First, if there is a file, populate the vector from that.
  if (tagfile.getValue() != "") {
    GetTagsFromFile(tagfile.getValue(), tagvec); 
  }
  // Now, add any canonical tags if the user wants to use that interface
  if (canonical.getValue()) {
    GetCanonicalTagValuesFromUser(tagvec);   
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
      tagvec.push_back(tokens); 
    }
  }

  
  for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {  
    string moviename = movienames.getValue()[0]; 
    dbprintf(1, str(boost::format("Opening movie file %1%\n")% moviename).c_str()); 
    smBase *sm = smBase::openFile(moviename.c_str(), 1);
    if (!sm) {
      dbprintf(0,"smtag: Error: Unable to open the file: %s\n", moviename.c_str());
      continue;
    }
    if (tagvec.size()) {
      for (uint tagnum =0; tagnum < tagvec.size(); tagnum++) {
        dbprintf(2, str(boost::format("Applying tag %1% and value %2%\n") % tagvec[tagnum][0] % tagvec[tagnum][1]).c_str()); 
        sm->SetMetaData(tagvec[tagnum][0], tagvec[tagnum][1]); 
        
      }
      dbprintf(2, str(boost::format("All tags applied for movie %1%\n") % moviename).c_str()); 
      sm->WriteMetaData(); 
      sm->closeFile(); 
    } // end loop over taglist
    else if (thumbnail.getValue() != -1)  {
      int64_t thumbframe =  thumbnail.getValue(); 
      dbprintf(1, str(boost::format("Setting thumbnail frame to %1%, FWIW.\n")%thumbnail.getValue()).c_str()); 
      sm->SetMetaData("thumbframe", thumbframe); 
    }
    dbprintf(1, "Done with movie %1%\n", moviename.c_str()); 
  } // end loop over movienames
     
  return 0;
}
