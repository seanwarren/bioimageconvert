/*****************************************************************************
  RAW support
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION

  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
  12/01/2005 15:27 - First creation
  2007-07-12 21:01 - reading raw

  Ver : 2

  *****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_raw_format.h"

#include <zlib.h>

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
#pragma warning(disable:4996)
#endif 

using namespace bim;

#define BIM_RAW_FORMAT_RAW   0
#define BIM_RAW_FORMAT_NRRD  1
#define BIM_RAW_FORMAT_MHD   2

//****************************************************************************
// Misc
//****************************************************************************

bim::RawParams::RawParams() {
    i = initImageInfo();
    header_offset = 0;
    big_endian = false;
    interleaved = false;
    res.resize(5, 0.0);
}

bim::RawParams::~RawParams() {
    if (datastream) {
        FormatHandle fmtHndl = bim::initFormatHandle();
        fmtHndl.stream = datastream;
        xclose(&fmtHndl);
    }
}


//****************************************************************************
// format
//****************************************************************************

const void mhd2PixelFormat(const std::string &pf, ImageInfo *info) {
    if (pf == "MET_UCHAR") {
        info->depth = 8;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "MET_USHORT") {
        info->depth = 16;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "MET_ULONG") {
        info->depth = 32;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "MET_ULONG_LONG") {
        info->depth = 64;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "MET_CHAR") {
        info->depth = 8;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "MET_SHORT") {
        info->depth = 16;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "MET_LONG") {
        info->depth = 32;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "MET_LONG_LONG") {
        info->depth = 64;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "MET_FLOAT") {
        info->depth = 32;
        info->pixelType = bim::FMT_FLOAT;
    }
    else if (pf == "MET_DOUBLE") {
        info->depth = 64;
        info->pixelType = bim::FMT_FLOAT;
    }
}

xstring replaceFileName(const xstring &path, const xstring &filename) {
#ifdef BIM_WIN
    xstring div = "\\";
#else
    xstring div = "/";
#endif
    std::vector<xstring> p = path.split(div);
    p[p.size() - 1] = filename;
    return xstring::join(p, div);
}

bool mhdGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return false;
    if (fmtHndl->internalParams == NULL) return false;
    if (fmtHndl->stream == NULL) return false;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    xstring buf(4096, 0); // very large header size, load in one shot
    xseek(fmtHndl, 0, SEEK_SET);
    bim::xread(fmtHndl, &buf[0], 1, buf.size() - 1);

    std::vector<xstring> lines = buf.replace("\r", "").split("\n");
    for (int i = 0; i < lines.size(); ++i) {
        std::vector<xstring> v = lines[i].split("=");
        if (v.size()>1)
            par->header.append_tag(v[0].strip(" "), v[1].strip(" "));
    }

    if (par->header.get_value("ObjectType") != "Image") return false;

    std::vector<int> dims = par->header.get_value("DimSize").splitInt(" ");
    if (dims.size() < 2) return false;
    info->width = dims[0];
    info->height = dims[1];
    if (dims.size() > 2) info->number_z = dims[2]; else info->number_z = 1;
    if (dims.size() > 3) info->number_t = dims[3]; else info->number_t = 1;
    info->number_pages = info->number_z * info->number_t;

    par->big_endian = par->header.get_value("ElementByteOrderMSB") == "True";

    par->res = par->header.get_value("ElementSize").splitDouble(" ");
    par->res.resize(5, 0);

    info->samples = par->header.get_value_int("ElementNumberOfChannels", 1);
    mhd2PixelFormat(par->header.get_value("ElementType"), info);
    par->header_offset = par->header.get_value_int("HeaderSize", 0);

    par->datafile = replaceFileName(xstring(fmtHndl->fileName), par->header.get_value("ElementDataFile"));

    return true;
}

const void nrrd2PixelFormat(const std::string &pf, ImageInfo *info) {
    if (pf == "uchar" || pf == "unsigned char" || pf == "uint8" || pf == "uint8_t") {
        info->depth = 8;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "ushort" || pf == "unsigned short" || pf == "unsigned short int" || pf == "uint16" || pf == "uint16_t") {
        info->depth = 16;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "uint" || pf == "unsigned int" || pf == "uint32" || pf == "uint32_t") {
        info->depth = 32;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "ulonglong" || pf == "unsigned long long" || pf == "unsigned long long int"  || pf == "uint64" || pf == "uint64_t") {
        info->depth = 64;
        info->pixelType = bim::FMT_UNSIGNED;
    }
    else if (pf == "signed char" || pf == "int8" || pf == "int8_t") {
        info->depth = 8;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "short" || pf == "short int" || pf == "signed short" || pf == "signed short int" || pf == "int16" || pf == "int16_t") {
        info->depth = 16;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "int" || pf == "signed int" || pf == "int32" || pf == "int32_t") {
        info->depth = 32;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "longlong" || pf == "long long" || pf == "long long int" || pf == "signed long long" || pf == "signed long long int" || pf == "int64" || pf == "int64_t") {
        info->depth = 64;
        info->pixelType = bim::FMT_SIGNED;
    }
    else if (pf == "float") {
        info->depth = 32;
        info->pixelType = bim::FMT_FLOAT;
    }
    else if (pf == "double") {
        info->depth = 64;
        info->pixelType = bim::FMT_FLOAT;
    }
}

bool nrrdGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return false;
    if (fmtHndl->internalParams == NULL) return false;
    if (fmtHndl->stream == NULL) return false;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    xstring buf(4096, 0); // very large header size, load in one shot
    xseek(fmtHndl, 0, SEEK_SET);
    bim::xread(fmtHndl, &buf[0], 1, buf.size() - 1);

    // find data offset
    par->header_offset = 0;
    for (int i = 0; i < 4096 - 4; ++i) {
        if (memcmp(&buf[i], "\n\n", 2) == 0) {
            par->header_offset = i + 2;
            break;
        }
        if (memcmp(&buf[i], "\r\n\r\n", 4) == 0) {
            par->header_offset = i + 4;
            break;
        }
    }
    buf.resize(par->header_offset, 0);

    std::vector<xstring> lines = buf.replace("\r", "").split("\n");
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].size()>0 && lines[i][0] == '#') continue;
        std::vector<xstring> v = lines[i].split(":");
        if (v.size() > 1)
            par->header.append_tag(v[0].strip(" "), v[1].strip(" ").lstrip("="));
    }

    std::vector<int> dims = par->header.get_value("sizes").splitInt(" ");
    if (dims.size() < 2) return false;

    // dima: this is a pure hack, there's no true mechanism to identify channels
    if (dims[0] < 10 && dims.size() > 2) {
        par->interleaved = true;
        info->samples = dims[0];
        info->width = dims[1];
        info->height = dims[2];
        if (dims.size() > 3) info->number_z = dims[3]; else info->number_z = 1;
        if (dims.size() > 4) info->number_t = dims[4]; else info->number_t = 1;
    } else {
        info->samples = 1;
        info->width = dims[0];
        info->height = dims[1];
        if (dims.size() > 2) info->number_z = dims[2]; else info->number_z = 1;
        if (dims.size() > 3) info->number_t = dims[3]; else info->number_t = 1;
    }
    info->number_pages = info->number_z * info->number_t;
    
    nrrd2PixelFormat(par->header.get_value("type"), info);

    par->big_endian = par->header.get_value("endian") == "big";

    par->res = par->header.get_value("spacings").splitDouble(" ");
    par->res.resize(5, 0);

    par->units = par->header.get_value("space units").split(" ");

    //par->header_offset = par->header.get_value_int("HeaderSize", 0);

    //par->header.get_value("encoding") == "raw"

    if (par->header.hasKey("data file"))
        par->datafile = replaceFileName(xstring(fmtHndl->fileName), par->header.get_value("data file"));

    return true;
}

bool rawGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return false;
    if (fmtHndl->internalParams == NULL) return false;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    *info = initImageInfo();
    info->number_pages = 1;
    info->samples = 1;
    info->imageMode = IM_MULTI;

    if (fmtHndl->subFormat == BIM_RAW_FORMAT_MHD) {
        return mhdGetImageInfo(fmtHndl);
    }
    if (fmtHndl->subFormat == BIM_RAW_FORMAT_NRRD) {
        return nrrdGetImageInfo(fmtHndl);
    }

    //fmtHndl->compression - offset
    //fmtHndl->quality - endian (0/1)  
    par->header_offset = 0;
    par->big_endian = false;

    if (fmtHndl->stream == NULL) return false;

    return true;
}

bool rawWriteImageHeader(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return false;
    if (fmtHndl->internalParams == NULL) return false;
    RawParams *par = (RawParams *)fmtHndl->internalParams;

    ImageBitmap *img = fmtHndl->image;
    if (img == NULL) return false;

    std::string infname = fmtHndl->fileName;
    if (infname.size() <= 1) return false;
    infname += ".info";

    std::string inftext = getImageInfoText(&img->i);


    FILE *stream;
    if ((stream = fopen(infname.c_str(), "wb")) != NULL) {
        fwrite(inftext.c_str(), sizeof(char), inftext.size(), stream);
        fclose(stream);
    }

    return true;
}

//----------------------------------------------------------------------------
// PARAMETERS, INITS
//----------------------------------------------------------------------------

#define BIM_FORMAT_RAW_MAGIC_SIZE 20

int rawValidateFormatProc(BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_RAW_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *)magic;

    if (memcmp(mag_num, "NRRD000", 7) == 0) return BIM_RAW_FORMAT_NRRD;
    if (fileName) {
        xstring filename(fileName);
        filename = filename.toLowerCase();
        if (filename.endsWith(".mhd")) return BIM_RAW_FORMAT_MHD;
    }

    return -1;
}

FormatHandle rawAquireFormatProc(void) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void rawCloseImageProc(FormatHandle *fmtHndl);
void rawReleaseFormatProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    rawCloseImageProc(fmtHndl);
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void rawCloseImageProc(FormatHandle *fmtHndl) {
    if (!fmtHndl) return;
    xclose(fmtHndl);
    bim::RawParams *par = (bim::RawParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint rawOpenImageProc(FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) rawCloseImageProc(fmtHndl);
    bim::RawParams *par = new bim::RawParams();
    fmtHndl->internalParams = (void *)par;

    fmtHndl->io_mode = io_mode;
    xopen(fmtHndl);
    if (!fmtHndl->stream) {
        rawCloseImageProc(fmtHndl);
        return 1;
    };

    if (io_mode == IO_READ) {
        if (!rawGetImageInfo(fmtHndl)) {
            rawCloseImageProc(fmtHndl);
            return 1;
        };
    }
    return 0;
}

//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint rawGetNumPagesProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    return par->i.number_pages;
}


ImageInfo rawGetImageInfoProc(FormatHandle *fmtHndl, bim::uint page_num) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// read
//----------------------------------------------------------------------------

template <typename T>
void read_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T * BIM_RESTRICT raw = (T *)in;
    T * BIM_RESTRICT p = (T *)out;
    raw += sample;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T * BIM_RESTRICT pp = p + x;
        T * BIM_RESTRICT rr = raw + x*samples;
        *pp = *rr;
    } // for x
}

static int read_raw_image(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    //ImageInfo *info = &par->i;  
    ImageBitmap *img = fmtHndl->image;
    ImageInfo *info = &img->i;

    //-------------------------------------------------
    // init the image
    //-------------------------------------------------

    if (!img->bits[0]) {
        if (allocImg(fmtHndl, &par->i, img) != 0) return 1;
    }

    //-------------------------------------------------
    // read image data
    //-------------------------------------------------
    unsigned int plane_size = getImgSizeInBytes(img);
    unsigned int cur_plane = fmtHndl->pageNumber;
    unsigned int header_size = par->header_offset;

    // seek past header size plus number of planes
    if (xseek(fmtHndl, header_size + plane_size*cur_plane*img->i.samples, SEEK_SET) != 0) return 1;

    if (!par->interleaved || img->i.samples == 1) {
        // read non-interleaved data
        for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
            if (xread(fmtHndl, img->bits[sample], 1, plane_size) != plane_size) return 1;
        }
    } else {
        // read interleaved data
        bim::uint64 buffer_sz = plane_size * img->i.samples;
        std::vector<unsigned char> buffer(buffer_sz);
        unsigned char *buf = (unsigned char *)&buffer[0];
        if (xread(fmtHndl, buf, 1, buffer_sz) != buffer_sz) return 1;

        for (int s = 0; s < img->i.samples; ++s) {
            // here we only care about the size of pixel and not its format
            if (img->i.depth == 8)
                read_channel<bim::uint8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 16)
                read_channel<bim::uint16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 32)
                read_channel<bim::uint32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 64)
                read_channel<bim::uint64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
        } // for sample
    }

    // swap endianness
    if (bim::bigendian != (int)par->big_endian)
    for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
        if (img->i.depth == 16)
            swapArrayOfShort((uint16*)img->bits[sample], plane_size / 2);
        else if (img->i.depth == 32)
            swapArrayOfLong((uint32*)img->bits[sample], plane_size / 4);
        else if (img->i.depth == 64)
            swapArrayOfDouble((float64*)img->bits[sample], plane_size / 8);
    }

    return 0;
}

static int read_raw_buffer(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageBitmap *img = fmtHndl->image;
    ImageInfo *info = &img->i;

    //-------------------------------------------------
    // init the image
    //-------------------------------------------------

    if (!img->bits[0]) {
        if (allocImg(fmtHndl, &par->i, img) != 0) return 1;
    }

    //-------------------------------------------------
    // read image data
    //-------------------------------------------------
    unsigned int plane_size = getImgSizeInBytes(img);
    unsigned int cur_plane = fmtHndl->pageNumber;

    // seek past header size plus number of planes
    unsigned int buf_offset = plane_size*cur_plane*img->i.samples;

    if (!par->interleaved || img->i.samples == 1) {
        // read non-interleaved data
        for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
            if (par->uncompressed.size() < buf_offset+plane_size) return 1;
            memcpy(img->bits[sample], &par->uncompressed[0] + buf_offset, plane_size);
        }
    }
    else {
        // read interleaved data
        bim::uint64 buffer_sz = plane_size * img->i.samples;
        if (par->uncompressed.size() < buf_offset + buffer_sz) return 1;

        for (int s = 0; s < img->i.samples; ++s) {
            // here we only care about the size of pixel and not its format
            if (img->i.depth == 8)
                read_channel<bim::uint8>(img->i.width, img->i.height, img->i.samples, s, &par->uncompressed[0] + buf_offset, img->bits[s]);
            else if (img->i.depth == 16)
                read_channel<bim::uint16>(img->i.width, img->i.height, img->i.samples, s, &par->uncompressed[0] + buf_offset, img->bits[s]);
            else if (img->i.depth == 32)
                read_channel<bim::uint32>(img->i.width, img->i.height, img->i.samples, s, &par->uncompressed[0] + buf_offset, img->bits[s]);
            else if (img->i.depth == 64)
                read_channel<bim::uint64>(img->i.width, img->i.height, img->i.samples, s, &par->uncompressed[0] + buf_offset, img->bits[s]);
        } // for sample
    }

    // swap endianness
    if (bim::bigendian != (int)par->big_endian)
    for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
        if (img->i.depth == 16)
            swapArrayOfShort((uint16*)img->bits[sample], plane_size / 2);
        else if (img->i.depth == 32)
            swapArrayOfLong((uint32*)img->bits[sample], plane_size / 4);
        else if (img->i.depth == 64)
            swapArrayOfDouble((float64*)img->bits[sample], plane_size / 8);
    }

    return 0;
}

static int read_mhd_image(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    if (!par->datastream) {
        FormatHandle h = bim::initFormatHandle();
        h.io_mode = IO_READ;
        h.fileName = (bim::Filename) par->datafile.c_str();
        xopen(&h);
        par->datastream = h.stream;
    }
    void *old_stream = fmtHndl->stream;
    fmtHndl->stream = par->datastream;
    int r = read_raw_image(fmtHndl);
    fmtHndl->stream = old_stream;
    return r;
}

// fix buffer deflate bug in zlib
const int zlib_uncompress_fix(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen) {
    z_stream stream;
    int err;

    stream.next_in = (z_const Bytef *)source;
    stream.avail_in = (uInt)sourceLen;
    // Check for source > 64K on 16-bit machine:
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit(&stream);
    if (err != Z_OK) return err;

    err = inflateReset2(&stream, 31);
    if (err != Z_OK) {
        inflateEnd(&stream);
        return err;
    }

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
        return err;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);
    return err;
}

static int read_nrrd_image(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    if (par->datafile.size()>0 && !par->datastream) {
        FormatHandle h = bim::initFormatHandle();
        h.io_mode = IO_READ;
        h.fileName = (bim::Filename) par->datafile.c_str();
        xopen(&h);
        par->datastream = h.stream;
    }
    void *old_stream = 0;
    if (par->datastream) {
        old_stream = fmtHndl->stream;
        fmtHndl->stream = par->datastream;
    }
    
    int r = 0;
    if (par->header.get_value("encoding") == "raw") {
        r = read_raw_image(fmtHndl);
    }
    else if (par->header.get_value("encoding") == "gzip" || par->header.get_value("encoding") == "gz") {
        // decompress the rest of the file in one shot
        if (par->uncompressed.size() < 1) {
            par->uncompressed.resize(info->width*info->height*(info->depth/8)*info->samples*info->number_pages, 0);

            unsigned int sz = bim::xsize(fmtHndl) - par->header_offset;
            std::vector<char> in(sz);
            xseek(fmtHndl, par->header_offset, SEEK_SET);
            bim::xread(fmtHndl, &in[0], 1, sz);
            uLongf outsz = par->uncompressed.size();
            if (zlib_uncompress_fix((Bytef*)&par->uncompressed[0], &outsz, (const Bytef*)&in[0], sz) != Z_OK)
                r = 1;
        }
        if (r == 0)
            r = read_raw_buffer(fmtHndl);
    }
    
    if (old_stream) {
        fmtHndl->stream = old_stream;
    }
    return r;
}

bim::uint rawReadImageProc(FormatHandle *fmtHndl, bim::uint page) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->stream == NULL) return 1;
    fmtHndl->pageNumber = page;

    if (fmtHndl->subFormat == BIM_RAW_FORMAT_MHD) {
        return read_mhd_image(fmtHndl);
    }
    if (fmtHndl->subFormat == BIM_RAW_FORMAT_NRRD) {
        return read_nrrd_image(fmtHndl);
    }
    return read_raw_image(fmtHndl);
}


//----------------------------------------------------------------------------
// metadata
//----------------------------------------------------------------------------

bim::uint raw_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    if (!hash) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    //----------------------------------------------------------------------------
    // Resolution
    //----------------------------------------------------------------------------
    hash->set_value(bim::PIXEL_RESOLUTION_X, par->res[0]);
    hash->set_value(bim::PIXEL_RESOLUTION_Y, par->res[1]);
    hash->set_value(bim::PIXEL_RESOLUTION_Z, par->res[2]);
    hash->set_value(bim::PIXEL_RESOLUTION_T, par->res[3]);

    if (par->units.size()>0) hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, par->units[0]);
    if (par->units.size()>1) hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, par->units[1]);
    if (par->units.size()>2) hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, par->units[2]);
    if (par->units.size()>3) hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, par->units[3]);

    //-------------------------------------------
    // channel names
    //-------------------------------------------
    for (unsigned int i = 0; i < info->samples; ++i) {
        hash->set_value(xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), xstring::xprintf("Channel %d", i));
    }

    bim::TagMap::const_iterator it;
    for (it = par->header.begin(); it != par->header.end(); ++it) {
        hash->append_tag(bim::CUSTOM_TAGS_PREFIX + (*it).first, (*it).second.as_string());
    }

    return 0;
}


//----------------------------------------------------------------------------
// write ITK MediaHeader
//----------------------------------------------------------------------------

const std::string mhdPixelFormat(ImageInfo *info) {
    if (info->depth == 8 && info->pixelType == bim::FMT_UNSIGNED)  return "MET_UCHAR";
    if (info->depth == 16 && info->pixelType == bim::FMT_UNSIGNED) return "MET_USHORT";
    if (info->depth == 32 && info->pixelType == bim::FMT_UNSIGNED) return "MET_ULONG";
    if (info->depth == 64 && info->pixelType == bim::FMT_UNSIGNED) return "MET_ULONG_LONG";
    if (info->depth == 8 && info->pixelType == bim::FMT_SIGNED)    return "MET_CHAR";
    if (info->depth == 16 && info->pixelType == bim::FMT_SIGNED)   return "MET_SHORT";
    if (info->depth == 32 && info->pixelType == bim::FMT_SIGNED)   return "MET_LONG";
    if (info->depth == 64 && info->pixelType == bim::FMT_SIGNED)   return "MET_LONG_LONG";
    if (info->depth == 32 && info->pixelType == bim::FMT_FLOAT)    return "MET_FLOAT";
    if (info->depth == 64 && info->pixelType == bim::FMT_FLOAT)    return "MET_DOUBLE";
    return "";
}

const int mhdWriteImageHeader(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageBitmap *img = fmtHndl->image;
    if (!img) return 1;
    ImageInfo *info = &img->i;

    xstring ofname = fmtHndl->fileName;
    ofname = ofname.replace(".mhd", ".raw");

    bim::xstring header = "ObjectType = Image\n";
    int nd = 2;
    if (info->number_z > 1) ++nd;
    if (info->number_t > 1) ++nd;
    header += xstring::xprintf("NDims = %d\n", nd);
    header += xstring::xprintf("DimSize = %d %d %d %d\n", info->width, info->height, info->number_z, info->number_t);
    if (bim::bigendian) header += "ElementByteOrderMSB = True\n"; else header += "ElementByteOrderMSB = False\n";

    // need to read meta
    //header += xstring::xprintf("ElementSize = %d %d %d %d\n", info->xRes, info->yRes, info->number_z, info->number_t);

    header += "ElementType = " + mhdPixelFormat(info) + "\n";
    header += xstring::xprintf("ElementNumberOfChannels = %d\n", info->samples);

    // very last element
    header += "HeaderSize = 0\n";
    header += xstring::xprintf("ElementDataFile = %s\n", ofname.c_str());

    if (xwrite(fmtHndl, (void *)header.c_str(), 1, header.size()) != header.size()) return 1;
    xflush(fmtHndl);
}

//----------------------------------------------------------------------------
// write NRRD Header
//----------------------------------------------------------------------------

const std::string nrrdPixelFormat(ImageInfo *info) {
    if (info->depth == 8 && info->pixelType == bim::FMT_UNSIGNED)  return "uint8";
    if (info->depth == 16 && info->pixelType == bim::FMT_UNSIGNED) return "uint16";
    if (info->depth == 32 && info->pixelType == bim::FMT_UNSIGNED) return "uint32";
    if (info->depth == 64 && info->pixelType == bim::FMT_UNSIGNED) return "uint64";
    if (info->depth == 8 && info->pixelType == bim::FMT_SIGNED)    return "int8";
    if (info->depth == 16 && info->pixelType == bim::FMT_SIGNED)   return "int16";
    if (info->depth == 32 && info->pixelType == bim::FMT_SIGNED)   return "int32";
    if (info->depth == 64 && info->pixelType == bim::FMT_SIGNED)   return "int64";
    if (info->depth == 32 && info->pixelType == bim::FMT_FLOAT)    return "float";
    if (info->depth == 64 && info->pixelType == bim::FMT_FLOAT)    return "double";
    return "";
}

const int nrrdWriteImageHeader(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageBitmap *img = fmtHndl->image;
    if (!img) return 1;
    ImageInfo *info = &img->i;

    bim::xstring header = "NRRD0004\n";
    int nd = 2;
    if (info->samples > 1) ++nd;
    if (info->number_z > 1) ++nd;
    if (info->number_t > 1) ++nd;
    //info->samples
    header += xstring::xprintf("dimension: %d\n", nd);
    if (info->samples>1)
        header += xstring::xprintf("sizes: %d %d %d %d %d\n", info->samples, info->width, info->height, info->number_z, info->number_t);
    else
        header += xstring::xprintf("sizes: %d %d %d %d\n", info->width, info->height, info->number_z, info->number_t);
    header += "type: " + nrrdPixelFormat(info) + "\n";
    if (bim::bigendian) header += "endian: big\n";
    header += "encoding: raw\n";

    //header += xstring::xprintf("spacings: %d %d %d %d\n", info->xRes, info->yRes, info->number_z, info->number_t);
    //header += xstring::xprintf("ElementNumberOfChannels = %d\n", info->samples);

    // very last element - empty line
    header += "\n";

    if (xwrite(fmtHndl, (void *)header.c_str(), 1, header.size()) != header.size()) return 1;
    xflush(fmtHndl);
}

//----------------------------------------------------------------------------
// write raw
//----------------------------------------------------------------------------

template <typename T>
void write_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T * BIM_RESTRICT raw = (T *)in;
    T * BIM_RESTRICT p = (T *)out;
    raw += sample;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T * BIM_RESTRICT pp = p + x;
        T * BIM_RESTRICT rr = raw + x*samples;
        *rr = *pp;
    } // for x
}

static int write_raw_image(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    RawParams *par = (RawParams *)fmtHndl->internalParams;
    ImageBitmap *img = fmtHndl->image;

    //-------------------------------------------------
    // write image data
    //-------------------------------------------------
    unsigned long plane_size = getImgSizeInBytes(img);

    if (!par->interleaved || img->i.samples == 1) {
        std::vector<unsigned char> buffer(plane_size);
        unsigned char *buf = (unsigned char *)&buffer[0];

        for (unsigned int sample = 0; sample < img->i.samples; ++sample) {
            memcpy(buf, img->bits[sample], plane_size);

            if (bim::bigendian != (int)par->big_endian) {
                if (img->i.depth == 16)
                    swapArrayOfShort((uint16*)buf, plane_size / 2);
                else if (img->i.depth == 32)
                    swapArrayOfLong((uint32*)buf, plane_size / 4);
                else if (img->i.depth == 64)
                    swapArrayOfDouble((float64*)buf, plane_size / 8);
            }

            if (xwrite(fmtHndl, buf, 1, plane_size) != plane_size) return 1;
        }
    }
    else {
        bim::uint64 buffer_sz = plane_size * img->i.samples;
        std::vector<unsigned char> buffer(buffer_sz);
        unsigned char *buf = (unsigned char *)&buffer[0];

        for (int s = 0; s < img->i.samples; ++s) {
            // here we only care about the size of pixel and not its format
            if (img->i.depth == 8)
                write_channel<bim::uint8>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 16)
                write_channel<bim::uint16>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 32)
                write_channel<bim::uint32>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);
            else if (img->i.depth == 64)
                write_channel<bim::uint64>(img->i.width, img->i.height, img->i.samples, s, buf, img->bits[s]);

            if (bim::bigendian != (int)par->big_endian) {
                if (img->i.depth == 16)
                    swapArrayOfShort((uint16*)buf, buffer_sz / 2);
                else if (img->i.depth == 32)
                    swapArrayOfLong((uint32*)buf, buffer_sz / 4);
                else if (img->i.depth == 64)
                    swapArrayOfDouble((float64*)buf, buffer_sz / 8);
            }

            if (xwrite(fmtHndl, buf, 1, buffer_sz) != buffer_sz) return 1;
        } // for sample
    }

    xflush(fmtHndl);
    return 0;
}

bim::uint rawWriteImageProc(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->stream == NULL) return 1;

    if (fmtHndl->subFormat == BIM_RAW_FORMAT_MHD && fmtHndl->pageNumber == 0)
        return mhdWriteImageHeader(fmtHndl);

    if (fmtHndl->subFormat == BIM_RAW_FORMAT_NRRD && fmtHndl->pageNumber == 0)
        nrrdWriteImageHeader(fmtHndl);

    return write_raw_image(fmtHndl);
}


//****************************************************************************
// EXPORTED
//****************************************************************************

#define BIM_RAW_NUM_FORMATS 3

FormatItem rawItems[BIM_RAW_NUM_FORMATS] = {
    {
        "RAW",              // short name, no spaces
        "RAW image pixels", // Long format name
        "raw",              // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        0, //canReadMeta;  // 0 - NO, 1 - YES
        0, //canWriteMeta; // 0 - NO, 1 - YES
        1, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 0, 0, 0, 0, 0 }
    }, {
        "NRRD",              // short name, no spaces
        "NRRD", // Long format name
        "nrrd",              // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        0, //canWriteMeta; // 0 - NO, 1 - YES
        1, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 0, 0, 0, 0, 0 }
    }, {
        "MHD",              // short name, no spaces
        "MHD", // Long format name
        "mhd",              // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        1, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        0, //canWriteMeta; // 0 - NO, 1 - YES
        1, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 0, 0, 0, 0, 0 }
    }
};

FormatHeader rawHeader = {

    sizeof(FormatHeader),
    "3.0.0",
    "RAW CODEC",
    "RAW CODEC",

    BIM_FORMAT_RAW_MAGIC_SIZE,                     // 0 or more, specify number of bytes needed to identify the file
    { 1, BIM_RAW_NUM_FORMATS, rawItems },   // dimJpegSupported,

    rawValidateFormatProc,
    // begin
    rawAquireFormatProc, //AquireFormatProc
    // end
    rawReleaseFormatProc, //ReleaseFormatProc

    // params
    NULL, //AquireIntParamsProc
    NULL, //LoadFormatParamsProc
    NULL, //StoreFormatParamsProc

    // image begin
    rawOpenImageProc, //OpenImageProc
    rawCloseImageProc, //CloseImageProc 

    // info
    rawGetNumPagesProc, //GetNumPagesProc
    rawGetImageInfoProc, //GetImageInfoProc

    // read/write
    rawReadImageProc, //ReadImageProc 
    rawWriteImageProc, //WriteImageProc
    NULL, //ReadImageTileProc
    NULL, //WriteImageTileProc
    NULL, //ReadImageLineProc
    NULL, //WriteImageLineProc
    NULL, //ReadImageThumbProc
    NULL, //WriteImageThumbProc
    NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc

    // meta data
    NULL, //ReadMetaDataProc
    NULL, //AddMetaDataProc
    NULL, //ReadMetaDataAsTextProc

    raw_append_metadata,
    NULL,
    NULL,
    ""

};

extern "C" {

    FormatHeader* rawGetFormatHeader(void) {
        return &rawHeader;
    }

} // extern C


