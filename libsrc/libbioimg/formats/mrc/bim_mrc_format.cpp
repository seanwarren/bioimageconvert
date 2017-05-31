/*****************************************************************************
    MRC format support
    Copyright (c) 2017 ViQi Inc
    Author: Dmitry Fedorov <dima@dimin.net>
    License: FreeBSD

    History:
    2017-05-30 - First creation

    Ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <cfloat>
#include <string>

#include "bim_mrc_format.h"

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

// windows: use secure C libraries with VS2005 or higher
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
#pragma warning(disable:4996)
//#define HAVE_SECURE_C
//#pragma message(">>>>> IBW: using secure c libraries")
#endif 

using namespace bim;

//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

void swapHeader(MrcHeader *h) {
    bim::uint32 *v = (bim::uint32 *) h;
    for (int i=0; i<56; ++i) {
        swapLong((bim::uint32*) &v[i]);
    }
}

void mrcGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    *info = initImageInfo();

    if (fmtHndl->stream == NULL) return;
    if (xseek(fmtHndl, 0, SEEK_SET) != 0) return;
    if (xread(fmtHndl, &par->header, 1, sizeof(MrcHeader)) != sizeof(MrcHeader)) return;

    // swap structure elements if running on Big endian machine...
    if (bim::bigendian) {
        swapHeader(&par->header);
    }
    
    // get correct type size
    switch (par->header.mode) {
    case MRC_MODE_INT8:
        par->pixel_size = 1;
        par->data_type = bim::TAG_SBYTE;
        par->data_format = bim::FMT_SIGNED;
        break;
    case MRC_MODE_INT16:
        par->pixel_size = 2;
        par->data_type = bim::TAG_SSHORT;
        par->data_format = bim::FMT_SIGNED;
        break;
    case MRC_MODE_UINT16:
        par->pixel_size = 2;
        par->data_type = bim::TAG_SHORT;
        par->data_format = bim::FMT_UNSIGNED;
        break;
    case MRC_MODE_FLOAT32:
        par->pixel_size = 4;
        par->data_type = bim::TAG_FLOAT;
        par->data_format = bim::FMT_FLOAT;
        break;
    case MRC_MODE_CINT16:
        par->pixel_size = 4;
        par->data_type = bim::TAG_FLOAT;
        par->data_format = bim::FMT_COMPLEX;
        break;
    case MRC_MODE_CFLOAT32:
        par->pixel_size = 8;
        par->data_type = bim::TAG_SRATIONAL;
        par->data_format = bim::FMT_COMPLEX;
        break;
    }

    par->data_offset = BIM_MRC_HEADER_SIZE + par->header.next; // unless extended header is used
    par->plane_size = par->header.nx*par->header.ny*par->pixel_size;

    // read extended header 
    if (par->header.next > 0) {
        int sz = sizeof(MrcHeaderExt);
        int nimg = 1024;
        par->exts.resize(nimg);
        if (xseek(fmtHndl, BIM_MRC_HEADER_SIZE, SEEK_SET) != 0) return;
        if (xread(fmtHndl, &par->exts[0], 1, sz*nimg) != sz*nimg) return;
    }

    // set image parameters
    info->width = par->header.nx;
    info->height = par->header.ny;
    info->samples = 1;
    info->number_pages = par->header.nz;
    info->imageMode = IM_GRAYSCALE;
    info->number_z = info->number_pages;
    info->depth = par->pixel_size*8;
    info->pixelType = par->data_format;
}

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int mrcValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_MRC_MAGIC_SIZE) return -1;
    MrcHeader *h = (MrcHeader *)magic; // header will be only guaranteed up to BIM_MRC_MAGIC_SIZE, disregard more advanced elements
    
    bim::uint32 mode = h->mode;
    //bim::uint32 ver = h->NVERSION;
    //char *ext = h->EXTTYP;

    if (bim::bigendian) {
        swapLong(&mode);
        //swapLong(&ver);
    }

    if (mode == 6 || (mode>=0 && mode<=4)) return 0;
    
    /*if (memcmp(ext, "CCP4", 4) == 0) return 0; // Format from CCP4 suite
    if (memcmp(ext, "MRCO", 4) == 0) return 0; // MRC format
    if (memcmp(ext, "SERI", 4) == 0) return 0; // SerialEM
    if (memcmp(ext, "AGAR", 4) == 0) return 0; // Agard
    if (memcmp(ext, "FEI1", 4) == 0) return 0; // FEI software, e.g.EPU and Xplore3D, Amira, Avizo

    if (ver == 20140) return 0; // Year * 10 + version within the year(base 0)*/

    return -1;
}

FormatHandle mrcAquireFormatProc(void) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void mrcCloseImageProc(FormatHandle *fmtHndl);
void mrcReleaseFormatProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    mrcCloseImageProc(fmtHndl);
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void mrcCloseImageProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose(fmtHndl);
    if (fmtHndl->internalParams != NULL) {
        MrcParams *par = (MrcParams *)fmtHndl->internalParams;
        delete par;
    }
    fmtHndl->internalParams = NULL;
}

bim::uint mrcOpenImageProc(FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) mrcCloseImageProc(fmtHndl);
    fmtHndl->internalParams = (void *) new MrcParams();

    fmtHndl->io_mode = io_mode;
    xopen(fmtHndl);
    if (!fmtHndl->stream) {
        mrcCloseImageProc(fmtHndl);
        return 1;
    };

    if (io_mode == IO_READ) {
        mrcGetImageInfo(fmtHndl);
        return 0;
    }
    return 1;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint mrcGetNumPagesProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;
    return par->i.number_pages;
}


ImageInfo mrcGetImageInfoProc(FormatHandle *fmtHndl, bim::uint page_num) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

bim::uint mrcReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->stream == NULL) return 1;
    fmtHndl->pageNumber = page;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    if (fmtHndl->stream == NULL) return 1;

    // get needed page 
    fmtHndl->pageNumber = page = bim::trim<bim::uint>(page, 0, info->number_pages - 1);

    //allocate image
    ImageBitmap *img = fmtHndl->image;
    if (allocImg(fmtHndl, info, img) != 0) return 1;

    xprogress(fmtHndl, 0, 10, "Reading MRC");

    bim::uint64 offset_page = par->plane_size * page;
    if (xseek(fmtHndl, par->data_offset + offset_page, SEEK_SET) != 0) return 1;
    if (xread(fmtHndl, img->bits[0], par->plane_size, 1) != 1) return 1;

    return 0;
}

bim::uint mrcWriteImageProc(FormatHandle *) {
    return 1;
}

//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint mrc_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    if (!hash) return 1;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;

    if (par->exts.size() > 0) {
        MrcHeaderExt *h = &par->exts[fmtHndl->pageNumber];

        hash->append_tag("pixel_resolution_x", h->pixel_size);
        hash->append_tag("pixel_resolution_y", h->pixel_size);
        hash->append_tag("pixel_resolution_z", h->pixel_size);

        hash->append_tag("pixel_resolution_unit_x", "meters");
        hash->append_tag("pixel_resolution_unit_y", "meters");
        hash->append_tag("pixel_resolution_unit_z", "meters");


        hash->append_tag("FEI/a_tilt", h->a_tilt);
        hash->append_tag("FEI/b_tilt", h->b_tilt);
        hash->append_tag("FEI/x_stage", h->x_stage);
        hash->append_tag("FEI/y_stage", h->y_stage);
        hash->append_tag("FEI/z_stage", h->z_stage);
        hash->append_tag("FEI/x_shift", h->x_shift);
        hash->append_tag("FEI/y_shift", h->y_shift);
        hash->append_tag("FEI/defocus", h->defocus);
        hash->append_tag("FEI/exp_time", h->exp_time);
        hash->append_tag("FEI/mean_int", h->mean_int);
        hash->append_tag("FEI/tilt_axis", h->tilt_axis);
        hash->append_tag("FEI/magnification", h->magnification);
        hash->append_tag("FEI/ht", h->ht);
        hash->append_tag("FEI/binning", h->binning);
        hash->append_tag("FEI/appliedDefocus", h->appliedDefocus);
    }

    return 0;
}

//****************************************************************************
// EXPORTED FUNCTION
//****************************************************************************

FormatItem mrcItems[1] = {
    {
        "MRC",            // short name, no spaces
        "Medical Research Council", // Long format name
        "mrc",        // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        0, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        0, //canWriteMeta; // 0 - NO, 1 - YES
        0, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 1, 0, 0, 0, 1 }
    }
};

FormatHeader mrcHeader = {

    sizeof(FormatHeader),
    "1.0.0",
    "MRC",
    "MRC",

    12,                    // 0 or more, specify number of bytes needed to identify the file
    { 1, 1, mrcItems },   //dimJpegSupported,

    mrcValidateFormatProc,
    // begin
    mrcAquireFormatProc, //AquireFormatProc
    // end
    mrcReleaseFormatProc, //ReleaseFormatProc

    // params
    NULL, //AquireIntParamsProc
    NULL, //LoadFormatParamsProc
    NULL, //StoreFormatParamsProc

    // image begin
    mrcOpenImageProc, //OpenImageProc
    mrcCloseImageProc, //CloseImageProc 

    // info
    mrcGetNumPagesProc, //GetNumPagesProc
    mrcGetImageInfoProc, //GetImageInfoProc


    // read/write
    mrcReadImageProc, //ReadImageProc 
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
    mrc_append_metadata, //AppendMetaDataProc

    NULL,
    NULL,
    ""

};

extern "C" {

FormatHeader* mrcGetFormatHeader(void) {
  return &mrcHeader;
}

} // extern C



