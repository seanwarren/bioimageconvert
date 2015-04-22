/*******************************************************************************

  Implementation of the Image Class, it uses smart pointer technology to implement memory
  sharing, simple cope operations simply point to the same memory addresses
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  Image file structure:
    
    1) Page:   each file may contain 1 or more pages, each page is independent

    2) Sample: in each page each pixel can contain 1 or more samples
               preferred number of samples: 1 (GRAY), 3(xRGB) and 4(RGBA)

    3) Depth:  each sample can be of 1 or more bits of depth
               preferred depths are: 8 and 16 bits per sample

    4) Allocation: each sample is allocated having in mind:
               All lines are stored contiguasly from top to bottom where
               having each particular line is byte alligned, i.e.:
               each line is allocated using minimal necessary number of bytes
               to store given depth. Which for the image means:
               size = ceil( ( (width*depth) / 8 ) * height )

  As a result of Sample/Depth structure we can get images of different 
  Bits per Pixel as: 1, 8, 24, 32, 48 and any other if needed

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#include <string>
#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>

#include <cstring>

#include "xtypes.h"
#include "xconf.h"
#include "bim_image.h"
#include "bim_img_format_utils.h"
#include "bim_buffer.h"
#include "bim_histogram.h"
#include "bim_image_pyramid.h"
#include "bim_image_5d.h"
#include "bim_image_proxy.h"
#include "bim_metatags.h"
#include "bim_primitives.h"

//#include <blob_manager.h>

#ifdef BIM_USE_IMAGEMANAGER
#include <meta_format_manager.h>
#endif //BIM_USE_IMAGEMANAGER

#include "resize.h"
#include "rotate.h"

#include "typeize_buffer.cpp"

using namespace bim;

//------------------------------------------------------------------------------------
// Image defs
//------------------------------------------------------------------------------------

std::vector<ImgRefs*> Image::refs;
const Image::map_modifiers Image::modifiers = Image::create_modifiers();

Image::Image() {
  bmp = NULL;
  connectToNewMemory();
}

Image::~Image() {
  disconnectFromMemory();
}

Image::Image(const Image& img) { 
  bmp = NULL;
  connectToMemory( img.bmp );
  if (img.metadata.size()>0) metadata = img.metadata;
}

Image::Image(bim::uint64 width, bim::uint64 height, bim::uint64 depth, bim::uint64 samples, DataFormat format) { 
  bmp = NULL;
  connectToNewMemory();
  create( width, height, depth, samples, format ); 
}

#ifdef BIM_USE_IMAGEMANAGER
Image::Image(const char *fileName, int page) {
  bmp = NULL;
  connectToNewMemory();
  fromFile( fileName, page ); 
}

Image::Image(const std::string &fileName, int page) { 
  bmp = NULL;
  connectToNewMemory();
  fromFile( fileName, page ); 
}
#endif //BIM_USE_IMAGEMANAGER

Image &Image::operator=( const Image & img ) { 
  connectToMemory( img.bmp );
  if (img.metadata.size()>0) this->metadata = img.metadata;
  return *this; 
}

Image Image::deepCopy() const { 
  if (bmp==NULL) return Image();
  Image img( this->width(), this->height(), this->depth(), this->samples(), this->pixelType() );
  img.bmp->i = this->bmp->i;
  if (img.metadata.size()>0) img.metadata = this->metadata;

  bim::uint64 sample, chan_size = img.bytesPerChan();
  for (sample=0; sample<bmp->i.samples; sample++) {
    memcpy( img.bmp->bits[sample], bmp->bits[sample], chan_size );
  }
  return img;
}

//------------------------------------------------------------------------------------
// shared memory part
//------------------------------------------------------------------------------------

inline void Image::print_debug() {
  /*
  for (int i=0; i<refs.size(); ++i) {
    std::cout << "[" << i << ": " << refs[i]->refs << " " << ((int*)&refs[i]->bmp);
    std::cout << " " << refs[i]->bmp.i.width << "*" << refs[i]->bmp.i.height;
    std::cout << "] ";
  }
  std::cout << "\n";
  */
}

int Image::getRefId( ImageBitmap *b ) {
  if (b == NULL) return -1;
  for (unsigned int i=0; i<refs.size(); ++i)
    if (b == &refs[i]->bmp) 
      return i;
  return -1;
}

int Image::getCurrentRefId() {
  return getRefId( bmp );
}

void Image::connectToMemory( ImageBitmap *b ) {
  disconnectFromMemory();

  int ref_id = getRefId( b );
  if (ref_id == -1) return;

  bmp = &refs.at(ref_id)->bmp;
  refs.at(ref_id)->refs++;
}

void Image::connectToUnmanagedMemory(ImageBitmap *b) {
    disconnectFromMemory();

    int ref_id = getRefId(b);
    if (ref_id == -1) {
        ImgRefs *new_ref = new ImgRefs;
        new_ref->bmp = *b;
        refs.push_back(new_ref);
        ref_id = refs.size() - 1;
    }

    bmp = &refs.at(ref_id)->bmp;
    refs.at(ref_id)->refs+=2;
}

void Image::connectToNewMemory() {
  disconnectFromMemory();
  
  // create a new reference
  ImgRefs *new_ref = new ImgRefs;
  refs.push_back( new_ref );
  size_t ref_id = refs.size()-1;

  refs.at(ref_id)->refs++;
  bmp = &refs.at(ref_id)->bmp;
  initImagePlanes( bmp );
}

void Image::disconnectFromMemory() {
  int ref_id = getCurrentRefId();
  bmp = NULL;

  // decrease the reference to the image
  if (ref_id == -1) return;
  refs.at(ref_id)->refs--;

  // check if current reference will be zero, then delete image
  if (refs[ref_id]->refs < 1) {
    ImgRefs *new_ref = refs[ref_id];
    deleteImg( &refs.at(ref_id)->bmp );
    refs.erase( refs.begin() + ref_id );
    delete new_ref;
  }
}

//------------------------------------------------------------------------------------
// Arithmetic ops
//------------------------------------------------------------------------------------

template <typename T>
void lineops_max ( void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
  T *src1 = (T*) psrc1;
  T *src2 = (T*) psrc2;
  T *dest = (T*) pdest;

  if (mask == NULL) {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = bim::max<T>(src1[x], src2[x]);
  } else {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = mask[x] > 0 ? bim::max<T>(src1[x], src2[x]) : src1[x];
  }
}

template <typename T>
void lineops_min(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
  T *src1 = (T*) psrc1;
  T *src2 = (T*) psrc2;
  T *dest = (T*) pdest;

  if (mask == NULL) {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = bim::min<T>(src1[x], src2[x]);
  } else {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = mask[x] > 0 ? bim::min<T>(src1[x], src2[x]) : src1[x];
  }
}

template <typename T>
void lineops_avg(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
  T *src1 = (T*) psrc1;
  T *src2 = (T*) psrc2;
  T *dest = (T*) pdest;

  if (mask == NULL) {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = (src1[x] + src2[x]) / (T)2;
  } else {
      for (unsigned int x = 0; x < w; ++x)
          dest[x] = mask[x] > 0 ? (src1[x] + src2[x]) / (T)2 : src1[x];
  }
}

template <typename T>
void lineops_replace(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = src2[x];
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = mask[x] > 0 ? src2[x] : src1[x];
    }
}

// uses mask weights to blend, without a mask becomes avg
template <typename T>
void lineops_blend(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = (src1[x] + src2[x]) / (T)2;
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = bim::trim<T, double>((src1[x] * (255.0 - mask[x]) + src2[x] * ((double)mask[x])) / 255.0);
    }
}

template <typename T>
void lineops_add(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = bim::trim<T,T>(src1[x] + src2[x]);
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = mask[x] > 0 ? bim::trim<T, T>(src1[x] + src2[x]) : src1[x];
    }
}

template <typename T>
void lineops_sub(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = bim::trim<T, T>(src1[x] - src2[x]);
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = mask[x] > 0 ? bim::trim<T, T>(src1[x] - src2[x]) : src1[x];
    }
}

template <typename T>
void lineops_mul(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = bim::trim<T, T>(src1[x] * src2[x]);
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = mask[x] > 0 ? bim::trim<T, T>(src1[x] * src2[x]) : src1[x];
    }
}

template <typename T>
void lineops_div(void *pdest, void *psrc1, void *psrc2, bim::uint64 &w, bim::uint8 *mask = NULL) {
    T *src1 = (T*)psrc1;
    T *src2 = (T*)psrc2;
    T *dest = (T*)pdest;

    if (mask == NULL) {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = bim::trim<T, T>(src1[x] / src2[x]);
    } else {
        for (unsigned int x = 0; x < w; ++x)
            dest[x] = mask[x] > 0 ? bim::trim<T, T>(src1[x] / src2[x]) : src1[x];
    }
}

//------------------------------------------------------------------------------------
// allocation
//------------------------------------------------------------------------------------

int Image::alloc( bim::uint64 w, bim::uint64 h, bim::uint64 samples, bim::uint64 depth, DataFormat format ) {
  bim::uint64 sample=0;
  this->free();
  if (bmp==NULL) return 1;

  bmp->i.width   = w;
  bmp->i.height  = h;
  bmp->i.samples = samples;
  bmp->i.depth   = depth;
  bmp->i.pixelType = format;
  long size     = bytesPerChan( );

  for (sample=0; sample<bmp->i.samples; sample++) {
    try {
      bmp->bits[sample] = new bim::uchar [ size ];
    } 
    catch (...) {
      bmp->bits[sample] = NULL;
      deleteImg( bmp );
      bmp->i = initImageInfo();      
      return 1;
    }
  }
  return 0;  
}

void Image::free( ) {
  connectToNewMemory();
  metadata.clear();
}

void* Image::bits(const unsigned int &sample) const { 
  if (!bmp) return NULL;
  unsigned int c = bim::trim<unsigned int, unsigned int>( sample, 0, samples()-1); 
  return (void *) bmp->bits[c]; 
}

void Image::setLutColor( bim::uint64 i, RGBA c ) {
  if (bmp==NULL) return;
  if ( i>=bmp->i.lut.count ) return;
  bmp->i.lut.rgba[i] = c;
}

void Image::setLutNumColors( bim::uint64 n ) {
  if (bmp==NULL) return;
  if ( n>256 ) return;
  bmp->i.lut.count = n;
}

Image Image::convertToDepth( const ImageLut &lut ) const {

  Image img;
  if (bmp==NULL) return img;
  if (lut.size() < (int)this->samples()) return *this;

  int depth = lut.depthOutput();
  DataFormat pxt = lut.dataFormatOutput();
  bim::uint64 w = bmp->i.width;
  bim::uint64 h = bmp->i.height;
  unsigned int num_pix = w * h;

  if ( img.alloc( w, h, bmp->i.samples, depth ) == 0 ) {
    img.bmp->i = this->bmp->i;
    img.bmp->i.depth = depth;
    img.bmp->i.pixelType = pxt;
    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
      lut[sample]->apply( bmp->bits[sample], img.bits(sample), num_pix );
    } // sample
  }

  img.metadata = this->metadata;
  return img;
}

Image Image::convertToDepth( int depth, Lut::LutType method, DataFormat pxt, Histogram::ChannelMode mode, ImageHistogram *hist, void *args ) const {
  Image img;
  if (!bmp) return img;

  // small optimization
  if ( this->depth()==depth && this->pixelType()==pxt && method==Lut::ltLinearFullRange ) 
    return *this;

  // run the whole thing
  bim::uint64 w = bmp->i.width;
  bim::uint64 h = bmp->i.height;
  unsigned int num_pix = w * h;
    
  if ( img.alloc( w, h, bmp->i.samples, depth ) == 0 ) {
      img.bmp->i = this->bmp->i;
      img.bmp->i.depth = depth;
      if (pxt!=FMT_UNDEFINED) img.bmp->i.pixelType = pxt;
    
      ImageHistogram ih;
      if (mode!=Histogram::cmSeparate) ih.setChannelMode(mode);
      if (!hist || !hist->isValid()) 
          ih.fromImage(*this); 
      else 
          ih = *hist;
      ImageHistogram oh( img.samples(), img.depth(), img.pixelType() );
      ImageLut       lut(ih, oh, method, args);

      for (unsigned int sample=0; sample<bmp->i.samples; ++sample )
          lut[sample]->apply( bmp->bits[sample], img.bits(sample), num_pix );

      // we have to update the hist after the operation
      if (hist && hist->isValid()) {
          for (unsigned int sample=0; sample<bmp->i.samples; ++sample )
              lut[sample]->apply( *ih[sample], *oh[sample] );
          (*hist) = oh;
      }
  }

  img.metadata = this->metadata;
  return img;
}

Image operation_stretch(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return img.convertToDepth(img.depth(), Lut::ltLinearDataRange, FMT_UNDEFINED, Histogram::cmSeparate, hist);
};

Image operation_depth(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Lut::LutType lut_method = Lut::ltLinearFullRange;
    DataFormat out_pixel_format = FMT_UNSIGNED;
    Histogram::ChannelMode chan_mode = Histogram::cmSeparate;
    double gamma = 1.0;
    double minv = 0, maxv = 0;
    std::vector<xstring> strl = arguments.split(",");
    int out_depth = strl[0].toInt(0);
    if (strl.size()>1) {
        if (strl[1].toLowerCase() == "f") lut_method = Lut::ltLinearFullRange;
        if (strl[1].toLowerCase() == "d") lut_method = Lut::ltLinearDataRange;
        if (strl[1].toLowerCase() == "t") lut_method = Lut::ltLinearDataTolerance;
        if (strl[1].toLowerCase() == "e") lut_method = Lut::ltEqualize;
        if (strl[1].toLowerCase() == "c") lut_method = Lut::ltTypecast;
        if (strl[1].toLowerCase() == "n") lut_method = Lut::ltFloat01;
        if (strl[1].toLowerCase() == "g") lut_method = Lut::ltGamma;
        if (strl[1].toLowerCase() == "l") lut_method = Lut::ltMinMaxGamma;
    }
    if (strl.size()>2) {
        if (strl[2].toLowerCase() == "u") out_pixel_format = FMT_UNSIGNED;
        if (strl[2].toLowerCase() == "s") out_pixel_format = FMT_SIGNED;
        if (strl[2].toLowerCase() == "f") out_pixel_format = FMT_FLOAT;
    }
    if (strl.size()>3) {
        if (strl[3].toLowerCase() == "cs") chan_mode = Histogram::cmSeparate;
        if (strl[3].toLowerCase() == "cc") chan_mode = Histogram::cmCombined;
    }
    if (strl.size()>5) {
        gamma = strl[3].toDouble(1.0);
        maxv = strl[4].toDouble(0.0);
        minv = strl[5].toDouble(0.0);
    }


    if (out_depth != 8 && out_depth != 16 && out_depth != 32 && out_depth != 64) {
        std::cout << xstring::xprintf("Output depth (%s bpp) is not supported! Ignored!\n", out_depth);
        return img;
    }

    if (lut_method == Lut::ltGamma) {
        return img.convertToDepth(out_depth, lut_method, out_pixel_format, chan_mode, hist, (void*)&gamma);
    }
    else if (lut_method == Lut::ltMinMaxGamma) {
        bim::min_max_gamma_args args;
        args.gamma = gamma;
        args.maxv = maxv;
        args.minv = minv;
        return img.convertToDepth(out_depth, lut_method, out_pixel_format, chan_mode, hist, (void*)&args);
    }
    else {
        return img.convertToDepth(out_depth, lut_method, out_pixel_format, chan_mode, hist);
    }
};

Image Image::normalize( int to_bpp, ImageHistogram *hist ) const {
  if (bmp==NULL) return Image();
  if (bmp->i.depth==to_bpp) return *this;
  return convertToDepth( to_bpp, Lut::ltLinearDataRange, FMT_UNSIGNED, Histogram::cmSeparate, hist );
}

Image operation_norm(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return img.normalize(8, hist);
};

Image Image::ROI( bim::uint64 x, bim::uint64 y, bim::uint64 w, bim::uint64 h ) const {

  Image img;
  if (bmp==NULL) return img;
  if ( x==0 && y==0 && bmp->i.width==w && bmp->i.height==h ) return *this;
  if (x >= bmp->i.width)  x = 0;
  if (y >= bmp->i.height) y = 0;
  if (w+x > bmp->i.width)  w = bmp->i.width-x;
  if (h+y > bmp->i.height) h = bmp->i.height-y;

  if ( img.alloc( w, h, bmp->i.samples, bmp->i.depth ) == 0 ) {
  
    int newLineSize = img.bytesPerLine();
    int oldLineSize = this->bytesPerLine();
    int Bpp = (long) ceil( ((double)bmp->i.depth) / 8.0 );

    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
      unsigned char *pl  = (unsigned char *) img.bits(sample);
      unsigned char *plo = ( (unsigned char *) bmp->bits[sample] ) + y*oldLineSize + x*Bpp;
      for (unsigned int yi=0; yi<h; yi++) {
        memcpy( pl, plo, w*Bpp );      
        pl  += newLineSize;
        plo += oldLineSize;
      } // for yi
    } // sample
  } // allocated image

  img.bmp->i = this->bmp->i;
  img.bmp->i.width = w;
  img.bmp->i.height = h;
  img.metadata = this->metadata;
  return img;
}

Image operation_roi(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    std::vector< bim::Rectangle<int> > rois;
    std::vector<xstring> strs = arguments.split(";");
    for (unsigned int i = 0; i<strs.size(); ++i) {
        std::vector<int> ints = strs[i].splitInt(",", -1);
        int x1 = ints.size()>0 ? ints[0] : -1;
        int y1 = ints.size()>1 ? ints[1] : -1;
        int x2 = ints.size()>2 ? ints[2] : -1;
        int y2 = ints.size()>3 ? ints[3] : -1;
        if (x1 >= 0 || x2 >= 0 || y1 >= 0 || y2 >= 0)
            rois.push_back(bim::Rectangle<int>(bim::Point<int>(x1, y1), bim::Point<int>(x2, y2)));
    }

    if (rois.size() < 0 || rois[0] < 0) return img;

    xstring template_filename = operations.arguments("-template");
    xstring o_fmt = operations.arguments("-t").toLowerCase();
    xstring options = operations.arguments("-options");

    for (int i = rois.size() - 1; i >= 0; --i) {
        bim::Rectangle<int> r = rois[i];

        // it's allowed to specify only one of the sizes, the other one will be computed
        if (r.p1.x == -1) r.p1.x = 0;
        if (r.p1.y == -1) r.p1.y = 0;
        if (r.p2.x == -1) r.p2.x = (int)img.width() - 1;
        if (r.p2.y == -1) r.p2.y = (int)img.height() - 1;

        if (r.p1.x >= r.p2.x || r.p1.y >= r.p2.y) {
            std::cout << "ROI parameters are invalid, ignored!\n";
            return img;
        }

        if (i == 0) {
            img = img.ROI(r.p1.x, r.p1.y, r.width(), r.height());
        }
        else {
            std::map<std::string, std::string> vars;
            vars["x1"] = xstring::xprintf("%d", r.p1.x);
            vars["y1"] = xstring::xprintf("%d", r.p1.y);
            vars["x2"] = xstring::xprintf("%d", r.p2.x);
            vars["y2"] = xstring::xprintf("%d", r.p2.y);
            xstring fn = template_filename.processTemplate(vars);

            Image o = img.ROI(r.p1.x, r.p1.y, r.width(), r.height());
            o.toFile(fn, o_fmt, options);
        }
    }
    return img;
};

// set ROI
void render_roi_replace(bim::uint64 x, bim::uint64 y, const Image &img, const Image &roi) {
    bim::uint64 w = roi.width();
    bim::uint64 h = roi.height();
    if (x >= img.width())  x = 0;
    if (y >= img.height()) y = 0;
    if (w + x > img.width())  w = img.width() - x;
    if (h + y > img.height()) h = img.height() - y;

    int newLineSize = roi.bytesPerLine();
    int oldLineSize = img.bytesPerLine();
    int Bpp = (long)ceil(((double)img.depth()) / 8.0);

    for (unsigned int sample = 0; sample<img.samples(); ++sample) {
        unsigned char *pl = (unsigned char *)roi.bits(sample);
        unsigned char *plo = ((unsigned char *)img.bits(sample)) + y*oldLineSize + x*Bpp;
        #pragma omp parallel for default(shared)
        for (register bim::int64 yi = 0; yi<h; yi++) {
            unsigned char *ppl = pl + yi*newLineSize;
            unsigned char *pplo = plo + yi*oldLineSize;
            memcpy(pplo, ppl, w*Bpp);
        } // for yi
    } // sample
}

template <typename T, typename F>
void render_roi(bim::uint64 x, bim::uint64 y, const Image &img, const Image &roi, F func, const Image &mask = Image()) {
    bim::uint64 w = (bim::uint64) roi.width();
    bim::uint64 h = (bim::uint64) roi.height();
    if (roi.depth() != img.depth()) return;
    if (roi.samples() != img.samples()) return;
    if (!mask.isEmpty() && mask.depth() != 8) return;
    if (x >= img.width())  x = 0;
    if (y >= img.height()) y = 0;
    if (w + x > img.width())  w = img.width() - x;
    if (h + y > img.height()) h = img.height() - y;

    bim::uint64 newLineSize = roi.bytesPerLine();
    bim::uint64 oldLineSize = img.bytesPerLine();
    bim::uint64 maskLineSize = mask.bytesPerLine();
    int Bpp = (long)ceil(((double)img.depth()) / 8.0);

    for (unsigned int sample = 0; sample<img.samples(); ++sample) {
        unsigned char *pl = (unsigned char *)roi.bits(sample);
        unsigned char *plo = ((unsigned char *)img.bits(sample)) + y*oldLineSize + x*Bpp;
        unsigned char *m = (unsigned char *)mask.bits(0);
        #pragma omp parallel for default(shared)
        for (register bim::int64 yi = 0; yi<h; yi++) {
            unsigned char *ppl = pl + yi*newLineSize;
            unsigned char *pplo = plo + yi*oldLineSize;
            unsigned char *pm = m + yi*maskLineSize;
            func(pplo, pplo, ppl, w, pm);
        } // for yi
    } // sample
}

template <typename T>
void render_roi_func(bim::uint64 x, bim::uint64 y, const Image &img, const Image &roi, Image::FuseMethod method, const Image &mask = Image()) {
    if (method == Image::fmReplace) {
        render_roi<T>(x, y, img, roi, lineops_replace<T>, mask);
    }
    else if (method == Image::fmMax) {
        render_roi<T>(x, y, img, roi, lineops_max<T>, mask);
    }
    else if (method == Image::fmMin) {
        render_roi<T>(x, y, img, roi, lineops_min<T>, mask);
    }
    else if (method == Image::fmAverage) {
        render_roi<T>(x, y, img, roi, lineops_avg<T>, mask);
    }
    else if (method == Image::fmBlend) {
        render_roi<T>(x, y, img, roi, lineops_blend<T>, mask);
    }
    else if (method == Image::fmAdd) {
        render_roi<T>(x, y, img, roi, lineops_add<T>, mask);
    }
    else if (method == Image::fmSubtract) {
        render_roi<T>(x, y, img, roi, lineops_sub<T>, mask);
    }
    else if (method == Image::fmMult) {
        render_roi<T>(x, y, img, roi, lineops_mul<T>, mask);
    }
    else if (method == Image::fmDiv) {
        render_roi<T>(x, y, img, roi, lineops_div<T>, mask);
    }
}

void Image::setROI(bim::uint64 x, bim::uint64 y, const Image &img, const Image &mask, FuseMethod method) {
    bim::uint64 w = img.width();
    bim::uint64 h = img.height();
    if (img.depth() != bmp->i.depth) return;
    if (img.samples() != bmp->i.samples) return;
    if (!mask.isEmpty() && mask.depth() != 8) return;

    if (method == fmReplace && mask.isEmpty()) {
        render_roi_replace(x, y, *this, img);
    } else if (img.depth() == 8 && img.pixelType() == FMT_UNSIGNED)
        render_roi_func<uint8>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 16 && img.pixelType() == FMT_UNSIGNED)
        render_roi_func<uint16>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_UNSIGNED)
        render_roi_func<uint32>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 8 && img.pixelType() == FMT_SIGNED)
        render_roi_func<int8>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 16 && img.pixelType() == FMT_SIGNED)
        render_roi_func<int16>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_SIGNED)
        render_roi_func<int32>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_FLOAT)
        render_roi_func<float32>(x, y, *this, img, method, mask);
    else
    if (img.depth() == 64 && img.pixelType() == FMT_FLOAT)
        render_roi_func<float64>(x, y, *this, img, method, mask);
}

void Image::setROI(bim::uint64 x, bim::uint64 y, bim::uint64 w, bim::uint64 h, const double &value) {
    Image solid(w, h, this->depth(), this->samples(), this->pixelType());
    solid.fill(value);
    this->setROI(x, y, solid);
}

std::string Image::getTextInfo() const {
    return getImageInfoText( &bmp->i ); 
}

#ifdef BIM_USE_IMAGEMANAGER
bool Image::fromFile( const char *fileName, int page ) {
  this->free();
  if (bmp==NULL) return false;

  MetaFormatManager fm;
  bool res = true;

  if (fm.sessionStartRead((const bim::Filename) fileName ) == 0) {
    fm.sessionReadImage( bmp, page );
   
    // getting metadata fields
    fm.sessionParseMetaData(0);
    metadata = fm.get_metadata();   

  } else res = false;

  fm.sessionEnd();

  return res;
}

bool Image::fromPyramidFile(const std::string &fileName, int page, int level, int tilex, int tiley, int tilesize) {
    this->free();
    if (bmp == NULL) return false;
    
    ImageProxy ip(fileName);
    if (!ip.isReady()) return false;

    if (tilex < 0 && level < 1) {
        ip.read(*this, page);
    } else if (tilex<0 && level>0) {
        ip.readLevel(*this, page, level);
    } else {
        if (tilesize < 1) return false;
        ip.readTile(*this, page, tilex, tiley, level, tilesize);
    }

    return true;
}

bool Image::toFile( const char *fileName, const char *formatName, const char *options ) {
  MetaFormatManager fm;
  fm.writeImage ( (const bim::Filename) fileName, bmp,  formatName, options);
  return true;
}

#endif //BIM_USE_IMAGEMANAGER


template <typename T>
void fill_channel ( T *p, const T &v, const unsigned int &num_points ) {
    #pragma omp parallel for default(shared)
    for (bim::int64 x=0; x<num_points; ++x)
        p[x] = v;
}

void Image::fill( double v ) {

  for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
      fill_channel<uint8>( (uint8*) bmp->bits[sample], (uint8) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
      fill_channel<uint16>( (uint16*) bmp->bits[sample], (uint16) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
      fill_channel<uint32>( (uint32*) bmp->bits[sample], (uint32) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
      fill_channel<int8>( (int8*) bmp->bits[sample], (int8) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
      fill_channel<int16>( (int16*) bmp->bits[sample], (int16) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
      fill_channel<int32>( (int32*) bmp->bits[sample], (int32) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
      fill_channel<float32>( (float32*) bmp->bits[sample], (float32) v, bmp->i.width*bmp->i.height );
    else
    if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
      fill_channel<float64>( (float64*) bmp->bits[sample], (float64) v, bmp->i.width*bmp->i.height );

  } // sample
}

bool Image::isUnTypedDepth() const {
  if ( bmp->i.depth==8) return false;
  else
  if ( bmp->i.depth==16) return false;
  else
  if ( bmp->i.depth==32) return false;
  else
  if ( bmp->i.depth==64) return false;
  else
  return true;
}

Image Image::ensureTypedDepth( ) const {
  Image img;
  if (bmp==NULL) return img;

  if (bmp->i.depth != 12 && bmp->i.depth != 4 && bmp->i.depth != 1) {
    return *this;
  }

  bim::uint64 w = bmp->i.width;
  bim::uint64 h = bmp->i.height;
  
  unsigned int out_depth = 8;
  if (bmp->i.depth == 12) out_depth = 16;

  if ( img.alloc( w, h, bmp->i.samples, out_depth ) == 0 )
  for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
    for (unsigned int y=0; y<h; ++y ) {
      void *dest = img.scanLine( sample, y ); 
      void *src = this->scanLine( sample, y );

      if (bmp->i.depth == 1)
        cnv_buffer_1to8bit( (unsigned char *)dest, (unsigned char *)src, w );
      else
      if (bmp->i.depth == 4)
        cnv_buffer_4to8bit( (unsigned char *)dest, (unsigned char *)src, w );
      else
      if (bmp->i.depth == 12)
        cnv_buffer_12to16bit( (unsigned char *)dest, (unsigned char *)src, w );

    } // for y
  } // sample
  
  img.bmp->i = this->bmp->i;
  img.bmp->i.depth = out_depth;
  img.metadata = this->metadata;
  return img;
}

// return a pointer to the buffer of line y formed in iterleaved format xRGB
// the image must be in 8 bpp, otherwise NULL is returned
bim::uchar *Image::scanLineRGB( bim::uint64 y ) {
  if (depth() != 8) return 0;
  buf.allocate( width()*3 );
  buf.fill(0);
  int chans = bim::min<int>( 3, samples() );

  for (unsigned int s=0; s<(unsigned int) chans; ++s) {
    bim::uchar *line_o = buf.bytes() + s;
    bim::uchar *line_i = scanLine( s, y );        
    for (unsigned int x=0; x<width(); ++x) {
       *line_o = line_i[x];
       line_o += 3;
    } // x
  } // s

  return buf.bytes();
}

//------------------------------------------------------------------------------------
// channels
//------------------------------------------------------------------------------------

TagMap remapMetadata( const TagMap &md, unsigned int samples, const std::vector<int> &mapping ) {

  if (md.size()==0) return md;
  TagMap metadata = md;

  std::vector<std::string> channel_names;
  for (unsigned int i=0; i<samples; ++i)
    channel_names.push_back( metadata.get_value(xstring::xprintf("channel_%d_name",i), xstring::xprintf("%d",i)) );

  for (unsigned int i=0; i<mapping.size(); ++i) {
    xstring new_name("empty");
    if (mapping[i]>=0 || mapping[i]<(int)samples) 
      if (mapping[i]<channel_names.size())
        new_name = channel_names[mapping[i]];
      else
        new_name = xstring::xprintf("%d",i);

    metadata.set_value( xstring::xprintf("channel_%d_name",i), new_name);
  }

  return metadata;
}

// fast but potentially dangerous function! It will affect all shared references to the same image!!!
// do deepCopy() before if you might have some shared references
// the result will have as many channels as there are entries in mapping
// invalid channel numbers or -1 will become black channels
// all black channels will point to the same area, so do deepCopy() if you'll modify them
void Image::remapChannels( const std::vector<int> &mapping ) {

  metadata = remapMetadata( metadata, bmp->i.samples, mapping );

  // check if we have any black channels
  bool empty_channels = false;
  for (unsigned int i=0; i<mapping.size(); ++i)
    if (mapping[i]<0 || mapping[i]>=(int)bmp->i.samples) {
      empty_channels = true;
      break;
    }

  // if there are empty channels, allocate space for one channel and init it to 0
  void *empty_buffer = NULL;
  long size = bytesPerChan( );
  if (empty_channels) {
    empty_buffer = new bim::uchar [ size ];
    memset( empty_buffer, 0, size );
  }

  // create a map to actual channel pointers
  std::vector<void*> channel_map( mapping.size() );
  
  for (unsigned int i=0; i<mapping.size(); ++i)
    if (mapping[i]<0 || mapping[i]>=(int)bmp->i.samples)
      channel_map[i] = empty_buffer;
    else
      channel_map[i] = bmp->bits[mapping[i]];

  // find unreferenced channels and destroy them
  for (unsigned int sample=0; sample<bmp->i.samples; ++sample) {
    bool found = false;
    for (unsigned int j=0; j<channel_map.size(); ++j)
      if (bmp->bits[sample] == channel_map[j]) { found = true; break; }
    
    if (!found) {
      delete [] (bim::uchar*) bmp->bits[sample];
      bmp->bits[sample] = NULL;
    }
  }

  // reinit channels
  for (unsigned int sample=0; sample<BIM_MAX_CHANNELS; ++sample)
    bmp->bits[sample] = NULL;

  // map channels
  for (unsigned int sample=0; sample<channel_map.size(); ++sample)
    bmp->bits[sample] = channel_map[sample];
  
  bmp->i.samples = (unsigned int) channel_map.size();
}

void Image::remapToRGB( ) {
  if ( samples() == 3 ) return;
  std::vector<int> map;
  if ( samples() == 1 ) {
    map.push_back(0);
    map.push_back(-1);
    map.push_back(-1);
  }
  if ( samples() == 2 ) {
    map.push_back(0);
    map.push_back(1);
    map.push_back(-1);
  }
  if ( samples() >= 3 ) {
    map.push_back(0);
    map.push_back(1);
    map.push_back(2);
  }
  remapChannels( map );
}

void Image::extractChannel( int c ) {
  std::vector<int> map;
  map.push_back(c);
  remapChannels( map );
}


//------------------------------------------------------------------------------------
// resize
//------------------------------------------------------------------------------------

TagMap resizeMetadata( const TagMap &md, unsigned int w_to, unsigned int h_to, unsigned int w_in, unsigned int h_in ) {
  TagMap metadata = md;
  if ( metadata.hasKey("pixel_resolution_x") ) {
    double new_res = metadata.get_value_double("pixel_resolution_x", 0) * ((double) w_in /(double) w_to );
    metadata.set_value("pixel_resolution_x", new_res);
  }
  if ( metadata.hasKey("pixel_resolution_y") ) {
    double new_res = metadata.get_value_double("pixel_resolution_y", 0) * ((double) h_in /(double) h_to );
    metadata.set_value("pixel_resolution_y", new_res);
  }
  return metadata;
}

template <typename T>
void downsample_line ( void *pdest, void *psrc1, void *psrc2, bim::uint64 &w ) {
  T *src1 = (T*) psrc1;
  T *src2 = (T*) psrc2;
  T *dest = (T*) pdest;

  unsigned int x2=0;
  for (unsigned int x=0; x<w; ++x) {
    dest[x] = (src1[x2] + src1[x2+1] + src2[x2] + src2[x2+1]) / 4.0;
    x2+=2;
  }
}

Image Image::downSampleBy2x( ) const {
  Image img;
  if (bmp==NULL) return img;
  bim::uint64 w = bmp->i.width / 2;
  bim::uint64 h = bmp->i.height / 2;
  if (img.alloc( w, h, bmp->i.samples, bmp->i.depth )!=0) return img;

  for (int sample=0; sample<(int)bmp->i.samples; ++sample ) {
    #pragma omp parallel for default(shared)
    for (int y=0; y<(int)h; ++y ) {
      unsigned int y2 = y*2;
      void *dest = img.scanLine( sample, y ); 
      void *src1 = this->scanLine( sample, y2 );
      void *src2 = this->scanLine( sample, y2+1 );

      if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
        downsample_line<uint8> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
        downsample_line<uint16> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
        downsample_line<uint32> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
        downsample_line<int8> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
        downsample_line<int16> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
        downsample_line<int32> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
        downsample_line<float32> ( dest, src1, src2, w );
      else
      if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
        downsample_line<float64> ( dest, src1, src2, w );
    }
  } // sample

  img.bmp->i = this->bmp->i;
  img.bmp->i.width  = w;
  img.bmp->i.height = h;
  img.metadata = resizeMetadata( this->metadata, w, h, width(), height() );
  return img;
}

//------------------------------------------------------------------------------------
// Interpolation
//------------------------------------------------------------------------------------

template <typename T, typename Tw>
void image_resample ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                      const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in, Image::ResizeMethod method ) {

  if (method == Image::szNearestNeighbor)
    image_resample_NN<T, Tw>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );

  if (method == Image::szBiLinear)
    image_resample_BL<T, Tw>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );

  if (method == Image::szBiCubic)
    image_resample_BC<T, Tw>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );

}

Image Image::resample( bim::uint w, bim::uint h, ResizeMethod method, bool keep_aspect_ratio ) const {
  Image img;
  if (bmp==NULL) return img;
  if (bmp->i.width==w && bmp->i.height==h) return *this;
  if (w==0 && h==0) return *this;

  if (keep_aspect_ratio)
    if ( (width()/(float)w) >= (height()/(float)h) ) h = 0; else w = 0;

  // it's allowed to specify only one of the sizes, the other one will be computed
  if (w == 0)
    w = bim::round<unsigned int>( width() / (height()/(float)h) );
  if (h == 0)
    h = bim::round<unsigned int>( height() / (width()/(float)w) );

  if (img.alloc( w, h, bmp->i.samples, bmp->i.depth )!= 0) return img;

  for (int sample=0; sample<(int)bmp->i.samples; ++sample ) {

    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
      image_resample<bim::uchar, float>( (bim::uchar*) img.bits(sample), img.width(), img.height(),img.width(),
                                 (bim::uchar*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
      image_resample<uint16, float>( (uint16*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (uint16*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
      image_resample<uint32, double>( (uint32*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (uint32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
      image_resample<int8, float>( (int8*) img.bits(sample), img.width(), img.height(),img.width(),
                                 (int8*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
      image_resample<int16, float>( (int16*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (int16*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
      image_resample<int32, double>( (int32*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (int32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
      image_resample<float32, double>( (float32*) img.bits(sample), img.width(), img.height(),img.width(),
                                   (float32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );
    else
    if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
      image_resample<float64, double>( (float64*) img.bits(sample), img.width(), img.height(),img.width(),
                                   (float64*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, method );

  } // sample

  img.bmp->i = this->bmp->i;
  img.bmp->i.width  = w;
  img.bmp->i.height = h;
  img.metadata = resizeMetadata( this->metadata, w, h, width(), height() );
  return img;
}

Image Image::resize( bim::uint w, bim::uint h, ResizeMethod method, bool keep_aspect_ratio ) const {
  if (bmp==NULL) return Image();
  if (bmp->i.width==w && bmp->i.height==h) return *this;
  if (w==0 && h==0) return *this;

  if (keep_aspect_ratio)
    if ( (width()/(float)w) >= (height()/(float)h) ) h = 0; else w = 0;

  // it's allowed to specify only one of the sizes, the other one will be computed
  if (w == 0)
    w = bim::round<unsigned int>( width() / (height()/(float)h) );
  if (h == 0)
    h = bim::round<unsigned int>( height() / (width()/(float)w) );

  // use pyramid if the size difference is large enough
  double vr = (double) std::max<double>(w, width()) / (double) std::min<double>(w, width());
  double hr = (double) std::max<double>(h, height()) / (double) std::min<double>(h, height());
  double rat = std::max(vr, hr);
  if (rat<1.9 || width()<=4096 && height()<=4096 || (unsigned int)w>=width() && (unsigned int)h>=height()) 
    return resample( w, h, method, keep_aspect_ratio );

  // use image pyramid
  ImagePyramid pyramid;
  pyramid.createFrom( *this );
  int level = pyramid.levelClosestTop( w, h );
  Image *image = pyramid.imageAt(level);
  if (image->isNull()) return resample( w, h, method, keep_aspect_ratio );
  Image img = image->resample( w, h, method, keep_aspect_ratio );
  pyramid.clear();

  // copy some important info stored in the original image
  img.bmp->i = this->bmp->i;
  img.bmp->i.width  = w;
  img.bmp->i.height = h;
  img.metadata = resizeMetadata( this->metadata, w, h, width(), height() );
  return img;
}


Image op_resize_resample(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c, const bim::xstring &operation) {
    std::vector<xstring> strl = arguments.split(",");

    unsigned int w = 0, h = 0;
    if (strl.size() >= 2) {
        w = strl[0].toInt(0);
        h = strl[1].toInt(0);
    }

    Image::ResizeMethod resize_method = Image::szNearestNeighbor;
    if (strl.size()>2) {
        if (strl[2].toLowerCase() == "nn") resize_method = Image::szNearestNeighbor;
        if (strl[2].toLowerCase() == "bl") resize_method = Image::szBiLinear;
        if (strl[2].toLowerCase() == "bc") resize_method = Image::szBiCubic;
    }

    bool resize_preserve_aspect_ratio = false;
    bool resize_no_upsample = false;
    if (strl.size()>3) {
        if (strl[3].toLowerCase() == "ar") resize_preserve_aspect_ratio = true;
        if (strl[3].toLowerCase() == "mx") { resize_preserve_aspect_ratio = true; resize_no_upsample = true; }
        if (strl[3].toLowerCase() == "noup") { resize_preserve_aspect_ratio = true; resize_no_upsample = true; }
    }

    if (w <= 0 && h <= 0) return img;
    if (resize_no_upsample && img.width() <= w && img.height() <= h ) return img;
    if (w <= 0 || h <= 0) resize_preserve_aspect_ratio = false;

    if (resize_preserve_aspect_ratio)
    if ((img.width() / (float)w) >= (img.height() / (float)h)) h = 0; else w = 0;

    // it's allowed to specify only one of the sizes, the other one will be computed
    if (w == 0)
        w = bim::round<unsigned int>(img.width() / (img.height() / (float)h));
    if (h == 0)
        h = bim::round<unsigned int>(img.height() / (img.width() / (float)w));

    if (operation == "-resize")
        return img.resize(w, h, resize_method);
    else
        return img.resample(w, h, resize_method);
};

Image operation_resize(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return op_resize_resample(img, arguments, operations, hist, c, "-resize");
};

Image operation_resample(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return op_resize_resample(img, arguments, operations, hist, c, "-resample");
};


//------------------------------------------------------------------------------------
// Rotation
//------------------------------------------------------------------------------------

TagMap rotateMetadata( const TagMap &md, const double &deg ) {
  TagMap metadata = md;
  if (fabs(deg)!=90) return metadata;

  if ( metadata.hasKey("pixel_resolution_x") && metadata.hasKey("pixel_resolution_y") ) {
    double y_res = metadata.get_value_double("pixel_resolution_x", 0);
    double x_res = metadata.get_value_double("pixel_resolution_y", 0);
    metadata.set_value("pixel_resolution_x", x_res);
    metadata.set_value("pixel_resolution_y", y_res);
  }
  return metadata;
}

template <typename T>
void image_rotate ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                      const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in, 
                      double deg ) {

  if (deg == 90)
    image_rotate_right<T>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );

  if (deg == -90)
    image_rotate_left<T>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );

  if (deg == 180) {
    image_flip<T>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in );
    image_mirror<T>( pdest, w_to, h_to, offset_to, pdest, w_to, h_to, offset_to );
  }

}

// only available values now are +90, -90 and 180
Image Image::rotate( double deg ) const {
  Image img;
  if (bmp==NULL) return img;
  if (deg!=90 && deg!=-90 && deg!=180) return *this;

  int w = this->width();
  int h = this->height();
  if (fabs(deg) == 90) {
    h = this->width();
    w = this->height();
  }

  if ( img.alloc( w, h, bmp->i.samples, bmp->i.depth ) == 0 )
  for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {

    if (bmp->i.depth==8)
      image_rotate<bim::uchar>( (bim::uchar*) img.bits(sample), img.width(), img.height(),img.width(),
                               (bim::uchar*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, deg );
    else
    if (bmp->i.depth==16)
      image_rotate<uint16>( (uint16*) img.bits(sample), img.width(), img.height(),img.width(),
                                (uint16*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, deg );
    else
    if (bmp->i.depth==32)
      image_rotate<uint32>( (uint32*) img.bits(sample), img.width(), img.height(),img.width(),
                                (uint32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, deg );
    else
    if (bmp->i.depth==64)
      image_rotate<float64>( (float64*) img.bits(sample), img.width(), img.height(),img.width(),
                                (float64*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width, deg );
  } // sample

  img.bmp->i = this->bmp->i;
  img.bmp->i.width  = w;
  img.bmp->i.height = h;
  img.metadata = rotateMetadata( this->metadata, deg );
  return img;
}

Image Image::rotate_guess() const {
    double angle = 0;
    bool mirror = false;
    
    xstring orientation = this->metadata.get_value( "Exif/Image/Orientation", "" );

    if (orientation == "top, left")           angle = 0; // exif value 1
    else if (orientation == "top, right")     { angle = 0; mirror = true; } // exif value 2
    else if (orientation == "bottom, right")  angle = 180; // exif value 3
    else if (orientation == "bottom, left")   { angle = 180; mirror = true; } // exif value 4
    else if (orientation == "left, top")      { angle = 90; mirror = true; } // exif value 5
    else if (orientation == "right, top")     angle = 90; // exif value 6
    else if (orientation == "right, bottom")  { angle = -90; mirror = true; } // exif value 7
    else if (orientation == "left, bottom")   angle = -90; // exif value 8

    Image img = this->rotate( angle );
    if (mirror)
        img = img.mirror();

    // reset orientation tag
    if (metadata.hasKey("Exif/Image/Orientation"))
        img.metadata.set_value( "Exif/Image/Orientation", "top, left" );  

    return img;
}

Image Image::mirror() const {
    if (bmp==NULL) return Image();
    Image img = this->deepCopy();

    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
        if (bmp->i.depth==8)
            image_mirror<bim::uchar>( (bim::uchar*) img.bits(sample), img.width(), img.height(),img.width(),
                                      (bim::uchar*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==16)
            image_mirror<uint16>( (uint16*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (uint16*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==32)
            image_mirror<uint32>( (uint32*) img.bits(sample), img.width(), img.height(),img.width(),
                                  (uint32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==64)
            image_mirror<float64>( (float64*) img.bits(sample), img.width(), img.height(),img.width(),
                                   (float64*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
    } // sample

    img.metadata = this->metadata;
    return img;
}

Image Image::flip() const {
    if (bmp==NULL) return Image();
    Image img = this->deepCopy();

    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
        if (bmp->i.depth==8)
            image_flip<bim::uchar>( (bim::uchar*) img.bits(sample), img.width(), img.height(),img.width(),
                                    (bim::uchar*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==16)
            image_flip<uint16>( (uint16*) img.bits(sample), img.width(), img.height(),img.width(),
                                (uint16*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==32)
            image_flip<uint32>( (uint32*) img.bits(sample), img.width(), img.height(),img.width(),
                                (uint32*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
        else
        if (bmp->i.depth==64)
            image_flip<float64>( (float64*) img.bits(sample), img.width(), img.height(),img.width(),
                                 (float64*) bmp->bits[sample], bmp->i.width, bmp->i.height, bmp->i.width );
    } // sample

    img.metadata = this->metadata;
    return img;
}

Image operation_rotate(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    if (arguments.toLowerCase() == "guess") {
        return img.rotate_guess();
    }
    else if (arguments.toDouble(0) != 0) {
        double rotate_angle = arguments.toDouble(0);
        if (rotate_angle != 0 && rotate_angle != 90 && rotate_angle != -90 && rotate_angle != 180) {
            std::cout << "This rotation angle value is not yet supported...\n";
            return img;
        }

        return img.rotate(rotate_angle);
    }
    return img;
};

Image operation_mirror(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return img.mirror();
};

Image operation_flip(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return img.flip();
};

//------------------------------------------------------------------------------------
// Pixel Arithmetic
//------------------------------------------------------------------------------------

template <typename T, typename F>
bool image_arithmetic(const Image &img, const Image &ar, F func, const Image &mask) {
    if (ar.width() != img.width()) return false;
    if (ar.height() != img.height()) return false;
    if (ar.samples() != img.samples()) return false;
    if (ar.depth() != img.depth()) return false;
    if (!mask.isEmpty() && mask.depth() != 8) return false;

    bim::uint64 w = (bim::uint64) img.width();
    bim::uint64 h = (bim::uint64) img.height();

    for (unsigned int sample = 0; sample<img.samples(); ++sample) {
        #pragma omp parallel for default(shared)
        for (bim::int64 y = 0; y<(bim::int64)h; ++y) {
            void *src = ar.scanLine(sample, y);
            void *dest = img.scanLine(sample, y);
            bim::uint8 *m = mask.isEmpty() ? NULL : mask.scanLine(0, y);
            func(dest, dest, src, w, m);
        }
    } // sample
    return true;
}

template <typename T, typename F>
bool Image::image_arithmetic(const Image &img, F func, const Image &mask) {
    return image_arithmetic<T, F>(*this, img, F, mask);
}

template <typename T>
bool image_arithmetic_func(const Image &img, const Image &ar, const Image &mask, Image::ArithmeticOperators op) {
    if (op == Image::aoAdd) {
        return image_arithmetic<T>(img, ar, lineops_add<T>, mask);
    }
    else if (op == Image::aoSub) {
        return image_arithmetic<T>(img, ar, lineops_sub<T>, mask);
    }
    else if (op == Image::aoMul) {
        return image_arithmetic<T>(img, ar, lineops_mul<T>, mask);
    }
    else if (op == Image::aoDiv) {
        return image_arithmetic<T>(img, ar, lineops_div<T>, mask);
    }
    else if (op == Image::aoMax) {
        return image_arithmetic<T>(img, ar, lineops_max<T>, mask);
    }
    else if (op == Image::aoMin) {
        return image_arithmetic<T>(img, ar, lineops_min<T>, mask);
    }
    else
        return false;
}

bool image_arithmetic_type(const Image &img, const Image &ar, const Image &mask, Image::ArithmeticOperators op) {
    if (img.depth() == 8 && img.pixelType() == FMT_UNSIGNED)
        return image_arithmetic_func<uint8>(img, ar, mask, op);
    else
    if (img.depth() == 16 && img.pixelType() == FMT_UNSIGNED)
        return image_arithmetic_func<uint16>(img, ar, mask, op);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_UNSIGNED)
        return image_arithmetic_func<uint32>(img, ar, mask, op);
    else
    if (img.depth() == 8 && img.pixelType() == FMT_SIGNED)
        return image_arithmetic_func<int8>(img, ar, mask, op);
    else
    if (img.depth() == 16 && img.pixelType() == FMT_SIGNED)
        return image_arithmetic_func<int16>(img, ar, mask, op);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_SIGNED)
        return image_arithmetic_func<int32>(img, ar, mask, op);
    else
    if (img.depth() == 32 && img.pixelType() == FMT_FLOAT)
        return image_arithmetic_func<float32>(img, ar, mask, op);
    else
    if (img.depth() == 64 && img.pixelType() == FMT_FLOAT)
        return image_arithmetic_func<float64>(img, ar, mask, op);
    else
        return false;
}

//--------------------------------------------------------------------------    
// image-based operators 
//--------------------------------------------------------------------------

bool Image::imageArithmetic(const Image &img, Image::ArithmeticOperators op, const Image &mask) {
    return image_arithmetic_type(*this, img, mask, op);
}

Image Image::operator+ (const Image &img) {
    Image r = this->deepCopy();
    r.imageArithmetic( img, Image::aoAdd );
    return r;
}

Image Image::operator- (const Image &img) {
    Image r = this->deepCopy();
    r.imageArithmetic(img, Image::aoSub);
    return r;
}

Image Image::operator/ (const Image &img) {
    Image r = this->deepCopy();
    r.imageArithmetic(img, Image::aoDiv);
    return r;
}

Image Image::operator* (const Image &img) {
    Image r = this->deepCopy();
    r.imageArithmetic(img, Image::aoMul);
    return r;
}

//--------------------------------------------------------------------------    
// numeric operators 
//--------------------------------------------------------------------------

struct numeric_args {
    double v;
    Image::ArithmeticOperators op;
};

template <typename T>
void operation_numeric(void *p, const bim::uint64 &w, const numeric_args &args, const unsigned char *m) {
    T *src = (T*)p;
    double v = args.v;

    if (args.op == Image::aoAdd) {
        for (bim::int64 x = 0; x<(bim::int64)w; ++x)
            src[x] = m[x]>0 ? bim::trim<T, T>(src[x] + v) : src[x];
    }
    else if (args.op == Image::aoSub) {
        for (bim::int64 x = 0; x<(bim::int64)w; ++x)
            src[x] = m[x]>0 ? bim::trim<T, T>(src[x] - v) : src[x];
    }
    else if (args.op == Image::aoMul) {
        for (bim::int64 x = 0; x<(bim::int64)w; ++x)
            src[x] = m[x]>0 ? bim::trim<T, T>(src[x] * v) : src[x];
    }
    else if (args.op == Image::aoDiv) {
        for (bim::int64 x = 0; x<(bim::int64)w; ++x)
            src[x] = m[x]>0 ? bim::trim<T, T>(src[x] / v) : src[x];
    }
}

bool Image::operationArithmetic(const double &v, const Image::ArithmeticOperators &op, const Image &mask) {
    numeric_args args;
    args.v = v;
    args.op = op;

    if (bmp->i.depth == 8 && bmp->i.pixelType == FMT_UNSIGNED)
        return pixel_operations<uint8>(operation_numeric<uint8>, args, mask);
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType == FMT_UNSIGNED)
        return pixel_operations<uint16>(operation_numeric<uint16>, args, mask);
    else
    if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_UNSIGNED)
        return pixel_operations<uint32>(operation_numeric<uint32>, args, mask);
    else
    if (bmp->i.depth == 8 && bmp->i.pixelType == FMT_SIGNED)
        return pixel_operations<int8>(operation_numeric<int8>, args, mask);
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType == FMT_SIGNED)
        return pixel_operations<int16>(operation_numeric<int16>, args, mask);
    else
    if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_SIGNED)
        return pixel_operations<int32>(operation_numeric<int32>, args, mask);
    else
    if (bmp->i.depth == 32 && bmp->i.pixelType == FMT_FLOAT)
        return pixel_operations<float32>(operation_numeric<float32>, args, mask);
    else
    if (bmp->i.depth == 64 && bmp->i.pixelType == FMT_FLOAT)
        return pixel_operations<float64>(operation_numeric<float64>, args, mask);
    else
        return false;
}

Image Image::operator+ (const double &v) {
    Image r = this->deepCopy();
    r.operationArithmetic(v, Image::aoAdd);
    return r;
}

Image Image::operator- (const double &v) {
    Image r = this->deepCopy();
    r.operationArithmetic(v, Image::aoSub);
    return r;
}

Image Image::operator/ (const double &v) {
    Image r = this->deepCopy();
    r.operationArithmetic(v, Image::aoDiv);
    return r;
}

Image Image::operator* (const double &v) {
    Image r = this->deepCopy();
    r.operationArithmetic(v, Image::aoMul);
    return r;
}

//------------------------------------------------------------------------------------
// Negative
//------------------------------------------------------------------------------------

template <typename T>
void image_negative ( void *pdest, void *psrc, unsigned int &w ) {
    T *src = (T*) psrc;
    T *dest = (T*) pdest;
    T max_val = std::numeric_limits<T>::max();
    #pragma omp parallel for default(shared)
    for (bim::int64 x=0; x<w; ++x)
        dest[x] = max_val - src[x];
}

Image Image::negative() const {
  Image img;
  if (bmp==NULL) return img;
  img = this->deepCopy();
  unsigned int plane_size_pixels = img.width()*img.height();

  for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {

    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
      image_negative<uint8>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
      image_negative<uint16>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
      image_negative<uint32>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
      image_negative<int8>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
      image_negative<int16>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
      image_negative<int32>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
      image_negative<float32>( img.bits(sample), bmp->bits[sample], plane_size_pixels );
    else
    if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
      image_negative<float64>( img.bits(sample), bmp->bits[sample], plane_size_pixels );

  } // sample

  return img;
}

Image operation_negative(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    return img.negative();
};

//------------------------------------------------------------------------------------
// Trim
//------------------------------------------------------------------------------------

template <typename T>
void image_trim ( void *pdest, void *psrc, unsigned int &w, double min_v, double max_v ) {
    T *src = (T*) psrc;
    T *dest = (T*) pdest;
    #pragma omp parallel for default(shared)
    for (bim::int64 x=0; x<w; ++x)
        dest[x] = bim::trim<T>(src[x], min_v, max_v);
}

void Image::trim(double min_v, double max_v) const {
  unsigned int plane_size_pixels = this->width()*this->height();

  for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {

    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
      image_trim<uint8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
      image_trim<uint16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
      image_trim<uint32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
      image_trim<int8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
      image_trim<int16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
      image_trim<int32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
      image_trim<float32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );
    else
    if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
      image_trim<float64>( this->bits(sample), bmp->bits[sample], plane_size_pixels, min_v, max_v );

  } // sample

}


//------------------------------------------------------------------------------------
// color_levels
//------------------------------------------------------------------------------------

template <typename T>
void image_color_levels ( void *pdest, void *psrc, unsigned int &w, double val_min, double val_max, double gamma ) {
    double out_min = bim::lowest<T>();
    double out_max = std::numeric_limits<T>::max();
    double out_range = out_max - out_min + 1;

    gamma = 1.0/gamma;
    val_max = pow(val_max, gamma);
    val_min = pow(val_min, gamma);
    double range = val_max - val_min;

    T *src = (T*) psrc;
    T *dest = (T*) pdest;
    #pragma omp parallel for default(shared)
    for (bim::int64 x=0; x<w; ++x) {
        double px = (pow((double)src[x], gamma)-val_min)*out_range/range;
        dest[x] = bim::trim<T,double>(px);
    }
}

void Image::color_levels( const double &val_min, const double &val_max, const double &gamma ) {
    double vmax = val_max, vmin = val_min;
    if (vmin == vmax) {
        ImageHistogram hist(*this);
        vmin = hist[0]->min_value();
        vmax = hist[0]->max_value();
        for (unsigned int sample=1; sample<bmp->i.samples; ++sample ) {
            vmin = bim::min<double>(vmin, hist[sample]->min_value());
            vmax = bim::max<double>(vmax, hist[sample]->max_value());
        }
    }

    unsigned int plane_size_pixels = this->width()*this->height();
    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
        if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
            image_color_levels<uint8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
            image_color_levels<uint16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
            image_color_levels<uint32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
            image_color_levels<int8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
            image_color_levels<int16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
            image_color_levels<int32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
            image_color_levels<float32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );
        else
        if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
            image_color_levels<float64>( this->bits(sample), bmp->bits[sample], plane_size_pixels, vmin, vmax, gamma );

    } // sample
}

Image operation_levels(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    double gamma = 1;
    double minv = 0, maxv = 0;
    std::vector<double> strl = arguments.splitDouble(",");
    if (strl.size()>0) minv = strl[0];
    if (strl.size()>1) maxv = strl[1];
    if (strl.size()>2) gamma = strl[2];

    img.color_levels(minv, maxv, gamma);
    return img;
};


//------------------------------------------------------------------------------------
// color_levels
//------------------------------------------------------------------------------------

template <typename T>
void image_brightness_contrast ( void *pdest, void *psrc, unsigned int &w, double b, double c, Histogram *h ) {
    double a = h->average();
    b *= a;
    T *src = (T*) psrc;
    T *dest = (T*) pdest;
    #pragma omp parallel for default(shared)
    for (bim::int64 x=0; x<w; ++x) {
        double px = ((double)src[x]);
        px = (px-a) * c + a;
        px += b;
        dest[x] = bim::trim<T, double>(px);
    }
}

// same as photoshop brightness/contrast command, both values in range [-100, 100]
void Image::color_brightness_contrast( const int &brightness, const int &contrast ) {
    unsigned int plane_size_pixels = this->width()*this->height();
    ImageHistogram hist(*this);
    double b = (brightness/100.0);
    double c = contrast>=0 ? (contrast/100.0)+1 : 1.0-(contrast/-200.0);

    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
        if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
            image_brightness_contrast<uint8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
            image_brightness_contrast<uint16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
            image_brightness_contrast<uint32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
            image_brightness_contrast<int8>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
            image_brightness_contrast<int16>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
            image_brightness_contrast<int32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
            image_brightness_contrast<float32>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );
        else
        if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
            image_brightness_contrast<float64>( this->bits(sample), bmp->bits[sample], plane_size_pixels, b, c, hist[sample] );

    } // sample
}

Image operation_brightnesscontrast(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    int brightness = 0;
    int contrast = 0;
    std::vector<int> strl = arguments.splitInt(",");
    if (strl.size()>0) brightness = strl[0];
    if (strl.size()>1) contrast = strl[1];

    if (brightness != 0.0 || contrast != 0.0)
        img.color_brightness_contrast(brightness, contrast);
    return img;
};

//------------------------------------------------------------------------------------
// pixel_counter
//------------------------------------------------------------------------------------

template <typename T>
bim::uint64 image_pixel_counter ( const Image &img, const unsigned int &sample, const double &threshold_above ) {
    std::vector<bim::uint64> c(img.height(), 0);
    #pragma omp parallel for default(shared)
    for (bim::int64 y=0; y<(bim::int64)img.height(); ++y) {
        T *src = (T*) img.scanLine( sample, y );
        for (bim::int64 x=0; x<(bim::int64)img.width(); ++x) {
            if (src[x]>threshold_above)
                c[y]++;
        }
    }

    bim::uint64 count = 0;
    for (bim::int64 y=0; y<(bim::int64)img.height(); ++y) {
        count += c[y];
    }
    return count;
}

// returns a number of pixels above the threshold
bim::uint64 Image::pixel_counter( const unsigned int &sample, const double &threshold_above ) {
    unsigned int plane_size_pixels = this->width()*this->height();

    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
        return image_pixel_counter<uint8>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
        return image_pixel_counter<uint16>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
        return image_pixel_counter<uint32>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
        return image_pixel_counter<int8>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
        return image_pixel_counter<int16>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
        return image_pixel_counter<int32>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
        return image_pixel_counter<float32>( *this, sample, threshold_above );
    else
    if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
        return image_pixel_counter<float64>( *this, sample, threshold_above );

    return 0;
}

std::vector<bim::uint64> Image::pixel_counter( const double &threshold_above ) {
    std::vector<bim::uint64> counts(this->samples());
    for (unsigned int sample=0; sample<bmp->i.samples; ++sample )
        counts[sample] = this->pixel_counter( sample, threshold_above );
    return counts;
}

//------------------------------------------------------------------------------------
// Row scan
//------------------------------------------------------------------------------------
template <typename T>
void image_row_scan ( void *pdest, const Image &img, bim::uint64 &sample, bim::uint64 &x ) {
    T *dest = (T*) pdest;
    #pragma omp parallel for default(shared)
    for (bim::int64 y=0; y<(bim::int64)img.height(); ++y) {
        T *src = (T*) img.scanLine( sample, y );
        dest[y] = src[x];
    }
}

void Image::scanRow( bim::uint64 sample, bim::uint64 x, bim::uchar *buf ) const {

  if (bmp->i.depth==8)
    image_row_scan<bim::uchar>( buf, *this, sample, x );
  else
  if (bmp->i.depth==16)
    image_row_scan<uint16>( buf, *this, sample, x );
  else
  if (bmp->i.depth==32 && bmp->i.pixelType!=FMT_FLOAT)
    image_row_scan<uint32>( buf, *this, sample, x );
  else
  if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
    image_row_scan<float32>( buf, *this, sample, x );
  else
  if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
    image_row_scan<float64>( buf, *this, sample, x );
}

//------------------------------------------------------------------------------------
// Operations generics
//------------------------------------------------------------------------------------

template <typename T, typename F, typename A>
bool Image::pixel_operations(F func, const A &args, const Image &mask) {
    if (bmp==NULL) return false;
    bim::uint64 w = (bim::uint64) bmp->i.width;
    bim::uint64 h = (bim::uint64) bmp->i.height;
    std::vector<unsigned char> mm(w, 255);

    for (unsigned int sample=0; sample<bmp->i.samples; ++sample ) {
        #pragma omp parallel for default(shared)
        for (bim::int64 y=0; y<(bim::int64)h; ++y ) {
            void *src = this->scanLine( sample, y );
            unsigned char *m = !mask.isEmpty() ? mask.scanLine(0, y) : &mm[0];
            func ( src, w, args, m );
        }
    } // sample
    return true;
}

//------------------------------------------------------------------------------------
// Threshold
//------------------------------------------------------------------------------------

struct threshold_args {
    double th;
    Image::ThresholdTypes method;
};

template <typename T>
void operation_threshold(void *p, const bim::uint64 &w, const threshold_args &args, const unsigned char *m) {
    double th = args.th;
    T *src = (T*) p;
    T max_val = std::numeric_limits<T>::max();
    T min_val = bim::lowest<T>();

    if (args.method == Image::ttLower) {
        for (bim::int64 x=0; x<(bim::int64)w; ++x)
            if (src[x] < th && m[x]>0) 
                src[x] = min_val; 
    } else if (args.method == Image::ttUpper) {
        for (bim::int64 x=0; x<(bim::int64)w; ++x)
        if (src[x] >= th && m[x]>0)
                src[x] = max_val;
    } else if (args.method == Image::ttBoth) { 
        for (bim::int64 x=0; x<(bim::int64)w; ++x)
        if (m[x]>0) {
            if (src[x] < th)
                src[x] = min_val;
            else
                src[x] = max_val;
        }
    }
}

bool Image::operationThreshold(const double &th, const Image::ThresholdTypes &method, const Image &mask) {
    threshold_args args;
    args.th = th;
    args.method = method;

    if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_UNSIGNED)
        return pixel_operations<uint8>  ( operation_threshold<uint8>, args, mask );
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_UNSIGNED)
        return pixel_operations<uint16>(operation_threshold<uint16>, args, mask);
    else      
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_UNSIGNED)
        return pixel_operations<uint32>(operation_threshold<uint32>, args, mask);
    else
    if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_SIGNED)
        return pixel_operations<int8>(operation_threshold<int8>, args, mask);
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_SIGNED)
        return pixel_operations<int16>(operation_threshold<int16>, args, mask);
    else      
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_SIGNED)
        return pixel_operations<int32>(operation_threshold<int32>, args, mask);
    else    
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_FLOAT)
        return pixel_operations<float32>(operation_threshold<float32>, args, mask);
    else      
    if (bmp->i.depth == 64 && bmp->i.pixelType==FMT_FLOAT)
        return pixel_operations<float64>(operation_threshold<float64>, args, mask);
    else
    return false;
}

Image operation_threshold(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    std::vector<xstring> strl = arguments.split(",");
    double threshold = strl[0].toDouble(0);
    Image::ThresholdTypes threshold_operation = Image::ttNone;
    if (strl.size()>1) {
        if (strl[1].toLowerCase() == "lower") threshold_operation = Image::ttLower;
        if (strl[1].toLowerCase() == "upper") threshold_operation = Image::ttUpper;
        if (strl[1].toLowerCase() == "both")  threshold_operation = Image::ttBoth;
    }

    if (threshold_operation != Image::ttNone)
        img.operationThreshold(threshold, threshold_operation);
    return img;
};

//------------------------------------------------------------------------------------
// Channel fusion
//------------------------------------------------------------------------------------

std::vector<bim::DisplayColor> init_fusion_from_meta(const Image &img) {
    std::vector<bim::DisplayColor> out_weighted_fuse_channels;
    std::vector<bim::DisplayColor> channel_colors_default = bim::defaultChannelColors();
    bim::TagMap m = img.get_metadata();
    for (bim::uint i = 0; i<img.samples(); ++i) {
        xstring key = xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), i);
        if (m.hasKey(key)) {
            xstring t = m.get_value(key, "");
            std::vector<int> cmp = t.splitInt(",");
            cmp.resize(3, 0);
            out_weighted_fuse_channels.push_back(bim::DisplayColor(cmp[0], cmp[1], cmp[2]));
        }
        else if (i<channel_colors_default.size()) {
            out_weighted_fuse_channels.push_back(channel_colors_default[i]);
        }
    }
    return out_weighted_fuse_channels;
}

void optimize_channel_map(std::set<int> *v, const Image &img) {
  std::set<int> s;

  // ensure all channels are valid
  for (std::set<int>::iterator i=v->begin(); i!=v->end(); ++i)
    s.insert( bim::trim<int>( *i, -1, img.samples()-1 ) );

  // remove -1 if any other value is present
  if (*s.rbegin()>-1)
    s.erase(-1);

  *v = s;
}

template <typename T>
void fuse_channels ( T *dest, const Image &srci, const std::set<int> &map ) {
    bim::uint64 s = srci.width() * srci.height();
    for (std::set<int>::const_iterator i=map.begin(); i!=map.end(); ++i) {
        T *src = (T*) srci.bits(*i);
        #pragma omp parallel for default(shared)
        for (bim::int64 x=0; x<(bim::int64)s; ++x)
            dest[x] = std::max( dest[x], src[x] );
    }
}

Image Image::fuse( const std::vector< std::set<int> > &mapping ) const {
  Image img;
  if (bmp==NULL) return img;

  std::vector< std::set<int> > map = mapping;
  for (int i=0; i<mapping.size(); ++i) 
    optimize_channel_map( &map[i], *this );
  
  unsigned int c = (unsigned int) map.size();
  if ( img.alloc( bmp->i.width, bmp->i.height, c, bmp->i.depth ) == 0 ) {
    img.fill(0);
    for (unsigned int sample=0; sample<img.samples(); ++sample ) {
      
      if ( *map[sample].begin()==-1 ) continue; // if mapping only has -1 then the output channel should be black
      else 
      if ( map[sample].size()==1 ) // if no fusion is going on simply copy the data
        memcpy( img.bits(sample), bmp->bits[*map[sample].begin()], img.bytesPerChan() );
      else // ok, here we fuse
      if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
        fuse_channels ( (uint8*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
        fuse_channels ( (uint16*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
        fuse_channels ( (uint32*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
        fuse_channels ( (int8*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
        fuse_channels ( (int16*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
        fuse_channels ( (int32*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
        fuse_channels ( (float32*) img.bits(sample), *this, map[sample] );
      else
      if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
        fuse_channels ( (float64*) img.bits(sample), *this, map[sample] );

    } // sample
  }

  img.bmp->i = this->bmp->i;
  img.bmp->i.samples = c;
  img.metadata = this->metadata;
  return img;
}

Image Image::fuse( int red, int green, int blue, int yellow, int magenta, int cyan, int gray ) const {
  std::vector< std::set<int> > mapping;
  std::set<int> rv, gv, bv;
  // Color mixing: Y=R+G, M=R+B, C=B+G, GR=R+G+B

  rv.insert(red);
  rv.insert(yellow);
  rv.insert(magenta);
  rv.insert(gray);

  gv.insert(green);
  gv.insert(yellow);
  gv.insert(cyan);
  gv.insert(gray);

  bv.insert(blue);
  bv.insert(magenta);
  bv.insert(cyan);
  bv.insert(gray);
  
  mapping.push_back(rv);
  mapping.push_back(gv);
  mapping.push_back(bv);

  // each of the vectors will be optimized by the fuse function, no need to take care of that here
  return fuse( mapping );
}

//------------------------------------------------------------------------------------
// Channel fusion
//------------------------------------------------------------------------------------

TagMap fuseMetadata( const TagMap &md, unsigned int samples, const std::vector< std::vector< std::pair<int,float> > > &map ) {

    if (md.size()==0) return md;
    TagMap metadata = md;

    std::vector<std::string> channel_names;
    for (unsigned int i=0; i<samples; ++i)
        channel_names.push_back( metadata.get_value(xstring::xprintf("channel_%d_name",i), xstring::xprintf("%d",i)) );

    for (unsigned int i=0; i<map.size(); ++i) {
        xstring new_name;
        for (std::vector< std::pair<int,float> >::const_iterator j=map[i].begin(); j!=map[i].end(); ++j) {
            if (j->second>0 && j->first>0 && j->first<channel_names.size()) {
                if (!new_name.empty()) 
                    new_name += " + ";
                new_name += channel_names[j->first];
            }
            //new_name = xstring::xprintf("%d",i);
        } 
        metadata.set_value( xstring::xprintf("channel_%d_name",i), new_name);
    }

    return metadata;
}

/*
template <typename T, typename To>
void norm_cpy ( Image *img, const Image &srci, double sh, double mu ) {
  for (unsigned int sample=0; sample<srci.samples(); ++sample ) {
    for (unsigned int y=0; y<srci.height(); ++y) {
        T *src = (T*) srci.scanLine(sample, y);
        To *dest = (To*) img->scanLine(sample, y); 
        for (unsigned int x=0; x<srci.width(); ++x)
            dest[x] = bim::trim<To, double>( (src[x]+sh)*mu, bim::lowest<T>(), std::numeric_limits<To>::max() );
    } // y
  } // sample
}

// this only works for DOUBLE srci!!!
void norm_copy ( Image *img, const Image &srci, double sh, double mu ) {
    if (img->depth()==8 && img->pixelType()==FMT_UNSIGNED)
      norm_cpy<BIM_DOUBLE, uint8> ( img, srci, sh, mu );
    else
    if (img->depth()==16 && img->pixelType()==FMT_UNSIGNED)
      norm_cpy<BIM_DOUBLE, uint16> ( img, srci, sh, mu );
    else
    if (img->depth()==32 && img->pixelType()==FMT_UNSIGNED)
      norm_cpy<BIM_DOUBLE, uint32> ( img, srci, sh, mu );
    else
    if (img->depth()==8 && img->pixelType()==FMT_SIGNED)
      norm_cpy<BIM_DOUBLE, int8> ( img, srci, sh, mu );
    else
    if (img->depth()==16 && img->pixelType()==FMT_SIGNED)
      norm_cpy<BIM_DOUBLE, int16> ( img, srci, sh, mu );
    else
    if (img->depth()==32 && img->pixelType()==FMT_SIGNED)
      norm_cpy<BIM_DOUBLE, int32> ( img, srci, sh, mu );
    else
    if (img->depth()==32 && img->pixelType()==FMT_FLOAT)
      norm_cpy<BIM_DOUBLE, float32> ( img, srci, sh, mu );
    else
    if (img->depth()==64 && img->pixelType()==FMT_FLOAT)
      norm_cpy<BIM_DOUBLE, float64> ( img, srci, sh, mu );
}
*/

template <typename T, typename To>
void fuse_channels_weights ( Image *img, int sample, const Image &srci, const std::vector< std::pair<int,float> > &map, double shift=0.0, double mult=1.0, Image::FuseMethod method=Image::fmAverage ) {
  double weight_sum=0;
  for (std::vector< std::pair<int,float> >::const_iterator i=map.begin(); i!=map.end(); ++i)
  //  weight_sum += i->second;
  //if (weight_sum==0) weight_sum=1;
      weight_sum++;

  #pragma omp parallel for default(shared)
  for (bim::int64 y=0; y<(bim::int64)srci.height(); ++y) {
    std::vector<T> black(srci.width(), 0);
    std::vector<double> line(srci.width());
    To *dest = (To*) img->scanLine(sample, y); 
    memset(&line[0], 0, srci.width()*sizeof(double));

    for (std::vector< std::pair<int,float> >::const_iterator i=map.begin(); i!=map.end(); ++i) {
      T *src = &black[0];
      if (i->first>=0)
          src = (T*) srci.scanLine(i->first, y);
  
      if (method == Image::fmAverage) {
          for (unsigned int x=0; x<srci.width(); ++x)
              line[x] += ((double)src[x])*i->second;
      } else if (method == Image::fmMax) {
          for (unsigned int x=0; x<srci.width(); ++x)
              line[x] = std::max<double>( ((double)src[x])*i->second, line[x] );
      }
    } // map

    if (method == Image::fmMax) {
        for (unsigned int x=0; x<srci.width(); ++x)
            dest[x] = (To) line[x];
    } else if (method == Image::fmAverage) {
        for (unsigned int x=0; x<srci.width(); ++x)
            dest[x] = bim::trim<To, double>(((line[x] / weight_sum) + shift)*mult, bim::lowest<T>(), std::numeric_limits<To>::max());
    }

  } // y
}

Image Image::fuse( const std::vector< std::vector< std::pair<int,float> > > &map, Image::FuseMethod method, ImageHistogram *hin ) const {
    Image img;
    if (bmp==NULL) return img;

    // compute the input image levels
    ImageHistogram hist;
    if (!hin || !hin->isValid())
        hist.fromImage(*this);
    else
        hist = *hin;

    double in_max = hist.max_value();
    double in_min = hist.min_value();
    double fu_min = in_max;
    double fu_max = in_min;
    for (unsigned int i=0; i<map.size(); ++i) {
        double camax=0, camin=0, canum=0;
        for (std::vector< std::pair<int,float> >::const_iterator j=map[i].begin(); j!=map[i].end(); ++j) {
            if (j->first>=0 && j->second>0) {
                camax += hist[j->first]->max_value() * j->second;
                camin += hist[j->first]->min_value() * j->second;
            }
            canum += 1;
        } 
        fu_max = std::max(fu_max, camax/canum);
        fu_min = std::min(fu_min, camin/canum);
    }
    double shift = in_min-fu_min;
    double mult  = (in_max-in_min)/(fu_max-fu_min);


    unsigned int c = (unsigned int) map.size();
    if ( img.alloc( bmp->i.width, bmp->i.height, c, bmp->i.depth ) == 0 ) {
    //if ( img.alloc( bmp->i.width, bmp->i.height, c, 64, FMT_FLOAT ) == 0 ) { // create a temp double image
        img.fill(0);
        for (unsigned int sample=0; sample<img.samples(); ++sample ) {
            if (map[sample].size() == 1 && map[sample].begin()->first < 0) // if no output is needed for this channel
                continue;
            else
            if (map[sample].size() == 1) // if no fusion is going on simply copy the data
              memcpy( img.bits(sample), bmp->bits[map[sample].begin()->first], img.bytesPerChan() );
            else // ok, here we fuse
            if (bmp->i.depth==8 && bmp->i.pixelType==FMT_UNSIGNED)
              fuse_channels_weights<uint8, uint8> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==16 && bmp->i.pixelType==FMT_UNSIGNED)
              fuse_channels_weights<uint16, uint16> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==32 && bmp->i.pixelType==FMT_UNSIGNED)
              fuse_channels_weights<uint32, uint32> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==8 && bmp->i.pixelType==FMT_SIGNED)
              fuse_channels_weights<int8, int8> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==16 && bmp->i.pixelType==FMT_SIGNED)
              fuse_channels_weights<int16, int16> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==32 && bmp->i.pixelType==FMT_SIGNED)
              fuse_channels_weights<int32, int32> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==32 && bmp->i.pixelType==FMT_FLOAT)
              fuse_channels_weights<float32, float32> ( &img, sample, *this, map[sample], shift, mult, method );
            else
            if (bmp->i.depth==64 && bmp->i.pixelType==FMT_FLOAT)
              fuse_channels_weights<float64, float64> ( &img, sample, *this, map[sample], shift, mult, method );
        } // sample
    }

    // final touches
    img.bmp->i = this->bmp->i;
    img.bmp->i.samples = c;
    //out.metadata = this->metadata;
    img.metadata = fuseMetadata( this->metadata, bmp->i.samples, map );
    return img;

  /*
  // bring the levels up to the original max
  double o_max = hist.max_value();
  double o_min = hist.min_value();

  ImageHistogram fhist(img);
  double f_max = fhist.max_value();
  double f_min = fhist.min_value();

  double shift = o_min-f_min;
  double mult  = (o_max-o_min)/(f_max-f_min);
  */

  /*
  Image out;
  if (out.alloc( bmp->i.width, bmp->i.height, c, bmp->i.depth)==0) {
      norm_copy ( &out, img, shift, mult );

      // final touches
      out.bmp->i = this->bmp->i;
      out.bmp->i.samples = c;
      //out.metadata = this->metadata;
      out.metadata = fuseMetadata( this->metadata, bmp->i.samples, map );
  }
  return out;
  */
}

Image Image::fuseToGrayscale() const {
  std::vector< std::vector< std::pair<int,float> > > mapping;
  std::vector< std::pair<int,float> > gray;

  if (this->samples()==3) {
    gray.push_back( std::make_pair( 0, 0.3f ) );
    gray.push_back( std::make_pair( 1, 0.59f ) );
    gray.push_back( std::make_pair( 2, 0.11f ) );
  } else {
    for (unsigned int i=0; i<this->samples(); ++i)
      gray.push_back( std::make_pair( i, 1.0f ) );
  }
  
  mapping.push_back(gray);
  return fuse( mapping );
}

// in this case we are producing an xRGB image for display, the mapping should contain xRGB values for each image channel
// max value of rgb components defines max contribution, 0 or negative means no contribution
Image Image::fuseToRGB(const std::vector<bim::DisplayColor> &mapping, Image::FuseMethod method, ImageHistogram *hist) const {
    std::vector< std::vector< std::pair<int,float> > > map;
    std::vector< std::pair<int,float> > red, green, blue;
  
    float max_contribution=0; // used to divide each components contribution
    for (int i=0; i<mapping.size(); ++i) {
        max_contribution = std::max<float>(max_contribution, (float) mapping[i].r);
        max_contribution = std::max<float>(max_contribution, (float) mapping[i].g);
        max_contribution = std::max<float>(max_contribution, (float) mapping[i].b);
    }

    for (int i=0; i<mapping.size(); ++i) {
        if (mapping[i].r>0)
            red.push_back( std::make_pair( i, mapping[i].r/max_contribution ) );
        else
            red.push_back( std::make_pair( -1, mapping[i].r/max_contribution ) );
        if (mapping[i].g>0)
            green.push_back( std::make_pair( i, mapping[i].g/max_contribution ) );
        else
            green.push_back( std::make_pair( -1, mapping[i].g/max_contribution ) );
        if (mapping[i].b>0)
            blue.push_back( std::make_pair( i, mapping[i].b/max_contribution ) );
        else
            blue.push_back( std::make_pair( -1, mapping[i].b/max_contribution ) );
    }
  
    map.push_back(red);
    map.push_back(green);
    map.push_back(blue);
    return fuse( map, method, hist );
}

Image operation_fusegrey(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    hist->clear(); // dima: should properly modify instead of clearing
    return img.fuseToGrayscale();
};

Image operation_fusergb(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::FuseMethod fuse_method = Image::fmAverage;
    if (operations.arguments("-fusemethod").toLowerCase() == "m") fuse_method = Image::fmMax; // dima: should be moved into command args

    std::vector<xstring> ch = arguments.split(";");
    // trim trailing empty channels
    for (int c = ch.size() - 1; c >= 0; --c) {
        if (ch[c] == "") ch.resize(c); else break;
    }
    std::vector<bim::DisplayColor> out_weighted_fuse_channels;
    for (int c = 0; c<ch.size(); ++c) {
        std::vector<int> cmp = ch[c].splitInt(",");
        cmp.resize(3, 0);
        out_weighted_fuse_channels.push_back(bim::DisplayColor(cmp[0], cmp[1], cmp[2]));
    }

    img = img.fuseToRGB(out_weighted_fuse_channels, fuse_method, hist);
    hist->clear(); // dima: should properly modify instead of clearing
    return img;
};

Image operation_fusemeta(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::FuseMethod fuse_method = Image::fmAverage;
    if (operations.arguments("-fusemethod").toLowerCase() == "m") fuse_method = Image::fmMax; // dima: should be moved into command args

    std::vector<bim::DisplayColor> out_weighted_fuse_channels = init_fusion_from_meta(img);
    img = img.fuseToRGB(out_weighted_fuse_channels, fuse_method, hist);
    hist->clear(); // dima: should properly modify instead of clearing
    return img;
};

Image operation_fuse(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::FuseMethod fuse_method = Image::fmAverage;
    if (operations.arguments("-fusemethod").toLowerCase() == "m") fuse_method = Image::fmMax; // dima: should be moved into command args

    std::vector< std::set<int> > out_fuse_channels;
    std::vector<xstring> fc = arguments.split(",");
    for (int i = 0; i<fc.size(); ++i) {
        std::vector<xstring> ss = fc[i].split("+");
        std::set<int> s;
        for (int p = 0; p<ss.size(); ++p)
            s.insert(ss[p].toInt() - 1);
        out_fuse_channels.push_back(s);
    }

    if (out_fuse_channels.size() > 0) {
        img = img.fuse(out_fuse_channels);
        hist->clear(); // dima: should properly modify instead of clearing
    }
    return img;
}

Image operation_fuse6(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::FuseMethod fuse_method = Image::fmAverage;
    if (operations.arguments("-fusemethod").toLowerCase() == "m") fuse_method = Image::fmMax; // dima: should be moved into command args

    std::vector<int> ch = arguments.splitInt(",");
    ch.resize(7, 0);
    std::set<int> rv, gv, bv;
    rv.insert(ch[0] - 1);
    rv.insert(ch[3] - 1);
    rv.insert(ch[4] - 1);
    rv.insert(ch[6] - 1);
    gv.insert(ch[1] - 1);
    gv.insert(ch[3] - 1);
    gv.insert(ch[5] - 1);
    gv.insert(ch[6] - 1);
    bv.insert(ch[2] - 1);
    bv.insert(ch[4] - 1);
    bv.insert(ch[5] - 1);
    bv.insert(ch[6] - 1);

    std::vector< std::set<int> > out_fuse_channels;
    out_fuse_channels.push_back(rv);
    out_fuse_channels.push_back(gv);
    out_fuse_channels.push_back(bv);

    if (out_fuse_channels.size() > 0) {
        img = img.fuse(out_fuse_channels);
        hist->clear(); // dima: should properly modify instead of clearing
    }
    return img;
}

//------------------------------------------------------------------------------------
//Append channels
//------------------------------------------------------------------------------------

void appendChannelMetadata( TagMap &md, const TagMap &i2md, int c1, int c2 ) {
  for (int i=0; i<c2; ++i) {
    xstring key1 = xstring::xprintf("channel_%d_name", c1+i);    
    xstring key2 = xstring::xprintf("channel_%d_name", i);
    if ( i2md.hasKey(key2) )
      md.set_value(key1, i2md.get_value(key2) );
  }
}


Image Image::appendChannels( const Image &i2 ) const {
  Image img;
  if (bmp==NULL) return img;

  if ( this->width()!=i2.width() || this->height()!=i2.height() || 
       this->depth()!=i2.depth() || this->pixelType()!=i2.pixelType() ) return img;
  int c = this->samples() + i2.samples();

  if ( img.alloc( bmp->i.width, bmp->i.height, c, bmp->i.depth ) == 0 ) {
    int out_sample=0;
    for (unsigned int sample=0; sample<this->samples(); ++sample ) {
      memcpy( img.bits(out_sample), this->bits(sample), this->bytesPerChan() );
      ++out_sample;
    }
    for (unsigned int sample=0; sample<i2.samples(); ++sample ) {
      memcpy( img.bits(out_sample), i2.bits(sample), i2.bytesPerChan() );
      ++out_sample;
    }
  }

  img.bmp->i = this->bmp->i;
  img.bmp->i.samples = c;
  img.metadata = this->metadata;
  appendChannelMetadata( img.metadata, i2.metadata, this->samples(), i2.samples() );
  return img;
}

//------------------------------------------------------------------------------------
//Append channels
//------------------------------------------------------------------------------------

void Image::updateGeometry(const unsigned int &z, const unsigned int &t, const unsigned int &c) {
  if (bmp==NULL) return; 
  if (z>0) {
    bmp->i.number_z = z;
    metadata.set_value( "image_num_z", z );
  }

  if (t>0) {
    bmp->i.number_t = t;
    metadata.set_value( "image_num_t", t );
  }

  if (c>1) {
      bmp->i.samples = c;
      metadata.set_value("image_num_c", c);
  }
}

void Image::updateResolution( const double r[4] ) {
  if (bmp==NULL) return; 
  bmp->i.xRes = r[0];
  bmp->i.yRes = r[1];
  bmp->i.resUnits = RES_um;

  metadata.set_value("pixel_resolution_x", r[0]);
  metadata.set_value("pixel_resolution_y", r[1]);
  metadata.set_value("pixel_resolution_z", r[2]);
  metadata.set_value("pixel_resolution_t", r[3]);
  metadata.set_value( "pixel_resolution_unit_x", "microns" );
  metadata.set_value( "pixel_resolution_unit_y", "microns" );
  metadata.set_value( "pixel_resolution_unit_z", "microns" );
  metadata.set_value( "pixel_resolution_unit_t", "seconds" );
}


//------------------------------------------------------------------------------------
// Filters
//------------------------------------------------------------------------------------

template <typename T>
void deinterlace_operation( Image *img, const Image::DeinterlaceMethod &method ) {

    bim::uint64 w = img->width();
    bim::uint64 h = img->height();
    bim::uint64 s = img->samples();
    int offset = 1;
    for (unsigned int sample=0; sample<s; ++sample ) {
        #pragma omp parallel for default(shared)
        for (bim::int64 y=0; y<(bim::int64)h-2; y+=2 ) {
            T *src1 = (T*) img->scanLine( sample, y );
            T *src2 = (T*) img->scanLine( sample, y+1 );
            T *src3 = (T*) img->scanLine( sample, y+2 );

            if (method == Image::deOdd) {
                for (unsigned int x = 0; x < w; ++x) {
                    src2[x] = src1[x];
                }
            } else if (method == Image::deEven) {
                for (unsigned int x = 0; x < w; ++x) {
                    src1[x] = src2[x];
                }
            } else if (method == Image::deAverage) {
                for (unsigned int x = 0; x < w; ++x) {
                    src2[x] = (src1[x] + src3[x]) / 2;
                }
            } else if (method == Image::deOffset) {
                for (unsigned int x = 0; x<w-offset; ++x) {
                    src1[x] = src1[x+offset];
                }
            }
        }
    } // sample
}

void Image::deinterlace( const DeinterlaceMethod &method ) {

  if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_UNSIGNED)
      deinterlace_operation<uint8>  ( this, method );
  else
  if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_UNSIGNED)
     deinterlace_operation<uint16> ( this, method );
  else      
  if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_UNSIGNED)
     deinterlace_operation<uint32> ( this, method );
  else
  if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_SIGNED)
     deinterlace_operation<int8>  ( this, method );
  else
  if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_SIGNED)
     deinterlace_operation<int16> ( this, method );
  else      
  if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_SIGNED)
     deinterlace_operation<int32> ( this, method );
  else    
  if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_FLOAT)
     deinterlace_operation<float32> ( this, method );
  else      
  if (bmp->i.depth == 64 && bmp->i.pixelType==FMT_FLOAT)
     deinterlace_operation<float64> ( this, method );
}

Image operation_deinterlace(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::DeinterlaceMethod deinterlace_method = Image::deAverage;
    if (arguments.toLowerCase() == "odd") deinterlace_method = Image::deOdd;
    if (arguments.toLowerCase() == "even") deinterlace_method = Image::deEven;
    if (arguments.toLowerCase() == "avg") deinterlace_method = Image::deAverage;
    if (arguments.toLowerCase() == "offset") deinterlace_method = Image::deOffset;

    img.deinterlace(deinterlace_method);
    return img;
};

#ifdef BIM_USE_EIGEN


template <typename T>
Image do_convolution( const Image *in, const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &filter ) {
/*
    bim::uint64 w = in->width();
    bim::uint64 h = in->height();
    bim::uint64 s = in->samples();
    Image out = in->deepCopy();

    for (unsigned int sample=0; sample<s; ++sample ) {
    #pragma omp parallel for default(shared)
        for (unsigned int y=0; y<h-2; y+=2 ) {
            T *src = (T*) img->scanLine( sample, y );

            for (unsigned int x=0; x<w; ++x) {
                src2[x] = src1[x]; 
            }

        }
    } // sample



	unsigned long x, y, xx, yy;
	long i, j;
	long height2=filter.rows()/2;
	long width2=filter.cols()/2;
	double tmp;


	writeablePixels pix_plane = WriteablePixels();
	for (x = 0; x < width; ++x) {
		for (y = 0; y < height; ++y) {
			tmp=0.0;
			for (i = -width2; i <= width2; ++i) {
				xx=x+i;
				if (xx < width && xx >= 0) {
					for(j = -height2; j <= height2; ++j) {
						yy=y+j;
						if (yy >= 0 && yy < height) {
							tmp += filter (j+height2, i+width2) * copy_pix_plane(yy,xx);
						}
					}
				}
			}
			pix_plane (y,x) = stats.add(tmp);
		}
	}

  */
  return Image();

}

Image Image::convolve( const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &filter ) const {
    if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_UNSIGNED)
        return do_convolution<uint8>  ( this, filter );
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_UNSIGNED)
        return do_convolution<uint16> ( this, filter );
    else      
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_UNSIGNED)
        return do_convolution<uint32> ( this, filter );
    else
    if (bmp->i.depth == 8 && bmp->i.pixelType==FMT_SIGNED)
        return do_convolution<int8>  ( this, filter );
    else
    if (bmp->i.depth == 16 && bmp->i.pixelType==FMT_SIGNED)
        return do_convolution<int16> ( this, filter );
    else      
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_SIGNED)
        return do_convolution<int32> ( this, filter );
    else    
    if (bmp->i.depth == 32 && bmp->i.pixelType==FMT_FLOAT)
        return do_convolution<float32> ( this, filter );
    else      
    if (bmp->i.depth == 64 && bmp->i.pixelType==FMT_FLOAT)
        return do_convolution<float64> ( this, filter );
    else
        return Image();
}

#endif //BIM_USE_EIGEN


template <typename T>
Image get_otsu( const Image *in ) {
/*

    bim::uint64 w = in->width();
    bim::uint64 h = in->height();
    bim::uint64 s = in->samples();
    Image out = in->deepCopy();

    for (unsigned int sample=0; sample<s; ++sample ) {
        for (unsigned int y=0; y<h-2; y+=2 ) {
            T *src = (T*) img->scanLine( sample, y );

            for (unsigned int x=0; x<w; ++x) {
                src2[x] = src1[x]; 
            }

        }
    } // sample

    #define OTSU_LEVELS 1024
     // binarization by Otsu's method based on maximization of inter-class variance
	double hist[OTSU_LEVELS];
	double omega[OTSU_LEVELS];
	double myu[OTSU_LEVELS];
	double max_sigma, sigma[OTSU_LEVELS]; // inter-class variance
	int i;
	int threshold;
    double min_val,max_val; // pixel range

	if (!dynamic_range) {
		histogram(hist,OTSU_LEVELS,true);
		min_val = 0.0;
		max_val = pow(2.0,bits)-1;
	} else {
		// to keep this const method from modifying the object, we use GetStats on a local Moments2 object
		Moments2 local_stats;
		GetStats (local_stats);
		min_val = local_stats.min();
		max_val = local_stats.max();
		histogram(hist,OTSU_LEVELS,false);
	}
  
	// omega & myu generation
	omega[0] = hist[0] / (width * height);
	myu[0] = 0.0;
	for (i = 1; i < OTSU_LEVELS; i++) {
		omega[i] = omega[i-1] + (hist[i] / (width * height));
		myu[i] = myu[i-1] + i*(hist[i] / (width * height));
	}
  
	// maximization of inter-class variance
	threshold = 0;
	max_sigma = 0.0;
	for (i = 0; i < OTSU_LEVELS-1; i++) {
		if (omega[i] != 0.0 && omega[i] != 1.0)
			sigma[i] = pow(myu[OTSU_LEVELS-1]*omega[i] - myu[i], 2) / 
				(omega[i]*(1.0 - omega[i]));
		else
			sigma[i] = 0.0;
		if (sigma[i] > max_sigma) {
			max_sigma = sigma[i];
			threshold = i;
		}
	}

	// threshold is a histogram index - needs to be scaled to a pixel value.
	return ( (((double)threshold / (double)(OTSU_LEVELS-1)) * (max_val - min_val)) + min_val );
*/

  return Image();
}

//------------------------------------------------------------------------------
// process based on a set of operations
// currently not 100% of operations are supported
//------------------------------------------------------------------------------

#ifdef BIM_USE_TRANSFORMS
Image operation_enhancemeta(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    //if (this->get_metadata_tag("DICOM/Rescale Type (0028,1054)", "") != "HU") continue;

    // read window center and width from the metadata
    xstring modality = img.get_metadata_tag("DICOM/Modality (0008,0060)", "");
    double wnd_center = img.get_metadata_tag_double("DICOM/Window Center (0028,1050)", 0.0);
    double wnd_width = img.get_metadata_tag_double("DICOM/Window Width (0028,1051)", 0.0);
    double slope = img.get_metadata_tag_double("DICOM/Rescale Slope (0028,1053)", 1.0);
    double intercept = img.get_metadata_tag_double("DICOM/Rescale Intercept (0028,1052)", -1024.0);
    if (modality != "CT" || wnd_width == 0) return img;

    int depth = img.depth();
    DataFormat pf = img.pixelType();
    if (depth < 16 || pf == FMT_UNSIGNED) return img;

    if (!img.transform_hounsfield_inplace(slope, intercept)) {
        img = img.transform_hounsfield(slope, intercept);
    }
    return img.enhance_hounsfield(depth, pf, wnd_center, wnd_width);
};
#endif

#ifdef BIM_USE_TRANSFORMS
Image operation_filter(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);
Image operation_transform_color(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);
Image operation_transform(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);
Image operation_hounsfield(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);
#endif
#ifdef BIM_USE_FILTERS
Image operation_superpixels(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);
#endif

std::map<std::string, ImageModifierProc> Image::create_modifiers() {
    std::map<std::string, ImageModifierProc> ops;
    ops["-stretch"] = operation_stretch;
    ops["-levels"] = operation_levels;
    ops["-brightnesscontrast"] = operation_brightnesscontrast;
    ops["-depth"] = operation_depth;
    ops["-norm"] = operation_norm;
    ops["-fusegrey"] = operation_fusegrey;
    ops["-fusergb"] = operation_fusergb;
    ops["-fusemeta"] = operation_fusemeta;
    ops["-fuse"] = operation_fuse;
    ops["-fuse6"] = operation_fuse6;
    ops["-deinterlace"] = operation_deinterlace;
    ops["-roi"] = operation_roi;
    ops["-resize"] = operation_resize;
    ops["-resample"] = operation_resample;
    ops["-rotate"] = operation_rotate;
    ops["-mirror"] = operation_mirror;
    ops["-flip"] = operation_flip;
    ops["-negative"] = operation_negative;
    ops["-threshold"] = operation_stretch;

    #ifdef BIM_USE_TRANSFORMS
    ops["-transform_color"] = operation_transform_color;
    ops["-transform"] = operation_transform;
    ops["-hounsfield"] = operation_hounsfield;
    ops["-enhancemeta"] = operation_enhancemeta;
    #endif

    #ifdef BIM_USE_FILTERS
    #ifdef BIM_USE_TRANSFORMS
    ops["-filter"] = operation_filter;
    #endif
    ops["-superpixels"] = operation_superpixels;
    #endif

    return ops;
}

void Image::process(const xoperations &operations, ImageHistogram *_hist, XConf *c) {
    ImageHistogram hist;
    if (!_hist) hist.fromImage(*this); else hist = *_hist;
    XConf cc;
    if (!c) c = &cc;

    for (xoperations::const_iterator it = operations.begin(); it != operations.end(); ++it) {
        std::string operation = it->first;
        bim::xstring arguments = it->second;
        map_modifiers::const_iterator fit = modifiers.find(operation);
        if (fit != modifiers.end()) {
            c->print(xstring::xprintf("About to run %s", operation.c_str()), 2);
            c->timerStart();
            ImageModifierProc f = (*fit).second;
            *this = (*f)(*this, arguments, operations, &hist, &cc);
            c->printElapsed(xstring::xprintf("%s ran in: ", operation.c_str()), 2);
        }
    }
}

//*****************************************************************************
//  ImageHistogram
//*****************************************************************************

ImageHistogram::ImageHistogram( int channels, int bpp, const DataFormat &fmt ) {
  if (channels > 0)
    histograms.resize( channels ); 

  if (bpp > 0)  
  for (int c=0; c<channels; ++c) {
    histograms[c].init( bpp, fmt );
  }
  this->channel_mode = Histogram::cmSeparate;
}

ImageHistogram::~ImageHistogram() {

}

// mask has to be an 8bpp image where pixels >128 belong to the object of interest
// if mask has 1 channel it's going to be used for all channels, otherwise they should match
void ImageHistogram::fromImage( const Image &img, const Image *mask ) {  
  if (mask && mask->depth()!=8) return;
  if (mask && mask->samples()>1 && mask->samples()<img.samples() ) return;
  
  histograms.resize( img.samples() ); 

  if (this->channel_mode == Histogram::cmSeparate) {
      if (mask)
        for (unsigned int c=0; c<img.samples(); ++c)
          histograms[c].newData( img.depth(), img.bits(c), img.numPixels(), img.pixelType(), (unsigned char*) mask->bits(c) );
      else
        for (unsigned int c=0; c<img.samples(); ++c)
          histograms[c].newData( img.depth(), img.bits(c), img.numPixels(), img.pixelType() );
  } else { // for combined channel histogram computation
      if (mask) {
          histograms[0].newData( img.depth(), img.bits(0), img.numPixels(), img.pixelType(), (unsigned char*) mask->bits(0) );
          for (unsigned int c=1; c<img.samples(); ++c)
              histograms[0].addData( img.bits(c), img.numPixels(), (unsigned char*) mask->bits(c) );
      } else {
          histograms[0].newData( img.depth(), img.bits(0), img.numPixels(), img.pixelType() );
          for (unsigned int c=1; c<img.samples(); ++c)
              histograms[0].addData( img.bits(c), img.numPixels() );
      }
      for (unsigned int c=1; c<img.samples(); ++c)
          histograms[c] = histograms[0];
  }
}

//------------------------------------------------------------------------------
// I/O
//------------------------------------------------------------------------------

/*
Histogram binary content:
0x00 'BIM1' - 4 bytes header
0x04 'IHS1' - 4 bytes spec
0x08 NUM    - 1xUINT32 number of histograms in the file (num channels in the image)
0xXX        - histograms
*/

const char IHistogram_mgk[4] = { 'B','I','M','1' };
const char IHistogram_spc[4] = { 'I','H','S','1' };

bool ImageHistogram::to(const std::string &fileName) {
  std::ofstream f( fileName.c_str(), std::ios_base::binary );
  return this->to(&f);
}

bool ImageHistogram::to(std::ostream *s) {
  // write header
  s->write( IHistogram_mgk, sizeof(IHistogram_mgk) );
  s->write( IHistogram_spc, sizeof(IHistogram_spc) );

  // write number of histograms
  uint32 sz = (uint32) this->histograms.size();
  s->write( (const char *) &sz, sizeof(uint32) );

  // write histograms
  for (unsigned int c=0; c<histograms.size(); ++c)
    this->histograms[c].to(s);

  s->flush();
  return true;
}

bool ImageHistogram::from(const std::string &fileName) {
  std::ifstream f( fileName.c_str(), std::ios_base::binary  );
  return this->from(&f);
}

bool ImageHistogram::from(std::istream *s) {
  // read header
  char hts_hdr[sizeof(IHistogram_mgk)];
  char hts_spc[sizeof(IHistogram_spc)];

  s->read( hts_hdr, sizeof(IHistogram_mgk) );
  if (memcmp( hts_hdr, IHistogram_mgk, sizeof(IHistogram_mgk) )!=0) return false; 

  s->read( hts_spc, sizeof(IHistogram_spc) );
  if (memcmp( hts_spc, IHistogram_spc, sizeof(IHistogram_spc) )) return false; 

  // read number of histograms
  uint32 sz;
  s->read( (char *) &sz, sizeof(uint32) );
  this->histograms.resize(sz);

  // read histograms
  for (unsigned int c=0; c<histograms.size(); ++c)
    this->histograms[c].from(s);

  return true;
}

bool ImageHistogram::toXML(const std::string &fileName) {
  std::ofstream f( fileName.c_str(), std::ios_base::binary );
  return this->toXML(&f);
}

inline void write_string(std::ostream *s, const std::string &str) {
    s->write( str.c_str(), str.size() );
}

bool ImageHistogram::toXML(std::ostream *s) {
    // write header
    write_string(s, "<resource>");

    // write histograms
    for (unsigned int c=0; c<histograms.size(); ++c) {
        write_string(s, xstring::xprintf("<histogram name=\"channel\" value=\"%d\">", c) );
        this->histograms[c].toXML(s);
        write_string(s, "</histogram>");
    }
    write_string(s, "</resource>");

    s->flush();
    return true;
}

double ImageHistogram::max_value() const {  
    double v=this->histograms[0].max_value();
    for (unsigned int c=1; c<histograms.size(); ++c)
        v = std::max(v, this->histograms[c].max_value());
    return v;
}
    
double ImageHistogram::min_value() const {
    double v=this->histograms[0].min_value();
    for (unsigned int c=1; c<histograms.size(); ++c)
        v = std::min(v, this->histograms[c].min_value());
    return v;
}

//*****************************************************************************
//  ImageLut
//*****************************************************************************

void ImageLut::init( const ImageHistogram &in, const ImageHistogram &out ) {
  if (in.size() != out.size()) return;
  luts.resize( in.size() );
  for (int i=0; i<in.size(); ++i) {
    luts[i].init( *in[i], *out[i] );
  }
}

void ImageLut::init( const ImageHistogram &in, const ImageHistogram &out, Lut::LutType type, void *args ) {
  if (in.size() != out.size()) return;
  luts.resize( in.size() );
  for (int i=0; i<in.size(); ++i) {
    luts[i].init( *in[i], *out[i], type, args );
  }
}

void ImageLut::init( const ImageHistogram &in, const ImageHistogram &out, Lut::LutGenerator custom_generator, void *args ) {
  if (in.size() != out.size()) return;
  luts.resize( in.size() );
  for (int i=0; i<in.size(); ++i) {
    luts[i].init( *in[i], *out[i], custom_generator, args );
  }
}


