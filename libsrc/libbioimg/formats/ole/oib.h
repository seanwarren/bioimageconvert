/*****************************************************************************
  Olympus Image Binary (OIB) format 
  Copyright (c) 2008, Center for Bio Image Informatics, UCSB
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2008-06-04 14:26:14 - First creation
    2008-09-15 19:04:47 - Fix for older files with unordered streams
    2008-11-06 13:36:43 - Parse preferred channel mapping
            
  Ver : 3
*****************************************************************************/

#ifndef BIM_OIB_FORMAT_H
#define BIM_OIB_FORMAT_H

#include <cstdio>
#include <vector>
#include <string>
#include <map>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

#include <tag_map.h>

//----------------------------------------------------------------------------
// Internal Format Structs
//----------------------------------------------------------------------------

namespace bim {

namespace oib {

class Axis {
public:
  Axis(): MaxSize(0), StartPosition(0.0), EndPosition(0.0) { }

  int MaxSize;

  double StartPosition;
  double EndPosition;

  std::string AxisCode;
  std::string AxisName;
  std::string PixUnit;
  std::string UnitName;

  //CalibrateValueA=100.0
  //CalibrateValueB=0.0
  //ClipPosition=0

  bool isValid() const;
};

class Params {
public:
  Params(): num_z(0), num_t(0), num_l(0) { }
  ~Params() {}
  
  int num_z, num_t, num_l;

  std::string oif_metadata;
  std::string oifFolderName;
  std::string oifFileName;

  std::vector< double > pixel_resolution;
  std::vector<std::string> channel_names;
  std::vector<int> display_lut; // rgb -> chan numbers
  std::vector<bim::DisplayColor> channel_mapping; // channel -> rgb 

  TagMap oib_info_hash;
  TagMap oif_metadata_hash;

  std::vector< Axis > axis;
};
 
} // namespace oib

} // namespace bim

#endif // BIM_OIB_FORMAT_H
