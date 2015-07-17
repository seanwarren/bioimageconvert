/*****************************************************************************
  Digital Camera RAW formats support 
  Copyright (c) 2012, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2012, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>

#include "bim_dcraw_format.h"

// windows: use secure C libraries with VS2005 or higher
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
  #pragma message(">>>>> bim_dcraw_format.cpp: ignoring secure libraries")
#endif 

#if defined WIN32 || defined WIN64
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif
#include <libraw/libraw.h>

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::DCRawParams::DCRawParams() {
  i = initImageInfo(); 
  processor = new LibRaw();
}

bim::DCRawParams::~DCRawParams() {
  if (processor) delete processor;
}

//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_DCRAW_MAGIC_SIZE 40

#define BIM_DCRAW_FORMAT_CANON    0
#define BIM_DCRAW_FORMAT_KODAK    1
#define BIM_DCRAW_FORMAT_MINOLTA  2
#define BIM_DCRAW_FORMAT_NIKON    3
#define BIM_DCRAW_FORMAT_OLYMPUS  4
#define BIM_DCRAW_FORMAT_PENTAX   5
#define BIM_DCRAW_FORMAT_FUJI     6
#define BIM_DCRAW_FORMAT_ROLLEI   7
#define BIM_DCRAW_FORMAT_SONY     8
#define BIM_DCRAW_FORMAT_SIGMA    9
#define BIM_DCRAW_FORMAT_DCRAW   10
#define BIM_DCRAW_FORMAT_DNG     11

int dcrawValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < BIM_FORMAT_DCRAW_MAGIC_SIZE) return -1;
  unsigned char *mag_num = (unsigned char *) magic;

  if (memcmp (mag_num, "MMMMRawT", 8)   == 0 ) return BIM_DCRAW_FORMAT_DCRAW;
  if (memcmp (mag_num, "IIII", 4)       == 0 ) return BIM_DCRAW_FORMAT_DCRAW;
  if (memcmp (mag_num, "\0MRM", 4)      == 0 ) return BIM_DCRAW_FORMAT_MINOLTA; // mrw - Konica/Minolta
  if (memcmp (mag_num, "IIRS", 4)       == 0 ) return BIM_DCRAW_FORMAT_OLYMPUS; // orf - Olympus
  if (memcmp (mag_num, "PXN", 3)        == 0 ) return BIM_DCRAW_FORMAT_DCRAW; // Logitech
  if (memcmp (mag_num, "FUJIFILM", 8)   == 0 ) return BIM_DCRAW_FORMAT_FUJI;
  if (memcmp (mag_num, "DSC-Image", 9)  == 0 ) return BIM_DCRAW_FORMAT_ROLLEI; // rollei
  if (memcmp (mag_num, "FOVb", 4)       == 0 ) return BIM_DCRAW_FORMAT_SIGMA; // x3f - Sigma / foveon chip
  //if (memcmp (mag_num, "\xff\xd8\xff\xe1", 4)       == 0 ) return 0; // PENTAX jpeg - use original jpeg reader

  if (length >= 14) 
    if (memcmp (mag_num+6, "HEAPCCDR", 8) == 0  ) return BIM_DCRAW_FORMAT_DCRAW;

  if (length >= 32) 
    if (memcmp (mag_num+25, "ARECOYK", 7) == 0  ) return BIM_DCRAW_FORMAT_DCRAW; //Contax

  // canon
  if (memcmp(mag_num, "II", 2)==0 || memcmp(mag_num, "MM", 2)==0) {
      if (memcmp(mag_num+8, "CR", 2)==0) return BIM_DCRAW_FORMAT_CANON; // CR2
      if (memcmp(mag_num+6, "HEAPCCDR", 8)==0) return BIM_DCRAW_FORMAT_CANON; // CRW
  }

  //II* - Nikon NEF and Adobe DNG are valid TIFF files accept them based on extension
  if (fileName && (memcmp(mag_num, "II", 2)==0 || memcmp(mag_num, "MM", 2)==0)) {
      xstring filename(fileName);
      filename = filename.toLowerCase();
      if (filename.endsWith(".nef")) return BIM_DCRAW_FORMAT_NIKON;
      if (filename.endsWith(".dng")) return BIM_DCRAW_FORMAT_DNG;
  }

  //if (memcmp (mag_num, "BM", 2) ) return 0; // BMQ got same magic as bmp...
  //if (memcmp (mag_num, "BR", 2) ) return 0;

  return -1;
}

FormatHandle dcrawAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void dcrawCloseImageProc (FormatHandle *fmtHndl);
void dcrawReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  dcrawCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void dcrawGetImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  bim::DCRawParams *par = (bim::DCRawParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  

  *info = initImageInfo();
  info->imageMode = IM_GRAYSCALE;
  info->tileWidth = 0;
  info->tileHeight = 0; 
  info->transparentIndex = 0;
  info->transparencyMatting = 0;
  info->lut.count = 0;

  // some of this might have to be updated while decoding the image
  info->number_pages = 1;
  info->width  = par->processor->imgdata.sizes.iwidth;
  info->height = par->processor->imgdata.sizes.iheight;
  info->depth = 16;
  info->pixelType = FMT_UNSIGNED;
  info->samples = 3;    
  info->number_pages = 1;
  info->imageMode = IM_RGB;
}

void dcrawCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  xclose ( fmtHndl );
  bim::DCRawParams *par = (bim::DCRawParams *) fmtHndl->internalParams;
  fmtHndl->internalParams = 0;
  delete par;
}

bim::uint dcrawOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) dcrawCloseImageProc (fmtHndl);  
  bim::DCRawParams *par = new bim::DCRawParams();
  fmtHndl->internalParams = (void *) par;

  if (io_mode == IO_READ) {
    try {
#if defined(BIM_WIN) && !defined(_DEBUG)
        bim::xstring fn(fmtHndl->fileName);
        if (par->processor->open_file((const wchar_t *)fn.toUTF16().c_str()) != LIBRAW_SUCCESS) {
#else
        if (par->processor->open_file(fmtHndl->fileName) != LIBRAW_SUCCESS) {
#endif       
          throw "Error opening DCRAW file";
        }
        dcrawGetImageInfo(fmtHndl);
    } catch(...) {
        dcrawCloseImageProc(fmtHndl); 
        return 1;
    }
  }
  else return 1;
  return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint dcrawGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  return 1;
}


ImageInfo dcrawGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
  ImageInfo ii = initImageInfo();
  if (fmtHndl == NULL) return ii;
  //fmtHndl->pageNumber = page_num;  
  bim::DCRawParams *par = (bim::DCRawParams *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint dcrawAddMetaDataProc (FormatHandle *fmtHndl) {
  fmtHndl=fmtHndl;
  return 1;
}


bim::uint dcrawReadMetaDataProc (FormatHandle *fmtHndl, bim::uint page, int group, int tag, int type) {
  if (fmtHndl == NULL) return 1;
  return 1;
}

char* dcrawReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint dcrawReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (fmtHndl == NULL) return 1;
  fmtHndl->pageNumber = page;
  bim::DCRawParams *par = (bim::DCRawParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  

  if (par->processor->unpack() != LIBRAW_SUCCESS)
      return 1;

  par->processor->imgdata.params.half_size      = 0;
  par->processor->imgdata.params.four_color_rgb = 0;
  par->processor->imgdata.params.highlight      = 0;
  par->processor->imgdata.params.user_flip      = -1;
  par->processor->imgdata.params.use_auto_wb    = 0;
  par->processor->imgdata.params.use_camera_wb  = 1;
  par->processor->imgdata.params.output_color   = 1;
  par->processor->imgdata.params.no_auto_bright = 1;
  par->processor->imgdata.params.bright         = 1.0;
  par->processor->imgdata.params.threshold      = 0;
  par->processor->imgdata.params.med_passes     = 0;
  par->processor->imgdata.params.gamm[0]        = 0.45;
  par->processor->imgdata.params.gamm[1]        = 4.5;
  par->processor->imgdata.params.output_bps     = 16;

  int ret = par->processor->dcraw_process();
  if (ret == LIBRAW_SUCCESS) {
      // dcraw_make_mem_image is only used to compute imgdata.color.curve
      libraw_processed_image_t *image = par->processor->dcraw_make_mem_image();
      if (image) LibRaw::dcraw_clear_mem(image);

      ImageBitmap *img = fmtHndl->image;
      if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

      unsigned short *pO = (unsigned short *) img->bits[0];
      unsigned short *p1 = (unsigned short *) img->bits[1];
      unsigned short *p2 = (unsigned short *) img->bits[2];
      for (unsigned int y=0; y<info->height; ++y) {
        xprogress( fmtHndl, y, info->height, "Reading DCRaw" );
        if (xtestAbort( fmtHndl ) == 1) break;  
        unsigned short *raw = par->processor->imgdata.image[info->width*y];
        for (unsigned int x=0; x<info->width; ++x) {
          *pO = par->processor->imgdata.color.curve[raw[0]];
          *p1 = par->processor->imgdata.color.curve[raw[1]];
          *p2 = par->processor->imgdata.color.curve[raw[2]];
          ++pO; ++p1; ++p2; raw+=4;
        } // for x
      } // for y
  } // if dcraw_process
  par->processor->recycle();
  return ret;
}

bim::uint dcrawWriteImageProc ( FormatHandle *fmtHndl ) {
  return 1;
  fmtHndl;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint dcraw_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (isCustomReading (fmtHndl)) return 1;
  
  try {
      bim::DCRawParams *par = (bim::DCRawParams *) fmtHndl->internalParams;
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "make", par->processor->imgdata.idata.make );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "model", par->processor->imgdata.idata.model );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "artist", par->processor->imgdata.other.artist );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "aperture", par->processor->imgdata.other.aperture );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "shutter", par->processor->imgdata.other.shutter );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "focal_length", par->processor->imgdata.other.focal_len );
      hash->append_tag( xstring(bim::CUSTOM_TAGS_PREFIX) + "iso_speed", par->processor->imgdata.other.iso_speed );
  } catch (...) {
  }

  // use EXIV2 to read metadata
  exiv_append_metadata (fmtHndl, hash );

  return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_DCRAW_NUM_FORMATS 12

FormatItem dcrawItems[BIM_DCRAW_NUM_FORMATS] = {
  { //0
    "CANON-RAW",            // short name, no spaces
    "Canon Digital Camera RAW", // Long format name
    "cr2|crw",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 1
    "KODAK-RAW",            // short name, no spaces
    "Kodak Digital Camera RAW", // Long format name
    "bay|dc2|dcr|k25|kdc",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 2
    "MINOLTA-RAW",            // short name, no spaces
    "Konica/Minolta Digital Camera RAW", // Long format name
    "mrw",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 3
    "NIKON-RAW",            // short name, no spaces
    "Nikon Digital Camera RAW", // Long format name
    "nef",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 4
    "OLYMPUS-RAW",            // short name, no spaces
    "Olympus Digital Camera RAW", // Long format name
    "orf",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 5
    "PENTAX-RAW",            // short name, no spaces
    "Pentax Digital Camera RAW", // Long format name
    "pef",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 6
    "FUJI-RAW",            // short name, no spaces
    "Fuji Digital Camera RAW", // Long format name
    "raf",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 7
    "ROLLEI-RAW",            // short name, no spaces
    "Rollei Digital Camera RAW", // Long format name
    "rdc",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 8
    "SONY-RAW",            // short name, no spaces
    "Sony Digital Camera RAW", // Long format name
    "srf",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 9
    "SIGMA-RAW",            // short name, no spaces
    "Sigma Digital Camera RAW", // Long format name
    "x3f",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 10
    "DCRAW",            // short name, no spaces
    "Digital Cameras RAW", // Long format name
    "bmq|cs1|fff|mos",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }, { // 11
    "DNG",            // short name, no spaces
    "Adobe Digital Negative", // Long format name
    "dng",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 1, 8, 16, 1 } 
  }
};


FormatHeader dcrawHeader = {

  sizeof(FormatHeader),
  "1.0.16",
  "DCRAW",
  "Digital Camera Raw",
  
  BIM_FORMAT_DCRAW_MAGIC_SIZE,
  {1, BIM_DCRAW_NUM_FORMATS, dcrawItems},
  
  dcrawValidateFormatProc,
  // begin
  dcrawAquireFormatProc, //AquireFormatProc
  // end
  dcrawReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  dcrawOpenImageProc, //OpenImageProc
  dcrawCloseImageProc, //CloseImageProc 

  // info
  dcrawGetNumPagesProc, //GetNumPagesProc
  dcrawGetImageInfoProc, //GetImageInfoProc


  // read/write
  dcrawReadImageProc, //ReadImageProc 
  NULL, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  dcrawReadMetaDataProc, //ReadMetaDataProc
  dcrawAddMetaDataProc,  //AddMetaDataProc
  dcrawReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  dcraw_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* dcrawGetFormatHeader(void)
{
  return &dcrawHeader;
}

} // extern C


