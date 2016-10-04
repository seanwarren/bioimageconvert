/*****************************************************************************
  NANOSCOPE format support
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    01/10/2005 12:17 - First creation
    02/08/2005 22:30 - Update for new magic
    09/12/2005 17:34 - updated to api version 1.3
            
  Ver : 3
*****************************************************************************/

#ifndef BIM_NANOSCOPE_FORMAT_H
#define BIM_NANOSCOPE_FORMAT_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

#include <stdio.h>
#include <vector>
#include <string>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* nanoscopeGetFormatHeader(void);
}

namespace bim {

#define BIM_NANOSCOPE_MAGIC_SIZE 11
const char nanoscopeMagic[12]  = "\\*File list";
const char nanoscopeMagicF[18] = "\\*Force file list";

class NanoscopeImg {
public:
  NanoscopeImg(): width(0), height(0), data_offset(0), xR(0.0), yR(0.0), zR(0.0) {}

  int width, height;
  long data_offset;
  std::string metaText;
  double xR, yR, zR; // pixel resolution for XYZ
  std::string data_type;
};

typedef std::vector<NanoscopeImg> NanoImgVector;

class NanoscopeParams {
public:
  NanoscopeParams(): channels(1) { i = bim::initImageInfo(); }
  bim::ImageInfo i;
  std::string metaText;
  NanoImgVector imgs;
  int channels;
};

//----------------------------------------------------------------------------
// LineReader
//----------------------------------------------------------------------------

#define BIM_LINE_BUF_SIZE 2048

class LineReader
{
public:
  LineReader( FormatHandle *newHndl );
  ~LineReader();

  std::string line(); 
  const char* line_c_str(); 

  bool readNextLine();
  bool isEOLines();

protected:
  std::string prline;

private:
  FormatHandle *fmtHndl;
  char buf[BIM_LINE_BUF_SIZE];
  int  bufpos;
  int  bufsize;
  bool eolines;

  bool loadBuff();
};


class TokenReader: public LineReader
{
public:
  TokenReader( FormatHandle *newHndl );

  bool isKeyTag   ();
  bool isImageTag ();
  bool isEndTag   ();
  bool compareTag ( const char *tag ); // compare exactly the tag
  bool isTag ( const char *tag );      // only verify if this token contains tag
  int  readParamInt ( const char *tag );
  void readTwoParamDouble ( const char *tag, double *p1, double *p2 );
  void readTwoDoubleRemString ( const char *tag, double *p1, double *p2, std::string *str );
  std::string readImageDataString ( const char *tag );
};

} // namespace bim

#endif // BIM_NANOSCOPE_FORMAT_H
