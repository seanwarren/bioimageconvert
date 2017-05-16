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

#ifndef BIM_RAW_FORMAT_H
#define BIM_RAW_FORMAT_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* rawGetFormatHeader(void);
}

namespace bim {

class RawParams {
public:
    RawParams();
    ~RawParams();

    ImageInfo i;
    unsigned int header_offset;
    bool big_endian;
    bool interleaved;

    std::vector<double> res;
    std::vector<xstring> units;
    xstring datafile;
    void *datastream;
    TagMap header;
    std::vector<char> uncompressed;
};

} // namespace bim

#endif // BIM_BMP_FORMAT_H
