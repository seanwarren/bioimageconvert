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

    // create a JXR decoder interface and initialize function pointers with *_WMP functions
    ERR error_code = PKImageDecode_Create_WMP(&par->pDecoder);
    JXR_CHECK(error_code);

    error_code = par->pDecoder->Initialize(par->pDecoder, par->pStream);
    JXR_CHECK(error_code);

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
    int width, height;	// image dimensions (in pixels)
    par->pDecoder->GetSize(par->pDecoder, &width, &height);
    info->width = width;
    info->height = height;

    float resX, resY;	// image resolution (in dots per inch)
    par->pDecoder->GetResolution(par->pDecoder, &resX, &resY);
    info->resUnits = RES_IN;
    info->xRes = resX;
    info->yRes = resY;

    info->number_z = 1;
    info->number_t = 1;
    info->number_pages = 1;
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
// READ/WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in + sample;
    T *p = (T *)out;
    int step = sizeof(T)*samples;
    size_t inrowsz = sizeof(T)*samples*W;
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

    // allocate output image
    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &par->i;
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;
        
    //CopyPixels(PKImageDecode *pDecoder, PKPixelFormatGUID out_guid_format, FIBITMAP *dib, int width, int height) {
        
    PKFormatConverter *pConverter = NULL;
    ERR error_code = 0;
    const PKRect rect = { 0, 0, info->width, info->height };

    try {
        PKPixelFormatGUID in_guid_format;
        error_code = par->pDecoder->GetPixelFormat(par->pDecoder, &in_guid_format);
        JXR_CHECK(error_code);

        const unsigned stride = getLineSizeInBytes(bmp) * info->samples;
        bool needs_conversion = false;

        if (!needs_conversion && info->samples == 1) {
            error_code = par->pDecoder->Copy(par->pDecoder, &rect, (U8*) bmp->bits[0], stride);
            JXR_CHECK(error_code);
        } else if (!needs_conversion) {
            bim::uint64 buf_sz = getImgSizeInBytes(bmp) * info->samples;
            std::vector<unsigned char> buffer(buf_sz, 0);
            error_code = par->pDecoder->Copy(par->pDecoder, &rect, (U8*)&buffer[0], stride);
            JXR_CHECK(error_code);
            for (int s = 0; s < info->samples; ++s) {
                copy_channel<uint8>(info->width, info->height, info->samples, s, &buffer[0], bmp->bits[s]);
            } // for sample
        } else { // we need to use the conversion API for complex types
            /*
            // allocate the pixel format converter
            error_code = PKCodecFactory_CreateFormatConverter(&pConverter);
            JXR_CHECK(error_code);

            // set the conversion function
            error_code = pConverter->Initialize(pConverter, pDecoder, NULL, out_guid_format);
            JXR_CHECK(error_code);

            // get the maximum stride
            unsigned cbStride = 0;
            {
                PKPixelInfo pPIFrom;
                PKPixelInfo pPITo;

                pPIFrom.pGUIDPixFmt = &in_guid_format;
                error_code = PixelFormatLookup(&pPIFrom, LOOKUP_FORWARD);
                JXR_CHECK(error_code);

                pPITo.pGUIDPixFmt = &out_guid_format;
                error_code = PixelFormatLookup(&pPITo, LOOKUP_FORWARD);
                JXR_CHECK(error_code);

                unsigned cbStrideFrom = ((pPIFrom.cbitUnit + 7) >> 3) * width;
                unsigned cbStrideTo = ((pPITo.cbitUnit + 7) >> 3) * width;
                cbStride = MAX(cbStrideFrom, cbStrideTo);
            }

            // allocate a local decoder / encoder buffer
            error_code = PKAllocAligned((void **)&pb, cbStride * height, 128);
            JXR_CHECK(error_code);

            // copy / convert pixels
            error_code = pConverter->Copy(pConverter, &rect, pb, cbStride);
            JXR_CHECK(error_code);

            // now copy pixels into the dib
            const size_t line_size = FreeImage_GetLine(dib);
            for (int y = 0; y < height; y++) {
                BYTE *src_bits = (BYTE*)(pb + y * cbStride);
                BYTE *dst_bits = (BYTE*)FreeImage_GetScanLine(dib, y);
                memcpy(dst_bits, src_bits, line_size);
            }

            // free the local buffer
            PKFreeAligned((void **)&pb);

            // free the pixel format converter
            PKFormatConverter_Release(&pConverter);
            */
        }

    } catch (...) {
        //PKFreeAligned((void **)&pb);
        if (pConverter) PKFormatConverter_Release(&pConverter);
        return 1;
    }
    return 0;
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

static void SetEncoderParameters(CWMIStrCodecParam *wmiSCP, const PKPixelInfo *pixelInfo, FormatHandle *fmtHndl, ImageInfo *info) {
    //wmiSCP->cfColorFormat = YUV_444;		// color format
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

    if (fmtHndl->quality == 100) {
        wmiSCP->uiDefaultQPIndex = 1;
    } else {
        wmiSCP->uiDefaultQPIndex = fmtHndl->quality / 100.0;
        if (fmtHndl->quality >= 50)
            wmiSCP->olOverlap = OL_ONE;
        else
            wmiSCP->olOverlap = OL_TWO;

        if (fmtHndl->quality >= 50 || pixelInfo->uBitsPerSample > 8)
            wmiSCP->cfColorFormat = YUV_444;
        else
            wmiSCP->cfColorFormat = YUV_420;
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
		

        //pEncoder->SetResolution(pEncoder, resX, resY);
		//WriteMetadata(pEncoder, dib);

        // encode image
        int stride = getLineSizeInBytes(bmp) * info->samples;
        size_t plane_sz = getImgSizeInBytes(bmp) * info->samples;
        std::vector<unsigned char> buffer(plane_sz, 0);
        if (buffer.size() < plane_sz) return 1;

        for (int s = 0; s<info->samples; ++s) {
            copy_from_channel<uint8>(info->width, info->height, info->samples, s, bmp->bits[s], &buffer[0]);
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


