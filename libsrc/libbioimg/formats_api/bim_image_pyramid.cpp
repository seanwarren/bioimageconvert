/*******************************************************************************

  Defines Image Pyramid Class
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#include <cmath>

#include "bim_image_pyramid.h"
#include <xstring.h>
#include <xtypes.h>

using namespace bim;

ImagePyramid::ImagePyramid() {
  init();
}

ImagePyramid::ImagePyramid(const Image &img, const int &v) {
    init();
    if (v>0)
        setMinImageSize(v);
    this->createFrom(img);  
}

ImagePyramid::~ImagePyramid() {
  free();
}

void ImagePyramid::init() {
  ImageStack::init();
  min_width = d_min_image_size;
  min_height = d_min_image_size;
}

int ImagePyramid::levelClosestTop( unsigned int w, unsigned int h ) const {
  if (handling_image) return 0;
  for (int i = (int)images.size()-1; i>0; --i) {
    if ( (images.at(i).width() >= w) || 
         (images.at(i).height() >= h) ) return i;
  }
  return 0;
}

int ImagePyramid::levelClosestBottom( unsigned int w, unsigned int h ) const {
  if (handling_image) return 0;
  if (images.size() <= 1) return 0;

  for (int i = (int)images.size()-2; i>=0; --i) {
    if ( (images.at(i).width() > w) || 
         (images.at(i).height() > h) ) return i+1;
  }
  return 0;
}


void ImagePyramid::createFrom( const Image &img ) {
  handling_image = true;
  images.clear();
  images.push_back( img );
  cur_position = 0;

  unsigned int side = bim::max<unsigned int>( images[0].width(), images[0].height() );
  int max_levels = (int) ceil( bim::log2<double>( side ) );
  int i = 0;
  while ( bim::max<unsigned int>( images[i].width(), images[i].height() ) > (unsigned int) min_width ) {
    xstring x;
    x.sprintf("Constructing pyramid, level %d", i+1);
    do_progress( i+1, max_levels, (char*) x.c_str() );
    images.push_back( images[i].downSampleBy2x() );
    ++i;
  }
  handling_image = false;
}

bool ImagePyramid::fromFile( const char *fileName, int page ) {
  cur_position = 0;  
  images.clear();
  Image img;
  bool res = img.fromFile( fileName, page );
  if (res) {
    img = img.ensureTypedDepth();
    img = img.ensureColorSpace();
    createFrom( img );
  }
  return res;
}
