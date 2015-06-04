#include <tclap_utils.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>
#include <sm/sm.h>
#include <vector>
#include <fstream>
#include "version.h"
#include "debugutil.h"
#include "../libpng/pngsimple.h"
#include <exiv2/exiv2.hpp>
#include <exiv2/image.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std; 

// =======================================================================
void errexit(TCLAP::CmdLine &cmd, string msg, bool usage=true) {
  cerr << endl << "*** ERROR *** : " << msg  << endl<< endl;
  if (usage) {
    cmd.getOutput()->usage(cmd); 
    cerr << endl << "*** ERROR *** : " << msg << endl << endl;
  }
  exit(1); 
}

//===================================================================
bool MatchesAPattern(const vector<boost::regex> &patterns, string &s, vector<bool> &matched) { 
smdbprintf(5, "MatchesAPattern: %d patterns to check\n", patterns.size()); 

  // check actual patterns in movie:
  for (uint patno=0; patno < patterns.size(); patno++) {
  smdbprintf(5, "MatchesAPattern: Comparing string \"%s\" to pattern \"%s\"\n",
             s.c_str(), patterns[patno].str().c_str()); 
    if (regex_search(s, patterns[patno])) {
    smdbprintf(5, "\n *** Found match. *** \n"); 
      matched[patno] = true; 
      return true; 
    }
  }
smdbprintf(5, "Found no matches for string \"%s\".\n", s.c_str()); 
  return false; 
}

void ListReservedTags(void) {
  cout << "The following tags are reserved and always match every movie: " << endl; 
  cout << "Name: Filename containing the movie." << endl; 
  cout << "Version: Give the streaming movie format version." << endl; 
  cout << "Format: give the movie compression format." << endl; 
  cout << "Xsize: X dimension of movie in pixels." << endl; 
  cout << "Ysize: Y dimension of movie in pixels." << endl; 
  cout << "Frames: number of movie frames." << endl; 
  cout << "Stereo: Tells whether the movie is stereo." << endl; 
  cout << "FPS: Movie frames per second." << endl; 
  cout << "Compression: movie compression as a RATIO (not %) of compressed to uncompressed (lower is smaller)." << endl; 
  cout << "LOD: Number of levels of detail (resolutions)." << endl; 
  cout << "Res n: Size and tile sizes for LOD n." << endl; 
  return; 
}


//===================================================================
void ExportPosterFrame(string imgname, smBase *sm) {

  // ----------------------------------------------------
  // Read the RGB data from the SM movie
  int64_t frameNum =  sm->getPosterFrame(); 
  int size[3] = {sm->getWidth(), sm->getHeight(), 3}; 
  unsigned long numbytes = size[0]*size[1]*size[2]; // twice as much as needed just for safety, and to fit any metadata that will go in. 
  unsigned long numbytes_requested = numbytes; 
  vector<unsigned char> source(numbytes*2, 0);
  sm->getFrame(frameNum, source.data(), 0, 0); 
  
  // -------------------------------------------------------------
  // Compress the buffer data into jpeg format for Exiv2 to use
  struct jpeg_compress_struct cinfo = {0};
  struct jpeg_error_mgr jerr;
  JSAMPROW row_ptr[1];
  int row_stride;
  
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  vector<unsigned char> dest(numbytes, 0);
  unsigned char *buffer = &dest[0]; 
  jpeg_mem_dest(&cinfo, &buffer, &numbytes);
  
  cinfo.image_width = sm->getWidth(); 
  cinfo.image_height = sm->getHeight(); 
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  
  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = sm->getWidth() * 3;
  
  while (cinfo.next_scanline < cinfo.image_height) {
    row_ptr[0] = &source[(cinfo.image_height - cinfo.next_scanline) * row_stride];
    jpeg_write_scanlines(&cinfo, row_ptr, 1);
  }
  
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  smdbprintf(2, str(boost::format("After compressing,  cinfo.dest->free_in_buffer = %1%, numbytes = %2%, numbytes_requested = %5%, with image size %3% x %4%\n ")% (int)(cinfo.dest->free_in_buffer) % numbytes %(sm->getWidth())% (sm->getHeight()) % numbytes_requested).c_str());  

  // ---------------------------------------------------------
  // Use Exiv2 library to add Exif metadata to the buffer
  string title = sm->getName(), authors="unknown", description="none", keywords="none", createDate="unknown", science="unknown"; 
  sm->GetMetaData("Title", title);
  sm->GetMetaData("Authors", authors); 
  sm->GetMetaData("Description", description); 
  sm->GetMetaData("Keywords", keywords); 
  sm->GetMetaData("Movie Create Date", createDate); 
  sm->GetMetaData("Science Type", science); 
    
  Exiv2::ExifData exifData;
  exifData["Exif.Image.XPTitle"] = title; 
  exifData["Exif.Image.XPAuthor"] = authors; 
  exifData["Exif.Photo.UserComment"] = description; 
  exifData["Exif.Image.XPKeywords"] = keywords; 
  exifData["Exif.Image.DateTimeOriginal"] = createDate; 
  exifData["Exif.Image.XPSubject"] = science; 
  
  uint32_t buffersize = numbytes_requested-numbytes; 

  bool inMemory = false; // in-core does not work yet, keep this to false

  if (inMemory) {
    // create an exiv2 image in memory using the jpeg buffer
    // For some reason this does not work yet.  
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(buffer, buffersize);
    image->setExifData(exifData); 
    image->writeMetadata(); 
    buffersize = image->io().size(); 
    cout << str(boost::format("image class has io size of %1%\n")%buffersize);
  
    // ---------------------------------------------------------
    // Write the buffer, now containing Exif metadata, to disk
    FILE * file = fopen(imgname.c_str(), "w"); 
    fwrite(buffer, buffersize, 1, file); 
    fclose(file); 
  } else {
    FILE * file = fopen(imgname.c_str(), "w"); 
    fwrite(buffer, buffersize, 1, file); 
    fclose(file); 

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(imgname);
    image->setExifData(exifData); 
    image->writeMetadata(); 
  }
  cout << "Wrote poster frame image " << imgname << endl; 
  return; 
}

//===================================================================
int main(int argc, char *argv[]) {
  TCLAP::CmdLine  cmd(str(boost::format("%1% queries tags in movies.  If only a single tag is queried, only the single value result will be reported without embellishment.")%argv[0]), ' ', BLOCKBUSTER_VERSION); 

  TCLAP::SwitchArg andFlag("A", "and", "Program only returns true (0) if all expressions match.  Normally, any matched expression causes a 0 return code.", cmd); 

  TCLAP::SwitchArg canonical("C", "canonical", "List all canonical tags for each movie.  If no movie name is given, simply list all canonical metadata with default values.", cmd); 

  TCLAP::ValueArg<string>  exportPosterFrame("e", "export-poster", "Export poster frame", false, "", "filename", cmd); 

  TCLAP::ValueArg<string> exportTagfile("E", "export-tagfile", "Extract a (JSON) tag file from each movie which can be read with smtag.  If the given filename is 'stdout' or '-', then output to stdout.", false, "", "filename", cmd); 

  TCLAP::SwitchArg filenameOnly("f", "only-filename", "Only print the filename of the matching movie(s).", cmd); 

  TCLAP::SwitchArg getinfoFlag("i", "info", "Get old-style \"sminfo\" metadata for movie, such as compression type, number of frames, etc.", cmd); 

  TCLAP::ValueArg<string> jsonFileNameFlag("J", "json-output", "Export a single JSON file, suitable for Lorenz import, containing tags for all movies.  If the given filename is 'stdout' or '-', then output to stdout.", false, "", "filename", cmd); 

  TCLAP::SwitchArg list("l", "list", "Lists all tags in movie(s) with their values.  Equivalent to -T '.*' -s -i.  This is the default behavior", cmd); 

  TCLAP::ValueArg<string> lorenzFileNameFlag("L", "lorenz-format", "Synonym for -J(q.v..", false, "", "filename", cmd); 

  TCLAP::SwitchArg matchAllFlag("", "match-all", "Same as -T '.*', matches all tags.", cmd); 

  TCLAP::SwitchArg posterInfo("n", "poster-number", "print frame number of poster", cmd); 

  TCLAP::SwitchArg prependFilenameFlag("p", "prepend-filename", "Print the filename of the matching movie(s) before each match.  Default: false, unless you are querying multiple movies.  See also --dont-prepend-filename", cmd); 

  TCLAP::SwitchArg dontPrependFilenameFlag("P", "dont-prepend-filename", "Never print the filename of the matching movie(s) before each match.", cmd); 

  TCLAP::SwitchArg quiet("q", "quiet", "Do not echo the tags to stdout.  Just return 0 on successful match. ", cmd); 

  TCLAP::ValueArg<int> verbosity("v", "verbosity", "set verbosity (0-5)", false, 0, "int", cmd); 

  TCLAP::SwitchArg reservedList("", "reserved-tag-list", "List all reserved tags and exit.", cmd); 

  TCLAP::MultiArg<string> tagPatternStrings("T", "Tag", "Regex pattern to match any substring of the tag name being queried.  Thus the pattern 'Duration' is the same as '.*Duration.*'  To match an exact string, use '^' and '$', i.e., \"^pattern$\".  See --reserved-tag-list", false, "regexp", cmd); 

  TCLAP::SwitchArg authors("", "authors", " Same as -T Authors", cmd); 

  TCLAP::SwitchArg codename("", "codename", "Same as -T Code Name", cmd); 

  TCLAP::SwitchArg description("", "description", "Same as -T Description", cmd); 

  TCLAP::SwitchArg keywords("", "keywords", "Same as -T Keywords, i.e., lists all keywords for this movie.  There is not yet a way to search for a single keyword, sorry.  Ask Rich Cook to implement this if it sounds useful.", cmd); 

  TCLAP::SwitchArg title("", "title", "Same as -T Title", cmd); 

  TCLAP::SwitchArg ucrl("", "ucrl", "Same as -T UCRL", cmd); 

  TCLAP::MultiArg<string> valuePatternStrings("V", "Value", "Regex pattern to match the value of any tags being queried", false, "regexp", cmd); 

  TCLAP::UnlabeledMultiArg<string> movienames("movienames", "movie name(s)", false, "movie name(s)", cmd); 


  try {
	cmd.parse(argc, argv);
  } catch(std::exception &e) {
    errexit(cmd, e.what()); 
  }

  if (reservedList.getValue()) {
    ListReservedTags(); 
    exit(0); 
  }
  
  // handle "sminfo" and --info persona 
  bool getinfo = false; 
  if (strstr(argv[0],"sminfo") || getinfoFlag.getValue()) {
    getinfo = true;     
  }   

  bool matchAll = matchAllFlag.getValue(), singleLine = true; //  = singleLineFlag.getValue(); 

  if (!canonical.getValue() && !posterInfo.getValue() && exportPosterFrame.getValue() == "" && !tagPatternStrings.getValue().size() && !valuePatternStrings.getValue().size() && !matchAll && !getinfo && !authors.getValue() && !codename.getValue() && !description.getValue() && !keywords.getValue() && !title.getValue() && !ucrl.getValue()) {
    matchAll = true; 
  }
  if (!movienames.getValue().size()) {
    if (canonical.getValue()) {
      cout << SM_MetaData::MetaDataSummary(SM_MetaData::CanonicalMetaDataAsMap(false), false)<< endl; 
    }
    else { 
      cmd.getOutput()->usage(cmd); 
    }
    exit (0); 
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
      if (authors.getValue()) tagPatterns.push_back(boost::regex("Authors"));
      if (codename.getValue()) tagPatterns.push_back(boost::regex("Code Name"));
      if (description.getValue()) tagPatterns.push_back(boost::regex("Description"));
      if (keywords.getValue()) tagPatterns.push_back(boost::regex("Keywords"));
      if (title.getValue()) tagPatterns.push_back(boost::regex("Title"));
      if (ucrl.getValue()) tagPatterns.push_back(boost::regex("UCRL"));
    }
    patternStrings = valuePatternStrings.getValue(); 
    for (uint patno = 0; patno < patternStrings.size(); patno++) {
      valuePatterns.push_back(boost::regex(patternStrings[patno])); 
    }
  }  
  string jsonFileName = jsonFileNameFlag.getValue(); 
  if (jsonFileName == "") {
	jsonFileName = lorenzFileNameFlag.getValue();
  }
  if (jsonFileName != "") {
	stringstream JsonString; 
	JsonString << "["; 
	for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {
      string filename = movienames.getValue()[fileno]; 
	  smBase *sm = smBase::openFile(filename.c_str(), O_RDONLY, 1);
	  if (fileno) {
		JsonString << ",\n"; 
	  }
	  if (!sm) {
		cerr << str(boost::format("WARNING: could not open movie file %s.")% filename) << endl; 
		SM_MetaData::WriteJsonError(&JsonString, filename); 
	  } else {
		TagMap moviedata = sm->GetMetaData(); 
		SM_MetaData::WriteMetaDataToStream(&JsonString, moviedata);
	  }
	}
	JsonString << "]\n"; 
	if (jsonFileName == "stdout" || 
		jsonFileName == "-") {
	  cout << JsonString.str(); 
	} else {
	  ofstream jsonFile(jsonFileName.c_str()); 
	  if (!jsonFile.is_open()) {
		errexit(cmd, str(boost::format("Error:  could not open JSON file %s for writing") % jsonFileName), false); 
	  }
	  jsonFile << JsonString.str(); 
	}
	exit(0); 
  }
  bool singleTag = (tagPatterns.size() == 1 && valuePatterns.size() == 0); 
  bool matchedAll = true, matchedAny = false; 

  // should we print the filename of matches before the match? 
  bool prependFilename = prependFilenameFlag.getValue(); 
  if (movienames.getValue().size() > 1) prependFilename = true;
  if (dontPrependFilenameFlag.getValue()) prependFilename = false; 

  sm_setVerbose(verbosity.getValue());  
  dbg_setverbose(verbosity.getValue()); 
  for (uint fileno = 0; fileno < movienames.getValue().size(); fileno++) {
    if (canonical.getValue()) {
      canonicalTags = SM_MetaData::CanonicalMetaDataAsMap(false); 
    }
    string filename = movienames.getValue()[fileno]; 
    smBase *sm = smBase::openFile(filename.c_str(), O_RDONLY, 1);
    string tagfilename = exportTagfile.getValue();
    if (!sm || sm->haveError()) {
      string errmsg; 
      if (!sm) {
        errmsg = str(boost::format("ERROR: Movie file %s is missing or corrupt.")% filename); 
      } else {
        errmsg = str(boost::format("ERROR: Movie file %s self-reported error while opening: %s")%filename%(sm->errorMessage())); 
      }
      if (tagfilename != "") {
        using boost::property_tree::ptree; 
        ptree pt;
        pt.put("name.type", "ASCII"); 
        pt.put("name.value", filename); 
        pt.put("error.type", "ASCII"); 
        pt.put("error.value", errmsg); 
        if (tagfilename != "stdout" && tagfilename != "-") {
          ofstream ofile(tagfilename.c_str()); 
          write_json(ofile, pt);
        }
        else {
          write_json(cout, pt); 
        }
      }
      errexit(cmd, errmsg, false); 
    }

    // Movie info case... (both sminfo and smquery file)
    if (getinfo) {  
      cout << sm->InfoString(verbosity.getValue()) << endl; 
    }
    if (tagfilename != "") {
      TagMap moviedata = sm->GetMetaData(); 
      if (tagfilename  != "stdout" && tagfilename != "-") {
        ofstream tagfile(tagfilename.c_str()); 
        if (!tagfile) {
          errexit(cmd, str(boost::format("Error:  could not open tag file %s for movie %s")% filename %  sm->getName()), false); 
        }
        SM_MetaData::WriteMetaDataToStream(&tagfile, moviedata);
      } else {
        SM_MetaData::WriteMetaDataToStream(&cout, moviedata);
      }
      if (quiet.getValue()) {
        continue; 
      } else {
        cout << "Wrote movie meta data tag file " << filename << endl; 
      }
    }
      
   smdbprintf(3, "Metadata for %s: (%d entries)\n", filename.c_str(), sm->mMetaData.size()); 
    if (exportPosterFrame.getValue() != "") {
      matchedAny = true; 
      ExportPosterFrame(exportPosterFrame.getValue(), sm);  
    }
   
    int32_t posternum = -1;
    int numMatches = 0; 
     // for long list format:
    vector<string> tagMatches, valueMatches, valueTypes, matchTypes;
    uint longestTagMatch = 0, longestValueType = 5; 
    vector<bool> matchedTags(tagPatterns.size(), false), matchedValues(valuePatterns.size(), false); 
    for (map <string,SM_MetaData>::iterator pos = sm->mMetaData.begin();
         pos != sm->mMetaData.end(); pos++) {
      string mdtag = pos->first, mdvalue = pos->second.ValueAsString(), 
        mdtype = pos->second.TypeAsString(); 
      bool tagmatch = MatchesAPattern(tagPatterns, mdtag, matchedTags), 
        valuematch = MatchesAPattern(valuePatterns, mdvalue, matchedValues);

      if (canonicalTags.find(mdtag) != canonicalTags.end()) {
        canonicalTags[mdtag].Set(mdtag,mdtype,mdvalue);                   
      }

      if (tagmatch || valuematch) {
        numMatches ++; 
        matchedAny = true; 
        if (quiet.getValue() && !andFlag.getValue()) {
          break; 
        }
      }

      if (filenameOnly.getValue()) {
        if (tagmatch || valuematch) {          
          cout << filename << endl; 
          break; 
        }
      }
      else {
        if ((tagmatch || valuematch)) {
          string matchtype; 
          if (matchAll) {
            matchtype = ""; 
          } else if (tagmatch && valuematch) {
            matchtype = "Both Match: "; 
          } else if (tagmatch) {
            matchtype = "Tag Match: ";
          } else if (valuematch) {
            matchtype = "Value Match: ";
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
      }
    }
    if (posterInfo.getValue()) {
      int64_t  frame = sm->getPosterFrame(); 
      cout << str(boost::format("%1%: poster frame: %2%\n") % filename % frame); 
    }
    if (canonical.getValue()) {
      cout << str(boost::format("Canonical tags for movie %s:")% filename) << endl; 
      vector<SM_MetaData> cmdata = SM_MetaData::CanonicalMetaData(false);
      for (uint i = 0; i<cmdata.size()-1; i++) {
        SM_MetaData * smdp = &canonicalTags[cmdata[i].mTag];
        cout << str(boost::format("%1%: (%2%) %3%:\n") % (smdp->mTag) % (smdp->TypeAsString()) % (smdp->ValueAsString()));
      }
    } 
    if (!quiet.getValue() ) {
      if (singleTag && valueMatches.size() == 1) {
        if (prependFilename) {
          cout << filename << ": "; 
        }
        cout << valueMatches[0] << endl;         
      } else {
        if (!tagMatches.size() && !matchedAny) {
          printf( "No tags for movie %s matched.\n", filename.c_str()); 
        }
        if (singleTag && valueMatches.size() > 1) {
          cout <<  "Warning: a single tag was specified but multiple tags matched the expression given" << endl; 
        }
        for (uint i=0; i< tagMatches.size(); i++) {
          SM_MetaData md(tagMatches[i], valueTypes[i], valueMatches[i]); 
          if (prependFilename) {
            cout << filename << ": "; 
          }
          cout << md.toShortString(matchTypes[i], longestValueType, longestTagMatch+2) << endl;
        }      
      }
    }
    if (andFlag.getValue()) {
      for (vector<bool>::iterator pos = matchedTags.begin(); pos != matchedTags.end() && matchedAll; pos++) {
        matchedAll = *pos && matchedAll;           
      } 
      for (vector<bool>::iterator pos = matchedValues.begin(); pos != matchedValues.end() && matchedAll; pos++) {
        matchedAll = *pos && matchedAll;           
      } 
    }
  smdbprintf(1, str(boost::format("Finished with movie %1%\n") % filename).c_str()); 
    delete sm;
  }

  if (andFlag.getValue()) {
    if (!quiet.getValue()) {
      if (matchedAll) {
      smdbprintf(1, "Matched all tags for all movies.\n"); 
      } 
      else {
      smdbprintf(1, "Did not match all tags for all movies.\n"); 
      }
    }
    return matchedAll ? 0: 1; 
  }

  if (!quiet.getValue()) {
    if (matchedAll) {
    smdbprintf(1, "Matched at least one tag among all movies.\n"); 
    } 
    else {
    smdbprintf(1, "Did not match any tags for any movies.\n"); 
    }
  }

  return (matchedAny || matchedAll)? 0 : 1;
}
