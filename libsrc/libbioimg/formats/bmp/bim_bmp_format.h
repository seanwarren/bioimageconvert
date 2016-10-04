/*****************************************************************************
  BMP support 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/22/2004 13:06 - First creation
        
  Ver : 2
*****************************************************************************/

#ifndef BIM_BMP_FORMAT_H
#define BIM_BMP_FORMAT_H

#include <cstdio>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// DLL EXPORT FUNCTION
extern "C" {
bim::FormatHeader* bmpGetFormatHeader(void);
}

namespace bim {

//----------------------------------------------------------------------------
// BMP internal structures, following Windows GDI:
//
//  [Structure]         [Bytes] 
//  BITMAPFILEHEADER    0x00 0x0D 
//  BITMAPINFOHEADER    0x0E 0x35 
//  RGBQUAD array       0x36 0x75 
//  Color-index array   0x76 0x275 
//----------------------------------------------------------------------------

#pragma pack(push, 1)
struct BITMAPFILEHEADER { 
  uint16  bfType;      // Specifies the file type, must be BM -> (0x42 0x4d)
  uint32  bfSize;      // Specifies the size, in bytes, of the bitmap file. 
  uint16  bfReserved1; // Reserved; must be zero. 
  uint16  bfReserved2; // Reserved; must be zero. 
  uint32  bfOffBits;   // Specifies the offset, in bytes, from the beginning of the 
                           // BITMAPFILEHEADER structure to the bitmap bits. 
}; 
#pragma pack(pop)

const int BMP_FILEHDR_SIZE = 14;	// size of BMP_FILEHDR data

const int BIM_BMP_OLD  = 12;			// old Windows/OS2 BMP size
const int BIM_BMP_WIN  = 40;			// new Windows BMP size
const int BIM_BMP_OS2  = 64;			// new OS/2 BMP size

const int BIM_BMP_RGB  = 0;				// no compression
const int BIM_BMP_RLE8 = 1;				// run-length encoded, 8 bits
const int BIM_BMP_RLE4 = 2;				// run-length encoded, 4 bits
const int BIM_BMP_BITFIELDS = 3;	// xRGB values encoded in data as bit-fields

#pragma pack(push, 1)
struct BITMAPINFOHEADER {
  uint32  biSize;          // size of this struct
  uint32  biWidth;         // pixmap width
  uint32  biHeight;        // pixmap height 
  uint16  biPlanes;        // should be 1
  uint16  biBitCount;      // number of bits per pixel
  uint32  biCompression;   // compression method
  uint32  biSizeImage;     // size of image
  uint32  biXPelsPerMeter; // horizontal resolution 
  uint32  biYPelsPerMeter; // vertical resolution 
  uint32  biClrUsed;       // number of colors used
  uint32  biClrImportant;  // number of important colors
};
#pragma pack(pop) 

typedef struct RGBQUAD {
  uint8  rgbBlue; 
  uint8  rgbGreen; 
  uint8  rgbRed; 
  uint8  rgbReserved; 
} RGBQUAD; 

//----------------------------------------------------------------------------
// Internal Format Info Struct
//----------------------------------------------------------------------------

typedef struct BmpParams {
  ImageInfo i;
  BITMAPFILEHEADER bf;
  BITMAPINFOHEADER bi;
} BmpParams;

} // namespace bim

#endif // BIM_BMP_FORMAT_H
