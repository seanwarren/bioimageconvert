/*****************************************************************************
 Extended String Class

 IMPLEMENTATION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   12/05/2005 23:38 - First creation

 Ver : 7
*****************************************************************************/

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "xstring.h"

#ifdef BIM_USE_CODECVT
#include <codecvt>
#endif

const int MAX_STR_SIZE = 1024*100;

#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #define HAVE_SECURE_C
  #pragma message(">>>>>  xstring: using secure c libraries")
#endif 

using namespace bim;

//******************************************************************************

xstring &xstring::sprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  sprintf( fmt, ap );
  va_end(ap);
  return *this;
}

xstring &xstring::saddf(const char *fmt, ...) {
  xstring result;
  va_list ap;
  va_start(ap, fmt);
  result.sprintf( fmt, ap );
  va_end(ap);
  *this += result;
  return ( *this );
}

xstring xstring::xprintf(const char *fmt, ...) {
  xstring result;
  va_list ap;
  va_start(ap, fmt);
  result.sprintf( fmt, ap );
  va_end(ap);
  return result;
}

xstring &xstring::sprintf( const char *format, va_list ap ) {
    std::vector<char> cbuf(MAX_STR_SIZE);
    #ifdef HAVE_SECURE_C  
    int w = vsprintf_s((char *)&cbuf[0], MAX_STR_SIZE, format, ap);
    #else
    //vsprintf((char *) cbuf, format, ap);
    int w = vsnprintf((char *) &cbuf[0], MAX_STR_SIZE, format, ap);
    #endif
    if (w > MAX_STR_SIZE) {
        cbuf.resize(w);
        #ifdef HAVE_SECURE_C  
        vsprintf_s((char *)&cbuf[0], w, format, ap);
        #else
        //vsprintf((char *) cbuf, format, ap);
        vsnprintf((char *)&cbuf[0], w, format, ap);
        #endif
    }

    xstring result = &cbuf[0];
    *this = result;
    return *this;
}

//******************************************************************************

xstring &xstring::insertAfterLast(const char *p, const xstring &s) {
  unsigned int sp = (unsigned int) this->rfind(p);
  if (sp != std::string::npos)
    this->insert(sp, s);
  return *this;
}

//******************************************************************************

// strips trailing chars
xstring &xstring::rstrip(const xstring &chars) {
  size_t p = this->find_last_not_of(chars);
  if (p != std::string::npos) {
    this->resize(p+1, ' ');
  } else { // if the string is only composed of those chars
    for (unsigned int i=0; i<chars.size(); ++i)
      if ((*this)[0]==chars[i]) { this->resize(0); break; }
  }
  return *this;
}

// strips leading chars
xstring &xstring::lstrip(const xstring &chars) {
  size_t p = this->find_first_not_of(chars);
  if (p != std::string::npos) {
    *this = this->substr( p, std::string::npos );
  } else { // if the string is only composed of those chars
    for (unsigned int i=0; i<chars.size(); ++i)
      if ((*this)[0]==chars[i]) { this->resize(0); break; }
  }
  return *this;
}

// strips leading and trailing chars
xstring &xstring::strip(const xstring &chars) {
  rstrip(chars);
  lstrip(chars);
  return *this;
}

bool is_zero(int i) { return i == 0; }

xstring &xstring::erase_zeros() {
    this->erase(std::remove_if(this->begin(), this->end(), is_zero), this->end() );
    return *this;
}

//******************************************************************************

template<typename RT, typename T, typename Trait, typename Alloc>
RT ss_atoi( const std::basic_string<T, Trait, Alloc>& the_string, RT def ) {
  std::basic_istringstream< T, Trait, Alloc> temp_ss(the_string);
  RT num;
  try {
    temp_ss.exceptions(std::ios::badbit | std::ios::failbit);
    temp_ss >> num;
  } 
  catch(std::ios_base::failure e) {
    return def;
  }
  return num;
}

int xstring::toInt(int def) const {
  return ss_atoi<int>(*this, def);
}

double xstring::toDouble(double def) const {
  return ss_atoi<double>(*this, def);
}

// converts from either numeric 0 or 1 or from textual 'true' or 'false'
bool xstring::toBool(bool def) const {
    if (this->toLowerCase() == "true") return true;
    if (this->toLowerCase() == "false") return false;
    if (this->toInt(0) == 0) return false;
    return true;
}

//******************************************************************************

bool xstring::operator==(const xstring &other) const {
  return ( size() == other.size()) && (memcmp( this->c_str(), other.c_str(), size() )==0);
}

bool xstring::operator==(const char *s) const {
  xstring other = s;
  return ( size() == other.size()) && (memcmp( this->c_str(), other.c_str(), size() )==0);
}

int xstring::compare(const xstring &s) const {
  return strncmp( this->c_str(), s.c_str(), std::min(this->size(), s.size()) );
}

//******************************************************************************

bool xstring::startsWith(const xstring &s) const {
  //if (s.size()>this->size()) return false;
  //return (this->substr( 0, s.size() ) == s);
  return ( strncmp( this->c_str(), s.c_str(), std::min(this->size(), s.size()) ) == 0);
}

bool xstring::endsWith(const xstring &s) const {
  if (s.size()>this->size()) return false;
  return (this->substr( this->size()-s.size(), s.size() ) == s);
}


//******************************************************************************
std::vector<xstring> xstring::split( const xstring &s ) const {

  std::vector<xstring> list;
  xstring part;

  std::string::size_type start = 0;
  std::string::size_type end;

  while (1) {
    end = this->find ( s, start );
    if (end == std::string::npos) {
      part = this->substr( start );
      list.push_back( part );
      break;
    } else {
      part = this->substr( start, end-start );
      start = end + s.size();
      list.push_back( part );
    }
  }
 
  return list;
}

std::vector<int> xstring::splitInt( const xstring &separator, const int &def ) const {
    std::vector<xstring> v = this->split(separator);
    std::vector<int> v2;
    for (int i=0; i<v.size(); ++i)
        v2.push_back( v[i].toInt(def) );
    return v2;
}

std::vector<double> xstring::splitDouble( const xstring &separator, const double &def ) const {
    std::vector<xstring> v = this->split(separator);
    std::vector<double> v2;
    for (int i=0; i<v.size(); ++i)
        v2.push_back( v[i].toDouble(def) );
    return v2;
}

//******************************************************************************

xstring xstring::join(std::vector<xstring> v, const xstring &separator) {
    if (v.size() == 0) return "";
    xstring s(v[0]);
    for (int i = 1; i < v.size(); ++i) {
        s += separator;
        s += v[i];
    }
    return s;
}

xstring xstring::join(std::vector<int> v, const xstring &separator) {
    std::vector<xstring> vv(v.size());
    for (int i = 0; i < v.size(); ++i) {
        vv[i] = xstring::xprintf("%d", v[i]);
    }
    return xstring::join(vv, separator);
}

xstring xstring::join(std::vector<double> v, const xstring &separator) {
    std::vector<xstring> vv(v.size());
    for (int i = 0; i < v.size(); ++i) {
        vv[i] = xstring::xprintf("%f", v[i]);
    }
    return xstring::join(vv, separator);
}


//******************************************************************************
std::string xstring::toLowerCase() const {
  std::string s = *this;
  std::transform(s.begin(), s.end(), s.begin(), tolower);
  return s;
}

std::string xstring::toUpperCase() const {
  std::string s = *this;
  std::transform(s.begin(), s.end(), s.begin(), toupper);
  return s;
}

//******************************************************************************
xstring xstring::left(std::string::size_type pos) const {
  return this->substr( 0, pos );
}

xstring xstring::left(const xstring &sub) const {
  std::string::size_type p = this->find(sub);
  if (p == std::string::npos) return xstring();
  return this->left(p);
}

xstring xstring::right(std::string::size_type pos) const {
  return this->substr( pos, std::string::npos );
}

xstring xstring::right(const xstring &sub) const {
  std::string::size_type p = this->rfind(sub);
  if (p == std::string::npos) return xstring();
  return this->right(p+sub.size());
}

xstring xstring::section(std::string::size_type start, std::string::size_type end) const {
  return this->substr( start, end-start );
}

xstring xstring::section(const xstring &start, const xstring &end, std::string::size_type pos) const {
  std::string::size_type s = this->find(start, pos);
  if (s == std::string::npos) return xstring(); else s+=start.size();
  std::string::size_type e = this->find(end, s);  
  if (e == std::string::npos) return xstring(); else e;
  return this->section(s, e);
}

//******************************************************************************
bool xstring::contains ( const xstring &str ) const {
  std::string::size_type loc = this->find( str, 0 );
  return loc != std::string::npos;
}

//******************************************************************************
xstring xstring::replace( const xstring &what, const xstring &with ) const {
  std::string::size_type p = this->find( what, 0 );
  if (p == std::string::npos) return *this;
  xstring r = *this;
  while (p != std::string::npos) {
    r = r.replace( p, what.size(), with.c_str() );
    p = r.find( what, p+with.size() );
  }
  return r;
}

// replaces variables encased in {} with the values from the map
xstring xstring::processTemplate(const std::map<std::string, std::string> &vars) const {
    xstring o = *this;
    std::map< std::string, std::string >::const_iterator it = vars.begin();
    while (it != vars.end()) {
        xstring k = xstring::xprintf("{%s}", it->first.c_str());
        xstring v = it->second;
        o = o.replace(k, v);
        ++it;
    }
    return o;
}


//******************************************************************************
// I/O
//******************************************************************************

bool xstring::toFile( const xstring &file_name ) const {
  std::ofstream f;
  f.open(file_name.c_str());
  if (!f.is_open()) return false;
  f << *this;
  f.close();
  return true;
}

bool xstring::fromFile( const xstring &file_name ) {
  std::ifstream f(file_name.c_str());
  if (!f.is_open()) return false;
  while (!f.eof()) {
    std::string line;
    std::getline(f, line);
    this->append(line);
    this->append("\n");
  }
  f.close();
  return true;
}

//******************************************************************************
// character encoding
//******************************************************************************

#ifdef BIM_USE_CODECVT

std::wstring xstring::toUTF16() const {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(*this);
}

void xstring::fromUTF16(const std::wstring &utf16) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    *this = converter.to_bytes(utf16);
}

/*
std::string xstring::toLatin1() const {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(*this);
}

void xstring::fromLatin1(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    *this = converter.to_bytes(utf16);
}*/

#endif

