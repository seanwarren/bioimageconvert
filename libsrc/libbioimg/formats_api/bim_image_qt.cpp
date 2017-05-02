/*******************************************************************************

  Implementation of the Image Class for Trolltech Qt
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/24/2006 12:45 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_QT

#include "bim_image.h"
#include "bim_img_format_utils.h"

#ifdef max
#undef max
#endif

#include <algorithm>

#pragma message("Image: Trolltech Qt support methods")

using namespace bim;

Image::Image(const QImage &qimg) {
  bmp = NULL;
  connectToNewMemory();
  fromQImage(qimg);
}

QImage  Image::toQImage ( ) const {
  int max_samples = std::min<int>( this->samples(), 3); 
  
  Image img;
  if (this->depth() != 8) {
    img = this->ensureTypedDepth();
    img = img.convertToDepth( 8, Lut::ltLinearDataRange );
  } else
    img = this->normalize();

  //------------------------------------------------------------------
  // Create QImage and copy data
  //------------------------------------------------------------------
  //QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied;
  QImage::Format imageFormat = QImage::Format_ARGB32;
  QImage image(img.width(), img.height(), imageFormat);

  for (int y=0; y<img.height(); ++y) {
    QRgb *dest = (QRgb *) image.scanLine(y);

    if (max_samples == 1) {
        uchar *src1 = img.scanLine(0, y);

        if (img.lutSize() <= 0) { // no LUT
          for (int x=0; x<img.width(); ++x) 
            dest[x] = qRgba ( src1[x], src1[x], src1[x], 255 );
        }
        else { // use LUT to create colors
          RGBA *pal = img.palette();
          for (int x=0; x<img.width(); ++x) 
            dest[x] = qRgba ( xR(pal[src1[x]]), xG(pal[src1[x]]), xB(pal[src1[x]]), 255 );
        }
    } // if (max_samples == 1) 


    if (max_samples == 2) {
        uchar *src0 = img.scanLine(0, y);
        uchar *src1 = img.scanLine(1, y);

        for (int x=0; x<img.width(); ++x) 
          dest[x] = qRgba ( src0[x], src1[x], 0, 255 );
    } // if (max_samples == 2) 

    if (max_samples >= 3) {
        uchar *src0 = img.scanLine(0, y);
        uchar *src1 = img.scanLine(1, y);
        uchar *src2 = img.scanLine(2, y);

        for (int x=0; x<img.width(); ++x) 
          dest[x] = qRgba ( src0[x], src1[x], src2[x], 255 );
    } // if (max_samples == 3) 

  } // for y

  return image;
}

QPixmap Image::toQPixmap ( ) const {
  QImage image = this->toQImage();
  return QPixmap::fromImage(image);
}

void Image::paint( QPainter *p ) const {
  p->drawImage ( 0, 0, this->toQImage() );
}

void Image::fromQImage( const QImage &qimg ) {

  if (qimg.depth() < 32) return;
  
  if ( qimg.width() != width() || qimg.height() != height() || this->depth()!=8 || this->samples()!=3 )
    this->alloc( qimg.width(), qimg.height(), 3, 8 );

  //------------------------------------------------------------------
  // Create QImage and copy data
  //------------------------------------------------------------------
  unsigned char *p0 = this->scanLine( 0, 0 );
  unsigned char *p1 = this->scanLine( 1, 0 );
  unsigned char *p2 = this->scanLine( 2, 0 );
  const unsigned char *pI = qimg.bits();  

  if (!bim::bigendian) {
    for (unsigned int y=0; y<height(); ++y) {
      for (unsigned int x=0; x<width(); ++x) {
        #if defined(Q_WS_WIN)
        *p0 = pI[2];
        *p1 = pI[1];
        *p2 = pI[0];
        #elif defined(Q_WS_MAC)
        *p0 = pI[2];
        *p1 = pI[1];
        *p2 = pI[0];      
        #else
        *p0 = pI[2];
        *p1 = pI[1];
        *p2 = pI[0];      
        #endif
        pI+=4; ++p0; ++p1; ++p2;
      }
    } // for y
  } else { 
    // big endian case, ppc macs or sun sparcs
    for (unsigned int y=0; y<height(); ++y) {
      for (unsigned int x=0; x<width(); ++x) {
        *p0 = pI[1];
        *p1 = pI[2];
        *p2 = pI[3];
        pI+=4; ++p0; ++p1; ++p2;
      }
    } // for y
  }
}


#endif //BIM_USE_QT

