/*****************************************************************************
  WEBP support
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

#include "bim_webp_format.h"

#include <webp/decode.h>
#include <webp/encode.h>

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::WEBPParams::WEBPParams() {
    i = initImageInfo();
    file = 0;
}

bim::WEBPParams::~WEBPParams() {
    if (file) delete file;
}

//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_WEBP_MAGIC_SIZE 13

int webpValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_WEBP_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *)magic;
    if (memcmp(mag_num+8, "WEBP", 4) == 0) return 0;
    return -1;
}

FormatHandle webpAquireFormatProc(void) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void webpCloseImageProc(FormatHandle *fmtHndl);
void webpReleaseFormatProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    webpCloseImageProc(fmtHndl);
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void webpSetWriteParameters(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    fmtHndl->order = 1; // use progressive encoding by default
    fmtHndl->quality = 95;

    if (!fmtHndl->options) return;
    xstring str = fmtHndl->options;
    std::vector<xstring> options = str.split(" ");
    if (options.size() < 1) return;

    int i = -1;
    while (i<(int)options.size() - 1) {
        i++;

        if (options[i] == "quality" && options.size() - i>0) {
            i++;
            fmtHndl->quality = options[i].toInt(100);
            continue;
        }
        else
        if (options[i] == "progressive" && options.size() - i>0) {
            i++;
            if (options[i] == "no") fmtHndl->order = 0;
            else
                fmtHndl->order = 1;
            continue;
        }

    } // while
}

void webpGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    *info = initImageInfo();

    const size_t bufsz = 40;
    std::vector<unsigned char> header(bufsz);
    par->file->seekg(0, std::ios::beg);
    par->file->read((char *) &header[0], bufsz);

    WebPBitstreamFeatures features;
    VP8StatusCode status = WebPGetFeatures((unsigned char *)&header[0], bufsz, &features);
    if (status != VP8_STATUS_OK) {
        throw;
    }

    info->width = features.width;
    info->height = features.height;
    if (features.has_alpha) {
        info->imageMode = IM_RGBA;
        info->samples = 4;
    } else {
        info->imageMode = IM_RGB;
        info->samples = 3;
    }
    info->number_z = 1;
    info->number_t = 1;
    info->number_pages = 1;
    info->number_dims = 2;
    info->depth = 8;
    info->pixelType = FMT_UNSIGNED;
}

void webpCloseImageProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose(fmtHndl);
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint webpOpenImageProc(FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) webpCloseImageProc(fmtHndl);
    bim::WEBPParams *par = new bim::WEBPParams();
    fmtHndl->internalParams = (void *)par;

    if (io_mode == IO_READ) {
#if defined(BIM_WIN)
        bim::xstring fn(fmtHndl->fileName);
        par->file = new std::fstream((const wchar_t *)fn.toUTF16().c_str(), std::fstream::in | std::fstream::binary);
#else
        par->file = new std::fstream(fmtHndl->fileName, std::fstream::in | std::fstream::binary);
#endif       
        try {
            webpGetImageInfo(fmtHndl);
        }
        catch (...) {
            webpCloseImageProc(fmtHndl);
            return 1;
        }
    } else if (io_mode == IO_WRITE) {
        webpSetWriteParameters(fmtHndl);
    }
    return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint webpGetNumPagesProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    return info->number_pages;
}


ImageInfo webpGetImageInfoProc(FormatHandle *fmtHndl, bim::uint page_num) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    fmtHndl->pageNumber = page_num;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out, int stride = 0) {
    T *raw = (T *)in + sample;
    T *p = (T *)out;
    int step = samples;
    size_t inrowsz = stride == 0 ? samples*W : stride;
    size_t ourowsz = W;

    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W > BIM_OMP_FOR2 && H > BIM_OMP_FOR2)
    for (bim::int64 y = 0; y < H; ++y) {
        T *lin = raw + y*inrowsz;
        T *lou = p + y*ourowsz;
        for (bim::int64 x = 0; x < W; ++x) {
            *lou = *lin;
            lou++;
            lin += step;
        } // for x
    } // for y
}

bim::uint webpReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;

    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    // allocate output image
    ImageBitmap *bmp = fmtHndl->image;
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;

    // load entire image in memory for webp to decode
    par->file->seekg(0, std::ios::end);
    size_t fsize = par->file->tellg();
    std::vector<unsigned char> buffer;
    buffer.resize(fsize);
    if (buffer.size() < fsize) return 1;
    par->file->seekg(0, std::ios::beg);
    par->file->read((char *) &buffer[0], fsize);

    // decode image
    uint8_t *webp_image = 0;
    int w=0, h=0;
    if (info->samples == 4)
        webp_image = WebPDecodeRGBA((const uint8_t *)&buffer[0], fsize, &w, &h);
    else
        webp_image = WebPDecodeRGB((const uint8_t *)&buffer[0], fsize, &w, &h);
    if (!webp_image) return 1;

    for (int s = 0; s < info->samples; ++s) {
        copy_channel<uint8>(info->width, info->height, info->samples, s, webp_image, bmp->bits[s]);
    } // for sample
    
    free(webp_image);
    return 0;
}

template <typename T>
void copy_from_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out + sample;
    int step = samples;
    size_t inrowsz = W;
    size_t ourowsz = samples*W;

    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W > BIM_OMP_FOR2 && H > BIM_OMP_FOR2)
    for (bim::int64 y = 0; y < H; ++y) {
        T *lin = raw + y*inrowsz;
        T *lou = p + y*ourowsz;
        for (bim::int64 x = 0; x < W; ++x) {
            *lou = *lin;
            lin++;
            lou += step;
        } // for x
    } // for y
}

bim::uint webpWriteImageProc(FormatHandle *fmtHndl) {
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &bmp->i;
    int out_samples = bim::trim<int>(info->samples, 3, 4);
    size_t fsize = getImgSizeInBytes(bmp) * out_samples;
    std::vector<unsigned char> buffer;
    buffer.resize(fsize);
    if (buffer.size() < fsize) return 1;
    memset(&buffer[0], 0, fsize);

    for (int s = 0; s<bim::min<int>(info->samples, 4); ++s) {
        copy_from_channel<uint8>(info->width, info->height, out_samples, s, bmp->bits[s], &buffer[0]);
    } // for sample

    // encode image
    float quality_factor = fmtHndl->quality;
    uint8_t *webp_image = 0;
    size_t imagesz;
    int stride = getLineSizeInBytes(bmp) * out_samples;
    if (out_samples == 4 && quality_factor < 100)
        imagesz = WebPEncodeRGBA((const uint8_t *)&buffer[0], info->width, info->height, stride, quality_factor, &webp_image);
    else if (out_samples == 4 && quality_factor >= 100)
        imagesz = WebPEncodeLosslessRGBA((const uint8_t *)&buffer[0], info->width, info->height, stride, &webp_image);
    if (out_samples < 4 && quality_factor < 100)
        imagesz = WebPEncodeRGB((const uint8_t *)&buffer[0], info->width, info->height, stride, quality_factor, &webp_image);
    else
        imagesz = WebPEncodeLosslessRGB((const uint8_t *)&buffer[0], info->width, info->height, stride, &webp_image);

    // write image to a file
    #if defined(BIM_WIN)
    bim::xstring fn(fmtHndl->fileName);
    std::fstream file((const wchar_t *)fn.toUTF16().c_str(), std::fstream::out | std::fstream::binary);
    #else
    std::fstream file(fmtHndl->fileName, std::fstream::out | std::fstream::binary);
    #endif   
    file.write((char *)webp_image, imagesz);
    file.close();

    free(webp_image);
    return 0;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint webp_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    if (!fmtHndl->fileName) return 1;

    // use EXIV2 to read metadata
    //exiv_append_metadata(fmtHndl, hash);

    return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_WEBP_NUM_FORMATS 1

FormatItem webpItems[BIM_WEBP_NUM_FORMATS] = {
    { //0
        "WEBP",            // short name, no spaces
        "WebP", // Long format name
        "webp",   // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        1, //canWriteMeta; // 0 - NO, 1 - YES
        0, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 16383, 16383, 1, 3, 4, 8, 8, 1 }
    }
};

FormatHeader webpHeader = {
    sizeof(FormatHeader),
    "0.4.3",
    "WebP",
    "WebP",

    BIM_FORMAT_WEBP_MAGIC_SIZE,
    { 1, BIM_WEBP_NUM_FORMATS, webpItems },

    webpValidateFormatProc,
    // begin
    webpAquireFormatProc, //AquireFormatProc
    // end
    webpReleaseFormatProc, //ReleaseFormatProc

    // params
    NULL, //AquireIntParamsProc
    NULL, //LoadFormatParamsProc
    NULL, //StoreFormatParamsProc

    // image begin
    webpOpenImageProc, //OpenImageProc
    webpCloseImageProc, //CloseImageProc 

    // info
    webpGetNumPagesProc, //GetNumPagesProc
    webpGetImageInfoProc, //GetImageInfoProc


    // read/write
    webpReadImageProc, //ReadImageProc 
    webpWriteImageProc, //WriteImageProc
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
    webp_append_metadata, //AppendMetaDataProc

    NULL,
    NULL,
    ""

};

extern "C" {

    FormatHeader* webpGetFormatHeader(void)
    {
        return &webpHeader;
    }

} // extern C


