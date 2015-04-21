/*******************************************************************************
 Configuration parameters from command line

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/08/2001 21:53:31 - First creation

 Ver : 1
*******************************************************************************/

#include <cstring>
#include <cstdio>

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include "xstring.h"
#include "xconf.h"

using namespace bim;

//--------------------------------------------------------------------------------------
// xoperations - a list of pairs of strings: operation,arguments 
// slow but working
//--------------------------------------------------------------------------------------

bool xoperations::contains(const xstring &operation) const {
    xoperations::const_iterator it = this->begin();
    while (it != this->end()) {
        if (it->first == operation) return true;
        ++it;
    }
    return false;
}

xstring xoperations::arguments(const xstring &operation) const {
    xoperations::const_iterator it = this->begin();
    while (it != this->end()) {
        if (it->first == operation) return it->second;
        ++it;
    }
    return "";
}

xoperations xoperations::left(const xstring &operation) const {
    xoperations left;
    xoperations::const_iterator it = this->begin();
    while (it != this->end()) {
        if (it->first == operation) break;
        left.push_back( *it );
        ++it;
    }
    return left;
}

xoperations xoperations::right(const xstring &operation) const {
    xoperations right;
    xoperations::const_reverse_iterator it = this->rbegin();
    while (it != this->rend()) {
        if (it->first == operation) break;
        right.insert(right.begin(), *it);
        ++it;
    }

    return right;
}

//--------------------------------------------------------------------------------------
// XConf
//--------------------------------------------------------------------------------------

void XConf::init() {
  arguments_defs.clear();
  arguments.clear();
}

void XConf::appendArgumentDefinition( const std::string &key, int number_values, const xstring &description ) {
  arguments_defs.insert( make_pair( key, number_values ) );
  arguments_descr[key] = description;
}

//--------------------------------------------------------------------------------------

// this function scans through comma separated strings and appends them to the vector
std::vector<xstring> XConf::scan_strings( char *line ) {
  std::vector<xstring> v;
  xstring s;

  while ( strstr( line, "," ) ) {
    s = line;
    int pos = s.find(",");    
    v.push_back( s.substr(0,pos) );
    line = strstr( line, "," ) + 1;
  }
  if (strlen(line)>0) v.push_back( line );

  return v;
}

#ifdef BIM_USE_CODECVT
int XConf::readParams(int argc, wchar_t** argv) {
    std::vector<char*> argv8(argc);
    std::vector<xstring> argv_utf8(argc);
    for (int i = 0; i < argc; ++i) {
        xstring s(argv[i]);
        argv_utf8[i] = s;
        argv8[i] = &argv_utf8[i][0];
    }
    return this->readParams(argc, &argv8[0]);
}
#endif

int XConf::readParams( int argc, char** argv ) {
  init();
  if (argc<2) return 1;
  int i=0;
  while (i<argc-1) {
    i++;
    xstring key = argv[i];
    key = key.toLowerCase();
    std::map<xstring, int>::const_iterator it = arguments_defs.find( key ); 
    if (it == arguments_defs.end()) continue;
    std::vector<xstring> strs;

    std::map< xstring, std::vector<xstring> >::iterator in_it = arguments.find( key ); 
    if ( in_it == arguments.end() ) {
      arguments.insert( make_pair( key, strs ) ); // in case keys are not repeating
      in_it = arguments.find( key ); 
    }

    int n = (*it).second;
    if (n < 0) {
      i++;
      if (argc-i < 1) break;
      strs = scan_strings( argv[i] );
    } else
    for (int p=0; p<n; p++) {
      i++;
      if (argc-i < 1) break;
      strs.push_back( argv[i] );
    }

    operations.push_back(make_pair(key, strs.size()>0 ? strs[0] : "")); // in case keys are ordered and may be repeating
    for (int x=0; x<strs.size(); x++)  
      in_it->second.push_back( strs[x] );

  } // while (i<argc-1)

  processArguments();
  cureParams();
  return 0;
}

void XConf::print( const std::string &s, int verbose_level ) const {
    if (this->verbose>=verbose_level)
        std::cout << s << std::endl;
}

void XConf::error(const std::string &s) const {
    std::cerr << s << std::endl;
}
void XConf::printElapsed(const std::string &s, int verbose_level) const {
    clock_t t = timerElapsed();
    print(xstring::xprintf("%s %f seconds", s.c_str(), (float)t / CLOCKS_PER_SEC), verbose_level);
}

//--------------------------------------------------------------------------------------

bool XConf::keyExists( const std::string &key ) const {
  std::map<xstring, std::vector<xstring> >::const_iterator it = arguments.find( key ); 
  return (it != arguments.end());
}

std::vector<xstring> XConf::getValues( const std::string &key ) const {
  std::vector<xstring> v;
  std::map<xstring, std::vector<xstring> >::const_iterator it = arguments.find( key ); 
  if (it == arguments.end()) return v;
  return (*it).second;
}

std::vector<int> XConf::getValuesInt( const std::string &key, int def ) const {
  std::vector<int> vi;
  std::vector<xstring> v = getValues( key );
  for (int i=0; i<v.size(); i++)
    vi.push_back( v[i].toInt(def) );
  return vi;
}

std::vector<double> XConf::getValuesDouble( const std::string &key, double def ) const {
  std::vector<double> vi;
  std::vector<xstring> v = getValues( key );
  for (int i=0; i<v.size(); i++)
    vi.push_back( v[i].toDouble(def) );
  return vi;
}

std::vector<xstring> XConf::splitValue( const std::string &key, const std::string &def, const xstring &separator ) const {
  if (!keyExists(key)) return std::vector<xstring>();
  return getValue(key, def).split(separator);
}

std::vector<int> XConf::splitValueInt( const std::string &key, int def, const xstring &separator ) const {
  if (!keyExists(key)) return std::vector<int>();
  std::vector<xstring> v = getValue(key).split(separator);
  std::vector<int> v2;
  for (int i=0; i<v.size(); ++i)
    v2.push_back( v[i].toInt(def) );
  return v2;
}

std::vector<double> XConf::splitValueDouble( const std::string &key, double def, const xstring &separator ) const {
  if (!keyExists(key)) return std::vector<double>();
  std::vector<xstring> v = getValue(key).split(separator);
  std::vector<double> v2;
  for (int i=0; i<v.size(); ++i)
    v2.push_back( v[i].toDouble(def) );
  return v2;
}

xstring XConf::getValue( const std::string &key, const std::string &def ) const {
  xstring v = def;
  std::map<xstring, std::vector<xstring> >::const_iterator it = arguments.find( key ); 
  if (it == arguments.end()) return v;
  std::vector<xstring> strs = (*it).second;
  if (strs.size()<1) return v;
  return strs[0];
}

int XConf::getValueInt( const std::string &key, int def ) const {
  xstring vs = getValue( key, "" );
  return vs.toInt(def);
}

double XConf::getValueDouble( const std::string &key, double def ) const {
  xstring vs = getValue( key, "" );
  return vs.toDouble(def);
}

//--------------------------------------------------------------------------------------

xstring XConf::usage() const {
  xstring str;
  
  int max_key_size = 0;
  std::map<xstring, int>::const_iterator it = arguments_defs.begin(); 
  while (it != arguments_defs.end() ) {
    max_key_size = std::max<int>(max_key_size, it->first.size());
    ++it;
  }
  //xstring key_format = xstring::xprintf( "%%%ds - ", max_key_size);

  it = arguments_defs.begin(); 
  while (it != arguments_defs.end() ) {
    //str += xstring::xprintf( key_format.c_str(), it->first.c_str() );
    xstring k = it->first;
    k.resize(max_key_size, ' ');
    str += k;
    str += " - ";

    std::map<xstring, xstring>::const_iterator itD = arguments_descr.find( it->first ); 
    if (itD != arguments_descr.end()) 
      str += itD->second; 
    else 
      str += "(no description)";

    str += "\n\n";
    ++it;
  }
  return str;
}

//--------------------------------------------------------------------------------------
// EXConf
//--------------------------------------------------------------------------------------

void EXConf::init() {
  XConf::init();
  appendArgumentDefinition( "-i", 1 );
  appendArgumentDefinition( "-o", 1 );
  appendArgumentDefinition( "-v", 0 );
  appendArgumentDefinition( "-par", -1 );
}

void EXConf::processArguments() {
  file_input  = getValue("-i");
  file_output = getValue("-o");
  parameters  = getValuesInt("-par");
}