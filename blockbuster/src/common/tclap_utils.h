#ifndef TCLAP_UTILS_H
#define TCLAP_UTILS_H 1 
#include <vector>
#include <tclap/CmdLine.h>
#include <iterator>

template <class T>
struct VectFromString {
  VectFromString() { valid = false; expectedElems = 0; }

  std::vector<T> elems;
  // operator= will be used to assign to the vector
  VectFromString operator=(const std::string &str)
  {
    std::istringstream iss(str);
    T value; 
    while (iss >> value) {
      elems.push_back(value); 
    }
    if (!elems.size() || (expectedElems != 0 && elems.size() != expectedElems))
      throw TCLAP::ArgParseException(str + " is not an appropriate list of the expected items");
    valid = true; 
    return *this;
  }
  
  T operator [] (int i) const {return elems[i]; }

  std::ostream& print(std::ostream &os) const
  {
	std::copy(elems.begin(), elems.end(), std::ostream_iterator<double>(os, " "));
	return os;
  }

  bool valid; 
  int expectedElems; 
};

// Create an ArgTraits for the Vect4 type that declares it to be
// of string like type, so TCLAP can assign a string to it
namespace TCLAP {
  template<class T>
  struct ArgTraits<VectFromString<T> > {
    typedef StringLike ValueCategory;
  };
}

#endif
