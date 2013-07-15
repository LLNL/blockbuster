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
#include <fstream>
typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;


using namespace std; 

// =======================================================================
void errexit(TCLAP::CmdLine &cmd, string msg) {
  cerr << endl << "*** ERROR *** : " << msg  << endl<< endl;
  cmd.getOutput()->usage(cmd); 
  cerr << endl << "*** ERROR *** : " << msg << endl << endl;
  exit(1); 
}

// =====================================================================
int main(int argc, char *argv[]) {
 
  TCLAP::CmdLine  cmd(str(boost::format("%1% sets and changes tags in movies.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", false, "movie name(s)", cmd); 
 
  TCLAP::SwitchArg quiet("q", "quiet", "wDo not echo the tags to stdout.  Just return 0 on successful match. ", cmd); 

  TCLAP::SwitchArg report("r", "report", "After all operations are complete, list all the tags in the file.", cmd); 
  
  TCLAP::SwitchArg canonical("C", "canonical", "Enter the canonical metadata for a movie interactively.  If no movie name is given, simply list all canonical metadata with default values.", cmd); 

  TCLAP::SwitchArg deleteMD("D", "delete-metadata", "Delete all meta data in the file before applying any other tags.  If given alone, then the file will have no metadata when finished.", cmd); 
  
  TCLAP::SwitchArg exportTagfile("E", "export-tagfile", "Instead of or in addition to applying tags to a movie, create a tag file from the current session which can be read with -f to feed another smtag session.  The tagfile name: moviename is used ending with '.tagfile' instead of '.sm'",  cmd); 
  
  TCLAP::ValueArg<string> tagfile("F", "tagfile", "a file containing name:value pairs to be set", false, "", "filename", cmd); 
  
  TCLAP::SwitchArg interactive("I", "interactive", "Enter the metadata for a movie interactively.", cmd); 
  
  TCLAP::ValueArg<string> lorenzFileName("L", "lorenz-file", "Export a single JSON file, suitable for Lorenz import, containing tags for all movies.", false, "", "filename", cmd); 

  TCLAP::ValueArg<int> thumbnail("N", "thumbnail", "set frame number of thumbnail", false, -1, "frameNum", cmd); 

  TCLAP::ValueArg<int> thumbres("R", "thumbres", "the X resolution of the thumbnail (Y res will be autoscaled based on X res)", false, 0, "numpixels", cmd); 

  TCLAP::MultiArg<string> taglist("T", "tag", "a name:value[:type] for a tag being set or added.  'type' can be 'ASCII', 'DOUBLE', or 'INT64' and defaults to 'ASCII'.", false, "tagname:value[:type]", cmd); 

  TCLAP::ValueArg<string> delimiter("", "delimiter", "Sets the delimiter for all -T arguments.",false, ":", "string", cmd); 

  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 
  
  //------------------------------------------------------------
  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
	std::cout << e.what() << std::endl;
	return 1;
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
  
  ofstream lorenzFile;
  if (lorenzFileName.getValue() != "") {
    lorenzFile.open(lorenzFileName.getValue().c_str()); 
    if (!lorenzFile) {
      errexit(cmd, str(boost::format("Error:  could not open lorenz file %s for writing") % lorenzFileName.getValue())); 
    }
  }
  // Now, initialize canonical tags if the user wants to use that interface.
  // This is a separate map because it can change from movie to movie. 
  if (!movienames.getValue().size()) {
    bool didSomething = false; 
    if (canonical.getValue()) {
      didSomething = true; 
      // this needs to be here to make sure the -t flags can override the canonical flags
      if (exportTagfile.getValue()) {
        SM_MetaData::GetCanonicalMetaDataValuesFromUser(tagmap, false, true);
      }
      else {
        SM_MetaData::MergeMetaData(tagmap, SM_MetaData::CanonicalMetaDataAsMap(false)); 
        cout << SM_MetaData::MetaDataSummary(tagmap, false)<< endl; 
      }
    }
    // ------------------------------------------------------------------------------------
    
    if (report.getValue()) {      
      didSomething = true; 
      cout << SM_MetaData::MetaDataSummary(tagmap) << endl; 
    } // if (report.getValue())
  
    if (exportTagfile.getValue()) {
      didSomething = true; 
      string filename = "tags.tagfile"; 
      ofstream tagfile(filename.c_str()); 
      if (!tagfile) {
        errexit(cmd, str(boost::format("Error:  could not open tag file %s")% filename)); 
      }
      SM_MetaData::WriteMetaDataToStream(tagfile, tagmap);
      if (!quiet.getValue()) {
        cout << "Wrote movie meta data tag file " << filename << endl; 
      }
    }
    if (lorenzFile.is_open()) {
      didSomething = true; 
      lorenzFile << "[\n"; 
      SM_MetaData::WriteMetaDataToStream(lorenzFile, tagmap);
      lorenzFile << "]\n"; 
    }
    if (!didSomething) {
      cmd.getOutput()->usage(cmd); 
      exit(0); 
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
    dbprintf(5, "Before setting metadata, there are %d metadata items\n", sm->mMetaData.size()); 
    if (deleteMD.getValue()) {
      sm->DeleteMetaData(); 
      cout << "Deleted metadata from " << moviename << endl; 
    }
    
    smdbprintf(5, "Adding tagmap to movie %s\n", moviename.c_str()); 
    sm->SetMetaData(tagmap); 
    TagMap moviedata = sm->GetMetaData(); 

    //--------------------------------------------------
    if (canonical.getValue()) {      
      if (moviedata["Title"].ValueAsString() == "") {
        moviedata["Title"] =  SM_MetaData("Title", moviename); 
      }
      SM_MetaData::GetCanonicalMetaDataValuesFromUser(moviedata, true, true);
      
      sm->SetMetaData(moviedata); 
    }
    
    //--------------------------------------------------
    //smdbprintf(5, "Adding tagmap to movie %s\n", moviename.c_str()); 
    //sm->SetMetaData(tagmap); 
    
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
      cout << SM_MetaData::MetaDataSummary(sm->GetMetaData()) << endl; 
    }    
    if (exportTagfile.getValue()) {
      TagMap moviedata = sm->GetMetaData(); 
      string filename = sm->getName(); 
      boost::replace_last(filename, ".sm", ".tagfile"); 
      if (filename == sm->getName()) {
        filename = filename + ".tagfile"; 
      }
      ofstream tagfile(filename.c_str()); 
      if (!tagfile) {
        errexit(cmd, str(boost::format("Error:  could not open tag file %s for movie %s")% filename %  sm->getName())); 
      }
      SM_MetaData::WriteMetaDataToStream(tagfile, moviedata);
      if (quiet.getValue()) {
        continue; 
      } else {
        cout << "Wrote movie meta data tag file " << filename << endl; 
      }
    }
    if (lorenzFile.is_open()) {
      TagMap moviedata = sm->GetMetaData(); 
      if (fileno) {
        lorenzFile << ",\n"; 
      } else {
        lorenzFile << "[\n"; 
      }
      SM_MetaData::WriteMetaDataToStream(lorenzFile, moviedata);
      if (fileno ==movienames.getValue().size()-1) {
        lorenzFile << "]\n"; 
      }
    }
    
    sm->WriteMetaData(); 
    sm->closeFile(); 
    dbprintf(1, str(boost::format("All flags applied for movie %1%\n") % moviename).c_str()); 
  } // end loop over movienames
  // } // end else (if doing movies instead of tagfile export)
  
  
  return 0;
}
