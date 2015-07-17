/*****************************************************************************
  BIORAD PIC IO
  UCSB/BioITR property
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    

  History:
    03/29/2004 22:23 - First creation
    05/10/2004 14:55 - Big endian support
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
        
  Ver : 3
*****************************************************************************/


//#include <cstring>
#include <string>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

#include "bim_biorad_pic_format.h"

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

static int read_biorad_image(FormatHandle *fmtHndl) {
  if (!fmtHndl || !fmtHndl->internalParams) return 1;
  BioRadPicParams *picPar = (BioRadPicParams *) fmtHndl->internalParams;
  ImageInfo *info = &picPar->i;  
  ImageBitmap *img = fmtHndl->image;

  // init the image
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;
  long offset = picPar->data_offset + fmtHndl->pageNumber * picPar->page_size_bytes;

  if (fmtHndl->stream == NULL) return 1;
  if ( xseek(fmtHndl, offset, SEEK_SET) != 0) return 1;
  if (xread( fmtHndl, img->bits[0], picPar->page_size_bytes, 1 ) != 1) return 1;
  if ( (img->i.depth == 16) && (bim::bigendian) ) 
    swapArrayOfShort((uint16*) img->bits[0], picPar->page_size_bytes/2);
  
  xprogress( fmtHndl, 0, 10, "Reading BIORAD" );

  if (info->imageMode == IM_MULTI) { // three pages are considered xRGB channels

    offset += picPar->page_size_bytes;
    if (xseek( fmtHndl, offset, SEEK_SET) != 0) return 1;
    if (xread( fmtHndl, img->bits[1], picPar->page_size_bytes, 1 ) != 1) return 1;

    if ( (img->i.depth == 16) && (bim::bigendian) ) 
      swapArrayOfShort((uint16*) img->bits[1], picPar->page_size_bytes/2);

    if (picPar->num_images > 2) {
      offset += picPar->page_size_bytes;
      if (xseek( fmtHndl, offset, SEEK_SET) != 0) return 1;
      if (xread( fmtHndl, img->bits[2], picPar->page_size_bytes, 1 ) != 1) return 1;

      if ( (img->i.depth == 16) && (bim::bigendian) ) 
        swapArrayOfShort((uint16*) img->bits[2], picPar->page_size_bytes/2);
    } else {
      memset( img->bits[2], 0, picPar->page_size_bytes );
    }
  }

  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint add_one_tag (FormatHandle *fmtHndl, int tag, const char* str) {
  uchar *buf = NULL;
  uint32 buf_size = strlen(str);
  uint32 buf_type = TAG_ASCII;

  if ( (buf_size == 0) || (str == NULL) ) return 1;
  else { // now add tag into structure
    TagItem item;

    buf = (unsigned char *) xmalloc(fmtHndl, buf_size + 1);
    strncpy((char *) buf, str, buf_size);
    buf[buf_size] = '\0';

    item.tagGroup  = META_BIORAD;
    item.tagId     = tag;
    item.tagType   = buf_type;
    item.tagLength = buf_size;
    item.tagData   = buf;

    addMetaTag( &fmtHndl->metaData, item);
  }

  return 0;
}

bim::uint read_biorad_metadata (FormatHandle *fmtHndl, int group, int , int )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  BioRadPicParams *par = (BioRadPicParams *) fmtHndl->internalParams;
  
  if ( (group != META_BIORAD) && (group != -1) ) return 1;

  if (par->note01.size() > 0) 
    add_one_tag ( fmtHndl, 01, par->note01.c_str() );
  if (par->note20.size() > 0) 
    add_one_tag ( fmtHndl, 20, par->note20.c_str() );
  if (par->note21.size() > 0) 
    add_one_tag ( fmtHndl, 21, par->note21.c_str() );

  return 0;
}


//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint biorad_append_metadata (FormatHandle *fmtHndl, bim::TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  BioRadPicParams *par = (BioRadPicParams *) fmtHndl->internalParams; 

  hash->append_tag( bim::PIXEL_RESOLUTION_X, par->pixel_size[0] );
  hash->append_tag( bim::PIXEL_RESOLUTION_Y, par->pixel_size[1] );
  hash->append_tag( bim::PIXEL_RESOLUTION_Z, par->pixel_size[2] );

  hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS );

  //date
  if (par->datetime.size()>0)
    hash->append_tag( bim::IMAGE_DATE_TIME, par->datetime );

  if (par->note01.size() > 0)
    hash->append_tag( bim::RAW_TAGS_PREFIX+"Note01", par->note01 );

  if (par->note20.size() > 0)
    hash->append_tag( bim::RAW_TAGS_PREFIX+"Note20", par->note20 );

  if (par->note21.size() > 0)
    hash->append_tag( bim::RAW_TAGS_PREFIX+"Note21", par->note21 );

  // Parse some of the notes
  hash->parse_ini( par->note20, "=", bim::CUSTOM_TAGS_PREFIX );

  return 0;
}
