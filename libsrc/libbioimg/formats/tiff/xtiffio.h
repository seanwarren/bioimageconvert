/*****************************************************************************
  Public interface to Extended TIFF tags
  
  Written by: Niles D. Ritter
  Extended by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>
  
  Target TIFF lib: 3.6.1

  Notes, some changes are still needed to libtiff itself:
    1) Disable message of unknown tag, we don't care to read it all the time

    2) Malformed tags are ignored by the library so some modifs are still
    to the real library file so tag would not be ignored:
    Here we go:
    Tag: 33629: Change in file tif_dirread.c
    {
      Line: 237
      Iside loop: for (dp = dir, n = dircount; n > 0; n--, dp++) {
      We need to change the type of this tag to LONG so it will be accepted 
      by the type check:        
      if (dp->tdir_tag == 33629) { dp->tdir_type = TIFF_LONG; } // dima
    }
    
  History:
    03/29/2004 22:23 - First creation
        
  Ver : 1
*****************************************************************************/

#ifndef BIM_XTIFFIO_H
#define BIM_XTIFFIO_H

//#ifndef XMD_H
//#define XMD_H // Shut JPEGlib up.
//#endif  

#include <tiffio.h>
#include <tiffiop.h>

// Define public Tag names and values here 

// STK TAGS
#define TIFFTAG_STK_UIC1     33628
#define TIFFTAG_STK_UIC2     33629
#define TIFFTAG_STK_UIC3     33630
#define TIFFTAG_STK_UIC4     33631

// PSIA TAGS
#define TIFFTAG_PSIA_MAGIC_NUMBER        50432
#define TIFFTAG_PSIA_VERSION             50433
#define TIFFTAG_PSIA_DATA                50434
#define TIFFTAG_PSIA_HEADER              50435
#define TIFFTAG_PSIA_COMMENTS            50436
#define TIFFTAG_PSIA_LINE_PROFILE_HEADER 50437

// EXIF
#define TIFFTAG_EXIF_IFD 34665

// FLUOVIEW TAGS
#define TIFFTAG_FLUO_MMHEADER    34361	
#define TIFFTAG_FLUO_MMSTAMP     34362	
#define TIFFTAG_FLUO_MMUSERBLOCK 34386

// CARL ZEISS LSM
#define TIFFTAG_CZ_LSMINFO       34412	

// MICRO-MANAGER
#define TIFFTAG_MICROMANAGER     51123

// GEOTIFF TAGS
// tags 33550 is a private tag registered to SoftDesk, Inc
#define TIFFTAG_GEOPIXELSCALE       33550
// tags 33920-33921 are private tags registered to Intergraph, Inc
#define TIFFTAG_INTERGRAPH_MATRIX    33920   // $use TIFFTAG_GEOTRANSMATRIX !
#define TIFFTAG_GEOTIEPOINTS         33922
// tags 34263-34264 are private tags registered to NASA-JPL Carto Group
#ifdef JPL_TAG_SUPPORT
#define TIFFTAG_JPL_CARTO_IFD        34263    // $use GeoProjectionInfo !
#endif
#define TIFFTAG_GEOTRANSMATRIX       34264    // New Matrix Tag replaces 33920
// tags 34735-3438 are private tags registered to SPOT Image, Inc 
#define TIFFTAG_GEOKEYDIRECTORY      34735
#define TIFFTAG_GEODOUBLEPARAMS      34736
#define TIFFTAG_GEOASCIIPARAMS       34737

/* 
 *  Define Printing method flags. These
 *  flags may be passed in to TIFFPrintDirectory() to
 *  indicate that those particular field values should
 *  be printed out in full, rather than just an indicator
 *  of whether they are present or not.
 */
#define	TIFFPRINT_GEOKEYDIRECTORY	0x80000000
#define	TIFFPRINT_GEOKEYPARAMS		0x40000000



/**********************************************************************
 *    Nothing below this line should need to be changed by the user.
 **********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

extern TIFF* XTIFFOpen(const char* name, const char* mode);
#ifdef __WIN32__
#include <wchar.h>
extern TIFF* XTIFFOpenW(const wchar_t* name, const char* mode);
#endif
extern TIFF* XTIFFFdOpen(int fd, const char* name, const char* mode);
extern void XTIFFClose(TIFF *tif);

extern TIFF* XTIFFClientOpen(const char* name, const char* mode, 
                                      thandle_t thehandle,
                                      TIFFReadWriteProc, TIFFReadWriteProc,
                                      TIFFSeekProc, TIFFCloseProc,
                                      TIFFSizeProc,
                                      TIFFMapFileProc, TIFFUnmapFileProc);
#if defined(__cplusplus)
}
#endif

#endif // BIM_XTIFFIO_H

