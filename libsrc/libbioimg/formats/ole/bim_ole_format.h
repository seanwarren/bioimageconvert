/*****************************************************************************
  Olympus Image Binary (OIB) format support
  Copyright (c) 2008, Center for Bio Image Informatics, UCSB
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2008-06-04 14:26:14 - First creation
    2008-09-15 19:04:47 - Fix for older files with unordered streams
    2008-11-06 13:36:43 - Parse preferred channel mapping
            
  Ver : 3
*****************************************************************************/

#ifndef BIM_OLE_FORMAT_H
#define BIM_OLE_FORMAT_H

#include <cstdio>
#include <vector>
#include <string>
#include <map>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>
#include <tag_map.h>

#include <pole.h>
#include "oib.h"
#include "zvi.h"

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* oleGetFormatHeader(void);
}

namespace bim {

namespace ole {

enum FORMAT {
  FORMAT_UNKNOWN = 0,
  FORMAT_OIB     = 1,
  FORMAT_ZVI     = 2
}; 

#define BIM_OLE_MAGIC_SIZE 8
const unsigned char magic[BIM_OLE_MAGIC_SIZE]  = { 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1 };

class Params {
public:
  Params(): storage(NULL), ole_format(FORMAT_UNKNOWN) { i = initImageInfo(); }
  ~Params() { if (storage) delete storage; }
  
  ImageInfo i;
  POLE::Storage *storage;
  FORMAT ole_format; 

  oib::Params oib_params;
  zvi::Directory zvi_dir;
};
 
} // namespace ole

} // namespace bim

#endif // BIM_OLE_FORMAT_H
