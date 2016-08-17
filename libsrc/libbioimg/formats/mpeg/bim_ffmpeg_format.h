/*****************************************************************************
  FFMPEG support 
  Copyright (c) 2008 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2008-02-01 14:45 - First creation
        
  Ver : 1
*****************************************************************************/

#ifndef BIM_FFMPEG_FORMAT_H
#define BIM_FFMPEG_FORMAT_H

#include <cstdio>
#include <vector>
#include <map>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

#include "FfmpegIVideo.h"
#include "FfmpegOVideo.h"

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* ffMpegGetFormatHeader(void);
}

namespace bim {

class FFMpegParams {
public:
  ImageInfo i;
  VideoIO::FfmpegIVideo ff_in;
  VideoIO::FfmpegOVideo ff_out;
  bool frame_sizes_set;
  float fps;
  int bitrate;

public:
  static std::vector< std::string > formats_write;
  static std::vector< std::string > formats_fourcc;
  static std::vector< AVCodecID > encoder_ids;
  static std::map< std::string, int > formats;
  static std::map< std::string, int > codecs;
  static std::vector<int> format_bitrate_scaler;
  static std::vector<int> default_bitrates;
  static std::map< int, float > fps_overrides; // some formats do not support setting fps, override for those
};

} // namespace bim

#endif // BIM_FFMPEG_FORMAT_H
