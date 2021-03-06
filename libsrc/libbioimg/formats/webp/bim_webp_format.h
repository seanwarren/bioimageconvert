/*****************************************************************************
  WEBP support
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
  2013-01-12 14:13:40 - First creation

  ver : 1
  *****************************************************************************/

#ifndef BIM_WEBP_FORMAT_H
#define BIM_WEBP_FORMAT_H

#include <cstdio>
#include <string>
#include <fstream>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
    bim::FormatHeader* webpGetFormatHeader(void);
}

struct WebPMux;

namespace bim {
    //----------------------------------------------------------------------------
    // internal format defs
    //----------------------------------------------------------------------------

    class WEBPParams {
    public:
        WEBPParams();
        ~WEBPParams();

        ImageInfo i;
        std::fstream *file;

        std::vector<char> buffer_icc;
        std::vector<char> buffer_xmp;
        std::vector<char> buffer_iptc;
        std::vector<char> buffer_exif;

        WebPMux* mux;
        std::vector<unsigned char> buffer_file;
        void ensure_mux();
    };

} // namespace bim

#endif // BIM_WEBP_FORMAT_H
