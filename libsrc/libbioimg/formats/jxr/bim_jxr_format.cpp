/*****************************************************************************
  JXR support
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
  2013-01-12 14:13:40 - First creation

  ver : 1
  *****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

#include <xtypes.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>

#include "bim_jxr_format.h"



using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::JXRParams::JXRParams() {
    i = initImageInfo();
    //reader = new gdcm::ImageReader();
}

bim::JXRParams::~JXRParams() {
    //if (reader) delete reader;
}

//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_JXR_MAGIC_SIZE 4

const unsigned char jxr_magic_number[4] = { 0x49, 0x49, 0xbc, 0x01 };

int jxrValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_JXR_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *)magic;
    if (memcmp(mag_num, jxr_magic_number, 4) == 0) return 0;
    return -1;
}

FormatHandle jxrAquireFormatProc(void) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void jxrCloseImageProc(FormatHandle *fmtHndl);
void jxrReleaseFormatProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    jxrCloseImageProc(fmtHndl);
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void jxrGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    *info = initImageInfo();
    /*

    gdcm::File & f = par->reader->GetFile();
    //gdcm::Image & img = par->reader->GetImage();

    const gdcm::DataSet& ds = f.GetDataSet();
    bool slice_thickness = find_tag(ds, 0x0018, 0x0050);
    bool slice_spacing = find_tag(ds, 0x0018, 0x0088);
    bool frame_time = find_tag(ds, 0x0018, 0x1063);
    bool num_temporal = find_tag(ds, 0x0020, 0x0105);
    bool number_frames = find_tag(ds, 0x0028, 0x0008);

    std::vector<unsigned int> dims = gdcm::ImageHelper::GetDimensionsValue(f);
    std::vector<double> spacing = gdcm::ImageHelper::GetSpacingValue(f);

    // some of this might have to be updated while decoding the image
    info->width = dims[0]; // img.GetColumns();
    info->height = dims[1]; // img.GetRows();
    unsigned int nd = dims.size(); //img.GetNumberOfDimensions();
    if (nd == 2) {
        info->number_z = 1;
        info->number_t = 1;
    }
    else if (nd == 3) {
        if (number_frames || num_temporal || frame_time) {
            info->number_z = 1;
            info->number_t = dims[2]; //img.GetDimension(2);
        }
        else {
            info->number_z = dims[2]; //img.GetDimension(2);
            info->number_t = 1;
        }
    }
    else if (nd >= 4) {
        info->number_z = dims[2]; //img.GetDimension(2);
        info->number_t = dims[3]; //img.GetDimension(3);
    }

    info->number_pages = info->number_z * info->number_t;

    // set XY scale
    info->resUnits = RES_mm;
    info->xRes = spacing[0]; //img.GetSpacing(0);
    info->yRes = spacing[1]; //img.GetSpacing(1);

    //---------------------------------------------------------------
    // define dims
    //---------------------------------------------------------------
    if (info->number_z > 1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_Z;
    }

    if (info->number_t > 1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_T;
    }

    if (info->number_z > 1 && info->number_t > 1) {
        info->number_dims = 5;
        info->dimensions[3].dim = DIM_Z;
        info->dimensions[4].dim = DIM_T;
    }

    //---------------------------------------------------------------
    // define pixels
    //---------------------------------------------------------------

    // set pixel depth and format
    //gdcm::PixelFormat pf = img.GetPixelFormat();
    gdcm::PixelFormat pf = gdcm::ImageHelper::GetPixelFormatValue(f);
    info->samples = pf.GetSamplesPerPixel();
    gdcm::PixelFormat::ScalarType st = pf.GetScalarType();

    if (st == gdcm::PixelFormat::UINT8) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::INT8) {
        info->depth = 8;
        info->pixelType = FMT_SIGNED;
    }
    else if (st == gdcm::PixelFormat::UINT12) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::INT12) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    }
    else if (st == gdcm::PixelFormat::UINT16) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::INT16) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    }
    else if (st == gdcm::PixelFormat::UINT32) {
        info->depth = 32;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::INT32) {
        info->depth = 32;
        info->pixelType = FMT_SIGNED;
    }
    else if (st == gdcm::PixelFormat::UINT64) {
        info->depth = 64;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::INT64) {
        info->depth = 64;
        info->pixelType = FMT_SIGNED;
    }
    else if (st == gdcm::PixelFormat::FLOAT16) {
        info->depth = 16;
        info->pixelType = FMT_FLOAT;
    }
    else if (st == gdcm::PixelFormat::FLOAT32) {
        info->depth = 32;
        info->pixelType = FMT_FLOAT;
    }
    else if (st == gdcm::PixelFormat::FLOAT64) {
        info->depth = 64;
        info->pixelType = FMT_FLOAT;
    }
    else if (st == gdcm::PixelFormat::SINGLEBIT) {
        info->depth = 1;
        info->pixelType = FMT_UNSIGNED;
    }
    else if (st == gdcm::PixelFormat::UNKNOWN) {
        //info->depth = 16;
        //info->pixelType = FMT_UNSIGNED;
    }

    //---------------------------------------------------------------
    // photometric mode
    //---------------------------------------------------------------

    info->imageMode = IM_GRAYSCALE;
    gdcm::PhotometricInterpretation photometric = gdcm::ImageHelper::GetPhotometricInterpretationValue(f);
    if (photometric == gdcm::PhotometricInterpretation::RGB) {
        info->imageMode = IM_RGB;
    }
    else if (photometric == gdcm::PhotometricInterpretation::ARGB) {
        info->imageMode = IM_RGBA;
    }
    else if (photometric == gdcm::PhotometricInterpretation::PALETTE_COLOR) {
        info->imageMode = IM_INDEXED;
    }

    */
}

void jxrCloseImageProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose(fmtHndl);
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint jxrOpenImageProc(FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) jxrCloseImageProc(fmtHndl);
    bim::JXRParams *par = new bim::JXRParams();
    fmtHndl->internalParams = (void *)par;

    if (io_mode == IO_READ) {
#if defined(BIM_WIN)
        bim::xstring fn(fmtHndl->fileName);
        //par->file = new std::fstream((const wchar_t *)fn.toUTF16().c_str(), std::fstream::in | std::fstream::binary);
        //par->reader->SetStream(*par->file);
#else
        par->reader->SetFileName(fmtHndl->fileName);
#endif       
        try {
            //par->reader->Read();
            //if (!par->reader->ReadInformation()) return 1;
            jxrGetImageInfo(fmtHndl);
        }
        catch (...) {
            jxrCloseImageProc(fmtHndl);
            return 1;
        }
    }
    else return 1;
    return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint jxrGetNumPagesProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    return info->number_pages;
}


ImageInfo jxrGetImageInfoProc(FormatHandle *fmtHndl, bim::uint page_num) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    fmtHndl->pageNumber = page_num;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out;
    raw += sample;
#pragma omp parallel for default(shared)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T *pp = p + x;
        T *rr = raw + x*samples;
        *pp = *rr;
    } // for x
}

bim::uint jxrReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;

    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    // allocate output image
    ImageBitmap *bmp = fmtHndl->image;
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;
    uint64 plane_sz = getImgSizeInBytes(bmp);
    /*
    gdcm::File & f = par->reader->GetFile();
    gdcm::PhotometricInterpretation photometric = gdcm::ImageHelper::GetPhotometricInterpretationValue(f);
    JXRParams::PlanarConfig planar_config = (JXRParams::PlanarConfig) gdcm::ImageHelper::GetPlanarConfigurationValue(f);
    gdcm::PixelFormat pf = gdcm::ImageHelper::GetPixelFormatValue(f);
    int pixelsize = pf.GetPixelSize();
    uint64 buffer_sz = info->width * info->height * pixelsize;

    if (buffer_sz != plane_sz*info->samples) return 1;

    std::vector<char> buffer(buffer_sz);
    gdcm::BoxRegion box;
    box.SetDomain(0, info->width - 1, 0, info->height - 1, page, page);
    par->reader->SetRegion(box);
    char *buf = (char *)&buffer[0];
    if (!par->reader->ReadIntoBuffer(buf, buffer_sz)) return 1;

    // simplest one channel case, read data directly into the image buffer
    if (info->samples == 1) {
        memcpy(bmp->bits[0], buf, plane_sz);
    }
    else if (info->samples == 3 && planar_config == JXRParams::RRRGGGBBB) {
        for (int s = 0; s < info->samples; ++s) {
            memcpy(bmp->bits[s], buf + plane_sz*s, plane_sz);
        }
    }
    else {
        // in multi-channel interleaved case read into appropriate channels
        for (int s = 0; s < info->samples; ++s) {
            if (bmp->i.depth == 8 && bmp->i.pixelType == FMT_UNSIGNED)
                copy_channel<uint8>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 16 && bmp->i.pixelType == FMT_UNSIGNED)
                copy_channel<uint16>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_UNSIGNED)
                copy_channel<uint32>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 64 && bmp->i.pixelType == FMT_UNSIGNED)
                copy_channel<uint64>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 8 && bmp->i.pixelType == FMT_SIGNED)
                copy_channel<int8>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 16 && bmp->i.pixelType == FMT_SIGNED)
                copy_channel<int16>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_SIGNED)
                copy_channel<int32>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 64 && bmp->i.pixelType == FMT_SIGNED)
                copy_channel<int64>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
                //if (bmp->i.depth == 16 && bmp->i.pixelType == FMT_FLOAT)
                //    copy_channel<float16>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
                //else
            if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_FLOAT)
                copy_channel<float32>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else
            if (bmp->i.depth == 64 && bmp->i.pixelType == FMT_FLOAT)
                copy_channel<float64>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
        } // for sample

    }
    */
    return 0;
}

bim::uint jxrWriteImageProc(FormatHandle *fmtHndl) {
    return 1;
    fmtHndl;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint jxr_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    if (!fmtHndl->fileName) return 1;

    // use EXIV2 to read metadata
    exiv_append_metadata(fmtHndl, hash);

    return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_JXR_NUM_FORMATS 1

FormatItem jxrItems[BIM_JXR_NUM_FORMATS] = {
    { //0
        "JXR",            // short name, no spaces
        "JPEG Extended Range (JPEG-XR)", // Long format name
        "jxr|hdp|wdp",   // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        1, //canWriteMeta; // 0 - NO, 1 - YES
        0, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 0, 0, 0, 0, 0 }
    }
};


FormatHeader jxrHeader = {

    sizeof(FormatHeader),
    "1.1.0",
    "JPEG-XR",
    "JPEG Extended Range (JPEG-XR)",

    BIM_FORMAT_JXR_MAGIC_SIZE,
    { 1, BIM_JXR_NUM_FORMATS, jxrItems },

    jxrValidateFormatProc,
    // begin
    jxrAquireFormatProc, //AquireFormatProc
    // end
    jxrReleaseFormatProc, //ReleaseFormatProc

    // params
    NULL, //AquireIntParamsProc
    NULL, //LoadFormatParamsProc
    NULL, //StoreFormatParamsProc

    // image begin
    jxrOpenImageProc, //OpenImageProc
    jxrCloseImageProc, //CloseImageProc 

    // info
    jxrGetNumPagesProc, //GetNumPagesProc
    jxrGetImageInfoProc, //GetImageInfoProc


    // read/write
    jxrReadImageProc, //ReadImageProc 
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
    jxr_append_metadata, //AppendMetaDataProc

    NULL,
    NULL,
    ""

};

extern "C" {

    FormatHeader* jxrGetFormatHeader(void)
    {
        return &jxrHeader;
    }

} // extern C


