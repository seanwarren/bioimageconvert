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
      
  ver: 2
        
*******************************************************************************/

#ifndef BIM_IMG_FMT_UTL_H
#define BIM_IMG_FMT_UTL_H

#include "bim_img_format_interface.h"

#include <string>

//------------------------------------------------------------------------------
// Safe calls for memory/io prototypes, if they are not supplied then
// standard functions are used
//------------------------------------------------------------------------------

namespace bim {

void* xmalloc ( FormatHandle *fmtHndl, BIM_SIZE_T size );
inline void* xmalloc ( BIM_SIZE_T size ) {
    return (void *) new char[size];
}

void* xfree   ( FormatHandle *fmtHndl, void *p );
// overload of dimin free to make a safe delete
inline void xfree( void **p ) {
  if ( *p == NULL ) return;
  delete (unsigned char *) *p;  
  *p = NULL;
}

void xprogress  ( FormatHandle *fmtHndl, uint64 done, uint64 total, char *descr);
void xerror     ( FormatHandle *fmtHndl, int val, char *descr);
int  xtestAbort ( FormatHandle *fmtHndl );

// the stream is specified by FormatHandle
void         xopen  (FormatHandle *fmtHndl);
int          xclose (FormatHandle *fmtHndl);

BIM_SIZE_T   xread  ( FormatHandle *fmtHndl, void *buffer, BIM_SIZE_T size, BIM_SIZE_T count );
BIM_SIZE_T   xwrite ( FormatHandle *fmtHndl, void *buffer, BIM_SIZE_T size, BIM_SIZE_T count );
int          xflush ( FormatHandle *fmtHndl );
int          xseek  ( FormatHandle *fmtHndl, BIM_OFFSET_T offset, int origin );
BIM_SIZE_T   xsize  ( FormatHandle *fmtHndl );
BIM_OFFSET_T xtell  ( FormatHandle *fmtHndl );
int          xeof   ( FormatHandle *fmtHndl );
   
//------------------------------------------------------------------------------
// tests for provided callbacks
//------------------------------------------------------------------------------
bool isCustomReading ( FormatHandle *fmtHndl );
bool isCustomWriting ( FormatHandle *fmtHndl );

//------------------------------------------------------------------------------------------------
// misc
//------------------------------------------------------------------------------------------------

FormatHandle initFormatHandle();
ImageInfo initImageInfo();

//------------------------------------------------------------------------------------------------
// swap
//------------------------------------------------------------------------------------------------
void swapData(int type, uint64 size, void* data);

//------------------------------------------------------------------------------------------------
// ImageBitmap
//------------------------------------------------------------------------------------------------

// you must call this function once declared image var
void initImagePlanes(ImageBitmap *bmp);

int allocImg( ImageBitmap *img, uint w, uint h, uint samples, uint depth);
// alloc image using w,h,s,d
int allocImg( FormatHandle *fmtHndl, ImageBitmap *img, uint w, uint h, uint samples, uint depth);
// alloc image using info
int allocImg( FormatHandle *fmtHndl, ImageInfo *info, ImageBitmap *img);
// alloc handle image using info
int allocImg( FormatHandle *fmtHndl, ImageInfo *info );

void deleteImg( ImageBitmap *img);
void deleteImg( FormatHandle *fmtHndl, ImageBitmap *img);

uint64 getLineSizeInBytes(ImageBitmap *img);
uint64 getImgSizeInBytes(ImageBitmap *img);
uint getImgNumColors(ImageBitmap *img);

int getSampleHistogram(ImageBitmap *img, long *hist, int sample);

std::string getImageInfoText( ImageInfo *info );

//------------------------------------------------------------------------------------------------
// metadata
//------------------------------------------------------------------------------------------------

void clearMetaTag(TagItem *tagItem);
void clearMetaTags(TagList *tagList);
//bool isTagPresent(TagList *tagList, int tag);
bool isTagPresent(TagList *tagList, int group, int tag);
//int tagPos(TagList *tagList, int tag);
int tagPos(TagList *tagList, int group, int tag);

int addMetaTag(TagList *tagList, TagItem item);

} // namespace bim

#endif //BIM_IMG_FMT_UTL_H

