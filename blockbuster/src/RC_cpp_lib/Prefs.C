/* MODIFIED BY: rcook on Thu May 15 17:42:03 PDT 2014 */
/* VERSION: 1.0 */
#define NO_BOOST 1
#include "Prefs.h"
#include "stringutil.h"
#include "errno.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using boost::property_tree::ptree; 

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
ArgType &ArgType::SetLongFlag(void) {
  mFlags.push_back(str(format("--%1%")%mKey)); 
  return *this; 
}

//====================================================
ArgType &ArgType::SetShortFlag(void) {
  mFlags.push_back(str(format("-%c")%mKey[0])); 
  return *this; 
}

//====================================================
ArgType &ArgType::SetFlags(string flag1, string flag2, string flag3) {
  if (flag1 == "" && flag2 == "" && flag3 == "") {
    SetLongFlag(); 
    SetShortFlag(); 
  }
  if (flag1 != "") {
    mFlags.push_back(flag1); 
  }
  if (flag2 != "") {
    mFlags.push_back(flag2); 
  }
  if (flag3 != "") {
    mFlags.push_back(flag3); 
  }
  return *this; 
}
 

//====================================================
const ArgType &ArgType::operator =(const ArgType &other) {
  mKey = other.mKey; 
  mType = other.mType; 
  mFlags = other.mFlags; 
  mMultiple = other.mMultiple; 
  mValues = other.mValues; 
  return *this; 
}
    
//====================================================
ArgType::operator string() {
  
  string output = str(format("mKey: \"%s\", mType: \"%s\", mMultiple: %s, mFlags: ")% mKey % mType % (mMultiple?"true":"false")); 
  vector<string>::iterator pos = mFlags.begin(); 
  output += "<"; 
  while (pos != mFlags.end()) {
    output += str(format("\"%s\"")%(*pos)); 
    if (++pos != mFlags.end()){
      output += ", "; 
    }  
  } 
  output += ">, mValues: "; 
  pos = mValues.begin(); 
  output += "<"; 
  while (pos != mValues.end()) {
    output += str(format("\"%s\"")%(*pos)); 
    if (++pos != mValues.end()){
      output += ", "; 
    }
  }
  output += ">"; 
  return output; 
}

//====================================================
void Preferences::Merge(const Preferences &other, bool addnew, bool overwrite) {

  if (!addnew && !overwrite) return; // one must be true for anything to happen

  std::map<string, ArgType >::const_iterator pos = other.mPrefs.begin(), endpos = other.mPrefs.end(), found; 
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

  char *dbptr = getenv("debugprefs");
  if (dbptr) {
    debugprefs = dbptr;
    cerr << "enabling debugging of prefs because debugprefs environment variable is set" << endl; 
  }

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
  return (outValue != ""); 
}

//====================================================
bool Preferences::TryGetDoubleValue(const string &key, double &outValue) const {
  try {
    outValue = GetDoubleValue(key, true); 
  } catch (...) {
    outValue = DOUBLE_FALSE;
    return false; 
  }
  return outValue!= DOUBLE_FALSE; 
}

//====================================================
bool Preferences::TryGetLongValue(const string &key, long &outValue) const {
  try {
    outValue = GetLongValue(key, true);   
  } catch(...) {
    outValue = LONG_FALSE;
    return false; 
  }
  return outValue != LONG_FALSE; 
}

//====================================================
vector<string> Preferences::GetValues(const std::string &key, bool dothrow) const {
  if (mPrefs.find(key) == mPrefs.end()) {
    if (dothrow) {
      throw string(key+" not found");
    } else {
      vector<string> blank(1); 
      return blank;
    }
  }
  return mPrefs.find(key)->second.mValues;
}

//====================================================
string Preferences::GetValue(const std::string &key, bool dothrow) const { 
  vector<string> values = GetValues(key,dothrow); 
  return values[0]; 
}


//====================================================
template <class T> 
T Preferences::GetValue(const std::string &key, bool dothrow, T &outval) const {
  string value = GetValue(key, dothrow);
  try {
    outval = boost::lexical_cast<T>(value); 
  } catch (...) {
    if (dothrow) {
      throw string("Bad value in  Preferences::GetValue");
    } 
  }
  return outval; 
}

//====================================================
template <class T> 
vector<T> Preferences::GetValues(const std::string &key, bool dothrow, vector<T> &outvals) const {
  vector<string> values = GetValues(key, dothrow);
  for ( vector<string>::iterator value = values.begin(); value != values.end(); value++) {
    try {
      outvals.push_back( boost::lexical_cast<T>(*value)); 
    } catch (...) {
      if (dothrow) {
        throw string("Bad value in  Preferences::GetValues");
      } 
    }
  }   
  return outvals; 
}

//====================================================
double Preferences::GetDoubleValue(const string &key, bool dothrow) const {
  double v; 
  return GetValue(key, dothrow, v); 
}

//====================================================
vector<double> Preferences::GetDoubleValues(const string &key, bool dothrow) const {
  vector<double> v; 
  return GetValues(key, dothrow, v); 
}

//====================================================
long Preferences::GetLongValue(const string &key, bool dothrow) const {
  long v; 
  return GetValue(key, dothrow, v); 
}

//====================================================
vector<long> Preferences::GetLongValues(const string &key, bool dothrow) const {
  vector<long> v; 
  return GetValues(key, dothrow, v); 
}

//====================================================
Preferences::operator string()  {
  string output = "Preferences: { \n"; 
  output += "  mValidArgs: {";
  for (map<string,ArgType>::iterator argpos = mValidArgs.begin(); argpos != mValidArgs.end(); argpos++) {
    output += str(format("\n    %s")%(string(argpos->second))); 
  }
  output += "  }\n"; 
  output += "  mPrefs: {"; 
  for (map<string,ArgType>:: iterator pref = mPrefs.begin(); pref != mPrefs.end(); pref++) {
    output += string(pref->second) + "\n"; 
  }
  output += "  }\n"; 
  return output; 
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
    SetValue(key,value);
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
bool Preferences::SaveToFile(bool createDir, bool clobber) {
  string filename = GetValue("Filename"); 
  if (filename == "") 
    return false; 

  if (createDir) {
    string::size_type idx = filename.rfind('/'); 
    if (idx != string::npos) {
      string dirname = filename.substr(0, idx); 
      system((string("mkdir -p ")+dirname).c_str()); 
    }
  }
  ptree saved; 
  if (!clobber) {
    ifstream infile(filename.c_str()); 
    
    if (infile.is_open()) {
      return false; // file exists; do not overwrite
    } 
  }

  // We can now write our json file
  ptree pt; 
  for (map<string, ArgType >::iterator pref = mPrefs.begin(); pref != mPrefs.end(); pref++) {
    pt.put(str(format("%s.mType")%(pref->second.mKey)), pref->second.mType); 
    pt.put(str(format("%s.mMultiple")%(pref->second.mKey)), 
           str(format("%d")%((int)(pref->second.mMultiple)))); 
    if (pref->second.mFlags.size()) {
      ptree flags;
      for (vector<string>::iterator flagval = pref->second.mFlags.begin(); 
           flagval != pref->second.mFlags.end(); flagval++) {
        ptree flag; 
        flag.put("", *flagval); 
        flags.push_back(make_pair("", flag)); 
      }
      pt.add_child(str(format("%s.mFlags")% (pref->second.mKey)), flags); 
    }
    if (pref->second.mValues.size()) {
      ptree values; 
      for (vector<string>::iterator valueval = pref->second.mValues.begin(); 
           valueval != pref->second.mValues.end(); valueval++) {
        ptree value; 
        value.put("", *valueval); 
        values.push_back(make_pair("", value)); 
      }
      pt.add_child(str(format("%s.mValues")% (pref->second.mKey)), values); 
    }
  }
  write_json(filename, pt); 
  return true; 
}

//====================================================
bool Preferences::ReadFromFile(bool throw_exceptions) {
  string filename = GetValue("Filename");
  { 
    ifstream file(filename.c_str()); 
    if (!file || !file.is_open()) {
      if (throw_exceptions) {
        throw str(boost::format("Cannot open filename %s")%filename);
      }
      return false; 
    }
    file.close(); // redundant but clear
  }

  try {
    ptree pt;
    read_json(filename, pt); 
    if (pt.empty()) {
      return false; 
    }
    for (ptree::iterator pos = pt.begin(); pos != pt.end(); pos++) {  
      string key = pos->first; 
      ArgType arg(key); 
      for (ptree::iterator child = pos->second.begin(); child != pos->second.end(); child++) {
        ptree childtree = child->second;
        if (child->first == "mType") {
          arg.mType = childtree.data(); 
        } else if (child->first == "mMultiple") {
          arg.mMultiple = lexical_cast<int>(childtree.data()); 
        } else if (child->first == "mFlags") {
          for (ptree::iterator flag = childtree.begin(); flag != childtree.end(); flag++) {
            arg.mFlags.push_back(flag->second.data()); 
          }
        } else if (child->first == "mValues") {
          for (ptree::iterator value = childtree.begin(); value != childtree.end(); value++) {
            arg.mValues.push_back(value->second.data()); 
          }
        }
      }
      mPrefs[arg.mKey] = arg; 
    }
  } catch (...) {
    cerr << "Exception in ReadFromFile" << endl;      
    if (throw_exceptions) {
      throw; 
    } 
    return false; 
  }
  return true; 
}



// ================================================================== 
// Returns a vector of unparsed arguments.  Does not modify argc and argv
vector<string> Preferences::GetFromArgs(int &argc, char *argv[], vector<ArgType>& args, bool rejectUnknown) {
  
  // first, initialize all keys in "argtypes" array to defaults if not already set to some value
  AddArgs(args); 
  return ParseArgs(argc, argv, rejectUnknown); 
}
      
// ================================================================== 
// Returns a vector of unparsed arguments ("positionals"). 
vector<string> Preferences::ParseArgs(int &argc, char *argv[], bool rejectUnknown) {
  // parse the argc and argv as command line options.
  // 
   for (int argnum = 1; argnum < argc; argnum++) { 
    ArgType *validArg = NULL;
    string currentArg = argv[argnum]; 
    string foundflag; 
    map<string, ArgType>::iterator argpos;     
    for (argpos = mValidArgs.begin();  
         !validArg && argpos != mValidArgs.end(); 
         ++argpos) {
      for (vector<string>::iterator flagpos = argpos->second.mFlags.begin(); 
           !validArg && flagpos != argpos->second.mFlags.end();
           flagpos++) {
        if (*flagpos == currentArg) {
          validArg = &(argpos->second);   
          foundflag = *flagpos; 
        }
      }
    }

    if (validArg) {      
      if (validArg->mType == "bool") {
        SetValue(validArg->mKey, 1); 
        continue; 
      } else {
        argnum++; 
        if (argnum == argc) {
          throw string("Flag ")+foundflag+string(" requires an argument");
        }     
        string arg = argv[argnum];        
        try {
          if (validArg->mType == "long") {
            boost::lexical_cast<long>(arg);
          } else if (validArg->mType == "double") {
            boost::lexical_cast<double>(arg); 
          } else {
            throw string("Bad keytype: ")+validArg->mType+string(" for key: ")+validArg->mKey; 
          }
          SetValue(validArg->mKey, arg, validArg->mType);
        } catch (string e) {
          cerr << e << endl; 
          throw ; 
        } catch (...) {
          throw str(format("Could not convert value %s to type %s for flag %s") % arg % validArg->mType % foundflag); 
        }
      }
    } /* end  if (found) */
    else { 
      if (currentArg[0] == '-' && rejectUnknown) {
        throw str(format("Unknown argument %s ")%currentArg); 
      }        
      mUnparsedArgs.push_back(currentArg); 
    }      
  } // end while (argnum < argc)
   return  mUnparsedArgs; 
} // end ParseArgs()
