/*******************************************************************************
 Command line imgcnv utility

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 Run arguments: [[-i | -o] FILE_NAME | -t FORMAT_NAME ]

  -i  - output file name, multiple -i are allowed, but in multiple case each will be interpreted as a 1 page image.
  -o  - output file name
  -t  - format to use

  -page    - pages to extract, should be followed by page numbers separated by comma, ex: -page 1,2,5
             page enumeration starts at 1 and ends at number_of_pages
             page number can be a dash where dash will be substituted by a range of values, ex: -page 1,-,5
             if dash is not followed by any number, maximum will be used, ex: '-page 1,-' means '-page 1,-,number_of_pages'
             if dash is a first caracter, 1 will be used, ex: '-page -,5' means '-page 1,-,5'

  [-roi x1,y1,x2,y2]
  -roi     - region of interest, should be followed by: x1,y1,x2,y2 that defines ROI rectangle, ex: -roi 10,10,100,100
             if x1 or y1 are ommited they will be set to 0, ex: -roi ,,100,100 means 0,0,100,100
             if x2 or y2 are ommited they will be set to image size, ex: -roi 10,10,, means 10,10,width-1,height-1

  [-resize w,h[,NN|BL|BC]]
  -resize - should be followed by: width and height of the new image
           if followed by comma and [NN|BL|BC] allowes to choose interpolation method
           NN - Nearest neighbor (default)
           BL - Bilinear
           BC - Bicubic
           Note: resize now is a smarter method that employs image pyramid for faster processing of large images

  [-resample w,h[,NN|BL|BC]]
  -resample - should be followed by: width and height of the new image
           if followed by comma and [NN|BL|BC] allowes to choose interpolation method
           NN - Nearest neighbor (default)
           BL - Bilinear
           BC - Bicubic

  [-depth integer[,F|D|T|E][,U|S|F]]
  -depth - output depth (in bits) per channel, allowed values now are: 8,16,32,64
           if followed by comma and [F|D|T|E] allowes to choose LUT method
           F - Linear full range
           D - Linear data range (default)
           T - Linear data range with tolerance ignoring very low values
           E - equalized
           if followed by comma and U|S|F] the type of output image can be defined
           U - Unsigned integer (with depths: 8,16,32)
           S - Signed integer (with depths: 8,16,32)
           F - Float (with depths: 32,64,80)

  [-remap int[,int]]
  -remap - set of integers separated by comma specifying output channel order (0 means empty channel), ex: 1,2,3

  [-create w,h,z,t,c,d]
  -create - creates a new image with w-width, h-height, z-num z, t-num t, c - channels, d-bits per channel

  [-rotate deg]
  -rotate - rotates the image by deg degrees, only accepted valueas now are: 90, -90 and 180

  [-sampleframes n]
  -sampleframes - samples for reading every Nth frame (useful for videos), ex: -sampleframes 5

  [-tile n]
  -tile - tilte the image and store tiles in output directory, ex: -tile 256

  [-options "xxxxx"]
  -options  - specify encoder specific options, ex: -options "fps 15 bitrate 1000"

  -norm    - normalize input into 8 bits output
  -stretch - stretch data in it's original range
  -meta    - print image's parsed meta-data
  -rawmeta - print image's raw meta-data in one huge pile
  -fmt     - print supported formats
  -multi   - create a multi-paged image if possible, valid for TIFF, GIF...
  -info    - print image info
  -supported - prints yes/no if the file can be decoded
  -display - creates 3 channel image with preferred channel mapping
  -project - combines by MAX all inout frames into one
  -projectmax - combines by MAX all inout frames into one
  -projectmin - combines by MIN all inout frames into one
  -negative - returns negative of input image

  ------------------------------------------------------------------------------
  Encoder specific options

  All video files AVI, SWF, MPEG, etc:
    fps N - specify Frames per Second, where N is a float number, if empty or 0 uses default, ex: fps 29.9
    bitrate N - specify bitrate, where N is an integer number, if empty or 0 uses default, ex: bitrate 1000

  JPEG:
    quality N - specify encoding quality 0-100, where 100 is best

  TIFF:
    compression N - where N can be: none, packbits, lzw, fax, ex: compression none

  ------------------------------------------------------------------------------
  Ex: imgcnv -i 1.jpg -o 2.tif -t TIFF


 History:
   08/08/2001 21:53:31 - First creation
   12/01/2005 20:54:00 - multipage support
   12/02/2005 14:27:00 - print image info
   02/07/2006 19:29:00 - ROI support
   01/29/2007 15:23:00 - support pixel formats different from power of two
                         done by converting incoming format into supported one,
                         now support only for 12 bit -> 16 bit conversion
   2010-01-25 18:55:54 - support for floating point images throughout the app
   2010-01-29 11:25:38 - preserve all metadata and correctly transform it
                
*******************************************************************************/

#define IMGCNV_VER "2.1.1"

#include <cmath>
#include <cstdio>
#include <cstring>

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <strstream>

#include <BioImageCore>
#include <BioImage>
#include <BioImageFormats>

#include "reg/registration.h"

//------------------------------------------------------------------------------
// return codes
//------------------------------------------------------------------------------

#define IMGCNV_ERROR_NONE                   0
#define IMGCNV_ERROR_NO_INPUT_FILE          1
#define IMGCNV_ERROR_NO_OUTPUT_FILE         2
#define IMGCNV_ERROR_READING_FILE           3
#define IMGCNV_ERROR_READING_FILE_RAW       4
#define IMGCNV_ERROR_WRITING_FILE           5
#define IMGCNV_ERROR_WRITING_NOT_SUPPORTED  6
#define IMGCNV_ERROR_CREATING_IMAGE         7
#define IMGCNV_ERROR_TIMEOUT                99

using namespace bim;

//------------------------------------------------------------------------------
// Command line arguments processing
//------------------------------------------------------------------------------

class DConf: public XConf {

public:
  std::vector<xstring> i_names;
  std::vector<xstring> c_names;
  xstring o_name;
  xstring o_fmt;
  bool normalize;
  bool print_meta;
  bool print_meta_parsed;
  bool print_meta_custom;
  std::string print_tag;
  bool print_formats;
  bool print_formats_xml;
  bool print_formats_html;
  bool multipage;
  bool print_info;
  bool supported;
  std::vector<int> page;
  bool raw_meta;

  bool roi;
  std::vector< bim::Rectangle<int> > rois;

  xstring template_filename;

  bool remap_channels;
  std::vector<int> out_channels;

  bool fuse_channels;
  bool fuse_to_grey;
  bool fuse_to_rgb;
  bool fuse_meta;
  Image::FuseMethod fuse_method;
  std::vector< std::set<int> > out_fuse_channels;
  std::vector<bim::DisplayColor> out_weighted_fuse_channels;

  int out_depth;
  DataFormat out_pixel_format;
  Lut::LutType lut_method;
  Histogram::ChannelMode chan_mode;
  double gamma;
  double minv, maxv;
  bool levels;
  int brightness;
  int contrast;
  
  bool version;

  bool create;
  unsigned int  w,h,z,t,c,d;
  bool resample;
  bool resize;
  bool resize3d;
  Image::ResizeMethod resize_method;
  bool resize_preserve_aspect_ratio;
  bool resize_no_upsample;

  bool textureAtlas;

  bool deinterlace;
  Image::DeinterlaceMethod deinterlace_method;

  bool filter;
  xstring filter_method;
  Image::TransformMethod transform;
  Image::TransformColorMethod transform_color;
  int superpixels;
  float superpixels_regularization;

  bool geometry;
  
  bool resolution;
  double resvals[4];

  bool stretch;

  bool raw;
  unsigned int  p,e;
  bool interleaved;
  DataFormat raw_type;

  bool display;
  bool rotate_guess;
  double rotate_angle;

  bool mirror;
  bool flip;

  bool project;
  bool project_min;
  ImageStack::RearrangeDimensions rearrange3d;

  bool negative;
  double threshold;
  Image::ThresholdTypes threshold_operation;
  bool count_pixels;

  std::string options;

  std::string loadomexml;
  std::string omexml;

  int res_level;
  int tile_size;
  int tile_xid;
  int tile_yid;
  int tile_x1;
  int tile_y1;
  int tile_x2;
  int tile_y2;

  bool mosaic;
  int mosaic_num_x;
  int mosaic_num_y;


  int sample_frames;
  int sample_frames_original;
  int skip_frames_leading;
  int skip_frames_trailing;
  int overlap_frame_sampling;

  bool no_overlap;
  int min_overlap;
  double overlap_frame_scale;

  Image img_previous;
  int reg_numpoints;
  int reg_max_width;
  
  std::string i_histogram_file;
  std::string o_histogram_file;
  std::string o_histogram_format;

public:
  virtual void cureParams();
  void curePagesArray( const int &num_pages );

protected: 
  virtual void init();    
  virtual void processArguments();
};

void DConf::init() {
  XConf::init();

  appendArgumentDefinition( "-i", 1, 
    "input file name, multiple -i are allowed, but in multiple case each will be interpreted as a 1 page image." );

  appendArgumentDefinition( "-il", 1, 
    "list input file name, containing input file name per line of the text file" );

  appendArgumentDefinition( "-c", 1, 
    "additional channels input file name, multiple -c are allowed, in which case multiple channels will be added, -c image must have the same size" );

  appendArgumentDefinition( "-o", 1, "output file name" );
  appendArgumentDefinition( "-t", 1, "output format" );

  appendArgumentDefinition( "-v",          0, "prints version" );

  appendArgumentDefinition( "-meta",        0, "print image's meta-data" );
  appendArgumentDefinition( "-meta-parsed", 0, "print image's parsed meta-data, excluding custom fields" );
  appendArgumentDefinition( "-meta-custom", 0, "print image's custom meta-data fields" );
  appendArgumentDefinition( "-meta-raw",    0, "print image's raw meta-data in one huge pile" );
  appendArgumentDefinition( "-meta-tag",    1, "prints contents of a requested tag, ex: -meta-tag pixel_resolution" );
  appendArgumentDefinition("-meta-keep",   1, "removes all except provided comma separated tags, ex: -meta-keep pixel_resolution,raw/icc_profile");
  appendArgumentDefinition("-meta-remove", 1, "removes provided comma separated tags, ex: -meta-remove pixel_resolution,raw/icc_profile");

  appendArgumentDefinition( "-rawmeta",     0, "print image's raw meta-data in one huge pile" );
  appendArgumentDefinition( "-info",        0, "print image info" );
  appendArgumentDefinition( "-supported",   0, "prints yes/no if the file can be decoded" );
  appendArgumentDefinition( "-loadomexml",  1, "reads OME-XML from a file and writes if output format is OME-TIFF" );

  appendArgumentDefinition( "-fmt",        0, "print supported formats" );
  appendArgumentDefinition( "-fmtxml",     0, "print supported formats in XML" );
  appendArgumentDefinition( "-fmthtml",    0, "print supported formats in HTML" );

  appendArgumentDefinition( "-multi",      0, "creates a multi-paged image if possible (TIFF,AVI), enabled by default" );
  appendArgumentDefinition( "-single",     0, "disables multi-page creation mode" );

  appendArgumentDefinition( "-stretch",    0, "stretch data to it's full range" );
  appendArgumentDefinition( "-norm",       0, "normalize input into 8 bits output" );
  appendArgumentDefinition( "-negative",   0, "returns negative of input image" );
  appendArgumentDefinition( "-display",    0, "creates 3 channel image with preferred channel mapping" );
  appendArgumentDefinition( "-project",    0, "combines by MAX all inout frames into one" );
  appendArgumentDefinition( "-projectmax", 0, "combines by MAX all inout frames into one" );
  appendArgumentDefinition( "-projectmin", 0, "combines by MIN all inout frames into one" );

  appendArgumentDefinition( "-mirror",     0, "mirror the image horizontally" );
  appendArgumentDefinition( "-flip",       0, "flip the image vertically" );

  appendArgumentDefinition("-icc-load",    1, "Load ICC profile from a file");
  appendArgumentDefinition("-icc-save",    1, "Save ICC profile into a file if present");
  appendArgumentDefinition("-icc-transform-file", 1, "Transform image to ICC profile loaded from a file");
  appendArgumentDefinition("-icc-transform-name", 1, "Transform image to ICC profile given by a name: srgb|lab|xyz|cmyk");

  xstring tmp = "tile the image and store tiles in the output directory, ex: -tile 256\n";
  tmp += "  argument defines the size of the tiles in pixels\n";
  tmp += "  tiles will be created based on the outrput file name with inserted L, X, Y, where";
  tmp += "    L - is a resolution level, L=0 is native resolution, L=1 is 2x smaller, and so on";
  tmp += "    X and Y - are tile indices in X and Y, where the first tile is 0,0, second in X is: 1,0 and so on";
  tmp += "  ex: '-o my_file.jpg' will produce files: 'my_file_LLL_XXX_YYY.jpg'\n";
  tmp += "\n";
  tmp += "  Providing more arguments will instruct extraction of embedded tiles with -tile SZ,XID,YID,L ex: -tile 256,2,4,3\n";
  tmp += "    SZ: defines the size of the tile in pixels\n";
  tmp += "    XID and YID - are tile indices in X and Y, where the first tile is 0,0, second in X is: 1,0 and so on";
  tmp += "    L - is a resolution level, L=0 is native resolution, L=1 is 2x smaller, and so on";
  appendArgumentDefinition( "-tile",    1, tmp );

  tmp = "compose an image from aligned tiles, ex: -mosaic 512,20,11\n";
  tmp += "  Arguments are defined as SZ,NX,NY where:\n";
  tmp += "    SZ: defines the size of the tile in pixels with width equal to height\n";
  tmp += "    NX - number of tile images in X direction";
  tmp += "    NY - number of tile images in Y direction";
  appendArgumentDefinition("-mosaic", 1, tmp);

  tmp = "extract a specified pyramidal level, ex: -res-level 4\n";
  tmp += "    L - is a resolution level, L=0 is native resolution, L=1 is 2X smaller, L=2 is 4X smaller, and so on";
  appendArgumentDefinition("-res-level", 1, tmp);

  appendArgumentDefinition( "-rotate", 1, 
    "rotates the image by deg degrees, only accepted valueas now are: 90, -90, 180, guess\nguess will extract suggested rotation from EXIF" );
  
  appendArgumentDefinition( "-remap", 1, 
    "Changes order and number of channels in the output, channel numbers are separated by comma (0 means empty channel), ex: -remap 1,2,3" );

  tmp = "Changes order and number of channels in the output additionally allowing combining channels\n";
  tmp += "Channels separated by comma specifying output channel order (0 means empty channel)\n";
  tmp += "multiple channels can be added using + sign, ex: -fuse 1+4,2+4+5,3";
  appendArgumentDefinition( "-fuse", 1, tmp );

  tmp = "Produces 3 channel image from up to 6 channels\n";
  tmp += "Channels separated by comma in the following order: Red,Green,Blue,Yellow,Magenta,Cyan,Gray\n";
  tmp += "(0 or empty value means empty channel), ex: -fuse6 1,2,3,4\n";
  appendArgumentDefinition( "-fuse6", 1, tmp );

  appendArgumentDefinition( "-fusegrey", 0, 
    "Produces 1 channel image averaging all input channels, uses RGB weights for 3 channel images and equal weights for all others, ex: -fusegrey" );

  appendArgumentDefinition( "-fusemeta", 0, 
    "Produces 3 channel image getting fusion weights from embedded metadata, ex: -fusemeta" );

  appendArgumentDefinition("-enhancemeta", 0,
      "Enhances an image beased on preferred settings, currently only CT hounsfield mode is supported, ex: -enhancemeta");

  tmp = "Produces 3 channel image from N channels, for each channel an RGB weight should be given\n";
  tmp += "Component contribution are separated by comma and channels are separated by semicolon:\n";
  tmp += "(0 or empty value means no output), ex: -fusergb 100,0,0;0,100,100;0;0,0,100\n";
  tmp += "Here ch1 will go to red, ch2 to cyan, ch3 not rendered and ch4 to blue\n";
  appendArgumentDefinition( "-fusergb", 1, tmp );

  tmp = "Defines fusion method, ex: -fusemethod a\n";
  tmp += "  should be followed by comma and [a|m]\n";
  tmp += "    a - Average\n";
  tmp += "    m - Maximum\n";
  appendArgumentDefinition( "-fusemethod", 1, tmp );

  tmp = "Re-arranges dimensions of a 3D image, ex: -rearrange3d xzy\n";
  tmp += "  should be followed by comma and [xzy|yzx]\n";
  tmp += "    xzy - rearranges XYZ -> XZY\n";
  tmp += "    yzx - rearranges XYZ -> YZX\n";
  appendArgumentDefinition( "-rearrange3d", 1, tmp );

  appendArgumentDefinition( "-create", 1, 
    "creates a new image with w-width, h-height, z-num z, t-num t, c - channels, d-bits per channel, ex: -create 100,100,1,1,3,8" );
  
  appendArgumentDefinition( "-geometry", 1, 
    "redefines geometry for any incoming image with: z-num z, t-num t and optionally c-num channels, ex: -geometry 5,1 or -geometry 5,1,3" );

  appendArgumentDefinition( "-resolution", 1, 
    "redefines resolution for any incoming image with: x,y,z,t where x,y,z are in microns and t in seconds  ex: -resolution 0.012,0.012,1,0" );

  appendArgumentDefinition( "-resample", 1, 
    "Is the same as resize, the difference is resample is brute force and resize uses image pyramid for speed" );

  appendArgumentDefinition( "-sampleframes", 1, 
    "samples for reading every Nth frame (useful for videos), ex: -sampleframes 5" );

  appendArgumentDefinition( "-skip-frames-leading", 1, 
    "skip N initial frames of a sequence, ex: -skip-frames-leading 5" );

  appendArgumentDefinition( "-skip-frames-trailing", 1, 
    "skip N final frames of a sequence, ex: -skip-frames-trailing 5" );

  appendArgumentDefinition( "-ihst", 1, 
    "read image histogram from the file and use for nhancement operations" );

  appendArgumentDefinition( "-ohst", 1, 
    "write image histogram to the file" );

  appendArgumentDefinition( "-ohstxml", 1, 
    "write image histogram to the XML file" );

  tmp = "output information about the processing progress, ex: -verbose\n";
  tmp += "  verbose allows argument that defines the amount of info, currently: 1 and 2\n";
  tmp += "  where: 1 is the light info output, 2 is full output\n";
  appendArgumentDefinition( "-verbose", 1, tmp );

  tmp = "Skips frames that overlap with the previous non-overlapping frame, ex: -no-overlap 5\n";
  tmp += "  argument defines maximum allowed overlap in %, in the example it is 5%\n";
  appendArgumentDefinition( "-no-overlap", 1, tmp );

  tmp = "Defines quality for image alignment in number of starting points, ex: -reg-points 200\n";
  tmp += "  Suggested range is in between 32 and 512, more points slow down the processing\n";
  appendArgumentDefinition( "-reg-points", 1, tmp );

  tmp = "Defines sampling after overlap detected until no overlap, used to reduce sampling if overlapping, ex: -overlap-sampling 5\n";
  appendArgumentDefinition( "-overlap-sampling", 1, tmp );
 
  tmp = "pages to extract, should be followed by page numbers separated by comma, ex: -page 1,2,5\n";
  tmp += "  page enumeration starts at 1 and ends at number_of_pages\n";
  tmp += "  page number can be a dash where dash will be substituted by a range of values, ex: -page 1,-,5";
  tmp += "  if dash is not followed by any number, maximum will be used, ex: '-page 1,-' means '-page 1,-,number_of_pages'\n";
  tmp += "  if dash is a first caracter, 1 will be used, ex: '-page -,5' means '-page 1,-,5'";
  appendArgumentDefinition( "-page", 1, tmp );

  tmp = "regions of interest, should be followed by: x1,y1,x2,y2 that defines ROI rectangle, ex: -roi 10,10,100,100\n";
  tmp += "  if x1 or y1 are ommited they will be set to 0, ex: -roi ,,100,100 means 0,0,100,100\n";
  tmp += "  if x2 or y2 are ommited they will be set to image size, ex: -roi 10,10,, means 10,10,width-1,height-1\n";
  tmp += "  if more than one region of interest is desired, specify separated by ';', ex: -roi 10,10,100,100;20,20,120,120\n";
  tmp += "  in case of multiple regions, specify a template for output file creation with following variables, ex: -template {output_filename}_{x1}.{y1}.{x2}.{y2}.tif";
  appendArgumentDefinition( "-roi", 1, tmp );

  tmp = "region of interest, should be followed by: x1,y1,x2,y2[,L] that defines ROI rectangle, ex: -tile-roi 10,10,100,100,0\n";
  tmp += "the difference from -roi is in how the image is loaded, in this case if operating on a tiled image\n";
  tmp += "only the required sub-region will be loaded, similar to tile interface but with arbitrary position\n";
  tmp += "this means that all enhancements will be local to the ROI and glogal histogram will be needed";
  tmp += "L is the pyramid level, 0=100%, 1=50%, 2=25%, etc...";
  appendArgumentDefinition("-tile-roi", 1, tmp);

  tmp = "Define a template for file names, ex: -template {output_filename}_{n}.tif\n";
  tmp += "  templates specify variables inside {} blocks, available variables vary for different processing";
  appendArgumentDefinition("-template", 1, tmp);

  appendArgumentDefinition("-textureatlas", 0, "Produces a texture atlas 2D image for 3D input images");
  tmp = "Creates custom texture atlas with: rows,cols ex: -texturegrid 5,7\n";
  appendArgumentDefinition("-texturegrid", 1, tmp);

  tmp = "output depth (in bits) per channel, allowed values now are: 8,16,32,64, ex: -depth 8,D,U\n";
  tmp += "  if followed by comma and [F|D|T|E] allowes to choose LUT method\n";
  tmp += "    F - Linear full range\n";
  tmp += "    D - Linear data range (default)\n";
  tmp += "    T - Linear data range with tolerance ignoring very low values\n";
  tmp += "    E - equalized\n";
  tmp += "    C - type cast\n";
  tmp += "    N - floating point number [0, 1]\n";
  tmp += "    G - Gamma correction, requires setting -gamma\n";
  tmp += "    L - Levels: Min, Max and Gamma correction, requires setting -gamma, -maxv and -minv\n";
  tmp += "  if followed by comma and [U|S|F] the type of output image can be defined\n";
  tmp += "    U - Unsigned integer (with depths: 8,16,32,64) (default)\n";
  tmp += "    S - Signed integer (with depths: 8,16,32,64)\n";
  tmp += "    F - Float (with depths: 32,64,80)\n";
  tmp += "  if followed by comma and [CS|CC] sets channel mode\n";
  tmp += "    CS - channels separate, each channel enhanced separately (default)\n";
  tmp += "    CC - channels combined, channels enhanced together preserving mutual relationships\n";
  appendArgumentDefinition( "-depth", 1, tmp );

  tmp = "enhances CT image using hounsfield scale, ex: -hounsfield 8,U,40,80\n";
  tmp += "  output depth (in bits) per channel, allowed values now are: 8,16,32,64\n";
  tmp += "  followed by comma and [U|S|F] the type of output image can be defined\n";
  tmp += "    U - Unsigned integer (with depths: 8,16,32,64) (default)\n";
  tmp += "    S - Signed integer (with depths: 8,16,32,64)\n";
  tmp += "    F - Float (with depths: 32,64,80)\n";
  tmp += "  followed by comma and window center\n";
  tmp += "  followed by comma and window width\n";
  tmp += "  optionally followed by comma and slope\n";
  tmp += "  followed by comma and intercept, ex: -hounsfield 8,U,40,80,1.0,-1024.0\n";
  tmp += "  if slope and intercept are not set, their values would be red from DICOM metadata, defaulting to 1 and -1024\n";
  appendArgumentDefinition("-hounsfield", 1, tmp);

  tmp = "sets gamma for histogram conversion: 0.5, 1.0, 2.2, etc, ex: -gamma 2.2\n";
  appendArgumentDefinition( "-gamma", 1, tmp );

  tmp = "sets max value for histogram conversion, ex: -maxv 240\n";
  appendArgumentDefinition( "-maxv", 1, tmp );

  tmp = "sets min value for histogram conversion, ex: -minv 20\n";
  appendArgumentDefinition( "-minv", 1, tmp );

  tmp = "color levels adjustment: min,max,gamma, ex: -levels 15,200,1.2\n";
  appendArgumentDefinition( "-levels", 1, tmp );

  tmp = "color brightness/contrast adjustment: brightness,contrast, each in range [-100,100], ex: -brightnesscontrast 50,-40\n";
  appendArgumentDefinition( "-brightnesscontrast", 1, tmp );


  if (keyExists( "-threshold" )) {
    std::vector<xstring> strl = splitValue("-threshold");
    threshold = strl[0].toDouble(0);
    if (strl.size()>1) {
        if ( strl[1].toLowerCase()=="lower" ) threshold_operation = Image::ttLower;
        if ( strl[1].toLowerCase()=="upper" ) threshold_operation = Image::ttUpper;
        if ( strl[1].toLowerCase()=="both" )  threshold_operation = Image::ttBoth;
    }
  }

  tmp = "thresholds the image, ex: -threshold 120,upper\n";
  tmp += "  value is followed by comma and [lower|upper|both] to selet thresholding method\n";
  tmp += "    lower - sets pixels below the threshold to lowest possible value\n";
  tmp += "    upper - sets pixels above or equal to the threshold to highest possible value\n";
  tmp += "    both - sets pixels below the threshold to lowest possible value and above or equal to highest\n";
  appendArgumentDefinition( "-threshold", 1, tmp );

  tmp = "counts pixels above and below a given threshold, requires output file name to store resultant XML file, ex: -pixelcounts 120\n";
  appendArgumentDefinition( "-pixelcounts", 1, tmp );

  tmp = "should be followed by: width and height of the new image, ex: -resize 640,480\n";
  tmp += "  if one of the numbers is ommited or 0, it will be computed preserving aspect ratio, ex: -resize 640,,NN\n";
  tmp += "  if followed by comma and [NN|BL|BC] allowes to choose interpolation method, ex: -resize 640,480,NN\n";
  tmp += "    NN - Nearest neighbor (default)\n";
  tmp += "    BL - Bilinear\n";
  tmp += "    BC - Bicubic\n";
  tmp += "  if followed by comma [AR|MX|NOUP], the sizes will be limited:\n";
  tmp += "    AR - resize preserving aspect ratio, ex: 640,640,NN,AR\n";
  tmp += "    MX|NOUP - size will be used as maximum bounding box, preserving aspect ratio and not upsampling, ex: 640,640,NN,MX";
  appendArgumentDefinition( "-resize", 1, tmp );

  tmp = "deinterlaces input image with one of the available methods, ex: -deinterlace avg\n";
  tmp += "    odd  - Uses odd lines\n";
  tmp += "    even - Uses even lines\n";
  tmp += "    avg  - Averages lines\n";
  appendArgumentDefinition( "-deinterlace", 1, tmp );

  tmp = "transforms input image, ex: -transform fft\n";
  tmp += "    chebyshev - outputs a transformed image in double precision\n";
  tmp += "    fft - outputs a transformed image in double precision\n";
  tmp += "    radon - outputs a transformed image in double precision\n";
  tmp += "    wavelet - outputs a transformed image in double precision";
  appendArgumentDefinition( "-transform", 1, tmp );

  tmp = "transforms input image 3 channel image in color space, ex: -transform_color rgb2hsv\n";
  tmp += "    hsv2rgb - converts HSV -> RGB\n";
  tmp += "    rgb2hsv - converts RGB -> HSV";
  appendArgumentDefinition( "-transform_color", 1, tmp );

  tmp = "Segments image using SLIC superpixel method, takes region size and regularization, ex: -superpixels 16,0.2[,0.7]\n";
  tmp += "    region size is in pixels\n";
  tmp += "    regularization - [0-1], where 0 means shape is least regular";
  tmp += "    minimum size - [0-1], is optional and defines the minimum region size computed from region size, 1 will ensure minimum size at region size. Default value is 0.7";
  appendArgumentDefinition( "-superpixels", 1, tmp );

  tmp = "filters input image, ex: -filter edge\n";
  tmp += "    edge - first derivative\n";
  tmp += "    otsu - b/w masked image\n";
  tmp += "    wndchrmcolor - color quantized hue image";
  appendArgumentDefinition( "-filter", 1, tmp );

  tmp = "performs 3D interpolation on an input image, ex: -resize3d 640,480,16\n";
  tmp += "  if one of the W/H numbers is ommited or 0, it will be computed preserving aspect ratio, ex: -resize3d 640,,16,NN\n";
  tmp += "  if followed by comma and [NN|BL|BC] allowes to choose interpolation method, ex: -resize3d 640,480,16,BC\n";
  tmp += "    NN - Nearest neighbor (default)\n";
  tmp += "    TL - Trilinear\n";
  tmp += "    TC - Tricubic\n";
  tmp += "  if followed by comma AR, the size will be used as maximum bounding box to resize preserving aspect ratio, ex: 640,640,16,BC,AR";
  appendArgumentDefinition( "-resize3d", 1, tmp );

  tmp = "reads RAW image with w,h,c,d,p,e,t,interleaved ex: -raw 100,100,3,8,10,0,uint8,1\n";
  tmp += "  w-width, h-height, c - channels, d-bits per channel, p-pages\n";
  tmp += "  e-endianness(0-little,1-big), if in doubt choose 0\n";
  tmp += "  t-pixel type: int8|uint8|int16|uint16|int32|uint32|float|double, if in doubt choose uint8\n";
  tmp += "  interleaved - (0-planar or RRRGGGBBB, 1-interleaved or RGBRGBRGB)";
  appendArgumentDefinition( "-raw", 1, tmp );

  tmp = "specify encoder specific options, ex: -options \"fps 15 bitrate 1000\"\n\n";
  tmp += "Video files AVI, SWF, MPEG, etc. encoder options:\n";
  tmp += "  fps N - specify Frames per Second, where N is a float number, if empty or 0 uses default, ex: -options \"fps 29.9\"\n";
  tmp += "  bitrate N - specify bitrate in Mb, where N is an integer number, if empty or 0 uses default, ex: -options \"bitrate 10000000\"\n\n";
  tmp += "JPEG encoder options:\n";
  tmp += "  quality N - specify encoding quality 0-100, where 100 is best, ex: -options \"quality 90\"\n";
  tmp += "  progressive no - disables progressive JPEG encoding\n";  
  tmp += "  progressive yes - enables progressive JPEG encoding (default)\n\n";    
  tmp += "TIFF encoder options:\n";
  tmp += "  compression N - where N can be: none, packbits, lzw, fax, jpeg, zip, lzma, jxr. ex: -options \"compression lzw\"\n";
  tmp += "  quality N - specify encoding quality 0-100, where 100 is best, ex: -options \"quality 90\"\n";
  tmp += "  tiles N - write tiled TIFF where N defined tile size, ex: tiles -options \"512\"\n";
  tmp += "  pyramid N - writes TIFF pyramid where N is a storage type: subdirs, topdirs, ex: -options \"compression lzw tiles 512 pyramid subdirs\"\n\n";
  tmp += "JPEG-2000 encoder options:\n";
  tmp += "  tiles N - write tiled TIFF where N defined tile size, ex: tiles -options \"2048\"\n";
  tmp += "  quality N - specify encoding quality 0-100, where 100 is lossless, ex: -options \"quality 90\"\n";
  tmp += "JPEG-XR encoder options:\n";
  tmp += "  quality N - specify encoding quality 0-100, where 100 is lossless, ex: -options \"quality 90\"\n";
  tmp += "WebP encoder options:\n";
  tmp += "  quality N - specify encoding quality 0-100, where 100 is lossless, ex: -options \"quality 90\"\n";
  appendArgumentDefinition( "-options", 1, tmp );

  

  // ---------------------------------------------
  // init the vars
  o_fmt = "TIFF";
  normalize     = false;
  print_meta    = false;
  print_meta_parsed = false;
  print_meta_custom = false;

  print_formats = false;
  print_formats_xml = false;
  print_formats_html = false;
  multipage     = true;
  print_info    = false;
  supported     = false;
  raw_meta      = false;
  //page          = 0; // first page is 1

  res_level = 0;
  tile_size = 0;
  tile_xid = -1;
  tile_yid = -1;
  tile_x1 = -1;
  tile_y1 = -1;
  tile_x2 = -1;
  tile_y2 = -1;

  mosaic = false;
  mosaic_num_x = 0;
  mosaic_num_y = 0;

  roi           = false;
 
  remap_channels = false;
  out_channels.resize(0);

  fuse_channels = false;
  fuse_to_grey  = false; 
  fuse_to_rgb   = false; 
  fuse_meta     = false;
  fuse_method   = Image::fmAverage;
  out_fuse_channels.resize(0);

  out_depth = 0;
  out_pixel_format = FMT_UNSIGNED;
  lut_method = Lut::ltLinearFullRange;
  chan_mode =  Histogram::cmSeparate;
  gamma=1;
  minv=0; maxv=0;
  levels=false;
  brightness=0;
  contrast=0;

  version       = false;

  create        = false;
  w=0; h=0; z=0; t=0; c=0; d=0;
  resize        = false;
  resize3d      = false;
  resample      = false;
  resize_method = Image::szNearestNeighbor;
  resize_preserve_aspect_ratio = false;
  resize_no_upsample = false;

  textureAtlas = false;

  deinterlace = false;
  deinterlace_method = Image::deAverage;

  filter = false;
  filter_method = "";
  transform = Image::tmNone;
  transform_color = Image::tmcNone;
  superpixels = 0;
  superpixels_regularization = 0.0;

  geometry = false;
  resolution = false;

  stretch = false;

  raw = false;
  raw_type = FMT_UNSIGNED;
  p = 0;
  e=0;
  interleaved = false;

  display = false;
  rotate_guess = false;
  rotate_angle = 0;

  mirror = false;
  flip = false;

  sample_frames = 0;
  skip_frames_leading=0;
  skip_frames_trailing=0;
  
  project = false;
  project_min = false;
  rearrange3d = ImageStack::adNone;

  negative = false;
  threshold = 0;
  threshold_operation = Image::ttNone;
  count_pixels = false;

  no_overlap = false;
  min_overlap = 0;
  overlap_frame_sampling = 0;
  reg_numpoints = REG_Q_GOOD_QUALITY;
  reg_max_width = 400; // 320 450 640

  #if defined(DEBUG) || defined(_DEBUG)
  verbose = 2;
  #else
  verbose = 0;
  #endif

  tile_size = 0;
}

void DConf::cureParams() {
  // if requested creating a new image
  if ( this->create ) {
    // while creating a new image we should not be extracting one page of that
    this->page.clear();
    // turn on multipage automatically
    this->multipage = true;
    // disable all printouts
    this->raw_meta = false;
    this->print_meta = false;
    this->print_info = false;
    this->resize = false;
    this->resize3d = false;
    this->textureAtlas = false;
    this->raw = false;
    this->deinterlace = false;
  }

  if ( this->i_names.size()>1 ) {
    // while creating a new image from multiple we should not be extracting one page of that
    //this->page.clear(); // dima: no need to impose this
  }

  if ( this->display ) {
    this->remap_channels = false;
    this->fuse_channels = false;
  }

  if ( this->project )
    this->multipage = false;

  if ( this->print_info || this->raw_meta || this->print_meta || this->print_formats || this->print_formats_xml || this->print_formats_html || this->supported )
    this->multipage = false;
}

void DConf::curePagesArray( const int &num_pages ) {

  for (int i=(int)page.size()-1; i>=0; --i) {
    if (page[i] > num_pages)
      page.erase(page.begin()+i);
  }

  for (int i=(int)page.size()-1; i>=0; --i) {
    if (page[i] <= 0) {
      int first = 0;
      int last = num_pages+1;

      if (i>0) first = page[i-1];
      if (i<page.size()-1) last = page[i+1];
      std::vector<int> vals;
      for (int x=first+1; x<last; ++x)
        vals.push_back(x);

      page.insert( page.begin()+i+1, vals.begin(), vals.end() );
      page.erase(page.begin()+i);
    }
  }
}

void DConf::processArguments() {

  i_names = getValues( "-i" );
  c_names = getValues( "-c" );
  o_name  = getValue( "-o" );
  if (keyExists("-t")) 
      o_fmt = getValue( "-t" ).toLowerCase();
  if (keyExists("-template")) 
      template_filename = getValue("-template");

  i_histogram_file = getValue( "-ihst" );
  o_histogram_file = getValue( "-ohst" );
  if (keyExists( "-ohstxml" )) {
      o_histogram_format = "xml";
      o_histogram_file = getValue( "-ohstxml" );
  }

  normalize  = keyExists( "-norm" ); 
  print_meta = keyExists( "-meta" ); 
  raw_meta   = keyExists( "-rawmeta" );
  raw_meta   = keyExists( "-meta-raw" );
  if (raw_meta) print_meta = true;  
  loadomexml = getValue( "-loadomexml" );
  
  print_meta_parsed = keyExists( "-meta-parsed" );
  print_meta_custom = keyExists( "-meta-custom" );
  if (print_meta_parsed) print_meta = true;  
  if (print_meta_custom) print_meta = true;  

  if (keyExists("-meta-tag")) {
    print_tag = getValue("-meta-tag");
    print_meta = true; 
  }

  print_formats      = keyExists( "-fmt" ); 
  print_formats_xml  = keyExists( "-fmtxml" );
  print_formats_html = keyExists( "-fmthtml" ); 
  if (print_formats_xml) print_formats = true;
  if (print_formats_html) print_formats = true;

  //multipage  = keyExists( "-multi" ); 
  if (keyExists("-single")) multipage = false;

  print_info = keyExists( "-info" ); 
  supported  = keyExists( "-supported" ); 
  stretch    = keyExists( "-stretch" ); 
  version    = keyExists( "-v" ); 
  display    = keyExists( "-display" ); 
  negative   = keyExists( "-negative" ); 
  mirror     = keyExists( "-mirror" ); 
  flip       = keyExists( "-flip" ); 

  if (keyExists( "-project" )) { project = true; project_min = false; }
  if (keyExists( "-projectmax")) { project = true; project_min = false; } 
  if (keyExists( "-projectmin")) { project = true; project_min = true; }   

  if (keyExists( "-rearrange3d" )) { 
      xstring str = getValue("-rearrange3d");
      if (str.toLowerCase() == "xzy") this->rearrange3d = ImageStack::adXZY;
      if (str.toLowerCase() == "yzx") this->rearrange3d = ImageStack::adYZX;
  }

  sample_frames          = getValueInt("-sampleframes", 0);
  sample_frames_original = sample_frames;
  skip_frames_leading    = getValueInt("-skip-frames-leading", 0);
  skip_frames_trailing   = getValueInt("-skip-frames-trailing", 0);
  overlap_frame_sampling = getValueInt("-overlap-sampling", 0);


  options       = getValue("-options");
  page          = splitValueInt( "-page", 0 );

  if (keyExists( "-il" )) {
      std::string listname = getValue("-il");
      std::ifstream text (listname.c_str());
      if (text.is_open()) {
          i_names.clear();
          while (text.good()) {
              xstring line;
              getline(text, line);
              line = line.strip(" ");
              if (line.size()>0)
                  i_names.push_back(line);
          }
          text.close();
      }
  }


  res_level = getValueInt("-res-level", 0);

  if (keyExists("-tile")) {
      std::vector<int> t = splitValueInt("-tile");
      if (t.size() == 1) {
          tile_size = t[0];
      } else if (t.size() > 3) {
          tile_size = t[0];
          tile_xid  = t[1];
          tile_yid  = t[2];
          res_level = t[3];
      }
  }

  if (keyExists("-tile-roi")) {
      std::vector<int> t = splitValueInt("-tile-roi");
      if (t.size() > 3) {
          tile_x1 = t[0];
          tile_y1 = t[1];
          tile_x2 = t[2];
          tile_y2 = t[3];
      }
      if (t.size() > 4) {
          res_level = t[4];
      }
  }

  if (keyExists("-mosaic")) {
      std::vector<int> t = splitValueInt("-mosaic");
      if (t.size() > 2) {
          tile_size = t[0];
          mosaic_num_x = t[1];
          mosaic_num_y = t[2];
          mosaic = true;
      }
  }


  if (keyExists( "-no-overlap" )) { 
    no_overlap = true; 
    min_overlap = getValueInt("-no-overlap", 0);
  }

  if (keyExists( "-reg-points" ))
    reg_numpoints = getValueInt("-reg-points", reg_numpoints);

  if (keyExists( "-verbose" ))
    verbose = getValueInt("-verbose", 1);

  if (keyExists( "-rotate" )) {
      if (getValue("-rotate").toLowerCase() == "guess")
          rotate_guess = true;
      else {
          rotate_angle = getValueDouble( "-rotate", 0 );
          if ( rotate_angle!=0 && rotate_angle!=90 && rotate_angle!=-90 && rotate_angle!=180 ) { 
              std::cout << "This rotation angle value is not yet supported..." << std::endl;
            exit(0);
          }
      }
  }

  if (keyExists( "-roi" )) {
      std::vector<xstring> strs = splitValue("-roi", "", ";");
      for (unsigned int i=0; i<strs.size(); ++i) {
          std::vector<int> ints = strs[i].splitInt(",", -1);
          int x1 = ints.size()>0 ? ints[0] : -1;
          int y1 = ints.size()>1 ? ints[1] : -1;
          int x2 = ints.size()>2 ? ints[2] : -1;
          int y2 = ints.size()>3 ? ints[3] : -1;
          if (x1 >= 0 || x2 >= 0 || y1 >= 0 || y2 >= 0)
              this->rois.push_back(bim::Rectangle<int>(bim::Point<int>(x1, y1), bim::Point<int>(x2, y2) ));
      }
      roi = false;
      if (rois.size()>0 && rois[0] >= 0) 
          roi = true;

  }

  if (keyExists( "-depth" )) {
    std::vector<xstring> strl = splitValue("-depth");
    out_depth = strl[0].toInt(0);
    if (strl.size()>1) {
      if ( strl[1].toLowerCase()=="f" ) lut_method = Lut::ltLinearFullRange;
      if ( strl[1].toLowerCase()=="d" ) lut_method = Lut::ltLinearDataRange;
      if ( strl[1].toLowerCase()=="t" ) lut_method = Lut::ltLinearDataTolerance;
      if ( strl[1].toLowerCase()=="e" ) lut_method = Lut::ltEqualize;
      if ( strl[1].toLowerCase()=="c" ) lut_method = Lut::ltTypecast;
      if ( strl[1].toLowerCase()=="n" ) lut_method = Lut::ltFloat01;
      if ( strl[1].toLowerCase()=="g" ) lut_method = Lut::ltGamma;
      if ( strl[1].toLowerCase()=="l" ) lut_method = Lut::ltMinMaxGamma;
    }
    if (strl.size()>2) {
      if ( strl[2].toLowerCase()=="u" ) out_pixel_format = FMT_UNSIGNED;
      if ( strl[2].toLowerCase()=="s" ) out_pixel_format = FMT_SIGNED;
      if ( strl[2].toLowerCase()=="f" ) out_pixel_format = FMT_FLOAT;
    }
    if (strl.size()>3) {
        if ( strl[3].toLowerCase()=="cs" ) chan_mode = Histogram::cmSeparate;
        if ( strl[3].toLowerCase()=="cc" ) chan_mode = Histogram::cmCombined;
    }

  }
  gamma = getValueDouble( "-gamma", 1.0 );
  maxv  = getValueDouble( "-maxv", 0.0 );
  minv  = getValueDouble( "-minv", 0.0 );

  if (keyExists( "-levels" )) {
    std::vector<double> strl = splitValueDouble("-levels");
    this->levels = true;
    if (strl.size()>0) minv = strl[0];
    if (strl.size()>1) maxv = strl[1];
    if (strl.size()>2) gamma = strl[2];
  }

  if (keyExists( "-brightnesscontrast" )) {
    std::vector<int> strl = splitValueInt("-brightnesscontrast");
    if (strl.size()>0) brightness = strl[0];
    if (strl.size()>1) contrast = strl[1];
  }

  if (keyExists( "-threshold" )) {
    std::vector<xstring> strl = splitValue("-threshold");
    threshold = strl[0].toDouble(0);
    if (strl.size()>1) {
        if ( strl[1].toLowerCase()=="lower" ) threshold_operation = Image::ttLower;
        if ( strl[1].toLowerCase()=="upper" ) threshold_operation = Image::ttUpper;
        if ( strl[1].toLowerCase()=="both" )  threshold_operation = Image::ttBoth;
    }
  }

  if (keyExists( "-pixelcounts" )) {
      threshold = getValueDouble("-pixelcounts");
      count_pixels = true;
      // requires output file name to store pixel counts
  }

  if (keyExists( "-remap" )) {
    out_channels = splitValueInt( "-remap" );
    for (unsigned int i=0; i<out_channels.size(); ++i)
      out_channels[i] = out_channels[i]-1;
    remap_channels = true;
  }

  if (keyExists( "-fuse" )) {
    std::vector<xstring> fc = splitValue( "-fuse" );
    for (int i=0; i<fc.size(); ++i) {
      std::vector<xstring> ss = fc[i].split( "+" );
      std::set<int> s;
      for (int p=0; p<ss.size(); ++p)
        s.insert( ss[p].toInt()-1 );
      out_fuse_channels.push_back(s);
    }
    if (out_fuse_channels.size()>0) fuse_channels = true;
  }

  if (keyExists( "-fuse6" )) {
    std::vector<int> c = splitValueInt( "-fuse6" );
    c.resize(7, 0);
    std::set<int> rv, gv, bv;
    rv.insert(c[0]-1);
    rv.insert(c[3]-1);
    rv.insert(c[4]-1);
    rv.insert(c[6]-1);
    gv.insert(c[1]-1);
    gv.insert(c[3]-1);
    gv.insert(c[5]-1);
    gv.insert(c[6]-1);
    bv.insert(c[2]-1);
    bv.insert(c[4]-1);
    bv.insert(c[5]-1);
    bv.insert(c[6]-1);

    out_fuse_channels.push_back(rv);
    out_fuse_channels.push_back(gv);
    out_fuse_channels.push_back(bv);
    if (out_fuse_channels.size()>0) fuse_channels = true;
  }

  if (keyExists( "-fusegrey" )) {
    fuse_to_grey = true;
    fuse_channels = true;
  }

  if (keyExists( "-fusemeta" )) {
    fuse_meta = true;
    fuse_channels = true;
  }

  if (keyExists( "-fusergb" )) {
    std::vector<xstring> ch = splitValue( "-fusergb", "", ";" );
    out_weighted_fuse_channels.clear();
    // trim trailing empty channels
    for (int c = ch.size()-1; c>=0; --c) {
        if (ch[c] == "") ch.resize(c); else break;
    }

    for (int c=0; c<ch.size(); ++c) {
        std::vector<int> cmp = ch[c].splitInt(",");
        cmp.resize(3,0);
        out_weighted_fuse_channels.push_back(bim::DisplayColor(cmp[0], cmp[1], cmp[2]));
    }

    fuse_channels = true;
    fuse_to_rgb = true;
  }

  if (keyExists( "-fusemethod" )) {
      if (getValue("-fusemethod").toLowerCase() == "m")
          fuse_method = Image::fmMax;
  }

  if (keyExists( "-create" )) {
    std::vector<int> ints = splitValueInt( "-create" );
    for (unsigned int x=0; x<ints.size(); ++x)
      if (ints[x] <= 0) { 
          std::cout << "Unable to create an image, some parameters are invalid!Note that one image lives in 1 time and 1 z points..." << std::endl;
        exit(0);
      }

    if ( ints.size() >= 6  ) {
      this->w = ints[0]; 
      this->h = ints[1]; 
      this->z = ints[2]; 
      this->t = ints[3]; 
      this->c = ints[4]; 
      this->d = ints[5];
      this->create = true;
    }
  }

  if (keyExists( "-geometry" )) {
    std::vector<int> ints = splitValueInt( "-geometry" );
    for (unsigned int x=0; x<ints.size(); ++x)
      if (ints[x] <= 0) { 
          std::cout << "Incorrect geometry values! Note that one image lives in 1 time and 1 z points..." << std::endl;
        exit(0);
      }

    if ( ints.size() >= 2  ) {
      this->z = ints[0]; 
      this->t = ints[1]; 
      this->geometry = true;
    }
    if (ints.size() > 2) {
        this->c = ints[2];
    }
  }

  if (keyExists( "-resolution" )) {
    std::vector<double> vals = splitValueDouble( "-resolution", -1.0 );
    for (unsigned int x=0; x<4; ++x) resvals[x] = 0.0;
    for (unsigned int x=0; x<vals.size(); ++x)
      if (vals[x]<0) { 
          this->error("Incorrect resolution values!");
        exit(0);
      } else
        this->resvals[x] = vals[x];
    if (vals.size()>0) this->resolution = true;
  }

  if ( keyExists("-resize") || keyExists("-resample") ) {
    std::vector<xstring> strl;
    if (keyExists("-resize")) strl = splitValue( "-resize" );
    if (keyExists("-resample")) { strl = splitValue( "-resample" ); resample = true; }

    this->w = 0; this->h = 0;
    if ( strl.size() >= 2 ) {
      this->w = strl[0].toInt(0);
      this->h = strl[1].toInt(0);
    }
    if (strl.size()>2) {
      if ( strl[2].toLowerCase() == "nn") resize_method = Image::szNearestNeighbor;
      if ( strl[2].toLowerCase() == "bl") resize_method = Image::szBiLinear;
      if ( strl[2].toLowerCase() == "bc") resize_method = Image::szBiCubic;
    }
    if (strl.size()>3) {
      if ( strl[3].toLowerCase() == "ar") resize_preserve_aspect_ratio = true;
      if ( strl[3].toLowerCase() == "mx") { resize_preserve_aspect_ratio = true; resize_no_upsample=true; }
      if ( strl[3].toLowerCase() == "noup") { resize_preserve_aspect_ratio = true; resize_no_upsample=true; }
    }

    if (this->w<=0 || this->h<=0) resize_preserve_aspect_ratio = false;
    if (this->w>0 || this->h>0) resize = true;
  }
  
  if (keyExists("-deinterlace")) {
    this->deinterlace = true;
    xstring strl = getValue( "-deinterlace");
    if ( strl.toLowerCase() == "odd") deinterlace_method = Image::deOdd;
    if ( strl.toLowerCase() == "even") deinterlace_method = Image::deEven;
    if ( strl.toLowerCase() == "avg") deinterlace_method = Image::deAverage;
    if (strl.toLowerCase() == "offset") deinterlace_method = Image::deOffset;
  }

  if (keyExists("-transform")) {
    xstring strl = getValue( "-transform");
    if ( strl.toLowerCase() == "chebyshev") transform = Image::tmChebyshev;
    if ( strl.toLowerCase() == "fft")       transform = Image::tmFFT;
    if ( strl.toLowerCase() == "radon")     transform = Image::tmRadon;
    if ( strl.toLowerCase() == "wavelet")   transform = Image::tmWavelet;
  }

  if (keyExists("-transform_color")) {
    xstring strl = getValue( "-transform_color");
    if ( strl.toLowerCase() == "rgb2hsv") transform_color = Image::tmcRGB2HSV;
    if ( strl.toLowerCase() == "hsv2rgb") transform_color = Image::tmcHSV2RGB;
  }

  if (keyExists("-superpixels")) {
      std::vector<double> vals = splitValueDouble( "-superpixels", 0.0 );
      if (vals.size()>0) 
          superpixels = (int) vals[0];     
      if (vals.size()>1) 
          superpixels_regularization = vals[1];
  }

  if (keyExists("-filter")) {
    this->filter = true;
    xstring strl = getValue( "-filter");
    filter_method = strl.toLowerCase();
  }

  if (keyExists("-resize3d")) {
    std::vector<xstring> strl;
    if (keyExists("-resize3d")) strl = splitValue( "-resize3d" );

    this->w = 0; this->h = 0; this->z = 0;
    if ( strl.size() >= 3 ) {
      this->w = strl[0].toInt(0);
      this->h = strl[1].toInt(0);
      this->z = strl[2].toInt(0);
    }
    if (strl.size()>3) {
      if ( strl[3].toLowerCase() == "nn") resize_method = Image::szNearestNeighbor;
      if ( strl[3].toLowerCase() == "tl") resize_method = Image::szBiLinear;
      if ( strl[3].toLowerCase() == "tc") resize_method = Image::szBiCubic;
    }
    if (strl.size()>4)
      if ( strl[4].toLowerCase() == "ar") resize_preserve_aspect_ratio = true;

    if (this->w<=0 || this->h<=0) resize_preserve_aspect_ratio = false;
    if (this->w>0 || this->h>0 || this->z>0) resize3d = true;
  }

  if (keyExists("-textureatlas") || keyExists("-texturegrid")) {
      this->textureAtlas = true;
  }

  if (keyExists( "-raw" )) {
    std::vector<xstring> strl = splitValue( "-raw" );
    this->w = 0; this->h = 0; this->c = 0; this->d = 0; this->p = 0; this->e = 0;
    if ( strl.size() >= 6 ) {
      this->w = strl[0].toInt(0); 
      this->h = strl[1].toInt(0); 
      this->c = strl[2].toInt(0); 
      this->d = strl[3].toInt(0);
      this->p = strl[4].toInt(0);
      this->e = strl[5].toInt(0);
    }

    if (strl.size()>6) {
      if ( strl[6]=="int8")   raw_type = FMT_SIGNED;
      if ( strl[6]=="uint8")  raw_type = FMT_UNSIGNED;
      if ( strl[6]=="int16")  raw_type = FMT_SIGNED;
      if ( strl[6]=="uint16") raw_type = FMT_UNSIGNED;
      if ( strl[6]=="int32")  raw_type = FMT_SIGNED;
      if ( strl[6]=="uint32") raw_type = FMT_UNSIGNED;
      if ( strl[6]=="float")  raw_type = FMT_FLOAT;
      if ( strl[6]=="double") raw_type = FMT_FLOAT;
    }

    if (strl.size() > 7) {
        this->interleaved = strl[7].toBool(false);
    }

    if (this->w>0 && this->h>0 && this->c>0 && this->d>0 && this->p>0 ) raw = true;
  }

}

//------------------------------------------------------------------------------
// Output
//------------------------------------------------------------------------------

void printAbout() {
    std::cout << xstring::xprintf("\nimgcnv ver: %s\n\n", IMGCNV_VER);
    std::cout << "Author: Dima V. Fedorov <http://www.dimin.net/>" << std::endl << std::endl;
    std::cout << "Arguments: [[-i | -o] FILE_NAME | -t FORMAT_NAME ]" << std::endl << std::endl;
    std::cout << "Ex: imgcnv -i 1.jpg -o 2.tif -t TIFF" << std::endl << std::endl;
}


void printFormats() {
  FormatManager fm;
  fm.printAllFormats();
}

void printFormatsXML() {
  FormatManager fm;
  fm.printAllFormatsXML();
}

void printFormatsHTML() {
  FormatManager fm;
  fm.printAllFormatsHTML();
}

void printMetaField( const xstring &key, const xstring &val ) {
  xstring v = val.replace( "\\", "\\\\" );
  v = v.erase_zeros();
  v = v.replace( "\n", "\\" );
  v = v.replace( "\"", "'" );
  v = v.removeSpacesBoth();

  std::cout << key << ": " << v << std::endl;
}

void printMeta( MetaFormatManager *fm ) {
  const bim::TagMap metadata = fm->get_metadata();
  bim::TagMap::const_iterator it;
  for(it = metadata.begin(); it != metadata.end(); ++it) {
    xstring s = (*it).first;
    if (!s.startsWith(bim::RAW_TAGS_PREFIX) && (*it).second.size() < 1024)
        printMetaField(s, (*it).second.as_string() );
  }
}

void printTag( MetaFormatManager *fm, const std::string &key ) {
    const bim::TagMap metadata = fm->get_metadata();
    bim::TagMap::const_iterator it = metadata.find(key);
    if (it != metadata.end())
        std::cout << metadata.get_value((*it).first);
}

void printMetaParsed(MetaFormatManager *fm) {
    const bim::TagMap metadata = fm->get_metadata();
    bim::TagMap::const_iterator it;
    for (it = metadata.begin(); it != metadata.end(); ++it) {
        xstring s = (*it).first;
        if (!s.startsWith(bim::CUSTOM_TAGS_PREFIX) && !s.startsWith(bim::RAW_TAGS_PREFIX))
            printMetaField((*it).first, (*it).second.as_string());
    }
}

void printMetaCustom(MetaFormatManager *fm) {
    const bim::TagMap metadata = fm->get_metadata();
    bim::TagMap::const_iterator it;
    for (it = metadata.begin(); it != metadata.end(); ++it) {
        xstring s = (*it).first;
        if (s.startsWith(bim::CUSTOM_TAGS_PREFIX))
            printMetaField((*it).first, (*it).second.as_string());
    }
}

void printMetaRaw(MetaFormatManager *fm) {
    const bim::TagMap metadata = fm->get_metadata();
    bim::TagMap::const_iterator it;
    for (it = metadata.begin(); it != metadata.end(); ++it) {
        xstring s = (*it).first;
        if (s.startsWith(bim::RAW_TAGS_PREFIX))
            printMetaField((*it).first, (*it).second.as_string() );
    }
}


//------------------------------------------------------------------------------
// Tiles
//------------------------------------------------------------------------------

int extractTiles(DConf *c) {
    xstring input_filename = c->i_names[0];
    xstring output_path = c->o_name;
    int tile_size = c->tile_size;

    if (output_path.size() < 1) {
        c->error("You must provide output path for tile storage!");
        return IMGCNV_ERROR_NO_OUTPUT_FILE; 
    }
 
    ImagePyramid ip;
    ip.setMinImageSize( tile_size );

    // read requested page, only the first page will be used !!!!
    int page = 0;
    if (c->page.size()>0) {
        page = c->page[0];
    }
    
    if (!ip.fromFile(input_filename, page)) {
        c->error("Input format is not supported");
        return IMGCNV_ERROR_READING_FILE;
    }

    for (int l=0; l<ip.numberLevels(); ++l) {
        Image *level_img = ip.imageAt(l);
        unsigned int i=0;
        c->print( xstring::xprintf("Level: %d", l), 2 );
        for (int y=0; y<level_img->height(); y+=tile_size) {
            unsigned int j=0;
            for (int x=0; x<level_img->width(); x+=tile_size) {
                Image tile = level_img->ROI( x, y, tile_size, tile_size );
                xstring ofname = output_path;
                ofname.insertAfterLast( ".", xstring::xprintf("_%.3d_%.3d_%.3d", l, j, i) );
                if (!tile.toFile(ofname, c->o_fmt, c->options )) return IMGCNV_ERROR_WRITING_FILE;
                ++j;
            } // j
            ++i;
        } // i
    } // l

    if (c->o_histogram_file.size()>0) {
        ImageHistogram h( *ip.imageAt(0) );
        h.to( c->o_histogram_file );
    }
    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// Compositing a mosaic from a set of aligned and non overlapping tiles
//------------------------------------------------------------------------------

int mosaicTiles(DConf *c) {
    int tile_size = c->tile_size;
    int num_x = c->mosaic_num_x;
    int num_y = c->mosaic_num_y;

    if (c->o_name.size() < 1) {
        c->error("You must provide output path for tile storage!");
        return IMGCNV_ERROR_NO_OUTPUT_FILE;
    }

    Image tile(c->i_names[0]);
    if (tile.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    Image img(num_x*tile_size, num_y*tile_size, tile.depth(), tile.samples(), tile.pixelType());
    img.fill(0);

    int i = 0;
    for (int y = 0; y<img.height(); y += tile_size) {
        for (int x = 0; x<img.width(); x += tile_size) {
            c->error(xstring::xprintf("%d,%d for %s\n", x, y, c->i_names[i].c_str()));
            Image tile(c->i_names[i]);
            if (!tile.isEmpty()) {
                tile.process(c->getOperations(), 0, c);
                img.setROI(x, y, tile);
            } else {
                c->error("Tile could not be loaded");
            }
            ++i;
        } // j
    } // i

    c->print("Writing output image", 2);
    if (!img.toFile(c->o_name, c->o_fmt, c->options))
        return IMGCNV_ERROR_WRITING_FILE;

    return IMGCNV_ERROR_NONE;
}


//------------------------------------------------------------------------------
// Histogram
//------------------------------------------------------------------------------

int extractHistogram(DConf *c) {
    Image img(c->i_names[0], 0);
    if (img.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    img.process(c->getOperations(), 0, c);
    ImageHistogram hist(img);

    if (c->o_histogram_format=="xml") 
        hist.toXML( c->o_histogram_file );
    else
        hist.to( c->o_histogram_file );

    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// Pixel counts
//------------------------------------------------------------------------------

inline void write_string(std::ostream *s, const std::string &str) {
    s->write( str.c_str(), str.size() );
}

int countPixels(DConf *c) {
    Image img(c->i_names[0], 0);
    if (img.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    std::vector<uint64> counts = img.pixel_counter( c->threshold );
    uint64 sz = img.width()*img.height();

    std::ofstream f( c->o_name.c_str(), std::ios_base::binary );
    write_string(&f, "<resource>");
    for (int s=0; s<counts.size(); ++s) {
        write_string(&f, xstring::xprintf("<pixelcounts name=\"channel\" value=\"%d\">", s));
        write_string(&f, xstring::xprintf("<tag name=\"above\" value=\"%d\" />", counts[s]));
        write_string(&f, xstring::xprintf("<tag name=\"below\" value=\"%d\" />", sz-counts[s]));
        write_string(&f, "</pixelcounts>");
    }
    write_string(&f, "</resource>");

    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// 3D interpolation
//------------------------------------------------------------------------------

int resize_3d(DConf *c) {
    c->print( "About to run resize3D", 2 );
    xoperations ops = c->getOperations();
    xoperations before = ops.left("-resize3d");
    xoperations after = ops.right("-resize3d");

    ImageStack stack(c->i_names, c->c, &before);
    if (stack.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    stack.ensureTypedDepth();
    stack.resize( c->w, c->h, c->z, c->resize_method, c->resize_preserve_aspect_ratio );  
    stack.process(after, 0, c);
    stack.toFile(c->o_name, c->o_fmt, c->options );
    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// rearrange dimensions
//------------------------------------------------------------------------------

int rearrangeDimensions(DConf *c) {
    c->print( "About to run Rearrange Dimensions", 2 );
    xoperations ops = c->getOperations();
    xoperations before = ops.left("-rearrange3d");
    xoperations after = ops.right("-rearrange3d");

    ImageStack stack(c->i_names, c->c, &before);
    if (stack.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    stack.ensureTypedDepth();
    stack.process(after, 0, c);
    if (!stack.rearrange3DToFile( c->rearrange3d, c->o_name, c->o_fmt, c->options )) {
        c->error(xstring::xprintf("Cannot write into: %s\n", c->o_name.c_str()));
        return IMGCNV_ERROR_WRITING_FILE;
    }
    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// texture atlas
//------------------------------------------------------------------------------

int texture_atlas(DConf *c) {
    c->print( "About to run textureAtlas", 2 );
    xoperations ops = c->getOperations();
    xstring op = "-textureatlas";
    if (ops.contains("-texturegrid")) op = "-texturegrid";
    xoperations before = ops.left(op);
    xoperations after = ops.right(op);
    xstring arguments = ops.arguments("-texturegrid");

    ImageStack stack(c->i_names, c->c, &before);
    if (stack.isEmpty()) return IMGCNV_ERROR_READING_FILE;
    stack.ensureTypedDepth();
    Image atlas = stack.textureAtlas(arguments);
    atlas.process(after, 0, c);
    atlas.toFile(c->o_name, c->o_fmt, c->options );
    return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// overlap detection
//------------------------------------------------------------------------------
bool is_overlapping_previous( const Image &img, DConf *c ) {

  if (c->img_previous.isEmpty()) {
    c->overlap_frame_scale = 1.0;
    if (img.width()<=c->reg_max_width && img.height()<=c->reg_max_width)
      c->img_previous = img.fuseToGrayscale();
    else {
      c->img_previous = img.resample(c->reg_max_width, c->reg_max_width, Image::szBiLinear, true).fuseToGrayscale();
      c->overlap_frame_scale = (double) img.width() / (double) c->img_previous.width();
    }
    return false;
  }
  
  reg::Params rp;
  rp.numpoints = c->reg_numpoints;
  rp.transformation = reg::Affine; //enum Transformation { RST, Affine, Translation, ST, ProjectiveNS  };

  // convert images if needed
  Image image2;
  if (c->overlap_frame_scale>1)
    image2 = img.resample( (int)((double)img.width()/c->overlap_frame_scale),(int)((double)img.height()/c->overlap_frame_scale), Image::szBiLinear, true).fuseToGrayscale();
  else
    image2 = img.fuseToGrayscale();

  //c->img_previous.toFile( "G:\\_florida_video_transects\\image1.png", "png" );
  //image2.toFile( "G:\\_florida_video_transects\\image2.png", "png" );

  // register
  int res = register_image_pair(&c->img_previous, &image2, &rp);

  bool overlapping = false;

  // verify 
  if ( res==REG_OK && (rp.goodbad==reg::Good || rp.goodbad==reg::Excellent) && rp.tiePoints1.size()>4 ) {
    overlapping = true;
  } else 
  if ( res==REG_OK && rp.goodbad==reg::Uncertain && rp.rmse<3 && rp.tiePoints1.size()>4 ) {
    overlapping = true;
  }
  //int min_overlap;

  if (!overlapping) c->img_previous = image2;

  return overlapping;
}

bool read_session_pixels(MetaFormatManager *fm, Image *img, unsigned int plane, DConf *c) {
    ImageInfo info = fm->sessionGetInfo();
    if (c->res_level>0 && info.number_levels>c->res_level && c->tile_size == 0 && c->tile_x1==-1 ) {  // read image level
        ImageProxy ip(fm);
        return ip.readLevel(*img, plane, c->res_level);
    } else if (info.number_levels > c->res_level && info.tileWidth > 0 && c->tile_size > 0 && c->tile_xid >= 0) { // read image tile
        ImageProxy ip(fm);
        return ip.readTile(*img, plane, c->tile_xid, c->tile_yid, c->res_level, c->tile_size);
    } else if (info.number_levels > c->res_level && info.tileWidth > 0 && c->tile_x1 >= 0 && c->tile_y1 >= 0 && c->tile_x2 >= 0 && c->tile_y2 >= 0) { // read image tile
        ImageProxy ip(fm);
        return ip.readRegion(*img, plane, c->tile_x1, c->tile_y1, c->tile_x2, c->tile_y2, c->res_level);
    } else { // read image normally
        return fm->sessionReadImage(img->imageBitmap(), plane) == 0;
    }
}

//------------------------------------------------------------------------------
// MAIN
//------------------------------------------------------------------------------
#ifdef BIM_WIN
int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
#else
int main( int argc, char** argv ) {
#endif

  DConf conf;
  if (conf.readParams(argc, argv) != 0)  {
      printAbout(); 
      conf.print(conf.usage(), 0);
      return IMGCNV_ERROR_NONE; 
  }

  MetaFormatManager fm; // input format manager
  MetaFormatManager ofm; // output format manager
  Image img;
  Image img_projected;

  if (conf.version) { 
      conf.print(xstring::xprintf("%s\n", IMGCNV_VER), 0);
      return IMGCNV_ERROR_NONE; 
  }

  if (conf.print_formats) { 
    if (conf.print_formats_xml) printFormatsXML(); 
    else
    if (conf.print_formats_html) printFormatsHTML(); 
    else printFormats(); 
    return IMGCNV_ERROR_NONE; 
  }

  if (conf.i_names.size() <= 0) { 
      conf.error("You must provide at least one input file!");
      return IMGCNV_ERROR_NO_INPUT_FILE; 
  }

  if (conf.supported) { 
      if (fm.sessionStartRead((const bim::Filename) conf.i_names[0].c_str()) != 0) 
          conf.print("no", 0); 
      else 
          conf.print("yes", 0);
      return IMGCNV_ERROR_NONE; 
  }

  // check if format supported for writing
  if (ofm.isFormatSupportsW(conf.o_fmt.c_str()) == false) {
      conf.error(xstring::xprintf("\"%s\" Format is not supported for writing!\n", conf.o_fmt.c_str()));
      return IMGCNV_ERROR_WRITING_NOT_SUPPORTED;
  }  

  /*if (conf.o_histogram_file.size()>0 && conf.i_names.size()>0 && conf.o_name.size()<1) { 
      return extractHistogram(&conf);
  }*/

  if (conf.mosaic && conf.tile_size > 0 && conf.mosaic_num_x > 0 && conf.mosaic_num_y > 0) {
      return mosaicTiles(&conf);
  }

  if (conf.tile_size > 0 && conf.tile_xid < 0 && conf.res_level <= 0) {
      return extractTiles(&conf);
  }

  if (conf.count_pixels && conf.i_names.size()>0 && conf.o_name.size()>0) { 
      return countPixels(&conf);
  }

  if (conf.resize3d) { 
    return resize_3d(&conf);
  }

  if (conf.rearrange3d != ImageStack::adNone) { 
    return rearrangeDimensions(&conf);
  }

  if (conf.textureAtlas) { 
    return texture_atlas(&conf);
  }

  // reading OME-XML from std-in
  if (conf.loadomexml.size()>0) {
    std::string line;
    std::ifstream myfile(conf.loadomexml.c_str());
    if (myfile.is_open()) {
      while (!myfile.eof() ) {
        getline (myfile,line);
        conf.omexml += line;
      }
    }
    myfile.close();  

    conf.print( xstring::xprintf("Red OME-XML with %d characters", conf.omexml.size()), 2 );
  }


  // Load histogram if requested, it will be used later if operation needs it
  ImageHistogram hist;
  if (conf.i_histogram_file.size()>0)
    hist.from( conf.i_histogram_file );


  //----------------------------------------------------------------------
  // start conversion process
  //----------------------------------------------------------------------
  bim::uint num_pages=0;
  bim::uint page=0;

  if (conf.create) { 
    // create image from defeined params
    img.create( conf.w, conf.h, conf.d, conf.c );
    img.fill(0);
    if (img.isNull()) { 
        conf.error("Error creating new image"); 
        return IMGCNV_ERROR_CREATING_IMAGE; 
    }
    num_pages = conf.t * conf.z;
  } else
  if (conf.i_names.size()==1) { 
    
    // load image from the file, in the normal way
    if (!conf.raw) {
      if (fm.sessionStartRead((const bim::Filename)conf.i_names[0].c_str()) != 0)  {
          conf.error("Input format is not supported");
          return IMGCNV_ERROR_READING_FILE;
      }
      num_pages = fm.sessionGetNumberOfPages();
    } else {
        // if reading RAW
        if (fm.sessionStartReadRAW((const bim::Filename)conf.i_names[0].c_str(), 0, (bool)conf.e, conf.interleaved) != 0)  {
            conf.error("Error opening RAW file");
            return IMGCNV_ERROR_READING_FILE_RAW;
      }
      num_pages = conf.p;
    }

    /*
    if (conf.page.size()>0) {
      // cure pages array first, removing invalid pages and dashes
      //curePagesArray( &conf, num_pages );
      conf.curePagesArray( num_pages );
      num_pages = (bim::uint) conf.page.size();
    }*/

  } else
  if (conf.i_names.size() > 1) { 
    // multiple input files, interpret each one as pages
    num_pages = (bim::uint) conf.i_names.size();
    // if channels are also stored as separate files
    if (conf.c > 1) {
        num_pages /= conf.c;
    }
  }

  if (conf.page.size()>0) {
      // cure pages array first, removing invalid pages and dashes
      conf.curePagesArray(num_pages);
      num_pages = (bim::uint) conf.page.size();
  }

  conf.print(xstring::xprintf("Number of frames: %d", num_pages), 2);

  // check if format supports writing multipage
  if (ofm.isFormatSupportsWMP(conf.o_fmt.c_str()) == false) conf.multipage = false;

  // start writing session if multipage 
  if (conf.multipage == true && conf.o_name.size()>0) {
    if (ofm.sessionStartWrite((const bim::Filename)conf.o_name.c_str(), conf.o_fmt.c_str(), conf.options.c_str()) != 0) {
        conf.error(xstring::xprintf("Cannot write into: %s\n", conf.o_name.c_str()));
        return IMGCNV_ERROR_WRITING_FILE;
    }
  }
  int sampling_frame = 0;


  // printing info - fast method
  if (conf.print_info) {
      ImageInfo info = fm.sessionGetInfo();

      // dima: special case when appending channels from other files
      if (conf.c > 1) {
          info.samples = conf.c;
      }

      if (info.width>0) {
          conf.print(xstring::xprintf("format: %s", fm.sessionGetFormatName()), 0);
          conf.print(getImageInfoText(&info), 0);
          return 0;
      }
  }

  // printing meta - fast method
  if (conf.print_meta) {
      fm.sessionParseMetaData(0);

      // dima: special case when appending channels from other files
      if (conf.c > 1) {
          fm.set_metadata_tag(bim::IMAGE_NUM_C, (int) conf.c);
          fm.delete_metadata_tag(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0));
          fm.delete_metadata_tag(xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0));
      }

      if (conf.print_meta_parsed)
        printMetaParsed( &fm );
      else
      if (conf.print_meta_custom)
        printMetaCustom( &fm );
      else
      if (conf.raw_meta)
        printMetaRaw( &fm );
      else
      if (conf.print_tag.size()>0) 
        printTag( &fm, conf.print_tag );
      else
        printMeta( &fm );

      return 0;
  }


  // WRITE IMAGES
  for (page=0; page<num_pages; ++page) {

    if (conf.skip_frames_leading>0 && (bim::uint)conf.skip_frames_leading>page)
      continue;

    if (conf.skip_frames_trailing>0 && (bim::uint)conf.skip_frames_trailing>num_pages-page)
      continue;

    if (conf.sample_frames>0 && sampling_frame>0) {
      ++sampling_frame;
      if (sampling_frame == conf.sample_frames) sampling_frame = 0;
      continue;
    }
    ++sampling_frame;

    // if it's raw reading we have to init raw input image
    if (conf.raw) {
        img.alloc(conf.w, conf.h, conf.c, conf.d, conf.raw_type);
        img.imageBitmap()->i.number_pages = conf.p;
    }
    
    // if normal reading
    unsigned int real_frame = page;
    if (conf.page.size()>0) real_frame = conf.page[page]-1;
    if (!conf.create && conf.i_names.size() == 1) {
        read_session_pixels(&fm, &img, real_frame, &conf);
    } else if (!conf.create && conf.i_names.size() > 1) { // if multiple file reading    
      int res=0;
      
      if (conf.raw)
          res = fm.sessionStartReadRAW((const bim::Filename)conf.i_names[0].c_str(), 0, (bool)conf.e, conf.interleaved);
      else {
          if (conf.c<=1) 
              res = fm.sessionStartRead((const bim::Filename)conf.i_names[real_frame].c_str());
          else {
              int mypage = real_frame*conf.c;
              res = fm.sessionStartRead((const bim::Filename)conf.i_names[real_frame*conf.c].c_str()); // read first channel out of requested, add later
          }
      }

      if (res != 0)  {
          conf.error(xstring::xprintf("Input format is not supported for: %s\n", conf.i_names[page].c_str()));
          return IMGCNV_ERROR_READING_FILE;
      }

      // read full image or level or tile
      read_session_pixels(&fm, &img, 0, &conf);
    }

    if (img.isNull()) continue;

    conf.print(xstring::xprintf("Got image for frame: %d/%d (%.1f%%)", real_frame + 1, num_pages, (real_frame + 1)*100.0 / num_pages*1.0), 1);


    // ------------------------------------------------------------------
    // metadata
    if (page == 0) {
        fm.sessionParseMetaData(0);
    }
    img.set_metadata(fm.get_metadata());

    // ------------------------------------------------------------------

    // update image's geometry
    if (conf.geometry) {
        img.updateGeometry(conf.z, conf.t);
    }

    if (conf.resolution)
      img.updateResolution( conf.resvals );

    if ( (conf.print_info == true) && (page == 0) ) {
        conf.print(xstring::xprintf("format: %s\n", fm.sessionGetFormatName()), 0);
        conf.print(img.getTextInfo(), 0);
    }

    // print out meta-data
    if (conf.print_meta && (page == 0) ) {
      if (conf.print_meta_parsed)
        printMetaParsed( &fm );
      else
      if (conf.print_meta_custom)
        printMetaCustom( &fm );
      else
      if (conf.raw_meta)
        printMetaRaw( &fm );
      else
      if (conf.print_tag.size()>0) 
        printTag( &fm, conf.print_tag );
      else
        printMeta( &fm );
    }

    xstring ofname = conf.o_name;
    if (ofname.size() < 1 && conf.o_histogram_file.size()<1) return IMGCNV_ERROR_NO_OUTPUT_FILE;

    // make sure red image is in supported pixel format, e.g. will convert 12 bit to 16 bit
    img = img.ensureTypedDepth();

    // ------------------------------------    
    // if asked to append channels
    if (conf.c_names.size()>0) {
      conf.print( "About to append channels", 2 );
      Image ccc_img;
      for (int ccc=0; ccc<conf.c_names.size(); ++ccc) {
        //ccc_img.fromFile( conf.c_names[ccc], page );
        ccc_img.fromPyramidFile(conf.c_names[ccc], page, conf.res_level, conf.tile_xid, conf.tile_yid, conf.tile_size);
        img = img.appendChannels( ccc_img );
      }
      if (conf.i_histogram_file.size()<1) 
          hist.clear(); // dima: probably should not clear the loaded histogram
      img.delete_metadata_tag(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0));
      img.delete_metadata_tag(xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0));
    }

    // if multiple file reading with separate channels
    if (!conf.create && conf.i_names.size()>1 && conf.c>1) {
        conf.print("About to append channels", 2);
        Image ccc_img;
        for (int ccc = 1; ccc<conf.c; ++ccc) {
            //ccc_img.fromFile(conf.i_names[real_frame*conf.c + ccc], 0);
            ccc_img.fromPyramidFile(conf.i_names[real_frame*conf.c + ccc], 0, conf.res_level, conf.tile_xid, conf.tile_yid, conf.tile_size);
            img = img.appendChannels(ccc_img);
        }
        if (conf.i_histogram_file.size()<1)
            hist.clear(); // dima: probably should not clear the loaded histogram
        img.delete_metadata_tag( xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0) );
        img.delete_metadata_tag( xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0) );
    }

    //======================================================================================
    // BEGIN OPS - operations are now applied according to the position in the command line
    //======================================================================================

    img.process(conf.getOperations(), &hist, &conf);

    //======================================================================================
    // END OPS - operations are now applied according to the position in the command line
    //======================================================================================

    // write histogram file
    if (conf.o_histogram_file.size()>0) {
        ImageHistogram h(img);
        if (conf.o_histogram_format == "xml")
            h.toXML(conf.o_histogram_file);
        else
            h.to(conf.o_histogram_file);
    }

    if (ofname.size() < 1 && conf.o_histogram_file.size()>0) return IMGCNV_ERROR_NONE;
    if (ofname.size() < 1) return IMGCNV_ERROR_NO_OUTPUT_FILE;

    // ------------------------------------    
    // Project
    //if ((operation == "-project" || operation == "-projectmax" || operation == "-projectmin") && conf.project) {
    if (conf.project) {
        conf.print("About to run project", 2);
        if (img_projected.isNull())
            img_projected = img.deepCopy();
        else
        if (!conf.project_min)
            img_projected.imageArithmetic(img, Image::aoMax);
        else
            img_projected.imageArithmetic(img, Image::aoMin);
        hist.clear();
    }

    // ------------------------------------    
    // Overlapping frames detection
    if (conf.no_overlap) {
      if (is_overlapping_previous(img, &conf)) {
          conf.print(xstring::xprintf("Overlap detected, skipping frame %d", real_frame), 1);
        if (conf.overlap_frame_sampling>0)
          conf.sample_frames = conf.overlap_frame_sampling;
        continue;
      }
      conf.sample_frames = conf.sample_frames_original;
    }


    // ------------------------------------    
    // write into a file
    conf.print( "About to write", 2 );
    if (!conf.project && ofname.size()>0) {

      if ( img.get_metadata().size()>0 ) 
          ofm.sessionWriteSetMetadata( img.get_metadata() );
      if (conf.omexml.size()>0)
          ofm.sessionWriteSetOMEXML(conf.omexml);

      if (conf.multipage == true) {
         if (ofm.sessionWriteImage( img.imageBitmap(), page )>0) break;
      } else { // if not multipage
        if (num_pages > 1)
          ofname.insertAfterLast( ".", xstring::xprintf("_%.6d", real_frame+1) );

        fm.writeImage((const bim::Filename)ofname.c_str(), img.imageBitmap(), conf.o_fmt.c_str(), conf.options.c_str(), (TagMap *)img.meta());
      } // if not multipage
    } // if not projecting

    // if the image was remapped then kill the data repos, it'll have to be reinited
    if (!conf.project) {
      conf.print( "About to clear image", 2 );
      img.clear();
    }

  } // for pages


  // if we were projecting an image then create correct mapping here and save
  if (conf.project) {
    fm.writeImage ( (const bim::Filename)conf.o_name.c_str(), img_projected, conf.o_fmt.c_str(), conf.options.c_str() );
    hist.clear();
  } // if storing projected file

  fm.sessionEnd(); 
  ofm.sessionEnd();

  // Store histogram if requested
  if (conf.o_histogram_file.size()>0) {
    if (!hist.isValid()) 
        hist.fromImage( img );

    if (conf.o_histogram_format=="xml") 
        hist.toXML( conf.o_histogram_file );
    else
        hist.to( conf.o_histogram_file );
  }

  return IMGCNV_ERROR_NONE;
}

//------------------------------------------------------------------------------
// dynamic library exported function
//------------------------------------------------------------------------------
/*
template <class charT, class traits = std::char_traits<charT>>
class stringbuf : public std::basic_streambuf<charT, traits> {
public:
    using char_type = charT;
    using traits_type = traits;
    using int_type = typename traits::int_type;
public:
    stringbuf() : buffer(1000000, 0) {
        this->setp(&buffer.front(), &buffer.back());
    }

    int_type overflow(int_type c) {
        if (traits::eq_int_type(c, traits::eof()))
            return traits::not_eof(c);

        std::ptrdiff_t diff = this->pptr() - this->pbase();

        buffer.resize(buffer.size() * 1.5);
        //this->setp(&buffer.front(), &buffer.back());
        this->setp(&this->buffer.front(), &this->buffer.front() + this->buffer.size());

        this->pbump(diff);
        *this->pptr() = traits::to_char_type(c);
        this->pbump(1);

        return traits::not_eof(traits::to_int_type(*this->pptr()));
    }

    std::basic_string<charT> str() const {
        return this->buffer.substr(0, this->pptr() - this->pbase());
    }

private:
    std::basic_string<charT> buffer;
};

//stringbuf<char> buf;
//std::ostream buffer(&buf);
*/

void handle_exception() {
    try {
        throw;
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
    }
    catch (const int i) {
        std::cerr << i << "\n";
    }
    catch (const long l) {
        std::cerr << l << "\n";
    }
    catch (const char *p) {
        std::cerr << p << "\n";
    }
    catch (...) {
        std::cerr << "unknown excepition\n";
    }
}

extern "C" {
#ifdef BIM_WIN
int imgcnv(int argc, wchar_t *argv[], char **out) {
    try {
        std::ostringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

        int retcode = wmain(argc, argv, NULL);

        std::string text = buffer.str();
        if (text.size() > 0) {
            *out = new char[text.size() + 1];
            memcpy(*out, &text[0], text.size());
            (*out)[text.size()] = 0;
        }
        std::cout.rdbuf(old);
        return retcode;
    }
    catch (...) {
        handle_exception();
        return 101;
    }
}
#else
    int imgcnv(int argc, char** argv, char **out) {
    try {
        std::ostringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

        int retcode = main(argc, argv);

        std::string text = buffer.str();
        if (text.size() > 0) {
            *out = new char[text.size() + 1];
            memcpy(*out, &text[0], text.size());
            (*out)[text.size()] = 0;
        }
        std::cout.rdbuf(old);
        return retcode;
    } catch (...) {
        handle_exception();
        return 101;
    }
}
#endif

void imgcnv_clear(char **out) {
    try {
        if (*out) delete[] * out;
        *out = NULL;
    } catch (...) {

    }
}

}
