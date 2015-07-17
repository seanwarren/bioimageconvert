/*****************************************************************************
  NIFTI support 
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#ifndef BIM_NIFTI_FORMAT_H
#define BIM_NIFTI_FORMAT_H

#include <cstdio>
#include <string>
#include <fstream>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* niftiGetFormatHeader(void);
}

struct nifti_1_header;
struct _nifti_image;
typedef _nifti_image nifti_image;

namespace bim {

//----------------------------------------------------------------------------
// internal format defs
//----------------------------------------------------------------------------

class NIFTIParams {
public:
    static std::map< int, xstring > intents;
    static std::map< int, xstring > xforms;

public:
    NIFTIParams();
    ~NIFTIParams();

    ImageInfo i;
    nifti_1_header *header;
    nifti_image *nim;
};

} // namespace bim

#endif // BIM_NIFTI_FORMAT_H
