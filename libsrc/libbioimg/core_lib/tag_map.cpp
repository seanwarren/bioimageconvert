/*******************************************************************************

  Map for tag/value pairs

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
  2008-03-18 17:13 - First creation
  2009-07-08 19:24 - INI parsing, setting values, iterative removing
  2015-07-28 15:44 - Extended values to variants and added types

  ver: 4

  *******************************************************************************/

#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <fstream>

#include "xstring.h"

#include "tag_map.h"

using namespace bim;

//******************************************************************************
// Variant
//******************************************************************************

std::vector<char> Variant::as_vector() const {
    return v;
}

const char * Variant::as_binary() const {
    return &this->v[0];
}

xstring Variant::as_string(const std::string &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("string") && sz>0) {
        const char *cc = &this->v[0];
        if (cc[sz - 1] == 0) return xstring(cc);
        xstring str(sz+1, 0);
        memcpy(&str[0], cc, sz);
        return str;
    } if (tt.startsWith("int")) {
        return xstring::xprintf("%d", this->as_int(0));
    } if (tt.startsWith("unsigned")) {
        return xstring::xprintf("%d", this->as_unsigned(0));
    } if (tt.startsWith("float")) {
        return xstring::xprintf("%f", this->as_float(0));
    } if (tt.startsWith("double")) {
        return xstring::xprintf("%f", this->as_double(0));
    } if (tt.startsWith("boolean")) {
        return this->as_boolean(false) ? "true" : "false";
    } else {
        std::vector<xstring> o;
        for (int i = 0; i < sz; ++i) {
            o.push_back(xstring::xprintf("%X", this->v[i]));
        }
        return xstring::join(o, ",");
    }

    return def;
}

int Variant::as_int(const int &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("int") && sz >= sizeof(int)) {
        int *v = (int *)&this->v[0];
        return *v;
    } else if (tt.startsWith("string")) {
        xstring str = this->as_string("");
        return str.toInt(def);
    }
    return def;
}

unsigned int Variant::as_unsigned(const unsigned int &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("unsigned") && sz >= sizeof(unsigned int)) {
        unsigned int *v = (unsigned int *)&this->v[0];
        return *v;
    } else if (tt.startsWith("string")) {
        xstring str = this->as_string("");
        return str.toInt(def);
    }
    return def;
}

double Variant::as_double(const double &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("double") && sz >= sizeof(double)) {
        double *v = (double *)&this->v[0];
        return *v;
    } else if (tt.startsWith("string")) {
        xstring str = this->as_string("");
        return str.toDouble(def);
    }
    return def;
}

float Variant::as_float(const float &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("float") && sz >= sizeof(float)) {
        float *v = (float *)&this->v[0];
        return *v;
    } else if (tt.startsWith("string")) {
        xstring str = this->as_string("");
        return str.toDouble(def);
    }
    return def;
}

bool Variant::as_boolean(const bool &def) const {
    xstring tt = this->t;
    size_t sz = this->v.size();
    if (tt.startsWith("boolean") && sz >= sizeof(bool)) {
        bool *v = (bool *)&this->v[0];
        return *v;
    } else if (tt.startsWith("string")) {
        xstring str = this->as_string("");
        return str.toLowerCase() == "true";
    }
    return def;
}

TagType Variant::type() const {
    return this->t;
}

unsigned int Variant::size() const {
    return this->v.size();
}

void Variant::set(const TagContainer &value, const TagType &type) {
    this->v = value;
    this->t = type;
}

void Variant::set(const char *value, unsigned int size, const TagType &type) {
    if (!value || size < 1) return;
    this->v.resize(size);
    this->t = type;
    memcpy(&this->v[0], value, size);
}

void Variant::set(const std::string &value, const TagType &type) {
    this->t = type;
    if (value.size() > 0) {
        this->v.resize(value.size()+1);
        this->v[value.size()] = 0;
        memcpy(&this->v[0], value.c_str(), value.size());
    }
}

void Variant::set(const int &value, const TagType &type) {
    this->t = type;
    this->v.resize(sizeof(value));
    memcpy(&this->v[0], &value, sizeof(value));
}

void Variant::set(const unsigned int &value, const TagType &type) {
    this->t = type;
    this->v.resize(sizeof(value));
    memcpy(&this->v[0], &value, sizeof(value));
}

void Variant::set(const double &value, const TagType &type) {
    this->t = type;
    this->v.resize(sizeof(value));
    memcpy(&this->v[0], &value, sizeof(value));
}

void Variant::set(const float &value, const TagType &type) {
    this->t = type;
    this->v.resize(sizeof(value));
    memcpy(&this->v[0], &value, sizeof(value));
}

void Variant::set(const bool &value, const TagType &type) {
    this->t = type;
    this->v.resize(sizeof(value));
    memcpy(&this->v[0], &value, sizeof(value));
}




//******************************************************************************
// TagMap
//******************************************************************************

bool TagMap::keyExists(const std::string &key) const {
    TagMap::const_iterator it = this->find(key);
    return (it != this->end());
}

//******************************************************************************

void TagMap::append_tag(const std::string &key, const Variant &value) {
    this->insert(make_pair(key, value));
}

void TagMap::append_tag(const std::string &key, const char *value, unsigned int size, const TagType &type) {
    this->insert(make_pair(key, Variant(value, size, type)));
}

void TagMap::append_tag(const std::string &key, const std::vector<char> &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const std::string &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const int &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const unsigned int &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const double &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const float &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}

void TagMap::append_tag(const std::string &key, const bool &value, const TagType &type) {
    this->insert(make_pair(key, Variant(value, type)));
}


//******************************************************************************

void TagMap::set_value(const std::string &key, const Variant &value) {
    (*this)[key] = value;
}

void TagMap::set_value(const std::string &key, const std::vector<char> &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const char *value, unsigned int size, const TagType &type) {
    (*this)[key] = Variant(value, size, type);
}

void TagMap::set_value(const std::string &key, const std::string &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const int &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const unsigned int &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const double &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const float &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

void TagMap::set_value(const std::string &key, const bool &value, const TagType &type) {
    (*this)[key] = Variant(value, type);
}

//******************************************************************************

TagType TagMap::get_type(const std::string &key) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).type();
    return TagType();
}

unsigned int TagMap::get_size(const std::string &key) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).size();
    return 0;
}

std::vector<char> TagMap::get_value_vec(const std::string &key) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_vector();
    return std::vector<char>();
}

const char * TagMap::get_value_bin(const std::string &key) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_binary();
    return 0;
}

xstring TagMap::get_value(const std::string &key, const std::string &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_string();
    return def;
}

int TagMap::get_value_int(const std::string &key, const int &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_int(def);
    return def;
}

unsigned int TagMap::get_value_unsigned(const std::string &key, const unsigned int &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_unsigned(def);
    return def;
}

double TagMap::get_value_double(const std::string &key, const double &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_double(def);
    return def;
}

float TagMap::get_value_float(const std::string &key, const float &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_float(def);
    return def;
}

bool TagMap::get_value_bool(const std::string &key, const bool &def) const {
    TagMap::const_iterator it = this->find(key);
    if (it != this->end())
        return ((*it).second).as_boolean(def);
    return def;
}

//******************************************************************************

std::string TagMap::get_key(const std::string &val) const {
    TagMap::const_iterator it = this->begin();
    while (it != this->end()) {
        xstring tt = it->second.type();
        if (tt.startsWith("string") && this->get_value(it->first) == xstring(val)) return it->first;
        ++it;
    }
    return std::string();
}

std::string TagMap::get_key(const int &value) const {
    return get_key(xstring::xprintf("%d", value));
}

std::string TagMap::get_key(const unsigned int &value) const {
    return get_key(xstring::xprintf("%u", value));
}

std::string TagMap::get_key(const double &value) const {
    return get_key(xstring::xprintf("%f", value));
}

std::string TagMap::get_key(const float &value) const {
    return get_key(xstring::xprintf("%f", value));
}

//******************************************************************************
std::string TagMap::get_key_where_value_startsWith(const std::string &val) const {
    TagMap::const_iterator it = this->begin();
    while (it != this->end()) {
        xstring tt = it->second.type();
        if (tt.startsWith("string")) {
            xstring s = this->get_value(it->first);
            if (s.startsWith(val)) return it->first;
        }
        ++it;
    }
    return std::string();
}

std::string TagMap::get_key_where_value_endsWith(const std::string &val) const {
    TagMap::const_iterator it = this->begin();
    while (it != this->end()) {
        xstring tt = it->second.type();
        if (tt.endsWith("string")) {
            xstring s = this->get_value(it->first);
            if (s.startsWith(val)) return it->first;
        }
        ++it;
    }
    return std::string();
}


//******************************************************************************

void TagMap::set_values(const std::map<std::string, TagValue> &tags, const std::string &prefix) {
    TagMap::const_iterator it = tags.begin();
    while (it != tags.end()) {
        set_value(prefix + it->first, it->second);
        it++;
    }
}

void TagMap::append_tags(const std::map<std::string, TagValue> &tags, const std::string &prefix) {
    TagMap::const_iterator it = tags.begin();
    while (it != tags.end()) {
        append_tag(prefix + it->first, it->second);
        it++;
    }
}

//******************************************************************************

std::string TagMap::readline(const std::string &str, int &pos) {
    std::string line;
    std::string::const_iterator it = str.begin() + pos;
    while (it < str.end() && *it != 0xA) {
        if (*it != 0xD)
            line += *it;
        else
            ++pos;
        ++it;
    }
    pos += (int)line.size();
    if (it < str.end() && *it == 0xA) ++pos;
    return line;
}

void TagMap::parse_ini(const std::string &ini, const std::string &separator, const std::string &prefix, const std::string &stop_at_dir) {
    std::string dir;
    int pos = 0;
    while (pos<ini.size()) {
        std::string line = TagMap::readline(ini, pos);
        if (line.size() <= 0) continue;

        // if directory
        if (*line.begin() == '[' && line.size() >= 1 && *(line.end() - 1) == ']') {
            dir = line.substr(1, line.size() - 2);
            if (stop_at_dir.size()>0 && dir == stop_at_dir) return;
            continue;
        }

        // if tag-value pair
        size_t eq_pos = line.find(separator);
        if (eq_pos != std::string::npos) { // found '='
            std::string key = line.substr(0, eq_pos);
            if (dir != "") key = dir + "/" + line.substr(0, eq_pos);
            std::string val = "";
            if (line.size() > eq_pos + 1) val = line.substr(eq_pos + 1);
            if (val.size() > 0 && *val.begin() == '"' && *(val.end() - 1) == '"') val = val.substr(1, val.size() - 2);
            this->set_value(prefix + key, val);
        }

    } // while
}

std::deque<std::string> TagMap::iniGetBlock(const std::string &ini, const std::string &dir_name) {
    std::deque<std::string> d;
    int pos = 0;
    bool found = false;
    while (pos < ini.size()) {
        std::string line = TagMap::readline(ini, pos);
        if (line.size() <= 0) continue;

        if (*line.begin() == '[' && line.size() >= 1 && *(line.end() - 1) == ']') { // if directory
            found = (dir_name == line.substr(1, line.size() - 2));
            continue;
        }

        if (found) d.push_back(line);
    } // while

    return d;
}

//******************************************************************************

void TagMap::delete_tag(const std::string &key) {
    TagMap::iterator it = this->find(key);
    if (it != this->end()) {
        this->erase(it);
    }
}

void TagMap::eraseKeysStaringWith(const std::string &str) {
    TagMap::iterator it = this->begin();
    while (it != this->end()) {
        xstring s = it->first;
        TagMap::iterator toerase = it;
        ++it;
        if (s.startsWith(str))
            this->erase(toerase);
    }
}

//******************************************************************************
std::string TagMap::join(const std::string &line_sep, const std::string &tag_val_sep) const {
    std::string s;
    TagMap::const_iterator it = this->begin();
    while (it != this->end()) {
        s += it->first;
        s += tag_val_sep;
        s += this->get_value(it->first);
        s += line_sep;
        ++it;
    }
    return s;
}


//******************************************************************************
// I/O
//******************************************************************************

bool TagMap::toFile(const std::string &file_name, const std::string &sep) const {
    std::ofstream f;
    f.open(file_name.c_str());
    if (!f.is_open()) return false;
    f << this->join("\n");
    f.close();
    return true;
}

bool TagMap::fromFile(const std::string &file_name, const std::string &sep) {
    std::ifstream f(file_name.c_str());
    if (!f.is_open()) return false;
    while (!f.eof()) {
        std::string line;
        std::getline(f, line);
        this->parse_ini(line, sep);
    }
    f.close();
    return true;
}

bool TagMap::fromString(const std::string &str, const std::string &seplines, const std::string &sepkey) {
    xstring s = str;
    std::vector<xstring> lines = s.split(seplines);
    for (int i = 0; i < lines.size(); ++i) {
        std::vector<xstring> e = lines[i].split(sepkey);
        if (e.size()>1) {
            this->set_value(e[0], e[1]);
        } else if (e.size() > 0) {
            this->set_value(e[0], "");
        }
    }
    return true;
}
