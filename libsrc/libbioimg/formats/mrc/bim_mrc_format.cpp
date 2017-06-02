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

#include <xtypes.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_format_misc.h>

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
    for (int i=0; i<25; ++i) {
        swapLong((bim::uint32*) &v[i]);
    }
    for (int i = 33; i<56; ++i) {
        swapLong((bim::uint32*) &v[i]);
    }
}

void mrcGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    MrcParams *par = (MrcParams *)fmtHndl->internalParams;
    MrcHeader *h = &par->header;
    ImageInfo *info = &par->i;
    *info = initImageInfo();

    if (fmtHndl->stream == NULL) return;
    if (xseek(fmtHndl, 0, SEEK_SET) != 0) return;
    if (xread(fmtHndl, h, 1, sizeof(MrcHeader)) != sizeof(MrcHeader)) return;

    // swap structure elements if running on Big endian machine
    // or handle incorrect cases of files written in big-endian format
    if (bim::bigendian || h->nlabl > 10 && h->mapc > 3 && h->mapr > 3 && h->maps > 3) {
        swapHeader(h);
    }

    // set image parameters
    info->width = h->nx;
    info->height = h->ny;
    info->imageMode = IM_GRAYSCALE;
    info->samples = 1;
    info->pixelType = bim::FMT_UNSIGNED;
    info->number_pages = h->nz;
    info->number_z = info->number_pages;

    // get correct type size
    switch (h->mode) {
    case MRC_MODE_INT8:
        info->depth = 8;
        info->pixelType = bim::FMT_SIGNED;
        break;
    case MRC_MODE_INT16:
        info->depth = 16;
        info->pixelType = bim::FMT_SIGNED;
        break;
    case MRC_MODE_UINT16:
        info->depth = 16;
        info->pixelType = bim::FMT_UNSIGNED;
        break;
    case MRC_MODE_FLOAT32:
        info->depth = 32;
        info->pixelType = bim::FMT_FLOAT;
        break;
    case MRC_MODE_CINT16:
        info->depth = 16;
        info->pixelType = bim::FMT_COMPLEX;
        break;
    case MRC_MODE_CFLOAT32:
        info->depth = 64;
        info->pixelType = bim::FMT_COMPLEX;
        break;
    case MRC_MODE_UINT4:
        info->depth = 4;
        info->pixelType = bim::FMT_UNSIGNED;
        break;
    case MRC_MODE_RGB8:
        info->depth = 8;
        info->pixelType = bim::FMT_UNSIGNED;
        info->samples = 3;
        break;
    }

    par->data_offset = BIM_MRC_HEADER_SIZE + h->next; // unless extended header is used

    // read extended header 
    if (h->next > 0) {
        int sz = sizeof(MrcHeaderExt);
        int nimg = 1024;
        par->exts.resize(nimg);
        if (xseek(fmtHndl, BIM_MRC_HEADER_SIZE, SEEK_SET) != 0) return;
        if (xread(fmtHndl, &par->exts[0], 1, sz*nimg) != sz*nimg) return;
    }
}

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

int mrcValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_MRC_MAGIC_SIZE) return -1;
    MrcHeader *h = (MrcHeader *)magic; // header will be only guaranteed up to BIM_MRC_MAGIC_SIZE, disregard more advanced elements
    
    bim::uint32 mode = h->mode;
    bim::uint32 ver = h->nversion;
    char *ext = h->extType;

    if (bim::bigendian) {
        swapLong(&mode);
        swapLong(&ver);
    }

    if (mode == 6 || mode == 16 || mode == 101 || (mode >= 0 && mode <= 4)) return 0;

    if (memcmp(ext, "CCP4", 4) == 0) return 0; // Format from CCP4 suite
    if (memcmp(ext, "MRCO", 4) == 0) return 0; // MRC format
    if (memcmp(ext, "SERI", 4) == 0) return 0; // SerialEM
    if (memcmp(ext, "AGAR", 4) == 0) return 0; // Agard
    if (memcmp(ext, "FEI1", 4) == 0) return 0; // FEI software, e.g.EPU and Xplore3D, Amira, Avizo

    if (ver == 20140) return 0; // Year * 10 + version within the year(base 0)

    // handle bad cases of files written in big-endian format
    swapLong(&mode);
    if (mode == 6 || mode == 16 || mode == 101 || (mode >= 0 && mode <= 4)) return 0;

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

    bim::uint64 plane_size = ceil(info->width*info->height*info->samples*(info->depth/8.0));
    bim::uint64 page_offset = plane_size * page;

    if (xseek(fmtHndl, par->data_offset + page_offset, SEEK_SET) != 0) return 1;
    if (info->samples == 1) {
        if (xread(fmtHndl, img->bits[0], plane_size, 1) != 1) return 1;
    } else {
        std::vector<bim::uint8> buf(plane_size);
        if (xread(fmtHndl, &buf[0], plane_size, 1) != 1) return 1;

        for (int s = 0; s < info->samples; ++s) {
            if (info->depth == 8)
                copy_sample_interleaved_to_planar<bim::uint8>(info->width, info->height, info->samples, s, &buf[0], img->bits[s]);
            else if (info->depth == 16)
                copy_sample_interleaved_to_planar<bim::uint16>(info->width, info->height, info->samples, s, &buf[0], img->bits[s]);
            else if (info->depth == 32)
                copy_sample_interleaved_to_planar<bim::uint32>(info->width, info->height, info->samples, s, &buf[0], img->bits[s]);
        } // for sample
    }

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
    MrcHeader *h = &par->header;

    // resolution
    try {
        hash->append_tag("pixel_resolution_x", h->xlen/h->mx);
        hash->append_tag("pixel_resolution_y", h->ylen / h->my);
        hash->append_tag("pixel_resolution_z", h->zlen / h->mz);

        hash->append_tag("pixel_resolution_unit_x", "meters");
        hash->append_tag("pixel_resolution_unit_y", "meters");
        hash->append_tag("pixel_resolution_unit_z", "meters");
    } catch (...) {
        //std::cerr << "unknown excepition\n";
    }

    // all other metadata
    hash->append_tag("MRC/nxstart", h->nxstart);
    hash->append_tag("MRC/nystart", h->nystart);
    hash->append_tag("MRC/nzstart", h->nzstart);
    hash->append_tag("MRC/mx", h->mx);
    hash->append_tag("MRC/my", h->my);
    hash->append_tag("MRC/mz", h->mz);
    hash->append_tag("MRC/xlen", h->xlen);
    hash->append_tag("MRC/ylen", h->ylen);
    hash->append_tag("MRC/zlen", h->zlen);
    hash->append_tag("MRC/alpha", h->alpha);
    hash->append_tag("MRC/beta", h->beta);
    hash->append_tag("MRC/gamma", h->gamma);
    hash->append_tag("MRC/amin", h->amin);
    hash->append_tag("MRC/amax", h->amax);
    hash->append_tag("MRC/amean", h->amean);
    hash->append_tag("MRC/lens", h->lens);
    hash->append_tag("MRC/nd1", h->nd1);
    hash->append_tag("MRC/nd2", h->nd2);
    hash->append_tag("MRC/vd1", h->vd1);
    hash->append_tag("MRC/vd2", h->vd2);
    hash->append_tag("MRC/amin", h->amin);
    hash->append_tag("MRC/amin", h->amin);

    // add labels
    for (int i = 0; i < h->nlabl; ++i) {
        xstring s = h->label[i];
        if (s.size() > 0) {
            hash->append_tag(xstring::xprintf("MRC/label_%.2d", i), s.c_str());
        }
    }
    
    // extended header
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



