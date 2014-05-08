/*****************************************************************************
  RAW support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    12/01/2005 15:27 - First creation
    2007-07-12 21:01 - reading raw
        
  Ver : 2

*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bim_raw_format.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

using namespace bim;

//****************************************************************************
//
// INTERNAL STRUCTURES
//
//****************************************************************************

bool rawGetImageInfo( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  RawParams *rawPar = (RawParams *) fmtHndl->internalParams;
  ImageInfo *info = &rawPar->i;  

  *info = initImageInfo();
  info->number_pages = 1;
  info->samples = 1;

  //fmtHndl->compression - offset
  //fmtHndl->quality - endian (0/1)  
  rawPar->header_offset = 0;
  rawPar->big_endian = false;

  if (fmtHndl->stream == NULL) return false;

  return true;
}

bool rawWriteImageInfo( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  RawParams *rawPar = (RawParams *) fmtHndl->internalParams;
  
  ImageBitmap *img = fmtHndl->image;
  if (img == NULL) return false;

  std::string infname = fmtHndl->fileName;
  if (infname.size() <= 1) return false; 
  infname += ".info";

  std::string inftext = getImageInfoText(&img->i);


  FILE *stream;
  if( (stream = fopen( infname.c_str(), "wb" )) != NULL ) {
    fwrite( inftext.c_str(), sizeof( char ), inftext.size(), stream );
    fclose( stream );
  }

  return true;
}

//****************************************************************************
//
// FORMAT DEMANDED FUNTIONS
//
//****************************************************************************


//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int rawValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  return -1;
}

FormatHandle rawAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void rawCloseImageProc (FormatHandle *fmtHndl);
void rawReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  rawCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

static RawParams bim_raw_params;

void rawCloseImageProc (FormatHandle *fmtHndl) {
    if (!fmtHndl) return;
    xclose ( fmtHndl );
}

bim::uint rawOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) rawCloseImageProc (fmtHndl);  
  //fmtHndl->internalParams = (void *) new RawParams [1];
  fmtHndl->internalParams = (void *) &bim_raw_params;

  fmtHndl->io_mode = io_mode;
  xopen(fmtHndl);
  if (!fmtHndl->stream) {
      rawCloseImageProc(fmtHndl);
      return 1;
  };

  if ( io_mode == IO_READ ) {
    if ( !rawGetImageInfo( fmtHndl ) ) { 
        rawCloseImageProc (fmtHndl); 
        return 1; 
    };
  }
  return 0;
}


bim::uint rawFOpenImageProc (FormatHandle *fmtHndl, char* fileName, ImageIOModes io_mode)
{
  fmtHndl->fileName = fileName;
  return rawOpenImageProc(fmtHndl, io_mode);
}

bim::uint rawIOpenImageProc (FormatHandle *fmtHndl, char* fileName, 
                                         BIM_IMAGE_CLASS *image, ImageIOModes io_mode)
{
  fmtHndl->fileName = fileName;
  fmtHndl->image    = image;
  return rawOpenImageProc(fmtHndl, io_mode);
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint rawGetNumPagesProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;

  return 1;
}


ImageInfo rawGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num )
{
  ImageInfo ii = initImageInfo();
  page_num;

  if (fmtHndl == NULL) return ii;
  RawParams *rawPar = (RawParams *) fmtHndl->internalParams;

  return rawPar->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint rawAddMetaDataProc (FormatHandle *fmtHndl)
{
  fmtHndl=fmtHndl;
  return 1;
}


bim::uint rawReadMetaDataProc (FormatHandle *fmtHndl, bim::uint page, int group, int tag, int type)
{
  fmtHndl; page; group; tag; type;
  return 1;
}

char* rawReadMetaDataAsTextProc ( FormatHandle *fmtHndl )
{
  fmtHndl;
  return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

//****************************************************************************
// READ PROC
//****************************************************************************

static int read_raw_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  RawParams *rawPar = (RawParams *) fmtHndl->internalParams;
  //ImageInfo *info = &rawPar->i;  
  ImageBitmap *img = fmtHndl->image;
  
  //-------------------------------------------------
  // init the image
  //-------------------------------------------------
  //if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  //-------------------------------------------------
  // read image data
  //-------------------------------------------------
  unsigned int plane_size  = getImgSizeInBytes( img ); 
  unsigned int cur_plane   = fmtHndl->pageNumber;
  unsigned int header_size = rawPar->header_offset; 

  // seek past header size plus number of planes
  if ( xseek( fmtHndl, header_size + plane_size*cur_plane*img->i.samples, SEEK_SET ) != 0) return 1;
  
  for (unsigned int sample=0; sample<img->i.samples; ++sample) {
    if (xread( fmtHndl, img->bits[sample], 1, plane_size ) != plane_size) return 1;

    // now it has to know whether to swap the data and so on...
    if ( bim::bigendian != (int)rawPar->big_endian) {
      if (img->i.depth == 16)
        swapArrayOfShort((uint16*) img->bits[sample], plane_size/2);

      if (img->i.depth == 32)
        swapArrayOfLong((uint32*) img->bits[sample], plane_size/4);
    }
  }


  return 0;
}

//****************************************************************************
// WRITE PROC
//****************************************************************************

static int write_raw_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  RawParams *rawPar = (RawParams *) fmtHndl->internalParams;
  ImageBitmap *img = fmtHndl->image;

  //-------------------------------------------------
  // write image data
  //-------------------------------------------------
  unsigned long plane_size = getImgSizeInBytes( img );
  
  for (unsigned int sample=0; sample<img->i.samples; ++sample)
    if (xwrite( fmtHndl, img->bits[sample], 1, plane_size ) != plane_size) return 1;

  xflush( fmtHndl );
  return 0;
}


bim::uint rawReadImageProc  ( FormatHandle *fmtHndl, bim::uint page )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;

  fmtHndl->pageNumber = page;
  return read_raw_image( fmtHndl );
}

bim::uint rawWriteImageProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;

  //if (fmtHndl->pageNumber <= 1) rawWriteImageInfo( fmtHndl );

  return write_raw_image( fmtHndl );
}



//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

FormatItem rawItems[1] = {
{
    "RAW",              // short name, no spaces
    "RAW image pixels", // Long format name
    "raw",              // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    0, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 } 
  }  
};

FormatHeader rawHeader = {

  sizeof(FormatHeader),
  "1.0.0",
  "DIMIN RAW CODEC",
  "RAW CODEC",
  
  2,                     // 0 or more, specify number of bytes needed to identify the file
  {1, 1, rawItems},   // dimJpegSupported,
  
  rawValidateFormatProc,
  // begin
  rawAquireFormatProc, //AquireFormatProc
  // end
  rawReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  rawOpenImageProc, //OpenImageProc
  rawCloseImageProc, //CloseImageProc 

  // info
  rawGetNumPagesProc, //GetNumPagesProc
  rawGetImageInfoProc, //GetImageInfoProc


  // read/write
  rawReadImageProc, //ReadImageProc 
  rawWriteImageProc, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  rawReadMetaDataProc, //ReadMetaDataProc
  rawAddMetaDataProc,  //AddMetaDataProc
  rawReadMetaDataAsTextProc, //ReadMetaDataAsTextProc

  NULL,
  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* rawGetFormatHeader(void)
{
  return &rawHeader;
}

} // extern C


