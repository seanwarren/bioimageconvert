/*******************************************************************************

  Implementation of the Image I/O for OpenCV
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2011-05-11 08:32:12 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_OPENCV
#pragma message("bim::Image: OpenCV support methods")

#include "bim_image.h"

#include <algorithm>
#include <limits>
#include <cstring>

#include <opencv2/core/core.hpp>

using namespace bim;

int toCVType(const Image *img ) {
    if (img->depth()==8 && img->pixelType()==FMT_UNSIGNED)
        return CV_8UC1;
    else
    if (img->depth()==16 && img->pixelType()==FMT_UNSIGNED)
        return CV_16UC1;
    else
    if (img->depth()==8 && img->pixelType()==FMT_SIGNED)
        return CV_8SC1;
    else
    if (img->depth()==16 && img->pixelType()==FMT_SIGNED)
        return CV_16SC1;
    else
    if (img->depth()==32 && img->pixelType()==FMT_SIGNED)
        return CV_32SC1;
    else
    if (img->depth()==32 && img->pixelType()==FMT_FLOAT)
        return CV_32FC1;
    else
    if (img->depth()==64 && img->pixelType()==FMT_FLOAT)
        return CV_64FC1;

    return CV_USRTYPE1; // some types are not available in opencv
}

cv::Mat Image::asCVMat(const unsigned int &channel) const { // shallow memory copy
  int sample = std::min<int>( this->samples()-1, channel); 
  int type = toCVType(this);
  if (type == CV_USRTYPE1) return cv::Mat();
  cv::Mat m(this->height(), this->width(), type, this->sampleBits(sample));
  return m;
}

cv::Mat Image::toCVMat(const unsigned int &channel) const { // deep memory copy
  int sample = std::min<int>( this->samples()-1, channel); 
  int type = toCVType(this);
  if (type == CV_USRTYPE1) return cv::Mat();
  cv::Mat m(this->height(), this->width(), type);

  // copy data
  for (int y=0; y<this->height(); ++y) {
      void *target = m.ptr(y);
      void *source = this->scanLine(sample, y);
      memcpy(target, source, this->bytesPerLine());
  }
  return m;
}

void Image::fromCVMat( const cv::Mat &m ) { // deep memory copy
  throw; // dima: not implemented
/*
  unsigned int nx = m.cols();
  unsigned int ny = m.rows();
  DataFormat pf = FMT_UNSIGNED;
  if (!std::numeric_limits<PixelType>::is_integer) 
    pf = FMT_FLOAT;
  else
  if (std::numeric_limits<PixelType>::is_signed) 
    pf = FMT_SIGNED;

  // allocate this image
  if (this->alloc( nx, ny, 1, sizeof(PixelType), pf)<=0) return;

  // copy data
  PixelType *target = (PixelType *) img.sampleBits(sample);
  PixelType *source = m.data();
  memcpy(target, source, img.bytesPerChan());
  */
}

//Image Image::filter_convolve( int w, int h=0, ResizeMethod method = szNearestNeighbor, bool keep_aspect_ratio = false ) const {}

#endif //BIM_USE_OPENCV
