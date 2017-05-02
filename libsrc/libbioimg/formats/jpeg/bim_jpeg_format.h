/*****************************************************************************
  JPEG support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/22/2004 13:06 - First creation
    08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
    2010-06-24 15:11 - EXIF/IPTC extraction
        
  Ver : 3
*****************************************************************************/

#ifndef BIM_JPEG_FORMAT_H
#define BIM_JPEG_FORMAT_H

#include <bim_img_format_interface.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* jpegGetFormatHeader(void);
}

struct jpeg_decompress_struct;
struct my_jpeg_source_mgr; 
struct my_error_mgr;

namespace bim {

class JpegParams {
public:
    JpegParams();
    ~JpegParams();

    ImageInfo i;
    jpeg_decompress_struct *cinfo;
    my_jpeg_source_mgr *iod_src;
    my_error_mgr *jerr;

    std::vector<char> buffer_icc;
    std::vector<char> buffer_exif;
    std::vector<char> buffer_iptc;
    std::vector<char> buffer_xmp;
    std::vector<std::string> comments;
};

} // namespace bim

#endif // BIM_JPEG_FORMAT_H
