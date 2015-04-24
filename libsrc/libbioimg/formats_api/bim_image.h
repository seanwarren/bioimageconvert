/*******************************************************************************

  Defines Image Class, it uses smart pointer technology to implement memory
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

  TODO:
    Add metadata structure and destroy it in the end

  History:
    03/23/2004 18:03 - First creation
      
  ver: 12
        
*******************************************************************************/

#ifndef BIM_IMAGE_H
#define BIM_IMAGE_H

#include "bim_img_format_interface.h"
#include "bim_img_format_utils.h"
#include "bim_buffer.h"
#include "bim_histogram.h"

#include "tag_map.h"
#include "xconf.h"

#define BIM_USE_IMAGEMANAGER

#ifdef BIM_USE_QT
#include <QImage>
#include <QPixmap>
#include <QPainter>
#endif //BIM_USE_QT

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#endif //WINDOWS

#ifdef BIM_USE_ITK
#include <itkImage.h>
#endif //BIM_USE_ITK

#ifdef BIM_USE_EIGEN
#include <Eigen/Dense>
#endif //BIM_USE_EIGEN

#ifdef BIM_USE_OPENCV
#include <opencv2/core/core.hpp>
#endif //BIM_USE_OPENCV

#ifdef BIM_USE_NUMPY
#include <ndarray.h>
#endif //BIM_USE_NUMPY

#include <cmath>
#include <vector>
#include <set>
#include <limits>
#include <map>

namespace bim {

class ImageHistogram;
class ImageLut;
class Image;

//------------------------------------------------------------------------------------
// aux
//------------------------------------------------------------------------------------

typedef Image(*ImageModifierProc) (Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c);

//------------------------------------------------------------------------------
// Image
//------------------------------------------------------------------------------

class ImgRefs {
public:
  ImgRefs() { refs = 0; }
  ~ImgRefs() { }

  ImgRefs(const ImgRefs &ir) {
    this->refs = ir.refs; 
    this->bmp  = ir.bmp;
  }

public:
  ImageBitmap bmp;
  unsigned int    refs;
};

class Image {
  public:
    enum ResizeMethod { 
      szNearestNeighbor=0, 
      szBiLinear=1, 
      szBiCubic=2
    };

    enum DeinterlaceMethod { 
      deOdd=0, 
      deEven=1, 
      deAverage=2,
      deOffset=3
    };

    enum FuseMethod { 
      fmAverage=0,  // combine pixels using mean operator, where Pbase = mean(Pbase, Px)
      fmMax=1,      // combine pixels using max operator, where Pbase = max(Pbase, Px)
      fmMin=2,      // combine pixels using min operator, where Pbase = min(Pbase, Px)
      fmReplace=3,  // replace pixels in the base image, where Pbase = Px
      fmBlend=4,    // using mask weights blend with base image, where Pbase = Pbase*Wbase + Px*Wpx
      fmAdd=5,      // combine pixels using add operator, where Pbase = Pbase + Px
      fmSubtract=6, // combine pixels using subtract operator, where Pbase = Pbase - Px
      fmMult=7,     // combine pixels using multiplication operator, where Pbase = Pbase * Px
      fmDiv=8       // combine pixels using division operator, where Pbase = Pbase / Px
    };

    enum ArithmeticOperators {
        aoAdd = 0,
        aoSub = 1,
        aoMul = 2,
        aoDiv = 3,
        aoMax = 4,
        aoMin = 5
    };

    enum LineOps { 
      loMax=0, 
      loMin=1, 
      loAvg=2
    };

    enum ThresholdTypes { 
      ttNone =0,
      ttBoth =1,  // sets pixels below the threshold to lowest possible value and above or equal to highest
      ttLower=2, // sets pixels below the threshold to lowest possible value 
      ttUpper=3  // sets pixels above or equal to the threshold to highest possible value
    };

  public:
    Image();
    ~Image();

    Image(uint64 width, uint64 height, uint64 depth, uint64 samples, DataFormat format=FMT_UNSIGNED);
    #ifdef BIM_USE_IMAGEMANAGER
    Image(const char *fileName, int page=0);
    Image(const std::string &fileName, int page=0);
    #endif //BIM_USE_IMAGEMANAGER

    // special function to create image class from an existing bitmap without managing its memory
    // it will not delete the bitmap when destroyed
    Image(ImageBitmap *b) { connectToUnmanagedMemory(b); }

    #ifdef BIM_USE_QT
    Image(const QImage &qimg);
    #endif //BIM_USE_QT


    // allow copy constructor, it will only point to the same memory area
    Image(const Image& );
    
    // it will only point to the same memory area
    Image &operator=(const Image & );

    // will create a new memory area and copy the image there
    Image  deepCopy() const;

    // will create a new memory area
    int        alloc( uint64 w, uint64 h, uint64 samples, uint64 depth, DataFormat format=FMT_UNSIGNED );
    // will connect image to the new empty memory area
    void       free( );

    bool       create ( uint64 width, uint64 height, uint64 depth, uint64 samples, DataFormat format=FMT_UNSIGNED )
    { return this->alloc( width, height, samples, depth, format ) ? true : false; }
    void       reset () { this->free(); }
    void       clear () { this->free(); }
    
    uchar *sampleBits( uint64 sample ) const;
    uchar *scanLine( uint64 sample, uint64 y ) const;
    uchar *pixelBits( uint64 sample, uint64 x, uint64 y ) const;
    void   scanRow( uint64 sample, uint64 x, uchar *buf ) const;

    template <typename T>
    T pixel( uint64 sample, uint64 x, uint64 y ) const;

    // return a pointer to the buffer of line y formed in iterleaved format xRGB
    // the image must be in 8 bpp, otherwise NULL is returned
    uchar *scanLineRGB( uint64 y );

    bool       isNull() const { if (bmp!=NULL) return bmp->bits[0] == NULL; else return false; }
    bool       isEmpty() const { return isNull(); }
    void      *bits(const unsigned int &sample) const;
    
    void       fill( double );

    inline uint64     bytesPerChan( ) const;
    inline uint64     bytesPerLine( ) const;
    inline uint64     bytesPerRow( ) const;
    inline uint64     bytesInPixels( int n ) const;
    inline uint64     bytesInImage( ) const { return bytesPerChan()*samples(); }
    inline uint64     availableColors( ) const;
    inline uint64     width()     const { return bmp==NULL ? 0 : bmp->i.width;  }
    inline uint64     height()    const { return bmp==NULL ? 0 : bmp->i.height; }
    inline uint64     numPixels() const { return bmp==NULL ? 0 : bmp->i.width * bmp->i.height; }
    inline uint64     depth()     const { return bmp==NULL ? 0 : bmp->i.depth;  }
    inline uint64     samples()   const { return bmp==NULL ? 0 : bmp->i.samples; }
    inline uint64     channels()  const { return samples(); }
    inline DataFormat pixelType() const { return bmp==NULL ? FMT_UNSIGNED : bmp->i.pixelType; }
    inline uint64     numT()      const { return bmp==NULL ? 0 : bmp->i.number_t; }
    inline uint64     numZ()      const { return bmp==NULL ? 0 : bmp->i.number_z; }

    void updateGeometry(const unsigned int &z = 0, const unsigned int &t = 0, const unsigned int &c = 0);
    void updateResolution( const double r[4] );
    void updateResolution( const std::vector< double > &r ) { updateResolution( &r[0] ); } 

    ImageBitmap *imageBitmap() { return bmp; }

    std::string getTextInfo() const;

    //--------------------------------------------------------------------------    
    // LUT - palette
    //--------------------------------------------------------------------------
    bool   hasLut() const;
    int    lutSize() const;
    RGBA*  palette() const { return bmp->i.lut.rgba; }
    RGBA   lutColor( uint64 i ) const;
    void   setLutColor( uint64 i, RGBA c );
    void   setLutNumColors( uint64 n );
    void   discardLut() { setLutNumColors(0); }

    //--------------------------------------------------------------------------    
    // OS/Lib dependent stuff
    //--------------------------------------------------------------------------

    #ifdef BIM_USE_QT
    QImage  toQImage  ( ) const;
    QPixmap toQPixmap ( ) const;
    void fromQImage( const QImage &qimg );
    // paint( QPaintDevice, ROI ) const;
    void paint( QPainter *p ) const;
    #endif //BIM_USE_QT

    #ifdef WIN32
    HBITMAP toWinHBITMAP ( ) const;
    // paint( HWINDOW, ROI );
    #endif //WIN32

    #ifdef BIM_USE_ITK
    template <typename PixelType>
    itk::Image< PixelType, 2 >::Pointer Image::toItkImage( const unsigned int &channel ) const;

    template <typename PixelType>
    void Image::fromItkImage( const itk::Image< PixelType, 2 > *image );
    #endif //BIM_USE_ITK

    #ifdef BIM_USE_EIGEN
    // c++0x version - much cleaner using default template arguments
    //template <typename PixelType, class MatrixType = Eigen::Matrix<PixelType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> > 
    //MatrixType toEigenMatrix(const unsigned int &channel) const; 

    template <typename PixelType> 
    Eigen::Matrix<PixelType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> toEigenMatrix(const unsigned int &channel) const; 

    template <typename PixelType>
    void fromEigenMatrix( const Eigen::Matrix<PixelType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> &m );
    #endif //BIM_USE_EIGEN

    #ifdef BIM_USE_OPENCV
    cv::Mat asCVMat(const unsigned int &channel) const; // shallow memory copy
    cv::Mat toCVMat(const unsigned int &channel) const; // deep memory copy
    void fromCVMat( const cv::Mat &m ); // deep memory copy
    #endif //BIM_USE_OPENCV


    #ifdef BIM_USE_NUMPY
    //template <typename PixelType> 
    //Ndarray<PixelType, ????> toNdarray() const; // dima: incomplete

    //template <typename PixelType>
    //void fromNdarray( const Ndarray<PixelType, ?????> &m ); // dima: incomplete
    #endif //BIM_USE_NUMPY

    #ifdef BIM_USE_IMAGEMANAGER
    bool fromFile( const char *fileName, int page=0 );
    bool fromFile( const std::string &fileName, int page=0 ) { 
      return fromFile( fileName.c_str(), page ); }
    bool fromPyramidFile(const std::string &fileName, int page=0, int level = 0, int tilex = -1, int tiley = -1, int tilesize = 0);

    bool toFile( const char *fileName, const char *formatName, const char *options=NULL );
    bool toFile( const std::string &fileName, const std::string &formatName ) {
      return toFile( fileName.c_str(), formatName.c_str() ); }
    bool toFile( const std::string &fileName, const std::string &formatName, const std::string &options ) {
      return toFile( fileName.c_str(), formatName.c_str(), options.c_str() ); }
    #endif //BIM_USE_IMAGEMANAGER

    //--------------------------------------------------------------------------    
    // Metadata
    //--------------------------------------------------------------------------

    TagMap      get_metadata() const { return metadata; }
    std::string get_metadata_tag( const std::string &key, const std::string &def ) const { return metadata.get_value( key, def ); }
    int         get_metadata_tag_int( const std::string &key, const int &def ) const { return metadata.get_value_int( key, def ); }
    double      get_metadata_tag_double( const std::string &key, const double &def ) const { return metadata.get_value_double( key, def ); }

    void        delete_metadata_tag(const std::string &key) { metadata.delete_tag(key); }

    void        set_metadata( const TagMap &md ) { metadata = md; }

    //--------------------------------------------------------------------------    
    // process an image based on command line arguments or a string
    //--------------------------------------------------------------------------

    void process(const xoperations &operations, ImageHistogram *hist=0, XConf *c=0);

    //--------------------------------------------------------------------------    
    // some operations
    //--------------------------------------------------------------------------
    
    Image convertToDepth( int depth, Lut::LutType method=Lut::ltLinearFullRange, DataFormat pxtype=FMT_UNDEFINED, 
                          Histogram::ChannelMode mode=Histogram::cmSeparate, ImageHistogram *hist=0, void *args=NULL ) const;
    Image convertToDepth( const ImageLut & ) const;
    Image normalize( int to_bpp = 8, ImageHistogram *hist=0 ) const;
    bool  isUnTypedDepth() const;    
    Image ensureTypedDepth() const;

    Image ROI(bim::uint64 x, bim::uint64 y, bim::uint64 w, bim::uint64 h) const;

    void  setROI(bim::uint64 x, bim::uint64 y, const Image &roi, const Image &mask = Image(), FuseMethod method = fmReplace);
    void  setROI(bim::uint64 x, bim::uint64 y, bim::uint64 w, bim::uint64 h, const double &value);

    //--------------------------------------------------------------------------    
    // geometry
    //--------------------------------------------------------------------------

    Image downSampleBy2x() const;
    // resample is the direct resampling, pure brute force
    Image resample( uint w, uint h=0, ResizeMethod method = szNearestNeighbor, bool keep_aspect_ratio = false ) const;
    // resize will use image pyramid if size difference is quite large
    Image resize( uint w, uint h=0, ResizeMethod method = szNearestNeighbor, bool keep_aspect_ratio = false ) const;

    // only available values now are +90, -90 and 180
    Image rotate( double deg ) const;
    Image rotate_guess() const; // rotates image guessing orientation using EXIF tags
    Image mirror() const; // mirror the image horizontally
    Image flip() const; // flip the image vertically 

    Image negative() const;

    void trim(double min_v, double max_v) const;

    // same as photoshop levels command
    void color_levels( const double &val_min, const double &val_max, const double &gamma );

    // same as photoshop brightness/contrast command, both values in range [-100, 100]
    void color_brightness_contrast( const int &brightness, const int &contrast );

    // returns a number of pixels above the threshold
    uint64 pixel_counter( const unsigned int &channel, const double &threshold_above );
    std::vector<uint64> pixel_counter( const double &threshold_above );

    //--------------------------------------------------------------------------    
    // some generics
    //--------------------------------------------------------------------------

    // Generic arithmetic with other image
    template <typename T, typename F>
    bool image_arithmetic(const Image &img, F func, const Image &mask = Image());

    // examples of arithmetic
    bool imageArithmetic(const Image &img, Image::ArithmeticOperators op, const Image &mask = Image());

    // generic operations with this image pixels
    template <typename T, typename F, typename A>
    bool pixel_operations(F func, const A &args, const Image &mask = Image());

    // examples of operations
    bool operationThreshold(const double &th, const ThresholdTypes &method = ttBoth, const Image &mask = Image());
    bool operationArithmetic(const double &v, const Image::ArithmeticOperators &op, const Image &mask = Image());
    
    //--------------------------------------------------------------------------    
    // operators 
    //--------------------------------------------------------------------------
    Image operator+ (const Image &img);
    Image operator- (const Image &img);
    Image operator/ (const Image &img);
    Image operator* (const Image &img);

    Image operator+ (const double &v);
    Image operator- (const double &v);
    Image operator/ (const double &v);
    Image operator* (const double &v);

    //--------------------------------------------------------------------------    
    // filters
    //--------------------------------------------------------------------------

    void deinterlace( const DeinterlaceMethod &method = deAverage );

    #ifdef BIM_USE_EIGEN
    Image convolve( const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &filter ) const; // not finished
    #endif //BIM_USE_EIGEN

    #ifdef BIM_USE_OPENCV
    //Image filter_convolve( int w, int h=0, ResizeMethod method = szNearestNeighbor, bool keep_aspect_ratio = false ) const;
    #endif //BIM_USE_OPENCV

    #ifdef BIM_USE_FILTERS
    enum FilterMethod { 
      fmNone=0, 
      fmEdge=1, 
      fmOtsu=2, 
    };
    //Image filter(FilterMethod type ) const;
    Image filter_edge() const;

    // regionSize in pixels, regularization 0-1, with 0 shape is least regular
    Image superpixels( bim::uint64 regionSize, float regularization ) const;
    #endif //BIM_USE_FILTERS

    //--------------------------------------------------------------------------    
    // transforms
    //--------------------------------------------------------------------------
    #ifdef BIM_USE_TRANSFORMS
    /*
    enum TransformGeometryMethod { 
      tmgNone=0, 
      tmgAffine=1, 
      tmgProjective=2, 
    };
    Image transform_geometry( m ) const;
    */

    enum TransformColorMethod { 
      tmcNone=0, 
      tmcRGB2HSV=1, 
      tmcHSV2RGB=2, 
      tmcRGB2WndChrmColor=3, 
    };
    Image transform_color( TransformColorMethod type ) const;

    enum TransformMethod { 
        tmNone         = 0, 
        tmFFT          = 1, 
        tmFFTInv       = 2, 
        tmWavelet      = 3, 
        tmWaveletInv   = 4, 
        tmChebyshev    = 5,
        tmChebyshevInv = 6,
        tmRadon        = 7,
        tmRadonInv     = 8,
    };
    Image transform( TransformMethod type ) const;

    // Hounsfield Units - used for CT (CAT) data
    // provided conversion maps from device dependent to HU (device independent) scale
    // typically this conversion will only make sense for 1 sample per pixel images with signed 16 bit pixels or floating point
    // most devices use slope == 1.0 and intercept == -1024.0
    Image transform_hounsfield(const double &slope=1.0, const double &intercept=-1024.0) const;
    
    // mutable version of same operation, more memory efficient, only valid for float and signed images
    bool transform_hounsfield_inplace(const double &slope, const double &intercept);

    // typical enhancement of CT images using Hounsfield scale, where pixels are normalized using
    // min and max computed from window center and window width given in Hounsfield Units
    // image MUST be previously converted to HU using transform_hounsfield or transform_hounsfield_inplace
    // Typical values of center/width:
    //    HeadSFT:          40 / 80  head soft tissue
    //    Brain             30 / 110
    //    NeckSFT :         60 / 300
    //    Bone :            400 / 2000 
    //    Temporal bones:   400 / 4000  (bones of the scull)
    //    Bone body:        350 / 2500 
    //    Soft Tissue :     40 / 500
    //    SoftTissue(PEDS): 40 / 400   just soft tissue CT (pediatric )
    //    Mediastinum:      400/1800
    //    Bronchial:        -180 / 2600
    //    Lung :            -350 / 2000
    //    Lung 2:           -700 / 1200
    //    Abdomen           -20 / 400
    //    Liver:            60 / 180
    //    Liver W/O:        40 / 150 without contrast
    //    Liver W/C:        100 / 150 with contrast
    //    P Fossa :         30 / 180
    //    CSpineSFT w/o :   40 / 250   Cervical spine without contrast
    //    TLSpineSFT w/o:   40 / 500   Thoracic and Lumbar spine
    //    INFARCT :         40 / 60
    //    OBLIQUE MIP :     200 / 700
    //    MYELOGRAM W/L:    60 / 650
    Image enhance_hounsfield(int depth, DataFormat pxtype, const double &wnd_center, const double &wnd_width, bool empty_outside_range = false ) const;

    // Experimental !!!
    // produces multi channel image with different ranges as separate channels
    // image MUST be previously converted to HU using transform_hounsfield or transform_hounsfield_inplace
    // defined 5 channels:
    //   1 : -inf to -100 : Lungs
    //   2 : -100 to -50  : Fat
    //   3 : -50  to 50   : Brain
    //   4 :  50  to 250  : Organs
    //   5 :  250 to inf  : Bones
    Image multi_hounsfield() const;


    #endif //BIM_USE_TRANSFORMS

    //--------------------------------------------------------------------------    
    // Channels
    //--------------------------------------------------------------------------

    // fast but potentially dangerous function! It will affect all shared references to the same image!!!
    // do deepCopy() before if you might have some shared references
    // the result will have as many channels as there are entries in mapping
    // invalid channel numbers or -1 will become black channels
    // all black channels will point to the same area, so do deepCopy() if you'll modify them
    // all copies of channels in different positions will simply point to memory locations so you can't modify them directly!!!!
    void remapChannels( const std::vector<int> &mapping );
    void remapToRGB();

    // since this method produces only one output channel it will not have any multiple channel pointers to the same meory space
    // althogh it will still affect other shared images so that it is recommended to use deepCopy() if you have any
    void extractChannel( int c );

    // creates an image with channels from both images starting with this
    Image appendChannels( const Image & ) const;

    // CHANNEL FUSION
    // fuse functions are slower but safer than the remap functions, they will physically create memory space for all output channels
    // they also allow mixing channels into the same output channel

    // generic channel fusion by selecting the max intensity of all inputs
    // * the size of the mapping vector dictates how many output channels there will be, 
    // * vector for each channel indicates which input channels are mixed to produce the output channel
    // * the output channel intensity equals the max of all inputs
    // note: see how this will be used in the overloaded fuse methods
    Image fuse( const std::vector< std::set<int> > &mapping ) const;

    // this overloaded method provides a simple way of mixing up to 7 channels into the output 3 channel image
    // this method is mostly good for fluorescence imagery with little colocalization 
    Image fuse( int red, int green, int blue, int yellow=-1, int magenta=-1, int cyan=-1, int gray=-1 ) const;

    // mixing channels using associated weights
    // * the size of the mapping vector dictates how many output channels there will be, 
    // * vector for each channel indicates which input channels are mixed to produce the output channel
    // * the pair should contain a number of the channel as first and weight 0-1 as a second
    // note: see how this will be used in fuseToGrayscale
    Image fuse( const std::vector< std::vector< std::pair<int,float> > > &mapping, FuseMethod method=fmAverage, ImageHistogram *hist=0 ) const;

    Image fuseToGrayscale() const;
    Image fuseToRGB(const std::vector<bim::DisplayColor> &mapping, FuseMethod method=fmAverage, ImageHistogram *hist=0) const;

  public:
      // special function to create image class from an existing bitmap without managing its memory
      // it will not delete the bitmap when destroyed
      void connectToUnmanagedMemory(ImageBitmap *b);

  private:
    // pointer to a shared bitmap
    ImageBitmap *bmp;

    // not shared image metadata - most of the time it is empty
    TagMap metadata;

    // not shared buffer
    DTypedBuffer<unsigned char> buf;

  private:
    static std::vector<ImgRefs*> refs;
    
    inline void print_debug();

    int  getCurrentRefId();
    int  getRefId( ImageBitmap *b );
    void connectToMemory( ImageBitmap *b );
    void connectToNewMemory();
    void disconnectFromMemory();

  private:
      typedef std::map<std::string, ImageModifierProc> map_modifiers;
      static map_modifiers create_modifiers();
      static const map_modifiers modifiers;
};

//------------------------------------------------------------------------------
// ImageHistogram
//------------------------------------------------------------------------------

class ImageHistogram {
  public:
    ImageHistogram( int channels=0, int bpp=0, const DataFormat &fmt=FMT_UNSIGNED );
    
    // mask has to be an 8bpp image where pixels >128 belong to the object of interest
    // if mask has 1 channel it's going to be used for all channels, otherwise they should match
    ImageHistogram( const Image &img, const Image *mask=0 ) { this->channel_mode = Histogram::cmSeparate; fromImage(img, mask); }
    ~ImageHistogram();

    void clear( ) { histograms.clear(); }
    bool isValid() const { return histograms.size()>0 && histograms[0].isValid(); }
    void setChannelMode( const Histogram::ChannelMode &m ) { channel_mode = m; }

    double max_value() const;
    double min_value() const;
    
    // mask has to be an 8bpp image where pixels >128 belong to the object of interest
    // if mask has 1 channel it's going to be used for all channels, otherwise they should match
    void fromImage( const Image &, const Image *mask=0 );
    
    int size() const { return (int) histograms.size(); }
    int channels() const { return size(); }

    const Histogram& histogram_for_channel( int c ) const { return histograms[c]; }
    inline const Histogram* operator[](unsigned int c) const { return &histograms[c]; }
    inline Histogram* operator[](unsigned int c) { return &histograms[c]; }

  public:
    // I/O
    bool to(const std::string &fileName);
    bool to(std::ostream *s);
    bool from(const std::string &fileName);
    bool from(std::istream *s);

    bool toXML(const std::string &fileName);
    bool toXML(std::ostream *s);

  protected:
    Histogram::ChannelMode channel_mode;
    std::vector<Histogram> histograms;
};

//------------------------------------------------------------------------------
// ImageLut
//------------------------------------------------------------------------------

class ImageLut {
  public:
    ImageLut() {}
    ImageLut( const ImageHistogram &in, const ImageHistogram &out ) { init(in, out); }
    ImageLut( const ImageHistogram &in, const ImageHistogram &out, Lut::LutType type, void *args=NULL ) { init(in, out, type, args); }
    ImageLut( const ImageHistogram &in, const ImageHistogram &out, Lut::LutGenerator custom_generator, void *args=NULL ) { init(in, out, custom_generator, args); }
    ~ImageLut() {}

    void init( const ImageHistogram &in, const ImageHistogram &out );
    void init( const ImageHistogram &in, const ImageHistogram &out, Lut::LutType type, void *args=NULL );
    void init( const ImageHistogram &in, const ImageHistogram &out, Lut::LutGenerator custom_generator, void *args=NULL );

    void clear( ) { luts.clear(); }
    
    int size() const { return (int) luts.size(); }
    int channels() const { return size(); }

    int depthInput()  const { if (size()<=0) return 0; return luts[0].depthInput(); }
    int depthOutput() const { if (size()<=0) return 0; return luts[0].depthOutput(); }
    DataFormat dataFormatInput()  const { if (size()<=0) return FMT_UNDEFINED; return luts[0].dataFormatInput(); }
    DataFormat dataFormatOutput() const { if (size()<=0) return FMT_UNDEFINED; return luts[0].dataFormatOutput(); }

    const Lut& lut_for_channel( int c ) const { return luts[c]; }
    inline const Lut* operator[](unsigned int c) const { return &luts[c]; }

  protected:
    std::vector<Lut> luts;
};

/******************************************************************************
  Image member functions
******************************************************************************/


inline uint64 Image::bytesPerLine() const {
  if (bmp==NULL) return 0;
  return (long) ceil( ((double)(bmp->i.width * bmp->i.depth)) / 8.0 );
}

inline uint64 Image::bytesPerRow() const {
  if (bmp==NULL) return 0;
  return (long) ceil( ((double)(bmp->i.height * bmp->i.depth)) / 8.0 );
}

inline uint64 Image::bytesInPixels( int n ) const {
  if (bmp==NULL) return 0;
  return (uint64) ceil( ((double)(n * bmp->i.depth)) / 8.0 );
}

inline uint64 Image::bytesPerChan( ) const {
  if (bmp==NULL) return 0;
  return (uint64) ceil( ((double)(bmp->i.width * bmp->i.depth)) / 8.0 ) * bmp->i.height;
}

inline uint64 Image::availableColors( ) const {
  if (bmp==NULL) return 0;
  return (uint64) pow( 2.0f, (float)(bmp->i.depth * bmp->i.samples) );
}

inline uchar *Image::sampleBits( uint64 s ) const {
  if (bmp==NULL) return NULL;
  if (s >= 512) return NULL;
  return (uchar *) (bmp->bits[s] ? bmp->bits[s] : NULL);
}

inline uchar* Image::scanLine( uint64 s, uint64 y ) const {
  if (bmp==NULL) return NULL;
  if (s >= 512) return NULL;
  if (y >= bmp->i.height) return NULL;

  return ((uchar *) bmp->bits[s]) + bytesPerLine()*y;
}

inline uchar* Image::pixelBits( uint64 s, uint64 x, uint64 y ) const {
  uchar *l = scanLine( s, y );
  if (l==NULL) return NULL;
  return l + bytesInPixels((int)x);
}

template <typename T>
T Image::pixel( uint64 sample, uint64 x, uint64 y ) const {
  if (x>width()) return std::numeric_limits<T>::quiet_NaN();
  if (y>height()) return std::numeric_limits<T>::quiet_NaN();
  T *p = (T*) pixelBits( sample, x, y );
  return *p;
}

inline bool Image::hasLut() const {
  if (!bmp) return false;
  return bmp->i.lut.count>0;
}

inline int Image::lutSize() const {
  if (!bmp) return 0;
  return bmp->i.lut.count;
}

inline RGBA Image::lutColor( uint64 i ) const {
  if (bmp==NULL) return 0;
  if ( i>=bmp->i.lut.count ) return 0;
  return bmp->i.lut.rgba[i];
}

} // namespace bim

#endif //BIM_IMAGE_H


