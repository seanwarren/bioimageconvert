/*****************************************************************************
  BIORAD PIC support 
  UCSB/BioITR property
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/22/2004 13:06 - First creation
    09/12/2005 17:34 - updated to api version 1.3
        
  Ver : 5
*****************************************************************************/

#ifndef BIM_BIORADPIC_FORMAT_H
#define BIM_BIORADPIC_FORMAT_H

#include <cstdio>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* bioRadPicGetFormatHeader(void);
}

namespace bim {

class BioRadPicParams {
public:
  BioRadPicParams();

  ImageInfo i;
  uint64 num_images;

  uint64 page_size_bytes;
  uint64 data_offset;
  uint64 notes_offset;
  uint64 has_notes;


  // metadata
  int magnification;  
  std::string datetime;
  double pixel_size[10]; 

  std::string note01;  
  std::string note20;
  std::string note21;
};

} // namespace bim

#endif // BIM_BIORADPIC_FORMAT_H
