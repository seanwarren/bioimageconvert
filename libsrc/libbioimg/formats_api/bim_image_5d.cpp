/*******************************************************************************

  5D image - defines a 5D image navigable by Z and T coordinates
  
  image is constructed as a list of T points each represented by an image stack 
  each Z location in a stack is represented by an image where pixels can be 
  accessed per channel

  image also contains all associated metadata as tags

  time points are cached in-memory by the last access time (based on allowed cache size)
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
     - First creation
      
  ver: 1
        
*******************************************************************************/

#include <cmath>

#include <xstring.h>
#include <xtypes.h>

#include "bim_image_5d.h"

using namespace bim;

bim::Image5D::Image5D(): number_axis(5) {
  maximum_cache_size = 200 * 1024 * 1024; // 200MB
  init();
}

bim::Image5D::~Image5D() {
  //free();
}

void bim::Image5D::init() {
  progress_proc = NULL;
  error_proc = NULL;
  test_abort_proc = NULL;
  metadata.clear();
  stacks.clear();
  cache_priority.clear();
  image_size.resize(0);
  image_size.resize(bim::Image5D::number_axis, 0);
  current_position.resize(0);
  current_position.resize(bim::Image5D::number_axis, 0);
  loop_time = true;
}

void bim::Image5D::clear() {
  init();
  fm.sessionEnd();
}

bool bim::Image5D::isLoaded() const {
  return fm.sessionIsReading();
}

bool bim::Image5D::isLoaded(const std::string &fileName) const {
  return fm.sessionIsReading(fileName);
}

void bim::Image5D::initFromMeta() {
  image_size[bim::Image5D::x] = metadata.get_value_double( "image_num_x", 0 );
  image_size[bim::Image5D::y] = metadata.get_value_double( "image_num_y", 0 );
  image_size[bim::Image5D::z] = metadata.get_value_double( "image_num_z", 1 );
  image_size[bim::Image5D::t] = metadata.get_value_double( "image_num_t", 1 );
  image_size[bim::Image5D::c] = metadata.get_value_double( "image_num_c", 1 );
  // if the image does not have any metadata, but has several pages, consider as a 3D stack
  int p = metadata.get_value_int( "image_num_p", 0 );
  if (image_size[bim::Image5D::t]<=1 && image_size[bim::Image5D::z]<=1 && p>1)
    image_size[bim::Image5D::z] = p;

}

bool bim::Image5D::fromFile( const std::string &fileName ) {
  if (!fm.sessionIsReading(fileName)) {
    init();
    if (fm.sessionStartRead((const bim::Filename) fileName.c_str())!=0) return false;

    // read metadata and init all needed variables
    
    // one image might have to be red here for proper metadata processing
    Image img;
    if (fm.sessionReadImage( img.imageBitmap(), 0 ) != 0) return false;

    fm.sessionParseMetaData(0);
    metadata = fm.get_metadata(); 
    initFromMeta();

    // populate the stacks vector with empty stacks
    stacks.resize(image_size[bim::Image5D::t]);
    for (unsigned int i=0; i<stacks.size(); ++i) stacks[i] = ImageStack();
    
  }

  // the images are not gonna be red here, we'll read them on request
  return true;
}

bool bim::Image5D::isCached() const {
  return !stacks[current_position[bim::Image5D::t]].isEmpty();
}

bool bim::Image5D::isCached( unsigned int t ) const {
  if (t>=image_size[bim::Image5D::t]) return false;
  if (t>=stacks.size()) return false;
  return !stacks[t].isEmpty();
}

Image *bim::Image5D::imageAt( unsigned int t, unsigned int z ) {
  if (t>=image_size[bim::Image5D::t]) return 0;
  ImageStack *s = this->stackAt(t);
  if (!s) return 0;
  return s->imageAt(z);
}

ImageStack *bim::Image5D::stackAt( unsigned int t ) {
  if (t>=image_size[bim::Image5D::t]) return 0;
  if (t>=stacks.size()) return 0;
  ImageStack *s = &stacks[t];
  
  // move to the back of the cache_priority
  cache_priority.remove(s);
  cache_priority.push_back(s);

  // if stack was discarded by caching, reload
  if (s->isEmpty()) {
    s->progress_proc = this->progress_proc;
    s->test_abort_proc = this->test_abort_proc;
    if (!s->fromFileManager(&fm, this->pagesOf(t)) ) return 0;
    updateCache();
  }
  return s;
}

std::vector<unsigned int> bim::Image5D::pagesOf(unsigned int t) const {
  // get numbers of pages belonging to the Z images in time T
  std::vector<unsigned int> pages;
  if (t>=image_size[bim::Image5D::t]) return pages;
  if (image_size[bim::Image5D::z]<=0) return pages;
  pages.resize(image_size[bim::Image5D::z], 0);
  for (unsigned int z=0; z<pages.size(); ++z) 
    pages[z] = ( image_size[bim::Image5D::z]*t ) + z;
  return pages;
}

double bim::Image5D::memorySize() const {
  double mem=0;
  for (unsigned int i=0; i<stacks.size(); ++i)
    mem += stacks[i].bytesInStack();
  return mem;
}

void bim::Image5D::updateCache() {
  if (memorySize()<=maximum_cache_size) return;
  while (memorySize()>maximum_cache_size && cache_priority.size()>1) {
    ImageStack *s = cache_priority.front();
    s->clear();
    cache_priority.pop_front();
  }
}

ImageStack *bim::Image5D::stack() {
  return this->stackAt( this->current_position[bim::Image5D::t] );
}

const ImageStack *bim::Image5D::current() const {
  return &stacks[this->current_position[bim::Image5D::t]];
}

Image *bim::Image5D::image() {
  return stack()->imageAt( this->current_position[bim::Image5D::z] );
}

void bim::Image5D::resetPosition() {
  this->current_position[bim::Image5D::t]=0;
  this->current_position[bim::Image5D::z]=0;
}

bool bim::Image5D::nextTimePoint() {
  return setTimePoint( ((int)this->current_position[bim::Image5D::t])+1 );
}

bool bim::Image5D::prevTimePoint() {
  return setTimePoint( ((int)this->current_position[bim::Image5D::t])-1 );
}

bool bim::Image5D::setTimePoint( const int &t_ ) {
  int tt=t_;
  if (loop_time && tt>=(int)this->image_size[bim::Image5D::t]) tt=0;
  if (loop_time && tt<0) tt=this->image_size[bim::Image5D::t]-1;
  if (this->current_position[bim::Image5D::t]>=0 && tt<this->image_size[bim::Image5D::t]) {
    this->current_position[bim::Image5D::t] = tt;
    return true;
  }
  return false;
}

bool bim::Image5D::nextZPosition() {
  return setZPosition( this->current_position[bim::Image5D::z]+1 );
}

bool bim::Image5D::prevZPosition() {
  return setZPosition( this->current_position[bim::Image5D::z]-1 );
}

bool bim::Image5D::setZPosition( const unsigned int &z ) {
  if (this->current_position[bim::Image5D::z]>0 && z<this->image_size[bim::Image5D::z]) {
    this->current_position[bim::Image5D::z] = z;
    return true;
  }
  return false;
}
