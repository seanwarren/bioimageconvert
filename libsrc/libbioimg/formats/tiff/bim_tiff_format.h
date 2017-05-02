/*****************************************************************************
  TIFF support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  Notes:
    Metadata can be red using readMetaData but can not br written to the file
    using addMetaData, it must be supplied with the formatHandler within
    writeImage function.

  History:
    03/29/2004 22:23 - First creation
        
  Ver : 1
*****************************************************************************/

#ifndef BIM_TIFF_FORMAT_H
#define BIM_TIFF_FORMAT_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

#include <tiffvers.h>
#include <tiffio.h>

// libtiff 3.8.2
#if (TIFFLIB_VERSION == 20060323)
typedef tsize_t  tiff_size_t;
typedef tdata_t  tiff_data_t;
typedef toff_t   tiff_offs_t;
typedef uint32   tiff_bcnt_t;
typedef tstrip_t tiff_strp_t;
#endif

// libtiff 3.9.2
#if (TIFFLIB_VERSION == 20091104)
typedef tsize_t  tiff_size_t;
typedef tdata_t  tiff_data_t;
typedef toff_t   tiff_offs_t;
typedef toff_t   tiff_bcnt_t;
typedef tstrip_t tiff_strp_t;
#endif

// libtiff 3.9.4
#if (TIFFLIB_VERSION == 20100615)
typedef tsize_t  tiff_size_t;
typedef tdata_t  tiff_data_t;
typedef toff_t   tiff_offs_t;
typedef toff_t   tiff_bcnt_t;
typedef tstrip_t tiff_strp_t;
#endif

// libtiff 4.0.X
#if (TIFFLIB_VERSION >= 20100101)
typedef tmsize_t tiff_size_t;
typedef void*    tiff_data_t;
typedef uint64   tiff_offs_t;
typedef uint64   tiff_bcnt_t;
typedef uint32   tiff_strp_t;
#endif

#include "bim_tiny_tiff.h"
#include "bim_psia_format.h"
#include "bim_stk_format.h"
#include "bim_fluoview_format.h"
#include "bim_cz_lsm_format.h"
#include "bim_ometiff_format.h"

#include <tif_dir.h>

namespace bim {

const unsigned char d_magic_tiff_CLLT[4] = {0x4d, 0x4d, 0x00, 0x2a};
const unsigned char d_magic_tiff_CLBG[4] = {0x49, 0x49, 0x2a, 0x00};
const unsigned char d_magic_tiff_MDLT[4] = {0x50, 0x45, 0x00, 0x2a};
const unsigned char d_magic_tiff_MDBG[4] = {0x45, 0x50, 0x2a, 0x00};
const unsigned char d_magic_tiff_BGLT[4] = {0x4d, 0x4d, 0x00, 0x2b};
const unsigned char d_magic_tiff_BGBG[4] = {0x49, 0x49, 0x2b, 0x00};

typedef enum {
  tstGeneric    = 0,
  tstStk        = 1,
  tstPsia       = 2,
  tstFluoview   = 3,
  tstCzLsm      = 4,
  tstOmeTiff    = 5,
  tstBigTiff    = 6,
  tstOmeBigTiff = 7,
  tstAndor      = 8
} BIM_TiffSubType;

class PyramidInfo {
public:
    static const int min_level_size = 128;

    enum Format {
        pyrFmtNone = 0,
        pyrFmtSubDirs = 1,
        pyrFmtTopDirs = 2
    };


public:
    PyramidInfo();
    Format format;
    unsigned int number_levels;
    std::vector<double> scales;
    std::vector<bim::uint64> directory_offsets;
public:
    void init();
    void addLevel(const double &scale, const bim::uint64 &offset = 0);
};

class TiffParams {
public:
  TiffParams();

  ImageInfo info;

  TIFF  *tiff;
  BIM_TiffSubType subType;
  TinyTiff::Tiff ifds;
  PyramidInfo pyramid;

  StkInfo stkInfo;
  psiaInfoHeader psiaInfo;
  FluoviewInfo fluoviewInfo;
  LsmInfo lsmInfo;
  OMETiffInfo omeTiffInfo;
};

} // namespace bim

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* tiffGetFormatHeader(void);
}

#endif // BIM_TIFF_FORMAT_H
