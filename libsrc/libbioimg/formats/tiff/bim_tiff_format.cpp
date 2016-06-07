/*****************************************************************************
  TIFF support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    03/29/2004 22:23 - First creation
    01/23/2007 20:42 - fixes in warning reporting
        
  Ver : 4
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "xtiffio.h"
#include "bim_tiff_format.h"

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>

using namespace bim;

void getImageInfo(TiffParams *tiffParams);
ImageInfo initImageInfo();

//#include "bim_tiff_format_io.cpp"
// IO functions defs
void init_image_palette( TIFF *tif, ImageInfo *info );
bim::uint read_tiff_metadata (FormatHandle *fmtHndl, TiffParams *tifParams, int group, int tag, int type);
char* read_text_tiff_metadata ( FormatHandle *fmtHndl, TiffParams *tifParams );
bim::uint tiff_append_metadata (FormatHandle *fmtHndl, TagMap *hash );
int read_tiff_image(FormatHandle *fmtHndl, TiffParams *tifParams);
int write_tiff_image(FormatHandle *fmtHndl, TiffParams *tifParams, ImageBitmap *img = NULL, bool subscale = false);

int read_tiff_image_level(FormatHandle *fmtHndl, TiffParams *tifParams, bim::uint page, bim::uint level);
int read_tiff_image_tile(FormatHandle *fmtHndl, TiffParams *tifParams, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level);
int ometiff_read_image_level(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams, bim::uint page, bim::uint level);
int ometiff_read_image_tile(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level);

bool omeTiffIsValid(bim::TiffParams *par);
bool isValidTiffFluoview(bim::TiffParams *par);
bool psiaIsTiffValid(bim::TiffParams *par);
bool lsmIsTiffValid(bim::TiffParams *par);
bool stkIsTiffValid(bim::TiffParams *par);
bool isValidTiffAndor(bim::TiffParams *par);

void omeTiffGetCurrentPageInfo(bim::TiffParams *par);
void fluoviewGetCurrentPageInfo(TiffParams *tiffParams);
void psiaGetCurrentPageInfo(TiffParams *tiffParams);
void lsmGetCurrentPageInfo(TiffParams *tiffParams);

int omeTiffGetInfo (bim::TiffParams *par);
int stkGetInfo (bim::TiffParams *par);
int psiaGetInfo (bim::TiffParams *par);
int fluoviewGetInfo (bim::TiffParams *par);
int lsmGetInfo (bim::TiffParams *par);

bim::int32 stkGetNumPlanes(TIFF *tif);



//****************************************************************************
// inits
//****************************************************************************

TiffParams::TiffParams() {
  this->info = initImageInfo();
  this->tiff = NULL;
  this->subType = tstGeneric;
}

// ----------------------------------------------------
// TIFF Pyramid

PyramidInfo::PyramidInfo() {
    this->init();
}

void PyramidInfo::init() {
    this->format = pyrFmtNone;
    this->number_levels = 1;
    this->scales.resize(1, 1.0);
    this->directory_offsets.resize(1, 0);
}

void PyramidInfo::addLevel(const double &scale, const bim::uint64 &offset) {
    ++this->number_levels;
    this->scales.resize(this->number_levels);
    this->scales[this->number_levels - 1] = scale;
    this->directory_offsets.resize(this->number_levels);
    this->directory_offsets[this->number_levels - 1] = offset;
}


//****************************************************************************
// STATIC FUNCTIONS THAT MIGHT BE PROVIDED BY HOST AND CALLING STUBS FOR THEM
//****************************************************************************

static ProgressProc  hostProgressProc  = NULL;
static ErrorProc     hostErrorProc     = NULL;
static TestAbortProc hostTestAbortProc = NULL;
static MallocProc    hostMallocProc    = NULL;
static FreeProc      hostFreeProc      = NULL;

static void localTiffProgressProc(long done, long total, char *descr) {
  if (hostProgressProc != NULL) hostProgressProc( done, total, descr );
}

static void localTiffErrorProc(int val, char *descr) {
  if (hostErrorProc != NULL) hostErrorProc( val, descr );
}

static bool localTiffTestAbortProc ( void ) {
  if (hostTestAbortProc != NULL) {
    if (hostTestAbortProc() == 1) return true; else return false;
  } else
    return false;
}

static void* localTiffMallocProc (bim::uint64 size) {
  if (hostMallocProc != NULL) 
    return hostMallocProc( size );
  else {
    //void *p = (void *) new char[size];
    void *p = (void *) _TIFFmalloc(size);  
    return p;
  }
}

static void* localTiffFreeProc (void *p) {
  if (hostFreeProc != NULL) 
    return hostFreeProc( p );
  else {
    //unsigned char *pu = (unsigned char*) p;
    //if (p != NULL) delete pu;
	  
	  if (p != NULL) _TIFFfree( p );
    return NULL;
  }
}

static void setLocalTiffFunctions ( FormatHandle *fmtHndl ) {
  hostProgressProc  = fmtHndl->showProgressProc; 
  hostErrorProc     = fmtHndl->showErrorProc; 
  hostTestAbortProc = fmtHndl->testAbortProc; 
  hostMallocProc    = fmtHndl->mallocProc; 
  hostFreeProc      = fmtHndl->freeProc; 
}

static void resetLocalTiffFunctions ( ) {
  hostProgressProc  = NULL; 
  hostErrorProc     = NULL; 
  hostTestAbortProc = NULL; 
  hostMallocProc    = NULL; 
  hostFreeProc      = NULL; 
}


//****************************************************************************
// CALLBACKS
//****************************************************************************

static tiff_size_t tiff_read(thandle_t handle, tiff_data_t data, tiff_size_t size) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  return (tiff_size_t) xread( fmtHndl, data, 1, size );
}

static tiff_size_t tiff_write(thandle_t handle, tiff_data_t data, tiff_size_t size) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  if ( fmtHndl->io_mode != IO_WRITE ) return 0;
  return (tiff_size_t) xwrite( fmtHndl, data, 1, size );
}

static tiff_offs_t tiff_seek(thandle_t handle, tiff_offs_t offset, int whence) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  if ( xseek( fmtHndl, offset, whence ) == 0 )
    return (tiff_offs_t) xtell(fmtHndl);
  else
    return 0;
}

static int tiff_close(thandle_t handle) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  xflush( fmtHndl );
  return xclose( fmtHndl );
}

static tiff_offs_t tiff_size(thandle_t handle) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  return xsize( fmtHndl );
}

static int tiff_mmap(thandle_t /*handle*/, tiff_data_t* /*data*/, tiff_offs_t* /*size*/) {
  return 1;
}

static void tiff_unmap(thandle_t /*handle*/, tiff_data_t /*data*/, tiff_offs_t /*size*/) {

}

//****************************************************************************
//
// FORMAT DEMANDED FUNTIONS
//
//****************************************************************************

//----------------------------------------------------------------------------
// UTILITARY FUNCTIONS
//----------------------------------------------------------------------------

unsigned int tiffGetNumberOfPages( TiffParams *par ) {
    unsigned int i=0;
    TIFF *tif = par->tiff;
    if (tif == NULL) return i;
    ImageInfo *info = &par->info;
  
    // if STK then get number of pages in special way
    if (par->subType == tstStk) {
    return stkGetNumPlanes( tif );
    }

    // very slow method for large tiff images with many pages
    /*
    TIFFSetDirectory(tif, 0);
    while (TIFFLastDirectory(tif) == 0) {
        if ( TIFFReadDirectory(tif) == 0) break;
        i++;
    }
    i++;
    return i;
    */

    // uses patched libtiff 4.0.3, tiff function returns uint16 which might not be enough
    unsigned int pages = TIFFNumberOfDirectories(tif);
    return pages;
}

void tiffReadResolution( TIFF *tif, bim::uint &units, double &xRes, double &yRes) {
  if (tif == NULL) return;
  
  float xresolution = 0;
  float yresolution = 0;
  short resolutionunit = 0;

  units = 0; xRes = 0; yRes = 0;

  if ( ( TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT , &resolutionunit) ) &&
       ( TIFFGetField(tif, TIFFTAG_XRESOLUTION , &xresolution) ) &&
       ( TIFFGetField(tif, TIFFTAG_YRESOLUTION , &yresolution) ) )
  {
    units = resolutionunit;
    xRes = xresolution;
    yRes = yresolution;
  }

  // here we need to read specific info here to define resolution correctly


}

bim::uint getTiffMode( TIFF *tif) {
  if (tif == NULL) return IM_GRAYSCALE;

  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 samplesperpixel = 1;
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

  if (photometric == PHOTOMETRIC_RGB && samplesperpixel==3) return IM_RGB;
  //if (photometric == PHOTOMETRIC_RGB && samplesperpixel==4) return IM_RGBA;
  if (photometric == PHOTOMETRIC_CIELAB) return IM_LAB;
  if (photometric == PHOTOMETRIC_ICCLAB) return IM_LAB;
  if (photometric == PHOTOMETRIC_ITULAB) return IM_LAB;
  if (photometric == PHOTOMETRIC_YCBCR) return IM_RGB; // return IM_YCbCr; // we convert in the driver
  if (photometric == PHOTOMETRIC_PALETTE) return IM_INDEXED;
  if (samplesperpixel > 1) return IM_MULTI;
    
  return IM_GRAYSCALE;
}

void detectTiffPyramid(TiffParams *tiffParams) {
    if (tiffParams == NULL) return;
    TIFF *tif = tiffParams->tiff;
    if (!tif) return;
    ImageInfo *info = &tiffParams->info;
    PyramidInfo *pyramid = &tiffParams->pyramid;

    pyramid->init();
    bim::uint32 sub_file_type = 10000;
    bim::uint64 current_dir = TIFFCurrentDirectory(tif);
    bim::uint32 width = 0, height=0, w = 0, h=0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    //----------------------------------------------------------------
    // Adobe Photoshop SubIFD pyramidal version
    //----------------------------------------------------------------
    bim::uint16 subIFDsCount = 0;
    bim::uint64* _subIFDs = NULL;
    TIFFGetField(tif, TIFFTAG_SUBFILETYPE, &sub_file_type);
    TIFFGetField(tif, TIFFTAG_SUBIFD, &subIFDsCount, &_subIFDs);
    if (sub_file_type == 0 && _subIFDs && subIFDsCount > 0) {
        // copy _subIFDs before it's reallocated by TIFFSetSubDirectory
        std::vector<bim::uint64> subIFDs(subIFDsCount, 0);
        if (_subIFDs) memcpy(&subIFDs[0], _subIFDs, subIFDsCount*sizeof(bim::uint64));

        int i = 0;
        bim::uint64 subdiroffset = subIFDs[i];
        while (TIFFSetSubDirectory(tif, subdiroffset) > 0) {
            
            if (TIFFGetField(tif, TIFFTAG_SUBFILETYPE, &sub_file_type) != 1) break;
            if (sub_file_type != FILETYPE_REDUCEDIMAGE) break;

            if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) == 0) break;
            double scale = (double)w / (double)width;
            pyramid->addLevel(scale, subdiroffset);
            ++i;
            // proper way to navigate subifds is by nextdiroffsets, photoshop uses this
            subdiroffset = tif->tif_nextdiroff;
            // libtiff and others write subdiroffsets in TIFFTAG_SUBIFD and do not set nextdiroffsets, fix for this
            if (subdiroffset == 0 && subIFDsCount > i) {
                subdiroffset = subIFDs[i];
            }
        }
        if (pyramid->number_levels > 1) {
            pyramid->format = PyramidInfo::pyrFmtSubDirs;
        }
    }

    //----------------------------------------------------------------
    // ImageMagick multi-page pyramidal version
    // the first directory is not guaranteed to have subfile type
    //----------------------------------------------------------------
    double prev_sz[2] = { width, height };
    if (pyramid->number_levels < 2) {
        TIFFSetDirectory(tif, current_dir);
        if (tif->tif_nextdiroff > 0) {
            bim::uint64 subdiroffset = tif->tif_nextdiroff;
            while (TIFFSetSubDirectory(tif, subdiroffset) > 0) {
                if (TIFFGetField(tif, TIFFTAG_SUBFILETYPE, &sub_file_type) == 1) {
                    // in case of properly stored file with sub-file-type
                    if (sub_file_type != FILETYPE_REDUCEDIMAGE) break;
                    if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) == 0) break;
                    double scale = (double)w / (double)width;
                    pyramid->addLevel(scale, subdiroffset);
                    subdiroffset = tif->tif_nextdiroff;
                } else {
                    // in case of an image stored without sub-file-type image size should be exactly half of previous page
                    // to qualify for a resolution level
                    prev_sz[0] /= 2.0;
                    prev_sz[1] /= 2.0;
                    if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) == 0) break;
                    if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h) == 0) break;
                    // a bit of wiggle room for sizes
                    if (fabs(prev_sz[0] - w) > 2.0 || fabs(prev_sz[1] - h) > 2.0) break;
                    double scale = (double)w / (double)width;
                    pyramid->addLevel(scale, subdiroffset);
                    subdiroffset = tif->tif_nextdiroff;
                }
            }
        }
        if (pyramid->number_levels > 1) {
            pyramid->format = PyramidInfo::pyrFmtTopDirs; // imagemagick style multi-page pyramid
        }
    }

    //----------------------------------------------------------------
    // finish
    //----------------------------------------------------------------
    
    // return to parent directory
    TIFFSetDirectory(tif, current_dir);
    info->number_levels = pyramid->number_levels;
}

void initPageInfo(TiffParams *tiffParams) {
  if (tiffParams == NULL) return;
  TIFF *tif = tiffParams->tiff;
  ImageInfo *info = &tiffParams->info;
  if (!tif) return;

  bim::uint32 height = 0; 
  bim::uint32 width = 0; 
  bim::uint16 bitspersample = 1;
  bim::uint16 samplesperpixel = 1;
  bim::uint16 sampleformat = 1;
  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 compression = COMPRESSION_NONE;
  bim::uint16 planarconfig;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
  TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleformat);

  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarconfig);

  if (photometric==PHOTOMETRIC_YCBCR && planarconfig==PLANARCONFIG_CONTIG && 
      compression==COMPRESSION_JPEG) {
    TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
    bitspersample = 8;
    samplesperpixel = 3;
  }

  if (photometric==PHOTOMETRIC_LOGLUV && planarconfig==PLANARCONFIG_CONTIG && 
      (compression==COMPRESSION_SGILOG ||compression==COMPRESSION_SGILOG24 )) {
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
    bitspersample = 8;
    samplesperpixel = 3;
  }

  if (photometric==PHOTOMETRIC_LOGL && compression==COMPRESSION_SGILOG) {
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
    bitspersample = 8;
  }

  info->width = width;
  info->height = height;

  info->samples = samplesperpixel;
  info->depth = bitspersample;
  info->pixelType = FMT_UNSIGNED;

  if (sampleformat == SAMPLEFORMAT_INT)
    info->pixelType = FMT_SIGNED;
  else
  if (sampleformat == SAMPLEFORMAT_IEEEFP)
    info->pixelType = FMT_FLOAT;


  if( !TIFFIsTiled(tif) ) {
    info->tileWidth = 0;
    info->tileHeight = 0; 
  } else {
    bim::uint32 columns, rows;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH,  &columns);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &rows);
    info->tileWidth = columns;
    info->tileHeight = rows; 
  }
    
  info->transparentIndex = 0;
  info->transparencyMatting = 0;

  info->imageMode = getTiffMode( tif );
  tiffReadResolution( tif, info->resUnits, info->xRes, info->yRes);
  detectTiffPyramid(tiffParams);
}

void getCurrentPageInfo(TiffParams *tiffParams) {
  if (tiffParams == NULL) return;
  TIFF *tif = tiffParams->tiff;
  ImageInfo *info = &tiffParams->info;
  if (!tif) return;

  if (tiffParams->subType!=tstFluoview && tiffParams->subType!=tstPsia && tiffParams->subType!=tstAndor)
    init_image_palette( tif, info );


  if ( tiffParams->subType == tstFluoview || tiffParams->subType == tstAndor )
    fluoviewGetCurrentPageInfo(tiffParams);
  else
  if ( tiffParams->subType == tstPsia )
    psiaGetCurrentPageInfo(tiffParams);
  else
  if ( tiffParams->subType == tstCzLsm ) 
    lsmGetCurrentPageInfo(tiffParams);
  else
  if ( tiffParams->subType == tstOmeTiff || tiffParams->subType == tstOmeBigTiff ) 
    omeTiffGetCurrentPageInfo(tiffParams);
}

void getImageInfo(TiffParams *par) {
  if (par == NULL) return;
  TIFF *tif = par->tiff;
  ImageInfo *info = &par->info;
  if (!tif) return;

  info->ver = sizeof(ImageInfo);
  
  initPageInfo( par );

  // read to which tiff sub type image pertence
  par->subType = tstGeneric;
  if (tif->tif_flags&TIFF_BIGTIFF) par->subType = tstBigTiff;

  if (stkIsTiffValid( par )) {
    par->subType = tstStk;
    stkGetInfo( par );
  } else
  if (psiaIsTiffValid( par )) {
    par->subType = tstPsia;
    psiaGetInfo ( par );
  } else
  if (isValidTiffFluoview(par)) {
    par->subType = tstFluoview;
    fluoviewGetInfo ( par );
  } else
  if (isValidTiffAndor(par)) {
    par->subType = tstAndor;
    fluoviewGetInfo ( par );
  } else
  if (lsmIsTiffValid( par )) {
    // lsm has thumbnails for each image, discard those
    par->subType = tstCzLsm;
    lsmGetInfo ( par );
  } else
  if (omeTiffIsValid(par)) {
    par->subType = tstOmeTiff;
    if (tif->tif_flags&TIFF_BIGTIFF) par->subType = tstOmeBigTiff;
    omeTiffGetInfo ( par );
  } else {
      // the generic TIFF case
      info->number_pages = tiffGetNumberOfPages( par ); // dima: takes a while to read
      PyramidInfo *pyramid = &par->pyramid;
      if (info->number_levels>1 && pyramid->format == PyramidInfo::pyrFmtTopDirs) {
          info->number_pages -= (info->number_levels - 1);
      }
  }

  getCurrentPageInfo( par );
}

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int tiffValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < 4) return -1;
  unsigned char *mag_num = (unsigned char *) magic;

  // ignore Canon raw files, CR2 and CRW
  if (length>20) {
    if (memcmp(mag_num+8, "CR", 2)==0) return -1; // CR2
    if (memcmp(mag_num+6, "HEAPCCDR", 8)==0) return -1; // CRW
  }

  // ignore Nikon NEF and Adobe DNG
  if (fileName) {
      xstring filename(fileName);
      filename = filename.toLowerCase();
      if (filename.endsWith(".nef") || filename.endsWith(".dng"))
          return -1;
  }

  if (memcmp(magic,d_magic_tiff_CLLT,4) == 0) return 0;
  if (memcmp(magic,d_magic_tiff_CLBG,4) == 0) return 0;
  if (memcmp(magic,d_magic_tiff_MDLT,4) == 0) return 0;
  if (memcmp(magic,d_magic_tiff_MDBG,4) == 0) return 0;
  if (memcmp(magic,d_magic_tiff_BGLT,4) == 0) return 0;
  if (memcmp(magic,d_magic_tiff_BGBG,4) == 0) return 0;
  return -1;
}

FormatHandle tiffAquireFormatProc( void ) {
  return initFormatHandle();
}

void tiffCloseImageProc (FormatHandle *fmtHndl);
void tiffReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  tiffCloseImageProc ( fmtHndl );  
  resetLocalTiffFunctions( );
}

//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void tiffSetWriteParameters (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  TiffParams *par = (TiffParams *) fmtHndl->internalParams;
  fmtHndl->compression = COMPRESSION_LZW;
  par->info.tileWidth = par->info.tileHeight = 0;
  if (!fmtHndl->options) return;

  xstring str = fmtHndl->options;
  std::vector<xstring> options = str.split( " " );
  if (options.size() < 1) return;
  
  int i = -1;
  while (i<(int)options.size()-1) {
    ++i;

    if (options[i] == "quality" && options.size() - i>0) {
        i++;
        fmtHndl->quality = options[i].toInt(90);
        continue;
    } else if ( options[i]=="compression" && options.size()-i>0 ) {
      ++i;
      if (options[i] == "none")     fmtHndl->compression = COMPRESSION_NONE;
      if (options[i] == "fax")      fmtHndl->compression = COMPRESSION_CCITTFAX4;
      if (options[i] == "lzw")      fmtHndl->compression = COMPRESSION_LZW;
      if (options[i] == "packbits") fmtHndl->compression = COMPRESSION_PACKBITS;
      if (options[i] == "zip")      fmtHndl->compression = COMPRESSION_ADOBE_DEFLATE;
      if (options[i] == "jpeg")     fmtHndl->compression = COMPRESSION_JPEG;
      if (options[i] == "lzma")     fmtHndl->compression = COMPRESSION_LZMA;
      //if (options[i] == "jxr")      fmtHndl->compression = COMPRESSION_JXR; // JPEG-XR
      continue;
    } else if (options[i] == "tiles" && options.size() - i>0) {
        par->info.tileWidth = par->info.tileHeight = options[++i].toInt(0);
        continue;
    } else if (options[i] == "pyramid" && options.size() - i>0) {
        xstring pf = options[++i];
        if (pf == "subdirs") par->pyramid.format = PyramidInfo::pyrFmtSubDirs;
        if (pf == "topdirs") par->pyramid.format = PyramidInfo::pyrFmtTopDirs;
        continue;
    }

  } // while
}

void tiffCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  TiffParams *par = (TiffParams *) fmtHndl->internalParams;

  if ( (par != NULL) && (par->tiff != NULL) ) {
    XTIFFClose( par->tiff );
    par->tiff = NULL;
  }

  // close stream handle
  if ( fmtHndl->stream && !isCustomReading(fmtHndl) ) xclose( fmtHndl );

  if (fmtHndl->io_mode == IO_WRITE && fmtHndl->metaData) {
      //exiv_write_metadata(fmtHndl->metaData, fmtHndl);
  }

  if (fmtHndl->internalParams != NULL) {
    delete par;
    fmtHndl->internalParams = NULL;
  }
}

bim::uint tiffOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (!fmtHndl) return 1;
  setLocalTiffFunctions( fmtHndl );

  tiffCloseImageProc( fmtHndl );
  TiffParams *tiffpar = new TiffParams();
  fmtHndl->internalParams = tiffpar;  

  TIFFSetWarningHandler(0);
  TIFFSetErrorHandler(0);

  if (io_mode == IO_WRITE) {
    std::string mode = "w";
    if (fmtHndl->subFormat==tstBigTiff || fmtHndl->subFormat==tstOmeBigTiff) mode = "w8";

    //extern TIFF* XTIFFOpenW(const wchar_t* name, const char* mode);

    if (isCustomWriting(fmtHndl) != true) {
#ifdef BIM_WIN
        bim::xstring fn(fmtHndl->fileName);
        tiffpar->tiff = XTIFFOpenW(fn.toUTF16().c_str(), mode.c_str());
#else
        tiffpar->tiff = XTIFFOpen(fmtHndl->fileName, mode.c_str());
#endif
    } else {
        tiffpar->tiff = XTIFFClientOpen(fmtHndl->fileName, mode.c_str(), // "wm"
            (thandle_t)fmtHndl, tiff_read, tiff_write, tiff_seek, tiff_close, tiff_size, tiff_mmap, tiff_unmap);
    }
    tiffSetWriteParameters (fmtHndl); 
    if (fmtHndl->subFormat==tstOmeTiff || fmtHndl->subFormat==tstOmeBigTiff)
      tiffpar->subType = tstOmeTiff;
    else
      tiffpar->subType = tstGeneric;
  } else { // if reading

    
    // Use libtiff internal methods where possible, especially with the upcoming libtiff 4
    //if (!fmtHndl->stream && !isCustomReading(fmtHndl) )
    //  fmtHndl->stream = fopen( fmtHndl->fileName, "rb" );
    //if (!fmtHndl->stream) return 1;

      if (isCustomReading(fmtHndl) != true) {
#ifdef BIM_WIN
          bim::xstring fn(fmtHndl->fileName);
          tiffpar->tiff = XTIFFOpenW(fn.toUTF16().c_str(), "r");
#else
          tiffpar->tiff = XTIFFOpen(fmtHndl->fileName, "r");
#endif
      } else {
          tiffpar->tiff = XTIFFClientOpen(fmtHndl->fileName, "r", // "rm"
              (thandle_t)fmtHndl, tiff_read, tiff_write, tiff_seek, tiff_close, tiff_size, tiff_mmap, tiff_unmap);
      }

    if (tiffpar->tiff != NULL) {
      tiffpar->ifds.read(tiffpar->tiff); // dima: very slow for large tiff images
      getImageInfo(tiffpar);
      fmtHndl->subFormat = tiffpar->subType;
    }
  }

  if (tiffpar->tiff == NULL) return 1;

  return 0;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint tiffGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  TiffParams *tiffpar = (TiffParams *) fmtHndl->internalParams;

  if (tiffpar->tiff == NULL) return 0;

  return (int) tiffpar->info.number_pages;
}


ImageInfo tiffGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
  ImageInfo ii = bim::initImageInfo();

  if (fmtHndl == NULL) return ii;
  if (fmtHndl->internalParams == NULL) return ii;
  TiffParams *tiffpar = (TiffParams *) fmtHndl->internalParams;
  TIFF *tif = tiffpar->tiff;
  if (tif == NULL) return ii;

  fmtHndl->pageNumber = page_num;
  fmtHndl->subFormat = tiffpar->subType;

  bim::uint64 currentDir = TIFFCurrentDirectory(tif);

  // now must read correct page and set image parameters
  if (fmtHndl->pageNumber!=0 && currentDir != fmtHndl->pageNumber) {
    if (tiffpar->subType != tstStk)
      TIFFSetDirectory(tif, fmtHndl->pageNumber);
    getImageInfo(tiffpar);
    getCurrentPageInfo( tiffpar );
  }

  return tiffpar->info;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint tiffAppendMetadataProc (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  return tiff_append_metadata(fmtHndl, hash );
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint tiffReadImageProc  ( FormatHandle *fmtHndl, bim::uint page )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  TiffParams *tiffpar = (TiffParams *) fmtHndl->internalParams;

  if (tiffpar->tiff == NULL) return 1;
  fmtHndl->pageNumber = page;
  
  return read_tiff_image(fmtHndl, tiffpar);
}

bim::uint tiffWriteImageProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  TiffParams *par = (TiffParams *) fmtHndl->internalParams;
  if (par->tiff == NULL) return 1;
  return write_tiff_image(fmtHndl, par); //, fmtHndl->image);
}

bim::uint tiffReadImageLevelProc(FormatHandle *fmtHndl, bim::uint page, bim::uint level) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    TiffParams *par = (TiffParams *)fmtHndl->internalParams;
    if (par->tiff == NULL) return 1;

    if (par->subType == tstOmeTiff || par->subType == tstOmeBigTiff) {
        return ometiff_read_image_level(fmtHndl, par, page, level);
    } else if (par->subType == tstGeneric || par->subType == tstBigTiff) {
        return read_tiff_image_level(fmtHndl, par, page, level);
    }
    return 1;
}

bim::uint tiffReadImageTileProc(FormatHandle *fmtHndl, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    TiffParams *par = (TiffParams *)fmtHndl->internalParams;
    if (par->tiff == NULL) return 1;

    if (par->subType == tstOmeTiff || par->subType == tstOmeBigTiff) {
        return ometiff_read_image_tile(fmtHndl, par, page, xid, yid, level);
    } else if (par->subType == tstGeneric || par->subType == tstBigTiff) {
        return read_tiff_image_tile(fmtHndl, par, page, xid, yid, level);
    }
    return 1;
}

//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

#define D_TIFF_NUM_FORMATS 10

FormatItem tiffItems[D_TIFF_NUM_FORMATS] = {
  {
    "TIFF",            // short name, no spaces
    "Tagged Image File Format", // Long format name
    "tif|tiff|fax|geotiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "STK",            // short name, no spaces
    "Metamorph Stack", // Long format name
    "stk",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "PSIA",            // short name, no spaces
    "AFM PSIA TIFF", // Long format name
    "tif|tiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "FLUOVIEW",            // short name, no spaces
    "Fluoview TIFF", // Long format name
    "tif|tiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "LSM",            // short name, no spaces
    "Carl Zeiss LSM 5/7", // Long format name
    "lsm",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "OME-TIFF",            // short name, no spaces
    "Open Microscopy TIFF", // Long format name
    "ome.tif|ome.tiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "BigTIFF",            // short name, no spaces
    "Tagged Image File Format (64bit)", // Long format name
    "btf|tif|tiff|geotiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "OME-BigTIFF",            // short name, no spaces
    "Open Microscopy BigTIFF", // Long format name
    "ome.btf|ome.tif|ome.tiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "ANDOR",            // short name, no spaces
    "Andor TIFF", // Long format name
    "tif|tiff",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }, {
    "MicroManager",            // short name, no spaces
    "MicroManager OME-TIFF", // Long format name
    "tif|tiff",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )    
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  }
};

FormatHeader tiffHeader = {

  sizeof(FormatHeader),
  "4.0.4",
  "BIM TIFF CODEC",
  "Tagged Image File Format variants",
  
  4,                      // 0 or more, specify number of bytes needed to identify the file
  {1, D_TIFF_NUM_FORMATS, tiffItems},   //tiffSupported,
  
  tiffValidateFormatProc,
  // begin
  tiffAquireFormatProc, //AquireFormatProc
  // end
  tiffReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  tiffOpenImageProc, //OpenImageProc
  tiffCloseImageProc, //CloseImageProc 

  // info
  tiffGetNumPagesProc, //GetNumPagesProc
  tiffGetImageInfoProc, //GetImageInfoProc


  // read/write
  tiffReadImageProc, //ReadImageProc 
  tiffWriteImageProc, //WriteImageProc
  tiffReadImageTileProc, //ReadImageTileProc
  NULL, //WriteImageTileProc
  tiffReadImageLevelProc, //ReadImageLevelProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //ReadImagePreviewProc
  
  // meta data
  NULL, //ReadMetaDataProc
  NULL, //AddMetaDataProc
  NULL, //ReadMetaDataAsTextProc
  tiffAppendMetadataProc, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" { 
  FormatHeader* tiffGetFormatHeader(void) { return &tiffHeader; } 
} // extern C





