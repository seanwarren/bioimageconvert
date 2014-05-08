/*****************************************************************************
  PNG support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    07/29/2004 16:31 - First creation
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
    03/28/2013 11:51 - Update to libpng 1.5.14
        
  Ver : 3
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bim_png_format.h"

#include <png.h>

using namespace bim;

//----------------------------------------------------------------------------
// MetaData tags
//----------------------------------------------------------------------------

#define BIM_PNG_TAG_TITLE        0
#define BIM_PNG_TAG_AUTHOR       1
#define BIM_PNG_TAG_DESCRIPTION  2
#define BIM_PNG_TAG_COPYRIGHT    3
#define BIM_PNG_TAG_TIME         4
#define BIM_PNG_TAG_SOFTWARE     5
#define BIM_PNG_TAG_DISCLAIMER   6
#define BIM_PNG_TAG_WARNING      7
#define BIM_PNG_TAG_SOURCE       8
#define BIM_PNG_TAG_COMMENT      9

const unsigned char png_magic[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

//****************************************************************************
// Misc
//****************************************************************************

bim::PngParams::PngParams() {
    i = initImageInfo(); 
    png_ptr = 0;
    info_ptr = 0;
    end_info = 0;
}

bim::PngParams::~PngParams() {
  
}


//****************************************************************************
// CALLBACKS
//****************************************************************************

static void dpng_read_fn( png_structp png_ptr, png_bytep data, png_size_t length )
{
  FormatHandle *fmtHndl = (FormatHandle *) png_get_io_ptr( png_ptr );

  BIM_SIZE_T nr = xread( fmtHndl, data, 1, length );
	if (nr <= length) {
    png_error( png_ptr, "Read Error" );
	  return;
	}
}

static void dpng_write_fn( png_structp png_ptr, png_bytep data, png_size_t length )
{
  FormatHandle *fmtHndl = (FormatHandle *) png_get_io_ptr( png_ptr );

  BIM_SIZE_T nr = xwrite( fmtHndl, data, 1, length );
	if (nr < length) {
    png_error( png_ptr, "Write Error" );
	  return;
	}
}

static void dpng_flush_fn( png_structp png_ptr )
{
  FormatHandle *fmtHndl = (FormatHandle *) png_get_io_ptr( png_ptr );

  xflush( fmtHndl );
}

//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

bool pngGetImageInfo( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  

  *info = initImageInfo();
  info->number_pages = 1;
  info->samples = 1;


  par->png_ptr = png_create_read_struct ( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
  if (!par->png_ptr) return false;
  
  par->info_ptr = png_create_info_struct( par->png_ptr );
  if (!par->info_ptr) {
    png_destroy_read_struct( &par->png_ptr, (png_infopp)NULL, (png_infopp)NULL );
    return false;
  }
  
  par->end_info = png_create_info_struct( par->png_ptr );
  if (!par->end_info) {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, (png_infopp)NULL );
    return false;
  }

  if (setjmp( png_jmpbuf(par->png_ptr) )) {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
    return false;
  }

  if ( isCustomReading ( fmtHndl ) != true )
    png_init_io( par->png_ptr, (FILE *) fmtHndl->stream );
  else
  {
    png_set_read_fn( par->png_ptr, (void*) fmtHndl, dpng_read_fn );
    png_read_info( par->png_ptr, par->info_ptr );
  } 

  // no gamma info


  //-----------------------------------------------------------------------
  // read image header
  //-----------------------------------------------------------------------
  png_uint_32 width;
  png_uint_32 height;
  int bit_depth;
  int color_type;

  png_set_benign_errors(par->png_ptr, true); // disable breaking on iCCP warnings
  png_read_info( par->png_ptr, par->info_ptr );
  png_get_IHDR( par->png_ptr, par->info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0 );

  info->width  = width;
  info->height = height;
  info->depth = bit_depth;
  info->samples = 1;
  info->imageMode = IM_GRAYSCALE;

  if ( color_type == PNG_COLOR_TYPE_GRAY ) 
  {
    info->samples = 1;
    info->imageMode = IM_GRAYSCALE;
  }

  if ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA ) 
  {
    info->samples = 2;
    info->imageMode = IM_GRAYSCALE;
  }

  if ( color_type == PNG_COLOR_TYPE_PALETTE ) 
  {
    info->samples = 1;
    info->imageMode = IM_INDEXED;
  }

  if ( color_type == PNG_COLOR_TYPE_RGB ) 
  {
    info->samples = 3;
    info->imageMode = IM_RGB;
  }
 
  if ( color_type == PNG_COLOR_TYPE_RGB_ALPHA ) 
  {
    info->samples = 4;
    info->imageMode = IM_RGBA;
  }
 
  //-------------------------------------------------
  // init palette
  //-------------------------------------------------
  if ( ( color_type == PNG_COLOR_TYPE_GRAY ) ||
       ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA ) ||
       ( color_type == PNG_COLOR_TYPE_PALETTE ) ) 
  {
    info->lut.count = 256;
    for (bim::uint i=0; i<256; i++) info->lut.rgba[i] = xRGB( i, i, i );
  }

  
  //-------------------------------------------------
  // read palette
  //-------------------------------------------------
  if ( color_type == PNG_COLOR_TYPE_PALETTE ) {
      int num_colors = 0;
      png_colorp palette;
      png_get_PLTE(par->png_ptr, par->info_ptr, &palette, &num_colors);

      png_bytep trans_alpha;
      int num_trans=0;
      png_color_16p trans_color;
      png_get_tRNS(par->png_ptr, par->info_ptr, &trans_alpha, &num_trans, &trans_color);

      if (num_colors>0) { // LUT is present
          info->lut.count = num_colors;
      
          if (num_trans>0) { // RGBA palette
              for (int i=0; i<num_colors; i++ ) 
                  info->lut.rgba[i] = xRGBA( palette[i].red, 
                                                palette[i].green, 
                                                palette[i].blue,
                                                trans_alpha[i] );
          } else { // xRGB palette
              for (int i=0; i<num_colors; i++ ) 
                  info->lut.rgba[i] = xRGB( palette[i].red, 
                                              palette[i].green, 
                                              palette[i].blue );
          }
      } // if num_col > 0
  } // if paletted

  return true;
}

//****************************************************************************
// READ PROC
//****************************************************************************

static int read_png_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  
  ImageBitmap *img = fmtHndl->image;
  
  //-------------------------------------------------
  // init the image
  //-------------------------------------------------
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  //-------------------------------------------------
  // read the image
  //-------------------------------------------------

  if (setjmp( png_jmpbuf(par->png_ptr) ))
  {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
    return 1;
  }

  int num_passes = png_set_interlace_handling( par->png_ptr );
  int pass;

  unsigned int bpl = getLineSizeInBytes( img );  
  unsigned long h = info->height;
  unsigned long y = 0;
 
  if (img->i.samples == 1)
  {
    for ( pass=0; pass<num_passes; pass++ )
    {
      while ( y < h ) 
      {
        png_bytep p = ((unsigned char *) img->bits[0]) + ( y * bpl );
        png_read_row( par->png_ptr, p, NULL );
        y++;
      }
      y = 0;
    } // interlace passes
  }
  else
  { // multi samples (channels)
    unsigned char *buf = new unsigned char [ bpl * img->i.samples ];
    
    if (num_passes == 0)
    { // faster code in the case of non interlaced image

      while ( y < h ) 
      {
        png_bytep pbuf = buf;
        png_read_row( par->png_ptr, pbuf, NULL ); 

        if ( img->i.samples == 3 )
        {
          unsigned long x = 0;
          uchar *p0 = ((unsigned char *) img->bits[0]) + ( y * bpl );
          uchar *p1 = ((unsigned char *) img->bits[1]) + ( y * bpl );
          uchar *p2 = ((unsigned char *) img->bits[2]) + ( y * bpl );
          for ( x=0; x<bpl*3; x+=3 )
          {
            *p0 = buf[x+0]; p0++; // R
            *p1 = buf[x+1]; p1++; // G
            *p2 = buf[x+2]; p2++; // B
          }
        }

        if ( img->i.samples == 4 )
        {
          unsigned long x = 0;
          uchar *p0 = ((unsigned char *) img->bits[0]) + ( y * bpl );
          uchar *p1 = ((unsigned char *) img->bits[1]) + ( y * bpl );
          uchar *p2 = ((unsigned char *) img->bits[2]) + ( y * bpl );
          uchar *p3 = ((unsigned char *) img->bits[3]) + ( y * bpl );
          for ( x=0; x<bpl*4; x+=4 )
          {
            *p0 = buf[x+0]; p0++; // R          
            *p1 = buf[x+1]; p1++; // G
            *p2 = buf[x+2]; p2++; // B
            *p3 = buf[x+3]; p3++; // A
          }
        }

        y++;
      } // while
    }
    else
    {  // slower code which handles interlaced images
      for ( unsigned int sample=0; sample<img->i.samples; ++sample )
        memset( img->bits[sample], 0, bpl * h );

      for ( pass=0; pass<num_passes; pass++ ) {    
        while ( y < h ) {
          png_bytep pbuf = buf;
          memset( buf, 0, bpl * img->i.samples );
          png_read_row( par->png_ptr, pbuf, NULL ); 

          if ( img->i.samples == 3 ) {
            unsigned long x = 0;
            uchar *p0 = ((unsigned char *) img->bits[0]) + ( y * bpl );
            uchar *p1 = ((unsigned char *) img->bits[1]) + ( y * bpl );
            uchar *p2 = ((unsigned char *) img->bits[2]) + ( y * bpl );
            for ( x=0; x<bpl*3; x+=3 ) {
              if (buf[x+0] != 0) *p0 = buf[x+0]; p0++; // R
              if (buf[x+1] != 0) *p1 = buf[x+1]; p1++; // G
              if (buf[x+2] != 0) *p2 = buf[x+2]; p2++; // B
            }
          }

          if ( img->i.samples == 4 ) {
            unsigned long x = 0;
            uchar *p0 = ((unsigned char *) img->bits[0]) + ( y * bpl );
            uchar *p1 = ((unsigned char *) img->bits[1]) + ( y * bpl );
            uchar *p2 = ((unsigned char *) img->bits[2]) + ( y * bpl );
            uchar *p3 = ((unsigned char *) img->bits[3]) + ( y * bpl );
            for ( x=0; x<bpl*4; x+=4 ) {
              *p0 = buf[x+0]; p0++; // R          
              *p1 = buf[x+1]; p1++; // G
              *p2 = buf[x+2]; p2++; // B
              *p3 = buf[x+3]; p3++; // A
            }
          }

          y++;
        } // while
        y = 0;
      } // interlace passes
    } // interlaced code
    delete [] buf;
  }

  png_read_end( par->png_ptr, par->end_info );

  return 0;
}


//****************************************************************************
// WRITE PROC
//****************************************************************************
template <typename T>
void write_png_buff ( ImageBitmap *img, T *buf, int y ) {
  
  unsigned int bpl = getLineSizeInBytes( img );  
  unsigned long x = 0;
  unsigned int w = (unsigned int) img->i.width;

  if ( img->i.samples == 2 ) {
    T *p0 = (T*) ( ((unsigned char *) img->bits[0]) + ( y * bpl ) );
    T *p1 = (T*) ( ((unsigned char *) img->bits[1]) + ( y * bpl ) );
    for ( x=0; x<w*3; x+=3 ) {
      buf[x+0] = *p0; p0++; // 1          
      buf[x+1] = *p1; p1++; // 2
      buf[x+2] = 0; // B
    }
  } // if 2 channels

  if ( img->i.samples == 3 ) {
    T *p0 = (T*) ( ((unsigned char *) img->bits[0]) + ( y * bpl ) );
    T *p1 = (T*) ( ((unsigned char *) img->bits[1]) + ( y * bpl ) );
    T *p2 = (T*) ( ((unsigned char *) img->bits[2]) + ( y * bpl ) );
    for ( x=0; x<w*3; x+=3 ) {
      buf[x+0] = *p0; p0++; // R          
      buf[x+1] = *p1; p1++; // G
      buf[x+2] = *p2; p2++; // B
    }
  } // if 3 channels

  if ( img->i.samples == 4 ) {
    T *p0 = (T*) ( ((unsigned char *) img->bits[0]) + ( y * bpl ) );
    T *p1 = (T*) ( ((unsigned char *) img->bits[1]) + ( y * bpl ) );
    T *p2 = (T*) ( ((unsigned char *) img->bits[2]) + ( y * bpl ) );
    T *p3 = (T*) ( ((unsigned char *) img->bits[3]) + ( y * bpl ) );
    for ( x=0; x<w*4; x+=4 ) {
      buf[x+0] = *p0; p0++; // R          
      buf[x+1] = *p1; p1++; // G
      buf[x+2] = *p2; p2++; // B
      buf[x+3] = *p3; p3++; // A
    }
  } // if 4 chanels
}

static int write_png_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;
  ImageBitmap *img = fmtHndl->image;
  ImageInfo *info = &img->i; 

  if (setjmp( png_jmpbuf(par->png_ptr) ))
  {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
    return 1;
  }
  
  int color_type = PNG_COLOR_TYPE_GRAY;;
  if ( info->samples == 1 ) color_type = PNG_COLOR_TYPE_GRAY;
  //if ( info->samples == 2 ) color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
  if ( info->samples == 2 ) color_type = PNG_COLOR_TYPE_RGB;
  if ( info->samples == 3 ) color_type = PNG_COLOR_TYPE_RGB;
  if ( info->samples == 4 ) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
  if ( info->imageMode == IM_INDEXED ) color_type = PNG_COLOR_TYPE_PALETTE;

  png_set_IHDR( par->png_ptr, par->info_ptr, info->width, info->height, info->depth, color_type, 
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);


  //-------------------------------------------------
  // write palette if any
  //-------------------------------------------------
  png_colorp palette = 0;
  if ( ( color_type == PNG_COLOR_TYPE_PALETTE ) && ( info->lut.count > 0 ) )  {
    int num_colors = info->lut.count;
    palette = new png_color[ num_colors ];

    for (int i=0; i<num_colors; i++ ) {
      palette[i].red   = (unsigned char) xR( info->lut.rgba[i] );
      palette[i].green = (unsigned char) xG( info->lut.rgba[i] );
      palette[i].blue  = (unsigned char) xB( info->lut.rgba[i] );
    }  

    png_set_PLTE( par->png_ptr, par->info_ptr, palette, num_colors );
  } // if paletted


  //-------------------------------------------------
  // write meta text if any
  //-------------------------------------------------
  if ( ( fmtHndl->metaData.count > 0 ) && ( fmtHndl->metaData.tags != NULL ) )
  {
    TagList *tagList = &fmtHndl->metaData;
    png_textp text_ptr = new png_text[ tagList->count ];    
    std::string str = "", keystr = ""; 
    unsigned int i;

    for (i=0; i<tagList->count; i++)
    {
      TagItem *tagItem = &tagList->tags[i];
      str = "";
      keystr = tagItem->tagId;

      if ( tagItem->tagType == TAG_ASCII ) 
        str = (char *) tagItem->tagData;
      
      if ( str.size() < 40 )
        text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
      else
        text_ptr[i].compression = PNG_TEXT_COMPRESSION_zTXt;

      char *keychar = new char [ keystr.size() ];
      char *strchar = new char [ str.size() ];
      strncpy ( keychar, keystr.c_str(), keystr.size());
      strncpy ( strchar, str.c_str(), str.size());
      text_ptr[i].key  = (png_charp) keychar;
      text_ptr[i].text = (png_charp) strchar;
    } // for i

    png_set_text( par->png_ptr, par->info_ptr, text_ptr, tagList->count );
    delete [] text_ptr;
  } // if there are meta tags

  //-------------------------------------------------
  // write image
  //-------------------------------------------------
  png_write_info( par->png_ptr, par->info_ptr );
  if ( (info->depth > 8) && (!bim::bigendian) )  png_set_swap( par->png_ptr );

  unsigned int bpl = getLineSizeInBytes( img );  
  unsigned long h = info->height;
  unsigned long y = 0;

  if (img->i.samples == 1) {
    while ( y < h ) {
      png_bytep p = ((unsigned char *) img->bits[0]) + ( y * bpl );
      png_write_row( par->png_ptr, p );
      y++;
    }
    y = 0;
  }
  else { // multi samples (channels)
    int ch = img->i.samples;
    if (ch == 2) ch = 3;
    unsigned char *buf = new unsigned char [ bpl * ch ];

    while ( y < h ) {
      png_bytep pbuf = buf;

      if (info->depth == 8)
        write_png_buff<uchar> ( img, (uchar *) buf, y );
      
      if (info->depth == 16)
        write_png_buff<uint16> ( img, (uint16 *) buf, y );

      png_write_row( par->png_ptr, pbuf );

      y++;
    } // while
 
    delete [] buf;
  }

  png_write_end( par->png_ptr, par->info_ptr );
  if (palette) delete palette;
  return 0;
}

//****************************************************************************
// READ METADATA TEXT PROC
//****************************************************************************

static char *png_read_meta_as_text(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return NULL;
  if (fmtHndl->internalParams == NULL) return NULL;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;

  char *buf = NULL;
  std::string str = ""; 
  
  if (setjmp( png_jmpbuf(par->png_ptr) ))
  {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
    return NULL;
  }

  png_textp text_ptr;
  int num_text = 0;
  png_get_text( par->png_ptr, par->info_ptr, &text_ptr, &num_text );

  while (num_text--) 
  {
    str += "[ Key: ";   
    str += text_ptr->key;
    str += " ]\n";
    str += text_ptr->text;
    str += "\n\n";
    text_ptr++;
  }

  buf = new char [str.size()+1];
  buf[str.size()] = '\0';
  memcpy( buf, str.c_str(), str.size() );
  
  return buf;
}

//----------------------------------------------------------------------------
// read meta data tag by tag
//----------------------------------------------------------------------------

static bim::uint add_one_png_tag (FormatHandle *fmtHndl, int tag, const char* str) {
    uchar *buf = NULL;
    uint32 buf_size = (uint32) strlen(str);
    uint32 buf_type = TAG_ASCII;

    if ( (buf_size == 0) || (str == NULL) ) return 1;

    // now add tag into structure
    TagItem item;

    buf = (unsigned char *) xmalloc(fmtHndl, buf_size + 1);
    strncpy((char *) buf, str, buf_size);
    buf[buf_size] = '\0';

    item.tagGroup  = META_PNG;
    item.tagId     = tag;
    item.tagType   = buf_type;
    item.tagLength = buf_size;
    item.tagData   = buf;

    addMetaTag( &fmtHndl->metaData, item);
    return 0;
}

static bim::uint read_png_metadata (FormatHandle *fmtHndl, int group, int tag, int type)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;
  
  if ( (group != META_PNG) && (group != -1) ) return 1;

  if (setjmp( png_jmpbuf(par->png_ptr) ))
  {
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
    return 1;
  }

  png_textp text_ptr;
  int num_text = 0;
  png_get_text( par->png_ptr, par->info_ptr, &text_ptr, &num_text );

  int i = 0;
  while ( i < num_text) {
    std::string str = "";
    str += "[Key: ";   
    str += text_ptr->key;
    str += "]\n";
    str += text_ptr->text;
    
    add_one_png_tag ( fmtHndl, i, str.c_str() );

    ++text_ptr;
    ++i;
  }

  tag; type;
  return 0;
}



//****************************************************************************
// FORMAT DEMANDED FUNTIONS
//****************************************************************************


//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int pngValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < 8) return -1;
  if (memcmp(magic, png_magic, 8) == 0) return 0;
  return -1;
}

FormatHandle pngAquireFormatProc( void ) {
  return initFormatHandle();
}

void pngCloseImageProc (FormatHandle *fmtHndl);
void pngReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  pngCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void pngCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;

  if (fmtHndl->internalParams == NULL) return;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;

  if ( fmtHndl->io_mode == IO_READ )
    png_destroy_read_struct( &par->png_ptr, &par->info_ptr, &par->end_info );
  else
    png_destroy_write_struct( &par->png_ptr, &par->info_ptr );

  delete par;
  fmtHndl->internalParams=0;
  xclose ( fmtHndl );
}

bim::uint pngOpenImageProc  ( FormatHandle *fmtHndl, ImageIOModes io_mode ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) pngCloseImageProc (fmtHndl);  
  fmtHndl->internalParams = (void *) new bim::PngParams();
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;

  fmtHndl->io_mode = io_mode;
  xopen(fmtHndl);
  if (!fmtHndl->stream) { 
      pngCloseImageProc(fmtHndl); 
      return 1; 
  };

  if ( io_mode == IO_READ ) {
    if ( !pngGetImageInfo( fmtHndl ) ) { pngCloseImageProc (fmtHndl); return 1; };
  } else if ( io_mode == IO_WRITE ) {
    par->png_ptr = png_create_write_struct ( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if (!par->png_ptr) return 1;

    par->info_ptr = png_create_info_struct( par->png_ptr );
    if (!par->info_ptr) {
       png_destroy_write_struct( &par->png_ptr, (png_infopp) NULL );
       return 1;
    }

    if ( isCustomWriting ( fmtHndl ) != true )
      png_init_io( par->png_ptr, (FILE *) fmtHndl->stream );
    else
    {
      png_set_write_fn( par->png_ptr, (void*) fmtHndl, dpng_write_fn, dpng_flush_fn);
      //png_read_info( par->png_ptr, par->info_ptr );
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint pngGetNumPagesProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;

  return 1;
}


ImageInfo pngGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint /*page_num*/ ) {
  ImageInfo ii = initImageInfo();
  if (fmtHndl == NULL) return ii;
  bim::PngParams *par = (bim::PngParams *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint pngAddMetaDataProc (FormatHandle * /*fmtHndl*/) {
  return 1;
}


bim::uint pngReadMetaDataProc (FormatHandle *fmtHndl, bim::uint /*page*/, int group, int tag, int type) {
  return read_png_metadata ( fmtHndl, group, tag, type );
}

char* pngReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  return png_read_meta_as_text( fmtHndl );
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint pngReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;

  fmtHndl->pageNumber = page;
  return read_png_image( fmtHndl );
}

bim::uint pngWriteImageProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;
  return write_png_image( fmtHndl );
}

//****************************************************************************
// EXPORTED FUNCTION
//****************************************************************************

FormatItem pngItems[1] = {
{
    "PNG",            // short name, no spaces
    "Portable Network Graphics", // Long format name
    "png",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 4, 1, 16, 0 } 
  }
};

FormatHeader pngHeader = {

  sizeof(FormatHeader),
  "1.0.0",
  "DIMIN PNG CODEC",
  "PNG CODEC",
  
  8,                     // 0 or more, specify number of bytes needed to identify the file
  { 1, 1, pngItems }, // ( ver, sub formats ) only one sub format
  
  pngValidateFormatProc,
  // begin
  pngAquireFormatProc, //AquireFormatProc
  // end
  pngReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  pngOpenImageProc, //OpenImageProc
  pngCloseImageProc, //CloseImageProc 

  // info
  pngGetNumPagesProc, //GetNumPagesProc
  pngGetImageInfoProc, //GetImageInfoProc


  // read/write
  pngReadImageProc, //ReadImageProc 
  pngWriteImageProc, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  pngReadMetaDataProc, //ReadMetaDataProc
  pngAddMetaDataProc,  //AddMetaDataProc
  pngReadMetaDataAsTextProc, //ReadMetaDataAsTextProc

  NULL,
  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* pngGetFormatHeader(void)
{
  return &pngHeader;
}

} // extern C


