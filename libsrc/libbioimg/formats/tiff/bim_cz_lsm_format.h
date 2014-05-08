/*****************************************************************************
  Carl Zeiss LSM definitions 
  Copyright (c) 2006 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23 - First creation

  Ver : 1
*****************************************************************************/

#ifndef BIM_CZ_LSM_FORMAT_H
#define BIM_CZ_LSM_FORMAT_H

#include <vector>
#include <map>

#include <bim_img_format_interface.h>

namespace bim {

//----------------------------------------------------------------------------
// Internal LSM Structures
//----------------------------------------------------------------------------

#define UINT8 uint8
#define UINT16 uint16
#define UINT32 uint32
#define SINT32 int32
#define FLOAT64 float64


// LSM v1.3?
#pragma pack(push, 1)
typedef struct CZ_LSMINFO_13 {
  UINT32   u32MagicNumber;
  SINT32   s32StructureSize;
  SINT32   s32DimensionX;              
  SINT32   s32DimensionY;
  SINT32   s32DimensionZ;
  SINT32   s32DimensionChannels;
  SINT32   s32DimensionTime;
  SINT32   s32DataType;                
  SINT32   s32ThumbnailX;               
  SINT32   s32ThumbnailY;
  FLOAT64  f64VoxelSizeX;               
  FLOAT64  f64VoxelSizeY;               
  FLOAT64  f64VoxelSizeZ;              
  UINT32   u32ScanType;
  UINT32   u32DataType;
  UINT32   u32OffsetVectorOverlay;
  UINT32   u32OffsetInputLut;
  UINT32   u32OffsetOutputLut;
  UINT32   u32OffsetChannelColors; // points to CZ_ChannelColors
  FLOAT64  f64TimeIntervall;
  UINT32   u32OffsetChannelDataTypes;
  UINT32   u32OffsetScanInformation; // points to CZ_ScanInformation
  UINT32   u32OffsetKsData;
  UINT32   u32OffsetTimeStamps; // points to CZ_TimeStamps
  UINT32   u32OffsetEventList;
  UINT32   u32OffsetRoi;
  UINT32   u32OffsetBleachRoi;
  UINT32   u32OffsetNextRecording;
  UINT32   u32Reserved [ 90 ];
} CZ_LSMINFO_13;
#pragma pack(pop)

// LSM v1.5-6.0
#pragma pack(push, 1)
typedef struct CZ_LSMINFO {
    UINT32 u32MagicNumber;       // 0x00300494C (release 1.3) or 0x00400494C (release 1.5 to 6.0).
    SINT32 s32StructureSize;     // Number of bytes in the structure.
    SINT32 s32DimensionX;        //Number of intensity values in x-direction.
    SINT32 s32DimensionY;        //Number of intensity values in y-direction.
    SINT32 s32DimensionZ;        //Number of intensity values in z-direction or in case of scan mode “Time Series Mean-of-ROIs” the Number of ROIs.
    SINT32 s32DimensionChannels; //Number of channels.
    SINT32 s32DimensionTime;     //Number of intensity values in time-direction.
    SINT32 s32DataType;          //Format of the intensity values: 
                                 //  1 for 8-bit unsigned integer,
                                 //  2 for 12-bit unsigned integer and5 for 32-bit float (for “Time Series Mean-of-ROIs” or topography height map images)
                                 //  0 in case of different data types for different channels. In the latter case the field 32OffsetChannelDataTypes contains further information.
    SINT32 s32ThumbnailX;        //Width in pixels of a thumbnail.
    SINT32 s32ThumbnailY;        //Height in pixels of a thumbnail.
    FLOAT64 f64VoxelSizeX;       //Distance of the pixels in x-direction in meter.
    FLOAT64 f64VoxelSizeY;       //Distance of the pixels in y-direction in meter.
    FLOAT64 f64VoxelSizeZ;       //Distance of the pixels in z-direction in meter.
    FLOAT64 f64OriginX;          //The x-offset of the center of the image in meter.
                                 //relative to the optical axis. For LSM images the x-direction is the direction of the x-scanner. 
                                 //For cameras it is the CCD horizontal direction (depending on how camera is aligned/mounted)
                                 //In releases prior 4.0 the entry was not used and the value 0 was written instead.
    FLOAT64 f64OriginY;          //The y-offset of the center of the image in meter.
                                 //relative to the optical axis. For LSM images the y-direction is the direction of the y-scanner. 
                                 //For cameras it is the CCD vertical direction (depending on how camera is aligned/mounted).
                                 //In releases prior 4.0 the entry was not used and the value 0 was written instead.
    FLOAT64 f64OriginZ;          //Not used
    UINT16 u16ScanType;          //Scan type:
                                 //0 - normal x-y-z-scan
                                 //1 - z-Scan (x-z-plane)
                                 //2 - line scan
                                 //3 - time series x-y
                                 //4 - time series x-z (release 2.0 or later)
                                 //5 - time series “Mean of ROIs” (release 2.0 or later)
                                 //6 - time series x-y-z (release 2.3 or later)
                                 //7 - spline scan (release 2.5 or later)
                                 //8 - spline plane x-z (release 2.5 or later)
                                 //9 - time series spline plane x-z (release 2.5 or later)
                                 //10 - point mode (release 3.0 or later)
    UINT16 u16SpectralScan;      //Spectral scan flag:
                                 //0 – no spectral scan.
                                 //1 – image has been acquired in spectral scan mode with a LSM 510 META or LSM 710 QUASAR detector (release 3.0 or later).
    UINT32 u32DataType;          //Data type:
                                 //0 - Original scan data
                                 //1 - Calculated data
                                 //2 - 3D reconstruction
                                 //3 - Topography height map
    UINT32 u32OffsetVectorOverlay; //File offset to the description of the vector overlay (can be 0, if not present).
    UINT32 u32OffsetInputLut;    //File offset to the channel input LUT with brightness and contrast properties (can be 0, if not present).
    UINT32 u32OffsetOutputLut;   //File offset to the color palette (can be 0, if not present).
    UINT32 u32OffsetChannelColors; //File offset to the list of channel colors and channel names (can be 0, if not present).
    FLOAT64 f64TimeIntervall;    //Time interval for time series in “s” (can be 0, if it is not a time series or if there is more detailed information in u32OffsetTimeStamps).
    UINT32 u32OffsetChannelDataTypes; //File offset to an array with UINT32-values with the format of the intensity values for the respective channels (can be 0, if not present). 
                                 //The contents of the array elements are
                                 //  1 - for 8-bit unsigned integer,
                                 //  2 - for 12-bit unsigned integer and
                                 //  5 - for 32-bit float (for “Time Series Mean-of-ROIs” ).
    UINT32 u32OffsetScanInformation; //File offset to a structure with information of the device settings used to scan the image (can be 0, if not present).
    UINT32 u32OffsetKsData;      //File offset to “Zeiss Vision KS-3D” specific data (can be 0, if not present).
    UINT32 u32OffsetTimeStamps;  //File offset to a structure containing the time stamps for the time indexes (can be 0, if it is not a time series).
    UINT32 u32OffsetEventList;   //File offset to a structure containing the experimental notations recorded during a time series (can be 0, if not present).
    UINT32 u32OffsetRoi;         //File offset to a structure containing a list of the ROIs used during the scan operation (can be 0, if not present).
    UINT32 u32OffsetBleachRoi;   //File offset to a structure containing a description of the bleach region used during the scan operation (can be 0, if not present).
    UINT32 u32OffsetNextRecording; //For “Time Series Mean-of-ROIs” and for „Line scans“ it is possible that a second image is stored in the file (can be 0, if not present). 
                                 //For “Time Series Mean-of-ROIs” it is an image with the ROIs. For „Line scans“ it is the image with the selected line.
                                 //In these cases u32OffsetNextRecording contains a file offset to a second file header. 
                                 //This TIFF-header and all sub-structures are built exactly the same way as a simple LSM 5/7 file. 
                                 //All offsets without exception are given there relative to the start of the second TIFF-header.
    FLOAT64 f64DisplayAspectX;   //Zoom factor for the image display in x-direction (0.0 for release 2.3 and earlier).
    FLOAT64 f64DisplayAspectY;   //Zoom factor for the image display in y-direction (0.0 for release 2.3 and earlier).
    FLOAT64 f64DisplayAspectZ;   //Zoom factor for the image display in z-direction (0.0 for release 2.3 and earlier).
    FLOAT64 f64DisplayAspectTime; //Zoom factor for the image display in time-direction (0.0 for release 2.3 and earlier).
    UINT32 u32OffsetMeanOfRoisOverlay; //File offset to the description of the vector overlay with the ROIs used during a scan in “Mean of ROIs” mode (can be 0, if not present).
    UINT32 u32OffsetTopoIsolineOverlay; //File offset to the description of the vector overlay for the topography–iso–lines and height display with the profile selection line (can be 0, if not present).
    UINT32 u32OffsetTopoProfileOverlay; //File offset to the description of the vector overlay for the topography–profile display (can be 0, if not present).
    UINT32 u32OffsetLinescanOverlay; //File offset to the description of the vector overlay for the line scan line selection with the selected line or Bezier curve (can be 0, if not present).
    UINT32 u32ToolbarFlags;      //Bit-field for disabled toolbar buttons:
                                 //bit 0 - “Corp” button
                                 //bit 1 - “Reuse” button.
                                 //If the bit is set the corresponding button is disabled.
    UINT32 u32OffsetChannelWavelength; //Offset to memory block with the wavelength range used during acquisition for the individual channels (new for release 3.0; can be 0, if not present).
    UINT32 u32OffsetChannelFactors; // Offset to memory block with scaling factor, offset and unit for each image channel. The data are currently used by images with ion concentration data. 
                                    // The display value is calculated by
                                    //DisplayValue = Factor * PixelIntensity / MaxPixelIntensity + Offset.
                                    //where "MaxPixelIntensity" is the maximum possible pixel intensity for the data type (4095 or 255).
                                    //The parameters are stored in an array of structures LSMCHANNELFACTORS (24 bytes per channel) with the members:
                                    //    FLOAT64 f64Factor; 
                                    //    FLOAT64 f64Offset; 
                                    //    UINT32 u32Unit; // eUnitNone – unknown or eUnitConcentration - Ion concentration in mol
                                    //    UINT32 u32Reserved [3];
    FLOAT64 f64ObjectiveSphereCorrection; //The inverse radius of the spherical error of the objective that was used during acquisition. 
                                    //This is the radius of the sphere that can be fitted on the topography reconstruction of an absolutely plane object that has been recorded with the objective.
    UINT32 u32OffsetUnmixParameters; //File offset to the parameters for linear unmixing that have been used to generate the image data from scan data of the spectral detector 
                                    //(new for release 3.2; can be 0, if not present).
    UINT32 u32OffsetAcquisitionParameters; //File offset to a block with acquisition parameters for support of the re-use function of the LSM 5/7 program (new for release 3.5; can be 0, if not present).
    UINT32 u32OffsetCharacteristics; //File offset to a block with user specified properties (new for release 3.5; can be 0, if not present).
    UINT32 u32OffsetPalette;         //File offset to a block with detailed color palette properties (new for release 3.5; can be 0, if not present).
    FLOAT64 f64TimeDifferenceX;      //The time difference for the acquisition of adjacent pixels in x-direction in seconds. The property is used by RICS analysis (new for release 5.0; can be 0, if not present).
    FLOAT64 f64TimeDifferenceY;      //The time difference for the acquisition of adjacent pixels in y-direction in seconds. The property is used by RICS analysis (new for release 5.0; can be 0, if not present).
    FLOAT64 f64TimeDifferenceZ;      //The time difference for the acquisition of adjacent pixels in z-direction in seconds. The property is used by RICS analysis (new for release 5.0; can be 0, if not present).
    UINT32 u32InternalUse1;          //Reserved for internal use. Writer should set this field to 0.
    SINT32 s32DimensionP;            //Number of intensity values in position-direction. (new for release 5.5; can be 0, if not present).
    SINT32 s32DimensionM;            //Number of intensity values in tile (mosaic)-direction.(new for release 5.5; can be 0, if not present).
    SINT32 s32DimensionsReserved[16]; //16 reserved 32-bit words, must be 0.
    UINT32 u32OffsetTilePositions;   //File offset to a block with the positions of the tiles in (new for release 5.5; can be 0, if not present).
    UINT32 u32Reserved [9];          //9 reserved 32-bit words, must be 0.
    UINT32 u32OffsetPositions;       //File offset to a block with the positions of the acquisition regions 
                                     //(new in release 6.2, can be 0, if not present, release 6.2 is NOT released yet! beta and special built only).
    UINT32 u32Reserved2 [21];         //21 reserved 32-bit words, must be 0.
} CZ_LSMINFO;
#pragma pack(pop)



#pragma pack(push, 1)
typedef struct CZ_ChannelColors {
  SINT32 s32BlockSize;    // Size of the structure in bytes including the name strings and colors.
  SINT32 s32NumberColors; // Number of colors in the color array; should be the same as the number of channels.
  SINT32 s32NumberNames;  // Number of character strings for the channel names; should be the same as the number of channels.
  SINT32 s32ColorsOffset; // Offset relative ti the start of the structure to the “UINT32” array of channel colors. 
                          // Each array entry contains a color with intensity values in the range 0..255 for the three color components
  SINT32 s32NamesOffset;  // Offset relative ti the start of the structure to the list of channel names. The list of channel names is a series of “\0”-terminated ANSI character strings.
  SINT32 s32Mono;         // If unequal zero the “Mono” button in the LSM-imagefenster  window was peressed
} CZ_ChannelColors;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CZ_ChannelColor {
  UINT8 r, g, b, a;
} CZ_ChannelColor;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CZ_Positions {
  UINT32 u32Tiles;    
} CZ_Positions;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CZ_Position {
    FLOAT64 x, y, z;
} CZ_Position;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CZ_TimeStamps {
    SINT32  s32Size;              // Size, in bytes, of the whole block used for time stamps.
    SINT32  s32NumberTimeStamps;	// Number of time stamps in the following list.
} CZ_TimeStamps;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CZ_TimeStamp {
    FLOAT64 timeStamp; // Time stamps in seconds relative to the start time of the LSM electronic unit controller program
} CZ_TimeStamp;
#pragma pack(pop)

// Data types
#define TYPE_SUBBLOCK 0
#define TYPE_ASCII    2
#define TYPE_LONG     4
#define TYPE_RATIONAL 5

//Every subblock starts with a list entry of the type TYPE_SUBBLOCK. The u32Entry field can be:

#define SUBBLOCK_RECORDING             0x10000000
#define SUBBLOCK_TRACKS				         0x20000000
#define SUBBLOCK_LASERS                0x30000000
#define SUBBLOCK_TRACK 				         0x40000000
#define SUBBLOCK_LASER				         0x50000000
#define SUBBLOCK_DETECTION_CHANNELS    0x60000000
#define SUBBLOCK_DETECTION_CHANNEL 	   0x70000000
#define SUBBLOCK_ILLUMINATION_CHANNELS 0x80000000
#define SUBBLOCK_ILLUMINATION_CHANNEL  0x90000000
#define SUBBLOCK_BEAM_SPLITTERS		     0xA0000000
#define SUBBLOCK_BEAM_SPLITTER 		     0xB0000000
#define SUBBLOCK_DATA_CHANNELS	       0xC0000000
#define SUBBLOCK_DATA_CHANNEL          0xD0000000
#define SUBBLOCK_TIMERS    		         0x11000000
#define SUBBLOCK_TIMER      		    	 0x12000000
#define SUBBLOCK_MARKERS               0x13000000
#define SUBBLOCK_MARKER                0x14000000
#define SUBBLOCK_END                   0xFFFFFFFF


#pragma pack(push, 1)
typedef struct CZ_ScanInformation {
  UINT32 u32Entry; // A value that specifies which data are stored
  UINT32 u32Type;  //	A value that specifies the type of the data stored in the "Varaibable length data" field.
                   // TYPE_SUBBLOCK	- start or end of a subblock
                   // TYPE_LONG		  - 32 bit signed integer
                   // TYPE_RATIONAL - 64 bit floatingpoint 
                   // TYPE_ASCII 		- zero terminated string.
  UINT32 u32Size;	 // Size, in bytes, of the "Varaibable length data" field.
} CZ_ScanInformation;
#pragma pack(pop)

#undef UINT8
#undef UINT16
#undef UINT32
#undef SINT32
#undef FLOAT64

//----------------------------------------------------------------------------
// LsmScanInfoEntry
//----------------------------------------------------------------------------
class LsmScanInfoEntry {
public:
  LsmScanInfoEntry(): entry_type(0), data_type(0) {}

  unsigned int entry_type;
  unsigned int data_type;
  std::vector<unsigned char> data;
  std::string path;

public:
  int readEntry (TIFF *tif, uint32 offset);
  inline size_t offsetSize() const;
};

//----------------------------------------------------------------------------
// TIFF Codec-wise Fluoview Structure 
//----------------------------------------------------------------------------
class LsmInfo {
public:
  LsmInfo(): ch(0), z_slices(0), t_frames(0), pages(0) {}

  int ch;        // number of channels in each image of the sequence
  uint64 z_slices;  // number of Z slices for each time instance
  uint64 t_frames;  // number of time frames in the sequence
  uint64 pages;     // the value of consequtive images generated by driver
  double res[4]; // pixel resolution for XYZT
  
  CZ_LSMINFO         lsm_info; // actual header structure retreived form the image
  CZ_TimeStamps      lsm_TimeStamps;
  
  CZ_ChannelColors   lsm_colors;
  std::vector<CZ_ChannelColor> channel_colors;
  std::vector<std::string>     channel_names;
  std::vector<int> display_lut; // rgb -> chan numbers

  std::vector<CZ_Position> positions;
  std::vector<CZ_Position> tile_positions;
  std::vector<CZ_TimeStamp> time_stamps;

  std::vector< LsmScanInfoEntry > scan_info_entries;
  std::map< unsigned int, std::string > key_names;
  std::string objective;
  std::string name;

  //internal
  uint64 pages_tiff;
};

} // namespace bim

#endif // BIM_CZ_LSM_FORMAT_H

