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

#include <JXRGlue.h>

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

// JXR I/O wrappers

static ERR _jxr_io_Read(WMPStream* pWS, void* pv, size_t cb) {
    bim::JXRParams *par = (bim::JXRParams*) pWS->state.pvObj;
    par->file->read((char*) pv, cb);
    return par->file->gcount() == cb ? WMP_errSuccess : WMP_errFileIO;
}

static ERR _jxr_io_Write(WMPStream* pWS, const void* pv, size_t cb) {
    bim::JXRParams *par = (bim::JXRParams*) pWS->state.pvObj;
    if (cb != 0) {
        par->file->write((char*)pv, cb);
        return (par->file->rdstate() & std::ifstream::badbit) == 0 ? WMP_errSuccess : WMP_errFileIO;
    }
    return WMP_errFileIO;
}

static ERR _jxr_io_SetPos(WMPStream* pWS, size_t offPos) {
    bim::JXRParams *par = (bim::JXRParams*) pWS->state.pvObj;
    par->file->seekg(offPos, std::fstream::beg);
    return (par->file->rdstate() & std::ifstream::badbit) == 0 ? WMP_errSuccess : WMP_errFileIO;
}

static ERR _jxr_io_GetPos(WMPStream* pWS, size_t* poffPos) {
    bim::JXRParams *par = (bim::JXRParams*) pWS->state.pvObj;
    int offset = par->file->tellg();
    if (offset == -1) return WMP_errFileIO;
    *poffPos = (size_t)offset;
    return WMP_errSuccess;
}

static Bool _jxr_io_EOS(WMPStream* pWS) {
    bim::JXRParams *par = (bim::JXRParams*) pWS->state.pvObj;
    return par->file->eof();
}

static ERR _jxr_io_Close(WMPStream** ppWS) {
    // do nothing, we'll free pStream in our close
    /*WMPStream *pWS = *ppWS;
    if (pWS) {
        free(pWS);
        *ppWS = NULL;
    }*/
    return WMP_errSuccess;
}

// JXR parameters

bim::JXRParams::JXRParams() {
    i = initImageInfo();
    pStream = NULL;
    pDecoder = NULL;
}

bim::JXRParams::~JXRParams() {
    if (pDecoder) pDecoder->Release(&pDecoder);
    if (pStream) free(pStream);
    if (file) delete file;
}

void bim::JXRParams::open(const char *filename, bim::ImageIOModes io_mode) {
    std::ios_base::openmode mode = io_mode == IO_READ ? std::fstream::in : std::fstream::out;
    #if defined(BIM_WIN)
    bim::xstring fn(filename);
    this->file = new std::fstream((const wchar_t *)fn.toUTF16().c_str(), mode | std::fstream::binary);
    #else
    this->file = new std::fstream(filename, mode | std::fstream::binary);
    #endif

    this->pStream = (WMPStream*)calloc(1, sizeof(WMPStream));
    if (this->pStream) {
        this->pStream->state.pvObj = this;
        this->pStream->Close = _jxr_io_Close;
        this->pStream->EOS = _jxr_io_EOS;
        this->pStream->Read = _jxr_io_Read;
        this->pStream->Write = _jxr_io_Write;
        this->pStream->SetPos = _jxr_io_SetPos;
        this->pStream->GetPos = _jxr_io_GetPos;
    }
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


void jxrSetWriteParameters(FormatHandle *fmtHndl) {
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

#define JXR_CHECK(error_code) \
if(error_code < 0) { \
    throw error_code; \
}

void jxrGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    *info = initImageInfo();
    ERR error_code = 0;

    // create a JXR decoder
    if (!par->pDecoder) {
        error_code = PKImageDecode_Create_WMP(&par->pDecoder);
        JXR_CHECK(error_code);

        error_code = par->pDecoder->Initialize(par->pDecoder, par->pStream);
        JXR_CHECK(error_code);
    }

    // set decoder parameters
    par->pDecoder->WMP.wmiSCP.uAlphaMode = 2;

    // get input file pixel format
    PKPixelInfo pixelInfo;
    PKPixelFormatGUID pguidSourcePF;
    error_code = par->pDecoder->GetPixelFormat(par->pDecoder, &pguidSourcePF);
    JXR_CHECK(error_code);
    pixelInfo.pGUIDPixFmt = &pguidSourcePF;
    
    error_code = PixelFormatLookup(&pixelInfo, LOOKUP_FORWARD);
    JXR_CHECK(error_code);

    if (pixelInfo.bdBitDepth == BD_1) {
        info->depth = 1;
        info->pixelType = FMT_UNSIGNED;
    } else if (pixelInfo.bdBitDepth == BD_8) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
    } else if (pixelInfo.bdBitDepth == BD_16) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    } else if (pixelInfo.bdBitDepth == BD_16S) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    } else if (pixelInfo.bdBitDepth == BD_16F) {
        info->depth = 16;
        info->pixelType = FMT_FLOAT;
    } else if (pixelInfo.bdBitDepth == BD_32) {
        info->depth = 32;
        info->pixelType = FMT_UNSIGNED;
    } else if (pixelInfo.bdBitDepth == BD_32S) {
        info->depth = 32;
        info->pixelType = FMT_SIGNED;
    } else if (pixelInfo.bdBitDepth == BD_32F) {
        info->depth = 32;
        info->pixelType = FMT_FLOAT;
    }

    info->imageMode = IM_MULTI;
    info->samples = pixelInfo.cChannel;

    // we need to deal with YUV_420 and YUV_422 and other non standard by conversion during decode
    if (pixelInfo.cfColorFormat == Y_ONLY) {
        info->imageMode = IM_GRAYSCALE;
    } else if (pixelInfo.cfColorFormat == CF_RGB) {
        info->imageMode = IM_RGB;
    } else if (pixelInfo.cfColorFormat == CF_RGBE) {
        info->imageMode = IM_RGBE;
    } else if (pixelInfo.cfColorFormat == YUV_444) {
        info->imageMode = IM_RGB;
    }

    // get image dimensions
    I32 width, height;	// image dimensions (in pixels)
    error_code = par->pDecoder->GetSize(par->pDecoder, &width, &height);
    JXR_CHECK(error_code);
    info->width = width;
    info->height = height;

    float resX, resY;	// image resolution (in dots per inch)
    par->pDecoder->GetResolution(par->pDecoder, &resX, &resY);
    info->resUnits = RES_IN;
    info->xRes = resX;
    info->yRes = resY;

    U32 frames;
    error_code = par->pDecoder->GetFrameCount(par->pDecoder, &frames);
    JXR_CHECK(error_code);

    info->number_z = 1;
    info->number_t = frames;
    info->number_pages = frames;
    if (frames>1)
        info->number_dims = 3;
    else
        info->number_dims = 2;
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

    par->open(fmtHndl->fileName, io_mode);
    try {
        if (io_mode == IO_READ) {
            jxrGetImageInfo(fmtHndl);
        }
        else if (io_mode == IO_WRITE) {
            jxrSetWriteParameters(fmtHndl);
        }
    } catch (...) {
        jxrCloseImageProc(fmtHndl);
        return 1;
    }

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
// READ
//----------------------------------------------------------------------------

bool getOutputPixelFormat(const PKPixelFormatGUID &guid_format_in, PKPixelFormatGUID &guid_format_out, ImageInfo *info) {

    if (guid_format_in == GUID_PKPixelFormat16bppRGB555 ||
        guid_format_in == GUID_PKPixelFormat16bppRGB565 ||
        guid_format_in == GUID_PKPixelFormat24bppBGR ||
        guid_format_in == GUID_PKPixelFormat32bppBGR ||
        guid_format_in == GUID_PKPixelFormat12bppYCC420 ||
        guid_format_in == GUID_PKPixelFormat16bppYCC422 ||
        guid_format_in == GUID_PKPixelFormat20bppYCC422 ||
        guid_format_in == GUID_PKPixelFormat32bppYCC422 ) 
    {
        guid_format_out = GUID_PKPixelFormat24bppRGB;
        info->imageMode = IM_RGB;
        info->depth = 8;
        info->samples = 3;
        info->pixelType = FMT_UNSIGNED;
        return true;
    } else 
    if (guid_format_in == GUID_PKPixelFormat32bppBGRA ||
        guid_format_in == GUID_PKPixelFormat32bppPBGRA ||
        guid_format_in == GUID_PKPixelFormat32bppPRGBA ||
        guid_format_in == GUID_PKPixelFormat64bppPRGBA ||
        guid_format_in == GUID_PKPixelFormat20bppYCC420Alpha ||
        guid_format_in == GUID_PKPixelFormat24bppYCC422Alpha ||
        guid_format_in == GUID_PKPixelFormat30bppYCC422Alpha ||
        guid_format_in == GUID_PKPixelFormat48bppYCC422Alpha)
    {
        guid_format_out = GUID_PKPixelFormat32bppRGBA;
        info->imageMode = IM_RGBA;
        info->depth = 8;
        info->samples = 4;
        info->pixelType = FMT_UNSIGNED;
        return true;
    } else
    if (guid_format_in == GUID_PKPixelFormat16bppGrayFixedPoint ||
        guid_format_in == GUID_PKPixelFormat16bppGrayHalf ||
        guid_format_in == GUID_PKPixelFormat32bppGrayFixedPoint ) 
    {
        guid_format_out = GUID_PKPixelFormat32bppGrayFloat;
        info->imageMode = IM_GRAYSCALE;
        info->depth = 32;
        info->samples = 1;
        info->pixelType = FMT_FLOAT;
        return true;
    } else
    if (guid_format_in == GUID_PKPixelFormat48bppRGBFixedPoint ||
        guid_format_in == GUID_PKPixelFormat32bppRGB101010 ||
        guid_format_in == GUID_PKPixelFormat96bppRGBFixedPoint ||
        guid_format_in == GUID_PKPixelFormat64bppRGBFixedPoint ||
        guid_format_in == GUID_PKPixelFormat128bppRGBFixedPoint ||
        guid_format_in == GUID_PKPixelFormat64bppRGBHalf ||
        guid_format_in == GUID_PKPixelFormat48bppRGBHalf ||
        guid_format_in == GUID_PKPixelFormat32bppRGBE ||
        guid_format_in == GUID_PKPixelFormat16bpp48bppYCC444FixedPoint )
    {
        guid_format_out = GUID_PKPixelFormat96bppRGBFloat;
        info->imageMode = IM_RGB;
        info->depth = 32;
        info->samples = 3;
        info->pixelType = FMT_FLOAT;
        return true;
    } else 
    if (guid_format_in == GUID_PKPixelFormat128bppPRGBAFloat ||
        guid_format_in == GUID_PKPixelFormat64bppRGBAFixedPoint ||
        guid_format_in == GUID_PKPixelFormat128bppRGBAFixedPoint ||
        guid_format_in == GUID_PKPixelFormat64bppRGBAHalf ||
        guid_format_in == GUID_PKPixelFormat64bppYCC444AlphaFixedPoint )
    {
        guid_format_out = GUID_PKPixelFormat128bppRGBAFloat;
        info->imageMode = IM_RGBA;
        info->depth = 32;
        info->samples = 4;
        info->pixelType = FMT_FLOAT;
        return true;
    }

    return false;
}

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out, int stride = 0 ) {
    T *raw = (T *)in + sample;
    T *p = (T *)out;
    int step = sizeof(T)*samples;
    size_t inrowsz = stride == 0 ? sizeof(T)*samples*W : stride;
    size_t ourowsz = sizeof(T)*W;

    #pragma omp parallel for default(shared)
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

bim::uint jxrReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ERR error_code = 0;

    error_code = par->pDecoder->SelectFrame(par->pDecoder, page);
    JXR_CHECK(error_code);
    jxrGetImageInfo(fmtHndl);

    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &par->i;

    PKFormatConverter *pConverter = NULL;
    unsigned char *pb = NULL;
    const PKRect rect = { 0, 0, info->width, info->height };

    try {
        PKPixelFormatGUID guid_format_in;
        error_code = par->pDecoder->GetPixelFormat(par->pDecoder, &guid_format_in);
        JXR_CHECK(error_code);

        PKPixelFormatGUID guid_format_out;
        bool needs_conversion = getOutputPixelFormat(guid_format_in, guid_format_out, info);

        // allocate output image
        if (allocImg(fmtHndl, info, bmp) != 0) return 1;
        const unsigned stride = getLineSizeInBytes(bmp) * info->samples;

        if (!needs_conversion && info->samples == 1) {
            error_code = par->pDecoder->Copy(par->pDecoder, &rect, (U8*) bmp->bits[0], stride);
            JXR_CHECK(error_code);
        } else if (!needs_conversion) {
            bim::uint64 buf_sz = getImgSizeInBytes(bmp) * info->samples;
            std::vector<unsigned char> buffer(buf_sz, 0);
            error_code = par->pDecoder->Copy(par->pDecoder, &rect, (U8*)&buffer[0], stride);
            JXR_CHECK(error_code);
            for (int s = 0; s < info->samples; ++s) {
                if (info->depth == 8)
                    copy_channel<bim::uint8>(info->width, info->height, info->samples, s, &buffer[0], bmp->bits[s]);
                else if (info->depth == 16)
                    copy_channel<bim::uint16>(info->width, info->height, info->samples, s, &buffer[0], bmp->bits[s]);
                else if (info->depth == 32)
                    copy_channel<bim::uint32>(info->width, info->height, info->samples, s, &buffer[0], bmp->bits[s]);
            } // for sample
        } else { // we need to use the conversion API for complex types
            // allocate the pixel format converter
            error_code = PKCodecFactory_CreateFormatConverter(&pConverter);
            JXR_CHECK(error_code);

            // set the conversion function
            error_code = pConverter->Initialize(pConverter, par->pDecoder, NULL, guid_format_out);
            JXR_CHECK(error_code);

            // get the maximum stride
            unsigned stride = 0;
            {
                PKPixelInfo pPIFrom;
                PKPixelInfo pPITo;

                pPIFrom.pGUIDPixFmt = &guid_format_in;
                error_code = PixelFormatLookup(&pPIFrom, LOOKUP_FORWARD);
                JXR_CHECK(error_code);

                pPITo.pGUIDPixFmt = &guid_format_out;
                error_code = PixelFormatLookup(&pPITo, LOOKUP_FORWARD);
                JXR_CHECK(error_code);

                unsigned int stride_from = ((pPIFrom.cbitUnit + 7) >> 3) * info->width;
                unsigned int stride_to = ((pPITo.cbitUnit + 7) >> 3) * info->width;
                stride = bim::max<unsigned int>(stride_from, stride_to);
            }

            // allocate a local decoder / encoder buffer
            error_code = PKAllocAligned((void **)&pb, stride * info->height, 128);
            JXR_CHECK(error_code);

            // copy / convert pixels
            error_code = pConverter->Copy(pConverter, &rect, pb, stride);
            JXR_CHECK(error_code);

            for (int s = 0; s < info->samples; ++s) {
                if (info->depth == 8)
                    copy_channel<bim::uint8>(info->width, info->height, info->samples, s, pb, bmp->bits[s], stride);
                else if (info->depth == 16)
                    copy_channel<bim::uint16>(info->width, info->height, info->samples, s, pb, bmp->bits[s], stride);
                else if (info->depth == 32)
                    copy_channel<bim::uint32>(info->width, info->height, info->samples, s, pb, bmp->bits[s], stride);
            } // for sample

            PKFreeAligned((void **)&pb);
            PKFormatConverter_Release(&pConverter);
        }

    } catch (...) {
        PKFreeAligned((void **)&pb);
        if (pConverter) PKFormatConverter_Release(&pConverter);
        return 1;
    }
    return 0;
}

//----------------------------------------------------------------------------
// WRITE
//----------------------------------------------------------------------------

// Y, U, V, YHP, UHP, VHP
int DPK_QPS_420[12][6] = {      // for 8 bit only
    { 66, 65, 70, 72, 72, 77 },
    { 59, 58, 63, 64, 63, 68 },
    { 52, 51, 57, 56, 56, 61 },
    { 48, 48, 54, 51, 50, 55 },
    { 43, 44, 48, 46, 46, 49 },
    { 37, 37, 42, 38, 38, 43 },
    { 26, 28, 31, 27, 28, 31 },
    { 16, 17, 22, 16, 17, 21 },
    { 10, 11, 13, 10, 10, 13 },
    { 5, 5, 6, 5, 5, 6 },
    { 2, 2, 3, 2, 2, 2 }
};

int DPK_QPS_8[12][6] = {
    { 67, 79, 86, 72, 90, 98 },
    { 59, 74, 80, 64, 83, 89 },
    { 53, 68, 75, 57, 76, 83 },
    { 49, 64, 71, 53, 70, 77 },
    { 45, 60, 67, 48, 67, 74 },
    { 40, 56, 62, 42, 59, 66 },
    { 33, 49, 55, 35, 51, 58 },
    { 27, 44, 49, 28, 45, 50 },
    { 20, 36, 42, 20, 38, 44 },
    { 13, 27, 34, 13, 28, 34 },
    { 7, 17, 21, 8, 17, 21 }, // Photoshop 100%
    { 2, 5, 6, 2, 5, 6 }
};

int DPK_QPS_16[11][6] = {
    { 197, 203, 210, 202, 207, 213 },
    { 174, 188, 193, 180, 189, 196 },
    { 152, 167, 173, 156, 169, 174 },
    { 135, 152, 157, 137, 153, 158 },
    { 119, 137, 141, 119, 138, 142 },
    { 102, 120, 125, 100, 120, 124 },
    { 82, 98, 104, 79, 98, 103 },
    { 60, 76, 81, 58, 76, 81 },
    { 39, 52, 58, 36, 52, 58 },
    { 16, 27, 33, 14, 27, 33 },
    { 5, 8, 9, 4, 7, 8 }
};

int DPK_QPS_16f[11][6] = {
    { 148, 177, 171, 165, 187, 191 },
    { 133, 155, 153, 147, 172, 181 },
    { 114, 133, 138, 130, 157, 167 },
    { 97, 118, 120, 109, 137, 144 },
    { 76, 98, 103, 85, 115, 121 },
    { 63, 86, 91, 62, 96, 99 },
    { 46, 68, 71, 43, 73, 75 },
    { 29, 48, 52, 27, 48, 51 },
    { 16, 30, 35, 14, 29, 34 },
    { 8, 14, 17, 7, 13, 17 },
    { 3, 5, 7, 3, 5, 6 }
};

int DPK_QPS_32f[11][6] = {
    { 194, 206, 209, 204, 211, 217 },
    { 175, 187, 196, 186, 193, 205 },
    { 157, 170, 177, 167, 180, 190 },
    { 133, 152, 156, 144, 163, 168 },
    { 116, 138, 142, 117, 143, 148 },
    { 98, 120, 123, 96, 123, 126 },
    { 80, 99, 102, 78, 99, 102 },
    { 65, 79, 84, 63, 79, 84 },
    { 48, 61, 67, 45, 60, 66 },
    { 27, 41, 46, 24, 40, 45 },
    { 3, 22, 24, 2, 21, 22 }
};

static void WriteTag(TagMap *hash, const std::string &key, DPKPROPVARIANT & varDst, DPKVARTYPE vt = DPKVT_EMPTY) {
    varDst.vt = vt;
    switch (vt) {
    case DPKVT_LPSTR:
        varDst.VT.pszVal = (char *) hash->get_value(key).c_str();
        break;
    case DPKVT_UI2:
        varDst.VT.uiVal = hash->get_value_int(key, 0);
        break;
    case DPKVT_UI4:
        varDst.VT.ulVal = hash->get_value_int(key, 0);
        break;
    default:
        break;
    }
}

static ERR WriteDescriptiveMetadata(PKImageEncode *pEncoder, TagMap *hash) {
    ERR error_code = 0;		// error code as returned by the interface
    DESCRIPTIVEMETADATA DescMetadata;

    // fill the DESCRIPTIVEMETADATA structure (use pointers to arrays when needed)
    WriteTag(hash, "Exif/Image/ImageDescription", DescMetadata.pvarImageDescription, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/Make", DescMetadata.pvarCameraMake, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/Model", DescMetadata.pvarCameraModel, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/Software", DescMetadata.pvarSoftware, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/DateTime", DescMetadata.pvarDateTime, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/Artist", DescMetadata.pvarArtist, DPKVT_LPSTR);
    WriteTag(hash, "Exif/Image/Copyright", DescMetadata.pvarCopyright, DPKVT_LPSTR);
    //WriteTag(hash, "Exif/Image/Rating", DescMetadata.pvarRatingStars);
    //WriteTag(hash, "Exif/Image/Rating", DescMetadata.pvarRatingValue);
    WriteTag(hash, "Iptc/Application2/Caption", DescMetadata.pvarCaption, DPKVT_LPSTR);
    //WriteTag(hash, "EXIF/", DescMetadata.pvarDocumentName, DPKVT_LPSTR);
    //WriteTag(hash, "EXIF/", DescMetadata.pvarPageName, DPKVT_LPSTR);
    //WriteTag(hash, WMP_tagPageNumber, DescMetadata.pvarPageNumber);
    //WriteTag(hash, "EXIF/", DescMetadata.pvarHostComputer, DPKVT_LPSTR);

    error_code = pEncoder->SetDescriptiveMetadata(pEncoder, &DescMetadata);
    return error_code;
}

static ERR WriteMetadata(PKImageEncode *pEncoder, ImageInfo *info, TagMap *hash) {
    ERR error_code = 0;
    char *profile = NULL;
    unsigned profile_size = 0;

    if (info->resUnits == RES_IN)
        pEncoder->SetResolution(pEncoder, info->xRes, info->yRes);

    try {
        // write descriptive metadata
        error_code = WriteDescriptiveMetadata(pEncoder, hash);
        JXR_CHECK(error_code);

        // write ICC profile
        /*
        FIICCPROFILE *iccProfile = FreeImage_GetICCProfile(dib);
        if (iccProfile->data) {
            error_code = pIE->SetColorContext(pIE, (U8*)iccProfile->data, iccProfile->size);
            JXR_CHECK(error_code);
        }

        // write IPTC metadata
        if (FreeImage_GetMetadataCount(FIMD_IPTC, dib)) {
            // create a binary profile
            if (write_iptc_profile(dib, &profile, &profile_size)) {
                // write the profile
                error_code = PKImageEncode_SetIPTCNAAMetadata_WMP(pIE, profile, profile_size);
                JXR_CHECK(error_code);
                // release profile
                free(profile);
                profile = NULL;
            }
        }

        // write XMP metadata
        {
            FITAG *tag_xmp = NULL;
            if (FreeImage_GetMetadata(FIMD_XMP, dib, g_TagLib_XMPFieldName, &tag_xmp)) {
                error_code = PKImageEncode_SetXMPMetadata_WMP(pIE, (BYTE*)FreeImage_GetTagValue(tag_xmp), FreeImage_GetTagLength(tag_xmp));
                JXR_CHECK(error_code);
            }
        }

        // write Exif metadata
        {
            if (tiff_get_ifd_profile(dib, FIMD_EXIF_EXIF, &profile, &profile_size)) {
                error_code = PKImageEncode_SetEXIFMetadata_WMP(pIE, profile, profile_size);
                JXR_CHECK(error_code);
                // release profile
                free(profile);
                profile = NULL;
            }
        }

        // write Exif GPS metadata
        {
            if (tiff_get_ifd_profile(dib, FIMD_EXIF_GPS, &profile, &profile_size)) {
                error_code = PKImageEncode_SetGPSInfoMetadata_WMP(pIE, profile, profile_size);
                JXR_CHECK(error_code);
                // release profile
                free(profile);
                profile = NULL;
            }
        }
        */

        return WMP_errSuccess;
    } catch (...) {
        free(profile);
        return error_code;
    }
}

template <typename T>
void copy_from_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out + sample;
    int step = sizeof(T)*samples;
    size_t inrowsz = sizeof(T)*W;
    size_t ourowsz = sizeof(T)*samples*W;

    #pragma omp parallel for default(shared)
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

PKPixelFormatGUID initGUIDPixelFormat(ImageInfo *info) {
    PKPixelFormatGUID guid_format = GUID_PKPixelFormatDontCare;
    if (info->samples == 1 && info->depth == 1 && info->pixelType == FMT_UNSIGNED) {
        guid_format = GUID_PKPixelFormatBlackWhite;
    }
    else if (info->samples == 1 && info->depth == 8 && info->pixelType == FMT_UNSIGNED) {
        guid_format = GUID_PKPixelFormat8bppGray;
    }
    else if (info->samples == 1 && info->depth == 16 && info->pixelType == FMT_UNSIGNED) {
        guid_format = GUID_PKPixelFormat16bppGray;
    }
    else if (info->samples == 1 && info->depth == 32 && info->pixelType == FMT_FLOAT) {
        guid_format = GUID_PKPixelFormat32bppGrayFloat;
    }
    else if (info->samples == 3 && info->depth == 8 && info->pixelType == FMT_UNSIGNED && info->imageMode == IM_RGB) {
        guid_format = GUID_PKPixelFormat24bppRGB;
    }
    else if (info->samples == 4 && info->depth == 8 && info->pixelType == FMT_UNSIGNED && info->imageMode == IM_RGBA) {
        guid_format = GUID_PKPixelFormat32bppRGBA;
    }
    else if (info->samples == 3 && info->depth == 16 && info->pixelType == FMT_UNSIGNED && info->imageMode == IM_RGB) {
        guid_format = GUID_PKPixelFormat48bppRGB;
    }
    else if (info->samples == 4 && info->depth == 16 && info->pixelType == FMT_UNSIGNED && info->imageMode == IM_RGBA) {
        guid_format = GUID_PKPixelFormat64bppRGBA;
    }
    else if (info->samples == 3 && info->depth == 32 && info->pixelType == FMT_FLOAT && info->imageMode == IM_RGB) {
        guid_format = GUID_PKPixelFormat96bppRGBFloat;
    }
    else if (info->samples == 4 && info->depth == 32 && info->pixelType == FMT_FLOAT && info->imageMode == IM_RGBA) {
        guid_format = GUID_PKPixelFormat128bppRGBAFloat;
    }
    else if (info->samples == 3 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat24bpp3Channels;
    }
    else if (info->samples == 4 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat32bpp4Channels;
    }
    else if (info->samples == 5 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat40bpp5Channels;
    }
    else if (info->samples == 6 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat48bpp6Channels;
    }
    else if (info->samples == 7 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat56bpp7Channels;
    }
    else if (info->samples == 8 && info->depth == 8 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat64bpp8Channels;
    }
    else if (info->samples == 3 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat48bpp3Channels;
    }
    else if (info->samples == 4 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat64bpp4Channels;
    }
    else if (info->samples == 5 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat80bpp5Channels;
    }
    else if (info->samples == 6 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat96bpp6Channels;
    }
    else if (info->samples == 7 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat112bpp7Channels;
    }
    else if (info->samples == 8 && info->depth == 16 && info->imageMode == IM_MULTI) {
        guid_format = GUID_PKPixelFormat128bpp8Channels;
    }
    return guid_format;
}

/*
ImageQuality  Q (BD==1)  Q (BD==8)   Q (BD==16)  Q (BD==32F) Subsample   Overlap
[0.0, 0.4]    8-IQ*5     (see table) (see table) (see table) 4:4:4       2
(0.4, 0.8)    8-IQ*5     (see table) (see table) (see table) 4:4:4       1
[0.8, 1.0)    8-IQ*5     (see table) (see table) (see table) 4:4:4       1
[1.0, 1.0]    1          1           1           1           4:4:4       0
*/

static void SetEncoderParameters(CWMIStrCodecParam *wmiSCP, const PKPixelInfo *pixelInfo, FormatHandle *fmtHndl, ImageInfo *info) {
    wmiSCP->cfColorFormat = YUV_444;		// color format
    wmiSCP->bdBitDepth = BD_LONG;			// internal bit depth
    wmiSCP->bfBitstreamFormat = SPATIAL;	// compressed image data in spatial order
    wmiSCP->bProgressiveMode = FALSE;		// sequential mode
    wmiSCP->olOverlap = OL_ONE;				// single level overlap processing 
    wmiSCP->cNumOfSliceMinus1H = 0;			// # of horizontal slices
    wmiSCP->cNumOfSliceMinus1V = 0;			// # of vertical slices
    wmiSCP->sbSubband = SB_ALL;				// keep all subbands
    wmiSCP->uAlphaMode = 0;					// 0:no alpha 1: alpha only else: something + alpha 
    wmiSCP->uiDefaultQPIndex = 1;			// quantization for grey or rgb layer(s), 1: lossless
    wmiSCP->uiDefaultQPIndexAlpha = 1;		// quantization for alpha layer, 1: lossless

    if (fmtHndl->order == 1)
        wmiSCP->bProgressiveMode = TRUE; // progressive mode

    float fltImageQuality = fmtHndl->quality / 100.0;
    
    // set quantization same way as JxrEncApp
    if (fmtHndl->quality == 100) {
        wmiSCP->uiDefaultQPIndex = 1;
    } else {
        // overlap
        if (fltImageQuality >= 0.5F)
            wmiSCP->olOverlap = OL_ONE;
        else
            wmiSCP->olOverlap = OL_TWO;

        // chroma sub-sampling
        if (fltImageQuality >= 0.5F || pixelInfo->uBitsPerSample > 8)
            wmiSCP->cfColorFormat = YUV_444;
        else
            wmiSCP->cfColorFormat = YUV_420;

        // bit depth
        if (pixelInfo->bdBitDepth == BD_1) {
            wmiSCP->uiDefaultQPIndex = (U8)(8 - 5.0F * fltImageQuality + 0.5F);
        } else {
            // remap [0.8, 0.866, 0.933, 1.0] to [0.8, 0.9, 1.0, 1.1]
            // to use 8-bit DPK QP table (0.933 == Photoshop JPEG 100)
            if (fltImageQuality > 0.8F && pixelInfo->bdBitDepth == BD_8 && wmiSCP->cfColorFormat != YUV_420 && wmiSCP->cfColorFormat != YUV_422) {
                fltImageQuality = 0.8F + (fltImageQuality - 0.8F) * 1.5F;
            }

            const int qi = (int)(10.0F * fltImageQuality);
            const float qf = 10.0F * fltImageQuality - (float)qi;

            const int *pQPs =
                (wmiSCP->cfColorFormat == YUV_420 || wmiSCP->cfColorFormat == YUV_422) ?
                DPK_QPS_420[qi] :
                (pixelInfo->bdBitDepth == BD_8 ? DPK_QPS_8[qi] :
                (pixelInfo->bdBitDepth == BD_16 ? DPK_QPS_16[qi] :
                (pixelInfo->bdBitDepth == BD_16F ? DPK_QPS_16f[qi] :
                DPK_QPS_32f[qi])));

            wmiSCP->uiDefaultQPIndex = (U8)(0.5F + (float)pQPs[0] * (1.0F - qf) + (float)(pQPs + 6)[0] * qf);
            wmiSCP->uiDefaultQPIndexU = (U8)(0.5F + (float)pQPs[1] * (1.0F - qf) + (float)(pQPs + 6)[1] * qf);
            wmiSCP->uiDefaultQPIndexV = (U8)(0.5F + (float)pQPs[2] * (1.0F - qf) + (float)(pQPs + 6)[2] * qf);
            wmiSCP->uiDefaultQPIndexYHP = (U8)(0.5F + (float)pQPs[3] * (1.0F - qf) + (float)(pQPs + 6)[3] * qf);
            wmiSCP->uiDefaultQPIndexUHP = (U8)(0.5F + (float)pQPs[4] * (1.0F - qf) + (float)(pQPs + 6)[4] * qf);
            wmiSCP->uiDefaultQPIndexVHP = (U8)(0.5F + (float)pQPs[5] * (1.0F - qf) + (float)(pQPs + 6)[5] * qf);
        }
    }

    if (info->imageMode == IM_RGBA)
        wmiSCP->uAlphaMode = 2;	// encode with a planar alpha channel
}

bim::uint jxrWriteImageProc(FormatHandle *fmtHndl) {
    if (!fmtHndl) return 1;
    bim::JXRParams *par = (bim::JXRParams *) fmtHndl->internalParams;
    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &bmp->i;

    PKPixelFormatGUID guid_format = initGUIDPixelFormat(info);
    if (guid_format == GUID_PKPixelFormatDontCare) return 1;

	PKImageEncode *pEncoder = NULL;
	ERR error_code = 0;
	try {
        PKPixelInfo pixelInfo;
		pixelInfo.pGUIDPixFmt = &guid_format;
		error_code = PixelFormatLookup(&pixelInfo, LOOKUP_FORWARD);
		JXR_CHECK(error_code);

		// create a JXR encoder interface and initialize function pointers with *_WMP functions
		error_code = PKImageEncode_Create_WMP(&pEncoder);
		JXR_CHECK(error_code);

		// attach the stream to the encoder and set all encoder parameters to zero
        error_code = pEncoder->Initialize(pEncoder, par->pStream, &pEncoder->WMP.wmiSCP, sizeof(CWMIStrCodecParam));
		JXR_CHECK(error_code);

        SetEncoderParameters(&pEncoder->WMP.wmiSCP, &pixelInfo, fmtHndl, info);
		pEncoder->SetPixelFormat(pEncoder, guid_format);
        pEncoder->SetSize(pEncoder, info->width, info->height);
		
        // write metadata
        //error_code = WriteMetadata(pEncoder, info, hash);
        //JXR_CHECK(error_code);

        // encode image
        int stride = getLineSizeInBytes(bmp) * info->samples;
        size_t plane_sz = getImgSizeInBytes(bmp) * info->samples;
        std::vector<unsigned char> buffer(plane_sz, 0);
        if (buffer.size() < plane_sz) return 1;

        for (int s = 0; s<info->samples; ++s) {
            if (info->depth == 8)
                copy_from_channel<bim::uint8>(info->width, info->height, info->samples, s, bmp->bits[s], &buffer[0]);
            else if (info->depth == 16)
                copy_from_channel<bim::uint16>(info->width, info->height, info->samples, s, bmp->bits[s], &buffer[0]);
            else if (info->depth == 32)
                copy_from_channel<bim::uint32>(info->width, info->height, info->samples, s, bmp->bits[s], &buffer[0]);
        } // for sample
        
        error_code = pEncoder->WritePixels(pEncoder, info->height, &buffer[0], stride);
		JXR_CHECK(error_code);

		pEncoder->Release(&pEncoder);
		return 0;
	} catch (...) {
		if (pEncoder) pEncoder->Release(&pEncoder);
        return 1;
	}
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint jxr_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    if (!fmtHndl->fileName) return 1;


    // get metadata & ICC profile
    //error_code = ReadMetadata(par->pDecoder, dib);
    //JXR_CHECK(error_code);

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
    jxrWriteImageProc, //WriteImageProc
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


