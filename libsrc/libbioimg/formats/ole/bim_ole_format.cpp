/*****************************************************************************
  Olympus Image Binary (OIB) format support
  Copyright (c) 2008, Center for Bio Image Informatics, UCSB
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2008-06-04 14:26:14 - First creation
    2008-09-15 19:04:47 - Fix for older files with unordered streams
    2008-11-06 13:36:43 - Parse preferred channel mapping
            
  Ver : 4
*****************************************************************************/

#include <cmath>
#include <cstring>

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <xstring.h>
#include <bim_metatags.h>

#include "bim_ole_format.h"

using namespace bim;
//#include "bim_oib_format_io.cpp"
//#include "bim_zvi_format_io.cpp"

int zviGetImageInfo( FormatHandle *fmtHndl );
int oibGetImageInfo( FormatHandle *fmtHndl );
bim::uint zvi_append_metadata (FormatHandle *fmtHndl, TagMap *hash );
bim::uint oib_append_metadata (FormatHandle *fmtHndl, TagMap *hash );
int zvi_read_image(FormatHandle *fmtHndl);
int read_oib_image(FormatHandle *fmtHndl);

//****************************************************************************
// FORMAT REQUIRED FUNCTIONS
//****************************************************************************

int oleValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < BIM_OLE_MAGIC_SIZE) return -1;
  if (memcmp( magic, ole::magic, BIM_OLE_MAGIC_SIZE ) != 0) return -1;
  // this is a very fast way of simply testing if the file is an OLE directory
  // better testing will happen in the read
  return 0;
}

FormatHandle oleAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void oleCloseImageProc ( FormatHandle *fmtHndl);
void oleReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  oleCloseImageProc ( fmtHndl );  
}

//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void oleCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams != NULL)
    delete (ole::Params *) fmtHndl->internalParams;
  fmtHndl->internalParams = NULL;
}

bim::uint oleOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) oleCloseImageProc (fmtHndl);  
  fmtHndl->internalParams = (void *) new ole::Params();
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;

  if (io_mode == IO_READ) {
    par->ole_format = ole::FORMAT_UNKNOWN;

    // try to load OLE storage
    par->storage = new POLE::Storage( fmtHndl->fileName ); // POLE uses utf-8 encoded filenames
    par->storage->open();
    if( par->storage->result() != POLE::Storage::Ok ) return 1;

    // test for incoming OLE format
    if (zvi::Directory::isValid(par->storage))
      if (zviGetImageInfo( fmtHndl )==0) {
        par->ole_format = ole::FORMAT_ZVI;
        fmtHndl->subFormat = par->ole_format-1;
      }
    
    if (par->ole_format == ole::FORMAT_UNKNOWN) 
      if (oibGetImageInfo( fmtHndl )==0) {
        par->ole_format = ole::FORMAT_OIB;
        fmtHndl->subFormat = par->ole_format-1;
      }

    if (par->ole_format == ole::FORMAT_UNKNOWN) 
      oleCloseImageProc(fmtHndl);

    return (par->ole_format == ole::FORMAT_UNKNOWN);
  }
  
  return 1;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint oleGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  return par->i.number_pages;
}


ImageInfo oleGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint /*page_num*/ ) {
  if (!fmtHndl) return initImageInfo();
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint oleReadMetaDataProc (FormatHandle *fmtHndl, bim::uint /*page*/, int group, int tag, int type) {
  if (!fmtHndl) return 1;
  if (!fmtHndl->internalParams) return 1;
  return 0;
}

char* oleReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  return NULL;
}

bim::uint oleAddMetaDataProc (FormatHandle * /*fmtHndl*/) {
  return 1;
}

bim::uint oleAppendMetadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (!fmtHndl) return 1;
  if (!fmtHndl->internalParams) return 1;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;

  if (par->ole_format == ole::FORMAT_ZVI) return zvi_append_metadata (fmtHndl, hash );
  else
  if (par->ole_format == ole::FORMAT_OIB) return oib_append_metadata (fmtHndl, hash );
  return 1;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint oleReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (!fmtHndl) return 1;
  if (!fmtHndl->internalParams) return 1;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  fmtHndl->pageNumber = page;

  if (par->ole_format == ole::FORMAT_ZVI) return zvi_read_image( fmtHndl );
  else
  if (par->ole_format == ole::FORMAT_OIB) return read_oib_image( fmtHndl );
  return 1;
}


//****************************************************************************
// EXPORTED FUNCTION
//****************************************************************************

#define BIM_OLE_NUM_FORMTAS 2

FormatItem oleFormatItems[BIM_OLE_NUM_FORMTAS] = {
  {
    "OIB",            // short name, no spaces
    "Olympus Image Binary", // Long format name
    "oib",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 1, 1, 16, 16, 1 } 
  },
  {
    "ZVI",            // short name, no spaces
    "Zeiss ZVI", // Long format name
    "zvi",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 1, 1, 16, 16, 1 } 
  }
};

FormatHeader bimOibHeader = {

  sizeof(FormatHeader),
  "2.0.0",
  "OLE CODEC for OIB and ZVI",
  "OLE CODEC for OIB and ZVI",
  
  BIM_OLE_MAGIC_SIZE,         // 0 or more, specify number of bytes needed to identify the file
  {1, BIM_OLE_NUM_FORMTAS, oleFormatItems},   //dimJpegSupported,
  
  oleValidateFormatProc,
  // begin
  oleAquireFormatProc, //AquireFormatProc
  // end
  oleReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  oleOpenImageProc, //OpenImageProc
  oleCloseImageProc, //CloseImageProc 

  // info
  oleGetNumPagesProc, //GetNumPagesProc
  oleGetImageInfoProc, //GetImageInfoProc


  // read/write
  oleReadImageProc, //ReadImageProc 
  NULL, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  NULL, //ReadMetaDataProc
  NULL,  //AddMetaDataProc
  NULL, //ReadMetaDataAsTextProc
  oleAppendMetadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* oleGetFormatHeader(void) {
  return &bimOibHeader;
}

} // extern C





