/*****************************************************************************
  FLUOVIEW definitions 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23 - First creation
    10/12/2005 15:03 - updates for 64 bit machines       

  Ver : 2
*****************************************************************************/

#ifndef BIM_FLUOVIEW_FORMAT_H
#define BIM_FLUOVIEW_FORMAT_H

#include <vector>
#include <string>

#include <bim_img_format_interface.h>

namespace bim {

//----------------------------------------------------------------------------
// Internal Fluoview Structures
//----------------------------------------------------------------------------

#define BIM_MMHEADER    34361
#define BIM_MMSTAMP     34362
#define BIM_ANDORBLOCK  34363 // ???
#define BIM_MMUSERBLOCK 34386

#define IMAGE_NAME_LENGTH   256
#define SPATIAL_DIMENSION   10
#define DIMNAME_LENGTH      16
#define UNITS_LENGTH        64

typedef uint32 MM_HANDLE;      // Size (bytes):     4

#pragma pack(push, 1)
typedef struct MM_BIM_INFO {
  char     Name[DIMNAME_LENGTH]; //Dimension name e.g. Width   16
  uint32   Size;                 //Image width etc              4
  float64  Origin;               //Origin                       8
  float64  Resolution;           //Image resolution             8
  char     Units[UNITS_LENGTH];  //Image calibration units     64
} MM_BIM_INFO;                        // Total Size (bytes):       100  

typedef struct MM_HEAD {
  int16     HeaderFlag;                 //Size of header structure             2
  uchar     ImageType;                  //Image Type                           1
  char      Name[IMAGE_NAME_LENGTH];    //Image name                         256
  uchar     Status;                     //image status                         1
  uint32    Data;                       //Handle to the data field             4
  uint32    NumberOfColors;             //Number of colors in palette          4
  uint32    MM_256_Colors;              //handle to the palette field          4
  uint32    MM_All_Colors;              //handle to the palette field          4
  uint32    CommentSize;                //Size of comments field               4
  uint32       Comment;                    //handle to the comment field          4
  MM_BIM_INFO  DimInfo[SPATIAL_DIMENSION]; //Dimension Info                    1000    
  uint32    SpatialPosition;            //obsolete???????????                  4
  int16     MapType;                    //Display mapping type                 2
  //short         reserved;                   //Display mapping type                 2
  float64    MapMin;                     //Display mapping minimum              8
  float64    MapMax;                     //Display mapping maximum              8
  float64    MinValue;                   //Image histogram minimum              8
  float64    MaxValue;                   //Image histogram maximum              8
  MM_HANDLE     Map;                        //Handle to gray level mapping array   4
  float64    Gamma;                      //Image gray level correction factor   8
  float64    Offset;                     //Image gray level correction offset   8
  MM_BIM_INFO   Gray;                       //                                   100
  MM_HANDLE     ThumbNail;                  //handle to the ThumbNail field        4
  uint32    UserFieldSize;              //Size of Voice field                  4
  MM_HANDLE     UserFieldHandle;            //handle to the Voice field            4
} MM_HEAD;                                  // Total Size (bytes):              1456
#pragma pack(pop)

//----------------------------------------------------------------------------
// TIFF Codec-wise Fluoview Structure 
//----------------------------------------------------------------------------
class FluoviewInfo {
public:
  FluoviewInfo(): ch(0), z_slices(0), t_frames(0), pages(0), xR(0), yR(0), zR(0), pages_tiff(0) {}

  int ch;       // number of channels in each image of the sequence
  uint64 z_slices; // number of Z slices for each time instance
  uint64 t_frames; // number of time frames in the sequence
  uint64 pages;    // the value of consequtive images generated by driver
  double xR, yR, zR; // pixel resolution for XYZ
  MM_HEAD head; // actual header structure retreived form the image

  //internal
  uint64 pages_tiff;
  std::vector<std::string> sample_names;
  std::vector<int> display_lut; // rgb -> chan numbers
  std::vector<bim::DisplayColor> channel_mapping; // channel -> rgb 
};

} // namespace bim

#endif // BIM_FLUOVIEW_FORMAT_H

