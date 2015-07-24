/*****************************************************************************
JPEG-2000 support
Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>
Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

Authors:
    Mario Emmenlauer <mario@emmenlauer.de>
    Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
    2015-04-19 14:20:00 - First creation
    2015-07-22 11:21:40 - Rewrite

ver : 2
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <xtypes.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>

#include "bim_jp2_format.h"

#include <openjpeg.h>

using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

// JP2 I/O wrappers

static void jp2_error_callback(const char *msg, void *client_data) {
    std::cerr << "[JP2 codec error] " << msg;
}

static void jp2_warning_callback(const char *msg, void *client_data) {
    std::cerr << "[JP2 codec warning] " << msg;
}

static OPJ_UINT64 _LengthProc(bim::JP2Params *par) {
    size_t cur_pos = par->file->tellg();
    par->file->seekg(0, std::fstream::end);
    size_t fsize = par->file->tellg();
    par->file->seekg(cur_pos, std::fstream::beg);
    return fsize;
}

static OPJ_SIZE_T _ReadProc(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
    bim::JP2Params *par = (bim::JP2Params*) p_user_data;
    par->file->read((char*)p_buffer, p_nb_bytes);
    size_t red = par->file->gcount();
    return (par->file->rdstate() & std::ifstream::badbit) == 0 && red>0? red : -1;
}

static OPJ_SIZE_T _WriteProc(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
    bim::JP2Params *par = (bim::JP2Params*) p_user_data;
    par->file->write((char*)p_buffer, p_nb_bytes);
    return (par->file->rdstate() & std::ifstream::badbit) == 0 ? p_nb_bytes : -1;
}

static OPJ_OFF_T _SkipProc(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
    bim::JP2Params *par = (bim::JP2Params*) p_user_data;
    par->file->seekg(p_nb_bytes, std::fstream::cur);
    return (par->file->rdstate() & std::ifstream::badbit) == 0 ? p_nb_bytes : -1;
}

static OPJ_BOOL _SeekProc(OPJ_OFF_T p_nb_bytes, FILE * p_user_data) {
    bim::JP2Params *par = (bim::JP2Params*) p_user_data;
    par->file->seekg(p_nb_bytes, std::fstream::beg);
    return (par->file->rdstate() & std::ifstream::badbit) == 0 ? OPJ_TRUE : OPJ_FALSE;
}


// JP2 parameters

bim::JP2Params::JP2Params() {
    i = initImageInfo();
    stream = NULL;
    codec = NULL;
    image = NULL;
    num_tiles_x = 0;
    num_tiles_y = 0;
}

bim::JP2Params::~JP2Params() {
    close();
}

void bim::JP2Params::open(const char *filename, bim::ImageIOModes io_mode) {
    std::ios_base::openmode mode = io_mode == IO_READ ? std::fstream::in : std::fstream::out;
    #if defined(BIM_WIN)
    bim::xstring fn(filename);
    this->file = new std::fstream((const wchar_t *)fn.toUTF16().c_str(), mode | std::fstream::binary);
    #else
    this->file = new std::fstream(filename, mode | std::fstream::binary);
    #endif

    this->stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, io_mode == IO_READ ? OPJ_TRUE : OPJ_FALSE);
    if (this->stream) {
        opj_stream_set_user_data(this->stream, this, NULL);
        opj_stream_set_user_data_length(this->stream, _LengthProc(this));
        opj_stream_set_read_function(this->stream, (opj_stream_read_fn)_ReadProc);
        opj_stream_set_write_function(this->stream, (opj_stream_write_fn)_WriteProc);
        opj_stream_set_skip_function(this->stream, (opj_stream_skip_fn)_SkipProc);
        opj_stream_set_seek_function(this->stream, (opj_stream_seek_fn)_SeekProc);

        if (io_mode == IO_READ)
            codec = opj_create_decompress(OPJ_CODEC_JP2);
        else
            codec = opj_create_compress(OPJ_CODEC_JP2);

        // configure the event callbacks
        opj_set_info_handler(codec, NULL, NULL);
        opj_set_warning_handler(codec, NULL, NULL); //opj_set_warning_handler(codec, jp2_warning_callback, NULL);
        opj_set_error_handler(codec, jp2_error_callback, NULL);

        if (io_mode == IO_READ) {
            // setup the decoder decoding parameters using user parameters
            opj_dparameters_t parameters;
            opj_set_default_decoder_parameters(&parameters);
            if (!opj_setup_decoder(codec, &parameters)) {
                throw "Failed to setup the decoder\n";
            }

            // read the main header of the codestream and if necessary the JP2 boxes
            if (!opj_read_header(stream, codec, &image)) {
                throw "Failed to read the header\n";
            }
        }
    }
}

void bim::JP2Params::close() {
    if (image) {
        opj_image_destroy(image);
        image = NULL;
    }
    if (codec) {
        opj_destroy_codec(codec);
        codec = NULL;
    }
    if (stream) {
        opj_stream_destroy(stream);
        stream = NULL;
    }
    if (file) {
        delete file;
        file = NULL;
    }
}


//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_JP2_MAGIC_SIZE 12

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
// position 45: "\xff\x52" 
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

int jp2ValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_JP2_MAGIC_SIZE) return -1;

    // The file is a valid jp2 image (openjpeg magic_format JP2_CFMT, correct extension .jp2):
    if (memcmp(magic, JP2_RFC3745_MAGIC, 12) == 0) return 0;
    if (memcmp(magic, JP2_MAGIC, 4) == 0) return 0;

    // The file is a valid jp2 image (openjpeg magic_format J2K_CFMT, correct extensions .j2k, .jpc, .j2c):
    if (memcmp(magic, J2K_CODESTREAM_MAGIC, 4) == 0) return 0;

    return -1;
}

FormatHandle jp2AquireFormatProc(void) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void jp2CloseImageProc(FormatHandle *fmtHndl);
void jp2ReleaseFormatProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    jp2CloseImageProc(fmtHndl);
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------


void jp2SetWriteParameters(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    fmtHndl->order = 1; // use progressive encoding by default
    fmtHndl->quality = 95;
    
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    par->i.tileWidth = par->i.tileHeight = 0;

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
        } else
        if (options[i] == "tiles" && options.size() - i>0) {
            par->i.tileWidth = par->i.tileHeight = options[++i].toInt(0);
            continue;
        }

    } // while
}

void jp2GetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    *info = initImageInfo();

    // header should be already loaded in the open

    opj_codestream_info_v2_t* pCodeStreamInfo = opj_get_cstr_info(par->codec);
    info->tileWidth = pCodeStreamInfo->tdx;
    info->tileHeight = pCodeStreamInfo->tdy;
    par->num_tiles_x = pCodeStreamInfo->tw;
    par->num_tiles_y = pCodeStreamInfo->th;
    info->number_levels = pCodeStreamInfo->m_default_tile_info.tccp_info[0].numresolutions;
    opj_destroy_cstr_info(&pCodeStreamInfo);

    info->width = par->image->comps[0].w;
    info->height = par->image->comps[0].h;
    info->samples = par->image->numcomps;
    info->depth = par->image->comps[0].prec;
    
    info->pixelType = FMT_UNSIGNED;
    if (par->image->comps[0].sgnd == 1)
        info->pixelType = FMT_SIGNED;
    // dima: need to identify when image is floating point
    //info->pixelType = FMT_FLOAT;
    //par->image->comps[0].sgnd
    //par->image->comps[0].bpp

    // dima: here we need to actually define the colorspace
    info->imageMode = IM_GRAYSCALE;
    if (par->image->color_space == OPJ_CLRSPC_SRGB) {
        info->imageMode = IM_RGB;
    } else if (par->image->color_space == OPJ_CLRSPC_SYCC) {
        info->imageMode = IM_YUV;
    } else if (par->image->color_space == OPJ_CLRSPC_EYCC) {
        info->imageMode = IM_YUV; // dima: needs fixing
    } else if (par->image->color_space == OPJ_CLRSPC_CMYK) {
        info->imageMode = IM_CMYK;
    } else if (info->samples > 1) {
        info->imageMode = IM_MULTI;
    }

    for (int c = 0; c < par->image->numcomps - 1; c++) {
        if ((par->image->comps[c].dx != par->image->comps[c + 1].dx) ||
            (par->image->comps[c].dy != par->image->comps[c + 1].dy) ||
            (par->image->comps[c].prec != par->image->comps[c + 1].prec)) 
        {
            throw "Non-uniform componnets are not supported\n";
        }
    }

    info->number_z = 1;
    info->number_t = 1;
    info->number_pages = 1;
    info->number_dims = 2;

    /*
    float resX, resY;	// image resolution (in dots per inch)
    par->pDecoder->GetResolution(par->pDecoder, &resX, &resY);
    info->resUnits = RES_IN;
    info->xRes = resX;
    info->yRes = resY;
    */
}

void jp2CloseImageProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose(fmtHndl);
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint jp2OpenImageProc(FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) jp2CloseImageProc(fmtHndl);
    bim::JP2Params *par = new bim::JP2Params();
    fmtHndl->internalParams = (void *)par;

    try {
        par->open(fmtHndl->fileName, io_mode);
        if (io_mode == IO_READ) {
            jp2GetImageInfo(fmtHndl);
        } else if (io_mode == IO_WRITE) {
            jp2SetWriteParameters(fmtHndl);
        }
    } catch (...) {
        jp2CloseImageProc(fmtHndl);
        return 1;
    }

    return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint jp2GetNumPagesProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    return info->number_pages;
}

ImageInfo jp2GetImageInfoProc(FormatHandle *fmtHndl, bim::uint page_num) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    fmtHndl->pageNumber = page_num;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// READ
//----------------------------------------------------------------------------

template <typename T>
void copy_component(bim::uint64 W, bim::uint64 H, int sample, const opj_image_comp_t &in, void *out) {
    OPJ_INT32 a = (in.sgnd ? 1 << (in.prec - 1) : 0);
    if (W > 32 && H > 32) { // use parallel version
        #pragma omp parallel for default(shared)
        for (bim::int64 y = 0; y < H; ++y) {
            T *p = ((T *)out) + y*W;
            bim::uint64 pos = y*W;
            for (bim::int64 x = 0; x < W; ++x) {
                *p = (T)(in.data[pos] + a);
                p++;
                pos++;
            } // for x
        } // for y
    } else {
        for (bim::int64 y = 0; y < H; ++y) {
            T *p = ((T *)out) + y*W;
            bim::uint64 pos = y*W;
            for (bim::int64 x = 0; x < W; ++x) {
                *p = (T)(in.data[pos] + a);
                p++;
                pos++;
            } // for x
        } // for y
    }
}

bim::uint jp2ReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;

    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &par->i;
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;

    try {
        // openjpeg unfortunately has some problem with multiple decodings, have to reset everything from scratch
        if (!par->image) 
            par->open(fmtHndl->fileName, fmtHndl->io_mode);

        if (!opj_decode(par->codec, par->stream, par->image)) {
            throw "Failed to decode image\n";
        }

        if (!opj_end_decompress(par->codec, par->stream)) {
            throw "Failed to finish decompression\n";
        }

        for (int s = 0; s < info->samples; ++s) {
            if (info->depth == 8)
                copy_component<bim::uint8>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 16)
                copy_component<bim::uint16>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 32)
                copy_component<bim::uint32>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
        } // for sample

        par->close();
    } catch (...) {
        par->close();
        return 1;
    }
    return 0;
}

bim::uint jp2ReadImageLevelProc(FormatHandle *fmtHndl, bim::uint page, bim::uint level) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    try {
        // openjpeg unfortunately has some problem with multiple decodings, have to reset everything from scratch
        if (!par->image)
            par->open(fmtHndl->fileName, fmtHndl->io_mode);

        // read a specific resolution level
        if (!opj_set_decoded_resolution_factor(par->codec, level)) {
            throw "Failed to set image level\n";
        }
        
        // setting resolution level does not change image size in opj_decode, a little hack to
        // update image size based on the resolution level
        info->width = bim::round<bim::uint64>((double)info->width / pow<double>(2.0, level));
        info->height = bim::round<bim::uint64>((double)info->height / pow<double>(2.0, level));
        for (int i = 0; i < info->samples; ++i) {
            par->image->comps[i].w = info->width;
            par->image->comps[i].h = info->height;
        }
        
        if (!opj_decode(par->codec, par->stream, par->image)) {
            throw "Failed to decode image level\n";
        }

        if (!opj_end_decompress(par->codec, par->stream)) {
            throw "Failed to finish decompression\n";
        }

        // setting resolution level does not change image size in opj_decode 
        //info->width = par->image->comps[0].w;
        //info->height = par->image->comps[0].h;
        ImageBitmap *bmp = fmtHndl->image;
        if (allocImg(fmtHndl, info, bmp) != 0) return 1;

        for (int s = 0; s < info->samples; ++s) {
            if (info->depth == 8)
                copy_component<bim::uint8>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 16)
                copy_component<bim::uint16>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 32)
                copy_component<bim::uint32>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
        } // for sample

        par->close();
    }
    catch (...) {
        par->close();
        return 1;
    }
    return 0;
}

bim::uint jp2ReadImageTileProc(FormatHandle *fmtHndl, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    if (xid>=par->num_tiles_x || yid >= par->num_tiles_y) {
        return 1;
    }

    try {
        // openjpeg unfortunately has some problem with multiple decodings, have to reset everything from scratch
        if (!par->image)
            par->open(fmtHndl->fileName, fmtHndl->io_mode);

        // read a specific resolution level
        if (!opj_set_decoded_resolution_factor(par->codec, level)) {
            throw "Failed to set image level\n";
        }
            
        OPJ_UINT32 tile_index = yid * par->num_tiles_x + xid;
        if (!opj_get_decoded_tile(par->codec, par->stream, par->image, tile_index)) {
            throw "Failed to decode tile\n";
        }

        if (!opj_end_decompress(par->codec, par->stream)) {
            throw "Failed to finish decompression\n";
        }

        info->width = par->image->comps[0].w;
        info->height = par->image->comps[0].h;
        ImageBitmap *bmp = fmtHndl->image;
        if (allocImg(fmtHndl, info, bmp) != 0) return 1;

        for (int s = 0; s < info->samples; ++s) {
            if (info->depth == 8)
                copy_component<bim::uint8>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 16)
                copy_component<bim::uint16>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
            else if (info->depth == 32)
                copy_component<bim::uint32>(info->width, info->height, s, par->image->comps[s], bmp->bits[s]);
        } // for sample

        par->close();
     } catch (...) {
        par->close();
        return 1;
    }
    return 0;
}

//----------------------------------------------------------------------------
// WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_from_component(bim::uint64 W, bim::uint64 H, int sample, const void *in, const opj_image_comp_t &out) {
    #pragma omp parallel for default(shared)
    for (bim::int64 y = 0; y < H; ++y) {
        T *p = ((T *)in) + y*W;
        bim::uint64 pos = y*W;
        for (bim::int64 x = 0; x < W; ++x) {
            out.data[pos] = (OPJ_INT32) (*p);
            p++;
            pos++;
        } // for x
    } // for y
}

bim::uint jp2WriteImageProc(FormatHandle *fmtHndl) {
    if (!fmtHndl) return 1;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;
    int tile_size = par->i.tileWidth;
    ImageBitmap *bmp = fmtHndl->image;
    ImageInfo *info = &bmp->i;
    
    opj_image_t *image = NULL;
    try {
        opj_cparameters_t parameters;
        opj_set_default_encoder_parameters(&parameters);
        parameters.cp_disto_alloc = 1;
        parameters.tcp_numlayers = 1;
        parameters.tcp_mct = (info->samples == 3) ? 1 : 0; // decide if MCT should be used
        
        // set the number of resolution levels
        int min_level_size = 50;
        unsigned int side_sz = bim::max<unsigned int>(info->width, info->height);
        if (tile_size > 0) {
            side_sz = tile_size;
            min_level_size = 4;
        }
        parameters.numresolution = ceil(bim::log2<double>(side_sz)) - ceil(bim::log2<double>(min_level_size)) + 1;

        if (fmtHndl->quality >= 100) { // lossless mode
            parameters.irreversible = 0;
            parameters.tcp_rates[0] = 0;
        } else {
            parameters.irreversible = 1;
            parameters.tcp_rates[0] = 100.0-fmtHndl->quality;
            //parameters.tcp_distoratio[0] = (double) fmtHndl->quality / 100.0;
            //parameters.cp_fixed_quality = OPJ_TRUE;
        }

        // select tiled writing
        if (tile_size>0) {
            parameters.tile_size_on = OPJ_TRUE;
            parameters.cp_tx0 = 0;
            parameters.cp_ty0 = 0;
            parameters.cp_tdx = tile_size;
            parameters.cp_tdy = tile_size;
        }

        // initialize image components 
        std::vector<opj_image_cmptparm_t> cmptparm(info->samples);
        memset(&cmptparm[0], 0, info->samples * sizeof(opj_image_cmptparm_t));
        for (int i = 0; i < info->samples; i++) {
            cmptparm[i].dx = parameters.subsampling_dx;
            cmptparm[i].dy = parameters.subsampling_dy;
            cmptparm[i].w = info->width;
            cmptparm[i].h = info->height;
            cmptparm[i].prec = info->depth;
            cmptparm[i].bpp = info->depth;
            cmptparm[i].sgnd = (info->pixelType == FMT_SIGNED) ? 1 : 0;
        }

        // dima: we need proper color space management 
        OPJ_COLOR_SPACE color_space = OPJ_CLRSPC_UNSPECIFIED;
        if (info->imageMode == IM_RGB) {
            //color_space = OPJ_CLRSPC_SRGB;
        } else if (info->imageMode == IM_YUV) {
            color_space = OPJ_CLRSPC_SYCC;
        } else if (info->imageMode == IM_CMYK) {
            color_space = OPJ_CLRSPC_CMYK;
        }

        opj_image_t *image = opj_image_create(info->samples, &cmptparm[0], color_space);
        if (!image) {
            throw "Could not create JP2 image buffer";
        }

        // set image offset and reference grid 
        image->x0 = parameters.image_offset_x0;
        image->y0 = parameters.image_offset_y0;
        image->x1 = parameters.image_offset_x0 + (info->width - 1) * parameters.subsampling_dx + 1;
        image->y1 = parameters.image_offset_y0 + (info->height - 1) * parameters.subsampling_dy + 1;

        for (int s = 0; s<info->samples; ++s) {
            if (info->depth == 8)
                copy_from_component<bim::uint8>(info->width, info->height, s, bmp->bits[s], image->comps[s]);
            else if (info->depth == 16)
                copy_from_component<bim::uint16>(info->width, info->height, s, bmp->bits[s], image->comps[s]);
            else if (info->depth == 32)
                copy_from_component<bim::uint32>(info->width, info->height, s, bmp->bits[s], image->comps[s]);
        } // for sample

        // setup the encoder parameters using the current image and using user parameters
        opj_setup_encoder(par->codec, &parameters, image);

        // encode the image
        if (!opj_start_compress(par->codec, image, par->stream)) {
            throw "Failed to init the encoder";
        }

        if (!opj_encode(par->codec, par->stream)) {
            throw "Failed to encode the image";
        }

        if (!opj_end_compress(par->codec, par->stream)) {
            throw "Failed to finilize the compression";
        }

        opj_image_destroy(image);
    } catch (...) {
        opj_image_destroy(image);
        return 1;
    }

    return 1;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

bim::uint jp2_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    if (!fmtHndl->fileName) return 1;
    bim::JP2Params *par = (bim::JP2Params *) fmtHndl->internalParams;

    // append image pyramid meta
    ImageInfo *info = &par->i;
    double scale = 1.0;
    std::vector<double> scales;
    for (int i = 0; i < info->number_levels; ++i) {
        scales.push_back(scale);
        scale /= 2.0;
    }
    hash->set_value(bim::IMAGE_NUM_RES_L, (int)info->number_levels);
    hash->set_value(bim::IMAGE_RES_L_SCALES, xstring::join(scales, ","));
    hash->set_value(bim::IMAGE_RES_STRUCTURE, bim::IMAGE_RES_STRUCTURE_FLAT);
    
    // Read the main header of the codestream, and if necessary the JP2 boxes
    std::vector<xstring> comments;
    if (par->image->cp_comment) {
        size_t endpos = 0;
        size_t startpos = 0;
        const char* comment;
        while (true) {
            ++endpos;
            if (par->image->cp_comment[endpos] == 0 && par->image->cp_comment[endpos + 1] != 0) {
                // A single zero character followed by a non-zero character indicates the end
                // of one comment. So here is a break between two concatenated comments:
                comment = (par->image->cp_comment + startpos);
                comments.push_back(comment);
                startpos = endpos + 1;
            }
            else if (par->image->cp_comment[endpos] == 0 && par->image->cp_comment[endpos + 1] == 0 && par->image->cp_comment[endpos + 2] == 0) {
                // Three consecutive zero characters indicate the end of the comments list.
                // So here is the end of all string comments:
                comment = (par->image->cp_comment + startpos);
                comments.push_back(comment);
                break;
            }
        }
    }

    for (size_t comidx = 0; comidx < comments.size(); ++comidx) {
        // NOTE: if comments contain an '=' sign, they are 'key=value' strings and
        // should be split on the '=' sign:
        const size_t pos = comments[comidx].find('=');
        if (pos != std::string::npos) {
            std::vector<xstring> v = comments[comidx].split("=");
            hash->append_tag("custom/" + v[0], v[1]);
        } else {
            hash->append_tag("custom/Comment", comments[comidx]);
        }
    }

    // Append EXIV2 metadata
    exiv_append_metadata(fmtHndl, hash);

    return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_JP2_NUM_FORMATS 1

FormatItem jp2Items[BIM_JP2_NUM_FORMATS] = {
    { //0
        "JP2",            // short name, no spaces
        "JPEG2000 File Format", // Long format name
        "jp2|j2k|jpx|jpc|jpt",   // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        1, //canWriteMeta; // 0 - NO, 1 - YES
        0, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 1, 0, 0, 0, 0, 1 }
    }
};


FormatHeader jp2Header = {

    sizeof(FormatHeader),
    "2.1.0",
    "JPEG-2000",
    "JP2-JFIF Compliant CODEC",

    BIM_FORMAT_JP2_MAGIC_SIZE,
    { 1, BIM_JP2_NUM_FORMATS, jp2Items },

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
    jp2ReadImageTileProc, //ReadImageTileProc
    NULL, //WriteImageTileProc
    jp2ReadImageLevelProc, //ReadImageLevelProc
    NULL, //WriteImageLineProc
    NULL, //ReadImageThumbProc
    NULL, //WriteImageThumbProc
    NULL, //, //ReadImagePreviewProc

    // meta data
    NULL, //ReadMetaDataProc
    NULL,  //AddMetaDataProc
    NULL, //ReadMetaDataAsTextProc
    jp2_append_metadata, //AppendMetaDataProc

    NULL,
    NULL,
    ""

};

extern "C" {

    FormatHeader* jp2GetFormatHeader(void)
    {
        return &jp2Header;
    }

} // extern C


