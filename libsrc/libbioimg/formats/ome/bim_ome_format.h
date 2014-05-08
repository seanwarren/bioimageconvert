/*****************************************************************************
  OME XML file format (Open Microscopy Environment)
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  DEFINITIONS
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    11/21/2005 15:43 - First creation
            
  Ver : 1
*****************************************************************************/

#ifndef BIM_OME_FORMAT_H
#define BIM_OME_FORMAT_H

#include <cstdio>
#include <vector>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* omeGetFormatHeader(void);
}

namespace bim {

/*****************************************************************************
  OME XML file format - quick reference

  OME/src/xml/schemas/BinaryFile.xsd

*****************************************************************************/

#define BIM_OME_MAGIC_SIZE 6

const char omeMagic[BIM_OME_MAGIC_SIZE] = "<?xml";

class OmeParams {
public:
    OmeParams();
    ~OmeParams();

    ImageInfo i;
};

} // namespace bim

#endif // BIM_OME_FORMAT_H
