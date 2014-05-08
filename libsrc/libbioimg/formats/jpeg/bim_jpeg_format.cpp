/*****************************************************************************
  JPEG support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/22/2004 13:06 - First creation
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
    2010-06-24 15:11 - EXIF/IPTC extraction
        
  Ver : 3
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <xstring.h>

#include "bim_jpeg_format.h"


// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

//#include <stdio.h> // jpeglib needs this to be pre-included
#include <setjmp.h>

#ifdef FAR
#undef FAR
#endif

#if defined(__RPCNDR_H__)
#define HAVE_BOOLEAN
#define boolean unsigned int
#endif

extern "C" {
//#define XMD_H // Shut JPEGlib up.

#include <jpeglib.h>
#include <jerror.h>

//#ifdef const
//#undef const // Remove crazy C hackery in jconfig.h
//#endif
}


using namespace bim;

#include "bim_jpeg_format_io.cpp"


//****************************************************************************
// Misc
//****************************************************************************

JpegParams::JpegParams() {
    i = initImageInfo(); 
    cinfo = 0;
    iod_src = 0;
    jerr = new my_error_mgr;
}

JpegParams::~JpegParams() {
    if (cinfo) {
        jpeg_destroy_decompress(cinfo);
        delete cinfo;  
    }
    if (iod_src) delete iod_src;  
    if (jerr) delete jerr;  
}


//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int jpegValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < 3) return -1;
  if (memcmp(magic,"\377\330\377",3) == 0) return 0;
  return -1;
}

FormatHandle jpegAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void jpegCloseImageProc(FormatHandle *fmtHndl);
void jpegReleaseFormatProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    jpegCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void jpegSetWriteParameters  (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;

  fmtHndl->order = 1; // use progressive encoding by default
  fmtHndl->quality = 95;

  if (!fmtHndl->options) return;
  xstring str = fmtHndl->options;
  std::vector<xstring> options = str.split( " " );
  if (options.size() < 1) return;
  
  int i = -1;
  while (i<(int)options.size()-1) {
    i++;

    if ( options[i]=="quality" && options.size()-i>0 ) {
      i++;
      fmtHndl->quality = options[i].toInt( 100 );
      continue;
    } else
    if ( options[i]=="progressive" && options.size()-i>0 ) {
      i++;    
      if (options[i]=="no") fmtHndl->order = 0;
      else
        fmtHndl->order = 1;      
      continue;
    }
    
  } // while
}

void jpegCloseImageProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;
    delete par;
    fmtHndl->internalParams = 0;
    xclose( fmtHndl );
}

bim::uint jpegOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) jpegCloseImageProc (fmtHndl);  

  fmtHndl->io_mode = io_mode;
  if ( io_mode == IO_READ ) {
      fmtHndl->internalParams = (void *) new bim::JpegParams();
      bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;
      
      xopen(fmtHndl);
      if (!fmtHndl->stream) { 
          jpegCloseImageProc (fmtHndl); 
          return 1; 
      };

      if ( !jpegGetImageInfo( fmtHndl ) ) { jpegCloseImageProc (fmtHndl); return 1; };
  }

  if (io_mode == IO_WRITE) {
      jpegSetWriteParameters(fmtHndl);
  }
  return 0;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint jpegGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  return 1;
}


ImageInfo jpegGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
  ImageInfo ii = initImageInfo();
  if (!fmtHndl) return ii;
  fmtHndl->pageNumber = page_num;
  fmtHndl->subFormat = 0;
  bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint jpegAddMetaDataProc (FormatHandle * /*fmtHndl*/) {
  return 1;
}


bim::uint jpegReadMetaDataProc (FormatHandle * /*fmtHndl*/, bim::uint /*page*/, int /*group*/, int /*tag*/, int /*type*/) {
  return 1;
}

char* jpegReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return NULL;
  return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint jpegReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (!fmtHndl) return 1;
  if (!fmtHndl->stream) return 1;
  fmtHndl->pageNumber = page;
  return read_jpeg_image( fmtHndl );
}

bim::uint jpegWriteImageProc ( FormatHandle *fmtHndl ) {
  if (!fmtHndl) return 1;
  xopen(fmtHndl);
  if (!fmtHndl->stream) return 1;
  bim::uint res = write_jpeg_image( fmtHndl );
  xflush( fmtHndl );
  xclose( fmtHndl );
  return res;
}


//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

FormatItem jpegItems[1] = {
  {
    "JPEG",            // short name, no spaces
    "JPEG File Interchange Format", // Long format name
    "jpg|jpeg|jpe|jif|jfif",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 4, 8, 8, 1 } 
  }
};

FormatHeader jpegHeader = {

  sizeof(FormatHeader),
  "1.1.0",
  "DIMIN JPEG CODEC",
  "JPEG-JFIF Compliant CODEC",
  
  3,                      // 0 or more, specify number of bytes needed to identify the file
  {1, 1, jpegItems},   // jpegSupported,
  
  jpegValidateFormatProc,
  // begin
  jpegAquireFormatProc, //AquireFormatProc
  // end
  jpegReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  jpegOpenImageProc, //OpenImageProc
  jpegCloseImageProc, //CloseImageProc 

  // info
  jpegGetNumPagesProc, //GetNumPagesProc
  jpegGetImageInfoProc, //GetImageInfoProc


  // read/write
  jpegReadImageProc, //ReadImageProc 
  jpegWriteImageProc, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //jpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  jpegReadMetaDataProc, //ReadMetaDataProc
  jpegAddMetaDataProc,  //AddMetaDataProc
  jpegReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  jpeg_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* jpegGetFormatHeader(void)
{
  return &jpegHeader;
}

} // extern C


