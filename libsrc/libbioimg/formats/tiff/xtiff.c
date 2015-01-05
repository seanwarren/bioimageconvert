/*****************************************************************************
  Public interface to Extended TIFF tags
  
  Written by: Niles D. Ritter
  Extended by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>
  
  Target TIFF lib: 3.6.1
  After libtiff 3.7 this code is mostly not needed and only the tag 
  extender is executed

  Notes, some changes are still needed to libtiff 3.6.1 itself:
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


  TODO:
    Tag 270 in STK consists of several NULL terminated strings...

  History:
    03/29/2004 22:23 - First creation
    09/22/2005 15:38 - simplified for libtiff 3.7
        
  Ver : 2
*****************************************************************************/
 
#include "xtiffio.h"

#include <stdio.h>

typedef struct BIM_XTiff {
  TIFFVSetMethod  parent_vsetfield; 
  TIFFVGetMethod  parent_vgetfield; 
  uint16   xd_num_uic2; 
  long*    xd_uic2;
} BIM_XTiff;

//----------------------------------------------------------------------------
// Static parent members
//----------------------------------------------------------------------------
static TIFFExtendProc _ParentExtender;

#define N(a)  (sizeof (a) / sizeof (a[0])) 

//----------------------------------------------------------------------------


/*  Tiff info structure.
 *
 *     Entry format:
 *        { TAGNUMBER, ReadCount, WriteCount, DataType, FIELDNUM, 
 *          OkToChange, PassDirCountOnSet, AsciiName }
 *
 *     For ReadCount, WriteCount, -1 = unknown.
 */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

static const TIFFFieldInfo xtiffFieldInfo[] = 
{
  // STK
  { TIFFTAG_STK_UIC1,  -1,-1, TIFF_LONG,     FIELD_CUSTOM, TRUE, TRUE, "StkUIC1" },
  { TIFFTAG_STK_UIC2,  -1,-1, TIFF_RATIONAL, FIELD_CUSTOM, TRUE, TRUE, "StkUIC2" },
  { TIFFTAG_STK_UIC3,  -1,-1, TIFF_RATIONAL, FIELD_CUSTOM, TRUE, TRUE, "StkUIC3" },
  { TIFFTAG_STK_UIC4,  -1,-1, TIFF_LONG,     FIELD_CUSTOM, TRUE, TRUE, "StkUIC4" },

  // PSIA
  { TIFFTAG_PSIA_MAGIC_NUMBER,          1, 1, TIFF_LONG,  FIELD_CUSTOM, TRUE, TRUE, "PsiaMagicNumber" },
  { TIFFTAG_PSIA_VERSION,               1, 1, TIFF_LONG,  FIELD_CUSTOM, TRUE, TRUE, "PsiaVersion" },
  { TIFFTAG_PSIA_DATA,                 -1,-1, TIFF_BYTE,  FIELD_CUSTOM, TRUE, TRUE, "PsiaData" },
  { TIFFTAG_PSIA_HEADER,               -1,-1, TIFF_UNDEFINED,  FIELD_CUSTOM, TRUE, TRUE, "PsiaHeader" },
  { TIFFTAG_PSIA_COMMENTS,             -1,-1, TIFF_ASCII, FIELD_CUSTOM, TRUE, TRUE, "PsiaComments" },
  { TIFFTAG_PSIA_LINE_PROFILE_HEADER,  -1,-1, TIFF_BYTE,  FIELD_CUSTOM, TRUE, TRUE, "PsiaLineProfileHeader" },

  // EXIF
  { TIFFTAG_EXIF_IFD, 1, 1, TIFF_LONG,  FIELD_CUSTOM, TRUE, TRUE, "ExifIFD" },

  // FLUOVIEW
  { TIFFTAG_FLUO_MMHEADER,     -1,-1, TIFF_BYTE, FIELD_CUSTOM, TRUE, TRUE, "FluoviewHeader" },
  { TIFFTAG_FLUO_MMSTAMP,      -1,-1, TIFF_BYTE, FIELD_CUSTOM, TRUE, TRUE, "FluoviewStamp" },
  { TIFFTAG_FLUO_MMUSERBLOCK , -1,-1, TIFF_BYTE, FIELD_CUSTOM, TRUE, TRUE, "FluoviewUserBlock" },

  // CARL ZEISS LSM
  { TIFFTAG_CZ_LSMINFO, -1,-1, TIFF_BYTE, FIELD_CUSTOM, TRUE, TRUE, "CarlZeissLSMInfo" },

  // GEOTIFF
  { TIFFTAG_GEOPIXELSCALE,    -1,-1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "GeoPixelScale" },
  { TIFFTAG_INTERGRAPH_MATRIX,-1,-1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "Intergraph TransformationMatrix" },
  { TIFFTAG_GEOTRANSMATRIX,   -1,-1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "GeoTransformationMatrix" },
  { TIFFTAG_GEOTIEPOINTS,     -1,-1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "GeoTiePoints" },
  { TIFFTAG_GEOKEYDIRECTORY,  -1,-1, TIFF_SHORT,  FIELD_CUSTOM, TRUE, TRUE, "GeoKeyDirectory" },
  { TIFFTAG_GEODOUBLEPARAMS,  -1,-1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE, "GeoDoubleParams" },
  { TIFFTAG_GEOASCIIPARAMS,   -1,-1, TIFF_ASCII,  FIELD_CUSTOM, TRUE, FALSE,"GeoASCIIParams" }
};

#undef FALSE
#undef TRUE

/*
int _XTIFFVGetField(TIFF* tif, ttag_t tag, va_list ap)
{
  BIM_XTiff *lXtiff = (BIM_XTiff *) tif->tif_clientinfo->data;

  switch (tag) 
  {
    
    case TIFFTAG_STK_UIC2:
      if ( (lXtiff->xd_num_uic2 != 0) && (lXtiff->xd_uic2 != NULL) )
      {
        *va_arg(ap, uint16*) = lXtiff->xd_num_uic2;
        *va_arg(ap, long**) = lXtiff->xd_uic2;
        break;
      }
      else return 0;

    case TIFFTAG_PSIA_COMMENTS:
      *va_arg(ap, uint16*) = 0;
      *va_arg(ap, char**) = NULL;
      break;

    default:
      // return inherited method
      return lXtiff->parent_vgetfield(tif,tag,ap);
      break;
  }

  return (1);
}

void initCopyArray (void **new_arr, void *arr_from, long size)
{
  if (*new_arr != NULL) _TIFFfree(*new_arr);

  *new_arr = _TIFFmalloc( size );
  if (*new_arr != NULL)
    _TIFFmemcpy(*new_arr, arr_from, size );
}

int _XTIFFVSetField(TIFF* tif, ttag_t tag, va_list ap)
{
  BIM_XTiff *lXtiff = (BIM_XTiff *) tif->tif_clientinfo->data;
  TIFFDirectory *td = &tif->tif_dir;

  switch (tag) {

    case TIFFTAG_STK_UIC2:
      lXtiff->xd_num_uic2 = (uint16) va_arg(ap, int);
      initCopyArray ((void**) &lXtiff->xd_uic2, (void*) va_arg(ap, long*), lXtiff->xd_num_uic2*6*sizeof(long));
      break;

    case TIFFTAG_PSIA_COMMENTS:
      // simply discard this tag
      break;

    default:
      return lXtiff->parent_vsetfield(tif,tag,ap);
      break;
    }

  return 1;
}

void _XTIFFPostInitialize(TIFF *tif)
{
  BIM_XTiff *bim_xtiff;

  if (tif->tif_clientinfo == NULL) {
    tif->tif_clientinfo = _TIFFmalloc( sizeof(TIFFClientInfoLink) ); 
    tif->tif_clientinfo->data = NULL;
  }

  if (tif->tif_clientinfo->data == NULL)
    tif->tif_clientinfo->data = _TIFFmalloc( sizeof(BIM_XTiff) );
  bim_xtiff = tif->tif_clientinfo->data;

  bim_xtiff->parent_vsetfield = NULL;
  bim_xtiff->parent_vgetfield = NULL;
  bim_xtiff->xd_num_uic2      = 0;
  bim_xtiff->xd_uic2          = NULL;
   
  tif->tif_clientinfo->next = NULL;
  tif->tif_clientinfo->name = "DIMIN_TIFF_TAG_EXTENSION";

  // Install the extended Tag field info
  TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));

  bim_xtiff->parent_vsetfield = tif->tif_tagmethods.vsetfield;
  tif->tif_tagmethods.vsetfield = _XTIFFVSetField;

  bim_xtiff->parent_vgetfield = tif->tif_tagmethods.vgetfield;
  tif->tif_tagmethods.vgetfield = _XTIFFVGetField;
}
*/

/**********************************************************************
 *    Nothing below this line should need to be changed.
 **********************************************************************/




/*
 *  This is the callback procedure, and is
 *  called by the DefaultDirectory method
 *  every time a new TIFF directory is opened.
 */

void _XTIFFDefaultDirectory(TIFF *tif)
{
    // set up our own defaults
    //_XTIFFPostInitialize( tif );

    // Install the extended Tag field info
    TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));

    /* Since an XTIFF client module may have overridden
     * the default directory method, we call it now to
     * allow it to set up the rest of its own methods.
     */

    if (_ParentExtender) 
        (*_ParentExtender)(tif);
}

/*
 *  XTIFF Initializer -- sets up the callback
 *   procedure for the TIFF module.
 */

static void _XTIFFInitialize(void)
{
  static int first_time=1;
      
  if (! first_time) return; /* Been there. Done that. */
  first_time = 0;

  // Grab the inherited method and install
  _ParentExtender = TIFFSetTagExtender(_XTIFFDefaultDirectory);
}



/**
 * GeoTIFF compatible TIFF file open function.
 *
 * @param name The filename of a TIFF file to open.
 * @param mode The open mode ("r", "w" or "a").
 *
 * @return a TIFF * for the file, or NULL if the open failed.
 *
This function is used to open GeoTIFF files instead of TIFFOpen() from
libtiff.  Internally it calls TIFFOpen(), but sets up some extra hooks
so that GeoTIFF tags can be extracted from the file.  If XTIFFOpen() isn't
used, GTIFNew() won't work properly.  Files opened
with XTIFFOpen() should be closed with XTIFFClose().

The name of the file to be opened should be passed as <b>name</b>, and an
opening mode ("r", "w" or "a") acceptable to TIFFOpen() should be passed as the
<b>mode</b>.<p>

If XTIFFOpen() fails it will return NULL.  Otherwise, normal TIFFOpen()
error reporting steps will have already taken place.<p>
 */

TIFF* XTIFFOpen(const char* name, const char* mode)
{
    TIFF *tif;

    /* Set up the callback */
    _XTIFFInitialize(); 
  
    /* Open the file; the callback will set everything up
     */
    tif = TIFFOpen(name, mode);
    return tif;
}

#ifdef __WIN32__
TIFF* XTIFFOpenW(const wchar_t* name, const char* mode) {
    TIFF *tif;

    /* Set up the callback */
    _XTIFFInitialize();

    /* Open the file; the callback will set everything up */
    tif = TIFFOpenW(name, mode);
    return tif;
}
#endif

TIFF* XTIFFFdOpen(int fd, const char* name, const char* mode)
{
    TIFF *tif;

    /* Set up the callback */
    _XTIFFInitialize(); 

    /* Open the file; the callback will set everything up
     */
    tif = TIFFFdOpen(fd, name, mode);
	  if (tif) tif->tif_fd = fd;
    return tif;
}

TIFF* XTIFFClientOpen(const char* name, const char* mode, thandle_t thehandle,
      TIFFReadWriteProc RWProc, TIFFReadWriteProc RWProc2,
      TIFFSeekProc SProc, TIFFCloseProc CProc,
      TIFFSizeProc SzProc,
      TIFFMapFileProc MFProvc, TIFFUnmapFileProc UMFProc )
{
    TIFF *tif;
    
    /* Set up the callback */
    _XTIFFInitialize(); 
    
    /* Open the file; the callback will set everything up
     */
    tif = TIFFClientOpen(name, mode, thehandle,
                         RWProc, RWProc2,
                         SProc, CProc,
                         SzProc,
                         MFProvc, UMFProc);
    
	  if (tif) tif->tif_fd = (int) thehandle;
    return tif;
}

/**
 * Close a file opened with XTIFFOpen().
 *
 * @param tif The file handle returned by XTIFFOpen().
 * 
 * If a GTIF structure was created with GTIFNew()
 * for this file, it should be freed with GTIFFree()
 * <i>before</i> calling XTIFFClose().
*/

void XTIFFClose(TIFF *tif)
{
 /*
  TIFFClientInfoLink *tif_clientinfo = tif->tif_clientinfo;
  BIM_XTiff *lXtiff;
  
  if (tif_clientinfo != NULL)
  {
    lXtiff = (BIM_XTiff *) tif->tif_clientinfo->data;
    if (lXtiff->xd_uic2 != NULL) { _TIFFfree(lXtiff->xd_uic2); lXtiff->xd_uic2 = NULL; }
    tif->tif_tagmethods.vsetfield = lXtiff->parent_vsetfield;
    tif->tif_tagmethods.vgetfield = lXtiff->parent_vgetfield;
    _TIFFfree( lXtiff );
    tif->tif_clientinfo->data = NULL;
    _TIFFfree( tif->tif_clientinfo );
    tif->tif_clientinfo = NULL;
  }
  */

  TIFFClose(tif);
}
