/*****************************************************************************
  OME XML file format (Open Microscopy Environment)
  UCSB/BioITR property
  Copyright (c) 2005 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  Base64 enc/dec extracted from: b64.c
    Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

  History:
    11/21/2005 15:43 - First creation
            
  Ver : 1
*****************************************************************************/

#include <string>

#include "bim_ome_format.h"

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif 

#include <math.h>
#include <limits.h>
#include <float.h>

using namespace bim;

//FLT_MIN FLT_MAX DBL_MAX DBL_MIN INT_MAX INT_MIN SHRT_MAX SHRT_MIN

//const char *ome_types[13] = { "Uint8", "Uint8", "int8", "Uint16", 
//                              "Uint32", "double", "int8", "Uint8", 
//                              "int16", "int32", "double", "float", "double" };

const char *bool_types[2] = { "false", "true" };


//----------------------------------------------------------------------------
// BASE 64 ENC/DEC
//----------------------------------------------------------------------------

// encoding LUT as described in RFC1113
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// decoding LUT, created by Bob Trower
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

// encode 3 8-bit binary bytes as 4 '6-bit' characters
void b64_encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

// decode 4 '6-bit' characters into 3 8-bit binary bytes
void b64_decodeblock( unsigned char in[4], unsigned char out[3] )
{   
    out[0] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[1] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[2] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}


//----------------------------------------------------------------------------
// WRITE PROC
//----------------------------------------------------------------------------

const char* omePixelType( ImageBitmap *img ) {
  std::string pt = "Uint8";
  if (img->i.depth==16 && img->i.pixelType==FMT_UNSIGNED) pt = "Uint16";
  if (img->i.depth==32 && img->i.pixelType==FMT_UNSIGNED) pt = "Uint32";
  if (img->i.depth==8  && img->i.pixelType==FMT_SIGNED) pt = "int8";
  if (img->i.depth==16 && img->i.pixelType==FMT_SIGNED) pt = "int16";
  if (img->i.depth==32 && img->i.pixelType==FMT_SIGNED) pt = "int32";
  if (img->i.depth==32 && img->i.pixelType==FMT_FLOAT)  pt = "float";
  if (img->i.depth==64 && img->i.pixelType==FMT_FLOAT)  pt = "double"; 
  return pt.c_str();
}

void omeWriteImageInfo( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;
  OmeParams *omePar = (OmeParams *) fmtHndl->internalParams;
  if (fmtHndl->stream == NULL) return;
  ImageBitmap *img = fmtHndl->image;

  std::string str;
  char buf[1024];


  // write info
  
  //<Image Name = "P1W1S1" PixelSizeX = "0.2" PixelSizeY = "0.2" PixelSizeZ = "0.2">
  if (img->i.resUnits == RES_um)
    sprintf(buf, "  <Image Name = \"%s\" PixelSizeX=\"0.2\" PixelSizeY=\"0.2\">\n", fmtHndl->fileName );
  else
    sprintf(buf, "  <Image Name = \"%s\" PixelSizeX=\"%f\" PixelSizeY=\"%f\">\n", fmtHndl->fileName, img->i.xRes, img->i.yRes );
  str = buf;


  str += "    <CreationDate>1111-11-11T11:11:11</CreationDate>\n";

  //<Pixels DimensionOrder = "XYCZT" 
  //PixelType = "int16" 
  //BigEndian = "true" 
  //SizeX = "20" 
  //SizeY = "20" 
  //SizeZ = "5" 
  //SizeC = "1" 
  //SizeT = "6">
  str += "    <Pixels DimensionOrder = \"XYCZT\" ";
  
  sprintf(buf, "PixelType = \"%s\" ", omePixelType(img) );
  str += buf;

  sprintf(buf, "BigEndian = \"%s\" ", bool_types[bim::bigendian] );
  str += buf;

  sprintf(buf, "SizeX = \"%d\" ", img->i.width );
  str += buf;

  sprintf(buf, "SizeY = \"%d\" ", img->i.height );
  str += buf;

  sprintf(buf, "SizeZ = \"%d\" ", img->i.number_z );
  str += buf;

  sprintf(buf, "SizeC = \"%d\" ", img->i.samples );
  str += buf;

  sprintf(buf, "SizeT = \"%d\">\n", img->i.number_t );
  str += buf;

  xwrite( fmtHndl, (void*) str.c_str(), 1, str.size() );
}


static int write_ome_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  OmeParams *omePar = (OmeParams *) fmtHndl->internalParams;

  if (fmtHndl->pageNumber == 0) omeWriteImageInfo( fmtHndl ); 
  ImageBitmap *img = fmtHndl->image; 

  // write channels
  for (unsigned int sample=0; sample<img->i.samples; ++sample) {
    // now write pixels
    std::string str = "      <Bin:BinData Compression=\"none\">";
    xwrite( fmtHndl, (void*) str.c_str(), 1, str.size() );  

    // now dump base64 bits
    uchar *p = (uchar *) img->bits[sample];
    int size_left = getImgSizeInBytes(img);
    uchar ascii_quatro[4];

    while (size_left>0)
    {
      b64_encodeblock( p, ascii_quatro, size_left );
      xwrite( fmtHndl, (void*) ascii_quatro, 1, 4 );
      p+=3;
      size_left-=3;
    }

    str = "</Bin:BinData>\n";
    xwrite( fmtHndl, (void*) str.c_str(), 1, str.size() ); 
  }

  return 0;
}

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------
/*
static int read_ibw_image(FormatHandle *fmtHndl)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;
  ImageInfo *info = &ibwPar->i; 
  if (fmtHndl->stream == NULL) return 1;
  
  // get needed page 
  if (fmtHndl->pageNumber < 0) fmtHndl->pageNumber = 0; 
  if (fmtHndl->pageNumber > info->number_pages) fmtHndl->pageNumber = info->number_pages-1;
  int page=fmtHndl->pageNumber;

  //allocate image
  ImageBitmap *img = fmtHndl->image;
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  //read page
  long ch_num_points = info->width * info->height;
  long ch_size = ch_num_points * ibwPar->real_bytespp; 
  uchar *chbuf = new uchar [ch_size];


  long ch_offset = ibwPar->data_offset + (ch_size * page);

  if ( xseek(fmtHndl, ch_offset, SEEK_SET) != 0) return 1;
  if (xread( fmtHndl, chbuf, ch_size, 1 ) != 1) return 1;

  // swap if neede
  if ( (bim::bigendian) && (ibwPar->little_endian == true) ) 
  {
    if ( (ibwPar->wh.type == NT_FP32) || (ibwPar->wh.type == NT_I32) )
      swapArrayOfLong((uint32*) chbuf, ch_size/4);

    if (ibwPar->wh.type == NT_FP64)
      swapArrayOfDouble((float64*) chbuf, ch_size/8);

    if (ibwPar->wh.type == NT_I16)
      swapArrayOfShort((uint16*) chbuf, ch_size/2);
    
    if (ibwPar->wh.type == NT_CMPLX)
      swapArrayOfLong((uint32*) chbuf, ch_size/4);
  }

  // normalize and copy
  if (ibwPar->wh.type == NT_FP32)
  {
    float max_val = FLT_MIN;
    float min_val = FLT_MAX;
    float *pb = (float *) chbuf;
    long x = 0;

    // find min and max
    for (x=0; x<ch_num_points; ++x) 
    {
      if (*pb > max_val) max_val = *pb;
      if (*pb < min_val) min_val = *pb;
      ++pb;
    }

    double range = (max_val - min_val) / 256.0;
    if (range == 0) range = 256;

    pb = (float *) chbuf;
    uchar *p = (uchar *) img->bits[0];

    // direct data copy
    for (x=0; x<ch_num_points; ++x) 
    {
      *p = iTrimUC ( (*pb - min_val) / range );
      ++pb;
      ++p;
    }
    
    /*
    // copy transposing the data
    long line_size = info->width;
    long y=0;

    for (y=0; y<info->height; ++y) 
    {
      p = ( (uchar *) img->bits[0]) + y;     
      for (x=0; x<info->width; ++x) 
      {
        *p = iTrimUC ( (*pb - min_val) / range );
        ++pb;
        p+=line_size;
      }
    }
    */
/*
  } // if (ibwPar->wh.type == NT_FP32) 

  delete [] chbuf;
  return 0;
}

*/

//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------
/*
bim::uint read_ome_metadata (FormatHandle *fmtHndl, int group, int tag, int type)
{
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  group; tag; type;
  return 0;
}

char* read_text_ome_metadata ( FormatHandle *fmtHndl )
{
  if (fmtHndl == NULL) return NULL;
  if (fmtHndl->internalParams == NULL) return NULL;
  IbwParams *ibwPar = (IbwParams *) fmtHndl->internalParams;
  
  char *buf = NULL;
 /*
  std::string str = "";
  char cstr[1024], *line, *l2, p1[1024], p2[1024];

  

  str += "[IBW image properties]\n";

  sprintf( cstr, "Width: %.2f um\n", nimg.width * nimg.xR );
  str += cstr;

  sprintf( cstr, "Height: %.2f um\n", nimg.height * nimg.yR );
  str += cstr;

*//*
  long buf_size = ibwPar->bh.noteSize;
  buf = new char [buf_size+1];
  buf[buf_size] = '\0';

  if ( xseek(fmtHndl, ibwPar->notes_offset, SEEK_SET) != 0) return NULL;
  if (xread( fmtHndl, buf, buf_size, 1 ) != 1) return NULL;

  for (int i=0; i<buf_size; ++i)
    if (buf[i] == 0x0d) buf[i] = 0x0a;

  //std::string

  //buf = new char [buf_size+1];
  //buf[buf_size] = '\0';
  //memcpy( buf, str.c_str(), buf_size );

  return buf;
  return NULL;
}
*/







