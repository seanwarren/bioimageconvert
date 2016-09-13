/*****************************************************************************
  Digital Camera RAW formats support 
  Copyright (c) 2012, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2012, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#ifndef BIM_DCRAW_FORMAT_H
#define BIM_DCRAW_FORMAT_H

#include <cstdio>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* dcrawGetFormatHeader(void);
}

class LibRaw;
namespace bim {

//----------------------------------------------------------------------------
// internal format defs
//----------------------------------------------------------------------------

class DCRawParams {
public:
  DCRawParams();
  ~DCRawParams();

  ImageInfo i;
  LibRaw *processor;
};

} // namespace bim

#endif // BIM_DCRAW_FORMAT_H
