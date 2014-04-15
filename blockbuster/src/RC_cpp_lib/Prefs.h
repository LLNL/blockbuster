/* MODIFIED BY: rcook on Fri Apr 11 18:31:40 PDT 2014 */
/* VERSION: 1.0 */
/* This file is an attempt to allow any application to read its preferences into what Randy Frank would call a "mapobj".  I have stolen his idea and hopefully improved it to be more general and more robust, because it no longer relies on pointers to store its information.  In fact, it no longer allows pointers to be stored at all.  The presumption is that this is non-volatile information which can be written to disk.  I don't know of a way yet to pickle C items.  Maybe later I'll change to a binary output format and then allow any data to be captured to disk.  Not today. 
   All values are stored as C++ strings.  Functions which set or get values as other types are merely converting a string to the desired type or vice-versa.

   To do:  multiple Preferences should be able to be written to a single file. 
 */
// $Revision: 1.20 $
#ifndef RCPREFS_H
#define RCPREFS_H

#include <map>
#include <iostream>
#include <vector> 
#include <string>
#include <fstream>
#include "stringutil.h"
#include <boost/format.hpp>

#define DOUBLE_FALSE (0.0)
#define LONG_FALSE  (0)
using namespace boost; 


//======================================================
struct ArgType {

  ArgType(string key=""): mKey(key), mType("string") {}
  ArgType(string key,  int autoFlags, int multi, string defaultVal);

  ArgType(string key, string preftype,  int autoFlags,  
          int multi=false, string defaultVal="");
  
  ArgType(string key,  string preftype, vector<string> flags, 
          int multi=false, string defaultVal="");

  ArgType(string key, string preftype, 
          string flag1, string flag2, 
          int multi=false, string defaultVal="");

  ArgType(string key, string preftype, string flag, 
          int multi=false, string defaultVal="");

  ArgType(const ArgType &other);

  const ArgType &operator =(const ArgType &other);
    
  operator string();

  bool operator !=(const ArgType &other) { 
    return !( *this == other); 
  }
  bool operator == (const ArgType &other) {
    return (mKey == other.mKey &&
            mType == other.mType && 
            mMultiple == other.mMultiple &&
            mFlags == other.mFlags && 
            mValues == other.mValues); 
  }

  string mKey, 
    mType; // "bool", "long", "double", "string"
  bool mMultiple; // interpret as bool
  vector<string> mFlags; // aliasable e.g. "--help" and "-h" 
  vector<string> mValues; 
};

//======================================================
typedef  std::map<std::string, ArgType> PrefsMap; 

//======================================================
class Preferences {
 public:
  //========================
  //Initialization
  Preferences(string filename = ""){
    Reset();
    SetValue("Filename", filename); 
  }
  
  /* Merge:  combine this with other.  
     Iff addnew is true, then keys in other but not in this will be added to this
     Iff overwrite is true, then keys in other overwrite any keys in this by the same name.
     Note that if both are false, then nothing will be done! 
  */
  void Merge(const Preferences &other, bool addnew=true, bool overwrite=true); 
  // conveniences:
  void MergeNoOverwrite(const Preferences &other) { Merge(other, true, false); }
  void MergeNoAdd(const Preferences &other) {Merge(other, false, true); }

  void Reset(void); 
  void ClearPrefs(void) { mPrefs.clear();}
  std::map<std::string, ArgType > GetPrefsMap(void) { return mPrefs; }

  //========================
  // meta-attributes


  void SetFilename(string filename) {
    SetValue("Filename", filename); 
  }
  string GetFilename(void) {
    return GetValue("Filename"); 
  }
  //========================
  //saving and restoring from disk

 //====================================================
  bool ReadFromFile(bool throw_exceptions=true); 
 
  bool SaveToFile(bool createDir=true, bool clobber=true); 
  //=============================
  /* Glean the prefs from the command line.  The "types" argument is a vector of ArgTypes, which tell the flag, the key to assign, and the type, e.g., 
     vector<ArgType> types; 
     types.push_back("-keep", "keep", "bool"); 
     types.push_back("-longflag", "longkey", "long"); 
     types.push_back("-doubleflag", "doublekey", "double"); 
     types.push_back("-d", "doublekey", "double"); 
     GetFromArgs(argc, argv, types); 

     When done, the options and their arguments will be stripped from argv and argc will be adjusted appropriately.  
  */ 
  
  //====================================================
  PrefsMap::iterator begin(void) { 
    return mPrefs.begin(); 
  }

  PrefsMap::iterator end(void) { 
    return mPrefs.end(); 
  }
  //====================================================
  Preferences &AddArg(ArgType arg) {
    // first look for duplicates: 
    for (vector<ArgType>::iterator argPos = mValidArgs.begin();  
         argPos != mValidArgs.end(); 
         ++argPos) {
      if (arg.mKey == argPos->mKey) {
        throw str(format("AddArg Error: duplicate key: %s")%arg.mKey);
      }
      for (vector<string>::iterator flag = argPos->mFlags.begin(); 
           flag != argPos->mFlags.end(); flag++) {
        if (find(arg.mFlags.begin(), arg.mFlags.end(), *flag) != arg.mFlags.end()) {
          throw str(format("AddArg Error: duplicate flag: %s")%(*flag));
        }
      }
    }
  
    // now add a value in Prefs by type
    mPrefs[arg.mKey] = arg; 
    
    return *this; 
  }
  
  //====================================================
  Preferences &AddArgs(std::vector<ArgType> &args) {
    for (vector<ArgType>::iterator argPos = args.begin(); argPos != args.end(); ++argPos) {
      AddArg(*argPos); 
    }
    return *this; 
  }
  
  //====================================================
  // for backwards compatibility
  Preferences &SetValidArgs(std::vector<ArgType> &args) {
    AddArgs(args); 
    return *this; 
  }
  
  //====================================================
  void GetFromArgs(int &argc, char *argv[], vector<ArgType> &argtypes, bool rejectUnknown=true);

  //====================================================
  void ParseArgs(int &argc, char *argv[], bool rejectUnknown=true); 
  //=============================
  // Copy the entire environment variable list into prefs, e.g., if $verbose is 5, then set Prefs["verbose"] to "5"
  void ReadFromEnvironment(void); 


  bool hasKey(string key) {
    return mPrefs.find(key) != mPrefs.end();
  }

  //=============================
  // AddEquivalentArgs: (not implemented yet)
  /* All keys in inEquivalents are considered to be the same as inOption when found on the command line.  inEquivalents are appended to any previous equivalents.  For example, if inOption = "keep" and inEquivalents is { "keep", "dontdiscard" }, then all the following set the value of variable "keep", when found on the command line: {--keep, -keep, --dontdiscard, -dontdiscard }.  Equivalencies can be chained together.  That is, 
     AddEquivalentArgs("arg", {"blah"});
     and
     AddEquivalentArgs("blah", {"blather"}); 
     makes "blather" an equivalent to "arg"!
  */
  // void AddEquivalentArgs(const string &inOption, const vector<string> &inEquivalents); 
  /* append the inEquivalent to the list of equivalent options for inOption */
  // void AddEquivalentArg(const string &inOption, const vector<string> &inEquivalents);


  //========================
  /* values: getting and setting (remember to set dirty bit!) 
     Note that all values are actually saved as strings and are converted, so this is not exactly a high-performance library.  :-) */
  void SetValue( std::string key, string value, bool autoFlags=false) {
    mPrefs[key] = ArgType(key, "string", autoFlags, false, value); 
  }
  void SetLongValue( std::string key, long value, bool autoFlags=false) {
    mPrefs[key] = ArgType(key, "long", autoFlags, false, str(format("%d")%value)); 
  }
  void SetDoubleValue( std::string key, double value, bool autoFlags=false) {
    mPrefs[key] = ArgType(key, "double", autoFlags, false, str(format("%f")%value)); 
  }
  void SetBoolValue( std::string key, bool value, bool autoFlags=false) {
    
    mPrefs[key] = ArgType(key, "bool", autoFlags, false, str(format("%d")%((int)value))); 
  }
  void DeleteValue( std::string key) {
    mPrefs.erase(key); // safe if key does not exist. 
  }

  // The following return false if no such key exists or bad values or an empty string are generated, 
  // They return true if it does and store the value: 
  bool TryGetValue(const std::string &key, std::string &outValue) const;
  bool TryGetLongValue(const std::string &key, long &outValue) const;
  bool TryGetDoubleValue(const std::string &key, double &outValue) const;
  // see below for GetBoolValue()

  /*!
    The following return 0, or "" if no such key exists or bad vals are generated. If dothrow is true, then  throw an exception for bad vals.
  */ 
  std::string GetValue(const  std::string &key, bool dothrow=false) const;
  vector<string> GetValues(const  std::string &key, bool dothrow=false) const;

  template <class T> 
    T GetValue(const std::string &key, bool dothrow, T &outval) const; 

  template <class T> 
    vector<T> GetValues(const std::string &key, bool dothrow, vector<T> &outvals) const; 

  long GetLongValue(const std::string &key, bool dothrow=false)  const;
  double GetDoubleValue(const  std::string &key, bool dothrow=false) const;

  vector<long> GetLongValues(const std::string &key, bool dothrow=false)  const;
  vector<double> GetDoubleValues(const  std::string &key, bool dothrow=false) const;

  ArgType GetArg(string key)  { return mPrefs[key]; }

  std::string operator [] (const std::string &key) const { 
    return GetValue(key); 
  }

  // special case: never throws;  not being defined is the same as being false
  bool GetBoolValue(const std::string &key) { 
    string value = GetValue(key, false); 
    return value == "true" || value == "1" || value == "on"; 
  }

  operator string() ;

 protected:
  std::string NextKey(std::ifstream&theFile);
  std::map<std::string, ArgType > ReadNextSection(std::ifstream &theFile);

  bool KeyValid(std::string key); //true if key is in mValidArgs

  void SaveSectionToFile(std::ofstream &outfile, std::map<std::string, ArgType > &section);

  // there is no copy constructor, so be sure these shallow copy:
  //vector< pair< string, vector<string> > > mEquivalents; 
  std::map<std::string, ArgType >  mPrefs; 
  std::vector<ArgType> mValidArgs; // only save these args or look for them in environment etc. if given
  char _dirty; 
  char _writtenToDisk;
};

#endif
