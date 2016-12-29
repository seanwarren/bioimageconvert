/*******************************************************************************

  Implementation of the Image Filters
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2011-05-11 08:32:12 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_FILTERS
#pragma message("bim::Image: Filters")

#include "bim_image.h"

#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

#include "slic.h"

using namespace bim;


//------------------------------------------------------------------------------------
// Edge
//------------------------------------------------------------------------------------

template <typename T>
void edge_filter( const Image &in, Image &out ) {
    if (in.width()   != out.width())   return;
    if (in.height()  != out.height())  return;
    if (in.samples() != out.samples()) return;
    if (in.depth()   != out.depth())   return;

    bim::uint64 w = (bim::uint64) in.width();
    bim::uint64 h = (bim::uint64) in.height();

    for (int sample=0; sample<in.samples(); sample++) {
        #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (h>BIM_OMP_FOR2)
        for (int y=1; y<h-1; ++y ) {
            T *src_m1 = (T *) in.scanLine ( sample, y-1 ); 
            T *src = (T *) in.scanLine ( sample, y ); 
            T *src_p1 = (T *) in.scanLine ( sample, y+1 ); 
            T *dst = (T *) out.scanLine( sample, y ); 
            for (unsigned int x=1; x<w-1; ++x) {
			    T max_y = (T) bim::max<double>(fabs((double) src[x] - src_m1[x]), fabs((double) src[x] - src_p1[x]));
			    T max_x = (T) bim::max<double>(fabs((double) src[x] - src[x-1]), fabs((double) src[x] - src[x+1]));
                dst[x] = bim::max<T>(max_x, max_y);
            }
        }
    } // for samples
}

Image Image::filter_edge() const {
    Image out = this->deepCopy(true);
    out.fill(0);

    if (this->depth()==8 && this->pixelType()==FMT_UNSIGNED)
        edge_filter<bim::uint8>  ( *this, out );
    else
    if (this->depth()==16 && this->pixelType()==FMT_UNSIGNED)
        edge_filter<bim::uint16> ( *this, out );
    else
    if (this->depth()==32 && this->pixelType()==FMT_UNSIGNED)
        edge_filter<bim::uint32> ( *this, out );
    else
    if (this->depth()==64 && this->pixelType()==FMT_UNSIGNED)
        edge_filter<bim::uint64> ( *this, out );
    else
    if (this->depth()==8 && this->pixelType()==FMT_SIGNED)
        edge_filter<bim::int8>   ( *this, out );
    else
    if (this->depth()==16 && this->pixelType()==FMT_SIGNED)
        edge_filter<bim::int16>  ( *this, out );
    else
    if (this->depth()==32 && this->pixelType()==FMT_SIGNED)
        edge_filter<bim::int32>  ( *this, out );
    else
    if (this->depth()==64 && this->pixelType()==FMT_SIGNED)
        edge_filter<bim::int64>  ( *this, out );
    else
    if (this->depth()==32 && this->pixelType()==FMT_FLOAT)
        edge_filter<bim::float32> ( *this, out );
    else
    if (this->depth()==64 && this->pixelType()==FMT_FLOAT)
        edge_filter<bim::float64> ( *this, out );

    return out;
}

//------------------------------------------------------------------------------------
// Superpixels
//------------------------------------------------------------------------------------

// regionSize in pixels, regularization 0-1, with 1 the shape is most regular
Image Image::superpixels( bim::uint64 regionSize, float regularization ) const {
    Image out(this->width(), this->height(), 32, 1, FMT_UNSIGNED);

    bim::uint32 *seg = (bim::uint32*) out.bits(0);
    bim::uint64 minRegionSize = bim::round<double>(regionSize * 0.7);
    
    regularization = regularization * (regionSize * regionSize);

    if (this->depth()==8 && this->pixelType()==FMT_UNSIGNED)
        slic_segment<bim::uint8, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==16 && this->pixelType()==FMT_UNSIGNED)
        slic_segment<bim::uint16, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==32 && this->pixelType()==FMT_UNSIGNED)
        slic_segment<bim::uint32, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==64 && this->pixelType()==FMT_UNSIGNED)
        slic_segment<bim::uint64, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==8 && this->pixelType()==FMT_SIGNED)
        slic_segment<bim::int8, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==16 && this->pixelType()==FMT_SIGNED)
        slic_segment<bim::int16, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==32 && this->pixelType()==FMT_SIGNED)
        slic_segment<bim::int32, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==64 && this->pixelType()==FMT_SIGNED)
        slic_segment<bim::int64, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==32 && this->pixelType()==FMT_FLOAT)
        slic_segment<bim::float32, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);
    else
    if (this->depth()==64 && this->pixelType()==FMT_FLOAT)
        slic_segment<bim::float64, float> (seg, this, this->width(), this->height(), this->samples(), regionSize, regularization, minRegionSize);

    return out;
}

Image operation_superpixels(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    int superpixels = 0;
    float superpixels_regularization = 0.0;
    std::vector<double> vals = arguments.splitDouble(",", 0.0);
    if (vals.size()>0)
        superpixels = (int)vals[0];
    if (vals.size()>1)
        superpixels_regularization = vals[1];

    if (superpixels > 0)
        return img.superpixels(superpixels, superpixels_regularization);
    return img;
};

#endif //BIM_USE_FILTERS
