/*****************************************************************************
  Igor binary file format v5 (IBW)
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  DEFINITIONS
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    10/19/2005 16:03 - First creation
            
  Ver : 1
*****************************************************************************/

#ifndef BIM_IBW_FORMAT_H
#define BIM_IBW_FORMAT_H

#include <cstdio>
#include <vector>
#include <string>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* ibwGetFormatHeader(void);
}

namespace bim {

/*****************************************************************************
  Igor binary file format - quick reference

  BinHeader5 structure  64 bytes
  WaveHeader5 structure excluding wData field 320 bytes
  Wave data Variable size
  Optional wave dependency formula  Variable size
  Optional wave note data Variable size
  Optional extended data units data Variable size
  Optional extended dimension units data  Variable size
  Optional dimension label data Variable size
  String indices used for text waves only Variable size

  Data order:
  row/column/layer/chunk

  Offsets:
  
  data_offset    = 64 + 320;
  formula_offset = data_offset + data_size;
  notes_offset   = formula_offset + formula_size;

  data_size = WaveHeader5.npnts * size_in_bytes_of[ WaveHeader5.type ]


*****************************************************************************/

//----------------------------------------------------------------------------
// Internal Format Structs
//----------------------------------------------------------------------------

#define BIM_IBW_MAGIC_SIZE 2

const unsigned char ibwMagicWin[BIM_IBW_MAGIC_SIZE] = {0x05, 0x00};
const unsigned char ibwMagicMac[BIM_IBW_MAGIC_SIZE] = {0x00, 0x05};

#pragma pack(push, 2)

#define MAXDIMS 4

typedef struct BinHeader5 {
  int16 version;                // Version number for backwards compatibility.
  int16 checksum;               // Checksum over this header and the wave header.
  int32 wfmSize;                // The size of the WaveHeader5 data structure plus the wave data.
  int32 formulaSize;            // The size of the dependency formula, if any.
  int32 noteSize;               // The size of the note text.
  int32 dataEUnitsSize;         // The size of optional extended data units.
  int32 dimEUnitsSize[MAXDIMS]; // The size of optional extended dimension units.
  int32 dimLabelsSize[MAXDIMS]; // The size of optional dimension labels.
  int32 sIndicesSize;           // The size of string indicies if this is a text wave.
  int32 optionsSize1;           // Reserved. Write zero. Ignore on read.
  int32 optionsSize2;           // Reserved. Write zero. Ignore on read.
} BinHeader5;


#define MAX_WAVE_NAME2 18 // Maximum length of wave name in version 1 and 2 files. Does not include the trailing null.
#define MAX_WAVE_NAME5 31 // Maximum length of wave name in version 5 files. Does not include the trailing null.
#define MAX_UNIT_CHARS 3

#define NT_CMPLX       1    // Complex numbers.
#define NT_FP32        2    // 32 bit fp numbers.
#define NT_FP64        4    // 64 bit fp numbers.
#define NT_I8          8    // 8 bit signed integer. Requires Igor Pro 2.0 or later.
#define NT_I16         0x10 // 16 bit integer numbers. Requires Igor Pro 2.0 or later.
#define NT_I32         0x20 // 32 bit integer numbers. Requires Igor Pro 2.0 or later.
#define NT_UNSIGNED    0x40 // Makes above signed integers unsigned. Requires Igor 

typedef struct WaveHeader5 {
  uint32 nextPointer;    // link to next wave in linked list. Used in memory only. Write zero. Ignore on read.

  uint32 creationDate;   // DateTime of creation.
  uint32 modDate;        // DateTime of last modification.

  int32  npnts;          // Total number of points (multiply dimensions up to first zero).
  int16  type;           // See types (e.g. NT_FP64) above. Zero for text waves.
  int16  dLock;          // Reserved. Write zero. Ignore on read.

  char   whpad1[6];               // Reserved. Write zero. Ignore on read.
  int16  whVersion;               // Write 1. Ignore on read.
  char   bname[MAX_WAVE_NAME5+1]; // Name of wave plus trailing null.
  int32  whpad2;                  // Reserved. Write zero. Ignore on read.
  uint32 dFolderPointer;          // Used in memory only. Write zero. Ignore on read.

  // Dimensioning info. [0] == rows, [1] == cols etc
  int32  nDim[MAXDIMS];           // Number of of items in a dimension -- 0 means no data.
  float64 sfA[MAXDIMS];            // Index value for element e of dimension d = sfA[d]*e + sfB[d].
  float64 sfB[MAXDIMS];

  // SI units
  char   dataUnits[MAX_UNIT_CHARS+1];         // Natural data units go here - null if none.
  char   dimUnits[MAXDIMS][MAX_UNIT_CHARS+1]; // Natural dimension units go here - null if none.

  int16  fsValid;                   // true if full scale values have meaning.
  int16  whpad3;                    // Reserved. Write zero. Ignore on read.
  float64 topFullScale;
  float64 botFullScale;              // The max and max full scale value for wave.

  uint32 dataEUnitsPointer;         // Used in memory only. Write zero. Ignore on read.
  uint32 dimEUnitsPointer[MAXDIMS]; // Used in memory only. Write zero. Ignore on read.
  uint32 dimLabelsPointer[MAXDIMS]; // Used in memory only. Write zero. Ignore on read.
  
  uint32 waveNoteHPointer;          // Used in memory only. Write zero. Ignore on read.
  int32  whUnused[16];              // Reserved. Write zero. Ignore on read.

  // The following stuff is considered private to Igor.

  int16  aModified;       // Used in memory only. Write zero. Ignore on read.
  int16  wModified;       // Used in memory only. Write zero. Ignore on read.
  int16  swModified;      // Used in memory only. Write zero. Ignore on read.
  
  char   useBits;         // Used in memory only. Write zero. Ignore on read.
  char   kindBits;        // Reserved. Write zero. Ignore on read.
  uint32 formulaPointer;  // Used in memory only. Write zero. Ignore on read.
  int32  depID;           // Used in memory only. Write zero. Ignore on read.
  
  int16  whpad4;          // Reserved. Write zero. Ignore on read.
  int16  srcFldr;         // Used in memory only. Write zero. Ignore on read.
  uint32 fileNamePointer; // Used in memory only. Write zero. Ignore on read.
  
  uint32 sIndicesPointer; // Used in memory only. Write zero. Ignore on read.

  float  wData[1];        // The start of the array of data. Must be 64 bit aligned.
} WaveHeader5;

#pragma pack(pop) 


//----------------------------------------------------------------------------
// Internal Format Info Struct
//----------------------------------------------------------------------------

class IbwParams {
public:
  IbwParams(): little_endian(false), formula_offset(0), notes_offset(0) { i = initImageInfo(); }

  bim::ImageInfo i;
  BinHeader5  bh;
  WaveHeader5 wh;
  long data_offset;
  bool little_endian;
  long formula_offset;
  long notes_offset;
  long data_size;
  int  real_bytespp;
  bim::DataType real_type;

  std::string note;
};

} // namespace bim

#endif // BIM_IBW_FORMAT_H
