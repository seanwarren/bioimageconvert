/*****************************************************************************
    MRC format support
    Copyright (c) 2017 ViQi Inc
    Author: Dmitry Fedorov <dima@dimin.net>
    License: FreeBSD

    History:
    2017-05-30 - First creation
            
    Ver : 1
*****************************************************************************/

#ifndef BIM_MRC_FORMAT_H
#define BIM_MRC_FORMAT_H

#include <stdio.h>
#include <vector>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* mrcGetFormatHeader(void);
}

namespace bim {

#define BIM_MRC_MAGIC_SIZE 128
#define BIM_MRC_HEADER_SIZE 1024

#define MRC_MODE_INT8     0    // 8 - bit signed integer(range - 128 to 127)
#define MRC_MODE_INT16    1    // 16 - bit signed integer
#define MRC_MODE_FLOAT32  2    // 32 - bit signed real
#define MRC_MODE_CINT16   3    // transform : complex 16 - bit integers
#define MRC_MODE_CFLOAT32 4    // transform : complex 32 - bit reals
#define MRC_MODE_UINT16   6    // 16 - bit unsigned integer
#define MRC_MODE_INT32    7    // 32 - bit signed integer
#define MRC_MODE_RGB8     16   // RGB uint8 per channel
#define MRC_MODE_UINT4    101  // 4 - bit unsigned integer

#define MRC_SPACEGROUP_IMAGE_STACK  0
#define MRC_SPACEGROUP_VOLUME       1
#define MRC_SPACEGROUP_VOLUME_STACK 401

#pragma pack(push, 2)
typedef struct MrcHeader {
    bim::int32 nx;          // number of columns in 3D data array(fast axis)
    bim::int32 ny;          // number of rows in 3D data array(medium axis)
    bim::int32 nz;          // number of sections in 3D data array(slow axis)
    bim::int32 mode;        // pixel mode
    bim::int32 nxstart;     // location of first column in unit cell
    bim::int32 nystart;     // location of first row in unit cell
    bim::int32 nzstart;     // location of first section in unit cell
    bim::int32 mx;          // sampling along x axis of unit cell
    bim::int32 my;          // sampling along y axis of unit cell
    bim::int32 mz;          // sampling along z axis of unit cell
    bim::float32 xlen;       // cell dimensions in angstroms
    bim::float32 ylen;       // cell dimensions in angstroms
    bim::float32 zlen;       // cell dimensions in angstroms
    bim::float32 alpha;      // cell angles in degrees
    bim::float32 beta;       // cell angles in degrees
    bim::float32 gamma;      // cell angles in degrees
    bim::int32 mapc;        // axis corresp to cols(1, 2, 3 for x, y, z)
    bim::int32 mapr;        // axis corresp to rows(1, 2, 3 for x, y, z)
    bim::int32 maps;        // axis corresp to sections(1, 2, 3 for x, y, z)
    bim::float32 amin;       // minimum pixel value of all images in file
    bim::float32 amax;       // maximum pixel value of all images in file
    bim::float32 amean;      // mean pixel value of all images in file
    bim::int32 ispg;         // space group number, should be 0 for an image stack, 1 for a volume
                             // Type of data	ISPG	NZ	  MZ
                             // Single image	0	    1	  1
                             // Image stack	    0	    n	  >=1
                             // Single volume	1 	    m	  m
                             // Volume stack	>=401   n*m	  m
    bim::int32 next;         // This value gives the offset (in bytes) from the end of the file header to the first dataset (image). 
                             // Thus you will find the first image at 1024 + next bytes
    char extra[8];           // not used, first two bytes should be 0
    char extType[4];         // Type of extended header, includes 'SERI' for SerialEM, 'FEI1' for FEI, 'AGAR' for Agard, not used by FEI
    bim::int32 nversion;     // MRC version that file conforms to, otherwise 0, not used by FEI
    char extra2[84];         // 

    bim::float32 xorg;       // set to 0 : not used; origin of image
    bim::float32 yorg;       //     
    bim::float32 zorg;       // 

    char map[4];             // Contains 'MAP ' to identify file type 
    bim::uint8 machst[4];    // Machine stamp; identifies byte order 

    bim::float32 rms;        // RMS deviation of densities from mean density 

    bim::int32 nlabl;        // number of labels being used
    char label[10][80];      // character text labels, Label 0 is used for copyright information (FEI)
} MrcHeader;
#pragma pack(pop) 


//The extended header contains the information about a maximum of 1024 images.
//Each section is 128 bytes long.The extended header is thus 1024 * 128 bytes
//(always the same length, regardless of how many images are present.

#define BIM_FEI_EXT_NUM_IMG 1024

#pragma pack(push, 2)
typedef struct FEIHeaderExt {
    bim::float32 a_tilt;         // Alpha tilt, in degrees
    bim::float32 b_tilt;         // Beta tilt, in degrees
    bim::float32 x_stage;        // Stage x position.Normally in SI units(meters), but some older files may be in micrometers.
                                 // Check by looking at values for x, y, z.If one of these exceeds 1, it will be micrometers
    bim::float32 y_stage;        // Stage y position.For testing of units see x_stage.
    bim::float32 z_stage;        // Stage z position.For testing of units see x_stage.
    bim::float32 x_shift;        // Image shift x.For testing of units see x_stage.
    bim::float32 y_shift;        // Image shift y.For testing of units see x_stage.
    bim::float32 defocus;        // Defocus as read from microscope.For testing of units see x_stage.
    bim::float32 exp_time;       // Exposure time in seconds.
    bim::float32 mean_int;       // Mean value of image.
    bim::float32 tilt_axis;      // The orientation of the tilt axis in the image in degrees.
                                 // Vertical to the top is 0°, the direction of positive rotation is anti - clockwise.
    bim::float32 pixel_size;     // The pixel size of the images in SI units(meters).
    bim::float32 magnification;  // The magnification used for recording the images.
    bim::float32 ht;             // Value of the high tension in SI units(volts).
    bim::float32 binning;        // The binning of the CCD or STEM acquisition
    bim::float32 appliedDefocus; // The intended application defocus in SI units(meters), as defined for example in the tomography parameters view.
    bim::float32 remainder;      // Not used
    char padding[60];
} FEIHeaderExt;
#pragma pack(pop) 

class MrcParams {
public:
    MrcParams() { i = initImageInfo(); }

    bim::ImageInfo i;
    MrcHeader header;
    std::vector<FEIHeaderExt> exts;
    bim::uint64 data_offset;
};


} // namespace bim

#endif // BIM_MRC_FORMAT_H
