/*******************************************************************************

  Implementation of the Image I/O for NumPy
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2011-05-11 08:32:12 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_NUMPY
#pragma message("Image: Numpy support methods")

#include "bim_image.h"

#include <algorithm>
#include <limits>

#include "ndarray.h"

template <typename PixelType, class ArrayType = Ndarray<PixelType, ????> > // dima: incomplete
ImageType::Pointer Image::toNdarray() const {

  int sample = std::min<int>( this->samples()-1, channel); 
  int bitdepth = sizeof(PixelType);
  DataFormat pf = FMT_UNSIGNED;
  if (!std::numeric_limits<PixelType>::is_integer) 
    pf = FMT_FLOAT;
  else
  if (std::numeric_limits<PixelType>::is_signed) 
    pf = FMT_SIGNED;
  
  // convert image into requested format
  Image img;
  if (this->depth()!=bitdepth || this->pixelType()!=pf) {
    img = this->ensureTypedDepth();
    img = img.convertToDepth( bitdepth, Lut::ltLinearDataRange, pf );
  } else
    img = *this;

  // create Eigen matrix
  MatrixType image(img.height(), img.width());

  // copy data
  PixelType *target = image.data();
  PixelType *source = (PixelType *) img.sampleBits(sample);
  memcpy(target, source, img.bytesPerChan());
  return image;
}

template <typename PixelType, class ArrayType = Ndarray<PixelType, ?????> > // dima: incomplete
void Image::fromNdarray( const ArrayType &m ) {

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
}

#endif //BIM_USE_NUMPY
