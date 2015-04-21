/*******************************************************************************

  Defines Image Format - Qt4 Utilities
  rely on: DimFiSDK version: 1
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    09/13/2005 19:50 - First creation
    10/10/2005 14:47 - bug writing only 2 channels fixed
      
  ver: 2
        
*******************************************************************************/

#include "bim_qt_utils.h"
#include "bim_img_format_utils.h"

QImage  qImagefromDimImage  (const ImageBitmap &img )
{
  int max_samples = img.i.samples; 
  if (max_samples > 3) max_samples = 3;
  uchar lut[3][65536];

  //------------------------------------------------------------------
  // if input image is 16 bpp/ch then normalize to 8 bpp/ch
  //------------------------------------------------------------------
  if (img.i.depth == 16)
  {
    BIM_UINT max_uint16 = (uint16) -1;
    long hist[65536];
    BIM_UINT min_col = 0;
    BIM_UINT max_col = max_uint16;
    int32 i;
    int range=0;
    BIM_UINT sample=0;

    for (sample=0; sample<max_samples; sample++)
    {
      uint16 *p16 = (uint16 *) img.bits[sample];
      getSampleHistogram((ImageBitmap *) &img, hist, sample);

      for (i=0; i<=max_uint16; i++) 
      {
        if (hist[i] != 0) 
        { min_col = i; break; }
      }
  
      for (i=max_uint16; i>=0; i--) 
      {
        if (hist[i] != 0) 
        { max_col = i; break; }
      }

      range = (max_col - min_col) / 256.0;
      if (range == 0) range = 256;

      for (i=0; i<=max_uint16; i++)  
        lut[sample][i] = iTrimUC ( (i - min_col) / range );
    }

  } // if 16 bpp then normalize

  //------------------------------------------------------------------
  // Create QImage and copy data
  //------------------------------------------------------------------
  QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied;
  QImage image(img.i.width, img.i.height, imageFormat);

  for (int y=0; y<img.i.height; ++y) 
  {
    QRgb *dest = (QRgb *) image.scanLine(y);

    unsigned int src_line_offset = img.i.width * y;

    if (max_samples == 1) 
    {
  
      if (img.i.depth == 8)
      {
        uchar *src1 = ((uchar *) img.bits[0]) + src_line_offset;

        if (img.i.lut.count <= 0)
        { // no LUT
          for (int x=0; x<img.i.width; ++x) 
            dest[x] = qRgb ( src1[x], src1[x], src1[x] );
        }
        else
        { // use LUT to create colors
          RGBA *pal = (RGBA *) img.i.lut.rgba;

          for (int x=0; x<img.i.width; ++x) 
            dest[x] = qRgb ( xR(pal[src1[x]]), xG(pal[src1[x]]), xB(pal[src1[x]]) );
        }
      } // 8 bpp
    
      if (img.i.depth == 16)
      {
        uint16 *src0 = ((uint16 *) img.bits[0]) + src_line_offset;

        if (img.i.lut.count <= 0)
        { // no LUT
          for (int x=0; x<img.i.width; ++x) 
            dest[x] = qRgb ( lut[0][src0[x]], lut[0][src0[x]], lut[0][src0[x]] );
        }
        else
        { // use LUT to create colors
          RGBA *pal = (RGBA *) img.i.lut.rgba;

          for (int x=0; x<img.i.width; ++x) 
            dest[x] = qRgb ( xR(pal[lut[0][src0[x]]]), xG(pal[lut[0][src0[x]]]), xB(pal[lut[0][src0[x]]]) );
        }
      } // 16 bpp      

    } // if (max_samples == 1) 


    if (max_samples == 2) 
    {
  
      if (img.i.depth == 8)
      {
        uchar *src0 = ((uchar *) img.bits[0]) + src_line_offset;
        uchar *src1 = ((uchar *) img.bits[1]) + src_line_offset;

        for (int x=0; x<img.i.width; ++x) 
          dest[x] = qRgb ( src0[x], src1[x], 0 );
      } // 8 bpp
    
      if (img.i.depth == 16)
      {
        uint16 *src0 = ((uint16 *) img.bits[0]) + src_line_offset;
        uint16 *src1 = ((uint16 *) img.bits[1]) + src_line_offset;

        for (int x=0; x<img.i.width; ++x) 
          dest[x] = qRgb ( lut[0][src0[x]], lut[1][src1[x]], 0 );
      } // 16 bpp      

    } // if (max_samples == 2) 

    if (max_samples >= 3) 
    {
      if (img.i.depth == 8)
      {
        uchar *src0 = ((uchar *) img.bits[0]) + src_line_offset;
        uchar *src1 = ((uchar *) img.bits[1]) + src_line_offset;
        uchar *src2 = ((uchar *) img.bits[2]) + src_line_offset;

        for (int x=0; x<img.i.width; ++x) 
          dest[x] = qRgb ( src0[x], src1[x], src2[x] );
      } // 8 bpp
    
      if (img.i.depth == 16)
      {
        uint16 *src0 = ((uint16 *) img.bits[0]) + src_line_offset;
        uint16 *src1 = ((uint16 *) img.bits[1]) + src_line_offset;
        uint16 *src2 = ((uint16 *) img.bits[2]) + src_line_offset;

        for (int x=0; x<img.i.width; ++x) 
          dest[x] = qRgb ( lut[0][src0[x]], lut[1][src1[x]], lut[2][src2[x]] );
      } // 16 bpp      

    } // if (max_samples == 3) 


  } // for y

  return image;
}

QPixmap qPixmapfromDimImage (const ImageBitmap &img )
{
  QImage image = qImagefromDimImage(img);
  return QPixmap::fromImage( image );
}



