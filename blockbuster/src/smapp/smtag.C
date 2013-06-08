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

using namespace std; 

int main(int argc, char *argv[]) {

  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  // If "set" is used, then the first unlabeled argument will be the tag value
  TCLAP::ValueArg<string> tag("t", "tag", "the name of a tag being set",true, 0, ""); 
  TCLAP::ValueArg<string> value("v", "value", "the value for the tag being set",false, 0, "", cmd); 
  
  TCLAP::ValueArg<int> thumbnail("n", "thumbnail", "set frame number of thumbnail",true, 0, "frameNum"); 
  TCLAP::ValueArg<int> xres("x", "xres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 
  
  TCLAP::SwitchArg canonical("c", "canonical", "Enter the canonical metadata for a movie interactively." , true, false); 

  cmd.xorAdd(tag,thumbnail); 
  
  
  
}
