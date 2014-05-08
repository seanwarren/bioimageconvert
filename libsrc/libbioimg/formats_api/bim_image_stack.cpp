/*******************************************************************************

  Defines Image Stack Class
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#include <cmath>
#include <cstring>

#include <string>
#include <sstream>
#include <map>
#include <vector>

#include "bim_image_stack.h"

#include <meta_format_manager.h>
#include <xstring.h>
#include <xtypes.h>

using namespace bim;

//------------------------------------------------------------------------------
// ImageStack
//------------------------------------------------------------------------------

ImageStack::ImageStack() {
  init();
}

ImageStack::~ImageStack() {
  free();
}

ImageStack::ImageStack( const char *fileName, unsigned int limit_width, unsigned int limit_height, int only_channel ) {
  init();
  fromFile(fileName, limit_width, limit_height, only_channel);
}

ImageStack::ImageStack( const std::string &fileName, unsigned int limit_width, unsigned int limit_height, int only_channel ) {
  init();
  fromFile(fileName, limit_width, limit_height, only_channel);
}

void ImageStack::free() {
  images.clear();
}

void ImageStack::init() {
  cur_position = 0;
  handling_image = false;
  progress_proc = NULL;
  error_proc = NULL;
  test_abort_proc = NULL;
}

ImageStack ImageStack::deepCopy() const {
  ImageStack stack = *this;
  for (unsigned int i=0; i<images.size(); ++i)
    stack.images[i] = images[i].deepCopy();
  return stack;
}

bool ImageStack::isEmpty() const { 
  if ( images.size()<=0 || handling_image ) return true;
  return imageAt(0)->isNull(); 
}

Image *ImageStack::imageAt( unsigned int position ) const {
  if ( (position >= images.size()) || handling_image ) 
    return (Image *) &empty_image;
  return (Image *) &images.at(position);
}

Image *ImageStack::image() const {
  if (handling_image) return (Image *) &empty_image;
  return imageAt( cur_position );
}
    
bool ImageStack::positionNext() {
  if (handling_image) return false;
  if (cur_position < (int)images.size()-1) {
    cur_position++;
    return true;
  }
  return false;
}

bool ImageStack::positionPrev() {
  if (handling_image) return false;
  if (cur_position > 0) {
    cur_position--;
    return true;
  }
  return false;
}

bool ImageStack::position0() {
  if (handling_image) return false;
  cur_position = 0;
  return true;
}

bool ImageStack::positionLast() {
  if (handling_image) return false;
  cur_position = (int)images.size()-1;
  return true;
}

bool ImageStack::positionSet( unsigned int l ) { 
  if (handling_image) return false;
  if (l>=images.size()) return false;
  cur_position = l; 
  return true;
}

//------------------------------------------------------------------------------

bool ImageStack::fromFile( const char *fileName, unsigned int limit_width, unsigned int limit_height, int channel ) {
  int res = 0;
  handling_image = true;
  images.clear();
  metadata.clear();
  cur_position = 0;
  MetaFormatManager fm;

  if ( (res = fm.sessionStartRead((const bim::Filename) fileName)) == 0) {

    int pages = fm.sessionGetNumberOfPages();
    for (int page=0; page<pages; ++page) {
      do_progress( page+1, pages, "Loading stack" );
      if (progress_abort()) break;

      // load page image, needs new clear image due to memory sharing
      Image img;
      if (fm.sessionReadImage( img.imageBitmap(), page ) != 0) break;

      // use channel constraint
      if ( channel>=0 )
        img.extractChannel( channel );

      // use size limits
      if ( (limit_width>0 || limit_height>0) && (limit_width<img.width() || limit_height<img.height()) )
        img = img.resample( limit_width, limit_height, Image::szBiCubic, true );
      images.push_back( img );
      
      if (page==0) {
        fm.sessionParseMetaData(0);
        metadata = fm.get_metadata();   
      }
    }
  }
  fm.sessionEnd(); 

  handling_image = false;
  return (res==0);
}

bool ImageStack::toFile( const char *fileName, const char *formatName, const char *options ) {
  MetaFormatManager fm;
  if (!fm.isFormatSupportsWMP( formatName )) return false;

  handling_image = true;
  int pages = (int)images.size();
  int res = 1;
  if ( (res = fm.sessionStartWrite((const bim::Filename) fileName, formatName, options)) == 0) {
    fm.sessionWriteSetMetadata( this->metadata );
    images[0].imageBitmap()->i.number_z = this->length();
    images[0].imageBitmap()->i.number_t = 1;
    for (int page=0; page<pages; ++page) {
      do_progress( page+1, pages, "Writing stack" );
      if (progress_abort()) break;
      fm.sessionWriteImage( images[page].imageBitmap(), page );
    }
  }
  fm.sessionEnd(); 

  handling_image = false;
  return (res==0);
}

bool ImageStack::fromFileManager( MetaFormatManager *m, const std::vector<unsigned int> &pages ) {
  int res = 0;
  handling_image = true;
  images.clear();
  metadata.clear();
  cur_position = 0;
  if (!m->sessionIsReading()) return false;
  int number_pages = m->sessionGetNumberOfPages();

  for (int i=0; i<pages.size(); ++i) {
    unsigned int page = pages[i];
    if (page>=number_pages) continue;

    #pragma omp master
    do_progress( i+1, (long) pages.size(), "Loading stack" );
    if (progress_abort()) break;

    // load page image, needs new clear image due to memory sharing
    Image img;
    if (m->sessionReadImage( img.imageBitmap(), page ) != 0) break;
    images.push_back( img );
  }

  handling_image = false;
  return (res==0);
}

double ImageStack::bytesInStack() const {
  double bytes=0;
  if (isEmpty()) return bytes;
  for (unsigned int i=0; i<images.size(); ++i)  
    bytes += images[i].bytesInImage();
  return bytes;
}

//------------------------------------------------------------------------------

void ImageStack::convertToDepth( int depth, Lut::LutType method, bool planes_independent, DataFormat pxtype ) {
  
  if (!planes_independent) {
    StackHistogram stack_histogram ( *this );
    if (pxtype==FMT_UNDEFINED) pxtype = this->pixelType();
    ImageHistogram out( this->samples(), depth, pxtype );
    ImageLut lut( stack_histogram, out, method );
    convertToDepth( lut );
  } else {
    for (unsigned int i=0; i<images.size(); ++i) {
      do_progress( i, (long) images.size(), "Converting depth" );
      images[i] = images[i].convertToDepth( depth, method, pxtype );
    }
  }
}

void ImageStack::convertToDepth( const ImageLut &lut ) {
  for (unsigned int i=0; i<images.size(); ++i) {
    do_progress( i, (long) images.size(), "Converting depth" );
    images[i] = images[i].convertToDepth( lut );
  }
}

void ImageStack::normalize( int to_bpp, bool planes_independent ) {
  this->convertToDepth( to_bpp, Lut::ltLinearDataRange, planes_independent, FMT_UNSIGNED );
}

void ImageStack::ensureTypedDepth() {
  for (unsigned int i=0; i<images.size(); ++i)
    images[i] = images[i].ensureTypedDepth();
}

//------------------------------------------------------------------------------

void ImageStack::negative() {
  for (unsigned int i=0; i<images.size(); ++i)
    images[i] = images[i].negative();
}

void ImageStack::discardLut() {
  for (unsigned int i=0; i<images.size(); ++i)
    images[i].discardLut();
}

void ImageStack::rotate( double deg ) {
  for (unsigned int i=0; i<images.size(); ++i) {
    do_progress( i, (long) images.size(), "Rotate" );
    images[i] = images[i].rotate(deg);
  }
}

void ImageStack::remapChannels( const std::vector<int> &mapping ) {
  for (unsigned int i=0; i<images.size(); ++i)
    images[i].remapChannels(mapping);
}

//------------------------------------------------------------------------------

inline double ensure_range( double a, double minv, double maxv ) {
  if (a<minv) a=minv;
  else
  if (a>maxv) a=maxv;
  return a;
}

void ImageStack::ROI( bim::uint x, bim::uint y, bim::uint z, bim::uint w, bim::uint h, bim::uint d ) {
  
  if (isEmpty()) return;
  if ( x==0 && y==0 && z==0 && width()==w && height()==h && length()==d ) return;

  if (x >= width())  x = 0;
  if (y >= height()) y = 0;
  if (z >= length()) z = 0;
  if (w<=0) w = width();
  if (h<=0) h = height();
  if (w+x > width())  w = width()-x;
  if (h+y > height()) h = height()-y;
  if (d+z > length()) d = length()-z;

  // first get rid of unused planes
  if (d+z>0 && d+z < length())
    images.erase( images.begin()+(d+z), images.end() );
  if (z > 0)
    images.erase( images.begin(), images.begin()+(z-1) );

  // then run ROI on each plane
  if ( x==0 && y==0 && width()==w && height()==h ) return;
  for (unsigned int i=0; i<images.size(); ++i) {
    do_progress( i, (long) images.size(), "ROI" );
    images[i] = images[i].ROI(x,y,w,h);
  }
}

ImageStack ImageStack::deepCopy( double x, double y, double z, double w, double h, double d ) const {
  if (isEmpty()) return ImageStack();
  if ( x==0 && y==0 && z==0 && width()==w && height()==h && length()==d ) return this->deepCopy();

  x = bim::round<int>( ensure_range( x, 0.0, width()-1.0 ) );
  w = bim::round<int>( ensure_range( w, 0.0, width()-x ) );
  y = bim::round<int>( ensure_range( y, 0.0, height()-1.0 ) );
  h = bim::round<int>( ensure_range( h, 0.0, height()-y ) );
  z = bim::round<int>( ensure_range( z, 0.0, length()-1.0 ) );
  d = bim::round<int>( ensure_range( d, 0.0, length()-z ) );

  ImageStack stack;
  for (unsigned int i=z; i<(unsigned int)(z+d); ++i)
    stack.images.push_back( images[i].ROI( (unsigned int)x, (unsigned int)y, (unsigned int)w, (unsigned int)h ) );
  stack.metadata = this->metadata;
  return stack;
}

//------------------------------------------------------------------------------

Image ImageStack::projectionXZAxis( unsigned int y ) const {
  if (y >= height()) return Image();
  
  unsigned int w = width();
  unsigned int h = length();
  unsigned int s = samples();
  unsigned int d = depth();
  Image img;
  if (img.alloc( w, h, s, d ) != 0) return img;
  
  #pragma omp parallel for default(shared)
  for (int64 z=0; z<length(); ++z) {
    for (unsigned int c=0; c<samples(); ++c) {
      unsigned char *line_in = images[z].scanLine( c, y );
      unsigned char *line_out = img.scanLine( c, z );
      memcpy( line_out, line_in, img.bytesPerLine() );
    } // for c
  } // for z 

  return img;
}

Image ImageStack::projectionYZAxis( unsigned int x ) const {
  if (x >= width()) return Image();
  
  unsigned int w = height();
  unsigned int h = length();
  unsigned int s = samples();
  unsigned int d = depth();
  Image img;
  if (img.alloc( w, h, s, d ) != 0) return img;

  #pragma omp parallel for default(shared)
  for (int64 z=0; z<length(); ++z) {
      std::vector<unsigned char> row_buf( img.bytesPerLine() );
      for (unsigned int c=0; c<samples(); ++c) {
          images[z].scanRow( c, x, &row_buf[0] );
          unsigned char *line_out = img.scanLine( c, z );
          memcpy( line_out, &row_buf[0], img.bytesPerLine() );
      } // for c
  } // for z 

  return img;
}

TagMap metadataRearrange3D( const TagMap &md, const ImageStack *stack, const ImageStack::RearrangeDimensions &operation ) {
  TagMap metadata = md;

  unsigned int sz_x = metadata.get_value_int("image_num_x", 0);
  unsigned int sz_y = metadata.get_value_int("image_num_y", 0);
  double res_x = metadata.get_value_double("pixel_resolution_x", 0);
  double res_y = metadata.get_value_double("pixel_resolution_y", 0);
  xstring units_x = metadata.get_value("pixel_resolution_unit_x");
  xstring units_y = metadata.get_value("pixel_resolution_unit_y");

  xstring image_num_z, pixel_resolution_z, pixel_resolution_unit_z;
  xstring image_num_t, pixel_resolution_t, pixel_resolution_unit_t;

  if (metadata.get_value_int("image_num_z", 1)>1) { // image is a Z stack
      image_num_z             = "image_num_z";
      pixel_resolution_z      = "pixel_resolution_z";
      pixel_resolution_unit_z = "pixel_resolution_unit_z";
      image_num_t             = "image_num_t";
      pixel_resolution_t      = "pixel_resolution_t";
      pixel_resolution_unit_t = "pixel_resolution_unit_t";
  } else if (metadata.get_value_int("image_num_t", 1)>1) { // image is a T series
      image_num_z             = "image_num_t";
      pixel_resolution_z      = "pixel_resolution_t";
      pixel_resolution_unit_z = "pixel_resolution_unit_t";
      image_num_t             = "image_num_z";
      pixel_resolution_t      = "pixel_resolution_z";
      pixel_resolution_unit_t = "pixel_resolution_unit_z";
  }

  if (image_num_z.size()<=1) return metadata;

  unsigned int sz_z = metadata.get_value_int(image_num_z, 1);
  double res_z      = metadata.get_value_double(pixel_resolution_z, 0);
  xstring units_z   = metadata.get_value(pixel_resolution_unit_z);

  if (operation == ImageStack::adXZY) { // XYZ -> XZY
      metadata.set_value("image_num_x", sz_x);
      metadata.set_value("image_num_y", sz_z);
      metadata.set_value(image_num_z, sz_y);
      metadata.set_value(image_num_t, 1);
      metadata.set_value("pixel_resolution_x", res_x);
      metadata.set_value("pixel_resolution_y", res_z);
      metadata.set_value(pixel_resolution_z, res_y);
      metadata.set_value(pixel_resolution_t, 0);
      metadata.set_value("pixel_resolution_unit_x", units_x);
      metadata.set_value("pixel_resolution_unit_y", units_z);
      metadata.set_value(pixel_resolution_unit_z, units_y);
      metadata.set_value(pixel_resolution_unit_t, "");
  } else if (operation == ImageStack::adYZX) { // XYZ -> YZX, rotated -90, need to counteract
      metadata.set_value("image_num_x", sz_z);
      metadata.set_value("image_num_y", sz_y);
      metadata.set_value(image_num_z, sz_x);
      metadata.set_value(image_num_t, 1);
      metadata.set_value("pixel_resolution_x", res_z);  // set reverse to counteract rotation
      metadata.set_value("pixel_resolution_y", res_y); // set reverse to counteract rotation
      metadata.set_value(pixel_resolution_z, res_x); // set reverse to counteract rotation
      metadata.set_value(pixel_resolution_t, 0);
      metadata.set_value("pixel_resolution_unit_x", units_z); // set reverse to counteract rotation
      metadata.set_value("pixel_resolution_unit_y", units_y); // set reverse to counteract rotation
      metadata.set_value(pixel_resolution_unit_z, units_x); // set reverse to counteract rotation
      metadata.set_value(pixel_resolution_unit_t, "");
  }

  return metadata;
}

bool ImageStack::rearrange3DToFile( const RearrangeDimensions &operation, const char *fileName, const char *formatName, const char *options ) const {
    unsigned int psize = operation == ImageStack::adXZY ? this->height(): this->width();

    MetaFormatManager fm;
    if (!fm.isFormatSupportsWMP( formatName )) return false;
    if (fm.sessionStartWrite((const bim::Filename) fileName, formatName, options) == 0) {
        
        TagMap meta = metadataRearrange3D( this->metadata, this, operation );
        fm.sessionWriteSetMetadata( meta );

        for (unsigned int p=0; p<psize; ++p) {
            Image image;
            if (operation == ImageStack::adXZY) 
                image = this->projectionXZAxis(p); // XYZ -> XZY
            else if (operation == ImageStack::adYZX) {
                image = this->projectionYZAxis(p); // XYZ -> YZX
                image = image.rotate(-90);
            }
            // this part should probably be removed in the main library
            image.imageBitmap()->i.number_pages = psize;
            image.imageBitmap()->i.number_z = meta.get_value_int("image_num_z", 1);
            image.imageBitmap()->i.number_t = meta.get_value_int("image_num_t", 1);
            fm.sessionWriteImage( image.imageBitmap(), p );
        }
        fm.sessionEnd(); 
    } else {
        fm.sessionEnd(); 
        return false;
    }
    return true;
}


TagMap resizeMetadata3d( const TagMap &md, unsigned int w_to, unsigned int h_to, unsigned int d_to, unsigned int w_in, unsigned int h_in, unsigned int d_in ) {
  TagMap metadata = md;

  metadata.set_value("image_num_z", d_to);

  if ( metadata.hasKey("pixel_resolution_x") ) {
    double new_res = metadata.get_value_double("pixel_resolution_x", 0) * ((double) w_in /(double) w_to );
    metadata.set_value("pixel_resolution_x", new_res);
  }
  if ( metadata.hasKey("pixel_resolution_y") ) {
    double new_res = metadata.get_value_double("pixel_resolution_y", 0) * ((double) h_in /(double) h_to );
    metadata.set_value("pixel_resolution_y", new_res);
  }
  if ( metadata.hasKey("pixel_resolution_z") ) {
    double new_res = metadata.get_value_double("pixel_resolution_z", 0) * ((double) d_in /(double) d_to );
    metadata.set_value("pixel_resolution_z", new_res);
  }
  return metadata;
}

void ImageStack::resize( bim::uint w, bim::uint h, bim::uint d, Image::ResizeMethod method, bool keep_aspect_ratio ) {
  uint wor = images[0].width();
  uint hor = images[0].height();
  uint dor = this->length();

  // first interpolate X,Y
  if ( (w!=0 || h!=0) && (w != width() || h != height() ) )
  for (unsigned int i=0; i<images.size(); ++i) {
    #pragma omp master
    do_progress( i, (long)images.size(), "Interpolating X/Y" );
    images[i] = images[i].resize(w, h, method, keep_aspect_ratio);
  }

  w = images[0].width();
  h = images[0].height();
  if (d==0)
    d = bim::round<unsigned int>( length() / (wor/(float)w) );

  // then interpolate Z
  if (d==0 || d==length()) return;

  // create stack with corect size 
  ImageStack stack = *this;
  stack.images.clear();
  for (int z=0; z<d; ++z)
    stack.append( images[0].deepCopy() );
  
  // now interpolate Z
  for (unsigned int y=0; y<stack.height(); ++y) {
    #pragma omp master
    do_progress( y, stack.height(), "Interpolating Z" );
    Image img_xz = projectionXZAxis( y );
    img_xz = img_xz.resize(w, d, method, false );
    
    #pragma omp parallel for default(shared)
    for (int z=0; z<d; ++z) {
      for (unsigned int c=0; c<stack.samples(); ++c) {
        unsigned char *line_out = stack.images[z].scanLine( c, y );
        unsigned char *line_in = img_xz.scanLine( c, z );
        memcpy( line_out, line_in, img_xz.bytesPerLine() );
      } // for c
    } // for z
  } // for y

  stack.metadata = resizeMetadata3d( this->metadata, w, h, d, wor, hor, dor );
  *this = stack;
}

void ImageStack::setNumberPlanes( unsigned int n ) {
  if (n == images.size()) return;
  if (n < images.size()) { images.resize( n ); return; }

  int d = n - (int)images.size();
  Image l = images[ images.size()-1 ];

  for (int z=0; z<d; ++z)
    this->append( l.deepCopy() );
}

Image ImageStack::pixelArithmeticMax() const {
  Image p = images[0].deepCopy();
  for (int z=1; z<images.size(); ++z)
    p.pixelArithmeticMax( images[z] );
  return p;
}

Image ImageStack::pixelArithmeticMin() const {
  Image p = images[0].deepCopy();
  for (int z=1; z<images.size(); ++z)
    p.pixelArithmeticMin( images[z] );
  return p;
}

Image ImageStack::textureAtlas() const {
    uint64 w = images[0].width();
    uint64 h = images[0].height();
    uint64 n = images.size();

    // start with atlas composed of a row of images
    uint64 ww = w*n, hh = h;
    double ratio = ww / (double)hh;
    // optimize side to be as close to ratio of 1.0
    for (int r = 2; r < images.size(); r++) {
        double ipr = ceil(n / (double)r);
        uint64 aw = w*ipr;
        uint64 ah = h*r;
        double rr = bim::max<double>(aw, ah) / bim::min<double>(aw, ah);
        if (rr < ratio) {
            ratio = rr;
            ww = aw;
            hh = ah;
        } else
            break;
    }

    // compose atlas image
    uint64 rows = ww / w;
    uint64 cols = hh / h;
    Image atlas(ww, hh, images[0].depth(), images[0].samples(), images[0].pixelType());
    atlas.fill(0);
    uint64 i = 0;
    uint64 y = 0;
    for (int r = 0; r < cols; ++r) {
        uint64 x = 0;
        for (int c = 0; c < rows; ++c) {
            if (i < images.size())
                atlas.setROI(x, y, images[i]);
            x += w;
            i++;
        }
        y += h;
    }
    return atlas;
}

//------------------------------------------------------------------------------
// StackHistogram
//------------------------------------------------------------------------------

StackHistogram::StackHistogram() {

}

StackHistogram::~StackHistogram() {

}

void StackHistogram::fromImageStack( const ImageStack &stack ) {  
  histograms.clear(); 
  histograms.resize( stack.samples() ); 

  // iterate first to get stats - slower but will not fail on float data
  for (unsigned int c=0; c<stack.samples(); ++c) {
    histograms[c].init( stack.depth(), stack.pixelType() );
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].updateStats( stack[i]->bits(c), stack[i]->numPixels() );
  } // channels  

  // iterate first to create histograms
  for (unsigned int c=0; c<stack.samples(); ++c) {
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].addData( stack[i]->bits(c), stack[i]->numPixels()  );
  } // channels
}

void StackHistogram::fromImageStack( const ImageStack &stack, const Image *mask ) {  
  if (mask && mask->depth()!=8) return;
  if (mask && mask->samples()>1 && mask->samples()<stack.samples() ) return;
  histograms.clear(); 
  histograms.resize( stack.samples() ); 

  // iterate first to get stats - slower but will not fail on float data
  for (unsigned int c=0; c<stack.samples(); ++c) {
    histograms[c].init( stack.depth(), stack.pixelType() );
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].updateStats( stack[i]->bits(c), stack[i]->numPixels(), (unsigned char *) mask->bits(c) );
  } // channels  

  // iterate first to create histograms
  for (unsigned int c=0; c<stack.samples(); ++c) {
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].addData( stack[i]->bits(c), stack[i]->numPixels(), (unsigned char *) mask->bits(c) );
  } // channels
}

void StackHistogram::fromImageStack( const ImageStack &stack, const ImageStack *mask ) {  
  if (mask && mask->depth()!=8) return;
  if (mask && mask->samples()>1 && mask->samples()<stack.samples() ) return;
  if (mask && mask->length()<stack.length() ) return;
  histograms.clear(); 
  histograms.resize( stack.samples() ); 

  // iterate first to get stats - slower but will not fail on float data
  for (unsigned int c=0; c<stack.samples(); ++c) {
    histograms[c].init( stack.depth(), stack.pixelType() );
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].updateStats( stack[i]->bits(c), stack[i]->numPixels(), (unsigned char *) mask->imageAt(i)->bits(c) );
  } // channels  

  // iterate first to create histograms
  for (unsigned int c=0; c<stack.samples(); ++c) {
    for (unsigned int i=0; i<stack.length(); ++i)
      histograms[c].addData( stack[i]->bits(c), stack[i]->numPixels(), (unsigned char *) mask->imageAt(i)->bits(c) );
  } // channels
}
