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

#ifndef BIM_IMAGE_5D_H
#define BIM_IMAGE_5D_H

#include <vector>
#include <list>

#include <meta_format_manager.h>
#include "bim_image.h"
#include "bim_image_stack.h"


namespace bim {

class Image5D {
  public:
    const int number_axis;
    enum Axis { x=0, y=1, z=2, t=3, c=4 };

  public:
    Image5D();
    ~Image5D();

    void init();
    void clear();

    bool isLoaded() const;
    bool isLoaded(const std::string &fileName) const;
    bool isCached() const;
    bool isCached( unsigned int t ) const;
    Image   *imageAt( unsigned int t, unsigned int z );
    ImageStack *stackAt( unsigned int t );
    //inline Image *operator[](unsigned int p) const { return imageAt(p); }

    ImageStack *stack(); // stack at the currently selected position
    const ImageStack *current() const; // difference with stack() is that current() is not going to load the image
    Image *image(); // image at the currently selected position
    void resetPosition();
    bool nextTimePoint();
    bool prevTimePoint();
    bool setTimePoint( const int &t );
    bool nextZPosition();
    bool prevZPosition();
    bool setZPosition( const unsigned int &z );

    unsigned int currentT() const { return current_position[Image5D::t]; }
    unsigned int currentZ() const { return current_position[Image5D::z]; }
    unsigned int numberT() const { return image_size[Image5D::t]; }
    unsigned int numberZ() const { return image_size[Image5D::z]; }

    bool getTimeLooping() const { return loop_time; }
    void setTimeLooping( bool v ) { loop_time=v; }

    bool fromFile( const char *fileName ) { return fromFile(fileName); }
    bool fromFile( const std::string &fileName );

    double getMaximumCacheSize() const { return maximum_cache_size; }
    void   setMaximumCacheSize( const double &bytes ) { maximum_cache_size = bytes; }

    const TagMap* getMetadata() const { return &metadata; }
    std::string fileName() const { return fm.sessionFilename(); }

  protected:
    // prohibit copy-constructor
    Image5D( const Image5D& ): number_axis(5) {}

  protected:
    std::vector <unsigned int> image_size;
    std::vector <unsigned int> current_position;
    double maximum_cache_size;
    bool loop_time;

    std::vector<ImageStack> stacks;
    std::list<ImageStack*>  cache_priority; // images in the front are the most likely to be removed
    TagMap metadata;

  private:
    MetaFormatManager fm;
    void initFromMeta();
    void updateCache();
    std::vector<unsigned int> pagesOf(unsigned int t) const;
    double memorySize() const;

  // callbacks
  public:
    ProgressProc progress_proc;
    ErrorProc error_proc;
    TestAbortProc test_abort_proc;

  protected:
    void do_progress( long done, long total, char *descr ) {
      if (progress_proc) progress_proc( done, total, descr );
    }
    bool progress_abort( ) {
      if (!test_abort_proc) return false;
      return (test_abort_proc() != 0);
    }
};

} // namespace bim

#endif // BIM_IMAGE_5D_H


