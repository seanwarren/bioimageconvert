/*******************************************************************************

  libBioIMage Format Interface version: 2.0
  
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
    Add: readMetaDataAsText

  History:
    03/23/2004 18:03 - First creation
    09/12/2005 17:06 - updated API to v1.3
    12/01/2005 16:24 - updated API to v1.4
    2008-04-04 14:11 - updated API to v1.6
    2009-06-29 16:21 - updated API to v1.7
    2010-01-25 16:45 - updated API to v1.8
    2012-01-01 16:45 - updated API to v2.0

  ver: 20
        
*******************************************************************************/

#ifndef BIM_IMG_FMT_IFC_H
#define BIM_IMG_FMT_IFC_H

#include <cstdio>

#include <xtypes.h>

namespace bim {

static const int NumberDisplayChannels = 7;
// channel to xRGB mapping, used to indicate which image channel maps to which display color
// 0:R, 1:G, 2:B, 3:R+G=Yellow, 4:R+B=Purple, 5:B+G=Cyan, 6:R+B+G=Gray 
enum DisplayChannels { Red=0, Green=1, Blue=2, Yellow=3, Magenta=4, Cyan=5, Gray=6 };

struct DisplayColor {
    int r, g, b;
    DisplayColor(): r(-1), g(-1), b(-1) {}
    DisplayColor(int r, int g, int b) { this->r=r; this->g=g; this->b=b; }
    bool isZero() const { return (this->r==0 && this->g==0 && this->b==0); }
    bool isCustom() const { return ((this->r>0 && this->r<255) || (this->g>0 && this->g<255) || (this->b>0 && this->b<255)); }
    bool operator==(DisplayColor& c) const { return (this->r==c.r && this->g==c.g && this->b==c.b); }
};


// HERE DEFINED DATA TYPES IN CONFORMANCE WITH TIFF STRUCTURE
// This types are used in tiff tags andmany independent structures
// like EXIF, let's stick to the it too
typedef enum {
  TAG_NOTYPE    = 0,  // placeholder
  TAG_BYTE      = 1,  // 8-bit unsigned integer
  TAG_ASCII     = 2,  // 8-bit bytes w/ last byte null
  TAG_SHORT     = 3,  // 16-bit unsigned integer
  TAG_LONG      = 4,  // 32-bit unsigned integer
  TAG_RATIONAL  = 5,  // 64-bit unsigned fraction
  TAG_SBYTE     = 6,  // 8-bit signed integer
  TAG_UNDEFINED = 7,  // 8-bit untyped data
  TAG_SSHORT    = 8,  // 16-bit signed integer
  TAG_SLONG     = 9,  // 32-bit signed integer
  TAG_SRATIONAL = 10, // 64-bit signed fraction
  TAG_FLOAT     = 11, // 32-bit IEEE floating point
  TAG_DOUBLE    = 12  // 64-bit IEEE floating point
} DataType;

// Data storage format used along with pixel depth (BPP)
typedef enum {
  FMT_UNDEFINED   = 0,  // placeholder type
  FMT_UNSIGNED    = 1,  // unsigned integer
  FMT_SIGNED      = 2,  // signed integer
  FMT_FLOAT       = 3   // floating point
} DataFormat;

// modes are declared similarry to Adobe Photoshop, but there are differences
typedef enum {
  IM_BITMAP    = 0,  // 1-bit
  IM_GRAYSCALE = 1,  // 8-bit
  IM_INDEXED   = 2,  // 8-bit
  IM_RGB       = 3,  // 24-bit
  IM_BGR       = 4,  // 24-bit
  IM_HSL       = 5,  // 24-bit
  IM_HSV       = 6,  // 24-bit
  IM_RGBA      = 7,  // 32-bit
  IM_ABGR      = 8,  // 32-bit
  IM_CMYK      = 9,  // 32-bit
  IM_MULTI     = 12  // undefined - image with many separate grayscale channels
} ImageModes;

typedef enum {
  IO_READ   = 0,
  IO_WRITE  = 1
} ImageIOModes;

// this strange numbering is due to be equal to TIFF metric units
typedef enum {
  RES_m   = 0, // Meters = 0.0254 of inch
  RES_NONE= 1, // inches
  RES_IN  = 2, // inches
  RES_cm  = 3, // Centi Meters 10^^-2 = 2.54 of inch
  RES_mm  = 4, // Mili  Meters 10^^-3
  RES_um  = 5, // Micro Meters 10^^-6
  RES_nm  = 6, // Nano  Meters 10^^-9
  RES_Gm  = 7, // Giga  Meters 10^^9  
  RES_Mm  = 8, // Mega  Meters 10^^6
  RES_km  = 9, // Kilo  Meters 10^^3
  RES_hm  = 10 // Hecto Meters 10^^2
} ResolutionUnits;

//------------------------------------------------------------------------------
// exemplar structures, you might use anything different
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// PALETTE
//------------------------------------------------------------------------------

typedef uint32 RGBA;
typedef struct LUT {
  uint count;
  RGBA rgba[256]; 
} LUT;

inline int xR( RGBA rgb )   
{ return (int)((rgb >> 16) & 0xff); }

inline int xG( RGBA rgb )   
{ return (int)((rgb >> 8) & 0xff); }

inline int xB( RGBA rgb )   
{ return (int)(rgb & 0xff); }

inline int xA( RGBA rgb )   
{ return (int)((rgb >> 24) & 0xff); }

inline RGBA xRGB( int r, int g, int b )
{ return (0xff << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

inline RGBA xRGBA( int r, int g, int b, int a )
{ return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

inline int xGray( int r, int g, int b )
{ return (r*11+g*16+b*5)/32; }

inline int xGray( RGBA rgb )
{ return xGray( xR(rgb), xG(rgb), xB(rgb) ); }


//------------------------------------------------------------------------------
// DIMENSIONS v1.4
// The default dimension order is: XYCZT
// each image MUST have XYC dimenstions as first in the list!!!
// C MUST be third dimention in order for the paged approach to work
// image is constituted of pages with XYC dimentions
// then the sequence of pages is interpreted as Z, T, ZT
//------------------------------------------------------------------------------

#define BIM_MAX_DIMS 10

typedef enum {
  DIM_0 = 0,  
  DIM_X = 1,
  DIM_Y = 2,
  DIM_C = 3,
  DIM_Z = 4,
  DIM_T = 5
} ImageDims;

typedef struct Dimension {
  ImageDims dim;
  //uint      resUnits;     // resolution units defined by: BIM_ResolutionUnits
  //float64    res;         // pixels per unit
  char *description;
  void *ext;
} Dimension;

//------------------------------------------------------------------------------
// CHANNELS v1.4
// Just a description of each channel, usually: R, G, B...
//------------------------------------------------------------------------------

#define BIM_MAX_CHANNELS 512

typedef struct Channels {
  char *description;
  void *ext;
} Channels;

//------------------------------------------------------------------------------
// IMAGE
//------------------------------------------------------------------------------

typedef struct ImageInfo {
  uint32      ver;           // must be == sizeof(ImageInfo)
  
  uint64      width;
  uint64      height;
  
  uint64      number_pages;   // number of images within the file
  uint64      number_levels;  // v1.1 number of levels in resolution pyramid
  
  // interpretative parameters, not needed for actual image decoding
  uint64      number_t;       // v1.4 number of time points
  uint64      number_z;       // v1.4 number of z depth points
  uint32      number_dims;    // v1.4 number of dimensions
  Dimension   dimensions[BIM_MAX_DIMS]; // v1.4 dimensions description
  Channels    channels[BIM_MAX_CHANNELS]; // v1.4 channels description
  uint32      file_format_id; // v1.4 id for the file format used

  RGBA        transparentIndex;
  RGBA        transparencyMatting;  // if A == 0: no matte
    
  uint32      imageMode;    // Image mode as declared in bim::ImageModes
  uint32      samples;      // Samples per pixel
  uint32      depth;        // Bits per sample, currently must be 8 or 16
  DataFormat  pixelType;    // type in which the pixels are stored, changed in v1.8
                              // related to depth but differentiate signed/unsigned/floats etc. 
  DataType    rowAlignment; // type to which each row is aligned v1.5

  LUT         lut;          // LUT, only used for Indexed Color images  

  uint32      resUnits;     // resolution units defined by: BIM_ResolutionUnits
  float64     xRes;         // pixels per unit
  float64     yRes;         // pixels per unit
  
  uint64      tileWidth;    // The width of the tiles. Zero if not set
  uint64      tileHeight;   // The height of the tiles. Zero if not set  
                      
} ImageInfo;

// here we should store samples like this: 0, 1, 2...
// sample meaning is defined by imageMode in ImageInfo
typedef struct ImageBitmap {
  ImageInfo i;
  void *bits[BIM_MAX_CHANNELS];  // pointer to RAW data by samples i.e. plane by plane, now restricted to 512 planes
} ImageBitmap;


//------------------------------------------------------------------------------
//  META DATA
//------------------------------------------------------------------------------
typedef enum {
  META_TIFF_TAG = 0,  // any tiff tag
  META_EXIF     = 1,  // whole EXIF buffer
  META_IPTC     = 2,  // whole IPTC buffer
  META_GEO_TIFF = 3,  // whole GeoTiff buffer
  META_BIORAD   = 4,  // any of BioRad PIC tag
  META_STK      = 5,  // text formated STK tags
  META_PNG      = 6,  // any of PNG text notes
  META_GENERIC  = 7   // OME tags: metadata hash or the whole OME XML
} MetaGroups;

typedef struct TagItem {
  uint32  tagGroup;  // group to which tag's ID pertense, e.g. TIFF, EXIF, BioPIC... value of (BIM_MetaGroups)
  uint32  tagId;     // tag ID: e.g. TIFF tag number
  uint32  tagType;   // type: format in which data is stored inside data buffer: value of DataType
  uint32  tagLength; // how many records of type is stored in data buffer
  void   *tagData;
} TagItem;

typedef struct TagList {
  uint32    count;
  TagItem  *tags;
} TagList;

typedef enum {
  METADATA_TAGS    = 0,  // points to TagMap
  METADATA_OMEXML  = 1   // points to the whole XML as a std::string
} GENERIC_MetaTags;

//******************************************************************************
// interface wide data type macros, define them using data types you desire
//******************************************************************************

class TagMap; // v1.7 the class for textual metadata storage

// some macros for data types used by interface
#define BIM_IMAGE_CLASS         ImageBitmap
#define BIM_STRING_CLASS        const char

//#define BIM_STREAM_CLASS        FILE
#define BIM_STREAM_CLASS        void

#define BIM_PARAM_STREAM_CLASS  FILE
#define BIM_INTPARAMS_CLASS     void
#define BIM_PARENT_WIN_CLASS    void
#define BIM_DIALOG_WIN_CLASS    void
#define BIM_MAGIC_STREAM        void             // could be stream or just memory buffer...
#define BIM_IMG_SERVICES        void             // by now it's empty
#define BIM_FORMAT_ID           uint32       // this number is a number in FormatList

#define BIM_OPTIONS_CLASS       char
#define BIM_METADATA_CLASS      TagMap          // v1.7

#define BIM_SIZE_T uint64
#define BIM_OFFSET_T int64


//******************************************************************************
// plug-in receves from the host program
//******************************************************************************

//-----------------------------------------------------
// generic functions that host provides
//-----------------------------------------------------
// host shows progress of the executon
// done  - value in a range of [0..total]
// total - range maximum
// descr - any text describing the executed procedure
typedef void (*ProgressProc)(uint64 done, uint64 total, char *descr);

// host shows error of the executon
// val   - error code, not treated at the moment
// descr - any text describing the error
typedef void (*ErrorProc)(int val, char *descr);

// plugin should use this to test is processing should be interrupted...
// If it returns true, the operation should be aborted 
typedef int (*TestAbortProc)( void );


//-----------------------------------------------------
// memory allocation prototypes, if these are null then 
// formats should use standard way of memory allocation
// using malloc...
//-----------------------------------------------------
typedef void* (*MallocProc) (uint64 size);
typedef void* (*FreeProc)   (void *p);

//-----------------------------------------------------
// io prototypes, if these are null then formats should 
// use standard C io functions: fwrite, fread...
//-----------------------------------------------------

// (fread)  read from stream  
typedef uint64 (*ReadProc)  ( void *buffer, uint64 size, uint64 count, BIM_STREAM_CLASS *stream );
              
// (fwrite) write into stream 
typedef uint64 (*WriteProc) ( void *buffer, uint64 size, uint64 count, BIM_STREAM_CLASS *stream );

// (flush) flushes a stream
typedef int    (*FlushProc) ( BIM_STREAM_CLASS *stream );

// (fseek)  seek within stream
typedef int    (*SeekProc)  ( BIM_STREAM_CLASS *stream, int64 offset, int origin );

// get file "stream" size (libtiff askes for it)
typedef uint64 (*SizeProc)  ( BIM_STREAM_CLASS *stream );

// (ftell) gets the current position in stream
typedef int64  (*TellProc)  ( BIM_STREAM_CLASS *stream );

// (feof) tests for end-of-file on a stream
typedef int    (*EofProc)   ( BIM_STREAM_CLASS *stream );

// (feof) tests for end-of-file on a stream
typedef int    (*CloseProc) ( BIM_STREAM_CLASS *stream );

//-----------------------------------------------------


//******************************************************************************
// structure passed by host to a format function
//******************************************************************************

typedef char* Filename;

typedef struct FormatHandle {
  
  uint32 ver; // must be == sizeof(FormatHandle)
  
  // IN
  ProgressProc      showProgressProc;    // function provided by host to show plugin progress
  ErrorProc         showErrorProc;       // function provided by host to show plugin error
  TestAbortProc     testAbortProc;       // function provided by host to test if plugin should interrupt processing
  
  MallocProc        mallocProc;          // function provided by host to allocate memory
  FreeProc          freeProc;            // function provided by host to free memory

  // some standard parameters are defined here, any specific goes inside internalParams
  BIM_MAGIC_STREAM      *magic;
  BIM_FORMAT_ID         subFormat;           // sub format used to read or write, same as FormatItem
  uint32            pageNumber;          // page number to retreive/write, if number is invalid thenpage 0 is retreived
  uint32            resolutionLevel;     // v1.1 resolution level retreive/write, considered 0 (highest) as default
  uchar             quality;             // number between 0 and 100 to specify quality (read/write)
  uint32            compression;         // 0 - specify lossless method
  uint32            order;               // progressive, normal, interlaced...
  uint32            roiX;                // v1.1 Region Of Interest: Top Left X (pixels), ONLY used for reading!!!
  uint32            roiY;                // v1.1 Region Of Interest: Top Left Y (pixels), ONLY used for reading!!!
  uint32            roiW;                // v1.1 Region Of Interest: Width (pixels), ONLY used for reading!!!
  uint32            roiH;                // v1.1 Region Of Interest: Height (pixels), ONLY used for reading!!!
  TagList           metaData;            // meta data tag list, ONLY used for writing, returned NULL while reading!!!
  
  BIM_IMG_SERVICES      *imageServicesProcs; // The suite of image processing services callbacks  
  BIM_INTPARAMS_CLASS   *internalParams;     // internal plugin parameters
  
  BIM_PARENT_WIN_CLASS  *parent;             // pointer to parent window
  void                  *hDllInstance;       // handle to loaded dynamic library instance (plug-in)

  bim::Filename         fileName;           // file name
  BIM_STREAM_CLASS      *stream;             // pointer to a file stream that might be open by host
  ImageIOModes      io_mode;             // v1.2 io mode for opened stream 
  // i/o callbacks
  ReadProc          readProc;            // v1.2 (fread)  read from stream
  WriteProc         writeProc;           // v1.2 (fwrite) write into stream 
  FlushProc         flushProc;           // v1.2 (flush) flushes a stream
  SeekProc          seekProc;            // v1.2 (fseek)  seek within stream
  SizeProc          sizeProc;            // v1.2 () get file "stream" size (libtiff askes for it)
  TellProc          tellProc;            // v1.2 (ftell) gets the current position in stream
  EofProc           eofProc;             // v1.2 (feof) tests for end-of-file on a stream
  CloseProc         closeProc;           // v1.2 (fclose) close stream

  // IN/OUT
  BIM_IMAGE_CLASS       *image;              // pointer to an image structure to read/write
  BIM_OPTIONS_CLASS     *options;            // v1.6 pointer to encoding options string
  //BIM_METADATA_CLASS    *hash;               // v1.7 pointer to TagMap for textual metadata

  void *param1; // reserved
  void *param2; // reserved
  void *param3; // reserved  

  char reserved[52];
  
} FormatHandle;
//-----------------------------------------------------


//******************************************************************************
// plug-in provides to the host program
//******************************************************************************

//------------------------------------------------------------------------------
//  FORMAT
//------------------------------------------------------------------------------

// in all restrictions 0 means no restriction
typedef struct FormatConstrains {
  uint32  maxWidth;
  uint32  maxHeight;
  uint32  maxPageNumber;
  uint32  minSamplesPerPixel; // v1.3
  uint32  maxSamplesPerPixel; // v1.3
  uint32  minBitsPerSample;   // v1.3
  uint32  maxBitsPerSample;   // v1.3
  bool    lutNotSupported;    // v1.3
} FormatConstrains;

typedef struct FormatItem {
  char *formatNameShort; // short name, no spaces
  char *formatNameLong;  // Long format name
  char *extensions;      // pipe "|" separated supported extension list
  bool canRead;           // 0 - NO, 1 - YES v1.3
  bool canWrite;          // 0 - NO, 1 - YES v1.3
  bool canReadMeta;       // 0 - NO, 1 - YES v1.3
  bool canWriteMeta;      // 0 - NO, 1 - YES v1.3
  bool canWriteMultiPage; // 0 - NO, 1 - YES v1.3
  FormatConstrains constrains; //  v1.3
  char reserved[17];
} FormatItem;

typedef struct FormatList {
  uint32        version;
  uint32        count;
  FormatItem    *item;
} FormatList;

//------------------------------------------------------------------------------
// NULL for any of this function means unavailable function 
//   ex: only reader shuld return NULL for all reader functions
//------------------------------------------------------------------------------

// this wil return if image can be opened, v 1.9
typedef int (*ValidateFormatProc) (BIM_MAGIC_STREAM *magic, uint length, const bim::Filename fileName);

//------------------------------------------------------------------------------
// FORMAT PARAMS and ALLOCATION
//------------------------------------------------------------------------------

// allocate space for parameters stuct and return it
typedef FormatHandle (*AquireFormatProc) ();

// delete parameters struct
typedef void (*ReleaseFormatProc) (FormatHandle *fmtHndl);

// return dialog window class for parameter initialization or create the window by itself
typedef BIM_DIALOG_WIN_CLASS* (*AquireIntParamsProc) (FormatHandle *fmtHndl);

// load/save parameters into stream
typedef void (*LoadFormatParamsProc)  (FormatHandle *fmtHndl, BIM_PARAM_STREAM_CLASS *stream);
typedef void (*StoreFormatParamsProc) (FormatHandle *fmtHndl, BIM_PARAM_STREAM_CLASS *stream);

//------------------------------------------------------------------------------
// OPEN/CLOSE IMAGE PROCs
//------------------------------------------------------------------------------

typedef uint (*OpenImageProc)  (FormatHandle *fmtHndl, ImageIOModes io_mode);
typedef uint (*FOpenImageProc) (FormatHandle *fmtHndl, Filename fileName, ImageIOModes io_mode);
typedef uint (*IOpenImageProc) (FormatHandle *fmtHndl, Filename fileName, 
                                         BIM_IMAGE_CLASS *image, ImageIOModes io_mode);

typedef void     (*CloseImageProc) (FormatHandle *fmtHndl);

//------------------------------------------------------------------------------
// IMAGE INFO PROCs
//------------------------------------------------------------------------------

// image info procs
typedef uint      (*GetNumPagesProc)  (FormatHandle *fmtHndl);
// image info for each page proc
typedef ImageInfo (*GetImageInfoProc) (FormatHandle *fmtHndl, uint page_num);


//------------------------------------------------------------------------------
// READ/WRITE PROCs
//------------------------------------------------------------------------------

// these are the actual read/write procs
typedef uint (*ReadImageProc)  (FormatHandle *fmtHndl, uint page);
typedef uint (*WriteImageProc) (FormatHandle *fmtHndl);

typedef uint(*ReadImageTileProc)    (FormatHandle *fmtHndl, uint page, uint64 xid, uint64 yid, uint level); // v2.0, modified 
typedef uint(*WriteImageTileProc)   (FormatHandle *fmtHndl, uint page, uint64 xid, uint64 yid, uint level); // v2.0, modified 

typedef uint(*ReadImageLevelProc)    (FormatHandle *fmtHndl, uint page, uint level); // v2.0, redesignated
typedef uint(*WriteImageLevelProc)   (FormatHandle *fmtHndl, uint page, uint level); // v2.0, redesignated

// difference with preview is that if there's a thumbnail in the image file
// then it will be upscaled/downscaled to meet w and h...
typedef uint (*ReadImageThumbProc)   (FormatHandle *fmtHndl, uint w, uint h);
typedef uint (*WriteImageThumbProc)  (FormatHandle *fmtHndl);

// difference with thumbnail is that if there's a thumbnail in the image file
// smaller then needed preview then actual image will be downscaled or to meet w and h,
// if image contains Resolution Levels they'll be used to retreive optimum data
// roi will be used as weel if defined in fmtHndl, if W and H are defined as 0 
// then ROI will be extracted
typedef uint (*ReadImagePreviewProc) (FormatHandle *fmtHndl, uint w, uint h);


//------------------------------------------------------------------------------
// METADATA PROCs
//------------------------------------------------------------------------------

// if cannot read any meta then return value different form 0
// any possible group = -1, any possible tag = -1
// so to read all possible metadata pass -1 as group and tag
// if tag or group are provided but not present in the file then return 2
typedef uint (*ReadMetaDataProc)    ( FormatHandle *fmtHndl, uint page, int group, int tag, int type);

// a simplistic approach to metadata retrieval, return null terminated string
// with possible new line caracters OR NULL if no data could be found
typedef char* (*ReadMetaDataAsTextProc) ( FormatHandle *fmtHndl );

// if cannot write requested meta then return value different form 0
// this will only work on formats allowing adding meta data into the file
// usually it should writen using WriteImageProc and meta data struct inside fmtHndl
typedef uint (*AddMetaDataProc)     ( FormatHandle *fmtHndl);

// v1.7 New format metadata appending function, that should append fields 
// directly into provided HashTable, this allows performing all parsing on the reader side
typedef uint (*AppendMetaDataProc)  ( FormatHandle *fmtHndl, BIM_METADATA_CLASS *hash);


//------------------------------------------------------------------------------
// *** HEADER
// structure that plug-in should provide for the host program
//------------------------------------------------------------------------------

typedef struct FormatHeader {
  uint  ver;          // must be == sizeof(FormatHeader)
  char *version;     // plugin version, e.g. "1.0.0"
  char *name;        // plugin name
  char *description; // plugin description
  
  uint                    neededMagicSize; // 0 or more, specify number of bytes needed to identify the file
  FormatList              supportedFormats;

  ValidateFormatProc      validateFormatProc; // return true if can handle file
  // begin
  AquireFormatProc        aquireFormatProc;
  // end
  ReleaseFormatProc       releaseFormatProc;
  
  // params
  AquireIntParamsProc     aquireParamsProc;
  LoadFormatParamsProc    loadParamsProc;
  StoreFormatParamsProc   storeParamsProc;

  // image begin
  OpenImageProc           openImageProc; 
  CloseImageProc          closeImageProc; 

  // info
  GetNumPagesProc         getNumPagesProc;
  GetImageInfoProc        getImageInfoProc;


  // read/write
  ReadImageProc           readImageProc; 
  WriteImageProc          writeImageProc;
  ReadImageTileProc       readImageTileProc; // v2.0, modified
  WriteImageTileProc      writeImageTileProc; // v2.0, modified
  ReadImageLevelProc      readImageLevelProc; // v2.0, redesignated
  WriteImageLevelProc     writeImageLevelProc; // v2.0, redesignated
  ReadImageThumbProc      readImageThumbProc;
  WriteImageThumbProc     writeImageThumbProc;
  ReadImagePreviewProc    readImagePreviewProc;
  
  // meta data
  ReadMetaDataProc        readMetaDataProc;
  AddMetaDataProc         addMetaDataProc;
  ReadMetaDataAsTextProc  readMetaDataAsTextProc;
  AppendMetaDataProc      appendMetaDataProc; // v1.7

  void *param1;       // reserved
  void *param2;       // reserved
  char reserved[100];
} FormatHeader;


// -----------------------------------------------------------------------------
// function prototypes
// -----------------------------------------------------------------------------

// This functions must be exported so plug-in could be accepted as valid by host
typedef FormatHeader* (*GetFormatHeader)(void);

} // namespace bim

#endif //BIM_IMG_FMT_IFC_H
