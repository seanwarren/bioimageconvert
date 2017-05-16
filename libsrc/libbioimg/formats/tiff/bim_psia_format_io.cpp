/*****************************************************************************
  TIFF PSIA IO 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23 - First creation
    10/10/2005 16:23 - image allocation fixed
        
  Ver : 2
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"
#include "memio.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

using namespace bim;

void invertImg(ImageBitmap *img);
unsigned int tiffGetNumberOfPages( TiffParams *tiffpar );

//----------------------------------------------------------------------------
// PSIA MISC FUNCTIONS
//----------------------------------------------------------------------------

bool psiaIsTiffValid(TiffParams *tiffParams) {
  if (tiffParams == NULL) return false;
  if (tiffParams->tiff->tif_flags&TIFF_BIGTIFF) return false;  
  if (tiffParams->ifds.tagPresentInFirstIFD(50434)) return true;
  if (tiffParams->ifds.tagPresentInFirstIFD(50435)) return true;
  return false;
}

void wstr2charcpy (char *trg, char *src, unsigned int n) {
  unsigned int i2=0;
  for (unsigned int i=0; i<n; i++) {
    trg[i] = src[i2];
    i2+=2;
  }
}

int psiaGetInfo (TiffParams *tiffParams) {
  if (tiffParams == NULL) return 1;
  if (tiffParams->tiff == NULL) return 1;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return 1;
  psiaInfoHeader *psiaInfo = &tiffParams->psiaInfo;
  tiffParams->info.number_pages = tiffGetNumberOfPages( tiffParams );

  if (!ifd->tagPresent(50435)) return 1;

  std::vector<bim::uint8> bufv;
  ifd->readTag(50435, &bufv);
  if (bufv.size()<=0) return 1;
  bim::uint8 *buf = &bufv[0];

  psiaInfo->dfLPFStrength = * (float64 *) (buf + BIM_PSIA_OFFSET_LPFSSTRENGTH); 
  psiaInfo->bAutoFlatten  = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_AUTOFLATTEN);      
  psiaInfo->bACTrack      = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_ACTRACK);          
  psiaInfo->nWidth        = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_WIDTH);            
  psiaInfo->nHeight       = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_HEIGHT);           
  psiaInfo->dfAngle       = * (float64 *) (buf + BIM_PSIA_OFFSET_ANGLE);
  psiaInfo->bSineScan     = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_SINESCAN);         
  psiaInfo->dfOverScan    = * (float64 *) (buf + BIM_PSIA_OFFSET_OVERSCAN);        
  psiaInfo->bFastScanDir  = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_FASTSCANDIR);      
  psiaInfo->bSlowScanDir  = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_SLOWSCANDIR);      
  psiaInfo->bXYSwap       = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_XYSWAP);           
  psiaInfo->dfXScanSize   = * (float64 *) (buf + BIM_PSIA_OFFSET_XSCANSIZE);       
  psiaInfo->dfYScanSize   = * (float64 *) (buf + BIM_PSIA_OFFSET_YSCANSIZE);
  psiaInfo->dfXOffset     = * (float64 *) (buf + BIM_PSIA_OFFSET_XOFFSET);         
  psiaInfo->dfYOffset     = * (float64 *) (buf + BIM_PSIA_OFFSET_YOFFSET);
  psiaInfo->dfScanRate    = * (float64 *) (buf + BIM_PSIA_OFFSET_SCANRATE);        
  psiaInfo->dfSetPoint    = * (float64 *) (buf + BIM_PSIA_OFFSET_SETPOINT);        
  psiaInfo->dtTipBias     = * (float64 *) (buf + BIM_PSIA_OFFSET_TIPBIAS);         
  psiaInfo->dfSampleBias  = * (float64 *) (buf + BIM_PSIA_OFFSET_SAMPLEBIAS);      
  psiaInfo->dfDataGain    = * (float64 *) (buf + BIM_PSIA_OFFSET_DATAGAIN);        
  psiaInfo->dfZScale      = * (float64 *) (buf + BIM_PSIA_OFFSET_ZSCALE);          
  psiaInfo->dfZOffset     = * (float64 *) (buf + BIM_PSIA_OFFSET_ZOFFSET);         
  psiaInfo->nDataMin      = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_DATAMIN);
  psiaInfo->nDataMax      = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_DATAMAX);
  psiaInfo->nDataAvg      = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_DATAAVG);
  psiaInfo->ncompression  = * (bim::uint32 *)  (buf + BIM_PSIA_OFFSET_NCOMPRESSION);

  // if running the MSB machine (motorola, power pc) then swap
  if (TinyTiff::bigendian) {
    TIFFSwabDouble ( &psiaInfo->dfLPFStrength );  
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bAutoFlatten );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bACTrack );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->nWidth );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->nHeight );
    TIFFSwabDouble ( &psiaInfo->dfAngle );  
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bSineScan );
    TIFFSwabDouble ( &psiaInfo->dfOverScan );  
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bFastScanDir ); 
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bSlowScanDir );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->bXYSwap );  
    TIFFSwabDouble ( &psiaInfo->dfXScanSize ); 
    TIFFSwabDouble ( &psiaInfo->dfYScanSize ); 
    TIFFSwabDouble ( &psiaInfo->dfXOffset ); 
    TIFFSwabDouble ( &psiaInfo->dfYOffset );   
    TIFFSwabDouble ( &psiaInfo->dfScanRate );
    TIFFSwabDouble ( &psiaInfo->dfSetPoint ); 
    TIFFSwabDouble ( &psiaInfo->dtTipBias ); 
    TIFFSwabDouble ( &psiaInfo->dfSampleBias );   
    TIFFSwabDouble ( &psiaInfo->dfZScale ); 
    TIFFSwabDouble ( &psiaInfo->dfZOffset ); 
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->nDataMin );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->nDataMax );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->nDataAvg );
    TIFFSwabLong   ( (bim::uint32 *) &psiaInfo->ncompression );
  }

  wstr2charcpy (psiaInfo->szSourceNameW,   (char *) (buf + BIM_PSIA_OFFSET_SOURCENAME), 32);
  wstr2charcpy (psiaInfo->szImageModeW,    (char *) (buf + BIM_PSIA_OFFSET_IMAGEMODE), 8);
  wstr2charcpy (psiaInfo->szSetPointUnitW, (char *) (buf + BIM_PSIA_OFFSET_SETPOINTUNIT), 8);
  wstr2charcpy (psiaInfo->szUnitW,         (char *) (buf + BIM_PSIA_OFFSET_UNIT), 8);

  return 0;
}

void psiaGetCurrentPageInfo(TiffParams *tiffParams) {
  if (tiffParams == NULL) return;
  ImageInfo *info = &tiffParams->info;
  if ( tiffParams->subType != tstPsia ) return;

  psiaInfoHeader *meta = &tiffParams->psiaInfo;
  info->resUnits = RES_um;
  info->xRes = meta->dfXScanSize / meta->nWidth;
  info->yRes = meta->dfYScanSize / meta->nHeight;

  info->depth     = 16;
  info->pixelType = FMT_UNSIGNED;
  info->samples   = 1;
  info->width     = tiffParams->psiaInfo.nWidth;
  info->height    = tiffParams->psiaInfo.nHeight;
}

//----------------------------------------------------------------------------
// READ/WRITE FUNCTIONS
//----------------------------------------------------------------------------

bim::uint psiaReadPlane(FormatHandle *fmtHndl, TiffParams *tiffParams, int plane, ImageBitmap *img) {
  if (tiffParams == 0) return 1;
  if (img        == 0) return 1;
  if (tiffParams->tiff == 0) return 1;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return 1;

  bim::uint sample = 0;
  register unsigned int y;
  uchar *p, *p2;

  img->i.depth     = 16;
  img->i.pixelType = FMT_UNSIGNED;
  img->i.samples   = 1;
  img->i.width     = tiffParams->psiaInfo.nWidth;
  img->i.height    = tiffParams->psiaInfo.nHeight;
  if ( allocImg( fmtHndl, &img->i, img) != 0 ) return 1;

  //--------------------------------------------------------------------
  // read actual image 
  //--------------------------------------------------------------------
  std::vector<bim::uint8> bufv;
  ifd->readTag(50434, &bufv);
  if (bufv.size()<getImgSizeInBytes(img)) return 1;
  bim::uint8 *buf = &bufv[0];  

  bim::uint32 line_size = (bim::uint32) img->i.width*2;
  p = ((uchar *) img->bits[0]) + (line_size * (img->i.height-1) );
  p2 = buf;
  for (y=0; y<img->i.height; y++ ) {
    xprogress( fmtHndl, y, img->i.height, "Reading PSIA" );
    if ( xtestAbort( fmtHndl ) == 1) break;  

    _TIFFmemcpy(p, p2, line_size);
    p  -= line_size;
    p2 += line_size;
  }

  if (TinyTiff::bigendian)
    TIFFSwabArrayOfShort( (bim::uint16 *) img->bits[0], getImgSizeInBytes(img)/2 );

  // psia data is stored inverted
  invertImg( img );

  return 0;
}

//----------------------------------------------------------------------------
// METADATA FUNCTIONS
//----------------------------------------------------------------------------

bim::uint append_metadata_psia (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  TiffParams *tiffParams = (TiffParams *) fmtHndl->internalParams;
  if (tiffParams->subType != tstPsia) return 1; 
  ImageInfo *info = &tiffParams->info;
  psiaInfoHeader *meta = &tiffParams->psiaInfo;

  hash->set_value( bim::PIXEL_RESOLUTION_X, info->xRes );
  hash->set_value( bim::PIXEL_RESOLUTION_Y, info->yRes );
  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS );

  std::map< int, std::string > psia_vals;
  psia_vals[0] = "Off";
  psia_vals[1] = "On";

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Source", meta->szSourceNameW );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Head Mode", meta->szImageModeW );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Low Pass Filter", meta->dfLPFStrength );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Auto Flatten", psia_vals[meta->bAutoFlatten] );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"AC Track", psia_vals[meta->bACTrack] );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Width", xstring::xprintf("%d (pixels)", meta->nWidth) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Height", xstring::xprintf("%d (pixels)", meta->nHeight) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Rotation", xstring::xprintf("%.2f (deg)", meta->dfAngle) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Sine Scan", psia_vals[meta->bSineScan] );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Over Scan", xstring::xprintf("%.2f (%%)", meta->dfOverScan) );

  if (meta->bFastScanDir == 0)    
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Fast Scan Dir", "Right to Left" );
  else
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Fast Scan Dir", "Left to Right" );

  if (meta->bSlowScanDir == 0)    
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Slow Scan Dir", "Top to Bototm" );
  else
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Slow Scan Dir", "Bottom to Top" );

  if (meta->bXYSwap == 0)    
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Fast Scan Axis", "X" );
  else
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Fast Scan Axis", "Y" );

  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"X Scan Size", xstring::xprintf("%.2f (%s)", meta->dfXScanSize, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Y Scan Size", xstring::xprintf("%.2f (%s)", meta->dfYScanSize, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"X Scan Offset", xstring::xprintf("%.2f (%s)", meta->dfXOffset, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Y Scan Offset", xstring::xprintf("%.2f (%s)", meta->dfYOffset, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Scan Rate", xstring::xprintf("%.2f (Hz)", meta->dfScanRate) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Set Point", xstring::xprintf("%.4f (%s)", meta->dfSetPoint, meta->szSetPointUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Tip Bias", xstring::xprintf("%.2f (V)", meta->dtTipBias) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Sample Bias", xstring::xprintf("%.2f (V)", meta->dfSampleBias) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Gain", xstring::xprintf("%.4E (%s/step)", meta->dfDataGain, meta->szUnitW) );

  //  double dval = meta->nDataMin * meta->dfDataGain;
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Min", xstring::xprintf("%.4G (%s)", meta->nDataMin * meta->dfDataGain, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Max", xstring::xprintf("%.4G (%s)", meta->nDataMax * meta->dfDataGain, meta->szUnitW) );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Data Avg", (const int) meta->nDataAvg );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Z Scale", meta->dfZScale );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Z Offset", meta->dfZOffset );
  hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"NCompression", (const int) meta->ncompression );
  
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return 1;
  xstring psia_comments = ifd->readTagString(50436);
  if (psia_comments.size()>0)
    hash->append_tag( bim::CUSTOM_TAGS_PREFIX+"Comments", psia_comments );

  return 0;
}


















