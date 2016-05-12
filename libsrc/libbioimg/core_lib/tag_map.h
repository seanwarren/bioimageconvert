/*******************************************************************************

  Map for tag/value pairs

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  Description:

  Mapping between tag names and their values
  Names: are any string with possible hierarchy encoded with the "/" slash character
  Values: are variant type typically storing strings, but possibly vectors, numbers, etc...

  types: indicate basic storage of the element and may be followed by additional 
  user-given type info, multiple types are comma separated
  basic types: binary|string|int|unsigned|double|float|boolean
  example of a user defined type: 
      "string,dicom:name:8082"
      "double,microns"

  History:
  2008-03-18 17:13 - First creation
  2009-07-08 19:24 - INI parsing, setting values, iterative removing
  2015-07-28 15:44 - Extended values to variants and added types

  ver: 4

  *******************************************************************************/

#ifndef BIM_TAG_MAP_H
#define BIM_TAG_MAP_H

#include <string>
#include <map>
#include <deque>

#include "xstring.h"

namespace bim {

    typedef std::vector<char> TagContainer;
    typedef std::string TagType;

    class Variant {
    public:
        explicit Variant() { }
        Variant(const Variant& _Right) { *this = _Right; }

        Variant(const TagContainer &value, const TagType &_type = "binary") { this->set(value, _type); }
        Variant(const char *value, unsigned int size, const TagType &_type = "binary") { this->set(value, size, _type); }
        Variant(const std::string &value, const TagType &_type = "string") { this->set(value, _type); }
        Variant(const int &value, const TagType &_type = "int") { this->set(value, _type); }
        Variant(const unsigned int &value, const TagType &_type = "unsigned") { this->set(value, _type); }
        Variant(const double &value, const TagType &_type = "double") { this->set(value, _type); }
        Variant(const float &value, const TagType &_type = "float") { this->set(value, _type); }
        Variant(const bool &value, const TagType &_type = "boolean") { this->set(value, _type); }

        TagContainer       as_vector() const;
        const char *       as_binary() const;
        xstring            as_string(const std::string &def = "") const;
        int                as_int(const int &def) const;
        unsigned int       as_unsigned(const unsigned int &def) const;
        double             as_double(const double &def) const;
        float              as_float(const float &def) const;
        bool               as_boolean(const bool &def) const;

        TagType            type() const;
        unsigned int       size() const;

        void set( const TagContainer &value, const TagType &type = "binary");
        void set( const char *value, unsigned int size, const TagType &type = "binary");
        void set( const std::string &value, const TagType &type = "string");
        void set( const int &value, const TagType &type = "int");
        void set( const unsigned int &value, const TagType &type = "unsigned");
        void set( const double &value, const TagType &type = "double");
        void set( const float &value, const TagType &type = "float");
        void set( const bool &value, const TagType &type = "boolean");

    private:
        TagContainer v;
        TagType t;
    };


    typedef Variant TagValue;
    class TagMap : public std::map<std::string, TagValue> {
    public:
        typedef std::map<std::string, TagValue >::const_iterator const_iterator;
        typedef std::map<std::string, TagValue >::iterator iterator;
        typedef std::map<std::string, TagValue >::const_reverse_iterator const_reverse_iterator;
        typedef std::map<std::string, TagValue >::reverse_iterator reverse_iterator;

    public:
        // constructors
        explicit TagMap() : std::map<std::string, TagValue>() {}
        TagMap(const TagMap& _Right) : std::map<std::string, TagValue>(_Right) {}
        TagMap(const std::map<std::string, TagValue>& _Right) : std::map<std::string, TagValue>(_Right) {}

        TagMap(const std::string &str, const std::string &seplines = ",", const std::string &sepkey = " ") { this->fromString(str, seplines, sepkey); }

        template<class InputIterator>
        TagMap(InputIterator _First,
            InputIterator _Last) : std::map<std::string, TagValue>(_First, _Last) {}

        bool keyExists(const std::string &key) const;
        bool hasKey(const std::string &key) const { return keyExists(key); }


        // additional methods
        xstring           get_value(const std::string &key, const std::string &def = "") const;
        std::vector<char> get_value_vec(const std::string &key) const;
        const char *      get_value_bin(const std::string &key) const;
        int               get_value_int(const std::string &key, const int &def) const;
        unsigned int      get_value_unsigned(const std::string &key, const unsigned int &def) const;
        double            get_value_double(const std::string &key, const double &def) const;
        float             get_value_float(const std::string &key, const float &def) const;
        bool              get_value_bool(const std::string &key, const bool &def) const;

        TagType           get_type(const std::string &key) const;
        unsigned int      get_size(const std::string &key) const;

        std::string get_key(const std::vector<char> &val) const;
        std::string get_key(const std::string &val) const;
        std::string get_key(const int &value) const;
        std::string get_key(const unsigned int &value) const;
        std::string get_key(const double &value) const;
        std::string get_key(const float &value) const;

        std::string get_key_where_value_startsWith(const std::string &val) const;
        std::string get_key_where_value_endsWith(const std::string &val) const;

        // Sets the value of a tag, if the key exists its value is modified
        void set_value(const std::string &key, const Variant &value);
        void set_value(const std::string &key, const std::vector<char> &value, const TagType &type = "binary");
        void set_value(const std::string &key, const char *value, unsigned int size, const TagType &type = "binary");
        void set_value(const std::string &key, const char *value, const TagType &type = "string");
        void set_value(const std::string &key, const std::string &value, const TagType &type = "string");
        void set_value(const std::string &key, const int &value, const TagType &type = "int");
        void set_value(const std::string &key, const unsigned int &value, const TagType &type = "unsigned");
        void set_value(const std::string &key, const double &value, const TagType &type = "double");
        void set_value(const std::string &key, const float &value, const TagType &type = "float");
        void set_value(const std::string &key, const bool &value, const TagType &type = "boolean");

        // Appends a tag if the key does not exist yet
        void append_tag(const std::string &key, const Variant &value);
        void append_tag(const std::string &key, const std::vector<char> &value, const TagType &type = "binary");
        void append_tag(const std::string &key, const char *value, unsigned int size, const TagType &type = "binary");
        void append_tag(const std::string &key, const char *value, const TagType &type = "string");
        void append_tag(const std::string &key, const std::string &value, const TagType &type = "string");
        void append_tag(const std::string &key, const int &value, const TagType &type = "int");
        void append_tag(const std::string &key, const unsigned int &value, const TagType &type = "unsigned");
        void append_tag(const std::string &key, const double &value, const TagType &type = "double");
        void append_tag(const std::string &key, const float &value, const TagType &type = "float");
        void append_tag(const std::string &key, const bool &value, const TagType &type = "boolean");

        void delete_tag(const std::string &key);
        void erase_tag(const std::string &key) { this->delete_tag(key); };

        void eraseKeysStaringWith(const std::string &str);

        // set tags from another hashtable using a prefix for a tag name
        void set_values(const std::map<std::string, TagValue> &tags, const std::string &prefix = "");

        // append tags from another hashtable, using a prefix for a tag name: prefix+name
        void append_tags(const std::map<std::string, TagValue> &tags, const std::string &prefix = "");

        // parse a string containing an INI formatted variables, using a prefix for a tag name: prefix+name
        void parse_ini(const std::string &ini, const std::string &separator = "=", const std::string &prefix = "", const std::string &stop_at_dir = "");
        static std::deque<std::string> iniGetBlock(const std::string &ini, const std::string &dir_name);

        std::string join(const std::string &line_sep, const std::string &tag_val_sep = ": ") const;
        std::string toString() const { return join("\n"); }

        // I/O
        bool toFile(const std::string &file_name, const std::string &sep = ": ") const;
        bool fromFile(const std::string &file_name, const std::string &sep = ": ");

        bool fromString(const std::string &str, const std::string &seplines = ",", const std::string &sepkey = " ");

    private:
        static std::string readline(const std::string &str, int &pos);
    };

} // namespace bim

#endif // BIM_TAG_MAP_H


