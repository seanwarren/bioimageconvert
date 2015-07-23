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

#ifndef BIM_JP2_FORMAT_H
#define BIM_JP2_FORMAT_H

#include <cstdio>
#include <string>
#include <fstream>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
    bim::FormatHeader* jp2GetFormatHeader(void);
}

typedef void * opj_stream_t;
typedef void * opj_codec_t;
struct opj_image;
typedef struct opj_image opj_image_t;

namespace bim {
    //----------------------------------------------------------------------------
    // internal format defs
    //----------------------------------------------------------------------------

    class JP2Params {
    public:
        JP2Params();
        ~JP2Params();

        ImageInfo i;
        std::fstream *file;
        opj_stream_t *stream;
        opj_codec_t *codec;
        opj_image_t *image;

        void open(const char *filename, bim::ImageIOModes mode);
        
        
        std::vector<std::string> comments;
    };

} // namespace bim

#endif // BIM_JP2_FORMAT_H
