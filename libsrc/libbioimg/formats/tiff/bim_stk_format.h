/*****************************************************************************
  STK definitions
  
  Writen by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>
  
  History:
    04/20/2004 23:33 - First creation
        
  Ver : 1
*****************************************************************************/


#ifndef BIM_STK_FORMAT_H
#define BIM_STK_FORMAT_H

#include "bim_tiff_format.h"

namespace bim {

#define BIM_STK_AutoScale              0
#define BIM_STK_MinScale               1
#define BIM_STK_MaxScale               2
#define BIM_STK_SpatialCalibration     3
#define BIM_STK_XCalibration           4
#define BIM_STK_YCalibration           5
#define BIM_STK_CalibrationUnits       6
#define BIM_STK_Name                   7
#define BIM_STK_ThreshState            8
#define BIM_STK_ThreshStateRed         9
//#define BIM_STK_ThreshStateRed        10
#define BIM_STK_ThreshStateGreen       11
#define BIM_STK_ThreshStateBlue        12
#define BIM_STK_ThreshStateLo          13
#define BIM_STK_ThreshStateHi          14
#define BIM_STK_Zoom                   15
#define BIM_STK_CreateTime             16
#define BIM_STK_LastSavedTime          17
#define BIM_STK_currentBuffer          18
#define BIM_STK_grayFit                19
#define BIM_STK_grayPointCount         20
#define BIM_STK_grayX                  21
#define BIM_STK_grayY                  22
#define BIM_STK_grayMin                23
#define BIM_STK_grayMax                24
#define BIM_STK_grayUnitName           25
#define BIM_STK_StandardLUT            26
#define BIM_STK_wavelength             27
#define BIM_STK_StagePosition          28
#define BIM_STK_CameraChipOffset       29
#define BIM_STK_OverlayMask            30
#define BIM_STK_OverlayCompress        31
#define BIM_STK_Overlay                32
#define BIM_STK_SpecialOverlayMask     33
#define BIM_STK_SpecialOverlayCompress 34
#define BIM_STK_SpecialOverlay         35
#define BIM_STK_ImageProperty          36 
#define BIM_STK_StageLabel             37
#define BIM_STK_AutoScaleLoInfo        38
#define BIM_STK_AutoScaleHiInfo        39
#define BIM_STK_AbsoluteZ              40
#define BIM_STK_AbsoluteZValid         41
#define BIM_STK_Gamma                  42
#define BIM_STK_GammaRed               43
#define BIM_STK_GammaGreen             44
#define BIM_STK_GammaBlue              45
#define BIM_STK_CameraBin              46
#define BIM_STK_NewLUT                 47
#define BIM_STK_PlaneProperty          49
/*
#define BIM_STK_ImagePropertyEx        48
#define BIM_STK_UserLutTable           50
#define BIM_STK_RedAutoScaleInfo       51
#define BIM_STK_RedAutoScaleLoInfo     52
#define BIM_STK_RedAutoScaleHiInfo     53
#define BIM_STK_RedMinScaleInfo        54
#define BIM_STK_RedMaxScaleInfo        55
#define BIM_STK_GreenAutoScaleInfo     56
#define BIM_STK_GreenAutoScaleLoInfo   57
#define BIM_STK_GreenAutoScaleHiInfo   58
#define BIM_STK_GreenMinScaleInfo      59
#define BIM_STK_GreenMaxScaleInfo      60
#define BIM_STK_BlueAutoScaleInfo      61
#define BIM_STK_BlueAutoScaleLoInfo    62
#define BIM_STK_BlueAutoScaleHiInfo    63
#define BIM_STK_BlueMinScaleInfo       64
#define BIM_STK_BlueMaxScaleInfo       65
#define BIM_STK_OverlayPlaneColor      66
*/

static int stk_tag_sizes_long[67] =
{
   1, 1, 1, 1,
   2, 2,
   1, // contains the size of following string in bytes
   1, // contains the size of following string in bytes
   1, 1, 1, 1,
   1, 1, 1, 2,
   2, 1, 1, 1,
   2, 2, 2, 2,
   1, // contains the size of following string in bytes
   1, 1,
   4, // 4*N longs
   4, // 4*N longs
   1, 1, 1, 1, 1, 1, 1,
   1, // N longs
   2, 2,
   2, // 2*N longs
   1, // N longs
   1, 1, 1, 1,
   2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

typedef struct StkRational {
  int32 num;
  int32 den;
} StkRational;

typedef struct StkPlaneProperty {
  int8 type;
  char *Key;
  char *Value;
  StkRational rational;
} StkPlaneProperty;

// each dynamic array here is f size N
typedef struct StkMetaData {
  int N;

  // UIC1 and UIC4
  int32 AutoScale;
  int32 MinScale;
  int32 MaxScale;
  int32 SpatialCalibration;
  int32 XCalibration[2];
  int32 YCalibration[2];
  char *CalibrationUnits;
  char *Name;
  int32 ThreshState;
  uint32 ThreshStateRed;
  uint32 ThreshStateGreen;
  uint32 ThreshStateBlue;
  uint32 ThreshStateLo;
  uint32 ThreshStateHi;
  int32 Zoom;
  int32 CreateTime[2];
  int32 LastSavedTime[2];
  int32 currentBuffer;
  int32 grayFit;
  int32 grayPointCount;
  int32 grayX[2];
  int32 grayY[2];
  int32 grayMin[2];
  int32 grayMax[2];
  char *grayUnitName;
  int32 StandardLUT;
  int32 UIC1wavelength; // discard it, don't read, the UIC3 value should be used
  int32 CameraBin[2];
  int32 NewLUT;
/*
  int32 ImagePropertyEx;
  int32 UserLutTable;
  int32 RedAutoScaleInfo;
  int32 RedAutoScaleLoInfo;
  int32 RedAutoScaleHiInfo;
  int32 RedMinScaleInfo;
  int32 RedMaxScaleInfo;
  int32 GreenAutoScaleInfo;
  int32 GreenAutoScaleLoInfo;
  int32 GreenAutoScaleHiInfo;
  int32 GreenMinScaleInfo;
  int32 GreenMaxScaleInfo;
  int32 BlueAutoScaleInfo;
  int32 BlueAutoScaleLoInfo;
  int32 BlueAutoScaleHiInfo;
  int32 BlueMinScaleInfo;
  int32 BlueMaxScaleInfo;
  int32 OverlayPlaneColor;
*/
  std::vector<StkPlaneProperty> PlaneProperties;
  
  // begin: Used internally by MetaMorph
  int32 OverlayMask;
  int32 OverlayCompress;
  int32 Overlay;
  int32 SpecialOverlayMask;
  int32 SpecialOverlayCompress;
  int32 SpecialOverlay;
  int32 ImageProperty;
  // end: Used internally by MetaMorph

  int32 AutoScaleLoInfo[2];
  int32 AutoScaleHiInfo[2];
  int32 Gamma; 
  int32 GammaRed; 
  int32 GammaGreen; 
  int32 GammaBlue; 

  // UIC3
  StkRational *wavelength;

  // UIC2
  StkRational *zDistance;
  int32 *creationDate;
  int32 *creationTime;
  int32 *modificationDate;
  int32 *modificationTime;

  //UIC4
  StkRational *StagePositionX;
  StkRational *StagePositionY;
  StkRational *CameraChipOffsetX;
  StkRational *CameraChipOffsetY;
  char *StageLabel; 
  StkRational *AbsoluteZ; 
  int32 *AbsoluteZValid; 

} StkMetaData;


class StkInfo {
public:
  StkInfo();
  ~StkInfo();

  tiff_strp_t strips_per_plane; // strips per plane
  tiff_offs_t *strip_offsets; // offsets of each strip
  tiff_bcnt_t *strip_bytecounts; // offsets of each strip
  StkMetaData metaData;

public:
  void init();
  void clear();
  void clearOffsets();
  void allocMetaInfo( int32 N );
};

} // namespace bim 

#endif // BIM_STK_FORMAT_H
