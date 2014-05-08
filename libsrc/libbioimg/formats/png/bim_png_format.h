/*****************************************************************************
  PNG support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    07/29/2004 18:09 - First creation
    03/28/2013 11:51 - Update to libpng 1.5.14
        
  Ver : 2
*****************************************************************************/

#ifndef BIM_PNG_FORMAT_H
#define BIM_PNG_FORMAT_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

#include <png.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* pngGetFormatHeader(void);
}

namespace bim {

class PngParams {
public:
    PngParams();
    ~PngParams();

    ImageInfo i;
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
};

} // namespace bim

#endif // BIM_PNG_FORMAT_H
