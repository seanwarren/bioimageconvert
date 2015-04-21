/*****************************************************************************
  TIFF STK IO 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  TODO:
    At this point it's not supporting more then one sample per pixel...
    Check compression related to stripes, we might change it later

  History:
    03/29/2004 22:23 - First creation
    09/28/2005 23:10 - fixed bugs for bigendian architecture
        
  Ver : 2
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>

#include <string>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

//#include "bim_stk_format.h"

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"
#include "memio.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 


#define HOURS_PER_DAY 24
#define MINS_PER_HOUR 60
#define SECS_PER_MIN  60
#define MSECS_PER_SEC 1000
#define MINS_PER_DAY  (HOURS_PER_DAY * MINS_PER_HOUR)
#define SECS_PER_DAY  (MINS_PER_DAY * SECS_PER_MIN)
#define MSECS_PER_DAY (SECS_PER_DAY * MSECS_PER_SEC)

using namespace bim;

void invertSample(ImageBitmap *img, const int &sample);

//----------------------------------------------------------------------------
// misc
//----------------------------------------------------------------------------
void initRational(StkRational *r, bim::int32 N) {
  if (r == NULL) return;
  for (int i=0; i<N; i++) {
    r[i].num = 0;
    r[i].den = 1;
  }
}

void initLong(bim::int32 *r, bim::int32 N) {
  if (r == NULL) return;
  for (int i=0; i<N; i++) r[i] = 0;
}

//----------------------------------------------------------------------------
// housekeeping
//----------------------------------------------------------------------------
StkInfo::StkInfo() {
  init();
}

StkInfo::~StkInfo() {
  clear();
}

void StkInfo::init() {
  this->strips_per_plane = 0;
  this->strip_offsets = NULL;
  this->strip_bytecounts = NULL;

  this->metaData.CalibrationUnits  = NULL;
  this->metaData.Name              = NULL;
  this->metaData.grayUnitName      = NULL;
  this->metaData.StagePositionX    = NULL;
  this->metaData.StagePositionY    = NULL;
  this->metaData.CameraChipOffsetX = NULL;
  this->metaData.CameraChipOffsetY = NULL;
  this->metaData.StageLabel        = NULL;
  this->metaData.AbsoluteZ         = NULL;
  this->metaData.AbsoluteZValid    = NULL;
  this->metaData.wavelength        = NULL;
  this->metaData.zDistance         = NULL;
  this->metaData.creationDate      = NULL;
  this->metaData.creationTime      = NULL;
  this->metaData.modificationDate  = NULL;
  this->metaData.modificationTime  = NULL;

  this->metaData.N                      = 0;
  this->metaData.AutoScale              = 0;
  this->metaData.MinScale               = 0;
  this->metaData.MaxScale               = 0;
  this->metaData.SpatialCalibration     = 0;
  this->metaData.XCalibration[0]        = 0;
  this->metaData.XCalibration[1]        = 1;
  this->metaData.YCalibration[0]        = 0;
  this->metaData.YCalibration[1]        = 1;
  this->metaData.ThreshState            = 0;
  this->metaData.ThreshStateRed         = 0;
  this->metaData.ThreshStateGreen       = 0;
  this->metaData.ThreshStateBlue        = 0;
  this->metaData.ThreshStateLo          = 0;
  this->metaData.ThreshStateHi          = 0;
  this->metaData.Zoom                   = 0;
  this->metaData.CreateTime[0]          = 0;
  this->metaData.CreateTime[1]          = 1;
  this->metaData.LastSavedTime[0]       = 0;
  this->metaData.LastSavedTime[1]       = 1;
  this->metaData.currentBuffer          = 0;
  this->metaData.grayFit                = 0;
  this->metaData.grayPointCount         = 0;
  this->metaData.grayX[0]               = 0;
  this->metaData.grayX[1]               = 1;
  this->metaData.grayY[0]               = 0;
  this->metaData.grayY[1]               = 1;
  this->metaData.grayMin[0]             = 0;
  this->metaData.grayMin[1]             = 1;
  this->metaData.grayMax[0]             = 0;
  this->metaData.grayMax[1]             = 1;
  this->metaData.StandardLUT            = 0;
  this->metaData.UIC1wavelength         = 0;
  this->metaData.OverlayMask            = 0;
  this->metaData.OverlayCompress        = 0;
  this->metaData.Overlay                = 0;
  this->metaData.SpecialOverlayMask     = 0;
  this->metaData.SpecialOverlayCompress = 0;
  this->metaData.SpecialOverlay         = 0;
  this->metaData.ImageProperty          = 0;
  this->metaData.AutoScaleLoInfo[0]     = 0;
  this->metaData.AutoScaleLoInfo[1]     = 1;
  this->metaData.AutoScaleHiInfo[0]     = 0;
  this->metaData.AutoScaleHiInfo[1]     = 1;
  this->metaData.Gamma                  = 0;
  this->metaData.GammaRed               = 0;
  this->metaData.GammaGreen             = 0;
  this->metaData.GammaBlue              = 0;
  this->metaData.CameraBin[0]           = 0;
  this->metaData.CameraBin[1]           = 0;
  this->metaData.NewLUT                 = 0;
/*
  this->metaData.ImagePropertyEx        = 0;
  this->metaData.UserLutTable           = 0;
  this->metaData.RedAutoScaleInfo       = 0;
  this->metaData.RedAutoScaleLoInfo     = 0;
  this->metaData.RedAutoScaleHiInfo     = 0;
  this->metaData.RedMinScaleInfo        = 0;
  this->metaData.RedMaxScaleInfo        = 0;
  this->metaData.GreenAutoScaleInfo     = 0;
  this->metaData.GreenAutoScaleLoInfo   = 0;
  this->metaData.GreenAutoScaleHiInfo   = 0;
  this->metaData.GreenMinScaleInfo      = 0;
  this->metaData.GreenMaxScaleInfo      = 0;
  this->metaData.BlueAutoScaleInfo      = 0;
  this->metaData.BlueAutoScaleLoInfo    = 0;
  this->metaData.BlueAutoScaleHiInfo    = 0;
  this->metaData.BlueMinScaleInfo       = 0;
  this->metaData.BlueMaxScaleInfo       = 0;
  this->metaData.OverlayPlaneColor      = 0;
*/
}

void StkInfo::clearOffsets() {
  if ( (this->strip_offsets != NULL) && (this->strips_per_plane > 0) )
    _TIFFfree(this->strip_offsets);

  if ( (this->strip_bytecounts != NULL) && (this->strips_per_plane > 0) )
    _TIFFfree(this->strip_bytecounts);

  this->strip_offsets    = NULL;
  this->strip_bytecounts = NULL;
  this->strips_per_plane = 0;
}

void StkInfo::clear() {
  clearOffsets();
 
  if (  this->metaData.CalibrationUnits != NULL )
    _TIFFfree(this->metaData.CalibrationUnits);

  if (  this->metaData.Name != NULL )
    _TIFFfree(this->metaData.Name);

  if (  this->metaData.grayUnitName != NULL )
    _TIFFfree(this->metaData.grayUnitName);

  if (  this->metaData.StagePositionX != NULL )
    _TIFFfree(this->metaData.StagePositionX);

  if (  this->metaData.StagePositionY != NULL )
    _TIFFfree(this->metaData.StagePositionY);

  if (  this->metaData.CameraChipOffsetX != NULL )
    _TIFFfree(this->metaData.CameraChipOffsetX);

  if (  this->metaData.CameraChipOffsetY != NULL )
    _TIFFfree(this->metaData.CameraChipOffsetY); 

  if (  this->metaData.StageLabel != NULL )  
    _TIFFfree(this->metaData.StageLabel);

  if (  this->metaData.AbsoluteZ != NULL )
    _TIFFfree(this->metaData.AbsoluteZ);

  if (  this->metaData.AbsoluteZValid != NULL )
    _TIFFfree(this->metaData.AbsoluteZValid);

  if (  this->metaData.wavelength != NULL )
    _TIFFfree(this->metaData.wavelength);

  if (  this->metaData.zDistance != NULL )
    _TIFFfree(this->metaData.zDistance);

  if (  this->metaData.creationDate != NULL )
    _TIFFfree(this->metaData.creationDate);

  if (  this->metaData.creationTime != NULL )
    _TIFFfree(this->metaData.creationTime);

  if (  this->metaData.modificationDate != NULL )
    _TIFFfree(this->metaData.modificationDate);

  if (  this->metaData.modificationTime != NULL )
    _TIFFfree(this->metaData.modificationTime);

  init();
}

void StkInfo::allocMetaInfo( bim::int32 N ) {
  if (N <= 0) return;

  this->metaData.N = N;

  this->metaData.StagePositionX = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.StagePositionX, N );

  this->metaData.StagePositionY = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.StagePositionY, N );

  this->metaData.CameraChipOffsetX = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.CameraChipOffsetX, N );

  this->metaData.CameraChipOffsetY = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.CameraChipOffsetY, N );

  this->metaData.AbsoluteZ = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.AbsoluteZ, N );

  this->metaData.AbsoluteZValid = (bim::int32 *) _TIFFmalloc( N*sizeof(bim::int32) );
  initLong(this->metaData.AbsoluteZValid, N );

  this->metaData.wavelength = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.wavelength, N );

  this->metaData.zDistance = (StkRational *) _TIFFmalloc( N*sizeof(StkRational) );
  initRational(this->metaData.zDistance, N );

  this->metaData.creationDate = (bim::int32 *) _TIFFmalloc( N*sizeof(bim::int32) );
  initLong(this->metaData.creationDate, N );

  this->metaData.creationTime = (bim::int32 *) _TIFFmalloc( N*sizeof(bim::int32) );
  initLong(this->metaData.creationTime, N );

  this->metaData.modificationDate = (bim::int32 *) _TIFFmalloc( N*sizeof(bim::int32) );
  initLong(this->metaData.modificationDate, N );

  this->metaData.modificationTime = (bim::int32 *) _TIFFmalloc( N*sizeof(bim::int32) );
  initLong(this->metaData.modificationTime, N );

  #if defined(DEBUG) || defined(_DEBUG)
  printf("STK: Allocated data for %d pages\n", N);  
  #endif  
}

//----------------------------------------------------------------------------
// STK MISC FUNCTIONS
//----------------------------------------------------------------------------
void JulianToYMD(bim::uint32 julian, unsigned short& year, unsigned char& month, unsigned char& day) {
  year=0; month=0; day=0;
  if (julian == 0) return;

  bim::int32  a, b, c, d, e, alpha;
  bim::int32  z = julian + 1;

  // dealing with Gregorian calendar reform
  if (z < 2299161L) {
    a = z;
  } else {
    alpha = (bim::int32) ((z - 1867216.25) / 36524.25);
    a = z + 1 + alpha - alpha / 4;
  }

  b = ( a > 1721423L ? a + 1524 : a + 1158 );
  c = (bim::int32) ((b - 122.1) / 365.25);
  d = (bim::int32) (365.25 * c);
  e = (bim::int32) ((b - d) / 30.6001);

  day   = (unsigned char) (b - d - (bim::int32)(30.6001 * e));
  month = (unsigned char) ((e < 13.5) ? e - 1 : e - 13) ; // -1 dima: sub 1 to match Date/Time TIFF tag, dima: old fix, not needed for new
  year  = (short)         ((month > 2.5 ) ? (c - 4716) : c - 4715);
}

bim::uint32 YMDToJulian(unsigned short year, unsigned char month, unsigned char day) {
  short a, b = 0;
  short work_year = year;
  short work_month = month;
  short work_day = day;

  // correct for negative year
  if (work_year < 0) {
    work_year++;
  }

  if (work_month <= 2) {
    work_year--;
    work_month += (short)12;
  }

  // deal with Gregorian calendar
  if (work_year*10000. + work_month*100. + work_day >= 15821015.) {
    a = (short)(work_year/100.);
    b = (short)(2 - a + a/4);
  }

  return  (bim::int32) (365.25*work_year) +
          (bim::int32) (30.6001 * (work_month+1)) +
          work_day + 1720994L + b;
}

void divMod(int Dividend, int Divisor, int &Result, int &Remainder)
{
  Result    = (int) ( ((double) Dividend) / ((double) Divisor) );
  Remainder = Dividend % Divisor;
}

// this convert miliseconds since midnight into hour/minute/second
void MiliMidnightToHMS(bim::uint32 ms, unsigned char& hour, unsigned char& minute, unsigned char& second) {
  hour=0; minute=0; second=0;  
  if (ms == 0) return;
  int h, m, s, fms, mc, msc;
  divMod(ms,  SECS_PER_MIN * MSECS_PER_SEC, mc, msc);
  divMod(mc,  MINS_PER_HOUR, h, m);
  divMod(msc, MSECS_PER_SEC, s, fms);
  hour = h; minute = m; second = s;
}

// String versions
std::string JulianToAnsi(bim::uint32 julian) {
  unsigned short y=0;
  unsigned char m=0, d=0;
  JulianToYMD(julian, y, m, d);
  return xstring::xprintf( "%.4d-%.2d-%.2d", y, m, d );
}

std::string MiliMidnightToAnsi(bim::uint32 ms) {
  unsigned char h=0, mi=0, s=0;
  MiliMidnightToHMS(ms, h, mi, s);
  return xstring::xprintf( "%.2d:%.2d:%.2d", h, mi, s );
}

std::string StkDateTimeToAnsi(bim::uint32 julian, bim::uint32 ms) {
  std::string s = JulianToAnsi(julian);
  s += " ";
  s += MiliMidnightToAnsi(ms);
  return s;
}

//----------------------------------------------------------------------------
// STK UTIL FUNCTIONS
//----------------------------------------------------------------------------

bool stkAreValidParams(TiffParams *tiffParams) {
  if (tiffParams == NULL) return false;
  if (tiffParams->tiff == NULL) return false;
  return true;
}

bim::int32 stkGetNumPlanes(TIFF *tif) {
  double     *d_list = NULL;
  bim::int32 *l_list = NULL;
  bim::int16  d_list_count[4];
  int res[4] = {0,0,0,0};

  if (tif == 0) return 0;

  res[0] = TIFFGetField(tif, TIFFTAG_STK_UIC2, &d_list_count[0], &d_list);
  res[1] = TIFFGetField(tif, TIFFTAG_STK_UIC3, &d_list_count[1], &d_list);
  res[2] = TIFFGetField(tif, TIFFTAG_STK_UIC4, &d_list_count[2], &l_list);
  // Mario Emmenlauer, 2015.03.25:
  // The value of the UIC1 tag was not reliable for some images, ignored:
  //res[3] = TIFFGetField(tif, TIFFTAG_STK_UIC1, &d_list_count[3], &l_list);

  // if tag 33629 exists then the file is valid STK file
  if (res[0] == 1) return d_list_count[0];
  if (res[1] == 1) return d_list_count[1];
  if (res[2] == 1) return d_list_count[2];
  if (res[3] == 1) return d_list_count[3];

  return 1;
}

bool stkIsTiffValid(TIFF *tif) {
  if (tif->tif_flags&TIFF_BIGTIFF) return false;    
  double *d_list = NULL;
  bim::int32 *l_list = NULL;
  bim::int16   d_list_count;
  int res[4] = {0,0,0,0};

  if (tif == 0) return false;

  res[0] = TIFFGetField(tif, TIFFTAG_STK_UIC2, &d_list_count, &d_list);
  res[1] = TIFFGetField(tif, TIFFTAG_STK_UIC3, &d_list_count, &d_list);
  res[2] = TIFFGetField(tif, TIFFTAG_STK_UIC4, &d_list_count, &l_list);
  res[3] = TIFFGetField(tif, TIFFTAG_STK_UIC1, &d_list_count, &d_list);

  // if tag 33629 exists then the file is valid STAK file
  if (res[0] == 1) return true;
  if (res[1] == 1) return true;
  if (res[2] == 1) return true;
  if (res[3] == 1) return true;

  return false;
}

bool stkIsTiffValid(TiffParams *tiffParams) {
  if (tiffParams == NULL) return false;
  if (tiffParams->tiff->tif_flags&TIFF_BIGTIFF) return false;

  // if tag 33629 exists then the file is valid STAK file
  if (tiffParams->ifds.tagPresentInFirstIFD(TIFFTAG_STK_UIC1)) return true;
  if (tiffParams->ifds.tagPresentInFirstIFD(TIFFTAG_STK_UIC2)) return true;
  if (tiffParams->ifds.tagPresentInFirstIFD(TIFFTAG_STK_UIC3)) return true;
  if (tiffParams->ifds.tagPresentInFirstIFD(TIFFTAG_STK_UIC4)) return true;

  return false;
}

template<typename Tb, typename To>
void copy_clean_tag_buffer( Tb **buf, bim::uint64 size_in_bytes, To **out ) {
  if (!*buf) return;
  bim::uint64 n = size_in_bytes / sizeof(Tb);
  bim::uint64 out_size_in_bytes = n * sizeof(To);
  *out = (To *) _TIFFmalloc( out_size_in_bytes );

  if (size_in_bytes == out_size_in_bytes)
    _TIFFmemcpy( *out, *buf, size_in_bytes );
  else
    // slow conversion
    for (unsigned int i=0; i<n; ++i)
      (*out)[i] = (*buf)[i];

  _TIFFfree( *buf );
}

void stkGetOffsets(TIFF *tif, TiffParams *tiffParams ) {
  /*
  the default libtiff strip reading is not good enough since it only gives us strip offsets for 
  the first plane although all the offsets for all the planes are actually stored
  so for uncompressed images use default and for LZW the custom methods
  */
  
  StkInfo *stkInfo = &tiffParams->stkInfo;
  stkInfo->strips_per_plane = TIFFNumberOfStrips( tif );

  if (stkInfo->strip_offsets != NULL) _TIFFfree(stkInfo->strip_offsets);
  stkInfo->strip_offsets = NULL;
  if (stkInfo->strip_bytecounts != NULL) _TIFFfree(stkInfo->strip_bytecounts);
  stkInfo->strip_bytecounts = NULL;

  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;

  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  int N = tiffParams->stkInfo.metaData.N;

  // -----------------------------
  // read Strip Offsets
  // -----------------------------
  bim::uint16 tag = 273;
  if (ifd->tagPresent(tag)) {
    bim::uint32 *buf=NULL; bim::uint64 size=0; bim::uint16 type=0;
    ifd->readTag(tag, size, type, (uchar **) &buf );
    copy_clean_tag_buffer<bim::uint32, tiff_offs_t>( &buf, size, &stkInfo->strip_offsets );

  } // strip offsets
  
  // -----------------------------
  // read strip_bytecounts also
  // -----------------------------
  tag = 279;
  if (ifd->tagPresent(tag)) {
    bim::uint32 *buf=NULL; bim::uint64 size=0; bim::uint16 type=0;
    ifd->readTag(tag, size, type, (uchar **) &buf );
    copy_clean_tag_buffer<bim::uint32, tiff_bcnt_t>( &buf, size, &stkInfo->strip_bytecounts );
  } // strip_bytecounts
}


//----------------------------------------------------------------------------
// PARSE UIC TAGS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// UIC2Tag - 33629  
//----------------------------------------------------------------------------
//At the indicated offset, there is a table of 6*N LONG values 
//indicating for each plane in order: 
//Z distance (numerator), Z distance (denominator), 
//creation date, creation time, modification date, modification time.
void stkParseUIC2Tag( TiffParams *tiffParams )
{
  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;

  StkInfo *stkInfo = &tiffParams->stkInfo;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  // read UIC2
  bim::uint16 tag = 33629;
  if (!ifd->tagPresent(tag)) return;

  bim::int32 i;
  bim::int32 N = stkInfo->metaData.N;
  bim::uint32 *buf = NULL;
  ifd->readTagCustom (tag, 6*N*sizeof(bim::int32), TAG_LONG, (uchar **) &buf);
  if (buf == NULL) return;

  for (i=0; i<N; i++) {
    stkInfo->metaData.zDistance[i].num    = buf[i*6+0];
    stkInfo->metaData.zDistance[i].den    = buf[i*6+1];
    stkInfo->metaData.creationDate[i]     = buf[i*6+2];
    stkInfo->metaData.creationTime[i]     = buf[i*6+3];
    stkInfo->metaData.modificationDate[i] = buf[i*6+4];
    stkInfo->metaData.modificationTime[i] = buf[i*6+5];
  }

  _TIFFfree(buf);
}

//----------------------------------------------------------------------------
// UIC3Tag - 33630 
//----------------------------------------------------------------------------
//A table of 2*N LONG values indicating for each plane in order: 
//wavelength (numerator), wavelength (denominator).
void stkParseUIC3Tag( TiffParams *tiffParams )
{
  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;

  StkInfo *stkInfo = &tiffParams->stkInfo;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  // read UIC3
  bim::uint16 tag = 33630;
  if (!ifd->tagPresent(tag)) return;
  bim::int32 i=0;
  bim::int32 N = tiffParams->stkInfo.metaData.N;
  bim::uint32 *buf = NULL;
 
  ifd->readTagCustom (tag, 2*N*sizeof(bim::uint32), TAG_LONG, (uchar **) &buf);
  if (buf == NULL) return;

  for (i=0; i<N; i++)
  {
    stkInfo->metaData.wavelength[i].num = buf[i*2];
    stkInfo->metaData.wavelength[i].den = buf[i*2+1];
  }

  _TIFFfree(buf);
}

//----------------------------------------------------------------------------
// UIC4Tag - 33631 (TIFFTAG_STK_UIC4)
//----------------------------------------------------------------------------
/*
UIC4Tag
Tag	= 33631 (835F.H)
Type	= LONG
N	= Number of planes

At the indicated offset, there is a series of pairs consisting of an ID code 
of size SHORT and a variable-sized (ID-dependent) block of data. Pairs should 
be read until an ID of 0 is encountered. The possible tags and their values 
are defined in the “Tag ID Codes” section below. The “AutoScale” tag never 
appears in this table (because its ID is used to terminate the table.)
*/
void stkParseUIC4Tag( TiffParams *tiffParams ) {
  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;

  StkInfo *stkInfo = &tiffParams->stkInfo;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  // read UIC4
  bim::uint16 tag = TIFFTAG_STK_UIC4;
  if (!ifd->tagPresent(tag)) return;

  bim::int32 N = stkInfo->metaData.N;
  short tag_id;
  std::vector<unsigned char> buffer;
  bim::int64 data_offset = ifd->tagOffset(tag);

  while (data_offset != 0) {
    // first read key for a tuple
    if (ifd->readBufNoAlloc(data_offset, 2, TIFF_SHORT, (bim::uint8 *) &tag_id) != 0) break;
    //safeguard, currently i only know of keys <= 45, stop otherwise
    if (tag_id == 0 || tag_id > BIM_STK_CameraChipOffset) break;
    data_offset += 2;

    if (tag_id == BIM_STK_StagePosition) {
      int buf_size = N*4*sizeof(bim::uint32);
      if (buffer.size() < buf_size) buffer.resize(buf_size);
      bim::int32 *buf = (bim::int32 *) &buffer[0];
      if (ifd->readBufNoAlloc(data_offset, buf_size, TIFF_LONG, (unsigned char *) buf) != 0) break;
      data_offset += buf_size;
      for (int i=0; i<N; ++i) {
        stkInfo->metaData.StagePositionX[i].num = buf[i*4+0];
        stkInfo->metaData.StagePositionX[i].den = buf[i*4+1];
        stkInfo->metaData.StagePositionY[i].num = buf[i*4+2];
        stkInfo->metaData.StagePositionY[i].den = buf[i*4+3];
      }
      continue;
    } // tag BIM_STK_StagePosition

    if (tag_id == BIM_STK_CameraChipOffset) {
      int buf_size = N*4*sizeof(bim::uint32);
      if (buffer.size() < buf_size) buffer.resize(buf_size);
      bim::int32 *buf = (bim::int32 *) &buffer[0];
      if (ifd->readBufNoAlloc(data_offset, buf_size, TIFF_LONG, (unsigned char *) buf ) != 0) break;
      data_offset += buf_size;
      for (int i=0; i<N; ++i) {
        stkInfo->metaData.CameraChipOffsetX[i].num = buf[i*4+0];
        stkInfo->metaData.CameraChipOffsetX[i].den = buf[i*4+1];
        stkInfo->metaData.CameraChipOffsetY[i].num = buf[i*4+2];
        stkInfo->metaData.CameraChipOffsetY[i].den = buf[i*4+3];
      }
      continue;
    } // tag BIM_STK_CameraChipOffset
  } // while
}

//----------------------------------------------------------------------------
// UIC1Tag - 33628
//----------------------------------------------------------------------------

//If this tag exists and is of type LONG, then at the indicated offset 
//there is a series of N pairs consisting of an ID code of size LONG and a 
//variable-sized (ID-dependent) block of data. The possible tags and their 
//values are defined in the "Tag ID Codes" section below. These values are 
//used internally by the Meta Imaging Series applications, and may not be 
//useful to other applications. To replicate the behavior of MetaMorph, 
//this table should be read and its values stored after the table 
//indicated by UIC4Tag is read.
// NOTE: difference form tag UIC4 is that this one contains exactly N pairs
// and long organization of data... We're not reading tag UIC4 at all at the moment

void stkReadStringEntry(bim::int32 offset, TiffParams *tiffParams, char **string) {
  if (offset <= 0) return;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  bim::int32 size;
  ifd->readBufNoAlloc( (toff_t) offset, (bim::uint64) sizeof( bim::int32 ), TAG_LONG, (uchar *) &size);
  *string = (char *) _TIFFmalloc( size+1 );
  (*string)[size] = '\0';
  ifd->readBufNoAlloc( (toff_t) offset+sizeof( bim::int32 ), (bim::uint64)  size, TAG_LONG, (uchar *) (*string));
}

void stkParseIDEntry(bim::int32 *pair, bim::int32 offset, TiffParams *tiffParams) {
  StkInfo *stkInfo = &tiffParams->stkInfo;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  if (pair[0] == BIM_STK_AutoScale)
    stkInfo->metaData.AutoScale = pair[1];

  if (pair[0] == BIM_STK_MinScale)
    stkInfo->metaData.MinScale = pair[1];

  if (pair[0] == BIM_STK_MaxScale)
    stkInfo->metaData.MaxScale = pair[1];

  if (pair[0] == BIM_STK_SpatialCalibration)
    stkInfo->metaData.SpatialCalibration = pair[1];

  if (pair[0] == BIM_STK_XCalibration)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.XCalibration);

  if (pair[0] == BIM_STK_YCalibration)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.YCalibration);

  if (pair[0] == BIM_STK_CalibrationUnits)
    stkReadStringEntry(pair[1], tiffParams, (char **) &stkInfo->metaData.CalibrationUnits);

  if (pair[0] == BIM_STK_Name)
    stkReadStringEntry(pair[1], tiffParams, (char **) &stkInfo->metaData.Name);

  if (pair[0] == BIM_STK_ThreshState)
    stkInfo->metaData.ThreshState = pair[1];

  if (pair[0] == BIM_STK_ThreshStateRed)
    stkInfo->metaData.ThreshStateRed = pair[1];

  if (pair[0] == BIM_STK_ThreshStateGreen)
    stkInfo->metaData.ThreshStateGreen = pair[1];

  if (pair[0] == BIM_STK_ThreshStateBlue)
    stkInfo->metaData.ThreshStateBlue = pair[1];

  if (pair[0] == BIM_STK_ThreshStateLo)
    stkInfo->metaData.ThreshStateLo = pair[1];

  if (pair[0] == BIM_STK_ThreshStateHi)
    stkInfo->metaData.ThreshStateHi = pair[1];

  if (pair[0] == BIM_STK_Zoom)
    stkInfo->metaData.Zoom = pair[1];

  if (pair[0] == BIM_STK_CreateTime)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.CreateTime);

  if (pair[0] == BIM_STK_LastSavedTime)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.LastSavedTime);

  if (pair[0] == BIM_STK_currentBuffer)
    stkInfo->metaData.currentBuffer = pair[1];

  if (pair[0] == BIM_STK_grayFit)
    stkInfo->metaData.grayFit = pair[1];

  if (pair[0] == BIM_STK_grayPointCount)
    stkInfo->metaData.grayPointCount = pair[1];

  if (pair[0] == BIM_STK_grayX)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.grayX);

  if (pair[0] == BIM_STK_grayY)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.grayY);

  if (pair[0] == BIM_STK_grayMin)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.grayMin);

  if (pair[0] == BIM_STK_grayMax)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.grayMax);

  if (pair[0] == BIM_STK_grayUnitName)
    stkReadStringEntry(pair[1], tiffParams, (char **) &stkInfo->metaData.grayUnitName);

  if (pair[0] == BIM_STK_StandardLUT)
    stkInfo->metaData.StandardLUT = pair[1];

  if (pair[0] == BIM_STK_wavelength)
    stkInfo->metaData.UIC1wavelength = pair[1];

  if (pair[0] == BIM_STK_AutoScaleLoInfo)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.AutoScaleLoInfo);

  if (pair[0] == BIM_STK_AutoScaleHiInfo)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.AutoScaleHiInfo);

  if (pair[0] == BIM_STK_Gamma)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) sizeof( bim::int32 ), TAG_LONG, (uchar *) &stkInfo->metaData.Gamma);

  if (pair[0] == BIM_STK_GammaRed)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) sizeof( bim::int32 ), TAG_LONG, (uchar *) &stkInfo->metaData.GammaRed);

  if (pair[0] == BIM_STK_GammaGreen)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) sizeof( bim::int32 ), TAG_LONG, (uchar *) &stkInfo->metaData.GammaGreen);

  if (pair[0] == BIM_STK_GammaBlue)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) sizeof( bim::int32 ), TAG_LONG, (uchar *) &stkInfo->metaData.GammaBlue);

  if (pair[0] == BIM_STK_CameraBin)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) &stkInfo->metaData.CameraBin);

  if (pair[0] == BIM_STK_NewLUT)
    stkInfo->metaData.NewLUT = pair[1];

  if (pair[0] == BIM_STK_PlaneProperty) {
      toff_t readpos = pair[1];

      bim::int8 prop_key_str_len = 0;
      readpos += 4;
      ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) sizeof(bim::int8), TAG_BYTE, (uchar *)&prop_key_str_len);

      char* prop_key_str = new char[(size_t)(prop_key_str_len + 1)];
      readpos += 1;
      ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) prop_key_str_len, TAG_BYTE, (uchar *)prop_key_str);
      prop_key_str[(size_t)prop_key_str_len] = '\0';

      #ifdef DEBUG
      std::cerr << "property key is '" << prop_key_str << "'" << std::endl;
      #endif

      bim::int8 prop_key_type = 0;
      readpos += prop_key_str_len + 4;
      ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) sizeof(bim::int8), TAG_BYTE, (uchar *)&prop_key_type);
      #ifdef DEBUG
      std::cerr << "property type is '" << (int)prop_key_type << "'." << std::endl;
      #endif

      readpos += 1;
      if (prop_key_type == 1) {
          StkRational rational;
          ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) 2 * sizeof(bim::int32), TAG_BYTE, (uchar *)&rational);

          StkPlaneProperty PlaneProperty;
          PlaneProperty.type = prop_key_type;
          PlaneProperty.Key = prop_key_str;
          PlaneProperty.rational = rational;
          stkInfo->metaData.PlaneProperties.push_back(PlaneProperty);
      }
      else if (prop_key_type == 2) {
          bim::int8 prop_val_str_len = 0;
          ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) sizeof(bim::int8), TAG_BYTE, (uchar *)&prop_val_str_len);

          if (prop_val_str_len == 0) {
              readpos += 4;
              ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) sizeof(bim::int8), TAG_BYTE, (uchar *)&prop_val_str_len);
          }

          char* prop_val_str = new char[(size_t)(prop_val_str_len + 1)];
          readpos += 1;
          ifd->readBufNoAlloc((toff_t)readpos, (bim::uint64) prop_val_str_len, TAG_BYTE, (uchar *)prop_val_str);
          prop_val_str[(size_t)prop_val_str_len] = '\0';

          #ifdef DEBUG
          std::cerr << "property value is '" << prop_val_str << "'" << std::endl;
          #endif

          StkPlaneProperty PlaneProperty;
          PlaneProperty.type = prop_key_type;
          PlaneProperty.Key = prop_key_str;
          PlaneProperty.Value = prop_val_str;
          stkInfo->metaData.PlaneProperties.push_back(PlaneProperty);
      }
      else {
          std::cerr << "stkParseIDEntry(): Could not parse property type '" << prop_key_type << "' for property '" << prop_key_str << "'." << std::endl;
      }
  }

/*
  if (pair[0] == BIM_STK_ImagePropertyEx)
    stkInfo->metaData.ImagePropertyEx = pair[1];

  if (pair[0] == BIM_STK_UserLutTable)
    stkInfo->metaData.UserLutTable = pair[1];

  if (pair[0] == BIM_STK_RedAutoScaleInfo)
    stkInfo->metaData.RedAutoScaleInfo = pair[1];

  if (pair[0] == BIM_STK_RedAutoScaleLoInfo)
    stkInfo->metaData.RedAutoScaleLoInfo = pair[1];

  if (pair[0] == BIM_STK_RedAutoScaleHiInfo)
    stkInfo->metaData.RedAutoScaleHiInfo = pair[1];

  if (pair[0] == BIM_STK_RedMinScaleInfo)
    stkInfo->metaData.RedMinScaleInfo = pair[1];

  if (pair[0] == BIM_STK_RedMaxScaleInfo)
    stkInfo->metaData.RedMaxScaleInfo = pair[1];

  if (pair[0] == BIM_STK_GreenAutoScaleInfo)
    stkInfo->metaData.GreenAutoScaleInfo = pair[1];

  if (pair[0] == BIM_STK_GreenAutoScaleLoInfo)
    stkInfo->metaData.GreenAutoScaleLoInfo = pair[1];

  if (pair[0] == BIM_STK_GreenAutoScaleHiInfo)
    stkInfo->metaData.GreenAutoScaleHiInfo = pair[1];

  if (pair[0] == BIM_STK_GreenMinScaleInfo)
    stkInfo->metaData.GreenMinScaleInfo = pair[1];

  if (pair[0] == BIM_STK_GreenMaxScaleInfo)
    stkInfo->metaData.GreenMaxScaleInfo = pair[1];

  if (pair[0] == BIM_STK_BlueAutoScaleInfo)
    stkInfo->metaData.BlueAutoScaleInfo = pair[1];

  if (pair[0] == BIM_STK_BlueAutoScaleLoInfo)
    stkInfo->metaData.BlueAutoScaleLoInfo = pair[1];

  if (pair[0] == BIM_STK_BlueAutoScaleHiInfo)
    stkInfo->metaData.BlueAutoScaleHiInfo = pair[1];

  if (pair[0] == BIM_STK_BlueMinScaleInfo)
    stkInfo->metaData.BlueMinScaleInfo = pair[1];

  if (pair[0] == BIM_STK_BlueMaxScaleInfo)
    stkInfo->metaData.BlueMaxScaleInfo = pair[1];

  if (pair[0] == BIM_STK_OverlayPlaneColor)
    stkInfo->metaData.OverlayPlaneColor = pair[1];
*/



  bim::int32 N = stkInfo->metaData.N;


  std::vector<unsigned char> buffer;

  // only the first page value can be red here
  if (pair[0] == BIM_STK_StagePosition) {
    int buf_size = N*4*sizeof(bim::uint32);
    if (buffer.size() < buf_size) buffer.resize(buf_size);
    bim::int32 *buf = (bim::int32 *) &buffer[0];
    if ( ifd->readBufNoAlloc( pair[1], buf_size, TAG_LONG, (unsigned char *) buf ) == 0) {
      for (int i=0; i<N; ++i) {
        stkInfo->metaData.StagePositionX[i].num = buf[i*4+0];
        stkInfo->metaData.StagePositionX[i].den = buf[i*4+1];
        stkInfo->metaData.StagePositionY[i].num = buf[i*4+2];
        stkInfo->metaData.StagePositionY[i].den = buf[i*4+3];
      } // for i
    } // if read tiff buf
  } // BIM_STK_StagePosition

  if (pair[0] == BIM_STK_CameraChipOffset) {
    int buf_size = N*4*sizeof(bim::uint32);
    if (buffer.size() < buf_size) buffer.resize(buf_size);
    bim::int32 *buf = (bim::int32 *) &buffer[0];
    if ( ifd->readBufNoAlloc( pair[1], buf_size, TAG_LONG, (unsigned char *) buf ) == 0) {
      for (int i=0; i<N; ++i) {
        stkInfo->metaData.CameraChipOffsetX[i].num = buf[i*4+0];
        stkInfo->metaData.CameraChipOffsetX[i].den = buf[i*4+1];
        stkInfo->metaData.CameraChipOffsetY[i].num = buf[i*4+2];
        stkInfo->metaData.CameraChipOffsetY[i].den = buf[i*4+3];
      } // for i
    } // if read tiff buf
  } // BIM_STK_CameraChipOffset

  
  if (pair[0] == BIM_STK_AbsoluteZ)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) N*2*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.AbsoluteZ);

  if (pair[0] == BIM_STK_AbsoluteZValid)
    ifd->readBufNoAlloc( (toff_t) pair[1], (bim::uint64) N*sizeof( bim::int32 ), TAG_LONG, (uchar *) stkInfo->metaData.AbsoluteZValid);
}

void stkParseUIC1Tag( TiffParams *tiffParams )
{
  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;

  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return;

  bim::uint16 tag = 33628;
  if (!ifd->tagPresent(tag)) return;
  
  bim::int32 N = tiffParams->stkInfo.metaData.N; 
  bim::int32 pair[2];

  bim::uint64 offset = ifd->tagOffset(tag);
  if (offset == -1) return;
  bim::uint64 num_ids = ifd->tagCount(tag);
  bim::uint64 id_offset = offset;

  // now read and parce ID table
  for (bim::uint64 i=0; i<num_ids; i++) {
   ifd->readBufNoAlloc( (toff_t) id_offset, (bim::uint64) 2*sizeof( bim::int32 ), TAG_LONG, (uchar *) pair);
    stkParseIDEntry(pair, (bim::uint32) id_offset, tiffParams);
    id_offset += 2*sizeof( bim::int32 );
  }

}

void stkParseUICTags( TiffParams *tiffParams ) {
  if (tiffParams == NULL) return;
  if (tiffParams->tiff == NULL) return;
  if (tiffParams->subType != tstStk) return;
  
  stkParseUIC2Tag( tiffParams );
  stkParseUIC3Tag( tiffParams );
  stkParseUIC4Tag( tiffParams );
  stkParseUIC1Tag( tiffParams );
}


//----------------------------------------------------------------------------
// STK INFO
//----------------------------------------------------------------------------

int stkGetInfo ( TiffParams *tiffParams )
{
  if (tiffParams == NULL) return 1;
  if (tiffParams->tiff == NULL) return 1;
  if (!tiffParams->ifds.isValid()) return 1;

  tiffParams->stkInfo.allocMetaInfo( stkGetNumPlanes( tiffParams->tiff ) );
  stkGetOffsets( tiffParams->tiff, tiffParams );
  stkParseUICTags( tiffParams );

  //---------------------------------------------------------------
  // define dims
  //---------------------------------------------------------------
  StkInfo *stk = &tiffParams->stkInfo;
  ImageInfo *info = &tiffParams->info;
  StkMetaData *meta = &stk->metaData;
  
  //info->samples = 1;
  info->number_pages = meta->N;
  info->number_z = 1;
  info->number_t = info->number_pages;

  if (meta->zDistance[0].den!=0 && meta->zDistance[0].num!=0) {
    double v = meta->zDistance[0].num / (double) meta->zDistance[0].den;
    if (v>0) {
      info->number_z = info->number_pages;
      info->number_t = 1;
    }
  }

  if (info->number_z > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_Z;
  }

  if (info->number_t > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_T;
  }

  if ((info->number_z > 1) && (info->number_t > 1)) {
    info->number_dims = 5;
    info->dimensions[3].dim = DIM_Z;        
    info->dimensions[4].dim = DIM_T;
  }

  return 0;
}

//----------------------------------------------------------------------------
// READ/WRITE FUNCTIONS
//----------------------------------------------------------------------------


bim::uint  stkReadPlane(TiffParams *tiffParams, int plane, ImageBitmap *img, FormatHandle *fmtHndl) {
  if (tiffParams == 0) return 1;
  if (img        == 0) return 1;
  if (tiffParams->tiff == 0) return 1;
  bim::uint sample = 0;

  TIFF *tif = tiffParams->tiff;
  bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 compress_tag;
  bim::uint16 bitspersample = 8;
  bim::uint16 samplesperpixel = 1;
  bim::uint32 height = 0; 
  bim::uint32 width = 0;
  toff_t old_pos = tif->tif_curoff;
  bim::uint32 rowsperstrip   = 1;

  (void) TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &compress_tag); 
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);  
  TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip); 


  bim::uint Bpp = (unsigned int) ceil( ((double) img->i.depth) / 8.0 );
  bim::int32 size = (bim::uint32)(img->i.width * img->i.height * Bpp);

  tiff_offs_t file_plane_offset = tiffParams->stkInfo.strip_offsets[0] + (size * plane);

  if (compress_tag != COMPRESSION_NONE) 
    file_plane_offset = tiffParams->stkInfo.strip_offsets[plane * tiffParams->stkInfo.strips_per_plane ]; 



  // position file position into begining of the plane
  if (tif->tif_seekproc((thandle_t) tif->tif_fd, file_plane_offset, SEEK_SET) != 0) {
    
    //---------------------------------------------------
    // load the buffer with decompressed plane data
    //---------------------------------------------------
  
    if (compress_tag == COMPRESSION_NONE) {
      xprogress( fmtHndl, 0, 10, "Reading STK" );
      
      tiff_size_t read_size = tif->tif_readproc((thandle_t) tif->tif_fd, img->bits[sample], size);
      if (read_size != size) return 1;
      // now swap bytes if needed
      tif->tif_postdecode(tif, (tidata_t) img->bits[sample], size); 
    } else {
      int strip_size = width * Bpp * samplesperpixel * rowsperstrip;
      uchar *buf = (uchar *) _TIFFmalloc( strip_size ); 

      // ---------------------------------------------------
      // let's tweak libtiff and change offsets and bytecounts for correct values for this plane
	    TIFFDirectory *td = &tif->tif_dir;

      tiff_offs_t *plane_strip_offsets = tiffParams->stkInfo.strip_offsets + (plane * tiffParams->stkInfo.strips_per_plane);
      _TIFFmemcpy( td->td_stripoffset, plane_strip_offsets, tiffParams->stkInfo.strips_per_plane * sizeof(tiff_offs_t) );

      tiff_bcnt_t *plane_strip_bytecounts = tiffParams->stkInfo.strip_bytecounts + (plane * tiffParams->stkInfo.strips_per_plane);
      _TIFFmemcpy( td->td_stripbytecount, plane_strip_bytecounts, tiffParams->stkInfo.strips_per_plane * sizeof(tiff_bcnt_t) );
      // ---------------------------------------------------

      int strip_num = plane * tiffParams->stkInfo.strips_per_plane;
      tiff_offs_t strip_offset = 0;
      while (strip_offset < tiffParams->stkInfo.strips_per_plane) {
        xprogress( fmtHndl, strip_offset, tiffParams->stkInfo.strips_per_plane, "Reading STK" );
        if ( xtestAbort( fmtHndl ) == 1) break;  

        tiff_size_t read_size = TIFFReadEncodedStrip (tif, strip_offset, (tiff_data_t) buf, (tiff_size_t) strip_size);
        if (read_size == -1) return 1;
        _TIFFmemcpy( ((unsigned char *) img->bits[sample])+strip_offset*strip_size, buf, read_size );
        ++strip_offset;
      }

      _TIFFfree( buf );  
    }

    // invert it if we got negative
    TIFFGetField(tiffParams->tiff, TIFFTAG_PHOTOMETRIC, &photometric);
    if (photometric == PHOTOMETRIC_MINISWHITE)
      invertSample(img, sample);
  }

  tif->tif_seekproc((thandle_t) tif->tif_fd, old_pos, SEEK_SET);

  return 0;
}


//----------------------------------------------------------------------------
// METADATA TEXT FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint stkAddOneTag (FormatHandle *fmtHndl, int tag, const char* str)
{
  uchar *buf = NULL;
  size_t buf_size = strlen(str);
  bim::uint32 buf_type = TAG_ASCII;

  if ( (buf_size == 0) || (str == NULL) ) return 1;
  else
  {
    // now add tag into structure
    TagItem item;

    buf = (unsigned char *) xmalloc(buf_size + 1);
    strncpy((char *) buf, str, buf_size);
    buf[buf_size] = '\0';

    item.tagGroup  = META_STK;
    item.tagId     = tag;
    item.tagType   = buf_type;
    item.tagLength = (bim::uint32) buf_size;
    item.tagData   = buf;

    addMetaTag( &fmtHndl->metaData, item);
  }

  return 0;
}


bim::uint stkReadMetaMeta (FormatHandle *fmtHndl, int group, int tag, int type)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  TiffParams *tiffParams = (TiffParams *) fmtHndl->internalParams;
  StkMetaData *meta = &tiffParams->stkInfo.metaData;
  
  if ( (group != META_STK) && (group != -1) ) return 1;

  // add tag UIC2Tag 33629, some info
  std::string str_uic2 = "";
  char text[1024];

  for (int i=0; i<meta->N; i++) {
    unsigned short y;
    unsigned char m, d;
    unsigned char h, mi, s;

    JulianToYMD(meta->creationDate[i], y, m, d);
    MiliMidnightToHMS(meta->creationTime[i], h, mi, s);

    sprintf(text, "Page %.3d: %.4d-%.2d-%.2d %.2d:%.2d:%.2d\n", i, y, m, d, h, mi, s );
    str_uic2 += text;
  }
 
  if (str_uic2.size() > 0) 
    stkAddOneTag ( fmtHndl, 33629, str_uic2.c_str() );

  return 0;
}

//***********************************************************************************************
// new metadata support
//***********************************************************************************************

std::string stk_readline( const std::string &str, int &pos ) {
  std::string line;
  std::string::const_iterator it = str.begin() + pos;
  while (it<str.end() && *it != 0xA ) {
    if (*it != 0xD) 
      line += *it;
    else
      ++pos;
    ++it;
  }
  pos += (int) line.size();
  if (it<str.end() && *it == 0xA) ++pos;
  return line;
}

template<typename T>
bool isValueRepeated( T *v, unsigned int N ) {
  if (!v) return true;
  if (N<=0) return true;
  T first = v[0];
  for (unsigned int i=1; i<N; ++i)
    if (v[i] != first) return false;
  return true;
}

template<>
bool isValueRepeated( StkRational *v, unsigned int N ) {
  if (!v) return true;
  if (N<=0) return true;
  StkRational first = v[0];
  for (unsigned int i=1; i<N; ++i)
    if (v[i].num != first.num || v[i].den != first.den) return false;
  return true;
}

void appendValues( StkRational *v, unsigned int N, const std::string &tag_name, TagMap *hash ) {
  xstring name, val;
  if ( isValueRepeated(v, N) ) N=1;
  for (bim::uint i=0; i<N; ++i)
    if (v[i].den!=0 && v[i].num!=0) {
      double vv = v[i].num / (double) v[i].den;
      if (N>1)
        name.sprintf( "%s_planes/%d", tag_name.c_str(), i );
      else
        name = tag_name;
      val.sprintf( "%f", vv );
      hash->append_tag( name, val );
      if (i==0) hash->append_tag( tag_name, val ); 
    }
}


bim::uint append_metadata_stk (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  TiffParams *tiffpar = (TiffParams *) fmtHndl->internalParams;
  StkMetaData *stk = &tiffpar->stkInfo.metaData;
  ImageInfo *info = &tiffpar->info;
  TinyTiff::IFD *ifd = tiffpar->ifds.firstIfd();
  if (!ifd) return 1;

  // is this STK?
  if (!ifd->tagPresent(33628)) return 0;

  xstring name, val;

  hash->append_tag( bim::IMAGE_NUM_Z, (const int) info->number_z );
  hash->append_tag( bim::IMAGE_NUM_T, (const int) info->number_t );

  hash->append_tag( bim::IMAGE_LABEL, stk->Name );

  //---------------------------------------------------------------------------
  // Date/Time
  //---------------------------------------------------------------------------
  {
    unsigned short y;
    unsigned char m, d, h, mi, s;
    unsigned int NN = stk->N;
    if ( isValueRepeated<bim::int32>(stk->creationDate, stk->N) && isValueRepeated<bim::int32>(stk->creationTime, stk->N) ) NN=1;
    for (int i=0; i<stk->N; i++) {
      JulianToYMD(stk->creationDate[i], y, m, d);
      MiliMidnightToHMS(stk->creationTime[i], h, mi, s);
      if (NN>1) 
        name.sprintf( bim::PLANE_DATE_TIME_TEMPLATE.c_str(), i );
      else
        name = bim::PLANE_DATE_TIME;
      val.sprintf( "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", y, m, d, h, mi, s );
      hash->append_tag( name, val );
    }
  }

  //---------------------------------------------------------------------------
  // get the pixel size in microns
  //---------------------------------------------------------------------------
  double pixel_size[4] = {0,0,0,0};

  name = stk->CalibrationUnits;
  if (stk->SpatialCalibration>0 && (name == "um" || name == "microns") ) {
    if (stk->XCalibration[1]!=0 && stk->XCalibration[0]!=0) 
      pixel_size[0] = stk->XCalibration[0] / (double) stk->XCalibration[1];
    if (stk->YCalibration[1]!=0 && stk->YCalibration[0]!=0) 
      pixel_size[1] = stk->YCalibration[0] / (double) stk->YCalibration[1];

    hash->append_tag( bim::PIXEL_RESOLUTION_X, pixel_size[0] );
    hash->append_tag( bim::PIXEL_RESOLUTION_Y, pixel_size[1] );
    hash->set_value( bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
    hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  }

  //---------------------------------------------------------------------------
  // add 3'd dimension
  //---------------------------------------------------------------------------
  if (info->number_z > 1) {
    if (stk->zDistance[0].den!=0 && stk->zDistance[0].num!=0)
      pixel_size[2] = stk->zDistance[0].num / (double) stk->zDistance[0].den;
    hash->append_tag( bim::PIXEL_RESOLUTION_Z, pixel_size[2] );
    hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  } else {
    hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS );    
    if (stk->N>=2) {
      unsigned char h1, m1, s1, h2, m2, s2;
      MiliMidnightToHMS(stk->creationTime[0], h1, m1, s1);
      MiliMidnightToHMS(stk->creationTime[1], h2, m2, s2);
      double delta_s = ( fabs((float)h2-h1)*60.0 + fabs((float)m2-m1))*60.0 + fabs((float)s2-s1);
      hash->append_tag( bim::PIXEL_RESOLUTION_T, delta_s );
    }
  }

  //------------------------------------------------------------
  // Long arrays
  //------------------------------------------------------------
  appendValues( stk->StagePositionX, stk->N, bim::STAGE_POSITION_X, hash );
  appendValues( stk->StagePositionY, stk->N, bim::STAGE_POSITION_Y, hash );
  appendValues( stk->zDistance, stk->N, bim::STAGE_DISTANCE_Z, hash );
  appendValues( stk->CameraChipOffsetX, stk->N, bim::CAMERA_SENSOR_X, hash );
  appendValues( stk->CameraChipOffsetY, stk->N, bim::CAMERA_SENSOR_Y, hash );

  if (info->number_z>0 && stk->AbsoluteZValid && stk->AbsoluteZValid[0]) 
    appendValues( stk->AbsoluteZ, stk->N, bim::STAGE_POSITION_Z, hash );

  //------------------------------------------------------------
  // Add tags from structure
  //------------------------------------------------------------

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Name", stk->Name );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"AutoScale", (const int) stk->AutoScale );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"MinScale", (const int) stk->MinScale );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"MaxScale", (const int) stk->MaxScale );

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"SpatialCalibration", (const int) stk->SpatialCalibration );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"XCalibration", xstring::xprintf("%d/%d", stk->XCalibration[0], stk->XCalibration[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"YCalibration", xstring::xprintf("%d/%d", stk->YCalibration[0], stk->YCalibration[1]) );
  if (stk->CalibrationUnits) hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"CalibrationUnits", stk->CalibrationUnits );


  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshState", (const int) stk->ThreshState );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshStateRed", (const unsigned int) stk->ThreshStateRed );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshStateGreen", (const unsigned int) stk->ThreshStateGreen );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshStateBlue", (const unsigned int) stk->ThreshStateBlue );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshStateLo", (const unsigned int) stk->ThreshStateLo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ThreshStateHi", (const unsigned int) stk->ThreshStateHi );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Zoom", (const int) stk->Zoom );

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"CreateTime", StkDateTimeToAnsi(stk->CreateTime[0], stk->CreateTime[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"LastSavedTime", StkDateTimeToAnsi(stk->LastSavedTime[0], stk->LastSavedTime[1]) );
  
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"currentBuffer", (const int) stk->currentBuffer );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayFit", (const int) stk->grayFit );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayPointCount", (const int) stk->grayPointCount );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayX", xstring::xprintf("%d/%d", stk->grayX[0], stk->grayX[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayY", xstring::xprintf("%d/%d", stk->grayY[0], stk->grayY[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayMin", xstring::xprintf("%d/%d", stk->grayMin[0], stk->grayMin[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayMax", xstring::xprintf("%d/%d", stk->grayMax[0], stk->grayMax[1]) );
  if (stk->grayUnitName) hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"grayUnitName", stk->grayUnitName );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"StandardLUT", (const int) stk->StandardLUT );
  if (stk->UIC1wavelength > 0) hash->append_tag(bim::CUSTOM_TAGS_PREFIX + "Wavelength", (const int)stk->UIC1wavelength);

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"AutoScaleLoInfo", xstring::xprintf("%d/%d", stk->AutoScaleLoInfo[0], stk->AutoScaleLoInfo[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"AutoScaleHiInfo", xstring::xprintf("%d/%d", stk->AutoScaleHiInfo[0], stk->AutoScaleHiInfo[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Gamma", (const int) stk->Gamma );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GammaRed", (const int) stk->GammaRed );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GammaGreen", (const int) stk->GammaGreen );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GammaBlue", (const int) stk->GammaBlue );
  

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"CameraBin", xstring::xprintf("%dx%d", stk->CameraBin[0], stk->CameraBin[1]) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"NewLUT", (const int) stk->NewLUT );
/*
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"ImagePropertyEx", (const int) stk->ImagePropertyEx );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"UserLutTable", (const int) stk->UserLutTable );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"RedAutoScaleInfo", (const int) stk->RedAutoScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"RedAutoScaleLoInfo", (const int) stk->RedAutoScaleLoInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"RedAutoScaleHiInfo", (const int) stk->RedAutoScaleHiInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"RedMinScaleInfo", (const int) stk->RedMinScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"RedMaxScaleInfo", (const int) stk->RedMaxScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GreenAutoScaleInfo", (const int) stk->GreenAutoScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GreenAutoScaleLoInfo", (const int) stk->GreenAutoScaleLoInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GreenAutoScaleHiInfo", (const int) stk->GreenAutoScaleHiInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GreenMinScaleInfo", (const int) stk->GreenMinScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"GreenMaxScaleInfo", (const int) stk->GreenMaxScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"BlueAutoScaleInfo", (const int) stk->BlueAutoScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"BlueAutoScaleLoInfo", (const int) stk->BlueAutoScaleLoInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"BlueAutoScaleHiInfo", (const int) stk->BlueAutoScaleHiInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"BlueMinScaleInfo", (const int) stk->BlueMinScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"BlueMaxScaleInfo", (const int) stk->BlueMaxScaleInfo );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"OverlayPlaneColor", (const int) stk->OverlayPlaneColor );
*/

  for (size_t vIdx = 0; vIdx < stk->PlaneProperties.size(); ++vIdx) {
    std::string key = std::string("Channel #0 ") + stk->PlaneProperties[vIdx].Key;
    std::string value;
    if (stk->PlaneProperties[vIdx].type == 1) {
      std::ostringstream tmpstr;
      tmpstr << (double)stk->PlaneProperties[vIdx].rational.num / (double)stk->PlaneProperties[vIdx].rational.den;
      value = tmpstr.str();
    } else {
      value = stk->PlaneProperties[vIdx].Value;
    }
    hash->append_tag(bim::CUSTOM_TAGS_PREFIX + key, value);
  }

  /*
  // begin: Used internally by MetaMorph
  bim::int32 OverlayMask;
  bim::int32 OverlayCompress;
  bim::int32 Overlay;
  bim::int32 SpecialOverlayMask;
  bim::int32 SpecialOverlayCompress;
  bim::int32 SpecialOverlay;
  bim::int32 ImageProperty;
  // end: Used internally by MetaMorph
*/

  //------------------------------------------------------------
  // Add tags from tiff
  //------------------------------------------------------------
  TIFF *tif = tiffpar->tiff;
  xstring tag_305 = ifd->readTagString(TIFFTAG_SOFTWARE);

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Software", tag_305 );
  //xstring tag_306 = read_tag_as_string(tif, ifd, TIFFTAG_DATETIME );

  xstring tag_270 = ifd->readTagString(TIFFTAG_IMAGEDESCRIPTION);
  hash->parse_ini( tag_270, ":", bim::CUSTOM_TAGS_PREFIX );

  int p=0;
  xstring line = stk_readline( tag_270, p );
  if (line.startsWith("Acquired from ")) {
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"AcquiredFrom", line.right("Acquired from ") );
  }


  return 0;
}



