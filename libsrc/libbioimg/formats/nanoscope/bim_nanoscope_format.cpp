/*****************************************************************************
  NANOSCOPE format support 
  UCSB/BioITR property
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    01/10/2005 12:17 - First creation
    02/08/2005 22:30 - Support for incomplete image sections
        
  Ver : 2
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>


#include "bim_nanoscope_format.h"

using namespace bim;
#include "bim_nanoscope_format_io.cpp"


//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

void addDefaultKeyTag(const char *token, NanoscopeParams *nanoPar)
{
  nanoPar->metaText.append("[");
  nanoPar->metaText.append( token+2 );
  nanoPar->metaText.append("]\n");
}

void addDefaultTag(const char *token, NanoscopeParams *nanoPar)
{
  nanoPar->metaText.append( token+1 );
  nanoPar->metaText.append("\n");
}

void addImgKeyTag(const char *token, NanoscopeParams *nanoPar)
{
  nanoPar->imgs.rbegin()->metaText.append("[");
  nanoPar->imgs.rbegin()->metaText.append( token+2 );
  nanoPar->imgs.rbegin()->metaText.append("]\n");
}

void addImgTag(const char *token, NanoscopeParams *nanoPar)
{
  nanoPar->imgs.rbegin()->metaText.append( token+1 );
  nanoPar->imgs.rbegin()->metaText.append("\n");
}

void nanoscopeGetImageInfo( FormatHandle *fmtHndl )
{
  bool inimage = false;

  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
  ImageInfo *info = &nanoPar->i; 
  
  info->ver = sizeof(ImageInfo);
  info->imageMode = IM_GRAYSCALE;
  info->tileWidth = 0;
  info->tileHeight = 0; 
  info->transparentIndex = 0;
  info->transparencyMatting = 0;
  info->lut.count = 0;
  info->samples = 1;
  info->depth = 16;

  if (fmtHndl->stream == NULL) return;

  TokenReader tr( fmtHndl );
  tr.readNextLine();

  while (!tr.isEOLines())
  {
    if ( tr.isImageTag() )
    {
      // now add new image
      nanoPar->imgs.push_back( NanoscopeImg() );
      addImgKeyTag( tr.line_c_str(), nanoPar );
      inimage=true;
      tr.readNextLine();
      continue;
    }
    
    if (!inimage)
    { // if capturing default parameters
      if ( tr.isKeyTag() )
        addDefaultKeyTag(tr.line_c_str(), nanoPar); // add key meta data tag
      else
        addDefaultTag(tr.line_c_str(), nanoPar);    // add simple meta data tag
    }
    else
    { // if capturing image parameters
      if ( tr.isTag( "\\Data offset:" ) )
        nanoPar->imgs.rbegin()->data_offset = tr.readParamInt ( "\\Data offset:" );
      else
      if ( tr.isTag( "\\Samps/line:" ) )
        nanoPar->imgs.rbegin()->width = tr.readParamInt ( "\\Samps/line:" );
      else
      if ( tr.isTag( "\\Number of lines:" ) )
        nanoPar->imgs.rbegin()->height = tr.readParamInt ( "\\Number of lines:" );
      else
        addImgTag(tr.line_c_str(), nanoPar);


      // parse metadata
      //\Scan size: 15 15 ~m - microns
      //\Scan size: 353.381 353.381 nm - nanometers
      if ( tr.isTag( "\\Scan size:" ) ) {
        //tr.readTwoParamDouble ( "\\Scan size:", &nanoPar->imgs.rbegin()->xR, &nanoPar->imgs.rbegin()->yR );
        
        std::string str;
        tr.readTwoDoubleRemString( "\\Scan size:", &nanoPar->imgs.rbegin()->xR, &nanoPar->imgs.rbegin()->yR, &str );
        
        // if size is in nm then convert to um
        if ( strncmp( str.c_str(), "nm", 2 ) == 0 ) {
          nanoPar->imgs.rbegin()->xR /= 1000.0;
          nanoPar->imgs.rbegin()->yR /= 1000.0;
        }
      }

      //\@2:Image Data: S [Height] "Height"
      if ( tr.isTag( "\\@2:Image Data:" ) ) {
        nanoPar->imgs.rbegin()->data_type = tr.readImageDataString ( "\\@2:Image Data:" );
      }
    }

    tr.readNextLine();
  } // while
  
  // walk trough image list and verify it's consistency
  NanoImgVector::iterator imgit = nanoPar->imgs.begin();

  while (imgit < nanoPar->imgs.end() )
  {
    if ( (imgit->width <= 0) || (imgit->height <= 0) || (imgit->data_offset <= 0) )
    {
      // before removing add metadata to image pool
      nanoPar->metaText.append( imgit->metaText.c_str() );
      nanoPar->imgs.erase(imgit);
    }
    else
      ++imgit;
  }

  // now finalize info
  NanoscopeImg nimg = nanoPar->imgs.at(0);
  if (fmtHndl->pageNumber < nanoPar->imgs.size())
    nimg = nanoPar->imgs.at( fmtHndl->pageNumber );

  info->width  = nimg.width;
  info->height = nimg.height;
  info->samples = 1;
  nanoPar->channels = 1;
  info->number_pages = nanoPar->imgs.size();
  //info->number_z = info->number_pages;

  // decide reading as pages or channels
  /*
  bool same_size = true;
  for (int i=0; i<nanoPar->imgs.size(); ++i) {
    NanoscopeImg nimgt = nanoPar->imgs.at(i);
    if (info->width != nimgt.width) { same_size = false; break; }
    if (info->height != nimgt.height) { same_size = false; break; }
  }

  if ( same_size && nanoPar->imgs.size()<=3 ) {
    info->number_pages = 1;
    info->samples = nanoPar->imgs.size();
  } else {
    info->number_pages = nanoPar->imgs.size();
    info->samples = 1;
  }
  nanoPar->channels = info->samples;
  */

  info->resUnits = RES_um;
  info->xRes = nimg.xR / nimg.width;
  info->yRes = nimg.yR / nimg.height;
}

//****************************************************************************
//
// FORMAT DEMANDED FUNTIONS
//
//****************************************************************************


//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int nanoscopeValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < BIM_NANOSCOPE_MAGIC_SIZE) return -1;
  if (memcmp( magic, nanoscopeMagic, BIM_NANOSCOPE_MAGIC_SIZE ) == 0) return 0;
  if (memcmp( magic, nanoscopeMagicF, BIM_NANOSCOPE_MAGIC_SIZE ) == 0) return 0;
  return -1;
}

FormatHandle nanoscopeAquireFormatProc( void )
{
  FormatHandle fp = initFormatHandle();
  return fp;
}

void nanoscopeCloseImageProc( bim::FormatHandle *fmtHndl);
void nanoscopeReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  nanoscopeCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void nanoscopeCloseImageProc (FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return;
  xclose ( fmtHndl );
  if (fmtHndl->internalParams != NULL) {
    NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
    delete nanoPar;
  }
  fmtHndl->internalParams = NULL;
}

bim::uint nanoscopeOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) nanoscopeCloseImageProc (fmtHndl);  
  fmtHndl->internalParams = (void *) new NanoscopeParams();

  fmtHndl->io_mode = io_mode;
  if (io_mode == IO_READ) {
    xopen(fmtHndl);
    if (!fmtHndl->stream) {
        nanoscopeCloseImageProc(fmtHndl);
        return 1;
    }
    nanoscopeGetImageInfo( fmtHndl );
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint nanoscopeGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
  return nanoPar->imgs.size();
}


ImageInfo nanoscopeGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint  ) {
  ImageInfo ii = initImageInfo();
  if (fmtHndl == NULL) return ii;
  NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
  return nanoPar->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint nanoscopeReadMetaDataProc (FormatHandle *fmtHndl, bim::uint , int group, int tag, int type) {
  if (fmtHndl == NULL) return 1;
  return read_nanoscope_metadata (fmtHndl, group, tag, type);
}

char* nanoscopeReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return NULL;
  return read_text_nanoscope_metadata ( fmtHndl );
}

bim::uint nanoscopeAddMetaDataProc (FormatHandle *) {
  return 1;
}




//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint nanoscopeReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;
  fmtHndl->pageNumber = page;
  return read_nanoscope_image( fmtHndl );
}

bim::uint nanoscopeWriteImageProc ( FormatHandle * ) {
  return 1;
}





//****************************************************************************
// LineReader
//****************************************************************************

LineReader::LineReader( FormatHandle *newHndl )
{
  fmtHndl = newHndl;
  
  if (fmtHndl == NULL) return;
  if (fmtHndl->stream == NULL) return;
  if ( xseek(fmtHndl, 0, SEEK_SET) != 0) return;
  eolines = false;
  loadBuff();
}

LineReader::~LineReader()
{

}

bool LineReader::loadBuff()
{
  bufsize = xread( fmtHndl, buf, 1, BIM_LINE_BUF_SIZE );
  bufpos = 0;
  if (bufsize <= 0) return false;
  return true;
}

bool LineReader::readNextLine()
{
  if (eolines) return false;
  prline = "";

  while ( (buf[bufpos] != 0xA) && (buf[bufpos] != 0xD) && (buf[bufpos] != 0x1A) ) 
  {
    if (bufpos >= bufsize) loadBuff();  
    if (bufpos >= bufsize) { eolines=true; break; }
    prline += buf[bufpos]; 
    ++bufpos;
  }

  if (buf[bufpos] == 0xA) ++bufpos;
  if (buf[bufpos] == 0xD) bufpos+=2;
  if (bufpos >= bufsize) loadBuff();  
  if (bufpos >= bufsize) eolines=true;
  if (buf[bufpos] == 0x1A) eolines=true;

  return true;
}

bool LineReader::isEOLines()
{
  return eolines;
}

std::string LineReader::line()
{
  return prline;
}

const char* LineReader::line_c_str()
{
  return prline.c_str();
}

//****************************************************************************
// TokenReader
//****************************************************************************

bool TokenReader::isKeyTag()
{
  if ( (prline[0]=='\\') && (prline[1]=='*') ) return true;
  return false;
}

bool TokenReader::isImageTag()
{
  if (isKeyTag())
  {
    //if (prline.compare("\\*Ciao force image list") == 0) return false;
    if (prline.find("image list", 0) != -1) 
      return true;
  }

  return false;
}

bool TokenReader::isEndTag()
{
  if (prline.compare("\\*File list end") == 0) return true;
  return false;
}


bool TokenReader::compareTag( const char *tag )
{
  if (prline.compare(tag) == 0) return true;
  return false;
}

bool TokenReader::isTag ( const char *tag )
{
  if (prline.find(tag) != -1) return true;
  return false;
}

int TokenReader::readParamInt ( const char *tag )
{
  int res=0;
  char ss[1024];
  sprintf(ss, "%s %%d", tag);

  sscanf( prline.c_str(), ss, &res );
  return res;
}

void TokenReader::readTwoParamDouble ( const char *tag, double *p1, double *p2 ) {
  char ss[1024];

  float pA=-1, pB=-1;
  sprintf(ss, "%s %%f %%f", tag);
  sscanf( prline.c_str(), ss, &pA, &pB );

  *p1 = pA; *p2 = pB;
}

void TokenReader::readTwoDoubleRemString ( const char *tag, double *p1, double *p2, std::string *str ) {
  char ss[1024], st[1024];

  float pA=-1, pB=-1;
  sprintf(ss, "%s %%f %%f %%s", tag);
  sscanf( prline.c_str(), ss, &pA, &pB, st );

  *p1 = pA; *p2 = pB;
  *str = st;
}

std::string TokenReader::readImageDataString ( const char *tag ) {
  xstring s = prline;
  //std::string str = s.section("[", "]");
  return s.section("\"", "\"");
}


TokenReader::TokenReader( FormatHandle *newHndl )
                : LineReader( newHndl )
{


}


//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

FormatItem nanoscopeItems[1] = {
  {
    "NANOSCOPE",            // short name, no spaces
    "NanoScope II/III", // Long format name
    "nan",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 1, 1, 16, 16, 1 } 
  }
};

FormatHeader nanoscopeHeader = {

  sizeof(FormatHeader),
  "1.0.1",
  "NANOSCOPE CODEC",
  "NANOSCOPE CODEC",
  
  12,                      // 0 or more, specify number of bytes needed to identify the file
  {1, 1, nanoscopeItems},   //dimJpegSupported,
  
  nanoscopeValidateFormatProc,
  // begin
  nanoscopeAquireFormatProc, //AquireFormatProc
  // end
  nanoscopeReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  nanoscopeOpenImageProc, //OpenImageProc
  nanoscopeCloseImageProc, //CloseImageProc 

  // info
  nanoscopeGetNumPagesProc, //GetNumPagesProc
  nanoscopeGetImageInfoProc, //GetImageInfoProc


  // read/write
  nanoscopeReadImageProc, //ReadImageProc 
  NULL, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  nanoscopeReadMetaDataProc, //ReadMetaDataProc
  nanoscopeAddMetaDataProc,  //AddMetaDataProc
  nanoscopeReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  nanoscope_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* nanoscopeGetFormatHeader(void)
{
  return &nanoscopeHeader;
}

} // extern C





