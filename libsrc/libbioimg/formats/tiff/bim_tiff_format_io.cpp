/*****************************************************************************
  TIFF IO 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  TODO:
    4) read preview image in xRGB 8bit

  History:
    03/29/2004 22:23 - First creation
        
  Ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"
#include "memio.h"
#include "bim_geotiff_parse.h"


// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

bim::uint append_metadata_omeTiff (bim::FormatHandle *fmtHndl, bim::TagMap *hash );
bim::uint omeTiffReadPlane( bim::FormatHandle *fmtHndl, bim::TiffParams *par, int plane );
int omeTiffWritePlane(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams);

using namespace bim;

// must include these guys here if not no access to internal TIFF structs

bim::uint stkReadMetaMeta (FormatHandle *fmtHndl, int group, int tag, int type);
bim::uint append_metadata_stk (FormatHandle *fmtHndl, TagMap *hash );
bim::uint  stkReadPlane(TiffParams *tiffParams, int plane, ImageBitmap *img, FormatHandle *fmtHndl);

bim::uint append_metadata_psia (FormatHandle *fmtHndl, TagMap *hash );
bim::uint psiaReadPlane(FormatHandle *fmtHndl, TiffParams *tiffParams, int plane, ImageBitmap *img);

bim::uint append_metadata_fluoview (FormatHandle *fmtHndl, TagMap *hash );
bim::uint fluoviewReadPlane( FormatHandle *fmtHndl, TiffParams *tiffParams, int plane );

bim::uint append_metadata_lsm (FormatHandle *fmtHndl, TagMap *hash );


//****************************************************************************
// color conversion procs
//****************************************************************************

template< typename T >
void invert_buffer(void *buf, const bim::uint64 &size) {
    T maxval = std::numeric_limits<T>::max();
    T *p = (T *) buf;  
    #pragma omp parallel for default(shared)
    for (bim::int64 i=0; i<size; i++)
        p[i] = maxval - p[i];
}

void invert_buffer_1bit(void *buf, const bim::uint64 &size) {
  int maxval = 1;
  int rest = size%8;
  unsigned int w = (unsigned int) floor( size/8.0 );
  unsigned char *p = (unsigned char *) buf; 
  if (rest>0) ++w;

  for (unsigned int x=0; x<w; ++x) {
    unsigned char b[8];
    b[0] = maxval - (p[x] >> 7);
    b[1] = maxval - ((p[x] & 0x40) >> 6);
    b[2] = maxval - ((p[x] & 0x20) >> 5);
    b[3] = maxval - ((p[x] & 0x10) >> 4);
    b[4] = maxval - ((p[x] & 0x08) >> 3);
    b[5] = maxval - ((p[x] & 0x04) >> 2);
    b[6] = maxval - ((p[x] & 0x02) >> 1);
    b[7] = maxval - (p[x] & 0x01);
    p[x] = (b[0]<<7) + (b[1]<<6) + (b[2]<<5) + (b[3]<<4) + (b[4]<<3) + (b[5]<<2) + (b[6]<<1) + b[7];
  } // for x
}

void invert_buffer_4bit(void *buf, const bim::uint64 &size) {
  int maxval = 15;
  bool even = ( size%2 == 0 );
  unsigned int w = (unsigned int) floor( size/2.0 );
  unsigned char *p = (unsigned char *) buf; 

  for (unsigned int x=0; x<w; ++x) {
    unsigned char b1 = maxval - (p[x] >> 4);
    unsigned char b2 = maxval - (p[x] & 0x0F);
    p[x] = (b1 << 4) + b2;
  } // for x

  // do the last pixel if the size is not even
  if (!even) {
    unsigned char b1 = maxval - (p[w] >> 4);
    p[w] = (b1 << 4);
  }
}

void invertSample(ImageBitmap *img, const int &sample) {
  bim::uint64 size = img->i.width * img->i.height; 

  // all typed will fall here
  if (img->i.depth==8 && img->i.pixelType==FMT_UNSIGNED)
    invert_buffer<bim::uint8>(img->bits[sample], size);
  else
  if (img->i.depth==8 && img->i.pixelType==FMT_SIGNED)
    invert_buffer<bim::int8>(img->bits[sample], size);
  else
  if (img->i.depth==16 && img->i.pixelType==FMT_UNSIGNED)
    invert_buffer<bim::uint16>(img->bits[sample], size);
  else
  if (img->i.depth==16 && img->i.pixelType==FMT_SIGNED)
    invert_buffer<bim::int16>(img->bits[sample], size);
  else
  if (img->i.depth==32 && img->i.pixelType==FMT_UNSIGNED)
    invert_buffer<bim::uint32>(img->bits[sample], size);
  else
  if (img->i.depth==32 && img->i.pixelType==FMT_SIGNED)
    invert_buffer<bim::int32>(img->bits[sample], size);
  else
  if (img->i.depth==32 && img->i.pixelType==FMT_FLOAT)
    invert_buffer<float32>(img->bits[sample], size);
  else
  if (img->i.depth==64 && img->i.pixelType==FMT_FLOAT)
    invert_buffer<float64>(img->bits[sample], size);
  else
  // we still have 1 and 4 bits
  if (img->i.depth==4 && img->i.pixelType==FMT_UNSIGNED)
    invert_buffer_4bit(img->bits[sample], size);
  else
  if (img->i.depth==1 && img->i.pixelType==FMT_UNSIGNED)
    invert_buffer_1bit(img->bits[sample], size);
}

void invertImg(ImageBitmap *img) {
    if (!img) return;
    for (unsigned int sample=0; sample<img->i.samples; sample++)
        invertSample(img, sample);
}

template< typename T >
void image_ycbcr_to_rgb(ImageBitmap *img, TiffParams *pars) {

    #define uint32 bim::uint32
    TIFFYCbCrToRGB* ycbcr = (TIFFYCbCrToRGB*) _TIFFmalloc(
		    TIFFroundup_32(sizeof (TIFFYCbCrToRGB), sizeof (long))  
		    + 4*256*sizeof (TIFFRGBValue)
		    + 2*256*sizeof (int)
		    + 3*256*sizeof (bim::int32)
	);
    #undef uint32
	if (!ycbcr) return;

    bim::uint64 size = img->i.width * img->i.height; 
	float *luma, *refBlackWhite;
	TIFFGetFieldDefaulted(pars->tiff, TIFFTAG_YCBCRCOEFFICIENTS, &luma);
	TIFFGetFieldDefaulted(pars->tiff, TIFFTAG_REFERENCEBLACKWHITE, &refBlackWhite);
	if (TIFFYCbCrToRGBInit(ycbcr, luma, refBlackWhite) >= 0) {
        T *rp = (T *) img->bits[0];
        T *gp = (T *) img->bits[1];  
        T *bp = (T *) img->bits[2];  
        #pragma omp parallel for default(shared)
        for (bim::int64 i=0; i<size; i++) {
            bim::uint32 r, g, b;
            bim::uint32 Y = rp[i];
            bim::int32 Cb = gp[i];
            bim::int32 Cr = bp[i];
            TIFFYCbCrtoRGB(ycbcr, Y, Cb, Cr, &r, &g, &b);
            rp[i] = r; gp[i] = g; bp[i] = b;
        }
    }
	_TIFFfree(ycbcr);
}

void imageYCbCr2RGB(ImageBitmap *img, TiffParams *pars) {
    if (img->i.depth==8 && img->i.pixelType==FMT_UNSIGNED)
        image_ycbcr_to_rgb<bim::uint8>(img, pars);
    else
    if (img->i.depth==8 && img->i.pixelType==FMT_SIGNED)
        image_ycbcr_to_rgb<bim::int8>(img, pars);
    else
    if (img->i.depth==16 && img->i.pixelType==FMT_UNSIGNED)
        image_ycbcr_to_rgb<bim::uint16>(img, pars);
    else
    if (img->i.depth==16 && img->i.pixelType==FMT_SIGNED)
        image_ycbcr_to_rgb<bim::int16>(img, pars);
    else
    if (img->i.depth==32 && img->i.pixelType==FMT_UNSIGNED)
        image_ycbcr_to_rgb<bim::uint32>(img, pars);
    else
    if (img->i.depth==32 && img->i.pixelType==FMT_SIGNED)
        image_ycbcr_to_rgb<bim::int32>(img, pars);
}


template< typename T >
void image_cielab_to_rgb(ImageBitmap *img, TiffParams *pars) {
    TIFFCIELabToRGB *cielab = (TIFFCIELabToRGB*) _TIFFmalloc(sizeof(TIFFCIELabToRGB));
	if (!cielab) return;
	
    TIFFDisplay display_sRGB = {
        { // XYZ -> luminance matrix
            { 3.2410F, -1.5374F, -0.4986F },
            { -0.9692F, 1.8760F, 0.0416F },
            { 0.0556F, -0.2040F, 1.0570F }
        },
        100.0F, 100.0F, 100.0F, // Light o/p for reference white
        255, 255, 255, // Pixel values for ref. white
        1.0F, 1.0F, 1.0F, // Residual light o/p for black pixel
        2.4F, 2.4F, 2.4F, // Gamma values for the three guns
    };

    bim::uint64 size = img->i.width * img->i.height; 
    float *whitePoint;
    float refWhite[3];
    TIFFGetFieldDefaulted(pars->tiff, TIFFTAG_WHITEPOINT, &whitePoint);
    refWhite[1] = 100.0F;
    refWhite[0] = whitePoint[0] / whitePoint[1] * refWhite[1];
    refWhite[2] = (1.0F - whitePoint[0] - whitePoint[1]) / whitePoint[1] * refWhite[1];
	if (TIFFCIELabToRGBInit(cielab, &display_sRGB, refWhite) >= 0) {
        T *rp = (T *) img->bits[0];
        T *gp = (T *) img->bits[1];  
        T *bp = (T *) img->bits[2];  
        #pragma omp parallel for default(shared)
        for (bim::int64 i=0; i<size; i++) {
            bim::uint32 r, g, b;
            float X, Y, Z;
            bim::uint32 L = rp[i];
            bim::int32 A = gp[i];
            bim::int32 B = bp[i];
            TIFFCIELabToXYZ(cielab, L, A, B, &X, &Y, &Z);
            TIFFXYZToRGB(cielab, X, Y, Z, &r, &g, &b);
            rp[i] = r; gp[i] = g; bp[i] = b;
        }
    }
	_TIFFfree(cielab);
}

void imageCIELAB2RGB(ImageBitmap *img, TiffParams *pars) {
    if (img->i.depth==8 && img->i.pixelType==FMT_UNSIGNED)
        image_cielab_to_rgb<bim::uint8>(img, pars);
    else
    if (img->i.depth==8 && img->i.pixelType==FMT_SIGNED)
        image_cielab_to_rgb<bim::int8>(img, pars);
    else
    if (img->i.depth==16 && img->i.pixelType==FMT_UNSIGNED)
        image_cielab_to_rgb<bim::uint16>(img, pars);
    else
    if (img->i.depth==16 && img->i.pixelType==FMT_SIGNED)
        image_cielab_to_rgb<bim::int16>(img, pars);
    else
    if (img->i.depth==32 && img->i.pixelType==FMT_UNSIGNED)
        image_cielab_to_rgb<bim::uint32>(img, pars);
    else
    if (img->i.depth==32 && img->i.pixelType==FMT_SIGNED)
        image_cielab_to_rgb<bim::int32>(img, pars);
}

void processPhotometric(ImageBitmap *img, TiffParams *pars, const bim::uint16 &photometric) {
    TIFF *tif = pars->tiff;
    if (photometric == PHOTOMETRIC_MINISWHITE) {
        invertImg(img);
    } else if (photometric == PHOTOMETRIC_YCBCR && img->i.depth<=32 && img->i.samples==3) {
        imageYCbCr2RGB(img, pars);
    } else if (photometric == PHOTOMETRIC_CIELAB && img->i.depth<=32 && img->i.samples==3) {
        //imageCIELAB2RGB(img, pars); // dima: not tested - will be added with test images
    }
}


//****************************************************************************
// MISC
//****************************************************************************

bool areValidParams(FormatHandle *fmtHndl, TiffParams *tifParams)
{
  if (fmtHndl == NULL) return false;
  if (tifParams == NULL) return false;
  if (tifParams->tiff == NULL) return false;
  if (fmtHndl->image == NULL) return false;

  return true;
}

void init_image_palette( TIFF *tif, ImageInfo *info ) {
  if (tif == NULL) return;
  if (info == NULL) return;
  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 bitspersample = 1;  
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);

  info->lut.count = 0;
  for (bim::uint i=0; i<256; i++) 
    info->lut.rgba[i] = xRGB( i, i, i );
  
  if (photometric == PHOTOMETRIC_PALETTE) { // palette
    bim::uint16 *red, *green, *blue;
    bim::uint num_colors = ( 1L << bitspersample );
    if (num_colors > 256) num_colors = 256;    

    TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);
    for (bim::uint i=0; i<num_colors; i++)
      info->lut.rgba[i] = xRGB( red[i]/256, green[i]/256, blue[i]/256 );

    info->lut.count = num_colors;
  } // if paletted
}

//****************************************************************************
// META DATA
//****************************************************************************

bim::uint read_one_tag (FormatHandle *fmtHndl, TiffParams *tifParams, bim::uint16 tag) {
  if (!areValidParams(fmtHndl, tifParams)) return 1;
  TinyTiff::IFD *ifd = tifParams->ifds.getIfd(fmtHndl->pageNumber);
  if (!ifd) return 1;

  //if (fmtHndl->pageNumber >= tifParams->ifds.count()) { fmtHndl->pageNumber = 0; }
  TIFF *tif = tifParams->tiff;

  uchar *buf=NULL; bim::uint16 buf_type; bim::uint64 buf_size;

  if ( (tifParams->subType == tstStk) && (tag == 33629) ) {// stk 33629 got custom size 6*N
    buf_type = TAG_LONG;
    bim::uint64 count = ifd->tagCount(tag);
    buf_size = ( count * 6 ) * TinyTiff::tag_size_bytes[buf_type];
    ifd->readTagCustom(tag, buf_size, buf_type, (uchar **) &buf);
  }
  else
    ifd->readTag(tag, buf_size, buf_type, (uchar **) &buf);



  if (buf_size==0 || !buf) return 1;
  else
  {
    // now add tag into structure
    TagItem item;

    item.tagGroup  = META_TIFF_TAG;
    item.tagId     = tag;
    item.tagType   = buf_type;
    item.tagLength = (bim::uint32)( buf_size / TinyTiff::tag_size_bytes[buf_type]);
    item.tagData   = buf;

    addMetaTag( &fmtHndl->metaData, item);
  }

  return 0;
}

bim::uint read_tiff_metadata (FormatHandle *fmtHndl, TiffParams *tifParams, int group, int tag, int type)
{
  if (!areValidParams(fmtHndl, tifParams)) return 1;
  if (group == META_BIORAD) return 1;
  TinyTiff::IFD *ifd = tifParams->ifds.getIfd(fmtHndl->pageNumber);
  if (!ifd) return 1;

  // first read custom formatted tags
  if (tifParams->subType == tstStk)
    stkReadMetaMeta (fmtHndl, group, tag, type);

  if (tag != -1)
    return read_one_tag ( fmtHndl, tifParams, tag );

  if ( (group == -1) || (group == META_TIFF_TAG) ) {
    for (bim::uint64 i=0; i<ifd->size(); i++ ) {
      TinyTiff::Entry *entry = ifd->getEntry(i);
      if (type == -1) {
        if (entry->tag>532 && entry->tag!=50434) 
            read_one_tag ( fmtHndl, tifParams, entry->tag );    
    
        switch( entry->tag ) 
        {
          case 269: //DocumentName
          case 270: //ImageDescription
          case 271: //Make
          case 272: //Model
          case 285: //PageName
          case 305: //Software
          case 306: //DateTime
          case 315: //Artist
          case 316: //HostComputer
            read_one_tag ( fmtHndl, tifParams, entry->tag );
            break;
        } // switch
      } // type == -1
      else
      {
        if (entry->type == type)
          read_one_tag ( fmtHndl, tifParams, entry->tag );

      } // type != -1

    } // for i<ifd count
  } // if no group

  return 0;
}

//----------------------------------------------------------------------------
// Textual METADATA
//----------------------------------------------------------------------------

void change_0_to_n (char *str, long size) {
  for (long i=0; i<size; i++)
    if (str[i] == '\0') str[i] = '\n'; 
}

void write_title_text(const char *text, MemIOBuf *outIOBuf)
{
  char title[1024];
  sprintf(title, "\n[%s]\n\n", text);
  MemIO_WriteProc( (thandle_t) outIOBuf, title, strlen(title)-1 );
}

void read_text_tag(TinyTiff::IFD *ifd, bim::uint tag, MemIOBuf *outIOBuf, const char *text) {
    if (!ifd->tagPresent(tag)) return;

    bim::uint64 buf_size;
    bim::uint16 buf_type;  
    uchar *buf = NULL;
    write_title_text(text, outIOBuf);
    ifd->readTag (tag, buf_size, buf_type, &buf);
    change_0_to_n ((char *) buf, (long) buf_size);
    MemIO_WriteProc( (thandle_t) outIOBuf, buf, buf_size );
    _TIFFfree( buf );
}

void read_text_tag(TinyTiff::IFD *ifd, bim::uint tag, MemIOBuf *outIOBuf) {
    if (!ifd->tagPresent(tag)) return;
    bim::uint64 buf_size;
    bim::uint16 buf_type;  
    uchar *buf = NULL;
  
    ifd->readTag (tag, buf_size, buf_type, &buf);
    change_0_to_n ((char *) buf, (long) buf_size);
    MemIO_WriteProc( (thandle_t) outIOBuf, buf, buf_size );
    _TIFFfree( buf );
}

char* read_text_tiff_metadata ( FormatHandle *fmtHndl, TiffParams *tifParams ) {
    return NULL;
}

//----------------------------------------------------------------------------
// New METADATA
//----------------------------------------------------------------------------

bim::uint append_metadata_generic_tiff (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;

  TiffParams *par = (TiffParams *) fmtHndl->internalParams;
  TinyTiff::IFD *ifd = par->ifds.firstIfd();
  if (!ifd) return 1;

  std::map< int, std::string > hash_tiff_tags;
  hash_tiff_tags[269] = "Document Name";
  hash_tiff_tags[270] = "Image Description";
  hash_tiff_tags[285] = "Page Name";
  hash_tiff_tags[271] = "Make";
  hash_tiff_tags[272] = "Model";
  hash_tiff_tags[305] = "Software";
  hash_tiff_tags[306] = "Date Time";
  hash_tiff_tags[315] = "Artist";
  hash_tiff_tags[316] = "Host Computer";

  std::map< int, std::string >::const_iterator it = hash_tiff_tags.begin();
  while (it != hash_tiff_tags.end()) {
    xstring tag_str = ifd->readTagString(it->first);
    if (tag_str.size()>0) hash->append_tag( xstring("custom/") + it->second, tag_str );
    it++;
  }

  // use EXIV2 to read metadata
  exiv_append_metadata (fmtHndl, hash );

  // use GeoTIFF to read metadata
  geotiff_append_metadata(fmtHndl, hash);

  return 0;
}

bim::uint append_metadata_qimaging_tiff (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;

  TiffParams *par = (TiffParams *) fmtHndl->internalParams;
  TinyTiff::IFD *ifd = par->ifds.firstIfd();
  if (!ifd) return 1;

  /*
[Image Description]
Exposure: 000 : 00 : 00 . 300 : 000
Binning: 2 x 2
Gain: 2.000000
%Accumulated%=0

[Software]
QCapture Pro

[Date Time]
08/28/2006 04:34:47.000 PM
  */

  // check if it's QImage tiff file
  // should exist private tags 50288 and 50296
  if (!ifd->tagPresent(50288)) return 0;
  if (!ifd->tagPresent(50296)) return 0;

  // tag 305 should be "QCapture Pro"
  xstring tag_software = ifd->readTagString(305);
  if ( tag_software != "QCapture Pro" ) return 0;
  
  // ok, we're sure it's QImaging
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Software", tag_software );


  xstring tag_description = ifd->readTagString(TIFFTAG_IMAGEDESCRIPTION);
  if (tag_description.size()>0)
    hash->parse_ini( tag_description, ":", bim::CUSTOM_TAGS_PREFIX );

  // read tag 306 - Date/Time
  xstring tag_datetime = ifd->readTagString(306);
  if (tag_datetime.size()>0) {
    int y=0, m=0, d=0, h=0, mi=0, s=0, ms=0;
    char ampm=0;
    //08/28/2006 04:34:47.000 PM
    sscanf( (char *)tag_datetime.c_str(), "%d/%d/%d %d:%d:%d.%d %c", &m, &d, &y, &h, &mi, &s, &ms, &ampm );
    if (ampm == 'P') h += 12;
    tag_datetime.sprintf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", y, m, d, h, mi, s);
    hash->append_tag( bim::IMAGE_DATE_TIME, tag_datetime );
  }

  //hash->append_tag( "pixel_resolution_x", par->pixel_size[0] );
  //hash->append_tag( "pixel_resolution_y", par->pixel_size[1] );
  //hash->append_tag( "pixel_resolution_z", par->pixel_size[2] );

  return 0;
}

bim::uint tiff_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  TiffParams *tifParams = (TiffParams *) fmtHndl->internalParams;

  append_metadata_qimaging_tiff (fmtHndl, hash );

  if (tifParams->subType == tstStk) 
    append_metadata_stk(fmtHndl, hash);
  else
  if (tifParams->subType == tstPsia) 
    append_metadata_psia(fmtHndl, hash);
  else
  if (tifParams->subType==tstFluoview || tifParams->subType==tstAndor)
    append_metadata_fluoview(fmtHndl, hash);
  else
  if (tifParams->subType == tstCzLsm)
    append_metadata_lsm(fmtHndl, hash);
  else
  if (tifParams->subType == tstOmeTiff || tifParams->subType == tstOmeBigTiff)
    append_metadata_omeTiff (fmtHndl, hash);
  else
    append_metadata_generic_tiff(fmtHndl, hash);

  return 0;
}

//----------------------------------------------------------------------------
// Write METADATA
//----------------------------------------------------------------------------


bim::uint write_tiff_metadata (FormatHandle *fmtHndl, TiffParams *tifParams)
{
  if (!areValidParams(fmtHndl, tifParams)) return 1;

  bim::uint i;
  TagList *tagList = &fmtHndl->metaData;
  void  *t_list = NULL;
  bim::int16 t_list_count;
  TIFF *tif = tifParams->tiff;

  if (tagList->count == 0) return 1;
  if (tagList->tags == NULL) return 1;

  for (i=0; i<tagList->count; i++) {
    TagItem *tagItem = &tagList->tags[i];
    if (tagItem->tagGroup == META_TIFF_TAG) {
      t_list = tagItem->tagData;
      t_list_count = tagItem->tagLength;

      TIFFSetField( tif, tagItem->tagId, tagItem->tagLength, tagItem->tagData ); 
    }
  }

  return 0;
}


//****************************************************************************
// WRITING LINE SEGMENT FROM BUFFER
//****************************************************************************

template< typename T >
void write_line_segment_t(void *po, void *bufo, ImageBitmap *img, bim::uint sample, bim::uint64 w) {
    T *p   = (T *) po;
    T *buf = (T *) bufo;  
    bim::uint nsamples = img->i.samples;
    for (bim::int64 x=sample, xi=0; x<w*nsamples; x+=nsamples, ++xi) {
        p[xi] = buf[x];
    }
}

void write_line_segment(void *po, void *bufo, ImageBitmap *img, bim::uint sample, bim::uint64 w) {
  if (img->i.depth==8)  
      write_line_segment_t<bim::uint8>  (po, bufo, img, sample, w);
  else
  if (img->i.depth==16) 
      write_line_segment_t<bim::uint16> (po, bufo, img, sample, w);
  else
  if (img->i.depth==32) 
      write_line_segment_t<bim::uint32> (po, bufo, img, sample, w);
  else
  if (img->i.depth==64) 
      write_line_segment_t<float64>(po, bufo, img, sample, w);
}


//****************************************************************************
// SCANLINE METHOD TIFF
//****************************************************************************

int read_scanline_tiff(TIFF *tif, ImageBitmap *img, FormatHandle *fmtHndl) {
    if (!tif || !img) return -1;
  
    bim::uint lineSize = getLineSizeInBytes( img );
    bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
    bim::uint16 planarConfig;
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

    //TIFFReadEncodedStrip

    if ( (planarConfig == PLANARCONFIG_SEPARATE) || (img->i.samples == 1) ) {
        for (bim::uint sample=0; sample<img->i.samples; sample++) {
            uchar *p = (uchar *) img->bits[sample];
            for(bim::uint y=0; y<img->i.height; y++) {
                xprogress( fmtHndl, y*(sample+1), img->i.height*img->i.samples, "Reading TIFF" );
                if ( xtestAbort( fmtHndl ) == 1) break;  
                if (!TIFFReadScanline(tif, p, y, sample)) return -1;
                p += lineSize;
            } // for y
        }  // for sample
    } else { // if image contain several samples in one same plane ex: RGBRGBRGB...
        uchar *buf = (uchar *) _TIFFmalloc( TIFFScanlineSize ( tif ) );
        for (bim::uint y=0; y<img->i.height; y++) {

            xprogress( fmtHndl, y, img->i.height, "Reading TIFF" );
            if ( xtestAbort( fmtHndl ) == 1) break;  

            if (!TIFFReadScanline(tif, buf, y, 0)) {
                _TIFFfree( buf );
                return -1;
            }
            // process YCrCb, etc data

            for (bim::uint sample=0; sample<img->i.samples; ++sample) {
                uchar *p = (uchar *) img->bits[sample] + (lineSize * y);
                write_line_segment(p, buf, img, sample, img->i.width);
            }  // for sample

        } // for y
        _TIFFfree( buf );
    }  

    return 0;
}

//****************************************************************************
// TILED METHOD TIFF
//****************************************************************************

int read_tiled_tiff(TIFF *tif, ImageBitmap *img, FormatHandle *fmtHndl) {
  if (!tif || !img) return 1;

  // if tiff is not tiled get out and never come back :-)
  if( !TIFFIsTiled(tif) ) return 1;

  bim::uint lineSize = getLineSizeInBytes( img );
  bim::uint bpp = ceil( (double) img->i.depth / 8.0 );
  bim::uint16 planarConfig;
  bim::uint32 columns, rows;
  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig);
  TIFFGetField(tif, TIFFTAG_TILEWIDTH,  &columns);
  TIFFGetField(tif, TIFFTAG_TILELENGTH, &rows);
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

  uchar *tile_buf = (uchar*) _TIFFmalloc( TIFFTileSize(tif) );
  if (!tile_buf) return 1;

  for (bim::uint y=0; y<img->i.height; y+=rows) {
    //if (y > img->i.height) break;

    // the tile height may vary 
    bim::uint tileH = (img->i.height-y >= rows) ? rows : (bim::uint) img->i.height-y;

    xprogress( fmtHndl, y, img->i.height, "Reading TIFF" );
    if ( xtestAbort( fmtHndl ) == 1) break; 

    bim::uint tileW = columns;
    for (bim::uint x=0; x<(bim::uint)img->i.width; x+=columns) {
  
      // the tile size is now treated by libtiff guys the
      // way that the size stay on unchanged      
      bim::uint tW = (img->i.width-x < columns) ? (bim::uint) img->i.width-x : (bim::uint) tileW;
      //if (img->i.width-x < columns) tW = (bim::uint32) img->i.width-x; else tW = (bim::uint32) tileW;


      if ( (planarConfig == PLANARCONFIG_SEPARATE) || (img->i.samples == 1) ) {
        for (bim::uint sample=0; sample<img->i.samples; sample++) {
          if (!TIFFReadTile(tif, tile_buf, x, y, 0, sample)) break;
 
          // now put tile into the image 
          for(bim::uint yi = 0; yi < tileH; yi++) {
              uchar *p = (uchar *) img->bits[sample] + (lineSize * (y+yi));
              _TIFFmemcpy(p+(x*bpp), tile_buf+(yi*tileW*bpp), tW);
          }
        }  // for sample

      } // if planar
      else { // if image contains several samples in one same plane ex: RGBRGBRGB...
        if (!TIFFReadTile(tif, tile_buf, x, y, 0, 0)) break;
        // process YCrCb, etc data

        for (bim::uint sample=0; sample<img->i.samples; sample++) {
          // now put tile into the image 
          #pragma omp parallel for default(shared) 
          for(bim::int64 yi = 0; yi < tileH; yi++) {
            uchar *p = (uchar *) img->bits[sample] + (lineSize * (y+yi));
            write_line_segment(p+(x*bpp), tile_buf+(yi*tileW*img->i.samples*bpp), img, sample, tW);
          }
        }  // for sample
      } // if not separate planes

    } // for x
  } // for y

  _TIFFfree(tile_buf);

  return 0;
}



//****************************************************************************
//*** TIFF READER
//****************************************************************************

// if the file is LSM then the strip size given in the file is incorrect, fix that
// by simply checking against the file size and adjusting if needed
void lsmFixStripByteCounts ( TIFF *tif, bim::uint32 row, tsample_t sample ) {

  TIFFDirectory *td = &tif->tif_dir;
  tiff_strp_t strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
  tiff_bcnt_t bytecount = td->td_stripbytecount[strip];
  if (tif->tif_size <= 0) tif->tif_size = TIFFGetFileSize(tif);

  if ( td->td_stripoffset[strip] + bytecount > tif->tif_size) {
    bytecount = tif->tif_size - td->td_stripoffset[strip];
    td->td_stripbytecount[strip] = bytecount;
  }
}

void getCurrentPageInfo(TiffParams *tiffParams);
int read_tiff_image(FormatHandle *fmtHndl, TiffParams *tifParams) {
  if (!areValidParams(fmtHndl, tifParams)) return 1;

  TIFF *tif = tifParams->tiff;
  ImageBitmap *img = fmtHndl->image;

  bim::uint32 height = 0; 
  bim::uint32 width = 0; 
  bim::uint16 bitspersample = 1;
  bim::uint16 samplesperpixel = 1;
  bim::uint32 rowsperstrip;  
  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 compression = COMPRESSION_NONE;
  bim::uint16 PlanarConfig;
  unsigned int currentDir = 0;

  currentDir = TIFFCurrentDirectory(tif);
  int needed_page_num = fmtHndl->pageNumber;
  if (tifParams->subType == tstCzLsm)
    needed_page_num = fmtHndl->pageNumber*2;

  // now must read correct page and set image parameters
  if (currentDir != needed_page_num)
  if (tifParams->subType != tstStk) {
    TIFFSetDirectory(tif, needed_page_num);

    currentDir = TIFFCurrentDirectory(tif);
    if (currentDir != needed_page_num) return 1;

    getCurrentPageInfo( tifParams );
  }
  
  if (tifParams->subType!=tstOmeTiff && tifParams->subType!=tstOmeBigTiff ) 
      img->i = tifParams->info;

  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
  TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);   
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &PlanarConfig);	// single image plane 

  // this is here due to some OME-TIFF do not conform with the standard and come with all channels in the same IFD
  if (tifParams->subType==tstOmeTiff || tifParams->subType==tstOmeBigTiff) {
    int r = omeTiffReadPlane( fmtHndl, tifParams, fmtHndl->pageNumber );
    if (r != 2) return r;
    img->i = tifParams->info;
  }

  // if image is PSIA then read and init it here
  if (tifParams->subType == tstPsia)
    return psiaReadPlane(fmtHndl, tifParams, fmtHndl->pageNumber, img);

  // if image is Fluoview and contains 1..4 channels
  if ( (tifParams->subType==tstFluoview || tifParams->subType==tstAndor) && (tifParams->fluoviewInfo.ch > 1) )
    return fluoviewReadPlane( fmtHndl, tifParams, fmtHndl->pageNumber );

  // if the file is LSM then the strip size given in the file is incorrect, fix that
  if (tifParams->subType == tstCzLsm)
    for (unsigned int sample=0; sample < samplesperpixel; ++sample)
      for (unsigned int y=0; y < height; ++y)
        lsmFixStripByteCounts( tif, y, sample );

  if ( allocImg( fmtHndl, &img->i, img) != 0 ) return 1;

  // if image is STK
  if (tifParams->subType == tstStk)
    return stkReadPlane(tifParams, fmtHndl->pageNumber, img, fmtHndl);


  if( !TIFFIsTiled(tif) )
    read_scanline_tiff(tif, img, fmtHndl);
  else
    read_tiled_tiff(tif, img, fmtHndl);

  processPhotometric(img, tifParams, photometric);

  return 0;
}

//****************************************************************************
// TIFF WRITER
//****************************************************************************

int write_tiff_image(FormatHandle *fmtHndl, TiffParams *tifParams) {
  if (!areValidParams(fmtHndl, tifParams)) return 1;

  if (tifParams->subType == tstOmeTiff || tifParams->subType == tstOmeBigTiff)
    return omeTiffWritePlane( fmtHndl, tifParams);

  TIFF *out = tifParams->tiff;
  ImageBitmap *img = fmtHndl->image;
 
  bim::uint32 height;
  bim::uint32 width;
  bim::uint32 rowsperstrip = (bim::uint32) -1;
  bim::uint16 bitspersample;
  bim::uint16 samplesperpixel;
  bim::uint16 photometric = PHOTOMETRIC_MINISBLACK;
  bim::uint16 compression;
  bim::uint16 planarConfig;

  width = (bim::uint32) img->i.width;
  height = (bim::uint32) img->i.height;
  bitspersample = img->i.depth;
  samplesperpixel = img->i.samples;
  if (img->i.imageMode == IM_RGB)   photometric = PHOTOMETRIC_RGB;
  if (img->i.imageMode == IM_MULTI) photometric = PHOTOMETRIC_RGB;
  if (samplesperpixel >= 2)         photometric = PHOTOMETRIC_RGB;
  if ( (img->i.imageMode == IM_INDEXED) && (img->i.lut.count > 0) && (samplesperpixel==1) && (bitspersample<=8) )
    photometric = PHOTOMETRIC_PALETTE;

  if ( (bitspersample == 1) && (samplesperpixel == 1) ) photometric = PHOTOMETRIC_MINISWHITE;
  // a failed attempt to force photoshop to load 2 channel image
  //if (samplesperpixel == 2) photometric = PHOTOMETRIC_SEPARATED;

  // handle standard width/height/bpp stuff
  TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
  TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitspersample);
  TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

 
  // set pixel format
  bim::uint16 sampleformat = SAMPLEFORMAT_UINT;
  if (img->i.pixelType == FMT_SIGNED) sampleformat = SAMPLEFORMAT_INT;
  if (img->i.pixelType == FMT_FLOAT)  sampleformat = SAMPLEFORMAT_IEEEFP;
  TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, sampleformat);


  // set planar config
  planarConfig = PLANARCONFIG_SEPARATE;	// separated planes 
  if (samplesperpixel==3 && bitspersample==8)
    planarConfig = PLANARCONFIG_CONTIG;

  /*
  if (img->i.imageMode == IM_MULTI)
    planarConfig = PLANARCONFIG_SEPARATE;	// separated planes 
  else
    planarConfig = PLANARCONFIG_CONTIG;	// mixed planes

  // now more tests for plane configuration
  if (samplesperpixel > 3) planarConfig = PLANARCONFIG_SEPARATE;
  if ( (samplesperpixel == 1) || (samplesperpixel == 3) ) 
    planarConfig = PLANARCONFIG_CONTIG;
  */
 
  TIFFSetField(out, TIFFTAG_PLANARCONFIG, planarConfig);	// separated planes


  TIFFSetField(out, TIFFTAG_SOFTWARE, "DIMIN TIFF WRAPPER <www.dimin.net>");

  //if( TIFFGetField( out, TIFFTAG_DOCUMENTNAME, &pszText ) )
  //if( TIFFGetField( out, TIFFTAG_IMAGEDESCRIPTION, &pszText ) )
  //if( TIFFGetField( out, TIFFTAG_DATETIME, &pszText ) )



  //------------------------------------------------------------------------------  
  // compression
  //------------------------------------------------------------------------------  

  compression = fmtHndl->compression;
  if (compression == 0) compression = COMPRESSION_NONE; 

  switch(bitspersample) {
  case 1  :
    if (compression != COMPRESSION_CCITTFAX4) compression = COMPRESSION_NONE;
    break;

  case 8  :
  case 16 :
  case 32 :
  case 64 :
    if ( (compression != COMPRESSION_LZW) && (compression != COMPRESSION_PACKBITS) )
      compression = COMPRESSION_NONE;  
    break;
  
  default :
    compression = COMPRESSION_NONE;
    break;
  }

  TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

  unsigned long strip_size = bim::max<unsigned long>( TIFFDefaultStripSize(out,-1), 1 );
  switch ( compression ) {
    case COMPRESSION_JPEG:
    {
      TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size+(16-(strip_size % 16)) );
      break;
    }

    case COMPRESSION_ADOBE_DEFLATE:
    {
      TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
      if ( (photometric == PHOTOMETRIC_RGB) ||
           ((photometric == PHOTOMETRIC_MINISBLACK) && (bitspersample >= 8)) )
        TIFFSetField( out, TIFFTAG_PREDICTOR, 2 );
      TIFFSetField( out, TIFFTAG_ZIPQUALITY, 9 );
      break;
    }

    case COMPRESSION_CCITTFAX4:
    {
      TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
      break;
    }

    case COMPRESSION_LZW:
    {
      TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
      if (planarConfig == PLANARCONFIG_SEPARATE)
         TIFFSetField( out, TIFFTAG_PREDICTOR, PREDICTOR_NONE );
      else
         TIFFSetField( out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL );
      break;
    }
    default:
    {
      TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
      break;
    }
  }

  //------------------------------------------------------------------------------  
  // Save resolution
  //------------------------------------------------------------------------------

  {
    double rx = img->i.xRes, ry = img->i.yRes;  
    bim::uint16 units = img->i.resUnits;
    
    if ( (img->i.xRes == 0) && (img->i.yRes == 0) || (img->i.resUnits == 1) ) {
      // Standard resolution some claim to be 72ppi... why not?
      units = RESUNIT_INCH;
      rx = 72.0; 
      ry = 72.0;
    }
    else
    if (img->i.resUnits != 2) {
      if (img->i.resUnits == 0)  { rx = pow(rx, -2); ry = pow(ry, -2); }
      if (img->i.resUnits == 4)  { rx = pow(rx, -1); ry = pow(ry, -1); }
      if (img->i.resUnits == 5)  { rx = pow(rx, -4); ry = pow(ry, -4); }
      if (img->i.resUnits == 6)  { rx = pow(rx, -7); ry = pow(ry, -7); }
      if (img->i.resUnits == 7)  { rx = pow(rx, 11); ry = pow(ry, 11); }
      if (img->i.resUnits == 8)  { rx = pow(rx, 8); ry = pow(ry, 8); }
      if (img->i.resUnits == 9)  { rx = pow(rx, 5); ry = pow(ry, 5); }
      if (img->i.resUnits == 10) { rx = pow(rx, 0); ry = pow(ry, 0); }
    }

    TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, units);
    TIFFSetField(out, TIFFTAG_XRESOLUTION, rx);
    TIFFSetField(out, TIFFTAG_YRESOLUTION, ry);
  }
 
  //------------------------------------------------------------------------------  
  // palettes (image colormaps are automatically scaled to 16-bits)
  //------------------------------------------------------------------------------ 
  bim::uint16 palr[256], palg[256], palb[256];
  if ( (photometric == PHOTOMETRIC_PALETTE) && (img->i.lut.count > 0) ) {
    bim::uint16 nColors = img->i.lut.count;
    for (int i=0; i<nColors; i++) {
      palr[i] = (bim::uint16) xR( img->i.lut.rgba[i] ) * 256;
      palg[i] = (bim::uint16) xG( img->i.lut.rgba[i] ) * 256;
      palb[i] = (bim::uint16) xB( img->i.lut.rgba[i] ) * 256;
    }
    TIFFSetField(out, TIFFTAG_COLORMAP, palr, palg, palb);
  }


  //------------------------------------------------------------------------------
  // writing meta data
  //------------------------------------------------------------------------------

  write_tiff_metadata (fmtHndl, tifParams);
  //TIFFFlush(out); // error in doing this, due to additional checks to the libtiff 4.0.0


  //------------------------------------------------------------------------------
  // writing image
  //------------------------------------------------------------------------------

  // if separate palnes or only one sample
  if ( (planarConfig == PLANARCONFIG_SEPARATE) || (samplesperpixel == 1) ) {
    bim::uint sample;
    bim::uint line_size = getLineSizeInBytes( img );

    for (sample=0; sample<img->i.samples; sample++) {
      uchar *bits = (uchar *) img->bits[sample];
      for (bim::uint32 y = 0; y <height; y++) {
        xprogress( fmtHndl, y*(sample+1), height*img->i.samples, "Writing TIFF" );
        if ( xtestAbort( fmtHndl ) == 1) break;  

        TIFFWriteScanline(out, bits, y, sample);
        bits += line_size;
      } // for y
    } // for samples

  } // if separate planes
  else
  { // if xRGB image
    bim::uint Bpp = (unsigned int) ceil( ((double) bitspersample) / 8.0 );
    uchar *buffer = (uchar *) _TIFFmalloc(width * 3 * Bpp);
    register bim::uint x, y;
    uchar *black_line = (uchar *) _TIFFmalloc(width * Bpp);
    memset( black_line, 0, width * Bpp );
    
    for (y = 0; y < height; y++) {
      xprogress( fmtHndl, y, height, "Writing TIFF" );
      if ( xtestAbort( fmtHndl ) == 1) break;  

      uchar *bufIn0 = ((uchar *) img->bits[0]) + y*width*Bpp;
      uchar *bufIn1 = ((uchar *) img->bits[1]) + y*width*Bpp;
      uchar *bufIn2 = NULL;
      if (samplesperpixel > 2)
        bufIn2 = ((uchar *) img->bits[2]) + y*width*Bpp;
      else
        bufIn2 = black_line;

      if (img->i.depth <= 8) { // 8 bits
        uchar *p  = (uchar *) buffer;
        
        for (x=0; x<width; x++) {
          p[0] = *(bufIn0 + x);
          p[1] = *(bufIn1 + x);
          p[2] = *(bufIn2 + x);
          p += 3;
        }

      } // if 8 bit
      else  { // 16 bits
        bim::uint16 *p  = (bim::uint16 *) buffer;
        bim::uint16 *p0 = (bim::uint16 *) bufIn0;   
        bim::uint16 *p1 = (bim::uint16 *) bufIn1; 
        bim::uint16 *p2 = (bim::uint16 *) bufIn2;
      
        for (x=0; x<width; x++) {
          p[0] = *(p0 + x);
          p[1] = *(p1 + x);
          p[2] = *(p2 + x);
          p += 3;
        }
      } // if 16 bit

      // write the scanline to disc
      TIFFWriteScanline(out, buffer, y, 0);
    }

    _TIFFfree(buffer);
    _TIFFfree(black_line);
  }

  TIFFWriteDirectory( out );
  TIFFFlushData(out);
  TIFFFlush(out);

  return 0;
}

