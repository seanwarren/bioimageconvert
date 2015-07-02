/*******************************************************************************

  Defines Image Stack Class
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_IMAGE_STACK_H
#define BIM_IMAGE_STACK_H

#include <vector>
#include <map>

#include "tag_map.h"
#include "bim_image.h"
#include "bim_histogram.h"

namespace bim {

class MetaFormatManager;
class xstring;

//------------------------------------------------------------------------------
// ImageStack
//------------------------------------------------------------------------------

class ImageStack {
  public:
    enum RearrangeDimensions { 
      adNone=0, 
      adXZY=1, 
      adYZX=2, 
    };

    ImageStack();
    ImageStack( const char *fileName, unsigned int limit_width=0, unsigned int limit_height=0, int only_channel=-1 );
    ImageStack( const std::string &fileName, unsigned int limit_width=0, unsigned int limit_height=0, int only_channel=-1 );
    ImageStack(const std::vector<xstring> &files, unsigned int number_channels = 0, const xoperations *ops = 0);
    ~ImageStack();

    void        free();
    void        clear() { free(); }
    ImageStack deepCopy() const;  
    ImageStack deepCopy( double x, double y, double z, double w=0, double h=0, double d=0 ) const;

    bool       isReady() const { return !handling_image; } // true if images can be used
    bool       isEmpty() const;

    int        size() const { return (int) images.size(); }
    int        count() const { return size(); }
    int        numberImages() const { return size(); }
    int        numberPlanes() const { return size(); }
    bool       planeInRange( int p ) const { if (p<0) return false; if (p>=size()) return false; return true; }
    void       setNumberPlanes( unsigned int );

    double     bytesInStack() const;

    inline uint64    width()      const { return size()<=0 ? 0 : images[0].width();  }
    inline uint64    height()     const { return size()<=0 ? 0 : images[0].height(); }
    inline uint64    length()     const { return size(); }
    inline uint64    depth()      const { return (bim::uint64) size()<=0 ? 0 : images[0].depth(); }
    inline uint64    samples()    const { return (bim::uint64) size()<=0 ? 0 : images[0].samples(); }
    inline DataFormat pixelType() const { return size()<=0 ? FMT_UNSIGNED : images[0].pixelType(); }

    Image *imageAt( unsigned int position ) const;
    Image *image() const; // image at the currently selected position
    inline Image *operator[](unsigned int p) const { return imageAt(p); }

    void append( const Image &img ) { images.push_back(img); }

    // return true if the level exists
    bool positionPrev();
    bool positionNext();
    bool position0();
    bool positionFirst() { return position0(); }
    bool positionLast();
    bool positionSet( unsigned int l );
    int  positionCurrent() const { return cur_position; }

    const std::map<std::string, std::string> &get_metadata() const { return metadata; }
    std::string get_metadata_tag( const std::string &key, const std::string &def ) const { return metadata.get_value( key, def ); }
    int         get_metadata_tag_int( const std::string &key, const int &def ) const { return metadata.get_value_int( key, def ); }
    double      get_metadata_tag_double( const std::string &key, const double &def ) const { return metadata.get_value_double( key, def ); }

    // I/O

    virtual bool fromFile(const char *fileName, unsigned int limit_width = 0, unsigned int limit_height = 0, int channel = -1, const xoperations *ops = 0);
    bool fromFile(const std::string &fileName, unsigned int limit_width = 0, unsigned int limit_height = 0, int channel = -1, const xoperations *ops = 0) {
        return fromFile(fileName.c_str(), limit_width, limit_height, channel, ops);
    }

    // use number_channels>0 if channels are stored as separate files
    bool fromFileList(const std::vector<xstring> &files, unsigned int number_channels = 0, const xoperations *ops = 0);

    bool fromFileManager( MetaFormatManager *m, const std::vector<unsigned int> &pages );

    virtual bool toFile( const char *fileName, const char *formatName, const char *options=NULL );
    bool toFile( const std::string &fileName, const std::string &formatName, const std::string &options = "" ) {
      return toFile( fileName.c_str(), formatName.c_str(), options.c_str() ); }

    // Operations
    void convertToDepth( int depth, Lut::LutType method = Lut::ltLinearFullRange, bool planes_independent = false, DataFormat pxtype = FMT_UNDEFINED );
    void convertToDepth( const ImageLut & );
    void normalize( int to_bpp = 8, bool planes_independent = false );
    void ensureTypedDepth();

    void resize( uint w, uint h=0, uint d=0, Image::ResizeMethod method = Image::szNearestNeighbor, bool keep_aspect_ratio = false );

    void ROI( uint x, uint y, uint z, uint w, uint h, uint d );

    //--------------------------------------------------------------------------    
    // process all images based on command line arguments or a string
    //--------------------------------------------------------------------------

    void process(const xoperations &operations, ImageHistogram *hist = 0, XConf *c = 0);

    // only available values now are +90, -90 and 180
    void rotate( double deg );

    void negative();
    void discardLut();
    void remapChannels( const std::vector<int> &mapping );

    Image projectionXZAxis( unsigned int y ) const;
    Image projectionYZAxis( unsigned int x ) const;
    
    bool rearrange3DToFile( const RearrangeDimensions &operation, const char *fileName, const char *formatName, const char *options=NULL ) const;
    bool rearrange3DToFile( const RearrangeDimensions &operation, const std::string &fileName, const std::string &formatName, const std::string &options = "" ) const {
      return this->rearrange3DToFile( operation, fileName.c_str(), formatName.c_str(), options.c_str() ); 
    }

    Image pixelArithmeticMax() const;
    Image pixelArithmeticMin() const;

    Image textureAtlas(int rows=0, int cols=0) const;
    Image textureAtlas(const xstring &arguments) const;

  protected:
    std::vector<Image> images;
    TagMap metadata;

    Image empty_image;
    int  cur_position;
    bool handling_image;

  protected:
    void init();

  // callbacks
  public:
    ProgressProc progress_proc;
    ErrorProc error_proc;
    TestAbortProc test_abort_proc;

  protected:
    void do_progress( bim::uint64 done, bim::uint64 total, char *descr ) {
      if (progress_proc) progress_proc( done, total, descr );
    }
    bool progress_abort( ) {
      if (!test_abort_proc) return false;
      return (test_abort_proc() != 0);
    }

};


//------------------------------------------------------------------------------
// StackHistogram
//------------------------------------------------------------------------------

class StackHistogram: public ImageHistogram {
public:
  StackHistogram( );
  StackHistogram( const ImageStack &stack ) { fromImageStack(stack); }
  StackHistogram( const ImageStack &stack, const Image *mask ) { fromImageStack(stack, mask); }
  StackHistogram( const ImageStack &stack, const ImageStack *mask ) { fromImageStack(stack, mask); }
  ~StackHistogram();

  void fromImageStack( const ImageStack &stack );

  // mask has to be an 8bpp image where pixels >128 belong to the object of interest
  // if mask has 1 channel it's going to be used for all channels, otherwise they should match
  void fromImageStack( const ImageStack &, const Image *mask );

  // mask has to be an 8bpp image where pixels >128 belong to the object of interest
  // if mask has 1 channel it's going to be used for all channels, otherwise they should match
  void fromImageStack( const ImageStack &, const ImageStack *mask );
};

} // namespace bim

#endif //BIM_IMAGE_STACK_H


