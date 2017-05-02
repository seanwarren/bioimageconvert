/*****************************************************************************
  DICOM support 
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#ifndef BIM_DICOM_FORMAT_H
#define BIM_DICOM_FORMAT_H

#include <cstdio>
#include <string>
#include <fstream>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* dicomGetFormatHeader(void);
}

namespace gdcm {
    class Image;
    class ImageReader;
    class ImageRegionReader;
}

namespace bim {

//----------------------------------------------------------------------------
// internal format defs
//----------------------------------------------------------------------------

class DICOMParams {

public:
    typedef enum {
        RGBRGBRGB = 0,
        RRRGGGBBB = 1,
    } PlanarConfig;

public:
    DICOMParams();
    ~DICOMParams();

    ImageInfo i;
    //gdcm::ImageReader *reader;
    gdcm::ImageRegionReader *reader;
    std::fstream *file; // only used in windows case to support UTF16 filenames
    //std::vector<char> buffer; // gdcm seems to be reading the whole 3d/4d image in memory at once, since we read by pages, keep this in memory
};

} // namespace bim

#endif // BIM_DICOM_FORMAT_H
