/*****************************************************************************
  Igor binary file format v5 (IBW)
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    10/19/2005 16:03 - First creation
            
  Ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

// windows: use secure C libraries with VS2005 or higher
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
  //#define HAVE_SECURE_C
  //#pragma message(">>>>> IBW: using secure c libraries")
#endif 

#include "bim_ibw_format.h"

using namespace bim;

#include "bim_ibw_format_io.cpp"

//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

void swapBinHeader5(BinHeader5 *bh)
{
  swapShort( (uint16*) &bh->checksum );
  swapLong ( (uint32*) &bh->wfmSize );  

  swapLong ( (uint32*) &bh->formulaSize ); 
  swapLong ( (uint32*) &bh->noteSize ); 
  swapLong ( (uint32*) &bh->dataEUnitsSize ); 
  swapLong ( (uint32*) &bh->sIndicesSize ); 

  swapLong ( (uint32*) &bh->wfmSize ); 
  swapLong ( (uint32*) &bh->wfmSize ); 

  for (int i=0; i<MAXDIMS; ++i)
  {
    swapLong ( (uint32*) &bh->dimEUnitsSize[i] ); 
    swapLong ( (uint32*) &bh->dimLabelsSize[i] ); 
  }
}

void swapWaveHeader5(WaveHeader5 *wh)
{
  swapLong ( (uint32*) &wh->creationDate );   
  swapLong ( (uint32*) &wh->modDate );   
  swapLong ( (uint32*) &wh->npnts );  
  swapShort( (uint16*) &wh->type );
  swapShort( (uint16*) &wh->fsValid );
  swapDouble( (float64*) &wh->topFullScale );
  swapDouble( (float64*) &wh->botFullScale );

  for (int i=0; i<MAXDIMS; ++i)
  {
    swapLong ( (uint32*) &wh->nDim[i] ); 
    swapDouble( (float64*) &wh->sfA[i] );
    swapDouble( (float64*) &wh->sfB[i] );
  }
}

void ibwReadNote ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  IbwParams *par = (IbwParams *) fmtHndl->internalParams;
  
  long buf_size = par->bh.noteSize;
  par->note.resize(buf_size+1);
  char *buf = &par->note[0];
  buf[buf_size] = '\0';

  if (xseek(fmtHndl, par->notes_offset, SEEK_SET) != 0) return;
  if (xread( fmtHndl, buf, buf_size, 1 ) != 1) return;

  for (int i=0; i<buf_size; ++i)
    if (buf[i] == 0x0d) buf[i] = 0x0a;
}

void ibwGetImageInfo( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;
  ImageInfo *info = &ibwPar->i; 
  *info = initImageInfo();

  if (fmtHndl->stream == NULL) return;
  if (xseek(fmtHndl, 0, SEEK_SET) != 0) return;
  if ( xread( fmtHndl, &ibwPar->bh, 1, sizeof(BinHeader5) ) != sizeof(BinHeader5)) return;
  
  // test if little-endian
  if (memcmp( &ibwPar->bh.version, ibwMagicWin, BIM_IBW_MAGIC_SIZE ) == 0) 
    ibwPar->little_endian = true;
  else
    ibwPar->little_endian = false;

  // swap structure elements if running on Big endian machine...
  if ( (bim::bigendian) && (ibwPar->little_endian == true) ) 
  {
    swapBinHeader5( &ibwPar->bh );
  }
 
  if ( xread( fmtHndl, &ibwPar->wh, 1, sizeof(WaveHeader5) ) != sizeof(WaveHeader5)) return;

  // swap structure elements if running on Big endian machine...
  if ( (bim::bigendian) && (ibwPar->little_endian == true) ) 
  {
    swapWaveHeader5( &ibwPar->wh );
  }

/*
  info->resUnits = BIM_RES_mim;
  info->xRes = nimg.xR / nimg.width;
  info->yRes = nimg.yR / nimg.height;
  */

  // get correct type size
  switch ( ibwPar->wh.type )
  {
    case NT_CMPLX:
      ibwPar->real_bytespp = 8;
      ibwPar->real_type  = TAG_SRATIONAL;
      break;
    case NT_FP32:
      ibwPar->real_bytespp = 4;
      ibwPar->real_type  = TAG_FLOAT;
      break;
    case NT_FP64:
      ibwPar->real_bytespp = 8;
      ibwPar->real_type  = TAG_DOUBLE;
      break;
    case NT_I8:
      ibwPar->real_bytespp = 1;
      ibwPar->real_type  = TAG_BYTE;
      break;
    case NT_I16:
      ibwPar->real_bytespp = 2;
      ibwPar->real_type  = TAG_SHORT;
      break;
    case NT_I32:
      ibwPar->real_bytespp = 4;
      ibwPar->real_type  = TAG_LONG;
      break;
    default:
      ibwPar->real_bytespp = 1;
      ibwPar->real_type  = TAG_BYTE;
  }

  // prepare needed vars
  ibwPar->data_offset    = sizeof(BinHeader5) + sizeof(WaveHeader5) - 4; //the last pointer is actually initial data
  ibwPar->data_size      = ibwPar->wh.npnts * ibwPar->real_bytespp;
  ibwPar->formula_offset = ibwPar->data_offset + ibwPar->data_size; 
  ibwPar->notes_offset   = ibwPar->formula_offset + ibwPar->bh.formulaSize;

  // set image parameters
  info->width   = ibwPar->wh.nDim[0];
  info->height  = ibwPar->wh.nDim[1];
  info->samples = 1;
  info->number_pages = ibwPar->wh.nDim[2];
  info->imageMode = IM_GRAYSCALE;
  //info->number_z = info->number_pages;
  
  // by now we'll normalize all data
  info->depth     = 8;
  info->pixelType = FMT_UNSIGNED;
  if (info->samples>1) 
      info->imageMode = IM_MULTI;

  //-------------------------------------------------
  // init palette
  //-------------------------------------------------
  info->lut.count = 0;
  for (int i=0; i<256; i++) info->lut.rgba[i] = xRGB(i, i, i);

  ibwReadNote ( fmtHndl );
}

//****************************************************************************
// FORMAT DEMANDED FUNTIONS
//****************************************************************************


//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int ibwValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName)
{
  if (length < BIM_IBW_MAGIC_SIZE) return -1;
  if (memcmp( magic, ibwMagicWin, BIM_IBW_MAGIC_SIZE ) == 0) return 0;
  if (memcmp( magic, ibwMagicMac, BIM_IBW_MAGIC_SIZE ) == 0) return 0;
  return -1;
}

FormatHandle ibwAquireFormatProc( void )
{
  FormatHandle fp = initFormatHandle();
  return fp;
}

void ibwCloseImageProc( FormatHandle *fmtHndl);
void ibwReleaseFormatProc (FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return;
  ibwCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void ibwCloseImageProc (FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return;
  xclose ( fmtHndl );
  if (fmtHndl->internalParams != NULL) 
  {
    IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;
    delete ibwPar;
  }
  fmtHndl->internalParams = NULL;
}

bim::uint ibwOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) ibwCloseImageProc (fmtHndl);  
  fmtHndl->internalParams = (void *) new IbwParams();

  fmtHndl->io_mode = io_mode;
  xopen(fmtHndl);
  if (!fmtHndl->stream) {
      ibwCloseImageProc(fmtHndl);
      return 1;
  };

  if (io_mode == IO_READ) {
      ibwGetImageInfo( fmtHndl );
      return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint ibwGetNumPagesProc ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;

  return ibwPar->i.number_pages;
}


ImageInfo ibwGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num )
{
  ImageInfo ii = initImageInfo();

  if (fmtHndl == NULL) return ii;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;

  return ibwPar->i;
  page_num;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint ibwReadMetaDataProc (FormatHandle *fmtHndl, bim::uint , int group, int tag, int type) {
  return 0;
}

char* ibwReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
  return NULL;
}

bim::uint ibwAddMetaDataProc (FormatHandle *) {
  return 1;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint ibwReadImageProc  ( FormatHandle *fmtHndl, bim::uint page )
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->stream == NULL) return 1;
  fmtHndl->pageNumber = page;
  return read_ibw_image( fmtHndl );
}

bim::uint ibwWriteImageProc ( FormatHandle * ) {
  return 1;
}



//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

FormatItem ibwItems[1] = {
  {
    "IBW",            // short name, no spaces
    "Igor binary file format v5", // Long format name
    "ibw",        // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 1, 0, 0, 0, 1 } 
  }
};

FormatHeader ibwHeader = {

  sizeof(FormatHeader),
  "1.0.0",
  "IBW",
  "IBW",
  
  12,                      // 0 or more, specify number of bytes needed to identify the file
  {1, 1, ibwItems},   //dimJpegSupported,
  
  ibwValidateFormatProc,
  // begin
  ibwAquireFormatProc, //AquireFormatProc
  // end
  ibwReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  ibwOpenImageProc, //OpenImageProc
  ibwCloseImageProc, //CloseImageProc 

  // info
  ibwGetNumPagesProc, //GetNumPagesProc
  ibwGetImageInfoProc, //GetImageInfoProc


  // read/write
  ibwReadImageProc, //ReadImageProc 
  NULL, //WriteImageProc
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
  ibw_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {
FormatHeader* ibwGetFormatHeader(void) { return &ibwHeader; }
} // extern C





