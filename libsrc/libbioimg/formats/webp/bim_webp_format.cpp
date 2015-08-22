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
#include <bim_lcms_parse.h>
#include <bim_format_misc.h>

#include "bim_webp_format.h"

#include <webp/mux.h>
#include <webp/demux.h>
#include <webp/decode.h>
#include <webp/encode.h>

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::WEBPParams::WEBPParams() {
    i = initImageInfo();
    file = 0;
    mux = 0;
}

bim::WEBPParams::~WEBPParams() {
    if (mux) WebPMuxDelete(mux);
    if (file) delete file;
}

void bim::WEBPParams::ensure_mux() {
    if (this->buffer_file.size() > 0) return;
    // load entire image in memory for webp to decode
    this->file->seekg(0, std::ios::end);
    size_t fsize = this->file->tellg();
    this->buffer_file.resize(fsize);
    if (this->buffer_file.size() < fsize) {
        throw "Error while reading input stream";
    }
    this->file->seekg(0, std::ios::beg);
    this->file->read((char *)&this->buffer_file[0], fsize);

    int copy_data = 0;
    WebPData bitstream;
    bitstream.bytes = &this->buffer_file[0];
    bitstream.size = this->buffer_file.size();
    this->mux = WebPMuxCreate(&bitstream, copy_data);
    if (!this->mux) {
        throw "Could not initialize MUX";
    }
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

const void free_webp_data(WebPData *block) {
    free((void*)block->bytes);
    block->bytes = 0;
    block->size = 0;
}

void webpGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    *info = initImageInfo();

    // WebP requires reading the entire file contents into memory
    par->ensure_mux();

    // gets the feature flags from the mux object
    uint32_t webp_flags = 0;
    if (WebPMuxGetFeatures(par->mux, &webp_flags) != WEBP_MUX_OK) {
        throw "Error reading MUX features";
    }

    // read image information
    WebPMuxFrameInfo webp_frame = { 0 };

    // dima: try to test all frames here if needed
    int frame = 2;
    while (WebPMuxGetFrame(par->mux, frame, &webp_frame) == WEBP_MUX_OK) {
        ++frame;
    };
    int number_of_frames = frame - 1;

    if (WebPMuxGetFrame(par->mux, 1, &webp_frame) != WEBP_MUX_OK) {
        throw "Error reading MUX frame";
    }

    WebPBitstreamFeatures features;
    if (WebPGetFeatures(webp_frame.bitstream.bytes, webp_frame.bitstream.size, &features) != VP8_STATUS_OK) {
        throw "Error reading image features";
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
    info->number_t = number_of_frames;
    info->number_pages = 1;
    info->number_dims = 2;
    info->depth = 8;
    info->pixelType = FMT_UNSIGNED;

    // read metadata, deletes are not needed since data points to original memory
    WebPData block;

    // Color profile
    if ((webp_flags & ICCP_FLAG) && WebPMuxGetChunk(par->mux, "ICCP", &block) == WEBP_MUX_OK) {
        par->buffer_icc.resize(block.size);
        memcpy(&par->buffer_icc[0], block.bytes, block.size);
    }

    // get XMP metadata
    if ((webp_flags & XMP_FLAG) && WebPMuxGetChunk(par->mux, "XMP ", &block) == WEBP_MUX_OK) {
        par->buffer_xmp.resize(block.size);
        memcpy(&par->buffer_xmp[0], block.bytes, block.size);
    }

    // get Exif metadata
    if ((webp_flags & EXIF_FLAG) && WebPMuxGetChunk(par->mux, "EXIF", &block) == WEBP_MUX_OK) {
        par->buffer_exif.resize(block.size);
        memcpy(&par->buffer_exif[0], block.bytes, block.size);
    }
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
// Metadata
//----------------------------------------------------------------------------

bim::uint webp_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;

    if (par->buffer_icc.size()>0)
        hash->set_value(bim::RAW_TAGS_ICC, par->buffer_icc, bim::RAW_TYPES_ICC);
    if (par->buffer_xmp.size()>0)
        hash->set_value(bim::RAW_TAGS_XMP, par->buffer_xmp, bim::RAW_TYPES_XMP);
    if (par->buffer_iptc.size()>0)
        hash->set_value(bim::RAW_TAGS_IPTC, par->buffer_iptc, bim::RAW_TYPES_IPTC);
    if (par->buffer_exif.size()>0)
        hash->set_value(bim::RAW_TAGS_EXIF, par->buffer_exif, bim::RAW_TYPES_EXIF);

    // use LCMS2 to parse color profile
    lcms_append_metadata(fmtHndl, hash);

    // EXIV2 does not red WebP files yet
    // dima: will initing EXIV2 from memory buffers 
    //exiv_append_metadata(fmtHndl, hash);

    return 0;
}

//----------------------------------------------------------------------------
// READ
//----------------------------------------------------------------------------

bim::uint webpReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;
    bim::WEBPParams *par = (bim::WEBPParams *) fmtHndl->internalParams;

    // load entire image in memory for webp to decode
    par->ensure_mux();

    WebPMuxFrameInfo webp_frame = { 0 };
    if (WebPMuxGetFrame(par->mux, page+1, &webp_frame) != WEBP_MUX_OK) {
        throw "Error reading MUX frame";
    }
    const uint8_t* data = webp_frame.bitstream.bytes;
    const size_t data_size = webp_frame.bitstream.size;

    // advanced WebP decoding
    WebPDecoderConfig decoder_config;
    WebPDecBuffer* const output_buffer = &decoder_config.output;
    WebPBitstreamFeatures* const features = &decoder_config.input;

    // This function must always be called first, unless WebPGetFeatures() is to be called
    if (!WebPInitDecoderConfig(&decoder_config)) {
        throw "Library version mismatch";
    }

    if (WebPGetFeatures(data, data_size, features) != VP8_STATUS_OK) {
        return 1;
    }

    // allocate output image
    ImageInfo *info = &par->i;
    ImageBitmap *bmp = fmtHndl->image;
    info->width = features->width;
    info->height = features->height;
    if (features->has_alpha) {
        info->imageMode = IM_RGBA;
        info->samples = 4;
    }
    else {
        info->imageMode = IM_RGB;
        info->samples = 3;
    }
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;

    // decode the image
    decoder_config.options.use_threads = 1;
    if (WebPDecode(data, data_size, &decoder_config) != VP8_STATUS_OK) {
        return 1;
    }
    const unsigned char *buffer = output_buffer->u.RGBA.rgba;
    const unsigned int stride = output_buffer->u.RGBA.stride;
    for (int s = 0; s < info->samples; ++s) {
        copy_sample_interleaved_to_planar<uint8>(info->width, info->height, info->samples, s, buffer, bmp->bits[s], stride);
    } // for sample

    WebPFreeDecBuffer(output_buffer);
    return 0;
}

//----------------------------------------------------------------------------
// WRITE
//----------------------------------------------------------------------------

static bool WriteMetadata(WebPMux* mux, TagMap *hash) {
    int copy_data = 1;

    // write ICC profile
    if (hash->hasKey(bim::RAW_TAGS_ICC) && hash->get_type(bim::RAW_TAGS_ICC) == bim::RAW_TYPES_ICC) {
        WebPData buffer;
        buffer.bytes = (uint8_t*)hash->get_value_bin(bim::RAW_TAGS_ICC);
        buffer.size = (size_t)hash->get_size(bim::RAW_TAGS_ICC);
        if (WebPMuxSetChunk(mux, "ICCP", &buffer, copy_data) != WEBP_MUX_OK) return false;
    }

    // write XMP profile
    if (hash->hasKey(bim::RAW_TAGS_XMP) && hash->get_type(bim::RAW_TAGS_XMP) == bim::RAW_TYPES_XMP) {
        WebPData buffer;
        buffer.bytes = (uint8_t*)hash->get_value_bin(bim::RAW_TAGS_XMP);
        buffer.size = (size_t)hash->get_size(bim::RAW_TAGS_XMP);
        if (WebPMuxSetChunk(mux, "XMP ", &buffer, copy_data) != WEBP_MUX_OK) return false;
    }

    // write EXIF profile
    if (hash->hasKey(bim::RAW_TAGS_EXIF) && hash->get_type(bim::RAW_TAGS_EXIF) == bim::RAW_TYPES_EXIF) {
        WebPData buffer;
        buffer.bytes = (uint8_t*)hash->get_value_bin(bim::RAW_TAGS_EXIF);
        buffer.size = (size_t)hash->get_size(bim::RAW_TAGS_EXIF);
        if (WebPMuxSetChunk(mux, "EXIF", &buffer, copy_data) != WEBP_MUX_OK) return false;
    }

    return true;
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
        copy_sample_planar_to_interleaved<uint8>(info->width, info->height, out_samples, s, bmp->bits[s], &buffer[0]);
    } // for sample

    // if writing first image
    bool first_frame = !par->mux;
    if (!par->mux) {
        par->mux = WebPMuxNew();
    }

    WebPMemoryWriter writer;
    WebPMemoryWriterInit(&writer);

    float quality_factor = fmtHndl->quality;
    int stride = getLineSizeInBytes(bmp) * out_samples;
    WebPPicture picture;
    WebPData webp_image;
    try {
        WebPConfig config;
        WebPConfigInit(&config);
        config.method = 3; // quality/speed trade-off (0=fast, 6=slower-better)
        if (quality_factor >= 100) {
            config.lossless = 1;
            picture.use_argb = 1;
        } else {
            config.lossless = 0;
            config.quality = quality_factor;
        }

        // validate encoding parameters
        if (WebPValidateConfig(&config) == 0) {
            throw "Failed to initialize encoder";
        }

        // init picture buffer
        if (!WebPPictureInit(&picture)) {
            throw "Couldn't initialize WebPPicture";
        }
        picture.width = info->width;
        picture.height = info->height;
        if (!WebPPictureAlloc(&picture)) return 1;   // memory error

        // Set up a byte-writing method (write-to-memory, in this case):
        WebPMemoryWriter writer;
        WebPMemoryWriterInit(&writer);
        picture.writer = WebPMemoryWrite;
        picture.custom_ptr = &writer;

        if (out_samples < 4)
            WebPPictureImportRGB(&picture, &buffer[0], stride);
        else
            WebPPictureImportRGBA(&picture, &buffer[0], stride);

        if (!WebPEncode(&config, &picture)) {
            throw "Failed to encode image";
        }

        WebPPictureFree(&picture);
        webp_image.bytes = writer.mem;
        webp_image.size = writer.size;
    } catch (...) {
        WebPPictureFree(&picture);
#if WEBP_ENCODER_ABI_VERSION > 0x0203
        WebPMemoryWriterClear(&writer);
#else
        if (writer.mem) free(writer.mem);
#endif
        return 1;
    }

    WebPData output_data = { 0 };
    try {
        // write into MUX
        int copy_data = 1;
        if (WebPMuxSetImage(par->mux, &webp_image, copy_data) != WEBP_MUX_OK) {
            throw "Failed to write image";
        }
        //WebPMuxError WebPMuxPushFrame(WebPMux* mux, const WebPMuxFrameInfo* frame, int copy_data);

        if (first_frame && !WriteMetadata(par->mux, fmtHndl->metaData)) {
            throw "Failed to write metadata";
        }

        // get data from mux in WebP RIFF format
        if (WebPMuxAssemble(par->mux, &output_data) != WEBP_MUX_OK) {
            throw "Failed to assemble MUX";
        }
    } catch (...) {
        WebPDataClear(&output_data);
#if WEBP_ENCODER_ABI_VERSION > 0x0203
        WebPMemoryWriterClear(&writer);
#else
        if (writer.mem) free(writer.mem);
#endif
        return 1;
    }

    // write image to a file
#if defined(BIM_WIN)
    bim::xstring fn(fmtHndl->fileName);
    std::fstream file((const wchar_t *)fn.toUTF16().c_str(), std::fstream::out | std::fstream::binary);
#else
    std::fstream file(fmtHndl->fileName, std::fstream::out | std::fstream::binary);
#endif   
    file.write((char *)output_data.bytes, output_data.size);
    file.close();

    // free WebP output file
    WebPDataClear(&output_data);
#if WEBP_ENCODER_ABI_VERSION > 0x0203
    WebPMemoryWriterClear(&writer);
#else
    if (writer.mem) free(writer.mem);
#endif

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


