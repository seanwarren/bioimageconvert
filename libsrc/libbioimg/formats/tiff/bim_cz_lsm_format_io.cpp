/*****************************************************************************
  Carl Zeiss LSM IO 
  Copyright (c) 2006 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23 - First creation
        
  Ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <algorithm>
#include <list>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_img_format_utils.h>

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"
#include "memio.h"

void read_text_tag(TinyTiff::IFD *ifd, bim::uint tag, MemIOBuf *outIOBuf);

using namespace bim;

unsigned int tiffGetNumberOfPages( TiffParams *tiffpar );

//----------------------------------------------------------------------------
// PSIA MISC FUNCTIONS
//----------------------------------------------------------------------------

void initMetaHash(LsmInfo *lsm) {
  if (lsm->key_names.size()>0) return;
  lsm->key_names[0x10000001] = "Name";
  lsm->key_names[0x4000000c] = "Name";
  lsm->key_names[0x50000001] = "Name";
  lsm->key_names[0x90000001] = "Name";
  lsm->key_names[0x90000005] = "Detection Channel Name";
  lsm->key_names[0xb0000003] = "Name";
  lsm->key_names[0xd0000001] = "Name";
  lsm->key_names[0x12000001] = "Name";
  lsm->key_names[0x14000001] = "Name";
  lsm->key_names[0x10000002] = "Description";
  lsm->key_names[0x14000002] = "Description";
  lsm->key_names[0x10000003] = "Notes";
  lsm->key_names[0x10000004] = "Objective";
  lsm->key_names[0x10000005] = "Processing Summary";
  lsm->key_names[0x10000006] = "Special Scan Mode";
  lsm->key_names[0x10000007] = "Scan Type";
  lsm->key_names[0x10000008] = "Scan Mode";
  lsm->key_names[0x10000009] = "Number of Stacks";
  lsm->key_names[0x1000000a] = "Lines Per Plane";
  lsm->key_names[0x1000000b] = "Samples Per Line";
  lsm->key_names[0x1000000c] = "Planes Per Volume";
  lsm->key_names[0x1000000d] = "Images Width";
  lsm->key_names[0x1000000e] = "Images Height";
  lsm->key_names[0x1000000f] = "Number of Planes";
  lsm->key_names[0x10000010] = "Number of Stacks";
  lsm->key_names[0x10000011] = "Number of Channels";
  lsm->key_names[0x10000012] = "Linescan XY Size";
  lsm->key_names[0x10000013] = "Scan Direction";
  lsm->key_names[0x10000014] = "Time Series";
  lsm->key_names[0x10000015] = "Original Scan Data";
  lsm->key_names[0x10000016] = "Zoom X";
  lsm->key_names[0x10000017] = "Zoom Y";
  lsm->key_names[0x10000018] = "Zoom Z";
  lsm->key_names[0x10000019] = "Sample 0X";
  lsm->key_names[0x1000001a] = "Sample 0Y";
  lsm->key_names[0x1000001b] = "Sample 0Z";
  lsm->key_names[0x1000001c] = "Sample Spacing";
  lsm->key_names[0x1000001d] = "Line Spacing";
  lsm->key_names[0x1000001e] = "Plane Spacing";
  lsm->key_names[0x1000001f] = "Plane Width";
  lsm->key_names[0x10000020] = "Plane Height";
  lsm->key_names[0x10000021] = "Volume Depth";
  lsm->key_names[0x10000034] = "Rotation";
  lsm->key_names[0x10000035] = "Precession";
  lsm->key_names[0x10000036] = "Sample 0Time";
  lsm->key_names[0x10000037] = "Start Scan Trigger In";
  lsm->key_names[0x10000038] = "Start Scan Trigger Out";
  lsm->key_names[0x10000039] = "Start Scan Event";
  lsm->key_names[0x10000040] = "Start Scan Time";
  lsm->key_names[0x10000041] = "Stop Scan Trigger In";
  lsm->key_names[0x10000042] = "Stop Scan Trigger Out";
  lsm->key_names[0x10000043] = "Stop Scan Event";
  lsm->key_names[0x10000044] = "Stop Scan Time";
  lsm->key_names[0x10000045] = "Use ROIs";
  lsm->key_names[0x10000046] = "Use Reduced Memory ROIs";
  lsm->key_names[0x10000047] = "User";
  lsm->key_names[0x10000048] = "Use B|C Correction";
  lsm->key_names[0x10000049] = "Position B|C Contrast 1";
  lsm->key_names[0x10000050] = "Position B|C Contrast 2";
  lsm->key_names[0x10000051] = "Interpolation Y";
  lsm->key_names[0x10000052] = "Camera Binning";
  lsm->key_names[0x10000053] = "Camera Supersampling";
  lsm->key_names[0x10000054] = "Camera Frame Width";
  lsm->key_names[0x10000055] = "Camera Frame Height";
  lsm->key_names[0x10000056] = "Camera Offset X";
  lsm->key_names[0x10000057] = "Camera Offset Y";
  lsm->key_names[0x40000001] = "Multiplex Type";
  lsm->key_names[0x40000002] = "Multiplex Order";
  lsm->key_names[0x40000003] = "Sampling Mode";
  lsm->key_names[0x40000004] = "Sampling Method";
  lsm->key_names[0x40000005] = "Sampling Number";
  lsm->key_names[0x40000006] = "Acquire";
  lsm->key_names[0x50000002] = "Acquire";
  lsm->key_names[0x7000000b] = "Acquire";
  lsm->key_names[0x90000004] = "Acquire";
  lsm->key_names[0xd0000017] = "Acquire";
  lsm->key_names[0x40000007] = "Sample Observation Time";
  lsm->key_names[0x40000008] = "Time Between Stacks";
  lsm->key_names[0x4000000d] = "Collimator 1 Name";
  lsm->key_names[0x4000000e] = "Collimator 1 Position";
  lsm->key_names[0x4000000f] = "Collimator 2 Name";
  lsm->key_names[0x40000010] = "Collimator 2 Position";
  lsm->key_names[0x40000011] = "Is Bleach Track";
  lsm->key_names[0x40000012] = "Bleach After Scan Number";
  lsm->key_names[0x40000013] = "Bleach Scan Number";
  lsm->key_names[0x40000014] = "Trigger In";
  lsm->key_names[0x12000004] = "Trigger In";
  lsm->key_names[0x14000003] = "Trigger In";
  lsm->key_names[0x40000015] = "Trigger Out";
  lsm->key_names[0x12000005] = "Trigger Out";
  lsm->key_names[0x14000004] = "Trigger Out";
  lsm->key_names[0x40000016] = "Is Ratio Track";
  lsm->key_names[0x40000017] = "Bleach Count";
  lsm->key_names[0x40000018] = "SPI Center Wavelength";
  lsm->key_names[0x40000019] = "Pixel Time";
  lsm->key_names[0x40000020] = "ID Condensor Frontlens";
  lsm->key_names[0x40000021] = "Condensor Frontlens";
  lsm->key_names[0x40000022] = "ID Field Stop";
  lsm->key_names[0x40000023] = "Field Stop Value";
  lsm->key_names[0x40000024] = "ID Condensor Aperture";
  lsm->key_names[0x40000025] = "Condensor Aperture";
  lsm->key_names[0x40000026] = "ID Condensor Revolver";
  lsm->key_names[0x40000027] = "Condensor Revolver";
  lsm->key_names[0x40000028] = "ID Transmission Filter 1";
  lsm->key_names[0x40000029] = "ID Transmission 1";
  lsm->key_names[0x40000030] = "ID Transmission Filter 2";
  lsm->key_names[0x40000031] = "ID Transmission 2";
  lsm->key_names[0x40000032] = "Repeat Bleach";
  lsm->key_names[0x40000033] = "Enable Spot Bleach Pos";
  lsm->key_names[0x40000034] = "Spot Bleach Position X";
  lsm->key_names[0x40000035] = "Spot Bleach Position Y";
  lsm->key_names[0x40000036] = "Bleach Position Z";
  lsm->key_names[0x50000003] = "Power";
  lsm->key_names[0x90000002] = "Power";
  lsm->key_names[0x70000003] = "Detector Gain";
  lsm->key_names[0x70000005] = "Amplifier Gain";
  lsm->key_names[0x70000007] = "Amplifier Offset";
  lsm->key_names[0x70000009] = "Pinhole Diameter";
  lsm->key_names[0x7000000c] = "Detector Name";
  lsm->key_names[0x7000000d] = "Amplifier Name";
  lsm->key_names[0x7000000e] = "Pinhole Name";
  lsm->key_names[0x7000000f] = "Filter Set Name";
  lsm->key_names[0x70000010] = "Filter Name";
  lsm->key_names[0x70000013] = "Integrator Name";
  lsm->key_names[0x70000014] = "Detection Channel Name";
  lsm->key_names[0x70000015] = "Detector Gain B|C 1";
  lsm->key_names[0x70000016] = "Detector Gain B|C 2";
  lsm->key_names[0x70000017] = "Amplifier Gain B|C 1";
  lsm->key_names[0x70000018] = "Amplifier Gain B|C 2";
  lsm->key_names[0x70000019] = "Amplifier Offset B|C 1";
  lsm->key_names[0x70000020] = "Amplifier Offset B|C 2";
  lsm->key_names[0x70000021] = "Spectral Scan Channels";
  lsm->key_names[0x70000022] = "SPI Wavelength Start";
  lsm->key_names[0x70000023] = "SPI Wavelength End";
  lsm->key_names[0x70000026] = "Dye Name";
  lsm->key_names[0xd0000014] = "Dye Name";
  lsm->key_names[0x70000027] = "Dye Folder";
  lsm->key_names[0xd0000015] = "Dye Folder";
  lsm->key_names[0x90000003] = "Wavelength";
  lsm->key_names[0x90000006] = "Power B|C 1";
  lsm->key_names[0x90000007] = "Power B|C 2";
  lsm->key_names[0xb0000001] = "Filter Set";
  lsm->key_names[0xb0000002] = "Filter";
  lsm->key_names[0xd0000004] = "Color";
  lsm->key_names[0xd0000005] = "Sample Type";
  lsm->key_names[0xd0000006] = "Bits Per Sample";
  lsm->key_names[0xd0000007] = "Ratio Type";
  lsm->key_names[0xd0000008] = "Ratio Track 1";
  lsm->key_names[0xd0000009] = "Ratio Track 2";
  lsm->key_names[0xd000000a] = "Ratio Channel 1";
  lsm->key_names[0xd000000b] = "Ratio Channel 2";
  lsm->key_names[0xd000000c] = "Ratio Const. 1";
  lsm->key_names[0xd000000d] = "Ratio Const. 2";
  lsm->key_names[0xd000000e] = "Ratio Const. 3";
  lsm->key_names[0xd000000f] = "Ratio Const. 4";
  lsm->key_names[0xd0000010] = "Ratio Const. 5";
  lsm->key_names[0xd0000011] = "Ratio Const. 6";
  lsm->key_names[0xd0000012] = "Ratio First Images 1";
  lsm->key_names[0xd0000013] = "Ratio First Images 2";
  lsm->key_names[0xd0000016] = "Spectrum";
  lsm->key_names[0x12000003] = "Interval";
}

//----------------------------------------------------------------------------
// PSIA MISC FUNCTIONS
//----------------------------------------------------------------------------

bool lsmIsTiffValid(TIFF *tif) {
  if (tif->tif_flags&TIFF_BIGTIFF) return false;      
  char *b_list = NULL;
  bim::int16 d_list_count;
  int res[3] = {0,0,0};

  if (tif == 0) return false;
  res[0] = TIFFGetField(tif, TIFFTAG_CZ_LSMINFO, &d_list_count, &b_list);

  if (res[0] == 1) return true;
  return false;
}


bool lsmIsTiffValid(TiffParams *tiffParams) {
  if (tiffParams == NULL) return false;
  if (tiffParams->tiff->tif_flags&TIFF_BIGTIFF) return false;    
  return tiffParams->ifds.tagPresentInFirstIFD(TIFFTAG_CZ_LSMINFO);
}

void doSwabLSMINFO_13(CZ_LSMINFO_13 *b) {
  TIFFSwabArrayOfLong( (bim::uint32*) &b->u32MagicNumber, 10 );
  TIFFSwabArrayOfDouble( (double*) &b->f64VoxelSizeX, 3 );
  TIFFSwabArrayOfLong( (bim::uint32*) &b->u32ScanType, 6 );
  TIFFSwabDouble( (double*) &b->f64TimeIntervall );
  TIFFSwabArrayOfLong( (bim::uint32*) &b->u32OffsetChannelDataTypes, 8 );
}

void doSwabLSMINFO(CZ_LSMINFO *b) {
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32MagicNumber, 10 );
  TIFFSwabArrayOfDouble( (double*) &b->f64VoxelSizeX, 6 );
  TIFFSwabArrayOfShort(  (bim::uint16*) &b->u16ScanType, 2 );
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32DataType, 5 );
  TIFFSwabArrayOfDouble( (double*) &b->f64TimeIntervall, 1 );
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32OffsetChannelDataTypes, 8 );
  TIFFSwabArrayOfDouble( (double*) &b->f64DisplayAspectX, 4 );
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32OffsetMeanOfRoisOverlay, 7 );
  TIFFSwabArrayOfDouble( (double*) &b->f64ObjectiveSphereCorrection, 1 );
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32OffsetUnmixParameters, 4 );
  TIFFSwabArrayOfDouble( (double*) &b->f64TimeDifferenceX, 3 );
  TIFFSwabArrayOfLong(   (bim::uint32*) &b->u32InternalUse1, 7 );
}

void doSwabLSMCOLORS(CZ_ChannelColors *b) {
  TIFFSwabArrayOfLong( (bim::uint32*) &b->s32BlockSize, 6 );
}

void lsm_read_ScanInformation (TiffParams *tiffParams);
void lsm_read_ChannelColors (TiffParams *par);
void lsm_read_Positions (TiffParams *par, bim::uint32 offset, std::vector<CZ_Position> *positions);
void lsm_read_TimeStamps (TiffParams *par, bim::uint32 offset, std::vector<CZ_TimeStamp> *timestamps);

#include <iostream>
#include <fstream>

int lsmGetInfo (TiffParams *tiffParams) {
  if (tiffParams == NULL) return 1;
  if (tiffParams->tiff == NULL) return 1;
  if (!tiffParams->ifds.isValid()) return 1;
  TinyTiff::IFD *ifd = tiffParams->ifds.firstIfd();
  if (!ifd) return 1;

  ImageInfo *info = &tiffParams->info;
  LsmInfo *lsm = &tiffParams->lsmInfo;
  CZ_LSMINFO *lsmi = &lsm->lsm_info;


  tiffParams->info.number_pages = tiffGetNumberOfPages( tiffParams );
  lsm->pages_tiff = tiffParams->info.number_pages;
  lsm->pages = tiffParams->info.number_pages / 2;
  tiffParams->info.number_pages = lsm->pages;

  lsm->ch = tiffParams->info.samples;
  if (tiffParams->info.samples > 1) 
    tiffParams->info.imageMode = IM_MULTI;
  else
    tiffParams->info.imageMode = IM_GRAYSCALE;    

  //bim::uint16 bitspersample = 1;  
  //TIFFGetField(tiffParams->tiff, TIFFTAG_BITSPERSAMPLE, &bitspersample);  
  //tiffParams->info.depth = bitspersample;

  // ------------------------------------------
  // get LSM INFO STRUCTURE
  // ------------------------------------------
  // zero the info struct
  memset( (void*)&lsm->lsm_info, 0, sizeof(CZ_LSMINFO) );

  uchar *buf = NULL;
  bim::uint64 size;
  bim::uint16 type;
  if (!ifd->tagPresent(TIFFTAG_CZ_LSMINFO )) return 1;
  ifd->readTag(TIFFTAG_CZ_LSMINFO, size, type, &buf);
  if ( (size <= 0) || (buf == NULL) ) return 1;
  
  // first read the version
  bim::uint32 version = * (bim::uint32 *) buf;
  if (TinyTiff::bigendian) TIFFSwabLong(&version);

  // if reading old LSM 1.3 file
  if (version == 0x00300494C) {
      // if header does not meet minimum size, bail
      if (size<136) return 1;
      if (TinyTiff::bigendian) doSwabLSMINFO_13((CZ_LSMINFO_13 *) buf);
      CZ_LSMINFO_13 *h13 = (CZ_LSMINFO_13 *) buf;

      // copy lsm 1.3 into 1,5 header
      memcpy( (void*)&lsm->lsm_info, buf, 72 );
      lsm->lsm_info.u16ScanType = h13->u32ScanType;
      // copy second part
      memcpy( (void*)&lsm->lsm_info.u32DataType, (void*) &h13->u32DataType, 60 );
  } else { 
      // new 1.5 - 6.0 version
      if (TinyTiff::bigendian) doSwabLSMINFO((CZ_LSMINFO *) buf);
      bim::uint64 minsz = std::min<bim::uint64>(sizeof(CZ_LSMINFO), size);
      memcpy( (void*)&lsm->lsm_info, buf, minsz );
      //lsm->lsm_info = * (CZ_LSMINFO *) buf;
  }
  _TIFFfree(buf);

  //---------------------------------------------------------------
  // retreive meta-data
  //---------------------------------------------------------------
  lsm->ch       = lsmi->s32DimensionChannels;
  lsm->t_frames = lsmi->s32DimensionTime;
  lsm->z_slices = lsmi->s32DimensionZ;

  lsm->res[0] = lsmi->f64VoxelSizeX; // x in meters
  lsm->res[1] = lsmi->f64VoxelSizeY; // y in meters
  lsm->res[2] = lsmi->f64VoxelSizeZ; // z in meters
  lsm->res[3] = lsmi->f64TimeIntervall; // t in seconds

  //---------------------------------------------------------------
  // define dims
  //---------------------------------------------------------------
  info->number_z = lsm->z_slices;
  info->number_t = lsm->t_frames;

  if (lsm->z_slices > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_Z;
  }

  if (lsm->t_frames > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_T;
  }

  if ((lsm->z_slices > 1) && (lsm->t_frames > 1)) {
    info->number_dims = 5;
    info->dimensions[3].dim = DIM_Z;        
    info->dimensions[4].dim = DIM_T;
  }

  //---------------------------------------------------------------
  // read sub blocks
  //---------------------------------------------------------------
  lsm_read_ChannelColors (tiffParams);
  lsm_read_ScanInformation (tiffParams);
  lsm_read_Positions  (tiffParams, lsm->lsm_info.u32OffsetPositions,     &lsm->positions);
  lsm_read_Positions  (tiffParams, lsm->lsm_info.u32OffsetTilePositions, &lsm->tile_positions);
  lsm_read_TimeStamps (tiffParams, lsm->lsm_info.u32OffsetTimeStamps,    &lsm->time_stamps);

  return 0;
}

void lsmGetCurrentPageInfo(TiffParams *tiffParams) {
  if (tiffParams == NULL) return;
  ImageInfo *info = &tiffParams->info;

  if ( tiffParams->subType == tstCzLsm ) {
    LsmInfo *lsm = &tiffParams->lsmInfo;  
    info->resUnits = RES_um;
    info->xRes = lsm->res[0] * 1000000;
    info->yRes = lsm->res[1] * 1000000;
  }

}

//----------------------------------------------------------------------------
// read CZ_ChannelColors
//----------------------------------------------------------------------------

void lsm_read_ChannelColors (TiffParams *par) {
    if (par == NULL) return;
    if (par->tiff == NULL) return;
    if (!par->ifds.isValid()) return;
    TinyTiff::IFD *ifd = par->ifds.firstIfd();
    if (!ifd) return;
    LsmInfo *lsm = &par->lsmInfo;
    CZ_LSMINFO *lsmi = &lsm->lsm_info;

    // read CZ_ChannelColors
    memset( &lsm->lsm_colors, 0, sizeof(CZ_ChannelColors) );
    if (lsm->lsm_info.u32OffsetChannelColors > 0) {
      ifd->readBufNoAlloc( lsm->lsm_info.u32OffsetChannelColors, sizeof(CZ_ChannelColors), TIFF_LONG, (unsigned char *) &lsm->lsm_colors );
    
      bim::uint32 colorsz = lsm->lsm_colors.s32BlockSize;
      std::vector<bim::uint8> buf(colorsz);
      ifd->readBufNoAlloc( lsm->lsm_info.u32OffsetChannelColors, colorsz, TIFF_BYTE, &buf[0] );
    
      // read colors
      int colors_num = lsm->lsm_colors.s32NumberColors;
      int colors_sz  = colors_num * sizeof(CZ_ChannelColor);
      if (lsm->lsm_colors.s32ColorsOffset>0 && lsm->lsm_colors.s32ColorsOffset+colors_sz<(int)colorsz) {
          lsm->channel_colors.resize( colors_num );
          memcpy( (void *) &lsm->channel_colors[0], (void *) &buf[lsm->lsm_colors.s32ColorsOffset], colors_sz );
          if (TinyTiff::bigendian) 
              TIFFSwabArrayOfLong( (bim::uint32*) &lsm->channel_colors[0], colors_num );
      }

      // read names
      int names_num = lsm->lsm_colors.s32NumberNames;
      if (lsm->lsm_colors.s32NamesOffset>0) {
          lsm->channel_names.resize( names_num );
          size_t offadd = 4; // each channel string seems to be pre pended with the bim::uint32 size variable
          for (int i=0; i<names_num; ++i) {
              lsm->channel_names[i] = (char *) &buf[lsm->lsm_colors.s32NamesOffset + offadd];
              const char *s = lsm->channel_names[i].c_str();
              offadd += lsm->channel_names[i].size()+5; // skip the next size var and 0 termination
          }
      }
    }

    // create suggested mapping
    size_t num_smaples = lsm->channel_colors.size();
    lsm->display_lut.resize((int) bim::NumberDisplayChannels, -1);
    std::vector<int> display_prefs(num_smaples, -1);
    bool display_channel_set=false;
    for (unsigned int sample=0; sample<num_smaples; ++sample) {
        int r = lsm->channel_colors[sample].r; // max: 255
        int g = lsm->channel_colors[sample].g; // max: 255
        int b = lsm->channel_colors[sample].b; // max: 255

        int display_channel=-1;
        if ( r>=1 && g==0 && b==0 ) display_channel = bim::Red;
        else
        if ( r==0 && g>=1 && b==0 ) display_channel = bim::Green;
        else
        if ( r==0 && g==0 && b>=1 ) display_channel = bim::Blue;
        else
        if ( r>=1 && g>=1 && b==0 ) display_channel = bim::Yellow;
        else
        if ( r>=1 && g==0 && b>=1 ) display_channel = bim::Magenta;
        else
        if ( r==0 && g>=1 && b>=1 ) display_channel = bim::Cyan;
        else
        if ( r>=1 && g>=1 && b>=1 ) display_channel = bim::Gray;
    
        display_prefs[sample] = display_channel;
        if (display_channel>=0) { 
            lsm->display_lut[display_channel] = sample; 
            display_channel_set=true; 
        }
    }

    // safeguard for some channels to be hidden by user preferences, in this case, ignore user preference
    if (display_channel_set) {
        int display=0;
        for (int vis=0; vis<bim::NumberDisplayChannels; ++vis)
            if (lsm->display_lut[vis]>-1) ++display;

        if (display<std::min<int>((int)num_smaples, bim::NumberDisplayChannels)) {
            display_channel_set = false;
            for (int i=0; i<(int) bim::NumberDisplayChannels; ++i) 
                lsm->display_lut[i] = -1;
        }
    }

    // if the image has no preferred mapping set something
    if (!display_channel_set)
      if (num_smaples == 1) 
        for (int i=0; i<3; ++i) 
          lsm->display_lut[i] = 0;
      else
        for (int i=0; i<std::min((int) bim::NumberDisplayChannels, (int)num_smaples); ++i) 
          lsm->display_lut[i] = i;

}

//----------------------------------------------------------------------------
// READ CZ_ScanInformation
//----------------------------------------------------------------------------

inline size_t LsmScanInfoEntry::offsetSize() const {
  size_t size = sizeof( CZ_ScanInformation );
  size += this->data.size();
  return size;
}

int LsmScanInfoEntry::readEntry (TIFF *tif, bim::uint32 offset) {

  CZ_ScanInformation si;
  int size = sizeof(CZ_ScanInformation);
  tif->tif_seekproc((thandle_t) tif->tif_fd, offset, SEEK_SET);  
  if (tif->tif_readproc((thandle_t) tif->tif_fd, &si, size) < (int) size) return 1;

  if (TinyTiff::bigendian) {
    TIFFSwabArrayOfLong( (bim::uint32*) &si.u32Entry, 1 );
    TIFFSwabArrayOfLong( (bim::uint32*) &si.u32Type, 1 );
    TIFFSwabArrayOfLong( (bim::uint32*) &si.u32Size, 1 );
  }

  this->entry_type = si.u32Entry;
  this->data_type  = si.u32Type;

  if (si.u32Size>0) {
    size = si.u32Size;
    this->data.resize(size);
    if (tif->tif_readproc((thandle_t) tif->tif_fd, &this->data[0], size) < (int) size) return 1;
  }
  return 0;
}

xstring LsmScanInfoEntry::toString() const {
    return xstring();
}

void lsm_read_ScanInformation(TiffParams *tiffParams) {
    if (tiffParams == NULL) return;
    if (tiffParams->tiff == NULL) return;
    if (!tiffParams->ifds.isValid()) return;

    ImageInfo *info = &tiffParams->info;
    LsmInfo *lsm = &tiffParams->lsmInfo;
    CZ_LSMINFO *lsmi = &lsm->lsm_info;

    //---------------------------------------------------------------
    // read CZ_ScanInformation
    //---------------------------------------------------------------
    std::vector<xstring> path;
    int block_track = 1, block_laser = 1, block_detection_channel = 1, block_illumination_channel = 1,
        block_beam_plitter = 1, block_data_channel = 1, block_timer = 1, block_marker = 1;

    int level = 0;
    lsm->scan_info_entries.clear();
    lsm->data_channels.clear();
    lsm->detection_channels.clear();
    lsm->illumination_channels.clear();
    unsigned int offset = lsm->lsm_info.u32OffsetScanInformation;
    while (offset > 0) {
        LsmScanInfoEntry block;
        if (block.readEntry(tiffParams->tiff, offset) != 0) break;

        if (block.data_type == TYPE_SUBBLOCK) {
            offset += 12;
            if (block.entry_type == SUBBLOCK_END) {
                --level;
                if (level == 0) break;
            } else
                ++level;

            if (block.entry_type == SUBBLOCK_RECORDING) { 
                path.push_back("Recording");
            } else if (block.entry_type == SUBBLOCK_END) { 
                path.pop_back();
            } else if (block.entry_type == SUBBLOCK_LASERS) { 
                path.push_back("Lasers");
            } else if (block.entry_type == SUBBLOCK_TRACKS) { 
                path.push_back("Tracks");
            } else if (block.entry_type == SUBBLOCK_DETECTION_CHANNELS) { 
                path.push_back("Detection channels");
            } else if (block.entry_type == SUBBLOCK_ILLUMINATION_CHANNELS) { 
                path.push_back("Illumination channels");
            } else if (block.entry_type == SUBBLOCK_BEAM_SPLITTERS) { 
                path.push_back("Beam splitters");
            } else if (block.entry_type == SUBBLOCK_DATA_CHANNELS) { 
                path.push_back("Data channels");
            } else if (block.entry_type == SUBBLOCK_TIMERS) { 
                path.push_back("Timers");
            } else if (block.entry_type == SUBBLOCK_MARKERS) { 
                path.push_back("Markers"); 
            } else if (block.entry_type == SUBBLOCK_TRACK) { 
                path.push_back(xstring::xprintf("Track%d", block_track++));
            } else if (block.entry_type == SUBBLOCK_LASER) { 
                path.push_back(xstring::xprintf("Laser%d", block_laser++));
            } else if (block.entry_type == SUBBLOCK_DETECTION_CHANNEL) { 
                path.push_back(xstring::xprintf("Detection channel%d", block_detection_channel++));
                lsm->detection_channels.push_back(xstring::join(path, "/"));
            } else if (block.entry_type == SUBBLOCK_ILLUMINATION_CHANNEL) {
                path.push_back(xstring::xprintf("Illumination channel%d", block_illumination_channel++));
                lsm->illumination_channels.push_back(xstring::join(path, "/"));
            } else if (block.entry_type == SUBBLOCK_BEAM_SPLITTER) { 
                path.push_back(xstring::xprintf("Beam splitter%d", block_beam_plitter++));
            } else if (block.entry_type == SUBBLOCK_DATA_CHANNEL) { 
                path.push_back(xstring::xprintf("Data channel%d", block_data_channel++));
                // we first enumerate all data channels and then need to look for linked detection channels, etc.
                lsm->data_channels.push_back(LsmDataChannel(block_data_channel-1, block_track-1, xstring::join(path, "/")));
            } else if (block.entry_type == SUBBLOCK_TIMER) { 
                path.push_back(xstring::xprintf("Timer%d", block_timer++));
            } else if (block.entry_type == SUBBLOCK_MARKER) { 
                path.push_back(xstring::xprintf("Marker%d", block_marker++));
            }
        } else {
            // not a TYPE_SUBBLOCK
            offset += (unsigned int)block.offsetSize();
            block.path = xstring::join(path, "/");
            lsm->scan_info_entries.push_back(block);
        }
    } // while (offset > 0)
}

//----------------------------------------------------------------------------
// read u32OffsetTilePositions
//----------------------------------------------------------------------------

void lsm_read_Positions (TiffParams *par, bim::uint32 offset, std::vector<CZ_Position> *positions) {
    if (par == NULL) return;
    if (par->tiff == NULL) return;
    if (!par->ifds.isValid()) return;
    TinyTiff::IFD *ifd = par->ifds.firstIfd();
    if (!ifd) return;
    LsmInfo *lsm = &par->lsmInfo;

    positions->clear();
    if (offset>0) {
        try {
            CZ_Positions poshead;      
            ifd->readBufNoAlloc( offset, sizeof(CZ_Positions), TIFF_LONG, (unsigned char *) &poshead );
            positions->resize(poshead.u32Tiles);
            ifd->readBufNoAlloc( offset+sizeof(CZ_Positions), 
                                 poshead.u32Tiles*sizeof(CZ_Position), 
                                 TIFF_DOUBLE, 
                                 (unsigned char*) &positions->at(0) );
        } catch (...) {
            // just a safe block of untested code:)
        }
    }
}

//----------------------------------------------------------------------------
// read u32OffsetTimeStamps
//----------------------------------------------------------------------------

void lsm_read_TimeStamps (TiffParams *par, bim::uint32 offset, std::vector<CZ_TimeStamp> *timestamps) {
    if (par == NULL) return;
    if (par->tiff == NULL) return;
    if (!par->ifds.isValid()) return;
    TinyTiff::IFD *ifd = par->ifds.firstIfd();
    if (!ifd) return;
    LsmInfo *lsm = &par->lsmInfo;

    timestamps->clear();
    if (offset>0) {
        try {
            CZ_TimeStamps header;      
            ifd->readBufNoAlloc( offset, sizeof(CZ_TimeStamps), TIFF_LONG, (unsigned char *) &header );
            timestamps->resize(header.s32NumberTimeStamps);
            ifd->readBufNoAlloc( offset+sizeof(CZ_TimeStamps), 
                                 header.s32NumberTimeStamps*sizeof(CZ_TimeStamp), 
                                 TIFF_DOUBLE, 
                                 (unsigned char*) &timestamps->at(0) );
        } catch (...) {
            // just a safe block of untested code:)
        }
    }
}


//----------------------------------------------------------------------------
// READ/WRITE FUNCTIONS
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// METADATA FUNCTIONS
//----------------------------------------------------------------------------

void get_and_set_double(TagMap *hash, const xstring &key_in, const xstring &key_out ) {
    if (hash->hasKey(key_in)) {
        double v = hash->get_value_double(key_in, 0);
        hash->set_value(key_out, v);
    }
}

void get_and_set_int(TagMap *hash, const xstring &key_in, const xstring &key_out) {
    if (hash->hasKey(key_in)) {
        int v = hash->get_value_int(key_in, 0);
        hash->set_value(key_out, v);
    }
}

void get_and_set_string(TagMap *hash, const xstring &key_in, const xstring &key_out) {
    if (hash->hasKey(key_in)) {
        xstring v = hash->get_value(key_in);
        hash->set_value(key_out, v);
    }
}

bim::uint append_metadata_lsm (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;

  TiffParams *tiffParams = (TiffParams *) fmtHndl->internalParams;
  if (tiffParams == NULL) return 1;
  LsmInfo *lsm = &tiffParams->lsmInfo;
  CZ_LSMINFO *lsmi = &lsm->lsm_info;

  hash->append_tag( bim::IMAGE_NUM_Z, (bim::uint) lsm->z_slices );
  hash->append_tag( bim::IMAGE_NUM_T, (bim::uint) lsm->t_frames );
  hash->append_tag( bim::IMAGE_NUM_C, lsm->ch );

  hash->append_tag( bim::PIXEL_RESOLUTION_X, lsm->res[0]*1000000 );
  hash->append_tag( bim::PIXEL_RESOLUTION_Y, lsm->res[1]*1000000 );
  hash->append_tag( bim::PIXEL_RESOLUTION_Z, lsm->res[2]*1000000 );
  hash->append_tag( bim::PIXEL_RESOLUTION_T, lsm->res[3] );

  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  hash->set_value( bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS );

  //----------------------------------------------------------------------------
  // Channel names and preferred mapping
  //----------------------------------------------------------------------------
  ImageInfo *info = &tiffParams->info;
  for (unsigned int i=0; i<std::min<size_t>(info->samples, lsm->channel_names.size()); ++i) {
    xstring tag_name = xstring::xprintf( bim::CHANNEL_NAME_TEMPLATE.c_str(), i );
    hash->append_tag( tag_name, lsm->channel_names[i] );
  }
  
  for (int i=0; i<lsm->channel_colors.size(); ++i) {
    xstring tag_name  = xstring::xprintf( bim::CHANNEL_COLOR_TEMPLATE.c_str(), i );
    xstring tag_value = xstring::xprintf( "%d,%d,%d", lsm->channel_colors[i].r, lsm->channel_colors[i].g, lsm->channel_colors[i].b );
    hash->append_tag( tag_name, tag_value );
  }

  // preferred lut mapping
  if (lsm->display_lut.size() >= bim::NumberDisplayChannels) {
    hash->set_value( bim::DISPLAY_CHANNEL_RED,     lsm->display_lut[bim::Red] );
    hash->set_value( bim::DISPLAY_CHANNEL_GREEN,   lsm->display_lut[bim::Green] );
    hash->set_value( bim::DISPLAY_CHANNEL_BLUE,    lsm->display_lut[bim::Blue] );
    hash->set_value( bim::DISPLAY_CHANNEL_YELLOW,  lsm->display_lut[bim::Yellow] );
    hash->set_value( bim::DISPLAY_CHANNEL_MAGENTA, lsm->display_lut[bim::Magenta] );
    hash->set_value( bim::DISPLAY_CHANNEL_CYAN,    lsm->display_lut[bim::Cyan] );
    hash->set_value( bim::DISPLAY_CHANNEL_GRAY,    lsm->display_lut[bim::Gray] );
  }

  //----------------------------------------------------------------------------
  // All other tags in custom field
  //----------------------------------------------------------------------------
  initMetaHash(lsm);
  for (int i=0; i<lsm->scan_info_entries.size(); i++) {
      LsmScanInfoEntry *e = &lsm->scan_info_entries.at(i);

      if (e->data.size()<=0) continue;
      if (lsm->key_names[e->entry_type]=="") continue;

      xstring key = xstring(bim::CUSTOM_TAGS_PREFIX) + e->path + "/" + lsm->key_names[e->entry_type];

      if (e->data_type == TYPE_ASCII && e->data.size()>1) {
          xstring line = (char*) &e->data[0];
          line = line.replace("\r", "").strip(" ");
          if (line.size()>0)
              hash->append_tag(key, line);
      } else if (e->data_type == TYPE_LONG && e->data.size()>=4) {
          hash->append_tag(key, *(int*)&e->data[0] );
      } else if (e->data_type == TYPE_RATIONAL && e->data.size()>=8) {
          hash->append_tag(key, *(double*)&e->data[0]);
      } else if (e->data_type == TYPE_BOOLEAN && e->data.size()>=4) {
          hash->append_tag(key, *(int*)&e->data[0] == 0);
      } else if (e->data_type == TYPE_DATE && e->data.size()>=4) {
          hash->append_tag(key, *(int*)&e->data[0]);
      }
  }

  //----------------------------------------------------------------------------
  // stage positions
  //----------------------------------------------------------------------------  
  for (unsigned int i=0; i<lsm->positions.size(); ++i) {
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_X.c_str(), i ), lsm->positions[i].x );
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_Y.c_str(), i ), lsm->positions[i].y );
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_Z.c_str(), i ), lsm->positions[i].z );
  }

  for (unsigned int i=0; i<lsm->tile_positions.size(); ++i) {
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_X.c_str(), i ), lsm->tile_positions[i].x );
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_Y.c_str(), i ), lsm->tile_positions[i].y );
      hash->set_value( xstring::xprintf( bim::STAGE_POSITION_TEMPLATE_Z.c_str(), i ), lsm->tile_positions[i].z );
  }

  //----------------------------------------------------------------------------
  // New style channel description
  //----------------------------------------------------------------------------

  // find all valid illumination channels
  std::vector<xstring> valid_illumination_channels;
  for (std::vector<xstring>::const_iterator it = lsm->illumination_channels.begin(); it != lsm->illumination_channels.end(); ++it) {
      xstring k = xstring(bim::CUSTOM_TAGS_PREFIX) + *it + "/" + lsm->key_names[ILLUMCHANNEL_ENTRY_AQUIRE];
      int acquire = hash->get_value_int(k, 0);
      k = xstring(bim::CUSTOM_TAGS_PREFIX) + *it + "/" + lsm->key_names[ILLUMCHANNEL_ENTRY_WAVELENGTH];
      double wavelength = hash->get_value_double(k, 0);

      if (acquire != 0 && wavelength != 0) {
            valid_illumination_channels.push_back(*it);
      }
  }
  lsm->illumination_channels = valid_illumination_channels;

  // for all data channels find their detection channels and illumination channels
  std::map<int, bool> valid_tracks; // needed to find unique valid tracks
  for (unsigned int i = 0; i < lsm->data_channels.size(); ++i) {
      LsmDataChannel *dc = &lsm->data_channels.at(i);
      valid_tracks[dc->track] = true;

      xstring k = xstring(bim::CUSTOM_TAGS_PREFIX) + dc->path + "/" + lsm->key_names[DATACHANNEL_ENTRY_NAME];
      dc->name = hash->get_value(k);
      if (dc->name.size() == 0) continue;
      std::vector<xstring> p = dc->path.split("/");
      p.pop_back(); p.pop_back(); // get to the track level
      xstring track_path = xstring::join(p, "/");

      // we now have to find detection channel with the same name in the same Track
      for (std::vector<xstring>::const_iterator it = lsm->detection_channels.begin(); it != lsm->detection_channels.end(); ++it) {
          if (it->startsWith(track_path)) {
              xstring k = xstring(bim::CUSTOM_TAGS_PREFIX) + *it + "/" + lsm->key_names[DETCHANNEL_DETECTION_CHANNEL_NAME];
              xstring name = hash->get_value(k);
              if (name == dc->name) {
                  dc->path_detection_channel = *it;
                  break;
              }
          }
      }

      // dima: valid illumination channles seem to be stored in the same order as data channels
      int num_illum = std::min<int>(i, lsm->illumination_channels.size()-1);
      dc->path_illumination_channel = lsm->illumination_channels[num_illum];
  }

  // fill in channel information
  int num_tracks = valid_tracks.size();
  int num_channels = std::min<int>(info->samples, lsm->data_channels.size());
  for (unsigned int i = 0; i<num_channels; ++i) {
      LsmDataChannel *dc = &lsm->data_channels.at(i);

      xstring k;
      xstring old_tag = xstring::xprintf( bim::CHANNEL_NAME_TEMPLATE.c_str(), i );
      xstring tag = xstring::xprintf(bim::CHANNEL_INFO_TEMPLATE.c_str(), i);
      xstring det_channel_path = xstring(bim::CUSTOM_TAGS_PREFIX) + dc->path_detection_channel + "/";
      xstring ilum_channel_path = xstring(bim::CUSTOM_TAGS_PREFIX) + dc->path_illumination_channel + "/";

      // name
      xstring name = dc->name;
      if (name.size()==0)
          name = xstring::xprintf("Ch%d", i+1);
      
      // Zeiss standard to set track number if multiple 
      if (num_tracks>1) {
          name += xstring::xprintf("-T%d", dc->track);
      }
      hash->set_value(tag + bim::CHANNEL_INFO_NAME, name);
      hash->set_value(old_tag, name);

      // dye
      k = xstring(bim::CUSTOM_TAGS_PREFIX) + dc->path + "/" + lsm->key_names[DATACHANNEL_ENTRY_DYE_NAME];
      xstring dye = hash->get_value(k);
      if (dye.empty()) {
          k = det_channel_path + lsm->key_names[DETCHANNEL_ENTRY_DYE_NAME];
          dye = hash->get_value(k);
      }
      if (!dye.empty())
          hash->set_value(tag + bim::CHANNEL_INFO_DYE, dye);

      // track description
      if (num_tracks > 1) {
          std::vector<xstring> p = dc->path.split("/");
          p.pop_back(); p.pop_back(); // get to the track level
          xstring track_path = xstring::join(p, "/");
          k = xstring(bim::CUSTOM_TAGS_PREFIX) + track_path + "/" + lsm->key_names[DATACHANNEL_ENTRY_NAME];
          if (hash->hasKey(k)) {
              xstring descr = hash->get_value(k);
              hash->set_value(tag + bim::CHANNEL_INFO_DESCRIPTION, descr);
          }
      }

      // color
      xstring color = xstring::xprintf("%d,%d,%d", lsm->channel_colors[i].r, lsm->channel_colors[i].g, lsm->channel_colors[i].b);
      hash->set_value(tag + CHANNEL_INFO_COLOR, color);

      // pinhole
      k = det_channel_path + lsm->key_names[DETCHANNEL_ENTRY_PINHOLE_DIAMETER];
      if (hash->hasKey(k)) {
          double pinhole_diameter = hash->get_value_double(k, 0);
          hash->set_value(tag + bim::CHANNEL_INFO_PINHOLE_RADIUS, pinhole_diameter / 2.0);
      }
      
      // direct values
      
      get_and_set_double(hash,
          det_channel_path + lsm->key_names[DETCHANNEL_ENTRY_DETECTOR_GAIN],
          tag + bim::CHANNEL_INFO_DETECTOR_GAIN);

      get_and_set_double(hash,
          det_channel_path + lsm->key_names[DETCHANNEL_ENTRY_AMPLIFIER_GAIN],
          tag + bim::CHANNEL_INFO_AMPLIFIER_GAIN);

      get_and_set_double(hash,
          det_channel_path + lsm->key_names[DETCHANNEL_ENTRY_AMPLIFIER_OFFS],
          tag + bim::CHANNEL_INFO_AMPLIFIER_OFFS);

      get_and_set_string(hash,
          det_channel_path + lsm->key_names[DETCHANNEL_FILTER_NAME],
          tag + bim::CHANNEL_INFO_FILTER_NAME);

      get_and_set_string(hash,
          det_channel_path + lsm->key_names[DETCHANNEL_FILTER_NAME],
          tag + bim::CHANNEL_INFO_EM_WAVELENGTH);

      // fetch data from illumination channel
      get_and_set_double(hash,
          ilum_channel_path + lsm->key_names[ILLUMCHANNEL_ENTRY_WAVELENGTH],
          tag + bim::CHANNEL_INFO_EX_WAVELENGTH);

      get_and_set_double(hash,
          ilum_channel_path + lsm->key_names[ILLUMCHANNEL_ENTRY_POWER],
          tag + bim::CHANNEL_INFO_POWER);

      //DECLARE_STR(CHANNEL_INFO_OBJECTIVE, "objective")
      //DECLARE_STR(CHANNEL_INFO_FLUOR, "fluor")
  }

  // find some other attributes of interest
  xstring rec_path = xstring(bim::CUSTOM_TAGS_PREFIX) + "Recording/";

  get_and_set_string(hash,
      rec_path + lsm->key_names[RECORDING_ENTRY_DESCRIPTION],
      bim::IMAGE_DESCRIPTION);

  get_and_set_string(hash,
      rec_path + lsm->key_names[RECORDING_ENTRY_NOTES],
      bim::IMAGE_NOTES);

  get_and_set_string(hash,
      rec_path + lsm->key_names[RECORDING_ENTRY_USER],
      bim::IMAGE_USER);

  // objective
  xstring objective = hash->get_value(rec_path + lsm->key_names[RECORDING_ENTRY_OBJECTIVE]);
  bim::parse_objective_from_string(objective, hash);

  // zoom
  double zoom_x = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_ZOOM_X], 1);
  double zoom_y = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_ZOOM_Y], 1);
  hash->set_value(bim::IMAGE_ZOOM, xstring::xprintf("%.2f,%.2f,1.0", zoom_x, zoom_y));

  // position
  double pos_x = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_SAMPLE_0X], 0);
  double pos_y = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_SAMPLE_0Y], 0);
  double pos_z = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_SAMPLE_0Z], 0);
  hash->set_value(bim::IMAGE_POSITION, xstring::xprintf("%f,%f,%f", pos_x, pos_y, pos_z));

  // size
  /*double w = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_PLANE_WIDTH], 0);
  double h = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_PLANE_HEIGHT], 0);
  double d = hash->get_value_double(rec_path + lsm->key_names[RECORDING_ENTRY_VOLUME_DEPTH], 0);
  hash->set_value(bim::IMAGE_SIZE, xstring::xprintf("%.2f,%.2f,%.2f", w, h, d));*/

  return 0;
}

