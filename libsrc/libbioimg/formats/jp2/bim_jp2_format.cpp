/*****************************************************************************
  JPEG2000 support 
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION
  
  Author: Mario Emmenlauer <mario@emmenlauer.de>

  History:
    04/19/2015 14:20 - First creation
        
  Ver : 1
*****************************************************************************/
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <xstring.h>

#include <tag_map.h>
#include <bim_exiv_parse.h>

#include "bim_jp2_format.h"
#include "bim_jp2_decompress.h"




//----------------------------------------------------------------------------
// MetaData tags
//----------------------------------------------------------------------------



//****************************************************************************
// Misc
//****************************************************************************

bim::Jp2Params::Jp2Params()
{
  i = initImageInfo(); 
}

bim::Jp2Params::~Jp2Params()
{

}


//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

bool jp2GetImageInfo( bim::FormatHandle *fmtHndl )
{
#ifdef DEBUG
  std::cerr << "jp2GetImageInfo() called" << std::endl;
#endif
  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;

/*
  // TODO FIXME: this is just an example taken from another file format.
  // Implement this missing functionality.
  bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;
  bim::ImageInfo *info = &par->i;  

  *info = initImageInfo();
  info->imageMode = IM_GRAYSCALE;
  info->tileWidth = 0;
  info->tileHeight = 0; 
  info->transparentIndex = 0;
  info->transparencyMatting = 0;
  info->lut.count = 0;
  info->samples = 1;

  info->number_pages = *(header+4) + *(header+5) * 256; // multiplatform approach, no swapping needed
  info->width  = *(header + 0) + *(header + 1) * 256;
  info->height = *(header + 2) + *(header + 3) * 256;

  par->has_notes = * (int32 *) (header + 10);
  if (bim::bigendian) swapLong ( (uint32*) &par->has_notes );
  par->num_images = info->number_pages;


  val = (short)( *(header + 14) + *(header + 15) * 256 ); 
  if (val == 1)   {
    info->depth = 8;
    info->pixelType = FMT_UNSIGNED;
  }  else   {
    info->depth = 16;
    info->pixelType = FMT_UNSIGNED;
  }

  par->data_offset = 76;
  par->page_size_bytes = info->width * info->height * (info->depth / 8);
  par->notes_offset = par->data_offset + par->page_size_bytes * info->number_pages;

  if ( (info->number_pages == 3) || (info->number_pages == 2) )   {
    info->samples = 3;    
    info->number_pages = 1;
    info->imageMode = IM_MULTI;
  }

  // if more than 1 page it's a z series
  if (info->number_pages > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_Z;
    info->number_z = info->number_pages;
  }
*/
  return true;
}

//****************************************************************************
// READ PROC
//****************************************************************************

// The READ PROC read_jp2_image() is defined in bim_jp2_decompress.cpp


//****************************************************************************
// WRITE PROC
//****************************************************************************

static int write_jp2_image( bim::FormatHandle *fmtHndl )
{
#ifdef DEBUG
  std::cerr << "write_jp2_image() called" << std::endl;
#endif

  return 1;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint jp2_append_metadata(bim::FormatHandle *fmtHndl, bim::TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;

  for (size_t comidx = 0; comidx < par->comments.size(); ++comidx) {
    // NOTE: if comments contain an '=' sign, they are 'key=value' strings and
    // should be split on the '=' sign:
    const size_t pos = par->comments[comidx].find('=');
    if (pos != std::string::npos) {
      std::string key = par->comments[comidx].substr(0, pos);
      std::string value = par->comments[comidx].substr(pos+1);
      hash->append_tag( "custom/" + key, value );
    } else {
      hash->append_tag( "custom/Comment", par->comments[comidx] );
    }
  }

  // Append EXIV2 metadata
  exiv_append_metadata (fmtHndl, hash );

  return 0;
}



//****************************************************************************
// FORMAT DEMANDED FUNTIONS
//****************************************************************************

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int jp2ValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < 12) return -1;

  // The file is a valid jp2 image (openjpeg magic_format JP2_CFMT, correct extension .jp2):
  if (memcmp(magic, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(magic, JP2_MAGIC, 4) == 0) return 0;
  // The file is a valid jp2 image (openjpeg magic_format J2K_CFMT, correct extensions .j2k, .jpc, .j2c):
  if (memcmp(magic, J2K_CODESTREAM_MAGIC, 4) == 0) return 0;

  return -1;
}

bim::FormatHandle jp2AquireFormatProc( void ) {
  return bim::initFormatHandle();
}

void jp2CloseImageProc(bim::FormatHandle *fmtHndl);
void jp2ReleaseFormatProc(bim::FormatHandle *fmtHndl) {
#ifdef DEBUG
  std::cerr << "jp2ReleaseFormatProc() called" << std::endl;
#endif

  if (fmtHndl == NULL) return;
  jp2CloseImageProc( fmtHndl );  
}

//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void jp2SetWriteParameters(bim::FormatHandle *fmtHndl) {
#ifdef DEBUG
  std::cerr << "jp2SetWriteParameters() called" << std::endl;
#endif

  if (fmtHndl == NULL) return;
/*
  fmtHndl->order = 1; // use progressive encoding by default
  fmtHndl->quality = 95;

  if (!fmtHndl->options) return;
  xstring str = fmtHndl->options;
  std::vector<xstring> options = str.split( " " );
  if (options.size() < 1) return;
  
  int i = -1;
  while (i<(int)options.size()-1) {
    i++;

    if ( options[i]=="quality" && options.size()-i>0 ) {
      i++;
      fmtHndl->quality = options[i].toInt( 100 );
      continue;
    } else
    if ( options[i]=="progressive" && options.size()-i>0 ) {
      i++;    
      if (options[i]=="no") fmtHndl->order = 0;
      else
        fmtHndl->order = 1;      
      continue;
    }
    
  } // while
*/
}

void jp2CloseImageProc(bim::FormatHandle *fmtHndl) {
#ifdef DEBUG
    std::cerr << "jp2CloseImageProc() called" << std::endl;
#endif

    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;
    delete par;
    fmtHndl->internalParams = NULL;
    xclose( fmtHndl );
}

bim::uint jp2OpenImageProc(bim::FormatHandle *fmtHndl, bim::ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) jp2CloseImageProc(fmtHndl);  

  fmtHndl->io_mode = io_mode;
  if ( io_mode == bim::IO_READ ) {
#ifdef DEBUG
    std::cerr << "jp2OpenImageProc(): setting read mode, and opening the file." << std::endl;
#endif

    fmtHndl->internalParams = (void *) new bim::Jp2Params();
    bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;

    bim::xopen(fmtHndl);
    if (!fmtHndl->stream) { 
      jp2CloseImageProc(fmtHndl); 
      return 1; 
    }

    if ( !jp2GetImageInfo( fmtHndl ) ) {
      jp2CloseImageProc(fmtHndl);
      return 1;
    };
  }

  // TODO FIXME: writing is currently not implemented/tested:
  if (io_mode == bim::IO_WRITE) {
#ifdef DEBUG
    std::cerr << "jp2OpenImageProc(): setting write mode, calling jp2SetWriteParameters()." << std::endl;
#endif
    jp2SetWriteParameters(fmtHndl);
  }

  return 0;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint jp2GetNumPagesProc( bim::FormatHandle *fmtHndl ) {
  // NOTE: Currently we support only a single page JPEG2000 file.
  //       I am not certain whether JPEG2000 supports multiple pages?

  if (fmtHndl == NULL) return 0;
  return 1;
}

bim::ImageInfo jp2GetImageInfoProc( bim::FormatHandle *fmtHndl, bim::uint page_num ) {
#ifdef DEBUG
  std::cerr << "jp2GetImageInfoProc() called" << std::endl;
#endif

  bim::ImageInfo ii = bim::initImageInfo();
  if (fmtHndl == NULL) return ii;

  bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint jp2AddMetaDataProc(bim::FormatHandle * /*fmtHndl*/) {
#ifdef DEBUG
  std::cerr << "jp2AddMetaDataProc() called" << std::endl;
#endif

  return 1;
}

bim::uint jp2ReadMetaDataProc(bim::FormatHandle * /*fmtHndl*/, bim::uint /*page*/, int /*group*/, int /*tag*/, int /*type*/) {
#ifdef DEBUG
  std::cerr << "jp2ReadMetaDataProc() called" << std::endl;
#endif

  return 1;
}

char* jp2ReadMetaDataAsTextProc( bim::FormatHandle *fmtHndl ) {
#ifdef DEBUG
  std::cerr << "jp2ReadMetaDataAsTextProc() called" << std::endl;
#endif

  if (fmtHndl == NULL) return NULL;
  return NULL;
}

//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint jp2ReadImageProc( bim::FormatHandle *fmtHndl, bim::uint page ) {
  if (fmtHndl == NULL) return 1;
  if (!fmtHndl->stream) return 1;

  fmtHndl->pageNumber = page;
  return read_jp2_image( fmtHndl );
}

bim::uint jp2WriteImageProc( bim::FormatHandle *fmtHndl ) {
#ifdef DEBUG
  std::cerr << "jp2WriteImageProc() called" << std::endl;
#endif

/*
  if (fmtHndl == NULL) return 1;
  bim::xopen(fmtHndl);
  if (!fmtHndl->stream) return 1;
  bim::uint res = write_jp2_image( fmtHndl );
  xflush( fmtHndl );
  xclose( fmtHndl );
  return res;
*/
  return 1;
}


//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

bim::FormatItem jp2Items[1] = {
  {
    "JP2",            // short name, no spaces
    "JPEG2000 File Format", // Long format name
    "jp2|j2k|jpx|jpc|jpt",  // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    1, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    1, //canWriteMeta; // 0 - NO, 1 - YES
    0, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 1, 1, 4, 8, 16, 1 } 
  }
};

bim::FormatHeader jp2Header = {
  sizeof(bim::FormatHeader),
  "2.1.0",
  "JPEG2000 CODEC",
  "JP2-JFIF Compliant CODEC",
  
  12,                 // 0 or more, specify number of bytes needed to identify the file
  {1, 1, jp2Items},   // jp2Supported,
  
  jp2ValidateFormatProc,
  // begin
  jp2AquireFormatProc, //AquireFormatProc
  // end
  jp2ReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  jp2OpenImageProc, //OpenImageProc
  jp2CloseImageProc, //CloseImageProc 

  // info
  jp2GetNumPagesProc, //GetNumPagesProc
  jp2GetImageInfoProc, //GetImageInfoProc


  // read/write
  jp2ReadImageProc, //ReadImageProc 
  jp2WriteImageProc, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //jp2ReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  jp2ReadMetaDataProc, //ReadMetaDataProc
  jp2AddMetaDataProc,  //AddMetaDataProc
  jp2ReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  jp2_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""
};

extern "C" {

bim::FormatHeader* jp2GetFormatHeader(void)
{
  return &jp2Header;
}

} // extern C

