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

int main(int argc, char *argv[]) {
 
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::SwitchArg canonical("c", "canonical", "Enter the canonical metadata for a movie interactively." , cmd); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", true, "movie name(s)", cmd); 

  TCLAP::ValueArg<string> tagfile("f", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::MultiArg<string> taglist("t", "tag", "a name:value pair for a tag being set", false, "tagname:value (value may contain spaces)", cmd); 

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


  smBase *sm = smBase::openFile(movienames.getValue()[0].c_str(), 1);
  if (!sm) {
    fprintf(stderr,"Unable to open the file: %s\n",movienames.getValue()[0].c_str());
    exit(1);
  }
 
  if (canonical.getValue()) {
    cerr << "canonical tag not implemented yet." << endl; 
    exit(0); 
  }

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
      dbprintf(1, str(boost::format("Got tag %1% and value %2%\n") % tokens[0] % tokens[1]).c_str()); 
      sm->SetMetaData(tokens[0], tokens[1]); 
      
    }
    sm->WriteMetaData(); 
    sm->closeFile(); 
  }
     
  return 0;
}
