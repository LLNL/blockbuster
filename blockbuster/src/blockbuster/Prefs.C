#define NO_BOOST 1
#include "Prefs.h"
#include "stringutil.h"
#include "errno.h"
#include <boost/lexical_cast.hpp>
/* class Preferences
   member functions 
*/
// $Revision: 1.27 $

//#ifdef __USE_GNU
extern char **environ;

//#endif

using namespace std;

// IF YOU WANT TO DEBUG PREFS, set the "debugprefs" environment variable to 1
static string debugprefs("false"); 
#define prefsdebug if (debugprefs != "false") cerr << __FILE__ <<" line "<<__LINE__ << ": "

//====================================================
void Preferences::Merge(const Preferences &other, bool addnew, bool overwrite) {

  if (!addnew && !overwrite) return; // one must be true for anything to happen

  std::map<string, string>::const_iterator pos = other.mPrefs.begin(), endpos = other.mPrefs.end(), found; 
  while (pos != endpos) {
    found = mPrefs.find(pos->first); 
    if ((addnew && found == mPrefs.end()) ||
        (overwrite && found != mPrefs.end()) ) {
      mPrefs[pos->first] = pos->second; 
    }
    ++pos; 
  }
}
      

//====================================================
void Preferences::Reset(void){
  ClearPrefs();
  mFilename  = "";

  char *dbptr = getenv("debugprefs");
  if (dbptr) {
    debugprefs = dbptr;
    cerr << "enabling debugging of prefs because debugprefs environment variable is set" << endl; 
  }
 
  SetLabel("SoleRecord");//if "SoleRecord", then we know there is only one section in the file
  _dirty = 0;
  _writtenToDisk = 0;
  return;
}

//====================================================
bool Preferences::TryGetValue(const string &key,  std::string &outValue) const {
  if (mPrefs.find(key) == mPrefs.end()) {
    return false; 
  }
  outValue = GetValue(key, true);    
  return true; 
}

//====================================================
bool Preferences::TryGetDoubleValue(const string &key, double &outValue) const {
  try {
    outValue = GetDoubleValue(key, true); 
  } catch (...) {
    outValue = DOUBLE_FALSE;
    return false; 
  }
  return true; 
}

//====================================================
bool Preferences::TryGetLongValue(const string &key, long &outValue) const {
  try {
    outValue = GetLongValue(key, true);   
  } catch(...) {
    outValue = LONG_FALSE;
    return false; 
  }
  return true; 
}

//====================================================
std::string Preferences::GetValue(const std::string &key, bool dothrow) const { 
  if (mPrefs.find(key) == mPrefs.end()) {
    if (dothrow) {
      throw string(key+" not found");
    } else {
      return string("");
    }
  }
  return mPrefs.find(key)->second;
}

//====================================================
double Preferences::GetDoubleValue(const string &key, bool dothrow) const {
  string value = GetValue(key, dothrow);
  try {
    return boost::lexical_cast<double>(value); 
  } catch (...) {
    if (dothrow) {
      throw string("Bad key in  Preferences::GetDoubleValue");
    } 
  }
  return DOUBLE_FALSE;  
}

//====================================================
long Preferences::GetLongValue(const string &key, bool dothrow) const {
  string value = GetValue(key, dothrow);
  try {
    return boost::lexical_cast<long>(value); 
  } catch (...) {
    if (dothrow) {
      throw string("Bad key in  Preferences::GetDoubleValue");
    } 
  }
  
  return LONG_FALSE;  
}



//====================================================
/* Look in the environment for new values for previously declared keys */ 
void Preferences::ReadFromEnvironment(void){
  prefsdebug << "ReadFromEnvironment"<<endl;
  char **envp = environ; 
  char *currentstring; 
  while ((currentstring = *envp) != NULL) {
    string keypair(currentstring); 
    string::size_type equals = keypair.find('=');
    if (equals == string::npos) {
      throw string("Error in environment:  environment string is malformed: \"") + keypair + "\""; 
    }    
    string key = keypair.substr(0, equals), value = keypair.substr(equals+1);
    prefsdebug << "Setting prefs key "<< key<< " to value from environment: " << value << endl; 
    mPrefs[key] = value;
    ++envp; 
  }
  return; 
}

//====================================================
/* skip comments and get the next valid token */
string Preferences::NextKey(ifstream&theFile){
  char buf[2048];
  string key("");
  prefsdebug << "Preferences::NextKey"<<endl; 
  try {
    while (theFile) {
      theFile >> key;     
      if (key.length() && key[0] == '#')     
	theFile.getline(buf, 2048);
      else
	break;
    }
  } catch (...) {
    cerr << "Error in NextKey()" << endl; 
    throw; 
  }
  return key;
}

//====================================================
map<string, string> Preferences::ReadNextSection(ifstream &theFile){
  map<string, string> theMap;
  char buf[2048];
  string key, value; 
  prefsdebug<<"Preferences::ReadNextSection "<<endl;
  try { 
    key = NextKey(theFile);
  } catch (string err ) {
    cerr << "Error getting first key in prefs"<<endl;
    throw err;
  }   if (!key.size())
    throw string("Unexpected EOF in prefs");

  if (key != "BEGINRECORD")
    throw string("Prefs records must begin with BEGINRECORD, not \"")+key+"\"";
  prefsdebug << "found BEGINRECORD in prefs file" << endl;
  fflush(stdout); 
  theFile.getline(buf, 2048);//toss away rest of line

  char gotlabel = 0;
  while (key.size()) {
    try {
      key = NextKey(theFile);
    } catch (string err) {
      cerr << "Error getting next key" << endl; 
      break; 
    }
    prefsdebug << "Found key: " << key << endl; 
    fflush(stdout); 
    if (!gotlabel) {
      if (key != "prefs_label") {
	// this needs to be the label
	throw string("Improperly formatted line in file: ")+key  + string(buf);
      }
      gotlabel=1;
    }
    if (key == "ENDRECORD")
      break;
    theFile.getline(buf, 2048); // get the rest of the line
    string value(buf);
    if (value == "") {
      prefsdebug<< "No valid value found for line, skipping..."<<endl;
      continue; 
    }
    string::size_type idx = value.find_first_not_of(" \t\n\r");
    value = value.substr(idx);
    theMap[key] =value;    
    prefsdebug<< "map["<<key<<"] = \""<<value<<"\""<<endl; 
    fflush(stdout); 
  }

  if (key != "ENDRECORD")
    throw string("Cannot find valid map in file.");
  return theMap;
}

//====================================================
 //open the file, seek past label, read my section, close file
/* this may optionally throw an exception if it can't be read from the file 
   by default it won't
 */
void Preferences::ReadFromFile(bool throw_exceptions){
  ifstream theFile(mFilename.c_str());
  prefsdebug<<"Preferences::ReadFromFile"<<endl;
  if (!theFile) {
    if (throw_exceptions) {
      throw string("Can't open file for reading: ")+mFilename;
    }
    return; 
  }
  map<string, string> current;
  current["prefs_label"] = "!!!??? nothing at all (no match)";
  string targetlabel;
  try {
    targetlabel=GetLabel(); 
  } catch (...) {
    cerr << "GetLabel failed in Preferences::ReadFromFile" << endl; 
    if (throw_exceptions) {
      throw; 
    }
    return; 
  }
  while (current["prefs_label"] != targetlabel){
    try {
      prefsdebug << "read next section"<<endl;
      current = ReadNextSection(theFile);
    }
    catch(string s){
      cerr << "Warning: can't read prefs \""<<GetLabel()<<"\" from file "<<mFilename<<endl;
      cerr << s << endl;
      if (throw_exceptions){
        throw s;
      }
      return;
    }
    map<string, string>::iterator pos = current.begin(), endpos = current.end(); 
    while (pos != endpos) {
      mPrefs[pos->first] = pos->second; 
      ++pos; 
    }
  }
  prefsdebug << "Finished reading prefs: "<< string(*this) << endl;
  _dirty = 0;
  _writtenToDisk = 1;
  return;
}

//====================================================
bool Preferences::KeyValid(string key) {
  vector<argType>::iterator pos = mValidArgs.begin(), endpos = mValidArgs.end(); 
  while (pos != endpos) {
    if (key == pos->mKey) return true; 
    ++pos; 
  }

  return false; 
}

//====================================================
void Preferences::SaveSectionToFile(ofstream &outfile, map<string, string> &section){
  outfile <<"BEGINRECORD"<<endl;
  outfile << "prefs_label\t" << section["prefs_label"]<<endl;
  map<string, string>::iterator pos = section.begin();
  while (pos != section.end()){
    if (KeyValid(pos->first)) {
      outfile << pos->first << "\t" ;
      if (!(pos->second).size())
        outfile << "(nil)"<<endl;
      else {
        string clean = pos->second; 
        replace(clean.begin(), clean.end(), '\n', '\t'); 
        outfile << clean << endl;
      }      
    }
    ++pos;
  }
  outfile << "ENDRECORD" << endl;
  return; 
}
  
//====================================================
void Preferences::SaveToFile(bool createDir, bool clobber){
  vector<map<string, string> > saved;
  map<string, string> current;  
  map<string, string>::iterator pos;
  
  if (createDir) {
    string::size_type idx = mFilename.rfind('/'); 
    if (idx != string::npos) {
      string dirname = mFilename.substr(0, idx); 
      system((string("mkdir -p ")+dirname).c_str()); 
    }
  }
  char found = 0;
  if (!clobber) {
    cerr << "preserving old pref file" << endl; 
    ifstream infile(mFilename.c_str()); 
    while (infile){
      try {
        current = ReadNextSection(infile);
        pos = current.find("prefs_label");
        if (pos!=current.end() && pos->second == GetLabel()) {
          found = 1;
          saved.push_back(mPrefs);
        }
        else 
          saved.push_back(current);
      }
      catch(...){
        break;
      }
    }
  }
   if (!found)
      saved.push_back(mPrefs);
 
  vector<map<string, string> >::iterator outpos = saved.begin();
  if (outpos == saved.end())
    throw string("Prefs: Fatal Error: nothing to save!");
  
  ofstream outfile(mFilename.c_str());
  if (!outfile)
    throw string("Cannot open file for writing: ")+mFilename;
  
  while (outpos != saved.end()){
    SaveSectionToFile(outfile, *outpos);
    ++outpos;
  }
  prefsdebug << "Finished writing prefs."<<endl;
  mPrefs = current;
  return;
}

void ConsumeArg(int argnum, int &argc, char *argv[]){
  while (argnum < argc-1) {
    argv[argnum] = argv[argnum+1];
    ++argnum; 
  }
  argv[argnum] = NULL; 
  argc--; 
}
void Preferences::GetFromArgs(int &argc, char *argv[], vector<argType>& argtypes) {
  
  // first, initialize all keys in "argtypes" array to defaults if not already set to some value
  vector<argType>::iterator argPos; 
  for (argPos = argtypes.begin(); argPos != argtypes.end(); ++argPos) {
    if (mPrefs.find(argPos->mKey) == mPrefs.end()) {      
      if (argPos->mType == "bool") {
        SetValue(argPos->mKey, false); 
      } else if (argPos->mType == "string") {
        SetValue(argPos->mKey, string(""));
      } else if (argPos->mType == "long" || argPos->mType == "double") {
        SetValue(argPos->mKey, 0); 
      }
    }
  }
  ParseArgs(argc, argv); 
  return; 
}
       
void Preferences::ParseArgs(int &argc, char *argv[]) {
  // parse the argc and argv as command line options.
  // 
  errno = 0; 
  int argnum = 0; 
  string foundflag; 
  while (argnum < argc) {
    for (argPos = argtypes.begin();  
         argPos != argtypes.end(); 
         ++argPos) {
      for (vector<string>::iterator flagpos = argPos->mFlags.begin(); 
           flagpos != argPos->mFlags.end();
           flagpos++) {
        if (*flagpos == string(argv[argnum])) {
          foundflag = *flagpos;   
          break; 
        }
      }
      if (foundflag != "") break; 
    }

    if (foundflag == "") {
      argnum++; 
    } else {
       // do not increment argnum here as you will simply be consuming args and reducing argc
      ConsumeArg(argnum, argc,argv); 
    
      if (argPos->mType == "bool") {
        SetValue(argPos->mKey, true); 
      } else {
        if (argnum == argc) {
          throw string("Flag ")+foundflag+string(" requires an argument");
        }     
        if (argPos->mType == "string") {
          SetValue(argPos->mKey, string(argv[argnum]));
        } else if (argPos->mType == "long") {
          SetValue(argPos->mKey, strtol(argv[argnum], NULL, 10));
          if (errno) {
            throw string("Could not convert ")+argv[argnum]+string(" to long");
          }
        } else if (argPos->mType == "double") {
          SetValue(argPos->mKey, strtod(argv[argnum], NULL) ); 
          if (errno) {
            throw string("Could not convert ")+argv[argnum]+string(" to double");
          }
        } else {
          throw string("Bad keytype: ")+argPos->mType+string(" for key: ")+argPos->mKey; 
        }
        ConsumeArg(argnum, argc,argv); 
      } // end check which arg type it is
    } /* end  if (found) */
  }// end while (argnum < argc)
}
