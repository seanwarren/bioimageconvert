/*****************************************************************************
  NANOSCOPE IO
  UCSB/BioITR property  
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    01/10/2005 12:17 - First creation
    02/08/2005 22:30 - Support for incomplete image sections
        
  Ver : 2
*****************************************************************************/

#include <string>
#include <climits>
#include <algorithm>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_nanoscope_format.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

#include <math.h>
#if defined(WIN32)
#include <limits.h>
#endif

#ifndef USHRT_MAX
  #define USHRT_MAX 0xffff
#endif

#ifndef SHRT_MAX
  #define SHRT_MAX 32767
#endif

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

static int read_nanoscope_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
  ImageInfo *info = &nanoPar->i; 
  if (fmtHndl->stream == NULL) return 1;
  
  int page = std::min<int>(fmtHndl->pageNumber, nanoPar->imgs.size()-1 );
  NanoscopeImg nimg = nanoPar->imgs.at(page);

  ImageBitmap *img = fmtHndl->image;
  info->width  = nimg.width;
  info->height = nimg.height;
  info->imageMode = IM_GRAYSCALE;
  info->samples = nanoPar->channels;
  info->depth = 16;
  unsigned long img_size_bytes = nimg.width * nimg.height * 2;

  // init the image
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;
  int line_size = getLineSizeInBytes(img);
  unsigned char *pbuf = new unsigned char [ line_size ];

  for (int sample=0; sample<nanoPar->channels; ++sample) {

    nimg = nanoPar->imgs.at( std::min<int>(page+sample, nanoPar->imgs.size()-1 ) );

    if ( xseek(fmtHndl, nimg.data_offset, SEEK_SET) != 0) return 1;
    if (xread( fmtHndl, img->bits[sample], img_size_bytes, 1 ) != 1) return 1;
    if ( (img->i.depth == 16) && (bim::bigendian) ) 
      swapArrayOfShort((uint16*) img->bits[sample], img_size_bytes/2);
    
    // now convert from SHORT to USHORT
    if (img->i.depth == 16) {
      int16 *pS  = (int16 *)  img->bits[sample];
      uint16 *pU = (uint16 *) img->bits[sample];
      for (unsigned int x=0; x<img_size_bytes/2; x++)
        pU[x] = (unsigned short) (pS[x] + SHRT_MAX);
    }

    // now convert from bottom-top to top-bottom
    unsigned char *pt = ((unsigned char*) img->bits[sample] ) + (line_size * (nimg.height-1));
    unsigned char *pb = (unsigned char*) img->bits[sample];

    for (int y=0; y<floor(info->height/2.0); y++) {
      xprogress( fmtHndl, y, (long) floor(info->height/2.0), "Reading NANOSCOPE" );
      if ( xtestAbort( fmtHndl ) == 1) break; 
      memcpy( pbuf, pb, line_size );
      memcpy( pb, pt, line_size );
      memcpy( pt, pbuf, line_size );
      pb+=line_size;
      pt-=line_size;
    }
  } // sample
  delete [] pbuf;

  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint nanoscope_add_one_tag (FormatHandle *fmtHndl, int tag, const char* str)
{
  uchar *buf = NULL;
  uint32 buf_size = strlen(str);
  uint32 buf_type = TAG_ASCII;

  if ( (buf_size == 0) || (str == NULL) ) return 1;
  else
  {
    // now add tag into structure
    TagItem item;

    buf = (unsigned char *) xmalloc(fmtHndl, buf_size + 1);
    strncpy((char *) buf, str, buf_size);
    buf[buf_size] = '\0';

    item.tagGroup  = META_BIORAD;
    item.tagId     = tag;
    item.tagType   = buf_type;
    item.tagLength = buf_size;
    item.tagData   = buf;

    addMetaTag(&fmtHndl->metaData, item);
  }

  return 0;
}

bim::uint read_nanoscope_metadata (FormatHandle *, int , int , int )
{
  return 0;
}

char* read_text_nanoscope_metadata ( FormatHandle *fmtHndl )
{
  return NULL;
  /*
  if (fmtHndl == NULL) return NULL;
  if (fmtHndl->internalParams == NULL) return NULL;
  NanoscopeParams *nanoPar = (NanoscopeParams *) fmtHndl->internalParams;
  
  NanoscopeImg nimg = nanoPar->imgs.at(0);
  if (fmtHndl->pageNumber < nanoPar->imgs.size())
    nimg = nanoPar->imgs.at( fmtHndl->pageNumber );


  char *buf = NULL;
  std::string str = "";
  char cstr[1024], *line, *l2, p1[1024], p2[1024];

  

  str += "[Nanoscope page properties]\n";

  sprintf( cstr, "Width: %.2f um\n", nimg.width * nimg.xR );
  str += cstr;

  sprintf( cstr, "Height: %.2f um\n", nimg.height * nimg.yR );
  str += cstr;

  // Microscope mode 
  //  \@MicroscopeList: S [TMMode] "Tapping"
  //  \@MicroscopeList: S [AFMMode] "Contact"
  line = strstr( (char*) nanoPar->metaText.c_str(), "@MicroscopeList: S [" );
  if (line != NULL) {
    sscanf( line, "@MicroscopeList: S [%s ""%s""", p1, p2 );
    sprintf( cstr, "Microscope mode: %s\n", p2 );
    if (cstr != NULL) str += cstr;
  }

  // Depth
  
  
  // Deflection


  // \Date: 06:32:07 PM Thu Dec 09 2004
  line = strstr( (char*) nanoPar->metaText.c_str(), "Date: " );
  if (line != NULL) {
    l2 = strstr( line, "\n" );
    memset( p1, 0, 1024 );
    strncpy( p1, line, l2-line );
    if (l2 != NULL) str += p1;
    str += "\n";
  }

  str += "\n";
  str += nimg.metaText;
  str += "\n";
  str += nanoPar->metaText;

  buf = new char [str.size()+1];
  buf[str.size()] = '\0';
  memcpy( buf, str.c_str(), str.size() );
  
  return buf;*/
}

//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

const char months[13][4] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int nanoscope_getMonthNum(const char *buf) {
  for (int i=1; i<=12; i++)
    if (strncmp( months[i], buf, 3 ) == 0) return i;
  return 0;
}

bim::uint nanoscope_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  NanoscopeParams *par = (NanoscopeParams *) fmtHndl->internalParams; 

  // datetime
  char *line = strstr( (char*) par->metaText.c_str(), "Date: " );
  if (line) {
    int h=0, mi=0, s=0, m=0, d=0, y=0;
    char t[1024], month[10];
    sscanf( line, "Date: %d:%d:%d %2s %3s %3s %d %d", &h, &mi, &s, t, t, month, &d, &y ); 
    m = nanoscope_getMonthNum( month );
    hash->append_tag( bim::IMAGE_DATE_TIME, xstring::xprintf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", y, m, d, h, mi, s) );
  }

  // channel names
  if (par->channels>1)
    for (int i=0; i<par->imgs.size(); ++i)
      hash->set_value( xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), par->imgs.at(i).data_type );
  else
    for (int i=0; i<par->imgs.size(); ++i)
      hash->set_value( xstring::xprintf(bim::IMAGE_NAME_TEMPLATE.c_str(), i), par->imgs.at(i).data_type );

  // Parse some of the notes
  hash->parse_ini( par->metaText, ":", bim::CUSTOM_TAGS_PREFIX );

  return 0;
}

