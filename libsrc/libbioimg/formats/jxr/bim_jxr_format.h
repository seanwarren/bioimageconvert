/*****************************************************************************
  JXR support
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
  2013-01-12 14:13:40 - First creation

  ver : 1
  *****************************************************************************/

#ifndef BIM_JXR_FORMAT_H
#define BIM_JXR_FORMAT_H

#include <cstdio>
#include <string>
#include <fstream>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
    bim::FormatHeader* jxrGetFormatHeader(void);
}

struct WMPStream;
struct tagPKImageDecode;
struct tagPKImageEncode;
typedef struct tagPKImageDecode PKImageDecode;
typedef struct tagPKImageEncode PKImageEncode;

namespace bim {
    //----------------------------------------------------------------------------
    // internal format defs
    //----------------------------------------------------------------------------

    class JXRParams {
    public:
        JXRParams();
        ~JXRParams();

        ImageInfo i;
        std::fstream *file;
        WMPStream *pStream;
        PKImageDecode *pDecoder;
        PKImageEncode *pEncoder;
        int frames_written;
        
        std::vector<char> buffer_icc;
        std::vector<char> buffer_xmp;
        std::vector<char> buffer_iptc;
        std::vector<char> buffer_photoshop;

        std::vector<char> buffer_exif;
        unsigned int offset_exif;
        std::vector<char> buffer_exifgps;
        unsigned int offset_exifgps;

        void open(const char *filename, bim::ImageIOModes mode);
    };

} // namespace bim

#endif // BIM_JXR_FORMAT_H
