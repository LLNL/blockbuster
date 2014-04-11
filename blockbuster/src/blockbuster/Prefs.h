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
struct argType {
  argType(string key) {
    argType(key, "string"); 
  }
  argType(string key, string preftype) {
    mKey = key; 
    mType = preftype; 
    mFlags.push_back(str(format("--%1%")%key)); 
    mFlags.push_back(str(format("-%c")%key[0])); 
  }  
  argType(vector<string> flags, string key, string preftype) {
    mKey = key; 
    mType = preftype; 
    mFlags = flags;  
  }
  argType(string longflag, string shortflag, string key, string preftype) {
    mKey = key; 
    mType = preftype; 
    mFlags.push_back(longflag); 
    mFlags.push_back(shortflag); 
  }
  argType(string flag, string key, string preftype) {
    mKey = key; 
    mType = preftype; 
    mFlags.push_back(flag); 
  }

  argType(const argType &other) {
    *this = other; 
  }

  const argType &operator =(const argType &other) {
    mKey = other.mKey; 
    mType = other.mType; 
    mFlags = other.mFlags; 
    return *this; 
  }

  vector<string> mFlags; // aliasable e.g. "--help" and "-h" 
  string mKey, 
    mType; // "bool", "long", "double", "string"
};
    
//======================================================
class Preferences {
 public:
  //========================
  //Initialization
  Preferences(){Reset();}
  Preferences(std::string filename) { Reset(); SetFile(filename);}
  
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
  std::map<std::string, std::string> GetPrefsMap(void) { return mPrefs; }

  //========================
  operator string() const {
    string tmp = string("Preferences: \n")
      + "mFilename: : " + mFilename + "\n" 
      +  "mPrefs:\n"; 
    map<string, string>::const_iterator pos = mPrefs.begin(), end = mPrefs.end();
    while(pos!=end) {
      tmp +=  " (" + pos->first + ", " + pos->second + ") \n";
      ++pos;
    }
    tmp +=  str(boost::format("_dirty: %1%\n_writtenToDisk: %2%\n")%_dirty%_writtenToDisk); ;
    return tmp; 
  }

  //========================
  // meta-attributes
  void SetFile(std::string name){ mFilename = name;}
  std::string GetFilename(void) { return mFilename;}

  // disambiguate self from other prefs in a file:
  void SetLabel(std::string label){ SetValue("prefs_label", label);}
  std::string GetLabel(void) { return GetValue ("prefs_label");}

  //========================
  //saving and restoring from disk
  void SaveToFile(bool createDir=true, bool clobber=true); //open the file, read and remember previous sections, read and discard my section, read and remember sections after me, then write previous section, my section, and trailing sections, then close the file.  Optionally create the needed directory for the file. 
  void SaveToFile(string filename, bool createDir=false) {
    //convenience function
    SetFile(filename); 
    SaveToFile(createDir); 
  }

  void ReadFromFile(bool throw_exceptions=false); //open the file, seek past label, read my section, close file
  void ReadFromFile(string filename, bool throw_exceptions=true) {
    SetFile(filename); 
    ReadFromFile(throw_exceptions); 
  }
  //=============================
  /* Glean the prefs from the command line.  The "types" argument is a vector of argTypes, which tell the flag, the key to assign, and the type, e.g., 
     vector<argType> types; 
     types.push_back("-keep", "keep", "bool"); 
     types.push_back("-longflag", "longkey", "long"); 
     types.push_back("-doubleflag", "doublekey", "double"); 
     types.push_back("-d", "doublekey", "double"); 
     GetFromArgs(argc, argv, types); 

     When done, the options and their arguments will be stripped from argv and argc will be adjusted appropriately.  
  */ 
  
  Preferences &AddArg(argType arg) {
    mValidArgs.push_back(arg); 
    return *this; 
  }
  
  Preferences &AddArgs(std::vector<argType> &args) {
    mValidArgs = args; 
    return *this; 
  }
  
  Preferences &SetValidArgs(std::vector<argType> &args) {
    AddArgs(args); 
    return *this; 
  }
  
  void GetFromArgs(int &argc, char *argv[], vector<argType> &argtypes);

  void ParseArgs(int &argc, char *argv[]); 
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
  template <class T>
  void SetValue(const std::string &key, T value) {
    mPrefs[key] = str(format("%1%")%value); 
  }
  void DeleteValue(const std::string &key) {
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
  long GetLongValue(const  std::string &key, bool dothrow=false)  const;
  double GetDoubleValue(const  std::string &key, bool dothrow=false) const;
  std::string operator [] (const std::string &key) const { return GetValue(key); }

  // special case: never throws;  not being defined is the same as being false
  bool GetBoolValue(const std::string &key) { 
    string value = GetValue(key, false); 
    return value == "true" || value == "1" || value == "on"; 
  }
  
 protected:
  std::string NextKey(std::ifstream&theFile);
  std::map<std::string, std::string> ReadNextSection(std::ifstream &theFile);

  bool KeyValid(std::string key); //true if key is in mValidArgs

  void SaveSectionToFile(std::ofstream &outfile, std::map<std::string, std::string> &section);

  // there is no copy constructor, so be sure these shallow copy:
  //vector< pair< string, vector<string> > > mEquivalents; 
  std::map<std::string, std::string>  mPrefs; 
  std::string mFilename;
  std::vector<argType> mValidArgs; // only save these args or look for them in environment etc. if given
  char _dirty; 
  char _writtenToDisk;
};

#endif
