/*****************************************************************************
 Extended String Class

 DEFINITION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   12/05/2005 23:38 - First creation

 Ver : 7
*****************************************************************************/

#ifndef XSTRING_H
#define XSTRING_H
//#pragma message(">>>>>  xstring: included enhanced string")

#include <cstdarg>
#include <string>
#include <vector>
#include <map>

#include "xtypes.h"

#ifdef BIM_WIN
#define BIM_USE_CODECVT
#endif

namespace bim {

class xstring: public std::string {
public:
  // constructors
  explicit xstring(): std::string() {}
  xstring( const char *cstr ): std::string() { *this += cstr; }
#ifdef BIM_USE_CODECVT
  xstring(const std::wstring &utf16) : std::string() { this->fromUTF16(utf16); }
#endif

  /*
  xstring( const value_type* _Ptr, 
           size_type _Count = npos ) : std::string( _Ptr, _Count ) {}
  */
  xstring( const std::string& _Right, 
           size_type _Roff = 0, 
           size_type _Count = npos ) : std::string( _Right, _Roff, _Count ) {}

  xstring( size_type _Count,
           value_type _Ch ) : std::string( _Count, _Ch ) {}

  template <class InputIterator >
  xstring( InputIterator _First, 
           InputIterator _Last ) : std::string( _First, _Last ) {}

  // members
  xstring &sprintf(const char *fmt, ...);
  xstring &saddf(const char *fmt, ...);
  xstring &insertAfterLast(const char *p, const xstring &s);

  static xstring xprintf(const char *fmt, ...);

  xstring rstrip(const xstring &chars) const; // strips trailing chars
  xstring lstrip(const xstring &chars) const; // strips leading chars
  xstring strip(const xstring &chars) const; // strips leading and trailing chars

  xstring erase_zeros() const; // removes all 0 chars from the string

  xstring removeSpacesLeft() const { return lstrip(" "); }
  xstring removeSpacesRight() const { return rstrip(" "); }
  xstring removeSpacesBoth() const { return strip(" "); }

  bool   toBool(bool def = false) const; // converts from either numeric 0 or 1 or from textual 'true' or 'false'
  int    toInt( int def = 0 ) const;
  double toDouble(double def = 0.0) const;

  xstring toLowerCase() const;
  xstring toUpperCase() const;

  bool operator==(const xstring &s) const;
  bool operator==(const char *s) const;
  bool operator!=(const xstring &s) const { return !(*this == s); }
  bool operator!=(const char *s) const { return !(*this == s); }

  bool startsWith(const xstring &s) const;
  bool endsWith(const xstring &s) const;

  int compare(const xstring &s) const;
  static inline int compare(const xstring &s1, const xstring &s2) { return s1.compare(s2); }

  std::vector<xstring> split( const xstring &separator, const bool &ignore_empty = true ) const;
  std::vector<int>     splitInt( const xstring &separator, const int &def=0 ) const;
  std::vector<double>  splitDouble( const xstring &separator, const double &def=0.0 ) const;

  static xstring join(std::vector<xstring> v, const xstring &separator);
  static xstring join(std::vector<int> v,     const xstring &separator);
  static xstring join(std::vector<double> v,  const xstring &separator);
  static xstring join(std::vector<unsigned char> v, const xstring &separator);

  xstring left(std::string::size_type pos) const;
  xstring left(const xstring &sub) const;
  xstring right(std::string::size_type pos) const;
  xstring right(const xstring &sub) const;
  xstring section(std::string::size_type start, std::string::size_type end) const;
  xstring section(const xstring &start, const xstring &end, std::string::size_type pos=0) const;

  bool contains ( const xstring &str ) const;

  using std::string::replace;
  xstring replace( const xstring &what, const xstring &with ) const;

  // replaces variables encased in {} with the values from the map
  xstring processTemplate( const std::map<std::string, std::string> &vars) const;

  // I/O
  bool toFile( const xstring &file_name ) const;
  bool fromFile( const xstring &file_name );

  // the xstring holds utf8 encoded text
#ifdef BIM_USE_CODECVT
  std::wstring toUTF16() const;
  void fromUTF16(const std::wstring &utf16);
  
  //std::string toLatin1() const;
  //void fromLatin1(const std::string &str);
#endif

private:
  xstring &sprintf( const char *format, va_list ap );  
};

} // namespace bim

#endif // XSTRING_H
