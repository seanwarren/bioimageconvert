/*****************************************************************************
  JPEG IO 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    

  History:
    03/29/2004 22:23 - First creation
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
    2010-06-24 15:11 - EXIF/IPTC extraction
        
  Ver : 3
*****************************************************************************/

#include "bim_jpeg_format.h"

#include <bim_metatags.h>
#include <bim_exiv_parse.h>

struct my_error_mgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};


extern "C" {
static void my_error_exit (j_common_ptr cinfo)
{
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    longjmp(myerr->setjmp_buffer, 1);
}
}

static const int max_buf = 4096;

//****************************************************************************
// READ STUFF
//****************************************************************************

struct my_jpeg_source_mgr : public jpeg_source_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    FormatHandle *fmtHndl;
    JOCTET buffer[max_buf];

public:
    my_jpeg_source_mgr(FormatHandle *new_hndl);
};

extern "C" {

static void dimjpeg_init_source(j_decompress_ptr)
{
}

static boolean dimjpeg_fill_input_buffer(j_decompress_ptr cinfo) {
  int num_read;
  my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;
  src->next_input_byte = src->buffer;
  
  //num_read = fread( src->buffer, 1, max_buf, src->stream);
  num_read = xread( src->fmtHndl, src->buffer, 1, max_buf );

  if ( num_read <= 0 ) {
    // Insert a fake EOI marker - as per jpeglib recommendation
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    src->bytes_in_buffer = 2;
  } 
  else 
    src->bytes_in_buffer = num_read;

  return (boolean)true;
}

static void dimjpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;

  // `dumb' implementation from jpeglib

  // Just a dumb implementation for now.  Could use fseek() except
  // it doesn't work on pipes.  Not clear that being smart is worth
  // any trouble anyway --- large skips are infrequent.
  if (num_bytes > 0) 
  {
    while (num_bytes > (long) src->bytes_in_buffer) 
    {
      num_bytes -= (long) src->bytes_in_buffer;
      (void) dimjpeg_fill_input_buffer(cinfo);
      // note we assume that qt_fill_input_buffer will never return false,
      // so suspension need not be handled.
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void dimjpeg_term_source(j_decompress_ptr)
{
}

} // extern C


inline my_jpeg_source_mgr::my_jpeg_source_mgr(FormatHandle *new_hndl)
{
  jpeg_source_mgr::init_source       = dimjpeg_init_source;
  jpeg_source_mgr::fill_input_buffer = dimjpeg_fill_input_buffer;
  jpeg_source_mgr::skip_input_data   = dimjpeg_skip_input_data;
  jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
  jpeg_source_mgr::term_source       = dimjpeg_term_source;
  fmtHndl = new_hndl;
  bytes_in_buffer = 0;
  next_input_byte = buffer;
}

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

bool jpegGetImageInfo( FormatHandle *fmtHndl ) {
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;

    par->iod_src = new my_jpeg_source_mgr( fmtHndl );
    par->cinfo = new jpeg_decompress_struct;
    jpeg_create_decompress(par->cinfo);
    par->cinfo->src = par->iod_src;
    par->cinfo->err = jpeg_std_error(par->jerr);
    par->jerr->error_exit = my_error_exit;

    if (!setjmp(par->jerr->setjmp_buffer)) {
        jpeg_read_header(par->cinfo, (boolean)true);
    }

    // set some image parameters
    par->i.number_pages = 1;
    par->i.depth = 8;
    par->i.width   = par->cinfo->image_width;
    par->i.height  = par->cinfo->image_height;
    par->i.samples = par->cinfo->num_components;
    return true;
}

static int read_jpeg_image(FormatHandle *fmtHndl) {
  bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;
  JSAMPROW row_pointer[1];
  ImageBitmap *image = fmtHndl->image;
  jpeg_decompress_struct *cinfo = par->cinfo;

  if (!setjmp(par->jerr->setjmp_buffer)) {
    jpeg_start_decompress(cinfo);

    if (allocImg( image, cinfo->output_width, cinfo->output_height, cinfo->output_components, 8 ) != 0) {
      jpeg_destroy_decompress(cinfo);      
      return 1;
    }

    if (cinfo->output_components == 1) {
      while (cinfo->output_scanline < cinfo->output_height) {
        xprogress( fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG" );
        if ( xtestAbort( fmtHndl ) == 1) break;  

        row_pointer[0] =  ((uchar *) image->bits[0]) + (cinfo->output_width*cinfo->output_scanline);
        (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
      }
    } // if 1 component
    

    if ( cinfo->output_components == 3 ) {
      row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];

      while (cinfo->output_scanline < cinfo->output_height) {
        xprogress( fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG" );
        if ( xtestAbort( fmtHndl ) == 1) break; 

        register unsigned int i;        
        (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
        uchar *row = row_pointer[0];        
        uchar* pix1 = ((uchar *) image->bits[0]) + cinfo->output_width * (cinfo->output_scanline-1);
        uchar* pix2 = ((uchar *) image->bits[1]) + cinfo->output_width * (cinfo->output_scanline-1);
        uchar* pix3 = ((uchar *) image->bits[2]) + cinfo->output_width * (cinfo->output_scanline-1);

        for (i=0; i<cinfo->output_width; i++) {
          *pix1++ = *row++;
          *pix2++ = *row++;
          *pix3++ = *row++;
        }

      } // while scanlines
      delete row_pointer[0];
    } // if 3 components

    if ( cinfo->output_components == 4 ) {
      row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];

      while (cinfo->output_scanline < cinfo->output_height) {
        xprogress( fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG" );
        if ( xtestAbort( fmtHndl ) == 1) break; 

        register unsigned int i;        
        (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
        uchar *row = row_pointer[0];        
        uchar* pix1 = ((uchar *) image->bits[0]) + cinfo->output_width * (cinfo->output_scanline-1);
        uchar* pix2 = ((uchar *) image->bits[1]) + cinfo->output_width * (cinfo->output_scanline-1);
        uchar* pix3 = ((uchar *) image->bits[2]) + cinfo->output_width * (cinfo->output_scanline-1);
        uchar* pix4 = ((uchar *) image->bits[3]) + cinfo->output_width * (cinfo->output_scanline-1);

        for (i=0; i<cinfo->output_width; i++) {
          *pix1++ = *row++;
          *pix2++ = *row++;
          *pix3++ = *row++;
          *pix4++ = *row++;
        }

      } // while scanlines
      delete row_pointer[0];
    } // if 4 components

    jpeg_finish_decompress(cinfo);
  }

  return 0;
}


//****************************************************************************
// WRITE STUFF
//****************************************************************************

struct my_jpeg_destination_mgr : public jpeg_destination_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    FormatHandle *fmtHndl;
    JOCTET buffer[max_buf];

public:
    my_jpeg_destination_mgr(FormatHandle *new_hndl);
};

extern "C" {

static void dimjpeg_init_destination(j_compress_ptr)
{
}

static void dimjpeg_exit_on_error(j_compress_ptr cinfo, FormatHandle *fmtHndl)
{
  // cinfo->err->msg_code = JERR_FILE_WRITE;
  xflush( fmtHndl );
  (*cinfo->err->error_exit)((j_common_ptr)cinfo);
}

static boolean dimjpeg_empty_output_buffer(j_compress_ptr cinfo)
{
  my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
  
  if ( xwrite( dest->fmtHndl, (char*)dest->buffer, 1, max_buf) != max_buf )
    dimjpeg_exit_on_error(cinfo, dest->fmtHndl);

  xflush( dest->fmtHndl );
  dest->next_output_byte = dest->buffer;
  dest->free_in_buffer = max_buf;

  return (boolean)true;
}

static void dimjpeg_term_destination(j_compress_ptr cinfo)
{
  my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
  unsigned int n = max_buf - dest->free_in_buffer;
  
  if ( xwrite( dest->fmtHndl, (char*)dest->buffer, 1, n ) != n )
    dimjpeg_exit_on_error(cinfo, dest->fmtHndl);

  xflush( dest->fmtHndl );
  dimjpeg_exit_on_error( cinfo, dest->fmtHndl );
}

} // extern C

inline my_jpeg_destination_mgr::my_jpeg_destination_mgr(FormatHandle *new_hndl)
{
  jpeg_destination_mgr::init_destination    = dimjpeg_init_destination;
  jpeg_destination_mgr::empty_output_buffer = dimjpeg_empty_output_buffer;
  jpeg_destination_mgr::term_destination    = dimjpeg_term_destination;
  fmtHndl = new_hndl;
  next_output_byte = buffer;
  free_in_buffer = max_buf;
}

//----------------------------------------------------------------------------
// WRITE PROC
//----------------------------------------------------------------------------

static int write_jpeg_image( FormatHandle *fmtHndl )
{
  ImageBitmap *image = fmtHndl->image;

  struct jpeg_compress_struct cinfo;
  JSAMPROW row_pointer[1];
  row_pointer[0] = 0;

  struct my_jpeg_destination_mgr *iod_dest = new my_jpeg_destination_mgr( fmtHndl );
  struct my_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);

  jerr.error_exit = my_error_exit;

  if (!setjmp(jerr.setjmp_buffer)) 
  {
    jpeg_create_compress(&cinfo);
    cinfo.dest = iod_dest;

    cinfo.image_width  = image->i.width;
    cinfo.image_height = image->i.height;


    LUT* cmap=0;
    bool gray=true;

    if ( image->i.samples == 1 )
    {
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
      gray = true;
    }
    else
    {
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
      gray = false;
    }

    if ( image->i.depth < 8 )
    {
      cmap = &image->i.lut;
      gray = true;
      if (cmap->count > 0) gray = false;
      cinfo.input_components = gray ? 1 : 3;
      cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
    }

    jpeg_set_defaults(&cinfo);
    int quality = fmtHndl->quality;
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;    
    jpeg_set_quality(&cinfo, quality, (boolean)true); // limit to baseline-JPEG values );

    if (fmtHndl->order == 1) jpeg_simple_progression(&cinfo);

    jpeg_start_compress(&cinfo, (boolean)true);

    row_pointer[0] = new uchar[cinfo.image_width*cinfo.input_components];
    int w = cinfo.image_width;
    long lineSizeBytes = getLineSizeInBytes( image );

    while (cinfo.next_scanline < cinfo.image_height) 
    {
      uchar *row = row_pointer[0];

      /*
      switch ( image.depth() ) 
      {
        case 1:
          if (gray) 
          {
            uchar* data = image.scanLine(cinfo.next_scanline);
            if ( image.bitOrder() == QImage::LittleEndian ) 
            {
              for (int i=0; i<w; i++) 
              {
                bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                row[i] = qRed(cmap[bit]);
              }
            } 
            else 
            {
              for (int i=0; i<w; i++) 
              {
                bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                row[i] = qRed(cmap[bit]);
              }
            }
          } 
          else 
          {
            uchar* data = image.scanLine(cinfo.next_scanline);
            if ( image.bitOrder() == QImage::LittleEndian ) 
            {
              for (int i=0; i<w; i++) 
              {
                bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                *row++ = qRed(cmap[bit]);
                *row++ = qGreen(cmap[bit]);
                *row++ = qBlue(cmap[bit]);
              }
            } 
            else 
            {
              for (int i=0; i<w; i++) 
              {
                bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                *row++ = qRed(cmap[bit]);
                *row++ = qGreen(cmap[bit]);
                *row++ = qBlue(cmap[bit]);
              }
            }
          }
          
          break;
        */


      //if 4 bits per sample, there should be only one sample
      if (image->i.depth == 4)
      {
        if (gray)
        {
          uchar* pix = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
          uchar pixH, pixL;
          for (int i=0; i<lineSizeBytes; i++) 
          {
            pixH = (unsigned char) ((*pix) << 4);
            pixL = (unsigned char) ((*pix) >> 4);
            *row++ = pixH;
            if (i+1<w) *row++ = pixL;
            pix++;
          }
        } // if one sample with 8 bits
        else
        {
          uchar pixH, pixL;
          uchar* pix = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;

          for (int i=0; i<lineSizeBytes; i++) 
          {
            pixH = (unsigned char) ((*pix) << 4);
            pixL = (unsigned char) ((*pix) >> 4);
            *row++ = (unsigned char) xR( cmap->rgba[pixH] );
            *row++ = (unsigned char) xG( cmap->rgba[pixH] );
            *row++ = (unsigned char) xB( cmap->rgba[pixH] );
            if (i+1<w) 
            {
              *row++ = (unsigned char) xR( cmap->rgba[pixL] );
              *row++ = (unsigned char) xG( cmap->rgba[pixL] );
              *row++ = (unsigned char) xB( cmap->rgba[pixL] );
            }
            pix++;
          }
        } // if paletted image
      } // 4 bits per sample

      
      //if 8 bits per sample
      if (image->i.depth == 8)
      {
        if (gray)
        {
          uchar* pix = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;

          memcpy( row, pix, w );
          row += w;
        } // if one sample with 8 bits
        else
        {
          if (image->i.samples == 1)
          {
            uchar* pix = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
            for (int i=0; i<w; i++) 
            {
              *row++ = (unsigned char) xR( cmap->rgba[*pix] );
              *row++ = (unsigned char) xG( cmap->rgba[*pix] );
              *row++ = (unsigned char) xB( cmap->rgba[*pix] );
              pix++;
            }
          } // if paletted image

          if (image->i.samples == 2)
          {
            uchar* pix1 = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
            uchar* pix2 = ((uchar *) image->bits[1]) + lineSizeBytes * cinfo.next_scanline;
            for (int i=0; i<w; i++) 
            {
              *row++ = *pix1;
              *row++ = *pix2;
              *row++ = 0;
              pix1++; pix2++;
            }
          } // if 2 samples

          if (image->i.samples >= 3)
          {
            uchar* pix1 = ((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
            uchar* pix2 = ((uchar *) image->bits[1]) + lineSizeBytes * cinfo.next_scanline;
            uchar* pix3 = ((uchar *) image->bits[2]) + lineSizeBytes * cinfo.next_scanline;
            for (int i=0; i<w; i++) 
            {
              *row++ = *pix1;
              *row++ = *pix2;
              *row++ = *pix3;
              pix1++; pix2++; pix3++;
            }
          } // if 3 or more samples
        } // if not gray
      } // 8 bits per sample


      //if 16 bits per sample
      if (image->i.depth == 16)
      {
        if (image->i.samples == 1)
        {
          uint16* pix = (uint16*) (((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline);

          for (int i=0; i<w; i++) 
          {
            *row++ = (uchar) (*pix / 256);
            ++pix;
          }
        } // if paletted image

        if (image->i.samples == 2)
        {
          uint16* pix1 = (uint16*) (((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline);
          uint16* pix2 = (uint16*) (((uchar *) image->bits[1]) + lineSizeBytes * cinfo.next_scanline);
          for (int i=0; i<w; i++) 
          {
            *row++ = (uchar) (*pix1 / 256);
            *row++ = (uchar) (*pix2 / 256);
            *row++ = 0;
            ++pix1; ++pix2;
          }
        } // if 2 samples

        if (image->i.samples >= 3)
        {
          uint16* pix1 = (uint16*) (((uchar *) image->bits[0]) + lineSizeBytes * cinfo.next_scanline);
          uint16* pix2 = (uint16*) (((uchar *) image->bits[1]) + lineSizeBytes * cinfo.next_scanline);
          uint16* pix3 = (uint16*) (((uchar *) image->bits[2]) + lineSizeBytes * cinfo.next_scanline);
          for (int i=0; i<w; i++) 
          {
            *row++ = (uchar) (*pix1 / 256);
            *row++ = (uchar) (*pix2 / 256);
            *row++ = (uchar) (*pix3 / 256);
            ++pix1; ++pix2; ++pix3;
          }
        } // if 3 or more samples

      } // 16 bits per sample

      jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
  }

  delete iod_dest;
  delete row_pointer[0];

  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint jpeg_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (!hash) return 1;
  if ( isCustomReading ( fmtHndl ) ) return 1;
  if (!fmtHndl->fileName) return 1;

  // use EXIV2 to read metadata
  exiv_append_metadata (fmtHndl, hash );

  return 0;
}


