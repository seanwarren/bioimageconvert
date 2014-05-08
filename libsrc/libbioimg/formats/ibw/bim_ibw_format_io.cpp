/*****************************************************************************
  Igor binary file format v5 (IBW) / IO
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    10/19/2005 16:03 - First creation
            
  Ver : 1
*****************************************************************************/

#include <string>

#include <cstring>
#include <cmath>
#include <climits>
#include <cfloat>

#include "bim_ibw_format.h"

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

static int read_ibw_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;
  ImageInfo *info = &ibwPar->i; 
  if (fmtHndl->stream == NULL) return 1;
  
  // get needed page 
  if (fmtHndl->pageNumber < 0) fmtHndl->pageNumber = 0; 
  if (fmtHndl->pageNumber > info->number_pages) fmtHndl->pageNumber = info->number_pages-1;
  int page=fmtHndl->pageNumber;

  //allocate image
  ImageBitmap *img = fmtHndl->image;
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  //read page
  long ch_num_points = info->width * info->height;
  long ch_size = ch_num_points * ibwPar->real_bytespp; 
  uchar *chbuf = new uchar [ch_size];


  long ch_offset = ibwPar->data_offset + (ch_size * page);

  xprogress( fmtHndl, 0, 10, "Reading IBW" );

  if ( xseek(fmtHndl, ch_offset, SEEK_SET) != 0) return 1;
  if (xread( fmtHndl, chbuf, ch_size, 1 ) != 1) return 1;

  // swap if neede
  if ( (bim::bigendian) && (ibwPar->little_endian == true) ) 
  {
    if ( (ibwPar->wh.type == NT_FP32) || (ibwPar->wh.type == NT_I32) )
      swapArrayOfLong((uint32*) chbuf, ch_size/4);

    if (ibwPar->wh.type == NT_FP64)
      swapArrayOfDouble((float64*) chbuf, ch_size/8);

    if (ibwPar->wh.type == NT_I16)
      swapArrayOfShort((uint16*) chbuf, ch_size/2);
    
    if (ibwPar->wh.type == NT_CMPLX)
      swapArrayOfLong((uint32*) chbuf, ch_size/4);
  }

  // normalize and copy
  if (ibwPar->wh.type == NT_FP32)
  {
    float max_val = FLT_MIN;
    float min_val = FLT_MAX;
    float *pb = (float *) chbuf;
    long x = 0;

    // find min and max
    for (x=0; x<ch_num_points; ++x) 
    {
      if (*pb > max_val) max_val = *pb;
      if (*pb < min_val) min_val = *pb;
      ++pb;
    }

    double range = (max_val - min_val) / 256.0;
    if (range == 0) range = 256;

    pb = (float *) chbuf;
    uchar *p = (uchar *) img->bits[0];

    // direct data copy
    for (x=0; x<ch_num_points; ++x) {
      *p = trim<uint8, int>((*pb - min_val) / range);
      ++pb;
      ++p;
    }
    
    /*
    // copy transposing the data
    long line_size = info->width;
    long y=0;

    for (y=0; y<info->height; ++y) 
    {
      p = ( (uchar *) img->bits[0]) + y;     
      for (x=0; x<info->width; ++x) 
      {
        *p = iTrimUC ( (*pb - min_val) / range );
        ++pb;
        p+=line_size;
      }
    }
    */

  } // if (ibwPar->wh.type == NT_FP32) 

  delete [] chbuf;
  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

const char months[13][4] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int ibw_getMonthNum(const char *buf) {
  for (int i=1; i<=12; i++)
    if (strncmp( months[i], buf, 3 ) == 0) return i;
  return 0;
}

bim::uint ibw_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  IbwParams *par = (IbwParams *) fmtHndl->internalParams; 

  //hash->append_tag( "pixel_resolution_x", par->pixel_size[0] );
  //hash->append_tag( "pixel_resolution_y", par->pixel_size[1] );
  //hash->append_tag( "pixel_resolution_z", par->pixel_size[2] );


  // datetime
  //Date: Tue, Sep 20, 2005
  //Time: 10:58:49 AM
  int h=0, mi=0, s=0, m=0, d=0, y=0;

  char *line = strstr( (char*) par->note.c_str(), "Date: " );
  if (line) {
    char t[1024], month[3];
    sscanf( line, "Date: %3s, %3s %d, %d", t, month, &d, &y ); 
    m = ibw_getMonthNum( month );
  }

  line = strstr( (char*) par->note.c_str(), "Time: " );
  if (line)
    sscanf( line, "Time: %d:%d:%d", &h, &mi, &s ); 

  hash->append_tag( bim::IMAGE_DATE_TIME, xstring::xprintf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", y, m, d, h, mi, s) );

  // Parse some of the notes
  hash->parse_ini( par->note, ":", bim::CUSTOM_TAGS_PREFIX );

  return 0;
}






