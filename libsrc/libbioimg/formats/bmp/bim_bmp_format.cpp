/*****************************************************************************
  BMP support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/22/2004 13:06 - First creation
    05/10/2004 14:55 - Big endian support
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
        
  Ver : 3
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <bim_metatags.h>

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

#include "bim_bmp_format.h"

using namespace bim;


//****************************************************************************
//
// INTERNAL STRUCTURES
//
//****************************************************************************

bool bmpGetImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  BmpParams *bmpPar = (BmpParams *) fmtHndl->internalParams;
  ImageInfo *info = &bmpPar->i;  

  *info = initImageInfo();
  info->number_pages = 1;
  info->samples = 1;


  if (fmtHndl->stream == NULL) return false;
  if (xseek(fmtHndl, 0, SEEK_SET) != 0) return false;
  if ( xread( fmtHndl, &bmpPar->bf, 1, sizeof(BITMAPFILEHEADER) ) != 
       sizeof(BITMAPFILEHEADER)) return false;
  
  // test if is BMP
  unsigned char *mag_num = (unsigned char *) &bmpPar->bf.bfType;
  if ( (mag_num[0] != 0x42) || (mag_num[1] != 0x4d) ) return false;

  // swap structure elements if running on Big endian machine...
  if (bim::bigendian) {
    swapLong( (uint32*) &bmpPar->bf.bfOffBits );
    swapLong( (uint32*) &bmpPar->bf.bfSize );
  }
 
  if ( xread( fmtHndl, &bmpPar->bi, 1, sizeof(BITMAPINFOHEADER) ) != 
       sizeof(BITMAPINFOHEADER)) return false;

  // swap structure elements if running on Big endian machine...
  if (bim::bigendian) {
    swapLong( (uint32*) &bmpPar->bi.biSize );    
    swapLong( (uint32*) &bmpPar->bi.biWidth );  
    swapLong( (uint32*) &bmpPar->bi.biHeight );  
    swapShort( (uint16*) &bmpPar->bi.biPlanes );
    swapShort( (uint16*) &bmpPar->bi.biBitCount );
    swapLong( (uint32*) &bmpPar->bi.biCompression );    
    swapLong( (uint32*) &bmpPar->bi.biSizeImage );    
    swapLong( (uint32*) &bmpPar->bi.biXPelsPerMeter );    
    swapLong( (uint32*) &bmpPar->bi.biYPelsPerMeter );    
    swapLong( (uint32*) &bmpPar->bi.biClrUsed );    
    swapLong( (uint32*) &bmpPar->bi.biClrImportant );    
  }

  // test for image parameters
  if ( !(bmpPar->bi.biBitCount == 1  || bmpPar->bi.biBitCount == 4  || bmpPar->bi.biBitCount == 8   || 
         bmpPar->bi.biBitCount == 16 || bmpPar->bi.biBitCount == 24 || bmpPar->bi.biBitCount == 32) ||
	       bmpPar->bi.biPlanes != 1 || bmpPar->bi.biCompression > BIM_BMP_BITFIELDS )
	return false;	// weird BMP image

  if ( !(bmpPar->bi.biCompression == BIM_BMP_RGB || 
        (bmpPar->bi.biBitCount == 4 && bmpPar->bi.biCompression == BIM_BMP_RLE4) ||
	      (bmpPar->bi.biBitCount == 8 && bmpPar->bi.biCompression == BIM_BMP_RLE8) || 
        ((bmpPar->bi.biBitCount == 16 || bmpPar->bi.biBitCount == 32) && bmpPar->bi.biCompression == BIM_BMP_BITFIELDS)) )
	return false;	// weird compression type

  info->width  = bmpPar->bi.biWidth;
  info->height = bmpPar->bi.biHeight;
  info->depth = 8;
  info->samples = 1;
  info->imageMode = IM_GRAYSCALE;

  if (bmpPar->bi.biBitCount < 8) info->depth = bmpPar->bi.biBitCount;
  if (bmpPar->bi.biBitCount > 8) { info->samples = 3; info->imageMode = IM_RGB; }
  if (bmpPar->bi.biBitCount == 32) { info->samples = 4; info->imageMode = IM_RGBA; }



  //-------------------------------------------------
  // init palette
  //-------------------------------------------------
  int num_colors = 0;
  if ( bmpPar->bi.biBitCount <= 8 )
    num_colors = bmpPar->bi.biClrUsed ? bmpPar->bi.biClrUsed : 1 << bmpPar->bi.biBitCount;  
  if (num_colors > 0) info->imageMode = IM_INDEXED;

  info->lut.count = 0;
  for (unsigned int i=0; i<256; i++) info->lut.rgba[i] = xRGB(i, i, i);

  
  //-------------------------------------------------
  // read palette
  //-------------------------------------------------
  if (xseek( fmtHndl, BMP_FILEHDR_SIZE + bmpPar->bi.biSize, SEEK_SET ) != 0) return false;

  if ( num_colors > 0 ) { // LUT is present
    uint8 rgb[4];
    info->lut.count = num_colors;
    int   rgb_len = bmpPar->bi.biSize == BIM_BMP_OLD ? 3 : 4;
    for ( unsigned int i=0; i<(bim::uint)num_colors; i++ ) {
      if ( xread( fmtHndl, (char *)rgb, 1, rgb_len ) != (unsigned int) rgb_len ) return false;
      info->lut.rgba[i] = xRGB( rgb[2], rgb[1], rgb[0] );
    }
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

int bmpValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < 2) return -1;
  unsigned char *mag_num = (unsigned char *) magic;
  if ( (mag_num[0] == 0x42) && (mag_num[1] == 0x4d) ) return 0;
  return -1;
}

FormatHandle bmpAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void bmpCloseImageProc (FormatHandle *fmtHndl);
void bmpReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  bmpCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void bmpCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  xclose ( fmtHndl );
  xfree ( &fmtHndl->internalParams );
}

bim::uint bmpOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) bmpCloseImageProc (fmtHndl);  
  fmtHndl->internalParams = (void *) new BmpParams [1];


  fmtHndl->io_mode = io_mode;
  xopen(fmtHndl);
  if (!fmtHndl->stream) {
      bmpCloseImageProc(fmtHndl);
      return 1;
  };

  if (io_mode == IO_READ && !bmpGetImageInfo(fmtHndl)) {
      bmpCloseImageProc (fmtHndl); 
      return 1;
  }

  return 0;
}


bim::uint bmpFOpenImageProc (FormatHandle *fmtHndl, char* fileName, ImageIOModes io_mode)
{
  fmtHndl->fileName = fileName;
  return bmpOpenImageProc(fmtHndl, io_mode);
}

bim::uint bmpIOpenImageProc (FormatHandle *fmtHndl, char* fileName, 
                                         BIM_IMAGE_CLASS *image, ImageIOModes io_mode)
{
  fmtHndl->fileName = fileName;
  fmtHndl->image    = image;
  return bmpOpenImageProc(fmtHndl, io_mode);
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint bmpGetNumPagesProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;

  return 1;
}


ImageInfo bmpGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num )
{
  ImageInfo ii = initImageInfo();
  page_num;

  if (fmtHndl == NULL) return ii;
  BmpParams *bmpPar = (BmpParams *) fmtHndl->internalParams;

  return bmpPar->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint bmpAddMetaDataProc (FormatHandle *)
{
  return 1;
}


bim::uint bmpReadMetaDataProc (FormatHandle *, bim::uint , int , int , int )
{
  return 1;
}

char* bmpReadMetaDataAsTextProc ( FormatHandle * )
{
  return NULL;
}

//****************************************************************************
// READ PROC
//****************************************************************************

static int read_bmp_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  BmpParams *bmpPar = (BmpParams *) fmtHndl->internalParams;
  ImageInfo *info = &bmpPar->i;  
  ImageBitmap *img = fmtHndl->image;
  
  //-------------------------------------------------
  // init the image
  //-------------------------------------------------
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  //-------------------------------------------------
  // read the image
  //-------------------------------------------------

  // offset can be bogus, verify
  if ( bmpPar->bf.bfOffBits <= 0 ) return false;
  if ( xseek( fmtHndl, bmpPar->bf.bfOffBits, SEEK_SET ) != 0 ) return false;
  
  int bpl = getLineSizeInBytes( img );
  unsigned long bplF = (( bmpPar->bi.biWidth * bmpPar->bi.biBitCount + 31)/32)*4;

    
  //-------------------------------------------------
  // 1,4,8 bit uncompressed BMP image
  //-------------------------------------------------
  if ( (bmpPar->bi.biBitCount == 1) || ( (bmpPar->bi.biBitCount <= 8) && (bmpPar->bi.biCompression == BIM_BMP_RGB) ) ) 
  {       
    int h = bmpPar->bi.biHeight;
    unsigned char *buf = new unsigned char [bplF];      
    
    while ( --h >= 0 ) {
      xprogress( fmtHndl, bmpPar->bi.biHeight-h, bmpPar->bi.biHeight, "Reading BMP" );

      uchar *p = ((unsigned char *) img->bits[0]) + ( h * bpl );
      if ( xread( fmtHndl, buf, 1, bplF ) != bplF) return 1;
      memcpy( p, buf, bpl );
    }
    delete [] buf;
  }

  //-------------------------------------------------
  // 4 bit BMP image
  //-------------------------------------------------  
  if ( ( bmpPar->bi.biBitCount == 4 ) && (bmpPar->bi.biCompression == BIM_BMP_RLE4) )
  {      
    /*
    int    buflen = ((w+7)/8)*4;
    uchar *buf    = new uchar[buflen];
    Q_CHECK_PTR( buf );

    int x=0, y=0, b, c, i;
    register uchar *p = line[h-1];
    uchar *endp = line[h-1]+w;
    while ( y < h ) {
      if ( (b=d->getch()) == EOF )
        break;
      if ( b == 0 ) {     // escape code
        switch ( (b=d->getch()) ) {
        case 0:     // end of line
          x = 0;
          y++;
          p = line[h-y-1];
          break;
        case 1:     // end of image
        case EOF:   // end of file
          y = h;    // exit loop
          break;
        case 2:     // delta (jump)
          x += d->getch();
          y += d->getch();
          
          // Protection
          if ( (bim::uint)x >= (bim::uint)w )
            x = w-1;
          if ( (bim::uint)y >= (bim::uint)h )
            y = h-1;
          
          p = line[h-y-1] + x;
          break;
        default:    // absolute mode
          // Protection
          if ( p + b > endp )
            b = endp-p;
          
          i = (c = b)/2;
          while ( i-- ) {
            b = d->getch();
            *p++ = b >> 4;
            *p++ = b & 0x0f;
          }
          if ( c & 1 )
            *p++ = d->getch() >> 4;
          if ( (((c & 3) + 1) & 2) == 2 )
            d->getch(); // align on word boundary
          x += c;
        }
      } else {      // encoded mode
        // Protection
        if ( p + b > endp )
          b = endp-p;
        
        i = (c = b)/2;
        b = d->getch();   // 2 pixels to be repeated
        while ( i-- ) {
          *p++ = b >> 4;
          *p++ = b & 0x0f;
        }
        if ( c & 1 )
          *p++ = b >> 4;
        x += c;
      }
    }
    */
  } // 4 bits

  //-------------------------------------------------
  // 8 bit BMP image RLE
  //-------------------------------------------------  
  if ( ( bmpPar->bi.biBitCount == 8 ) && (bmpPar->bi.biCompression == BIM_BMP_RLE8) )
  {    
    /*
    int x=0, y=0, b;
    register uchar *p = line[h-1];
    while ( y < h ) {
      if ( (b=d->getch()) == EOF )
        break;
      if ( b == 0 ) {     // escape code
        switch ( (b=d->getch()) ) {
        case 0:     // end of line
          x = 0;
          y++;
          p = line[h-y-1];
          break;
        case 1:     // end of image
        case EOF:   // end of file
          y = h;    // exit loop
          break;
        case 2:     // delta (jump)
          x += d->getch();
          y += d->getch();
          p = line[h-y-1] + x;
          break;
        default:    // absolute mode
          if ( d->readBlock( (char *)p, b ) != b )
            return false;
          if ( (b & 1) == 1 )
            d->getch(); // align on word boundary
          x += b;
          p += b;
        }
      } else {      // encoded mode
        memset( p, d->getch(), b ); // repeat pixel
        x += b;
        p += b;
      }
    }
    */
  }
  
  //-------------------------------------------------
  // xRGB BMP image
  //-------------------------------------------------  
  if ( bmpPar->bi.biBitCount == 16 || bmpPar->bi.biBitCount == 24 || bmpPar->bi.biBitCount == 32 )
  {  
    int h = bmpPar->bi.biHeight;
    unsigned char *bufF = new unsigned char [bplF];

    while ( --h >= 0 ) {
      xprogress( fmtHndl, bmpPar->bi.biHeight-h, bmpPar->bi.biHeight, "Reading BMP" );
      if ( xtestAbort( fmtHndl ) == 1) break;  

      if ( xread( fmtHndl, bufF, bplF, 1 ) != 1 ) return 1;

      if ( bmpPar->bi.biBitCount == 16 ) {
        // nothing yet
        // 565 - format
      }
      
      if ( bmpPar->bi.biBitCount == 24 ) {
        unsigned long x = 0;
        uchar *p0 = ((unsigned char *) img->bits[0]) + ( h * bpl );
        uchar *p1 = ((unsigned char *) img->bits[1]) + ( h * bpl );
        uchar *p2 = ((unsigned char *) img->bits[2]) + ( h * bpl );
        for ( x=0; x<(unsigned long)bpl*3; x+=3 ) {
          *p0 = bufF[x+2]; p0++; // R
          *p1 = bufF[x+1]; p1++; // G
          *p2 = bufF[x+0]; p2++; // B
        }
      }

      if ( bmpPar->bi.biBitCount == 32 ) {
        unsigned long x = 0;
        uchar *p0 = ((unsigned char *) img->bits[0]) + ( h * bpl );
        uchar *p1 = ((unsigned char *) img->bits[1]) + ( h * bpl );
        uchar *p2 = ((unsigned char *) img->bits[2]) + ( h * bpl );
        uchar *p3 = ((unsigned char *) img->bits[3]) + ( h * bpl );
        for ( x=0; x<(unsigned long)bpl*4; x+=4 ) {
          *p0 = bufF[x+2]; p0++; // R          
          *p1 = bufF[x+1]; p1++; // G
          *p2 = bufF[x+0]; p2++; // B
          *p3 = bufF[x+3]; p3++; // A
        }
      }

    }
    delete[] bufF;
  }

  return 0;
}


//****************************************************************************
// WRITE PROC
//****************************************************************************

static int write_bmp_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  BmpParams *bmpPar = (BmpParams *) fmtHndl->internalParams;
  ImageBitmap *img = fmtHndl->image;
  //ImageInfo *info = &img->i;   
  
  uint64 bpl = getLineSizeInBytes( img );
  uint64 num_cols = getImgNumColors( img );
  uint64 nbits = img->i.depth * img->i.samples;
  uint64 bpl_bmp = (( img->i.width * nbits + 31)/32)*4;

  //-------------------------------------------------
  // write BMP header
  //-------------------------------------------------  
  memcpy( &bmpPar->bf.bfType, "BM", 2 );
  bmpPar->bf.bfReserved1 = bmpPar->bf.bfReserved2 = 0;  // reserved, should be zero
  if (nbits <= 8)
    bmpPar->bf.bfOffBits = static_cast<uint32>(BMP_FILEHDR_SIZE + BIM_BMP_WIN + num_cols*4);
  else
    bmpPar->bf.bfOffBits = BMP_FILEHDR_SIZE + BIM_BMP_WIN;
  bmpPar->bf.bfSize      = static_cast<uint32>(bmpPar->bf.bfOffBits + bpl_bmp * img->i.height);

  // swap structure elements if running on Big endian machine...
  if (bim::bigendian) {
    swapLong( (uint32*) &bmpPar->bf.bfOffBits );
    swapLong( (uint32*) &bmpPar->bf.bfSize );
  }

  if (xwrite( fmtHndl, &bmpPar->bf, 1, sizeof(bmpPar->bf) ) != sizeof(bmpPar->bf)) return 1;

  //-------------------------------------------------
  // write image header
  //------------------------------------------------- 
  bmpPar->bi.biSize          = BIM_BMP_WIN;   // build info header
  bmpPar->bi.biWidth         = static_cast<uint32>(img->i.width);
  bmpPar->bi.biHeight        = static_cast<uint32>(img->i.height);
  bmpPar->bi.biPlanes        = 1;
  bmpPar->bi.biBitCount      = (unsigned short) nbits;
  bmpPar->bi.biCompression   = BIM_BMP_RGB;
  bmpPar->bi.biSizeImage     = static_cast<uint32>(bpl_bmp * img->i.height);
  bmpPar->bi.biXPelsPerMeter = 2834; // 72 dpi default
  bmpPar->bi.biYPelsPerMeter = 2834;
  if (nbits <= 8) {
    bmpPar->bi.biClrUsed       = static_cast<uint32>(num_cols);
    bmpPar->bi.biClrImportant  = static_cast<uint32>(num_cols);
  }
  else {
    bmpPar->bi.biClrUsed       = 0;
    bmpPar->bi.biClrImportant  = 0;
  }

  if (bim::bigendian) {
    swapLong( (uint32*) &bmpPar->bi.biSize );    
    swapLong( (uint32*) &bmpPar->bi.biWidth );  
    swapLong( (uint32*) &bmpPar->bi.biHeight );  
    swapShort( (uint16*) &bmpPar->bi.biPlanes );
    swapShort( (uint16*) &bmpPar->bi.biBitCount );
    swapLong( (uint32*) &bmpPar->bi.biCompression );    
    swapLong( (uint32*) &bmpPar->bi.biSizeImage );    
    swapLong( (uint32*) &bmpPar->bi.biXPelsPerMeter );    
    swapLong( (uint32*) &bmpPar->bi.biYPelsPerMeter );    
    swapLong( (uint32*) &bmpPar->bi.biClrUsed );    
    swapLong( (uint32*) &bmpPar->bi.biClrImportant );    
  }

  if (xwrite( fmtHndl, &bmpPar->bi, 1, sizeof(bmpPar->bi) ) != sizeof(bmpPar->bi)) return 1;

  //-------------------------------------------------
  // write image palette if there's any
  //------------------------------------------------- 
  unsigned int i;
  if (nbits <= 8)
  {    
    unsigned char *color_table = new unsigned char [ num_cols*4 ];
    unsigned char *rgb = color_table;

    if (img->i.lut.count > 0)
      for ( i=0; i<num_cols; i++ ) 
      {
        *rgb++ = (unsigned char) xB ( img->i.lut.rgba[i] );
        *rgb++ = (unsigned char) xG ( img->i.lut.rgba[i] );
        *rgb++ = (unsigned char) xR ( img->i.lut.rgba[i] );
        *rgb++ = 0;
      }
    else
      for ( i=0; i<num_cols; i++ ) 
      {
        *rgb++ = bim::trim<uchar, uint64>(i * (256/num_cols));
        *rgb++ = bim::trim<uchar, uint64>(i * (256/num_cols));
        *rgb++ = bim::trim<uchar, uint64>(i * (256/num_cols));
        *rgb++ = 0;
      }

    if ( xwrite( fmtHndl, color_table, 1, num_cols*4 ) != num_cols*4) return 1;
    delete [] color_table;
  }


  //-------------------------------------------------
  // write image data
  //-------------------------------------------------
  unsigned char *buf = new unsigned char[bpl_bmp];
  memset( buf, 0, bpl_bmp );
  int h = (int) img->i.height;

  while ( --h >= 0 ) {
    
    if (nbits <= 8)  {
      uchar *p = ((unsigned char *) img->bits[0]) + ( h * bpl );
      memcpy( buf, p, bpl );
    }
  
    if ( nbits == 16 ) {
      // nothing yet
    }
    
    if ( ( img->i.depth == 8 ) && ( img->i.samples == 3 ) )
    {
      unsigned long x = 0;
      uchar *p0 = ((unsigned char *) img->bits[0]) + ( h * bpl );
      uchar *p1 = ((unsigned char *) img->bits[1]) + ( h * bpl );
      uchar *p2 = ((unsigned char *) img->bits[2]) + ( h * bpl );
      for ( x=0; x<bpl*3; x+=3 )
      {
        buf[x+2] = *p0++; // R
        buf[x+1] = *p1++; // G
        buf[x+0] = *p2++; // B
      }
    }

    if ( ( img->i.depth == 8 ) && ( img->i.samples == 4 ) )
    {
      unsigned long x = 0;
      uchar *p0 = ((unsigned char *) img->bits[0]) + ( h * bpl );
      uchar *p1 = ((unsigned char *) img->bits[1]) + ( h * bpl );
      uchar *p2 = ((unsigned char *) img->bits[2]) + ( h * bpl );
      uchar *p3 = ((unsigned char *) img->bits[3]) + ( h * bpl );
      for ( x=0; x<bpl*4; x+=4 )
      {
        buf[x+3] = *p0++; // R
        buf[x+2] = *p1++; // G
        buf[x+1] = *p2++; // B
        buf[x+0] = *p3++; // A
      }
    }

    if (xwrite( fmtHndl, buf, 1, bpl_bmp ) != bpl_bmp) return 1;

  } // while --h
  delete[] buf;

  xflush( fmtHndl );
  return 0;

}



//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint bmpReadImageProc  ( FormatHandle *fmtHndl, bim::uint page )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;

  fmtHndl->pageNumber = page;
  return read_bmp_image( fmtHndl );
}

bim::uint bmpWriteImageProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;
  return write_bmp_image( fmtHndl );
}

//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

FormatItem bmpItems[1] = {
{
    "BMP",            // short name, no spaces
    "Windows Bitmap", // Long format name
    "bmp",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    0, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 4, 1, 8, 0 } 
  }  
};

FormatHeader bmpHeader = {

  sizeof(FormatHeader),
  "1.0.0",
  "BMP",
  "BMP CODEC",
  
  2,                     // 0 or more, specify number of bytes needed to identify the file
  {1, 1, bmpItems},   // dimJpegSupported,
  
  bmpValidateFormatProc,
  // begin
  bmpAquireFormatProc, //AquireFormatProc
  // end
  bmpReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  bmpOpenImageProc, //OpenImageProc
  bmpCloseImageProc, //CloseImageProc 

  // info
  bmpGetNumPagesProc, //GetNumPagesProc
  bmpGetImageInfoProc, //GetImageInfoProc


  // read/write
  bmpReadImageProc, //ReadImageProc 
  bmpWriteImageProc, //WriteImageProc
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

  NULL,
  NULL,
  NULL,
  ""

};

extern "C" {
FormatHeader* bmpGetFormatHeader(void) { return &bmpHeader; }
} // extern C

