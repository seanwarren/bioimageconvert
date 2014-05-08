/* ZVI - Portable C++ library to read Zeiss ZVI image files

   Authors:

     Dmitry Fedorov 
     Copyright 2010 <www.bioimage.ucsb.edu> <www.dimin.net> 

     Michel Boudinot
     Copyright 2010 <Michel.Boudinot@inaf.cnrs-gif.fr>
   

   Version: 0.3

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted provided that the following conditions 
   are met:
   * Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
   * Neither the name of the authors nor the names of its contributors may be 
     used to endorse or promote products derived from this software without 
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
   THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ctime>

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <algorithm>

#include <xstring.h>
#include <tag_map.h>

#include "zvi.h"

using namespace bim;

//---------------------------------------------------------------------------------
// zvi::BaseAccessor
//---------------------------------------------------------------------------------

bool zvi::BaseAccessor::ensureType( POLE::Stream *s ) const {
  bim::uint16 vt;
  POLE::uint64 red = s->read( (unsigned char*) &vt, sizeof(bim::uint16) );   
  if (vt == this->type) return true;
  s->seek( s->tell() - sizeof(bim::uint16) );
  return false;
}

bool zvi::BaseAccessor::skip( POLE::Stream *s ) const {
  if (!ensureType(s)) return false; 
  return skip_value(s);
}

bool zvi::BaseAccessor::read( POLE::Stream *s, std::vector<unsigned char> *v ) const {
  if (!ensureType(s)) return false; 
  return read_value(s, v);
}


bool zvi::BaseAccessor::skip_value( POLE::Stream *s ) const {
  if (this->size==0) return true;
  s->seek( s->tell() + this->size );
  return true;
}

bool zvi::BaseAccessor::read_value( POLE::Stream *s, std::vector<unsigned char> *v ) const {
  v->resize(this->size);
  if (this->size==0) return true;
  POLE::uint64 red = s->read( (unsigned char*) &(*v)[0], this->size );
  return (red == this->size);
}

//---------------------------------------------------------------------------------
// Accessors
//---------------------------------------------------------------------------------

class EMPTY_Accessor: public zvi::BaseAccessor {
public:
  EMPTY_Accessor() { type=zvi::VT_EMPTY; size=0; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    return "EMPTY"; 
  }
};

class NULL_Accessor: public zvi::BaseAccessor {
public:
  NULL_Accessor() { type=zvi::VT_NULL; size=0; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    return "NULL"; 
  }
};

class I2_Accessor: public zvi::BaseAccessor {
public:
  I2_Accessor() { type=zvi::VT_I2; size=2; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%d", *(bim::int16*) &(*v)[offset] );
  }
};

class I4_Accessor: public zvi::BaseAccessor {
public:
  I4_Accessor() { type=zvi::VT_I4; size=4; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%ld", *(bim::int32*) &(*v)[offset] );
  }
};

class R4_Accessor: public zvi::BaseAccessor {
public:
  R4_Accessor() { type=zvi::VT_R4; size=4; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%f", *(float*) &(*v)[offset] );
  }
};

class R8_Accessor: public zvi::BaseAccessor {
public:
  R8_Accessor() { type=zvi::VT_R8; size=8; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%f", *(double*) &(*v)[offset] );
  }
};

class DATE_Accessor: public zvi::BaseAccessor {
public:
  DATE_Accessor() { type=zvi::VT_DATE; size=8; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    double dv = *( double* )&(*v)[offset];
    if (dv==0) return "";
    xstring s("Www Mmm 00 00:00:00 0000"); //("YYYY-mm-dd HH:MM:SS");
    time_t date = (time_t)( ( dv - 25569.0 - 1.0/24.0) * 86400.0 );
    try {
      char *ts = ctime(&date);
      if (!ts) return "";
      s = ts;
    } catch (...) {
      return "";
    }
    return s.strip("\x0A\x0D\x20");
  }
};

class BSTR_Accessor: public zvi::BaseAccessor {
public:
  BSTR_Accessor() { type=zvi::VT_BSTR; size=0; }

  virtual bool skip_value( POLE::Stream *s ) const {
    bim::int32 sb;
    POLE::uint64 red = s->read( (unsigned char*) &sb, 4 );
    if (red<4) return false;

    if (sb==0) return true;
    s->seek( s->tell() + sb );
    return true;
  }

  virtual bool read_value( POLE::Stream *s, std::vector<unsigned char> *v ) const {
    bim::int32 sb;
    POLE::uint64 red = s->read( (unsigned char*) &sb, 4 );
    if (red<4) return false;

    if (sb==0) return true;
    v->resize(sb);
    red = s->read( (unsigned char*) &(*v)[0], sb );
    return red == sb;
  }

  //  bstr string is kind of utf-16 (16 bit unicode windows)
  //  kludge: skip every 2nd bytes.
  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    xstring s(v->size()/2, ' ');
    int j=0;
    for( int i=offset; i<v->size(); i+=2 ) {
       char c = * (char*) &(*v)[i];
       if (c!=0) s[j++] = c;
    }
    return s.strip("\x0A\x0D\x20");
  }

};

// VT_BLOB and VT_STORED_OBJECT are also BSTR_Accessor
class BLOB_Accessor: public BSTR_Accessor {
public:
  BLOB_Accessor() { type=zvi::VT_BLOB; size=0; }
};

class STORED_OBJECT_Accessor: public BSTR_Accessor {
public:
  STORED_OBJECT_Accessor() { type=zvi::VT_STORED_OBJECT; size=0; }
};

class DISPATCH_Accessor: public zvi::BaseAccessor {
public:
  DISPATCH_Accessor() { type=zvi::VT_DISPATCH; size=16; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    return "DISPATCH"; 
  }
};

class UNKNOWN_Accessor: public zvi::BaseAccessor {
public:
  UNKNOWN_Accessor() { type=zvi::VT_UNKNOWN; size=16; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    return "UNKNOWN"; 
  }
};

class BOOL_Accessor: public zvi::BaseAccessor {
public:
  BOOL_Accessor() { type=zvi::VT_BOOL; size=2; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    if (*(bim::int16*)&(*v)[offset] == 0) return "false";
    return "true"; 
  }
};

class I1_Accessor: public zvi::BaseAccessor {
public:
  I1_Accessor() { type=zvi::VT_I1; size=1; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%d", *(bim::int8*) &(*v)[offset] );
  }
};

class UI1_Accessor: public zvi::BaseAccessor {
public:
  UI1_Accessor() { type=zvi::VT_UI1; size=1; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%ud", *(bim::uint8*) &(*v)[offset] );
  }
};

class UI2_Accessor: public zvi::BaseAccessor {
public:
  UI2_Accessor() { type=zvi::VT_UI2; size=2; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%ud", *(bim::uint16*) &(*v)[offset] );
  }
};

class UI4_Accessor: public zvi::BaseAccessor {
public:
  UI4_Accessor() { type=zvi::VT_UI4; size=4; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%lud", *(bim::uint32*) &(*v)[offset] );
  }
};

class I8_Accessor: public zvi::BaseAccessor {
public:
  I8_Accessor() { type=zvi::VT_I8; size=8; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%lld", *(bim::int64*) &(*v)[offset] );
  }
};

class UI8_Accessor: public zvi::BaseAccessor {
public:
  UI8_Accessor() { type=zvi::VT_UI8; size=8; }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    if (v->size()<this->size+offset) return ""; 
    return xstring::xprintf( "%llud", *(bim::uint64*) &(*v)[offset] );
  }
};

class ARRAY_Accessor: public zvi::BaseAccessor {
public:
  ARRAY_Accessor() { type=zvi::VT_ARRAY; size=0; }

  virtual bool skip_value( POLE::Stream *s ) const {
    bim::int16 sb;
    POLE::uint64 red = s->read( (unsigned char*) &sb, 2 );
    if (red<2) return false;

    if (sb==0) return true;
    s->seek( s->tell() + sb );
    return true;
  }

  virtual bool read_value( POLE::Stream *s, std::vector<unsigned char> *v ) const {
    bim::int16 sb;
    POLE::uint64 red = s->read( (unsigned char*) &sb, 2 );
    if (red<2) return false;

    if (sb==0) return true;
    v->resize(sb);
    red = s->read( (unsigned char*) &(*v)[0], sb );
    return red == sb;
  }

  virtual std::string toString(const std::vector<unsigned char> *v, unsigned int offset) const {
    return "";
  }

};

//---------------------------------------------------------------------------------
// Accessors
//---------------------------------------------------------------------------------

std::map< bim::uint16, zvi::BaseAccessor* > zvi::Accessors::accessors;

const zvi::BaseAccessor* zvi::Accessors::get(bim::uint16 type) {
  init();
  std::map< bim::uint16, BaseAccessor* >::iterator it = accessors.find(type);
  if (it == accessors.end()) return NULL;
  return it->second; 
}

void zvi::Accessors::init() {
  if (accessors.size()>0) return;
  accessors.insert( std::make_pair( VT_EMPTY, new EMPTY_Accessor() ) );
  accessors.insert( std::make_pair( VT_NULL, new NULL_Accessor() ) );

  accessors.insert( std::make_pair( VT_I1, new I1_Accessor() ) );
  accessors.insert( std::make_pair( VT_I2, new I2_Accessor() ) );
  accessors.insert( std::make_pair( VT_I4, new I4_Accessor() ) );
  accessors.insert( std::make_pair( VT_I8, new I8_Accessor() ) );

  accessors.insert( std::make_pair( VT_R4, new R4_Accessor() ) );
  accessors.insert( std::make_pair( VT_R8, new R8_Accessor() ) );
  
  accessors.insert( std::make_pair( VT_DATE, new DATE_Accessor() ) );
  accessors.insert( std::make_pair( VT_DISPATCH, new DISPATCH_Accessor() ) );
  accessors.insert( std::make_pair( VT_UNKNOWN, new UNKNOWN_Accessor() ) );
  accessors.insert( std::make_pair( VT_BOOL, new BOOL_Accessor() ) );

  accessors.insert( std::make_pair( VT_BSTR, new BSTR_Accessor() ) );
  accessors.insert( std::make_pair( VT_BLOB, new BLOB_Accessor() ) );
  accessors.insert( std::make_pair( VT_STORED_OBJECT, new STORED_OBJECT_Accessor() ) );

  accessors.insert( std::make_pair( VT_UI1, new UI1_Accessor() ) );
  accessors.insert( std::make_pair( VT_UI2, new UI2_Accessor() ) );
  accessors.insert( std::make_pair( VT_UI4, new UI4_Accessor() ) );
  accessors.insert( std::make_pair( VT_UI8, new UI8_Accessor() ) );

  accessors.insert( std::make_pair( VT_ARRAY, new ARRAY_Accessor() ) );
}

//---------------------------------------------------------------------------------
// Entry
//---------------------------------------------------------------------------------

zvi::Entry::Entry(const std::string &n, const bim::uint16 &t, const bim::uint32 &_offset) {
  type = t;
  name = n;
  offset = _offset;
}

bool zvi::Entry::read( POLE::Stream *s ) {
  POLE::uint64 red = s->read( (unsigned char*) &this->type, sizeof(bim::uint16) );   
  if (red != sizeof(bim::uint16)) return false;

  const zvi::BaseAccessor *a = zvi::Accessors::get(this->type);
  if (!a) return false;
  if (!a->read_value(s, &this->value)) return false;
  return true;
}

bool zvi::Entry::skip( POLE::Stream *s ) {
  POLE::uint64 red = s->read( (unsigned char*) &this->type, sizeof(bim::uint16) );   
  if (red != sizeof(bim::uint16)) return false;

  const zvi::BaseAccessor *a = zvi::Accessors::get(this->type);
  if (!a) return false;
  if (!a->skip_value(s)) return false;
  return true;
}

std::string zvi::Entry::toString() const {
  if (this->value.size()<=0) return std::string();
  const zvi::BaseAccessor *a = zvi::Accessors::get(this->type);
  if (!a) return std::string();
  return a->toString(&this->value);
}

bim::uint16 zvi::Entry::toUInt16() const {
  if (this->value.size()<sizeof(bim::uint16)) return 0;
  return * (bim::uint16*) &this->value[0];
}

bim::uint32 zvi::Entry::toUInt32() const {
  if (this->value.size()<sizeof(bim::uint32)) return 0;
  return * (bim::uint32*) &this->value[0];
}

bim::uint64 zvi::Entry::toUInt64() const {
  if (this->value.size()<sizeof(bim::uint64)) return 0;
  return * (bim::uint64*) &this->value[0];
}

//---------------------------------------------------------------------------------
// Stream
//---------------------------------------------------------------------------------

zvi::Stream::Stream( POLE::Storage *storage, const std::string &stream_name ) {
  stream_ = new POLE::Stream( storage, stream_name );
  if (this->isValid()) local_stream=true;
}

bool zvi::Stream::isValid() const {
  return (stream_ && !stream_->fail());
}

zvi::Stream::~Stream() {
  if (local_stream) delete stream_;
}

bim::uint16 zvi::Stream::currentType() {
  bim::uint16 vt;
  POLE::uint64 red = stream_->read( (unsigned char*) &vt, sizeof(bim::uint16) );
  stream_->seek( stream_->tell() - sizeof(bim::uint16) );
  if (red<sizeof(bim::uint16)) return VT_ILLEGAL;
  return vt;
}

bim::uint16 zvi::Stream::seekToNext() {
  bim::uint16 vt;
  POLE::uint64 red = stream_->read( (unsigned char*) &vt, sizeof(bim::uint16) );
  if (red<sizeof(bim::uint16)) vt=VT_ILLEGAL;
    
  const zvi::BaseAccessor *a = zvi::Accessors::get(vt);
  if (a) a->skip_value(stream_); 
  return currentType();
}

bool zvi::Stream::findNextType( const bim::uint16 &type ) {
  if (currentType() == type) return true;
  while ( seekToNext() != type )
     if( stream_->tell() >= stream_->size() ) return false; // no valid image data
  return true;
}

bool zvi::Stream::readNextValue( const zvi::Entry &e, TagMap *info ) {
  const zvi::BaseAccessor *a = zvi::Accessors::get(e.getType());
  if (!a) return false;

  if (e.getName() == "")
    return a->skip(stream_); 
  else {
    std::vector<unsigned char> v;
    if (!a->read(stream_, &v)) return false;
    info->set_value( e.getName(), a->toString(&v) );
  }
  return true;
}

bool zvi::Stream::readParseValues( const std::list<zvi::Entry> &defs, TagMap *info ) {
  std::list<zvi::Entry>::const_iterator it = defs.begin();
  while(it != defs.end()) {
    if (!this->readNextValue( *it, info )) return false;
    ++it;
  }
  return true;
}

bool zvi::Stream::readParseBlock( const bim::uint16 &vt, const std::list<Entry> &defs, TagMap *info ) {

  const zvi::BaseAccessor *a = zvi::Accessors::get(vt);
  if (!a) return false;

  std::vector<unsigned char> v;
  if (!a->read(stream_, &v)) return false;
  if (v.size()<=0) return false;

  std::list<zvi::Entry>::const_iterator et = defs.begin();
  while(et != defs.end()) {
    a = zvi::Accessors::get(et->getType());

    // now parse the value
    if (a) info->set_value( et->getName(), a->toString(&v, et->getOffset()) );
    et++;
  }
  return true;
}


//---------------------------------------------------------------------------------
// Image
//---------------------------------------------------------------------------------

zvi::Image::Image() {
  init();
}

zvi::Image::Image(Stream *s) {
  init();
  fromStream( s );
}

zvi::Image::Image(POLE::Storage *s, unsigned int image_id) {
  init();
  fromStorage( s, image_id );
}

bool zvi::Image::isValid() const { 
  return data_offset!=0 && image_info.size()>0; 
}

void zvi::Image::init() {
  
  // ImageItemContentsHeader
  ImageItemContentsHeader.push_back( Entry("Version", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("Type", VT_I4 ) );
  ImageItemContentsHeader.push_back( Entry("", VT_EMPTY ) );
  ImageItemContentsHeader.push_back( Entry("", VT_EMPTY ) );
  ImageItemContentsHeader.push_back( Entry("ImageWidth", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("ImageHeight", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("ImageDepth", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("PixelFormat", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("ImageCount", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("ValidBitsPerPixel", VT_I4) );
  ImageItemContentsHeader.push_back( Entry("PluginCLSID", VT_BLOB) );

  // CoordinateBlock
  CoordinateBlock.push_back( Entry("Coordinate/Z_ID", VT_I4, 8) );
  CoordinateBlock.push_back( Entry("Coordinate/Channel_ID", VT_I4, 12) );
  CoordinateBlock.push_back( Entry("Coordinate/Time_ID", VT_I4, 16) );

  // RawImageDataHeader
  RawImageDataHeader.push_back( Entry("raw/width", VT_I4, 0) );
  RawImageDataHeader.push_back( Entry("raw/height", VT_I4, 4) );
  RawImageDataHeader.push_back( Entry("raw/depth", VT_I4, 8) );
  RawImageDataHeader.push_back( Entry("raw/pixelWidth", VT_I4, 12) );
  RawImageDataHeader.push_back( Entry("raw/pixelFormat", VT_I4, 16) );
  RawImageDataHeader.push_back( Entry("raw/validBitsPerPixel", VT_I4, 20) );
}

bool zvi::Image::fromStream( zvi::Stream *s ) {

  // Image Item Contents Header
  if (!s->readParseValues( this->ImageItemContentsHeader, &image_info )) return false;

  // Others, coordinate block (VT_BLOB)
  if (!s->readParseBlock( zvi::VT_BLOB, this->CoordinateBlock, &image_info )) return false;

  // scan image <contents> for 1st VT_ARRAY variant
  if (!s->findNextType(zvi::VT_ARRAY)) return false;
  if (!s->readParseBlock( zvi::VT_ARRAY, this->RawImageDataHeader, &image_info )) return false;

  // move stream pointer back to start of image data
  this->data_offset = s->stream()->tell() - 4072; // 4096-6x4

  return true;
}

bool zvi::Image::fromStorage( POLE::Storage *s, unsigned int image_id ) {
  zvi::Stream zvi_stream( s, xstring::xprintf("/Image/Item(%d)/Contents", image_id) );
  if (!zvi_stream.isValid()) return false;
  return this->fromStream( &zvi_stream );
}

unsigned int pixelType2Bpp( int pixelType ) {
   switch (pixelType) {
     case 1:  return 24;
     case 2:  return 32;
     case 3:  return 8;
     case 4:  return 16;
     case 5:  return 32;
     case 6:  return 32;
     case 7:  return 64;
     case 8:  return 48;
     case 9:  return 96;
     default: return 0;
   }
}

bool zvi::Image::readImagePixels( Stream *s, const unsigned int &buf_size, unsigned char *buf ) {
  if (this->data_offset==0) fromStream(s);
  if (this->data_offset==0) return false;
  s->stream()->seek(this->data_offset);
  if (s->stream()->tell()!=this->data_offset) return false;

  unsigned int w = image_info.get_value_int( "raw/width", 0 );
  unsigned int h = image_info.get_value_int( "raw/height", 0 );
  unsigned int bpp = pixelType2Bpp(image_info.get_value_int( "raw/pixelFormat", 0 ));
  unsigned int vbp = image_info.get_value_int( "raw/validBitsPerPixel", 0 );
  
  // we do not support JPEG or LZW compressed images as of yet
  if (vbp<=1) return false;

  unsigned int image_size=w*h*(bpp/8);

  if (buf_size<image_size) return false;
  POLE::uint64 red = s->stream()->read( buf, image_size );
  if (red<image_size) return false;
  return true;
}

//---------------------------------------------------------------------------------
// Metadata
//---------------------------------------------------------------------------------

zvi::Metadata::Metadata() {
  init();
}

zvi::Metadata::Metadata(Stream *s) {
  init();
  fromStream(s);
}

zvi::Metadata::Metadata(POLE::Storage *s) {
  init();
  fromStorage(s);
}

bool zvi::Metadata::isValid() const { 
  return tags.size()>0; 
}

void zvi::Metadata::init() {
  
  // TagsContentsHeader
  TagsContentsHeader.push_back( Entry("Version", VT_I4) );
  TagsContentsHeader.push_back( Entry("TagCount", VT_I4) );

  // TagBlock
  // used Entry directly, so not needed now
  //TagBlock.push_back( Entry("Value", VT_EMPTY) );
  //TagBlock.push_back( Entry("ID", VT_I4) );
  //TagBlock.push_back( Entry("Attribute", VT_I4) );
}

std::string getTagName( bim::int32 tag ) {
  switch (tag) {
     case 222: return "Compression";
     case 258: return "BlackValue";
     case 259: return "WhiteValue";
     case 260: return "ImageDataMappingAutoRange";
     case 261: return "Thumbnail";
     case 262: return "GammaValue";
     case 264: return "ImageOverExposure";
     case 265: return "ImageRelativeTime1";
     case 266: return "ImageRelativeTime2";
     case 267: return "ImageRelativeTime3";
     case 268: return "ImageRelativeTime4";
     case 300: return "ImageRelativeTime";
     case 333: return "RelFocusPosition1";
     case 334: return "RelFocusPosition2";
     case 513: return "ObjectType";
     case 515: return "ImageWidth";
     case 516: return "ImageHeight";
     case 517: return "Number Raw Count";
     case 518: return "PixelType";
     case 519: return "NumberOfRawImages";
     case 520: return "ImageSize";
     case 523: return "Acquisition pause annotation";
     case 530: return "Document Subtype";
     case 531: return "Acquisition Bit Depth";
     case 532: return "Image Memory Usage (RAM)";
     case 534: return "Z-Stack single representative";
     case 769: return "Scale Factor for X";
     case 770: return "Scale Unit for X";
     case 771: return "Scale Width";
     case 772: return "Scale Factor for Y";
     case 773: return "Scale Unit for Y";
     case 774: return "Scale Height";
     case 775: return "Scale Factor for Z";
     case 776: return "Scale Unit for Z";
     case 777: return "Scale Depth";
     case 778: return "Scaling Parent";
     case 1001: return "Date";
     case 1002: return "code";
     case 1003: return "Source";
     case 1004: return "Message";
     case 1025: return "Acquisition Date";
     case 1026: return "8-bit acquisition";
     case 1027: return "Camera Bit Depth";
     case 1029: return "MonoReferenceLow";
     case 1030: return "MonoReferenceHigh";
     case 1031: return "RedReferenceLow";
     case 1032: return "RedReferenceHigh";
     case 1033: return "GreenReferenceLow";
     case 1034: return "GreenReferenceHigh";
     case 1035: return "BlueReferenceLow";
     case 1036: return "BlueReferenceHigh";
     case 1041: return "FrameGrabber Name";
     case 1042: return "Camera";
     case 1043: return "CameraUniqId";
     case 1044: return "CameraTriggerSignalType";
     case 1045: return "CameraTriggerEnable";
     case 1046: return "GrabberTimeout";
     case 1281: return "MultiChannelEnabled";
     case 1282: return "MultiChannel Color";
     case 1283: return "MultiChannel Weight";
     case 1284: return "Channel Name";
     case 1536: return "DocumentInformationGroup";
     case 1537: return "Title";
     case 1538: return "Author";
     case 1539: return "Keywords";
     case 1540: return "Comments";
     case 1541: return "SampleID";
     case 1542: return "Subject";
     case 1543: return "RevisionNumber";
     case 1544: return "Save Folder";
     case 1545: return "FileLink";
     case 1546: return "Document Type";
     case 1547: return "Storage Media";
     case 1548: return "File ID";
     case 1549: return "Reference";
     case 1550: return "File Date";
     case 1551: return "File Size";
     case 1553: return "Filename";
     case 1792: return "ProjectGroup";
     case 1793: return "Acquisition Date";
     case 1794: return "Last modified by";
     case 1795: return "User company";
     case 1796: return "User company logo";
     case 1797: return "Image";
     case 1800: return "User ID";
     case 1801: return "User Name";
     case 1802: return "User City";
     case 1803: return "User Address";
     case 1804: return "User Country";
     case 1805: return "User Phone";
     case 1806: return "User Fax";
     case 2049: return "Objective Name";
     case 2050: return "Optovar";
     case 2051: return "Reflector";
     case 2052: return "Condenser Contrast";
     case 2053: return "Transmitted Light Filter 1";
     case 2054: return "Transmitted Light Filter 2";
     case 2055: return "Reflected Light Shutter";
     case 2056: return "Condenser Front Lens";
     case 2057: return "Excitation Filter Name";
     case 2060: return "Transmitted Light Fieldstop Aperture";
     case 2061: return "Reflected Light Aperture";
     case 2062: return "Condenser N.A.";
     case 2063: return "Light Path";
     case 2064: return "HalogenLampOn";
     case 2065: return "Halogen Lamp Mode";
     case 2066: return "Halogen Lamp Voltage";
     case 2068: return "Fluorescence Lamp Level";
     case 2069: return "Fluorescence Lamp Intensity";
     case 2070: return "LightManagerEnabled";
     case 2071: return "RelFocusPosition";
     case 2072: return "Focus Position";
     case 2073: return "Stage Position X";
     case 2074: return "Stage Position Y";
     case 2075: return "Microscope Name";
     case 2076: return "Objective Magnification";
     case 2077: return "Objective N.A.";
     case 2078: return "MicroscopeIllumination";
     case 2079: return "External Shutter 1";
     case 2080: return "External Shutter 2";
     case 2081: return "External Shutter 3";
     case 2082: return "External Filter Wheel 1 Name";
     case 2083: return "External Filter Wheel 2 Name";
     case 2084: return "Parfocal Correction";
     case 2086: return "External Shutter 4";
     case 2087: return "External Shutter 5";
     case 2088: return "External Shutter 6";
     case 2089: return "External Filter Wheel 3 Name";
     case 2090: return "External Filter Wheel 4 Name";
     case 2103: return "Objective Turret Position";
     case 2104: return "Objective Contrast Method";
     case 2105: return "Objective Immersion Type";
     case 2107: return "Reflector Position";
     case 2109: return "Transmitted Light Filter 1 Position";
     case 2110: return "Transmitted Light Filter 2 Position";
     case 2112: return "Excitation Filter Position";
     case 2113: return "Lamp Mirror Position";
     case 2114: return "External Filter Wheel 1 Position";
     case 2115: return "External Filter Wheel 2 Position";
     case 2116: return "External Filter Wheel 3 Position";
     case 2117: return "External Filter Wheel 4 Position";
     case 2118: return "Lightmanager Mode";
     case 2119: return "Halogen Lamp Calibration";
     case 2120: return "CondenserNAGoSpeed";
     case 2121: return "TransmittedLightFieldstopGoSpeed";
     case 2122: return "OptovarGoSpeed";
     case 2123: return "Focus calibrated";
     case 2124: return "FocusBasicPosition";
     case 2125: return "FocusPower";
     case 2126: return "FocusBacklash";
     case 2127: return "FocusMeasurementOrigin";
     case 2128: return "FocusMeasurementDistance";
     case 2129: return "FocusSpeed";
     case 2130: return "FocusGoSpeed";
     case 2131: return "FocusDistance";
     case 2132: return "FocusInitPosition";
     case 2133: return "Stage calibrated";
     case 2134: return "StagePower";
     case 2135: return "StageXBacklash";
     case 2136: return "StageYBacklash";
     case 2137: return "StageSpeedX";
     case 2138: return "StageSpeedY";
     case 2139: return "StageSpeed";
     case 2140: return "StageGoSpeedX";
     case 2141: return "StageGoSpeedY";
     case 2142: return "StageStepDistanceX";
     case 2143: return "StageStepDistanceY";
     case 2144: return "StageInitialisationPositionX";
     case 2145: return "StageInitialisationPositionY";
     case 2146: return "MicroscopeMagnification";
     case 2147: return "ReflectorMagnification";
     case 2148: return "LampMirrorPosition";
     case 2149: return "FocusDepth";
     case 2150: return "MicroscopeType";
     case 2151: return "Objective Working Distance";
     case 2152: return "ReflectedLightApertureGoSpeed";
     case 2153: return "External Shutter";
     case 2154: return "ObjectiveImmersionStop";
     case 2155: return "Focus Start Speed";
     case 2156: return "Focus Acceleration";
     case 2157: return "ReflectedLightFieldstop";
     case 2158: return "ReflectedLightFieldstopGoSpeed";
     case 2159: return "ReflectedLightFilter 1";
     case 2160: return "ReflectedLightFilter 2";
     case 2161: return "ReflectedLightFilter1Position";
     case 2162: return "ReflectedLightFilter2Position";
     case 2163: return "TransmittedLightAttenuator";
     case 2164: return "ReflectedLightAttenuator";
     case 2165: return "Transmitted Light Shutter";
     case 2166: return "TransmittedLightAttenuatorGoSpeed";
     case 2167: return "ReflectedLightAttenuatorGoSpeed";
     case 2176: return "TransmittedLightVirtualFilterPosition";
     case 2177: return "TransmittedLightVirtualFilter";
     case 2178: return "ReflectedLightVirtualFilterPosition";
     case 2179: return "ReflectedLightVirtualFilter";
     case 2180: return "ReflectedLightHalogenLampMode";
     case 2181: return "ReflectedLightHalogenLampVoltage";
     case 2182: return "ReflectedLightHalogenLampColorTemperature";
     case 2183: return "ContrastManagerMode";
     case 2184: return "Dazzle Protection Active";
     case 2195: return "Zoom";
     case 2196: return "ZoomGoSpeed";
     case 2197: return "LightZoom";
     case 2198: return "LightZoomGoSpeed";
     case 2199: return "LightZoomCoupled";
     case 2200: return "TransmittedLightHalogenLampMode";
     case 2201: return "TransmittedLightHalogenLampVoltage";
     case 2202: return "TransmittedLightHalogenLampColorTemperature";
     case 2203: return "Reflected Coldlight Mode";
     case 2204: return "Reflected Coldlight Intensity";
     case 2205: return "Reflected Coldlight Color Temperature";
     case 2206: return "Transmitted Coldlight Mode";
     case 2207: return "Transmitted Coldlight Intensity";
     case 2208: return "Transmitted Coldlight Color Temperature";
     case 2209: return "Infinityspace Portchanger Position";
     case 2210: return "Beamsplitter Infinity Space";
     case 2211: return "TwoTv VisCamChanger Position";
     case 2212: return "Beamsplitter Ocular";
     case 2213: return "TwoTv CamerasChanger Position";
     case 2214: return "Beamsplitter Cameras";
     case 2215: return "Ocular Shutter";
     case 2216: return "TwoTv CamerasChangerCube";
     case 2218: return "Ocular Magnification";
     case 2219: return "Camera Adapter Magnification";
     case 2220: return "Microscope Port";
     case 2221: return "Ocular Total Magnification";
     case 2222: return "Field of View";
     case 2223: return "Ocular";
     case 2224: return "CameraAdapter";
     case 2225: return "StageJoystickEnabled";
     case 2226: return "ContrastManager Contrast Method";
     case 2229: return "CamerasChanger Beamsplitter Type";
     case 2235: return "Rearport Slider Position";
     case 2236: return "Rearport Source";
     case 2237: return "Beamsplitter Type Infinity Space";
     case 2238: return "Fluorescence Attenuator";
     case 2239: return "Fluorescence Attenuator Position";
     case 2252: return "Sideport Turret Position";
     case 2253: return "Sideport Left Percent";
     case 2254: return "Sideport Right Percent";
     case 2255: return "Reflected Light Shutter";
     case 2256: return "Sideport Left Function";
     case 2257: return "Sideport Right Function";
     case 2258: return "Baseport Slider Port Function";
     case 2259: return "Sideport Lightpath";
     case 2260: return "Sideport Lightpath ID";
     case 2261: return "Objective ID";
     case 2262: return "Reflector ID";
     case 2298: return "Microscope Port (Base)";
     case 2307: return "Camera Framestart Left";
     case 2308: return "Camera Framestart Top";
     case 2309: return "Camera Frame Width";
     case 2310: return "Camera Frame Height";
     case 2311: return "Camera Binning";
     case 2312: return "CameraFrameFull";
     case 2313: return "CameraFramePixelDistance";
     case 2318: return "DataFormatUseScaling";
     case 2319: return "CameraFrameImageOrientation";
     case 2320: return "VideoMonochromeSignalType";
     case 2321: return "VideoColorSignalType";
     case 2322: return "MeteorChannelInput";
     case 2323: return "MeteorChannelSync";
     case 2324: return "WhiteBalanceEnabled";
     case 2325: return "CameraWhiteBalanceRed";
     case 2326: return "CameraWhiteBalanceGreen";
     case 2327: return "CameraWhiteBalanceBlue";
     case 2331: return "CameraFrameScalingFactor";
     case 2562: return "Meteor Camera Type";
     case 2564: return "Exposure Time [ms]";
     case 2568: return "CameraExposureTimeAutoCalculate";
     case 2569: return "Meteor Gain Value";
     case 2571: return "Meteor Gain Automatic";
     case 2572: return "MeteorAdjustHue";
     case 2573: return "MeteorAdjustSaturation";
     case 2574: return "MeteorAdjustRedLow";
     case 2575: return "MeteorAdjustGreenLow";
     case 2576: return "Meteor Blue Low";
     case 2577: return "MeteorAdjustRedHigh";
     case 2578: return "MeteorAdjustGreenHigh";
     case 2579: return "MeteorBlue High";
     case 2582: return "CameraExposureTimeCalculationControl";
     case 2585: return "AxioCamFadingCorrectionEnable";
     case 2587: return "CameraLiveImage";
     case 2588: return "CameraLiveEnabled";
     case 2589: return "LiveImageSyncObjectName";
     case 2590: return "CameraLiveSpeed";
     case 2591: return "CameraImage";
     case 2592: return "CameraImageWidth";
     case 2593: return "CameraImageHeight";
     case 2594:	return "CameraImagePixelType";
     case 2595: return "CameraImageShMemoryName";
     case 2596: return "CameraLiveImageWidth";
     case 2597: return "CameraLiveImageHeight";
     case 2598: return "CameraLiveImagePixelType";
     case 2599: return "CameraLiveImageShMemoryName";
     case 2600: return "CameraLiveMaximumSpeed";
     case 2601: return "CameraLiveBinning";
     case 2602: return "CameraLiveGainValue";
     case 2603: return "CameraLiveExposureTimeValue";
     case 2604: return "CameraLiveScalingFactor";
     case 2817: return "ImageIndexColumn";
     case 2818: return "ImageIndexRow";
     case 2819: return "Image Z Index";
     case 2820: return "Image Channel Index";
     case 2821: return "Image Time Index";
     case 2822: return "Image Tile Index";
     case 2823: return "Image acquisition Index";
     case 2827: return "Image Scene Index";
     case 2838: return "Number Columns";
     case 2839: return "Number Rows";
     case 2841: return "Original Stage Position X";
     case 2842: return "Original Stage Position Y";
     case 3088: return "LayerDrawFlags";
     case 3334: return "RemainingTime";
     case 3585: return "User Field 1";
     case 3586: return "User Field 2";
     case 3587: return "User Field 3";
     case 3588: return "User Field 4";
     case 3589: return "User Field 5";
     case 3590: return "User Field 6";
     case 3591: return "User Field 7";
     case 3592: return "User Field 8";
     case 3593: return "User Field 9";
     case 3594: return "User Field 10";
     case 3840: return "ID";
     case 3841: return "Name";
     case 3842: return "Value";
     case 5501: return "PvCamClockingMode";
     case 5510: return "CameraBias";
     case 5512: return "CameraUnsharpEnabled";
     case 5523: return "CameraUnsharpTag";
     case 5528: return "CameraWhiteBalanceTemperature";
     case 6122: return "Lampswitch";
     case 8193: return "Autofocus Status Report";
     case 8194: return "Autofocus Position";
     case 8195: return "Autofocus Position Offset";
     case 8196: return "Autofocus Empty Field Threshold";
     case 8197: return "Autofocus Calibration Name";
     case 8198: return "Autofocus Current Calibration Item";
     case 20478: return "Feature Source";
     case 65537: return "CameraFrameFullWidth";
     case 65538: return "CameraFrameFullHeight";
     case 65541: return "AxioCam Shutter Signal";
     case 65542: return "AxioCam Delay Time";
     case 65543: return "AxioCam Shutter Control";
     case 65544: return "AxioCam BlackRefIsCalculated";
     case 65545: return "AxioCam Black Reference";
     case 65547: return "Camera Shading Correction";
     case 65550: return "AxioCam Enhance Color";
     case 65551: return "AxioCam NIR Mode";
     case 65552: return "CameraShutterCloseDelay";
     case 65553: return "CameraWhiteBalanceAutoCalculate";
     case 65556: return "AxioCam NIR Mode Available";
     case 65557: return "AxioCam Fading Correction Available";
     case 65559: return "AxioCam Enhance Color Available";
     case 65565: return "MeteorVideoNorm";
     case 65566: return "MeteorAdjustWhiteReference";
     case 65567: return "MeteorBlackReference";
     case 65568: return "MeteorChannelInputCountMono";
     case 65570: return "MeteorChannelInputCountRGB";
     case 65571: return "MeteorEnableVCR";
     case 65572: return "Meteor Brightness";
     case 65573: return "Meteor Contrast";
     case 65575: return "AxioCam Selector";
     case 65576: return "AxioCam Type";
     case 65577: return "AxioCam Info";
     case 65580: return "AxioCam Resolution";
     case 65581: return "AxioCam Color Model";
     case 65582: return "AxioCam MicroScanning";
     case 65585: return "Amplification Index";
     case 65586: return "Device Command";
     case 65587: return "BeamLocation";
     case 65588: return "ComponentType";
     case 65589: return "ControllerType";
     case 65590: return "CameraWhiteBalanceCalculationRedPaint";
     case 65591: return "CameraWhiteBalanceCalculationBluePaint";
     case 65592: return "CameraWhiteBalanceSetRed";
     case 65593: return "CameraWhiteBalanceSetGreen";
     case 65594: return "CameraWhiteBalanceSetBlue";
     case 65595: return "CameraWhiteBalanceSetTargetRed";
     case 65596: return "CameraWhiteBalanceSetTargetGreen";
     case 65597: return "CameraWhiteBalanceSetTargetBlue";
     case 65598: return "ApotomeCamCalibrationMode";
     case 65599: return "ApoTome Grid Position";
     case 65600: return "ApotomeCamScannerPosition";
     case 65601: return "ApoTome Full Phase Shift";
     case 65602: return "ApoTome Grid Name";
     case 65603: return "ApoTome Staining";
     case 65604: return "ApoTome Processing Mode";
     case 65605: return "ApotmeCamLiveCombineMode";
     case 65606: return "ApoTome Filter Name";
     case 65607: return "Apotome Filter Strength";
     case 65608: return "ApotomeCamFilterHarmonics";
     case 65609: return "ApoTome Grating Period";
     case 65610: return "ApoTome Auto Shutter Used";
     case 65611: return "Apotome Cam Status";
     case 65612: return "ApotomeCamNormalize";
     case 65613: return "ApotomeCamSettingsManager";
     case 65614: return "DeepviewCamSupervisorMode";
     case 65615: return "DeepView Processing";
     case 65616: return "DeepviewCamFilterName";
     case 65617: return "DeepviewCamStatus";
     case 65618: return "DeepviewCamSettingsManager";
     case 65619: return "DeviceScalingName";
     case 65620: return "CameraShadingIsCalculated";
     case 65621: return "CameraShadingCalculationName";
     case 65622: return "CameraShadingAutoCalculate";
     case 65623: return "CameraTriggerAvailable";
     case 65626: return "CameraShutterAvailable";
     case 65627: return "AxioCam ShutterMicroScanningEnable";
     case 65628: return "ApotomeCamLiveFocus";
     case 65629: return "DeviceInitStatus";
     case 65630: return "DeviceErrorStatus";
     case 65631: return "ApotomeCamSliderInGridPosition";
     case 65632: return "Orca NIR Mode Used";
     case 65633: return "Orca Analog Gain";
     case 65634: return "Orca Analog Offset";
     case 65635: return "Orca Binning";
     case 65636: return "Orca Bit Depth";
     case 65637: return "ApoTome Averaging Count";
     case 65638: return "DeepView DoF";
     case 65639: return "DeepView EDoF";
     case 65643: return "DeepView Slider Name";
     case 65651: return "ApoTome Decay";
     case 65652: return "ApoTome Epsilon";
     case 65655: return "AxioCam AnalogGainEnable";
     case 65657: return "ApoTome Phase Angles";
     case 65658: return "ApoTome Image Format";
     case 65661: return "ApoTome Burst Mode";
     case 65662: return "ApoTome Generic Camera Name";
     case 65666: return "AxioCam Saturation";
     case 16777216: return "ComplexImage";
     case 16777488: return "Excitation Wavelength";
     case 16777489: return "Emission Wavelength";
     default: return xstring::xprintf( "tagID_%d", tag);          
  }
}

std::string pixelTypeString( int pixelType ) {
   switch (pixelType) {
     case 1:  return "8-bit xRGB Triple (B, G, R)";
     case 2:  return "8-bit xRGB Quad (B, G, R, A)";
     case 3:  return "8-bit grayscale";
     case 4:  return "16-bit unsigned integer";
     case 5:  return "32-bit unsigned integer";
     case 6:  return "32-bit IEEE float";
     case 7:  return "64-bit IEEE float";
     case 8:  return "16-bit unsigned xRGB Triple (B, G, R)";
     case 9:  return "32-bit xRGB Triple (B, G, R)";
     default: return xstring::xprintf("%d", pixelType);
   }
}

std::string zvi::Metadata::parseTagValue( bim::int32 tag, const Entry &val ) {
  switch (tag) {     
    case 518:  return pixelTypeString( val.toUInt32() );
    case 2594: return pixelTypeString( val.toUInt32() );
    default:   return val.toString();
  }
}

bool zvi::Metadata::fromStream( zvi::Stream *s, bool rootStorage ) {
  // skip Root Storage CLSID 16 bytes 
  if (rootStorage) s->stream()->seek(16);    
  TagMap tags_info;

  // TagsContentsHeader
  if (!s->readParseValues( this->TagsContentsHeader, &tags_info )) return false;

  int tagcount = tags_info.get_value_int( "TagCount", 0 );  
  for( int i=0; i<tagcount; ++i ) {
    Entry val("Value");
    if (!val.read( s->stream() )) break;

    Entry id("ID");
    if (!id.read( s->stream() )) break;

    Entry attr("Attribute");
    if (!attr.skip( s->stream() )) break;

    tags.set_value( getTagName(id.toUInt32()), this->parseTagValue(id.toUInt32(), val) );
  }  

  return true;
}

bool zvi::Metadata::fromStorage( POLE::Storage *s ) {
  zvi::Stream zvi_stream( s, "/Tags");
  if (!zvi_stream.isValid()) return false;
  return this->fromStream( &zvi_stream, true );
}

//---------------------------------------------------------------------------------
// Scaling
//---------------------------------------------------------------------------------

zvi::Scaling::Scaling() {
  init();
}

zvi::Scaling::Scaling(Stream *s) {
  init();
  fromStream(s);
}

zvi::Scaling::Scaling(POLE::Storage *s) {
  init();
  fromStorage(s);
}

bool zvi::Scaling::isValid() const { 
  return tags.size()>0; 
}

std::string scalingUnitString( int scalingUnit ) {
   switch (scalingUnit) {
     case   0: return "Pixel";
     case  72: return "Meter";
     case  76: return "Micrometer";
     case  77: return "Nanometer";
     case  81: return "Inch";
     case  84: return "Micrometer";     
     case 136: return "Second";
     case 139: return "Millisecond";
     case 140: return "Microsecond";
     case 145: return "Minute";
     case 146: return "Hour";
     default:  return xstring::xprintf("%d", scalingUnit);
   }
}

void zvi::Scaling::init() {
  
  // ScalingContentsHeader
  ScalingContentsHeader.push_back( Entry("Version", VT_I4) );
  ScalingContentsHeader.push_back( Entry("Key", VT_BSTR) );
  ScalingContentsHeader.push_back( Entry("", VT_I4) );

  ScalingContentsHeader.push_back( Entry("ScalingFactorX", VT_R8) );
  ScalingContentsHeader.push_back( Entry("ScalingUnitX", VT_I4) );
  ScalingContentsHeader.push_back( Entry("", VT_EMPTY) ); // Unit - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Origin - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Angle - not used

  ScalingContentsHeader.push_back( Entry("ScalingFactorY", VT_R8) );
  ScalingContentsHeader.push_back( Entry("ScalingUnitY", VT_I4) );
  ScalingContentsHeader.push_back( Entry("", VT_EMPTY) ); // Unit - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Origin - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Angle - not used

  ScalingContentsHeader.push_back( Entry("ScalingFactorZ", VT_R8) );
  ScalingContentsHeader.push_back( Entry("ScalingUnitZ", VT_I4) );
  ScalingContentsHeader.push_back( Entry("", VT_EMPTY) ); // Unit - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Origin - not used
  ScalingContentsHeader.push_back( Entry("", VT_R8) ); // Angle - not used
}

bool zvi::Scaling::fromStream( zvi::Stream *s ) {

  // ScalingContentsHeader
  s->readParseValues( this->ScalingContentsHeader, &tags );

  if (tags.hasKey("ScalingUnitX"))
    tags.set_value( "ScalingUnitX", scalingUnitString(tags.get_value_int("ScalingUnitX", 0)) );
  if (tags.hasKey("ScalingUnitY"))
    tags.set_value( "ScalingUnitY", scalingUnitString(tags.get_value_int("ScalingUnitY", 0)) );
  if (tags.hasKey("ScalingUnitZ"))
    tags.set_value( "ScalingUnitZ", scalingUnitString(tags.get_value_int("ScalingUnitZ", 0)) );

  return true;
}

bool zvi::Scaling::fromStorage( POLE::Storage *s ) {
  zvi::Stream zvi_stream( s, "/Image/Scaling/Contents");
  if (!zvi_stream.isValid()) return false;
  return this->fromStream( &zvi_stream );
}

//---------------------------------------------------------------------------------
// PlaneStreamMap 
//---------------------------------------------------------------------------------

inline std::string zvi::PlaneStreamMap::create_key( const unsigned int &c, const unsigned int &z, const unsigned int &t ) const {
  return xstring::xprintf( "%.4d_%.4d_%.4d", c, z, t );
}

std::string zvi::PlaneStreamMap::get_stream_name( const unsigned int &c, const unsigned int &z, const unsigned int &t ) const {
  return get_value( create_key(c, z, t) );
}

void zvi::PlaneStreamMap::set_stream_name( const unsigned int &c, const unsigned int &z, const unsigned int &t, const std::string &stream_name ) {
  set_value( create_key(c, z, t), stream_name );
}

//---------------------------------------------------------------------------------
// Directory 
//---------------------------------------------------------------------------------

zvi::Directory::Directory() { init(); }
zvi::Directory::Directory(POLE::Storage *s) { init(); fromStorage(s); }

void zvi::Directory::init() {
  number_c=0; number_z=0; number_t=0; number_p=0;
  width=0; height=0; pixel_format=0;
}

bool zvi::Directory::isValid(POLE::Storage *s) {
  std::list<std::string> required_streams;
  required_streams.push_back( "/Tags" ); // must have global tags
  required_streams.push_back( "/Image/Item(0)/Contents" ); // must have at least one image
  required_streams.push_back( "/Image/Item(0)/Tags/Contents" ); // must have tags for the first image

  std::list<std::string>::const_iterator it = required_streams.begin();
  while(it != required_streams.end()) {
    POLE::Stream* stream = new POLE::Stream( s, *it );
    bool is_valid_stream = (stream && !stream->fail() && !s->isDirectory(*it) );
    delete stream;
    if (!is_valid_stream) return false;
    ++it;
  }
  return true;
}

bool zvi::Directory::isValid() const {
  if (number_c<=0) return false;
  if (number_z<=0) return false;
  if (number_t<=0) return false;
  if (number_p<=0) return false;
  if (width<=0) return false;
  if (height<=0) return false;
  return true;
}

unsigned int zvi::Directory::pixelBitDepth() const { 
  return pixelType2Bpp(pixel_format); 
}

bool zvi::Directory::parseEmbeddedMetadata() {

  // contents of /Tags
  zvi::Metadata zvi_metadata;
  zvi_metadata.fromStorage( storage );
  if (zvi_metadata.isValid())
    tags = *zvi_metadata.meta();

  // contents of /Image/Scaling/Contents
  zvi::Scaling zvi_scaling;
  zvi_scaling.fromStorage( storage );
  if (zvi_scaling.isValid())
    scaling = *zvi_scaling.meta();

  // contents of /Image/Scaling/Tags/Contents
  //zvi::Stream s( storage, "/Image/Scaling/Tags/Contents" );
  //if (zvi_stream.isValid()) {
  //  zvi::Metadata scaling_metadata(&s);
  //  if (scaling_metadata.isValid())
  //    scalingTags = *scaling_metadata.meta();
  //}

  return true;
}

bool zvi::Directory::parseImageGeometry() {

  std::map<int, std::string> channels_mapping;

  // attempt to read all available images and populate the extent of CZT axis
  unsigned int plane=0;
  while (1) {
    std::string stream_name = xstring::xprintf("/Image/Item(%d)/Contents", plane);
    zvi::Stream zvi_stream( storage, stream_name );
    if (!zvi_stream.isValid()) break;

    zvi::Image img(&zvi_stream);
    if (!img.isValid()) break;

    unsigned int z_id = img.info()->get_value_int( "Coordinate/Z_ID", 0 );
    unsigned int c_id = img.info()->get_value_int( "Coordinate/Channel_ID", 0 );
    unsigned int t_id = img.info()->get_value_int( "Coordinate/Time_ID", 0 );

    // channels my not start at 0 and may be anything like: 0-3, 1, 2, 1-2, 0
    // thus the real number of channels and the proper mapping will have to be guessed
    channels_mapping[c_id] = stream_name;

    planes.set_stream_name( c_id, z_id, t_id, stream_name );
    ++number_p;
    number_c = std::max( number_c, c_id+1 ); 
    number_z = std::max( number_z, z_id+1 );  
    number_t = std::max( number_t, t_id+1 ); 

    if (plane==0) {
      width = img.info()->get_value_int( "raw/width", 0 );
      height = img.info()->get_value_int( "raw/height", 0 );
      pixel_format = img.info()->get_value_int( "raw/pixelFormat", 0 );
    }

    ++plane;
  }

  // now that we know the real number of channels, update geometry and channel mapping
  number_c = (unsigned int) channels_mapping.size(); 
  this->sequenced_to_zvi_channel.clear();
  std::map< int, std::string >::const_iterator it = channels_mapping.begin();
  int c=0;
  while (it != channels_mapping.end()) {
    this->sequenced_to_zvi_channel[c] = it->first;
    ++it;
    ++c;
  }

  return true;
}

bool zvi::Directory::parseCreateMetadata() {

  // get necessary channel info
  for (unsigned int c=0; c<number_c; ++c) {
    xstring stream_name = planes.get_stream_name( this->sequenced_to_zvi_channel[c], 0, 0 );
    stream_name = stream_name.replace( ")/Contents", ")/Tags/Contents" );
    zvi::Stream s( storage, stream_name );
    if (!s.isValid()) continue;
    zvi::Metadata img_metadata(&s);
    if (img_metadata.isValid()) {
      if (img_metadata.meta()->hasKey("Channel Name"))
        image_info.set_value( xstring::xprintf("channel_name_%d", c), img_metadata.meta()->get_value("Channel Name") );
    } // if parsed metadata
  }

  //------------------------------------------------------------
  // create display to channel mapping
  //------------------------------------------------------------
  std::map<int, int> display_to_sequenced;
  std::map< int, int >::const_iterator it = sequenced_to_zvi_channel.begin();
  while (it != sequenced_to_zvi_channel.end()) {
    display_to_sequenced[it->second] = it->first;
    ++it;
  }

  // now write tags based on the order
  std::map<int, std::string> display_channel_names;
  display_channel_names[0] = "red";
  display_channel_names[1] = "green";
  display_channel_names[2] = "blue";
  display_channel_names[3] = "yellow";
  display_channel_names[4] = "magenta";
  display_channel_names[5] = "cyan";
  display_channel_names[6] = "gray";

  // init mapping to empty channels
  std::map< int, std::string >::const_iterator itt = display_channel_names.begin();
  while (itt != display_channel_names.end()) {
    image_info.set_value( xstring::xprintf("display_channel_%s", itt->second.c_str() ), -1 );
    ++itt;
  }

  // now write real image channels
  it = display_to_sequenced.begin();
  while (it != display_to_sequenced.end()) {
    if (display_channel_names.find( it->first ) != display_channel_names.end( ))
      image_info.set_value( xstring::xprintf("display_channel_%s", display_channel_names[it->first].c_str() ), it->second );
    ++it;
  }

  return true;
}

bool zvi::Directory::fromStorage( POLE::Storage *s ) {
  if (!zvi::Directory::isValid(s)) return false;
  storage = s;

  // first parse embedded metadata
  if (!this->parseEmbeddedMetadata()) return false;

  // second read all image items and create image geometry
  if (!this->parseImageGeometry()) return false;

  // third create metadata knowing geometry
  if (!this->parseCreateMetadata()) return false;

  return false;
}

bool zvi::Directory::readImagePixels( const unsigned int &c, const unsigned int &z, const unsigned int &t, const unsigned int &buf_size, unsigned char *buf ) {

  std::string stream_name = planes.get_stream_name( this->sequenced_to_zvi_channel[c], z, t );
  zvi::Stream zvi_stream( storage, stream_name );
  if (!zvi_stream.isValid()) return false;
  
  zvi::Image zvi_image;
  zvi_image.fromStream( &zvi_stream );

  unsigned int z_id = zvi_image.info()->get_value_int( "Coordinate/Z_ID", 0 );
  unsigned int c_id = zvi_image.info()->get_value_int( "Coordinate/Channel_ID", 0 );
  unsigned int t_id = zvi_image.info()->get_value_int( "Coordinate/Time_ID", 0 );
  if (this->sequenced_to_zvi_channel[c]!=c_id || z!=z_id || t!=t_id) return false;

  return zvi_image.readImagePixels( &zvi_stream, buf_size, buf );
}

