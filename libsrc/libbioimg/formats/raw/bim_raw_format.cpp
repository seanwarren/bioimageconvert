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

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_raw_format.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::RawParams::RawParams() {
    i = initImageInfo();
    header_offset = 0;
    big_endian = false;
    interleaved = false;
}

bim::RawParams::~RawParams() {
    // pass
}


//****************************************************************************
//
// INTERNAL STRUCTURES
//
//****************************************************************************

bool rawGetImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  RawParams *par = (RawParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  

  *info = initImageInfo();
  info->number_pages = 1;
  info->samples = 1;

  //fmtHndl->compression - offset
  //fmtHndl->quality - endian (0/1)  
  par->header_offset = 0;
  par->big_endian = false;

  if (fmtHndl->stream == NULL) return false;

  return true;
}

bool rawWriteImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  RawParams *par = (RawParams *) fmtHndl->internalParams;
  
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

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

#define BIM_FORMAT_RAW_MAGIC_SIZE 20

#define BIM_RAW_FORMAT_RAW   0
#define BIM_RAW_FORMAT_NRRD  1
#define BIM_RAW_FORMAT_MHD   2

int rawValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_RAW_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *)magic;

    //if (memcmp(mag_num, "MMMMRawT", 8) == 0) return BIM_RAW_FORMAT_NRRD;
    //if (memcmp(mag_num, "IIII", 4) == 0) return BIM_RAW_FORMAT_MHD;

    //if (length >= 32)
    //if (memcmp(mag_num + 25, "ARECOYK", 7) == 0) return BIM_DCRAW_FORMAT_DCRAW; //Contax

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

void rawCloseImageProc (FormatHandle *fmtHndl) {
    if (!fmtHndl) return;
    xclose ( fmtHndl );
    bim::RawParams *par = (bim::RawParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint rawOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) rawCloseImageProc (fmtHndl);  
  bim::RawParams *par = new bim::RawParams();
  fmtHndl->internalParams = (void *)par;

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

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint rawGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;

  return 1;
}


ImageInfo rawGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
  ImageInfo ii = initImageInfo();
  page_num;

  if (fmtHndl == NULL) return ii;
  RawParams *par = (RawParams *) fmtHndl->internalParams;

  return par->i;
}

//----------------------------------------------------------------------------
// read
//----------------------------------------------------------------------------

template <typename T>
void read_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out;
    raw += sample;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T *pp = p + x;
        T *rr = raw + x*samples;
        *pp = *rr;
    } // for x
}

static int read_raw_image(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    //ImageInfo *info = &par->i;  
    ImageBitmap *img = fmtHndl->image;
    ImageInfo *info = &img->i;

    //-------------------------------------------------
    // init the image
    //-------------------------------------------------
    info->imageMode = IM_GRAYSCALE;
    //if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

    //-------------------------------------------------
    // read image data
    //-------------------------------------------------
    unsigned int plane_size = getImgSizeInBytes(img);
    unsigned int cur_plane = fmtHndl->pageNumber;
    unsigned int header_size = par->header_offset;

    // seek past header size plus number of planes
    if (xseek(fmtHndl, header_size + plane_size*cur_plane*img->i.samples, SEEK_SET) != 0) return 1;

    if (!par->interleaved || img->i.samples==1) {
        for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
            if (xread(fmtHndl, img->bits[sample], 1, plane_size) != plane_size) return 1;
        }
    }
    else { // read interleaved data
        bim::uint64 buffer_sz = plane_size * img->i.samples;
        std::vector<unsigned char> buffer(buffer_sz);
        unsigned char *buf = (unsigned char *)&buffer[0];
        if (xread(fmtHndl, buf, 1, buffer_sz) != buffer_sz) return 1;

        for (int s = 0; s < img->i.samples; ++s) {
            if (img->i.depth == 8 && img->i.pixelType == FMT_UNSIGNED)
                read_channel<bim::uint8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 16 && img->i.pixelType == FMT_UNSIGNED)
                read_channel<bim::uint16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 32 && img->i.pixelType == FMT_UNSIGNED)
                read_channel<bim::uint32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 64 && img->i.pixelType == FMT_UNSIGNED)
                read_channel<bim::uint64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 8 && img->i.pixelType == FMT_SIGNED)
                read_channel<bim::int8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 16 && img->i.pixelType == FMT_SIGNED)
                read_channel<bim::int16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 32 && img->i.pixelType == FMT_SIGNED)
                read_channel<bim::int32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 64 && img->i.pixelType == FMT_SIGNED)
                read_channel<bim::int64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
                //if (img->i.depth == 16 && img->i.pixelType == FMT_FLOAT)
                //    read_channel<bim::float16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
                //else
            if (img->i.depth == 32 && img->i.pixelType == FMT_FLOAT)
                read_channel<bim::float32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else
            if (img->i.depth == 64 && img->i.pixelType == FMT_FLOAT)
                read_channel<bim::float64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
        } // for sample
    }

    // swap endianness
    if (bim::bigendian != (int)par->big_endian)
    for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
        if (img->i.depth == 16)
            swapArrayOfShort((uint16*)img->bits[sample], plane_size / 2);
        else if (img->i.depth == 32)
            swapArrayOfLong((uint32*)img->bits[sample], plane_size / 4);
        else if (img->i.depth == 64)
            swapArrayOfDouble((float64*)img->bits[sample], plane_size / 8);
    }

    return 0;
}

bim::uint rawReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->stream == NULL) return 1;
    fmtHndl->pageNumber = page;
    return read_raw_image(fmtHndl);
}

//----------------------------------------------------------------------------
// write
//----------------------------------------------------------------------------

template <typename T>
void write_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out;
    raw += sample;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T *pp = p + x;
        T *rr = raw + x*samples;
        *rr = *pp;
    } // for x
}

static int write_raw_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  RawParams *par = (RawParams *) fmtHndl->internalParams;
  ImageBitmap *img = fmtHndl->image;

  //-------------------------------------------------
  // write image data
  //-------------------------------------------------
  unsigned long plane_size = getImgSizeInBytes( img );
  
  if (!par->interleaved || img->i.samples == 1) {
      std::vector<unsigned char> buffer(plane_size);
      unsigned char *buf = (unsigned char *)&buffer[0];

      for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
          memcpy(buf, img->bits[sample], plane_size);

          if (bim::bigendian != (int)par->big_endian) {
              if (img->i.depth == 16)
                  swapArrayOfShort((uint16*)buf, plane_size / 2);
              else if (img->i.depth == 32)
                  swapArrayOfLong((uint32*)buf, plane_size / 4);
              else if (img->i.depth == 64)
                  swapArrayOfDouble((float64*)buf, plane_size / 8);
          }

          if (xwrite(fmtHndl, buf, 1, plane_size) != plane_size) return 1;
      }
  }
  else {
      bim::uint64 buffer_sz = plane_size * img->i.samples;
      std::vector<unsigned char> buffer(buffer_sz);
      unsigned char *buf = (unsigned char *)&buffer[0];

      for (int s = 0; s < img->i.samples; ++s) {
          if (img->i.depth == 8 && img->i.pixelType == FMT_UNSIGNED)
              write_channel<bim::uint8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 16 && img->i.pixelType == FMT_UNSIGNED)
              write_channel<bim::uint16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 32 && img->i.pixelType == FMT_UNSIGNED)
              write_channel<bim::uint32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 64 && img->i.pixelType == FMT_UNSIGNED)
              write_channel<bim::uint64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 8 && img->i.pixelType == FMT_SIGNED)
              write_channel<bim::int8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 16 && img->i.pixelType == FMT_SIGNED)
              write_channel<bim::int16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 32 && img->i.pixelType == FMT_SIGNED)
              write_channel<bim::int32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 64 && img->i.pixelType == FMT_SIGNED)
              write_channel<bim::int64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
              //if (img->i.depth == 16 && img->i.pixelType == FMT_FLOAT)
              //    write_channel<bim::float16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
              //else
          if (img->i.depth == 32 && img->i.pixelType == FMT_FLOAT)
              write_channel<bim::float32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
          else
          if (img->i.depth == 64 && img->i.pixelType == FMT_FLOAT)
              write_channel<bim::float64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);

          if (bim::bigendian != (int)par->big_endian) {
              if (img->i.depth == 16)
                  swapArrayOfShort((uint16*)buf, buffer_sz / 2);
              else if (img->i.depth == 32)
                  swapArrayOfLong((uint32*)buf, buffer_sz / 4);
              else if (img->i.depth == 64)
                  swapArrayOfDouble((float64*)buf, buffer_sz / 8);
          }

          if (xwrite(fmtHndl, buf, 1, buffer_sz) != buffer_sz) return 1;
      } // for sample
  }

  xflush( fmtHndl );
  return 0;
}

bim::uint rawWriteImageProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;
  //if (fmtHndl->pageNumber <= 1) rawWriteImageInfo( fmtHndl );
  return write_raw_image( fmtHndl );
}

//----------------------------------------------------------------------------
// metadata
//----------------------------------------------------------------------------

bim::uint raw_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    if (!hash) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageBitmap *img = fmtHndl->image;

    //-------------------------------------------
    // channel names
    //-------------------------------------------
    for (unsigned int i = 0; i<img->i.samples; ++i) {
        hash->set_value(xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), xstring::xprintf("Channel %d", i));
    }

    return 0;
}



//****************************************************************************
// EXPORTED
//****************************************************************************

#define BIM_RAW_NUM_FORMATS 3

FormatItem rawItems[BIM_RAW_NUM_FORMATS] = {
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
  },
  {
      "NRRD",              // short name, no spaces
      "NRRD", // Long format name
      "nrrd",              // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      1, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
      { 0, 0, 0, 0, 0, 0, 0, 0 }
  },
  {
      "MHD",              // short name, no spaces
      "MHD", // Long format name
      "mhd",              // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      1, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
      { 0, 0, 0, 0, 0, 0, 0, 0 }
  }
};

FormatHeader rawHeader = {

  sizeof(FormatHeader),
  "3.0.0",
  "RAW CODEC",
  "RAW CODEC",
  
  BIM_FORMAT_RAW_MAGIC_SIZE,                     // 0 or more, specify number of bytes needed to identify the file
  { 1, BIM_RAW_NUM_FORMATS, rawItems },   // dimJpegSupported,
  
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
  NULL, //ReadMetaDataProc
  NULL,  //AddMetaDataProc
  NULL, //ReadMetaDataAsTextProc

  raw_append_metadata,
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


