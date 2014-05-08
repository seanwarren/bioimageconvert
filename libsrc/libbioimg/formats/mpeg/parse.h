#ifndef PARSE_H
#define PARSE_H

// $Date: 2009-02-11 00:54:06 -0500 (Wed, 11 Feb 2009) $
// $Revision: 719 $

/*
videoIO: granting easy, flexible, and efficient read/write access to video 
files in Matlab on Windows and GNU/Linux platforms.

Copyright (c) 2006 Gerald Dalley

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

Portions of this software link to code licensed under the Gnu General 
Public License (GPL).  As such, they must be licensed by the more 
restrictive GPL license rather than this MIT license.  If you compile 
those files, this library and any code of yours that uses it automatically
becomes subject to the GPL conditions.  Any source files supplied by 
this library that bear this restriction are clearly marked with internal
comments.

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#include <map>
#include <set>
#include <string>
#include <sstream>
#include <string.h>
#include <math.h>
#include "debug.h"

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
inline int strcasecmp(const char *s1, const char *s2) { return _stricmp(s1, s2); }
#endif

namespace VideoIO
{
  struct StrCaseCmp {
    bool operator()(std::string const &a, std::string const &b) const {
      return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
  };

  template<typename T>
  static std::string toString(T const &x) {
    std::ostringstream o;
    if (!(o << x)) VrRecoverableThrow("Could not convert value to a string");
    return o.str();
  }

  // This is a slightly-enhanced string->string mapping.  It uses case
  // insensitive key comparisons and supports basic parsing of numeric
  // types for the values.  It also tracks which keys have been examined.
  class KeyValueMap : public std::map<std::string, std::string, StrCaseCmp> 
  {
  public:
    typedef std::map<std::string, std::string, StrCaseCmp> Super;

    inline iterator find(std::string const &key) {
      checkedKeys.insert(key);
      return Super::find(key);
    }

    // Checks to see if the has a mapping.  Updates checkedKeys.
    inline bool hasKey(const char *key) {
      TRACE;
      KeyValueMap::const_iterator iter = find(key);
      return (iter != end() && iter->second.size() > 0);
    }

    // Checks to see if the has a mapping.  Updates checkedKeys.
    inline bool hasKeyEvenIfEmptyValue(const char *key) {
      TRACE;
      KeyValueMap::const_iterator iter = find(key);
      return (iter != end());
    }

    // Returns the value associated with the key, as an integer.
    // If no mapping exists or if the associated string value cannot be parsed 
    // as an integer, an exception is thrown.  Updates checkedKeys.
    template <class T>
    inline T parseInt(const char *key) { 
      TRACE;
      KeyValueMap::const_iterator iter = find(key);
      VrRecoverableCheckMsg(iter != end(),
        "No mapping exists for " << key);
      char const *str = iter->second.c_str();
      char *errLoc = NULL;
      errno = 0;
      long int tmpVal = strtol(str, &errLoc, 0);
      VrRecoverableCheckMsg(errno != EINVAL,  
        "Could not convert " << key << "'s value of " << str << 
        " to an integer."); 
      VrRecoverableCheckMsg(errno != ERANGE,  
        "Out-of-range error when attempting to convert " << key << 
        "'s value of " << str << " to an integer.");
      VrRecoverableCheckMsg(*errLoc == '\0',  
        "Parse error at character " << errLoc - str << 
        " when attempting to convert " << key << "'s value of " << 
        str << " to an integer.");
      return (T)tmpVal;
    }

    // Returns the value associated with the key, as an floating point number.
    // If no mapping exists or if the associated string value cannot be parsed 
    // as an float, an exception is thrown.  Updates checkedKeys.
    template <class T>
    inline T parseFloat(const char *key) {
      TRACE;
      KeyValueMap::const_iterator iter = find(key);
      VrRecoverableCheckMsg(iter != end(),
        "No mapping exists for " << key);
      char const *str = iter->second.c_str();
      char *endLoc = NULL;
      errno = 0;
      double tmpVal = strtod(str, &endLoc);
      VrRecoverableCheckMsg(strlen(str) == (endLoc-str),  
        "Invalid character at position " << (endLoc - str) << " in \"" <<
        str << "\" when attempting to extract a double value.");
      VrRecoverableCheckMsg(!((tmpVal == HUGE_VAL) && (errno == ERANGE)), 
        "Overflow error in parsing \"" << str << "\".");
      VrRecoverableCheckMsg(!((tmpVal == 0) && (errno == ERANGE)), 
        "Underflow error in parsing \"" << str << "\".");
      return (T)tmpVal;
    }

    // Sometimes we want to require that arguments come in pairs, for
    // example if "fpsNum" is specified, we also need "fpsDenom".  This
    // function does both-or-nothing parsing.
    template <class I>
    bool intPairParse(const char *n1, I &i1, const char *n2, I &i2) {
      const int nargs = hasKey(n1) + hasKey(n2);
      VrRecoverableCheckMsg(nargs == 2 || nargs == 0, 
        "Either both or neither of \"" << n1 << "\" and \"" << n2 << 
        "\" must be supplied.");
      if (nargs == 2) {
        i1 = parseInt<I>(n1);
        i2 = parseInt<I>(n2);
        return true;
      }
      return false;
    };

    // allows NTSC (29.97) to be represented (almost) exactly
    static const int DEFAULT_FPS_DENOM = 1000000; // ffmpeg's favorite denom

    // Custom parser for all the different ways of specifying a frames
    // per second value.
    bool fpsParse(int &num, int &denom) {
      TRACE;
      if (hasKey("framesPerSecond")) {
        denom = DEFAULT_FPS_DENOM;
        num   = (int)(parseFloat<double>("framesPerSecond") * denom);
        return true;
      }
      if (hasKey("fps")) {
        denom = DEFAULT_FPS_DENOM;
        num   = (int)(parseFloat<double>("fps") * denom);
        return true;
      }
      if (intPairParse("framesPerSecond_num", num,
                       "framesPerSecond_denom", denom)) {
        return true;
      }
      if (intPairParse("fpsNum",num, "fpsDenom",denom)) {
        return true;
      }
      return false;
    }

    // Sometimes it's useful to keep track of all the keys that have been
    // checked so we can later determine if there are any mappings that have
    // not been examined.  Here is the list of keys that have been checked
    // by a call to hasKey or any of the above parsing methods.
    std::set<std::string> getUncheckedKeys() const {
      std::set<std::string> unchecked;
      for (const_iterator i=begin(); i!=end(); i++) {
        if (checkedKeys.find(i->first) == checkedKeys.end()) {
          unchecked.insert(i->first);
        }
      }
      return unchecked;
    }

    void alertUncheckedKeys(char const *msg) const {
      typedef std::set<std::string> S;
      S const unchecked = getUncheckedKeys();
      if (!unchecked.empty()) {
        std::stringstream errMsg;
        errMsg << msg;

        S::const_iterator i=unchecked.begin();
        errMsg << "\"" << *i << "\"";
        ++i;

        for (; i!=unchecked.end(); ++i) {
          errMsg << ", \"" << *i << "\"";
        }

        VrRecoverableThrow(errMsg.str());
      }
    }

    std::set<std::string> const &getCheckedKeys() const { return checkedKeys; }

    void resetCheckedKeys() { checkedKeys.clear(); }

  private:
    std::set<std::string> checkedKeys;

    inline const_iterator find(std::string const &key) const {
      VrRecoverableThrow("this method has been hidden");
    }


  }; /* namespace VideoIO */

};

#endif
