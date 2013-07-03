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
#include <map> 
typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;


using namespace std; 

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
 
  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.", cmd); 

  TCLAP::SwitchArg deleteMD("D", "delete-metadata", "Delete all meta data in the file before applying any other tags.  If given alone, then the file will have no metadata when finished.", cmd); 
  
  TCLAP::ValueArg<string> exportTagfile("E", "export-tagfile", "Instead of applying tags to a movie, create a tag file from the current session which can be read with -f to start another smtag session.", false, "", "filename", cmd); 
  
  TCLAP::ValueArg<string> tagfile("F", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd); 

  TCLAP::ValueArg<string> delimiter("", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd); 

  TCLAP::ValueArg<int> thumbnail("N", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd); 

  TCLAP::ValueArg<int> thumbres("R", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 

  TCLAP::SwitchArg report("L", "list", "After all operations are complete, list all the tags in the file.", cmd); 
  
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

  SM_MetaData::SetDelimiter(delimiter.getValue()); 

  TagMap tagmap; 
  TagMap canonicalTags; 
  
  // First, if there is a file, populate the vector from that.
  if (tagfile.getValue() != "") {
    SM_MetaData::GetMetaDataFromFile(tagfile.getValue(), tagmap); 
  }

  // Next, override with any tags explicitly from the command line. 
  if (taglist.getValue().size()) {
    for (uint tagnum = 0; tagnum < taglist.getValue().size(); tagnum++) {
      SM_MetaData md; 
      md.SetFromDelimitedString(taglist.getValue()[tagnum]);
      tagmap[md.mTag] = md; 
    }
  }

  // Now, initialize canonical tags if the user wants to use that interface.
  // This is a separate map because it can change from movie to movie.  
  if (canonical.getValue()) {
    // this needs to be here to make sure the -t flags can override the canonical flags
    SM_MetaData::GetCanonicalMetaDataValuesFromUser(canonicalTags);   
  }
  // ------------------------------------------------------------------------------------
  if (exportTagfile.getValue() != "") {
    SM_MetaData::WriteMetaDataToFile(exportTagfile.getValue(), tagmap);

    if (report.getValue()) {
      for (TagMap::iterator pos = canonicalTags.begin(); 
           pos != canonicalTags.end(); pos++) {
        if (tagmap.find(pos->first) == tagmap.end())
          tagmap[pos->first] = canonicalTags[pos->first]; 
      }
      
     cout << SM_MetaData::MetaDataSummary(tagmap) << endl; 
    } // if (report.getValue())
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
        if (canonicalTags[APPLY_ALL_TAG].ValueAsString() != "yes") {
          SM_MetaData::GetCanonicalMetaDataValuesFromUser(canonicalTags);   
        }
        sm->SetMetaData(canonicalTags); 
     }
      
      //--------------------------------------------------
      smdbprintf(5, "Adding tagmap to movie %s\n", moviename.c_str()); 
      sm->SetMetaData(tagmap); 
      
      //--------------------------------------------------
      if (thumbnail.getValue() != -1)  {
        sm->SetThumbnailFrame(thumbnail.getValue()); 
        if (thumbres.getValue() != -1) {
          sm->SetThumbnailRes(thumbres.getValue()); 
        }
      }
      
      //--------------------------------------------------
      dbprintf(5, "After setting metadata, there are %d metadata items\n", sm->mMetaData.size()); 
      
      // ----------------------------------------------
      if (report.getValue()) {        
        cout << SM_MetaData::MetaDataSummary(tagmap) << endl; 
      }    
      sm->WriteMetaData(); 
      sm->closeFile(); 
      dbprintf(1, str(boost::format("All flags applied for movie %1%\n") % moviename).c_str()); 
    } // end loop over movienames
  } // end else (if doing movies instead of tagfile export)
  
  
  return 0;
}
