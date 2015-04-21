/*******************************************************************************
 Configuration parameters from command line

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/08/2001 21:53:31 - First creation

 Ver : 1
*******************************************************************************/

#ifndef XCONF_H
#define XCONF_H

#include <ctime>

#include <string>
#include <vector>
#include <map>

#include "xstring.h"

namespace bim {

//--------------------------------------------------------------------------------------
// xoperations - a list of pairs of strings: operation,arguments 
//--------------------------------------------------------------------------------------

typedef std::pair<xstring, xstring> xoperation;

class xoperations : public std::vector<xoperation> {
public:
    // constructors
    explicit xoperations() : std::vector<xoperation>() {}

    bool contains(const xstring &operation) const;
    xstring arguments(const xstring &operation) const;

    xoperations left(const xstring &operation) const;
    xoperations right(const xstring &operation) const;
};


//--------------------------------------------------------------------------------------
// XConf
//--------------------------------------------------------------------------------------

class XConf {

public:
  XConf() {}
  XConf(int argc, char** argv) { readParams( argc, argv ); }
  ~XConf() {}

  int readParams( int argc, char** argv );
#ifdef BIM_USE_CODECVT
  int readParams(int argc, wchar_t** argv);
#endif

  //number_values: 0 - no vals, -1 - comma separated list of values 
  void appendArgumentDefinition( const std::string &key, int number_values, const xstring &description="" );

public:
    xstring usage() const;
    void print( const std::string &s, int verbose_level = 1 ) const;
    void error(const std::string &s) const;

    void timerStart() { this->timer = clock(); }
    inline clock_t timerElapsed() const { return clock() - timer; }
    void printElapsed(const std::string &s, int verbose_level = 1) const;

public:
  bool keyExists( const std::string &key ) const;
  bool hasKey(const std::string &key ) const { return keyExists(key); }

  std::vector<xstring> getValues( const std::string &key ) const;
  std::vector<int>     getValuesInt( const std::string &key, int def=0 ) const;
  std::vector<double>  getValuesDouble( const std::string &key, double def=0.0 ) const;

  std::vector<xstring> splitValue( const std::string &key, const std::string &def="", const xstring &separator="," ) const;
  std::vector<int>     splitValueInt( const std::string &key, int def=0, const xstring &separator="," ) const;
  std::vector<double>  splitValueDouble( const std::string &key, double def=0.0, const xstring &separator="," ) const;

  xstring getValue( const std::string &key, const std::string &def=std::string() ) const;
  int     getValueInt( const std::string &key, int def=0 ) const;
  double  getValueDouble( const std::string &key, double def=0.0 ) const;

  xoperations getOperations() const { return this->operations; }


protected: 
    // defines unique argument names and a number of values in each, 0 - no vals, -1 - comma separated list of values 
    std::map<xstring, int> arguments_defs;
    std::map<xstring, xstring> arguments_descr;
    clock_t timer;

    int verbose;

    // after the processing the arguments will have all arguments
    std::map<xstring, std::vector<xstring> > arguments;
    xoperations operations;

protected: 
    virtual void init();
    virtual void processArguments() {}
    virtual void cureParams() {}

public:
    static std::vector<xstring> scan_strings( char *line );
};

//--------------------------------------------------------------------------------------
// EXConf - an example of how to use XConf
//--------------------------------------------------------------------------------------

class EXConf: public XConf {

public:
  std::string file_input;
  std::string file_output;
  std::vector<int> parameters;

protected: 
  virtual void init();    
  virtual void processArguments();
};

} // namespace bim

#endif // XCONF_H
