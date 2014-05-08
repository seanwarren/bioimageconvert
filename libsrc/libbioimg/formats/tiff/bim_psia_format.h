/*****************************************************************************
  PSIA definitions
  
  Writen by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>
  
  History:
    04/20/2004 23:33 - First creation
    10/12/2005 15:03 - updates for 64 bit machines     
        
  Ver : 2
*****************************************************************************/

#ifndef BIM_PSIA_FORMAT_H
#define BIM_PSIA_FORMAT_H

#include <bim_img_format_interface.h>

namespace bim {

#define BIM_PSIA_OFFSET_SOURCENAME   4
#define BIM_PSIA_OFFSET_IMAGEMODE    68
#define BIM_PSIA_OFFSET_LPFSSTRENGTH 84
#define BIM_PSIA_OFFSET_AUTOFLATTEN  92
#define BIM_PSIA_OFFSET_ACTRACK      96
#define BIM_PSIA_OFFSET_WIDTH        100
#define BIM_PSIA_OFFSET_HEIGHT       104
#define BIM_PSIA_OFFSET_ANGLE        108
#define BIM_PSIA_OFFSET_SINESCAN     116
#define BIM_PSIA_OFFSET_OVERSCAN     120
#define BIM_PSIA_OFFSET_FASTSCANDIR  128
#define BIM_PSIA_OFFSET_SLOWSCANDIR  132
#define BIM_PSIA_OFFSET_XYSWAP       136
#define BIM_PSIA_OFFSET_XSCANSIZE    140
#define BIM_PSIA_OFFSET_YSCANSIZE    148
#define BIM_PSIA_OFFSET_XOFFSET      156
#define BIM_PSIA_OFFSET_YOFFSET      164
#define BIM_PSIA_OFFSET_SCANRATE     172
#define BIM_PSIA_OFFSET_SETPOINT     180
#define BIM_PSIA_OFFSET_SETPOINTUNIT 188
#define BIM_PSIA_OFFSET_TIPBIAS      204
#define BIM_PSIA_OFFSET_SAMPLEBIAS   212
#define BIM_PSIA_OFFSET_DATAGAIN     220
#define BIM_PSIA_OFFSET_ZSCALE       228
#define BIM_PSIA_OFFSET_ZOFFSET      236
#define BIM_PSIA_OFFSET_UNIT         244
#define BIM_PSIA_OFFSET_DATAMIN      260
#define BIM_PSIA_OFFSET_DATAMAX      264
#define BIM_PSIA_OFFSET_DATAAVG      268
#define BIM_PSIA_OFFSET_NCOMPRESSION 272

typedef struct psiaInfoHeader {
  char    szSourceNameW[32]; // Topography, ZDetector etc.
  char    szImageModeW[8];   //AFM, NCM etc.
  float64 dfLPFStrength;     //float64 dfLPFStrength; // LowPass filter strength.
  int32   bAutoFlatten;      // Automatic flatten after imaging.
  int32   bACTrack;          // AC Track
  int32   nWidth;            // Image size: nWidth: Number of Columns
  int32   nHeight;           //Image size: nHeight: Number of Rows.
  float64 dfAngle;           //Angle of Fast direction about positive x-axis.
  int32   bSineScan;         // Sine Scan
  float64 dfOverScan;        // Overscan rate
  int32   bFastScanDir;      //non-zero when scan up, 0 for scan down.
  int32   bSlowScanDir;      //non-zero for forward, 0 for backward.
  int32   bXYSwap;           //Swap fast-slow scanning dirctions.
  float64 dfXScanSize;       //X, Y Scan size in um.
  float64 dfYScanSize;
  float64 dfXOffset;         // X,Y offset in microns.
  float64 dfYOffset;
  float64 dfScanRate;        // Scan speed in rows per second.
  float64 dfSetPoint;        // Error signal set point.
  char    szSetPointUnitW[8];
  float64 dtTipBias;         // Tip Bias Voltage
  float64 dfSampleBias;      // Sample Bias Voltage
  float64 dfDataGain;        // Data Gain
  float64 dfZScale;          // Scale , now it is always 1.
  float64 dfZOffset;         // Z Offset in step, now it is always 0.
  char    szUnitW[8];
  int32   nDataMin;
  int32   nDataMax;
  int32   nDataAvg;
  int32   ncompression;
} psiaInfoHeader;

} // namespace bim

#endif // BIM_PSIA_FORMAT_H
