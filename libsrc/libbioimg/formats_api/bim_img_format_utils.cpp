/*******************************************************************************

  Defines Image Format Utilities
  rely on: DimFiSDK version: 1
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  Image file structure:
    
    1) Page:   each file may contain 1 or more pages, each page is independent

    2) Sample: in each page each pixel can contain 1 or more samples
               preferred number of samples: 1 (GRAY), 3(xRGB) and 4(RGBA)

    3) Depth:  each sample can be of 1 or more bits of depth
               preferred depths are: 8 and 16 bits per sample

    4) Allocation: each sample is allocated having in mind:
               All lines are stored contiguasly from top to bottom where
               having each particular line is byte alligned, i.e.:
               each line is allocated using minimal necessary number of bytes
               to store given depth. Which for the image means:
               size = ceil( ( (width*depth) / 8 ) * height )

  As a result of Sample/Depth structure we can get images of different 
  Bits per Pixel as: 1, 8, 24, 32, 48 and any other if needed

  History:
    04/08/2004 11:57 - First creation
    10/10/2005 15:15 - Fixes in allocImg to read palette for images
    2008-06-27 14:57 - Fixes by Mario Emmenlauer to support large files
      
  ver: 4
        
*******************************************************************************/

#include "bim_img_format_utils.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#include <xstring.h>
#include <bim_metatags.h>

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

using namespace bim;

const char *dimNames[6] = { "none", "X", "Y", "C", "Z", "T" };

//------------------------------------------------------------------------------
// tests for provided callbacks
//------------------------------------------------------------------------------

bool bim::isCustomReading ( FormatHandle *fmtHndl ) {
  if ( ( fmtHndl->stream != NULL ) && 
       ( fmtHndl->readProc != NULL ) && 
       ( fmtHndl->seekProc != NULL ) 
     ) return true;
  return false;
}

bool bim::isCustomWriting ( FormatHandle *fmtHndl ) {
  if ( ( fmtHndl->stream != NULL ) &&
       ( fmtHndl->writeProc != NULL ) && 
       ( fmtHndl->seekProc  != NULL ) &&
       ( fmtHndl->flushProc != NULL )
     ) return true;
  return false;
}

//------------------------------------------------------------------------------
// Safe calls for callbacks
//------------------------------------------------------------------------------

void bim::xprogress ( FormatHandle *fmtHndl, uint64 done, uint64 total, char *descr) {
  if ( fmtHndl->showProgressProc != NULL ) 
    fmtHndl->showProgressProc ( done, total, descr );
}

void bim::xerror ( FormatHandle *fmtHndl, int val, char *descr) {
  if ( fmtHndl->showErrorProc != NULL ) 
    fmtHndl->showErrorProc ( val, descr );
}

int  bim::xtestAbort ( FormatHandle *fmtHndl ) {
  if ( fmtHndl->testAbortProc != NULL ) 
    return fmtHndl->testAbortProc ( );
  else
    return 0;
}

//------------------------------------------------------------------------------
// Safe calls for memory/io prototypes, if they are not supplied then
// standard functions are used
//------------------------------------------------------------------------------

void* bim::xmalloc( FormatHandle *fmtHndl, BIM_SIZE_T size ) {
  if ( fmtHndl->mallocProc != NULL ) 
    return fmtHndl->mallocProc ( size );
  else {
    void *p = (void *) new char[size];
    return p;
  }
}

void* bim::xfree( FormatHandle *fmtHndl, void *p ) {
  if ( fmtHndl->freeProc != NULL ) 
    return fmtHndl->freeProc( p );
  else {
    unsigned char *pu = (unsigned char*) p;
    if (p != NULL) delete pu;  
    return NULL;
  }
}

BIM_SIZE_T bim::xread ( FormatHandle *fmtHndl, void *buffer, BIM_SIZE_T size, BIM_SIZE_T count ) {
  if ( fmtHndl == NULL ) return 0;
  if ( fmtHndl->stream == NULL ) return 0;
  if ( fmtHndl->readProc != NULL ) 
    return fmtHndl->readProc( buffer, size, count, fmtHndl->stream );
  else
    return fread( buffer, size, count, (FILE *) fmtHndl->stream );
}

BIM_SIZE_T bim::xwrite ( FormatHandle *fmtHndl, void *buffer, BIM_SIZE_T size, BIM_SIZE_T count ) {
  if ( fmtHndl == NULL ) return 0;
  if ( fmtHndl->stream == NULL ) return 0;    
  if ( fmtHndl->writeProc != NULL ) 
    return fmtHndl->writeProc( buffer, size, count, fmtHndl->stream );
  else
    return fwrite( buffer, size, count, (FILE *) fmtHndl->stream );
}

int bim::xflush ( FormatHandle *fmtHndl ) {
  if ( fmtHndl == NULL ) return EOF;
  if ( fmtHndl->stream == NULL ) return EOF;  
  if ( fmtHndl->flushProc != NULL ) 
    return fmtHndl->flushProc( fmtHndl->stream );
  else
    return fflush( (FILE *) fmtHndl->stream );
}

int bim::xseek ( FormatHandle *fmtHndl, BIM_OFFSET_T offset, int origin ) {
  if ( fmtHndl == NULL ) return 1;
  if ( fmtHndl->stream == NULL ) return 1;

	off_t off_io = (off_t) offset;
	if ((BIM_OFFSET_T) off_io != offset) return -1;

  if ( fmtHndl->seekProc != NULL ) {
    return fmtHndl->seekProc( fmtHndl->stream, off_io, origin );
  } else {
    return fseek( (FILE *) fmtHndl->stream, off_io, origin );
  }
}

BIM_SIZE_T bim::xsize ( FormatHandle *fmtHndl ) {
  if ( fmtHndl == NULL ) return 0;
  if ( fmtHndl->stream == NULL ) return 0;   
  BIM_SIZE_T fsize = 0; 
  if ( fmtHndl->sizeProc != NULL ) 
    return fmtHndl->sizeProc( fmtHndl->stream );
  else {
    BIM_SIZE_T p = xtell(fmtHndl);
    fseek( (FILE *) fmtHndl->stream, 0, SEEK_END );
    BIM_SIZE_T end_p = xtell(fmtHndl);
    fseek( (FILE *) fmtHndl->stream, (long) p, SEEK_SET );
    return end_p;
  }
}

BIM_OFFSET_T bim::xtell ( FormatHandle *fmtHndl ) {
  if ( fmtHndl->stream == NULL ) return 0;  
  if ( fmtHndl->tellProc != NULL ) 
    return fmtHndl->tellProc( fmtHndl->stream );
  else
    return ftell( (FILE *) fmtHndl->stream );
}

int bim::xeof ( FormatHandle *fmtHndl ) {
  if ( fmtHndl->stream == NULL ) return 1;
  if ( fmtHndl->eofProc != NULL ) 
    return fmtHndl->eofProc( fmtHndl->stream );
  else
    return feof( (FILE *) fmtHndl->stream );
}

void bim::xopen(FormatHandle *fmtHndl) {
    if (!fmtHndl) return;
    if (isCustomReading(fmtHndl) == true) return;
#ifdef BIM_WIN
    xstring fn(fmtHndl->fileName);
    if (fmtHndl->io_mode == IO_WRITE)
        fmtHndl->stream = _wfopen(fn.toUTF16().c_str(), L"wb");
    else
        fmtHndl->stream = _wfopen(fn.toUTF16().c_str(), L"rb");
#else
    if (fmtHndl->io_mode == IO_WRITE)
        fmtHndl->stream = fopen(fmtHndl->fileName, "wb");
    else
        fmtHndl->stream = fopen(fmtHndl->fileName, "rb");
#endif
}

int bim::xclose ( FormatHandle *fmtHndl ) {
  if ( fmtHndl == NULL ) return EOF;
  if ( fmtHndl->stream == NULL ) return EOF;
  int res;
  if ( fmtHndl->closeProc != NULL ) 
    res = fmtHndl->closeProc( fmtHndl->stream );
  else
    res = fclose( (FILE *) fmtHndl->stream );

  fmtHndl->stream = NULL;
  return res;
}
   
//------------------------------------------------------------------------------
// SWAP TYPES
//------------------------------------------------------------------------------

void bim::swapData(int type, uint64 size, void* data) {
  if ( (type == TAG_SHORT) || (type == TAG_SSHORT) )
    swapArrayOfShort( (uint16*) data, size );

  if ( (type == TAG_LONG) || (type == TAG_SLONG) || (type == TAG_FLOAT) )
    swapArrayOfLong( (uint32*) data, size );

  if (type == TAG_RATIONAL)
    swapArrayOfLong( (uint32*) data, size );

  if (type == TAG_DOUBLE)
    swapArrayOfDouble( (float64*) data, size );
}

//------------------------------------------------------------------------------
// Init parameters
//------------------------------------------------------------------------------

FormatHandle bim::initFormatHandle()
{
  FormatHandle tp;
  tp.ver = sizeof(FormatHandle);
  
  tp.showProgressProc = NULL;
  tp.showErrorProc = NULL;
  tp.testAbortProc = NULL;
  tp.mallocProc = NULL;
  tp.freeProc = NULL;  

  tp.subFormat = 0;
  tp.pageNumber = 0;
  tp.resolutionLevel = 0;
  tp.quality = 0;
  tp.compression = 0;
  tp.order = 0;
  tp.metaData = NULL;
  tp.roiX = 0;
  tp.roiY = 0;
  tp.roiW = 0;
  tp.roiH = 0;

  tp.imageServicesProcs = NULL;
  tp.internalParams = NULL;
  tp.fileName = NULL;
  tp.parent = NULL;

  tp.stream  = NULL;
  tp.io_mode = IO_READ;

  tp.image = NULL;
  tp.options = NULL;
  
  tp.readProc  = NULL;
  tp.writeProc = NULL;
  tp.flushProc = NULL;
  tp.seekProc  = NULL;
  tp.sizeProc  = NULL;
  tp.tellProc  = NULL;
  tp.eofProc   = NULL;
  tp.closeProc = NULL;

  
  return tp;
}


ImageInfo bim::initImageInfo() {
  ImageInfo tp;
  tp.ver = sizeof(ImageInfo);
  
  tp.width = 0;
  tp.height = 0;
  
  tp.number_pages = 0;
  tp.number_levels = 0;
    
  tp.number_t = 1;
  tp.number_z = 1;

  tp.transparentIndex = 0;
  tp.transparencyMatting = 0;
    
  tp.imageMode = 0;
  tp.samples = 0;
  tp.depth = 0;
  tp.pixelType = FMT_UNSIGNED;
  tp.rowAlignment = TAG_BYTE;

  tp.resUnits = 0;  
  tp.xRes = 0;
  tp.yRes = 0;
  
  tp.tileWidth = 0;
  tp.tileHeight = 0;

  tp.lut.count = 0;

  tp.file_format_id = 0;

  uint i;

  // dimensions
  for (i=0; i<BIM_MAX_DIMS; ++i)
  {
    tp.dimensions[i].dim = DIM_0;
    tp.dimensions[i].description = NULL;
    tp.dimensions[i].ext = NULL;
  }

  tp.number_dims = 3;
  for (i=0; i<tp.number_dims; ++i)
  {
    tp.dimensions[i].dim = (ImageDims) (DIM_0+i+1);
  }

  // channels
  for (i=0; i<BIM_MAX_CHANNELS; ++i)
  {
    tp.channels[i].description = NULL;
    tp.channels[i].ext = NULL;
  }

  return tp;
}


//------------------------------------------------------------------------------
// ImageBitmap
//------------------------------------------------------------------------------

uint64 bim::getLineSizeInBytes(ImageBitmap *img) {
    return (uint64) ceil( ((double)(img->i.width * img->i.depth)) / 8.0 );
}

uint64 bim::getImgSizeInBytes(ImageBitmap *img) {
    return (uint64) ceil( ((double)(img->i.width * img->i.depth)) / 8.0 ) * img->i.height;
}

uint bim::getImgNumColors(ImageBitmap *img) {
    return (uint) pow( 2.0f, (float)(img->i.depth * img->i.samples) );
}

void bim::initImagePlanes(ImageBitmap *bmp) {
    if (!bmp) return;
    bmp->i = initImageInfo();
    for (uint i=0; i<512; i++)
        bmp->bits[i] = NULL;  
}

int bim::allocImg( ImageBitmap *img, uint w, uint h, uint samples, uint depth) {
    if (!img) return 1;
  
    for (uint sample=0; sample<img->i.samples; sample++) 
        if (img->bits[sample] != NULL) 
            xfree( &img->bits[sample] );

    initImagePlanes( img );

    img->i.width = w;
    img->i.height = h;
    img->i.imageMode = 0;
    img->i.samples = samples;
    img->i.depth = depth;
    uint64 size = getImgSizeInBytes( img );
    for (uint sample=0; sample<img->i.samples; sample++) {
        img->bits[sample] = new uchar [size];
        if (!img->bits[sample]) return 1;
    }
    return 0;
}

int bim::allocImg( FormatHandle *fmtHndl, ImageBitmap *img, uint w, uint h, uint samples, uint depth) {
    if (!img) return 1;
  
    for (uint sample=0; sample<img->i.samples; sample++) 
        if (img->bits[sample] != NULL) 
            img->bits[sample] = xfree( fmtHndl, img->bits[sample] );

    initImagePlanes( img );

    img->i.width = w;
    img->i.height = h;
    img->i.samples = samples;
    img->i.depth = depth;
    uint64 size = getImgSizeInBytes( img );

    for (uint sample=0; sample<img->i.samples; sample++) {
        img->bits[sample] = (uchar *) xmalloc( fmtHndl, size );
        if (img->bits[sample] == NULL) return 1;
    }
    return 0;
}

// alloc image using info
int bim::allocImg( FormatHandle *fmtHndl, ImageInfo *info, ImageBitmap *img) {
  if (!fmtHndl || !info || !img) return 1;

  ImageInfo iminfo = *info;
  if ( allocImg( fmtHndl, img, info->width, info->height, info->samples, info->depth ) == 0) {
      img->i = iminfo;
      return 0;
  }
  return 1;
}

// alloc handle image using info
int bim::allocImg( FormatHandle *fmtHndl, ImageInfo *info ) {
  return allocImg( fmtHndl, info, fmtHndl->image ); 
}

void bim::deleteImg(ImageBitmap *img) {
  if (!img) return;
  for (uint sample=0; sample<img->i.samples; ++sample) {
    if (img->bits[sample]) {
      void *p = img->bits[sample];
      for (uint i=0; i<img->i.samples; ++i)
        if (img->bits[i] == p) img->bits[i] = NULL;
      delete (uchar*) p;
    } // if channel found
  } // for samples
}

void bim::deleteImg( FormatHandle *fmtHndl, ImageBitmap *img) {
  if (!img) return;
  for (uint sample=0; sample<img->i.samples; ++sample) {
    if (img->bits[sample]) {
      void *p = img->bits[sample];
      for (unsigned int i=0; i<img->i.samples; ++i)
        if (img->bits[i] == p) img->bits[i] = NULL;
      xfree( fmtHndl, p );
    } // if channel found
  } // for samples
}

int bim::getSampleHistogram(ImageBitmap *img, long *hist, int sample)
{
  if (img == 0) return -1;
  uint i;
  int num_used = 0;
  unsigned long size = img->i.width * img->i.height;
  uint max_uint16 = (uint16) -1;
  uint max_uchar = (uchar) -1;  

  if (img->i.depth == 16) 
  {
    uint16 *p = (uint16 *) img->bits[sample];  
    for (i=0; i<=max_uint16; i++) hist[i] = 0;
    for (i=0; i<size; i++) {
      ++hist[*p];
      p++;
    }
    for (i=0; i<=max_uint16; i++) if (hist[i] != 0) ++num_used;
  }
  else // 8bit
  {
    uchar *p = (uchar *) img->bits[sample];  
    for (i=0; i<=max_uchar; i++) hist[i] = 0;
    for (i=0; i<size; i++) {
      ++hist[*p];
      p++;
    }
    for (i=0; i<=max_uchar; i++) if (hist[i] != 0) ++num_used;
  }

  return 0;
}


std::string bim::getImageInfoText(ImageInfo *info) {
    bim::xstring inftext="";
    if (info == NULL) return inftext;

    inftext += xstring::xprintf("pages: %d\n",     info->number_pages ); 
    inftext += xstring::xprintf("channels: %d\n",  info->samples );  
    inftext += xstring::xprintf("width: %d\n",     info->width);
    inftext += xstring::xprintf("height: %d\n",    info->height);
    inftext += xstring::xprintf("zsize: %d\n",     info->number_z );
    inftext += xstring::xprintf("tsize: %d\n",     info->number_t );
    inftext += xstring::xprintf("depth: %d\n",     info->depth ); 
    inftext += xstring::xprintf("pixelType: %d\n", info->pixelType );
    if (bim::bigendian) inftext+="endian: big\n"; else inftext+="endian: little\n";
    inftext += xstring::xprintf("resUnits: %d\n",  info->resUnits );
    inftext += xstring::xprintf("xRes: %f\n",      info->xRes );
    inftext += xstring::xprintf("yRes: %f\n",      info->yRes );

    inftext += xstring::xprintf("%s: %d\n", bim::IMAGE_NUM_RES_L.c_str(), info->number_levels);

    if (info->tileWidth > 0 && info->tileHeight > 0) {
        inftext += xstring::xprintf("%s: %d\n", bim::TILE_NUM_X.c_str(), info->tileWidth);
        inftext += xstring::xprintf("%s: %d\n", bim::TILE_NUM_Y.c_str(), info->tileHeight);
    }

    inftext += "dimensions:";
    try {
        if (info->number_dims >= BIM_MAX_DIMS) info->number_dims = BIM_MAX_DIMS-1;
        for (unsigned int i=0; i<info->number_dims; ++i) {
            inftext += xstring::xprintf(" %s", dimNames[info->dimensions[i].dim] );
        } 
    } catch (...) {
        inftext += "unknown";     
    }
    inftext += "\n";  


    inftext += "channelsDescription:";  
    try {
        for (unsigned int i=0; i<info->samples; ++i)
            if (info->channels[i].description != NULL) {
                inftext += " ";
                inftext += info->channels[i].description;
            } 
    } catch (...) {
        inftext += "unknown";     
    }
    inftext += "\n";  

    return inftext;
}


