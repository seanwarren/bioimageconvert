/*******************************************************************************
  
  Map for tag/value pairs
    
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2008-03-18 17:13 - First creation
    2009-07-08 19:24 - INI parsing, setting values, iterative removing
      
  ver: 3
        
*******************************************************************************/

#include "tag_map.h"
#include "xstring.h"

#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <fstream>

using namespace bim;

//******************************************************************************

bool TagMap::keyExists( const std::string &key ) const {
  std::map<std::string, std::string>::const_iterator it = this->find( key ); 
  return (it != this->end());
}

//******************************************************************************

void TagMap::append_tag( const std::string &key, const std::string &value ) {
  this->insert( make_pair( key, value ) );
}

void TagMap::append_tag( const std::string &key, const int &value ) {
  append_tag( key, xstring::xprintf("%d", value) );
}

void TagMap::append_tag( const std::string &key, const unsigned int &value ) {
  append_tag( key, xstring::xprintf("%u", value) );
}

void TagMap::append_tag( const std::string &key, const double &value ) {
  append_tag( key, xstring::xprintf("%f", value) );
}

void TagMap::append_tag( const std::string &key, const float &value ) {
  append_tag( key, xstring::xprintf("%f", value) );
}

//******************************************************************************

void TagMap::set_value( const std::string &key, const std::string &value ) {
  (*this)[key] = value;
}

void TagMap::set_value( const std::string &key, const int &value ) {
  set_value( key, xstring::xprintf("%d", value) );
}

void TagMap::set_value( const std::string &key, const unsigned int &value ) {
  set_value( key, xstring::xprintf("%u", value) );
}

void TagMap::set_value( const std::string &key, const double &value ) {
  set_value( key, xstring::xprintf("%f", value) );
}

void TagMap::set_value( const std::string &key, const float &value ) {
  set_value( key, xstring::xprintf("%f", value) );
}

//******************************************************************************

std::string TagMap::get_value( const std::string &key, const std::string &def ) const {
  std::string v = def;
  std::map<std::string, std::string>::const_iterator it = this->find( key ); 
  if (it != this->end() )
    v = (*it).second;
  return v;
}

int TagMap::get_value_int( const std::string &key, const int &def ) const {
  xstring str = this->get_value( key, "" );
  return str.toInt( def );
}

double TagMap::get_value_double( const std::string &key, const double &def ) const {
  xstring str = this->get_value( key, "" );
  return str.toDouble( def );
}

//******************************************************************************

std::string TagMap::get_key( const std::string &val ) const {
  std::map< std::string, std::string >::const_iterator it = this->begin();
  while (it != this->end() ) {
    if ( it->second == val ) return it->first;
    ++it;
  }
  return std::string();
}

std::string TagMap::get_key( const int &value ) const {
  return get_key( xstring::xprintf("%d", value) );
}

std::string TagMap::get_key( const unsigned int &value ) const {
  return get_key( xstring::xprintf("%u", value) );
}

std::string TagMap::get_key( const double &value ) const {
  return get_key( xstring::xprintf("%f", value) );
}

std::string TagMap::get_key( const float &value ) const {
  return get_key( xstring::xprintf("%f", value) );
}

//******************************************************************************
std::string TagMap::get_key_where_value_startsWith( const std::string &val ) const {
  std::map< std::string, std::string >::const_iterator it = this->begin();
  while (it != this->end() ) {
    xstring s = it->second;
    if ( s.startsWith(val) ) return it->first;
    ++it;
  }
  return std::string();
}

std::string TagMap::get_key_where_value_endsWith( const std::string &val ) const {
  std::map< std::string, std::string >::const_iterator it = this->begin();
  while (it != this->end() ) {
    xstring s = it->second;
    if ( s.endsWith(val) ) return it->first;
    ++it;
  }
  return std::string();
}


//******************************************************************************

void TagMap::set_values( const std::map<std::string, std::string> &tags, const std::string &prefix ) {
  std::map< std::string, std::string >::const_iterator it = tags.begin();
  while (it != tags.end()) {
    set_value( prefix + it->first, it->second );
    it++;
  }
}

void TagMap::append_tags( const std::map<std::string, std::string> &tags, const std::string &prefix ) {
  std::map< std::string, std::string >::const_iterator it = tags.begin();
  while (it != tags.end()) {
    append_tag( prefix + it->first, it->second );
    it++;
  }
}

//******************************************************************************

std::string TagMap::readline( const std::string &str, int &pos ) {
  std::string line;
  std::string::const_iterator it = str.begin() + pos;
  while (it<str.end() && *it != 0xA ) {
    if (*it != 0xD) 
      line += *it;
    else
      ++pos;
    ++it;
  }
  pos += (int) line.size();
  if (it<str.end() && *it == 0xA) ++pos;
  return line;
}

void TagMap::parse_ini( const std::string &ini, const std::string &separator, const std::string &prefix, const std::string &stop_at_dir ) {
  std::string dir;
  int pos=0;
  while (pos<ini.size()) {
    std::string line = TagMap::readline( ini, pos );
    if (line.size()<=0) continue;

    // if directory
    if (*line.begin()=='[' && line.size()>=1 && *(line.end()-1)==']') {
      dir = line.substr( 1, line.size()-2);
      if (stop_at_dir.size()>0 && dir == stop_at_dir) return;
      continue;
    }

    // if tag-value pair
    size_t eq_pos = line.find(separator);
    if (eq_pos != std::string::npos ) { // found '='
      std::string key = line.substr( 0, eq_pos );
      if (dir!="") key = dir + "/" + line.substr( 0, eq_pos );
      std::string val = "";
      if (line.size()>eq_pos+1) val = line.substr( eq_pos+1 );
      if ( val.size()>0 && *val.begin() == '"' && *(val.end()-1) == '"' ) val = val.substr( 1, val.size()-2);
      (*this)[prefix+key] = val; 
    }

  } // while
}

std::deque<std::string> TagMap::iniGetBlock( const std::string &ini, const std::string &dir_name ) {
  std::deque<std::string> d;
  int pos=0;
  bool found=false;
  while (pos<ini.size()) {
    std::string line = TagMap::readline( ini, pos );
    if (line.size()<=0) continue;

    if (*line.begin()=='[' && line.size()>=1 && *(line.end()-1)==']') { // if directory
      found = (dir_name == line.substr( 1, line.size()-2));
      continue;
    }
    
    if (found) d.push_back(line);
  } // while

  return d;
}

//******************************************************************************
void TagMap::eraseKeysStaringWith( const std::string &str ) {
  std::map< std::string, std::string >::iterator it = this->begin();
  while (it != this->end()) {
    xstring s = it->first;
    std::map< std::string, std::string >::iterator toerase = it;
    ++it;
    if ( s.startsWith(str) )
      this->erase(toerase);
  }
}

//******************************************************************************
std::string TagMap::join( const std::string &line_sep, const std::string &tag_val_sep  ) const {
  std::string s;
  std::map< std::string, std::string >::const_iterator it = this->begin();
  while (it != this->end()) {
    s += it->first;
    s += tag_val_sep;
    s += it->second;
    s += line_sep;
    ++it;
  }
  return s;
}


//******************************************************************************
// I/O
//******************************************************************************

bool TagMap::toFile( const std::string &file_name, const std::string &sep ) const {
  std::ofstream f;
  f.open(file_name.c_str());
  if (!f.is_open()) return false;
  f << this->join("\n");
  f.close();
  return true;
}

bool TagMap::fromFile( const std::string &file_name, const std::string &sep ) {
  std::ifstream f(file_name.c_str());
  if (!f.is_open()) return false;
  while (!f.eof()) {
    std::string line;
    std::getline(f, line);
    this->parse_ini( line, sep );
  }
  f.close();
  return true;
}

