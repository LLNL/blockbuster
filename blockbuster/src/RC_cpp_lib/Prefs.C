/* MODIFIED BY: rcook on Fri Apr 11 18:31:40 PDT 2014 */
/* VERSION: 1.0 */
#define NO_BOOST 1
#include "Prefs.h"
#include "stringutil.h"
#include "errno.h"
#include <boost/lexical_cast.hpp>
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
ArgType::ArgType(string key,  int autoFlags, int multi, string defaultVal):
  mKey(key), mType("string"), mMultiple(multi), mValues(1,defaultVal){
  if (autoFlags) {
    mFlags.push_back(str(format("--%1%")%key)); 
    mFlags.push_back(str(format("-%c")%key[0])); 
  }
  return;
 
}

//====================================================
ArgType::ArgType(string key, string preftype,  int autoFlags, 
                 int multi, string defaultVal):
  mKey(key), mType(preftype), mMultiple(multi), mValues(1,defaultVal) {
  if (autoFlags) {
    mFlags.push_back(str(format("--%1%")%key)); 
    mFlags.push_back(str(format("-%c")%key[0])); 
  }
  return; 
}
  
//====================================================
ArgType::ArgType(string key, string preftype, vector<string> flags, 
                 int multi, string defaultVal):
  mKey(key), mType(preftype), mMultiple(multi), 
  mFlags(flags), mValues(1,defaultVal) {
  return; 
}

//====================================================
ArgType::ArgType(string key, string preftype, 
                 string flag1, string flag2, 
                  int multi, string defaultVal):
  mKey(key), mType(preftype), mMultiple(multi), 
  mValues(1,defaultVal) {
  mFlags.push_back(flag1); 
  mFlags.push_back(flag2); 
  return; 
}

//====================================================
ArgType::ArgType(string key, string preftype, string flag, 
                 int multi, string defaultVal):
  mKey(key), mType(preftype), mMultiple(multi), 
  mFlags(1,flag), mValues(1,defaultVal) {
  return; 
}

//====================================================
ArgType::ArgType(const ArgType &other) {
  *this = other; 
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
  string output = str(format("mKey: \"%s\", mMultiple: %d, mType: \"%s\", mFlags: ")% mKey % ((int)mMultiple) % mType); 
  vector<string>::iterator pos = mFlags.begin(); 
  if (pos != mFlags.end()) {
    output += "<"; 
  }
  while (pos != mFlags.end()) {
    output += str(format("\"%s\"")%(*pos)); 
    if (++pos == mFlags.end()){
      output += ">"; 
    } else {
      output += ", "; 
    }
  }
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
  for (vector<ArgType>::iterator argpos = mValidArgs.begin(); argpos != mValidArgs.end(); argpos++) {
    output += str(format("\n    %s")%(string(*argpos))); 
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
   try {
     ptree pt;
     read_json(GetValue("Filename"), pt); 
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
void ConsumeArg(int argnum, int &argc, char *argv[]){
  while (argnum < argc-1) {
    argv[argnum] = argv[argnum+1];
    ++argnum; 
  }
  argv[argnum] = NULL; 
  argc--; 
}

// ================================================================== 
void Preferences::GetFromArgs(int &argc, char *argv[], vector<ArgType>& args, bool rejectUnknown) {
  
  // first, initialize all keys in "argtypes" array to defaults if not already set to some value
  AddArgs(args); 
  ParseArgs(argc, argv, rejectUnknown); 
  return; 
}
      
// ================================================================== 
void Preferences::ParseArgs(int &argc, char *argv[], bool rejectUnknown) {
  // parse the argc and argv as command line options.
  // 
  errno = 0; 
  int argnum = 0; 
  string foundflag; 
  while (argnum < argc) {
    string currentArg = argv[argnum]; 
    vector<ArgType>::iterator validArg; 
    for (validArg = mValidArgs.begin();  
         validArg != mValidArgs.end(); 
         ++validArg) {
      for (vector<string>::iterator flagpos = validArg->mFlags.begin(); 
           flagpos != validArg->mFlags.end();
           flagpos++) {
        if (*flagpos == currentArg) {
          foundflag = *flagpos;   
          break; 
        }
      }
      if (foundflag != "") break; 
    }

    if (foundflag == "") {
      if (rejectUnknown && currentArg[0] == '-') {
        throw str(format("Unknown argument %s ")%currentArg); 
      }        
      argnum++; 
    } else {
      // do not increment argnum here as you will simply be consuming args and reducing argc
      ConsumeArg(argnum, argc,argv); 
      if (validArg->mType == "bool") {
        SetBoolValue(validArg->mKey, true); 
      } else {
        if (argnum == argc) {
          throw string("Flag ")+foundflag+string(" requires an argument");
        }     
        string value = argv[argnum];
        try {
          if (validArg->mType == "string") {
            SetValue(validArg->mKey, value);
          } else if (validArg->mType == "long") {
            SetLongValue(validArg->mKey, boost::lexical_cast<long>(value));
          } else if (validArg->mType == "double") {
            SetDoubleValue(validArg->mKey,  boost::lexical_cast<double>(value)); 
          } else {
            throw string("Bad keytype: ")+validArg->mType+string(" for key: ")+validArg->mKey; 
          }
        } catch (...) {
          throw str(format("Could not convert value %s to type %s for flag %s") % value % validArg->mType % foundflag); 
        }
      }// end check which arg type it is
      ConsumeArg(argnum, argc,argv); 
    } /* end  if (found) */
  } // end while (argnum < argc)
} // end ParseArgs()
