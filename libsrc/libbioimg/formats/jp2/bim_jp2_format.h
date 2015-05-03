/*****************************************************************************
  JPEG2000 support 
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION

  Programmer: Mario Emmenlauer <mario@emmenlauer.de>

  History:
    04/19/2015 14:20 - First creation
        
  Ver : 1
*****************************************************************************/

#ifndef BIM_JP2_FORMAT_H
#define BIM_JP2_FORMAT_H

#include <vector>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>


// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* jp2GetFormatHeader(void);
}

namespace bim {

class Jp2Params {
public:
    Jp2Params();
    ~Jp2Params();

  ImageInfo i;

  // metadata
  std::vector<std::string> comments;
};

} // namespace bim

#endif // BIM_JP2_FORMAT_H
