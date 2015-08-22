/*****************************************************************************
  Olympus Image Binary (OIB) format support
  Copyright (c) 2008, Center for Bio Image Informatics, UCSB
  
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2008-06-04 14:26:14 - First creation
    2008-09-15 19:04:47 - Fix for older files with unordered streams    
    2008-11-06 13:36:43 - Parse preferred channel mapping

  Ver : 4
*****************************************************************************/

#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <tiffio.h>

// libtiff 3.9.4
#if (TIFFLIB_VERSION <= 20100615)
typedef tsize_t tiff_size_t;
typedef tdata_t tiff_data_t;
typedef toff_t  tiff_offs_t;
#endif

// libtiff 4.0.X
#if (TIFFLIB_VERSION >= 20100101)
typedef tmsize_t tiff_size_t;
typedef void*    tiff_data_t;
typedef uint64   tiff_offs_t;
#endif

#include <bim_img_format_interface.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include <pole.h>

#include "bim_ole_format.h"
#include "oib.h"

using namespace bim;

//****************************************************************************
// Tiff callbacks
//****************************************************************************

static tiff_size_t stream_tiff_read(thandle_t handle, tiff_data_t data, tiff_size_t size) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  POLE::Stream *stream = (POLE::Stream *) fmtHndl->stream;
  return (tiff_size_t) stream->read( (unsigned char *) data, size );
}

static tiff_size_t stream_tiff_write(thandle_t /*handle*/, tiff_data_t /*data*/, tiff_size_t /*size*/) {
  return 0;
}

static tiff_offs_t stream_tiff_seek(thandle_t handle, tiff_offs_t offset, int whence) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  POLE::Stream *stream = (POLE::Stream *) fmtHndl->stream;
  tiff_size_t off = offset;
  if (whence == SEEK_CUR) off = stream->tell() + offset;
  if (whence == SEEK_END) off = stream->size() + offset - 1;
  stream->seek( off );
  return stream->tell();
}

static int stream_tiff_close(thandle_t handle) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  POLE::Stream *stream = (POLE::Stream *) fmtHndl->stream;
  return 0;
}

static tiff_offs_t stream_tiff_size(thandle_t handle) {
  FormatHandle *fmtHndl = (FormatHandle *) handle;
  POLE::Stream *stream = (POLE::Stream *) fmtHndl->stream;
  return stream->size();
}

static int stream_tiff_mmap(thandle_t /*handle*/, tiff_data_t* /*data*/, tiff_offs_t* /*size*/) {
  return 1;
}

static void stream_tiff_unmap(thandle_t /*handle*/, tiff_data_t /*data*/, tiff_offs_t /*size*/) {
}

//****************************************************************************
// MISC
//****************************************************************************

bool oib::Axis::isValid() const {
  if (MaxSize<=0) return false;
  if (StartPosition == EndPosition) return false;
  return true;
}

//****************************************************************************
// MISC
//****************************************************************************

int read_stream_as_wstring( POLE::Storage *storage, const char *name, std::wstring &str ) {
 
  POLE::Stream *stream = new POLE::Stream( storage, name );
  if (!stream || stream->size() <= 0) return 1;
  if( stream->fail() ) return 1;
  str.resize( stream->size() );
  POLE::uint64 red = stream->read( (unsigned char *) &str[0], stream->size() );
  delete stream;
  if (red != str.size()) return 1;
  return 0;
}

int read_stream_as_string( POLE::Storage *storage, const char *name, std::string &str ) {
  std::wstring wstr;
  int res = read_stream_as_wstring( storage, name, wstr );
  if (res!=0) return res;

  size_t size = wstr.size()-1;
  str.resize( size );
  char *p = (char *) &wstr[0];
  p+=2;

  for( unsigned i=0; i<size; ++i ) {
    str[i] = *p;
    p+=2;
  }
  
  return 0;
}


//****************************************************************************
// LUTs
//****************************************************************************
/*
LUT file will be named as “xxx_LUTX.lut”
Where, X is the channel number
*/
xstring get_lut_stream_key( oib::Params *par, int sample ) {
  xstring stream_name = par->oifFolderName;
  xstring part;
  part.sprintf("_LUT%d.lut", sample+1);

 // v 1.0.0.0 - Stream00022=Storage00001/FILENAME_LUT1.lut
  xstring key_name = par->oib_info_hash.get_key( stream_name + "/" + par->oifFileName + part );
  
  // v 2.0.0.0 - Stream00022=Storage00001/s_LUT1.lut
  if (key_name.size()<=0)
    key_name = par->oib_info_hash.get_key( stream_name + "/s" + part );

  //v X.X.X.X slow gessing if we did not find the stream, hope that _LUT1.lut construct will be found
  if (key_name.size()<=0)
    key_name = par->oib_info_hash.get_key_where_value_endsWith( part );

  return key_name;
}

TagMap parse_lut_stream( ole::Params *par, int sample ) {
  std::string info_header("OibSaveInfo/");
  xstring stream_name = get_lut_stream_key( &par->oib_params, sample );

  //strip initial "OibSaveInfo/" from the key
  stream_name.erase( 0, info_header.size() );
  stream_name = "/" + par->oib_params.oifFolderName + "/" + stream_name;

  std::string lut_data;
  TagMap lut_info;
  if (read_stream_as_string( par->storage, stream_name.c_str(), lut_data )!=0) return lut_info;
  lut_info.parse_ini( lut_data, "=", "", "ColorLUTData" );
  return lut_info;
}

//****************************************************************************
// CHANNELS
//****************************************************************************
/*
XYZ file will be named as “xxx_C00mZ00n.tif”
XYZT file will be named as “xxx_C00mZ00nT00p.tif”
XYLZT file will be named as “xxx_C00mL00qZ00nT00p.tif”
Where, C00m shows Channel No.m, Z00n shows Z Slice No. n, L00q shows Lambda Slice No.q, T00p shows Time Slice No.p.
*/
std::string get_channel_stream_name( oib::Params *par, int page, int sample ) {
  int z=0, t=0, l=0;
  int nz = std::max(par->num_z, 1);
  int nt = std::max(par->num_t, 1);
  int nl = std::max(par->num_l, 1);
  xstring stream_name, part;

  // get proper z, t and l from page number that's in order ZT
  l = (page/nz)/nt;
  t = (page - l*nt*nz) / nz;
  z = page - nt*l - nz*t;

  // create filename
  //stream_name += par->oifFolderName;
  //stream_name += "/";
  //stream_name += par->oifFileName;
  //stream_name += "_";
  
  // Order here DOES MATTER!!!!
  // channel
  part.sprintf("C%.3d", sample+1);
  stream_name += part;
  // L
  if (par->num_l>0) {
    part.sprintf("L%.3d", l+1);
    stream_name += part;
  }
  // Z
  if (par->num_z>0) {
    part.sprintf("Z%.3d", z+1);
    stream_name += part;
  }
  // T
  if (par->num_t>0) {
    part.sprintf("T%.3d", t+1);
    stream_name += part;
  }
  stream_name += ".tif";

  return stream_name;
}


//****************************************************************************
// INTERNAL STRUCTURES
//****************************************************************************

int oibGetImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  ole::Params *olePar = (ole::Params *) fmtHndl->internalParams;
  oib::Params *par = &olePar->oib_params;
  ImageInfo *info = &olePar->i; 
  
  info->ver = sizeof(ImageInfo);
  info->imageMode = IM_GRAYSCALE;
  info->tileWidth = 0;
  info->tileHeight = 0; 
  info->transparentIndex = 0;
  info->transparencyMatting = 0;
  info->lut.count = 0;

  // load first OibInfo.txt
  std::string oib_info;  
  if (read_stream_as_string( olePar->storage, "/OibInfo.txt", oib_info )!=0) return 1;
  TagMap oib_info_hash;
  oib_info_hash.parse_ini( oib_info );

  std::string MainFileName = std::string() + "/" + oib_info_hash.get_value("OibSaveInfo/MainFileName");
  par->oifFolderName = oib_info_hash.get_value("OibSaveInfo/ThumbFolderName");
  par->oifFileName = oib_info_hash.get_value("OibSaveInfo/" + oib_info_hash.get_value("OibSaveInfo/MainFileName"));
  if (par->oifFileName.size()>4) par->oifFileName.resize( par->oifFileName.size()-4 );
  par->oib_info_hash = oib_info_hash;

  // load MainFileName and parse all metadata
  if (read_stream_as_string( olePar->storage, MainFileName.c_str(), par->oif_metadata )!=0) return 1;
  par->oif_metadata_hash.parse_ini( par->oif_metadata );

  xstring axis_name;
  int axis_count = par->oif_metadata_hash.get_value_int( "Axis Parameter Common/AxisCount", 0 );
  for (int i=0; i<8; ++i) {
    par->axis.push_back( oib::Axis() );
    axis_name.sprintf("Axis %d Parameters Common", i);
    par->axis[i].MaxSize  = par->oif_metadata_hash.get_value_int( axis_name+"/MaxSize", 0 );

    par->axis[i].StartPosition  = par->oif_metadata_hash.get_value_double( axis_name+"/StartPosition", 0 );
    par->axis[i].EndPosition  = par->oif_metadata_hash.get_value_double( axis_name+"/EndPosition", 0 );

    par->axis[i].AxisCode = par->oif_metadata_hash.get_value( axis_name+"/AxisCode", "" );
    par->axis[i].AxisName = par->oif_metadata_hash.get_value( axis_name+"/AxisName", "" );
    par->axis[i].PixUnit  = par->oif_metadata_hash.get_value( axis_name+"/PixUnit", "" );
    par->axis[i].UnitName = par->oif_metadata_hash.get_value( axis_name+"/UnitName", "" );
  }

  info->depth  = par->oif_metadata_hash.get_value_int( "Reference Image Parameter/ImageDepth", 0 )*8;
  info->width  = par->axis[0].MaxSize;
  info->height = par->axis[1].MaxSize;
  info->samples = par->axis[2].MaxSize;

  // pixel resolution
  par->pixel_resolution.resize(4);
  for (int i=0; i<4; ++i) par->pixel_resolution[i] = 0;
  if (par->axis[0].MaxSize != 0)
    par->pixel_resolution[0] = fabs(par->axis[0].EndPosition-par->axis[0].StartPosition) / (double) par->axis[0].MaxSize;
  if (par->axis[1].MaxSize != 0)
    par->pixel_resolution[1] = fabs(par->axis[1].EndPosition-par->axis[1].StartPosition) / (double) par->axis[1].MaxSize;
  if (par->axis[3].MaxSize != 0)
    par->pixel_resolution[2] = fabs(par->axis[3].EndPosition-par->axis[3].StartPosition) / (double) par->axis[3].MaxSize;
  if (par->axis[4].MaxSize != 0)
    par->pixel_resolution[3] = fabs(par->axis[4].EndPosition-par->axis[4].StartPosition) / (double) par->axis[4].MaxSize;

  // read axis units and scale pixel size accordingly
  if (par->axis[0].PixUnit == "nm") par->pixel_resolution[0] /= 1000.0;
  if (par->axis[1].PixUnit == "nm") par->pixel_resolution[1] /= 1000.0;
  if (par->axis[3].PixUnit == "nm") par->pixel_resolution[2] /= 1000.0;
  if (par->axis[4].PixUnit == "ms") par->pixel_resolution[3] /= 1000.0;

  //---------------------------------------------------------------
  // image geometry
  //---------------------------------------------------------------

  // add Z and T pages
  info->number_pages = 1;
  info->number_z = 1;
  info->number_t = 1;

  if (axis_count>3 && par->axis[3].isValid()) {
    info->number_pages *= par->axis[3].MaxSize;
    info->number_z = std::max(par->axis[3].MaxSize, 1);
    par->num_z = par->axis[3].MaxSize;
  }

  if (axis_count>3 && par->axis[4].isValid()) {
    info->number_pages *= par->axis[4].MaxSize;
    info->number_t = std::max(par->axis[4].MaxSize, 1);
    par->num_t = par->axis[4].MaxSize;
  }

  if (par->axis[6].isValid()) par->num_l = par->axis[6].MaxSize;


  //---------------------------------------------------------------
  // pixel depth
  //---------------------------------------------------------------
  if (info->depth != 16) 
    info->pixelType = FMT_UNSIGNED;
  else
    info->pixelType = FMT_UNSIGNED;

  info->resUnits = RES_um;
  info->xRes = par->pixel_resolution[0];
  info->yRes = par->pixel_resolution[1];

  //---------------------------------------------------------------
  // Channel names
  //---------------------------------------------------------------
  //"Reference Image Parameter/ImageDepth"
  xstring channel_dir;
  par->channel_names.clear();
  int c=1;
  while (c < std::max( (int)info->samples, (int)16) ) {
    channel_dir.sprintf("Channel %d Parameters", c);
    int phys_num = par->oif_metadata_hash.get_value_int( channel_dir+"/Physical CH Number", -1 );
    if (phys_num >= 0)
      par->channel_names.push_back( par->oif_metadata_hash.get_value( channel_dir+"/DyeName", "" ) );
    else break;
    ++c;
  }

  //---------------------------------------------------------------
  // Channel mapping
  //---------------------------------------------------------------
  // Here we basically read Contrast and guess where each channel should be mapped
  // we'll use Red/Contrast Green/Contrast Blue/Contrast, when contrast is 100 
  // that channel will be the preferred mapping

  // first initialize the lut
  par->display_lut.resize((int) bim::NumberDisplayChannels, -1);
  par->channel_mapping.resize(info->samples);
  std::vector<int> display_prefs(info->samples, -1);
  bool display_channel_set=false;
  for (unsigned int sample=0; sample<info->samples; ++sample) {
    TagMap lut_info = parse_lut_stream( olePar, sample );
    double r = lut_info.get_value_int( "Red/Contrast", 0 );   // max: 100
    double g = lut_info.get_value_int( "Green/Contrast", 0 ); // max: 100
    double b = lut_info.get_value_int( "Blue/Contrast", 0 );  // max: 100
    par->channel_mapping[sample] = bim::DisplayColor( static_cast<int>(r*2.555), static_cast<int>(g*2.555), static_cast<int>(b*2.555));

    int display_channel=-1;
    if ( r>=1 && g==0 && b==0 ) display_channel = bim::Red;
    else
    if ( r==0 && g>=1 && b==0 ) display_channel = bim::Green;
    else
    if ( r==0 && g==0 && b>=1 ) display_channel = bim::Blue;
    else
    if ( r>=1 && g>=1 && b==0 ) display_channel = bim::Yellow;
    else
    if ( r>=1 && g==0 && b>=1 ) display_channel = bim::Magenta;
    else
    if ( r==0 && g>=1 && b>=1 ) display_channel = bim::Cyan;
    else
    if ( r>=1 && g>=1 && b>=1 ) display_channel = bim::Gray;
    
    display_prefs[sample] = display_channel;
    if (display_channel>=0) { par->display_lut[display_channel] = sample; display_channel_set=true; }
  }

  // safeguard for some channels to be hidden by user preferences, in this case, ignore user preference
  if (display_channel_set) {
    int display=0;
    for (int vis=0; vis<bim::NumberDisplayChannels; ++vis)
      if (par->display_lut[vis]>-1) ++display;

    if (display<std::min<int>(info->samples, bim::NumberDisplayChannels)) {
      display_channel_set = false;
      for (int i=0; i<(int) bim::NumberDisplayChannels; ++i) 
        par->display_lut[i] = -1;
    }
  }

  // if the image has no preferred mapping set something
  if (!display_channel_set) {
    if (info->samples == 1) 
      for (int i=0; i<3; ++i) par->display_lut[i] = 0;
    else
      for (int i=0; i<std::min((int) bim::NumberDisplayChannels, (int)info->samples); ++i) 
        par->display_lut[i] = i;

    // safeguard channel fusion parameters, if something is strange in the metadata
    std::vector<bim::DisplayColor> default_colors;
    default_colors.push_back( bim::DisplayColor(255,   0,   0) ); // red
    default_colors.push_back( bim::DisplayColor(0,   255,   0) ); // green
    default_colors.push_back( bim::DisplayColor(0,     0, 255) ); // blue
    default_colors.push_back( bim::DisplayColor(255, 255, 255) ); // gray
    default_colors.push_back( bim::DisplayColor(255,   0, 255) ); // magenta
    default_colors.push_back( bim::DisplayColor(  0, 255, 255) ); // cyan
    default_colors.push_back( bim::DisplayColor(255, 255,   0) ); // yellow

    for (size_t i=0; i<std::min<size_t>(info->samples, default_colors.size()); ++i)
        par->channel_mapping[i] = default_colors[i];
  
  }

  

  //---------------------------------------------------------------
  // define dims
  //---------------------------------------------------------------
  if (info->number_z>1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_Z;
  }

  if (info->number_t>1) {
    info->number_dims = 4;
    info->dimensions[3].dim = DIM_T;
  }

  if (info->number_z>1 && info->number_t>1) {
    info->number_dims = 5;
    info->dimensions[3].dim = DIM_Z;        
    info->dimensions[4].dim = DIM_T;
  }


  return 0;
}

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

// find stream name independent of the OIB version
std::string get_stream_name( oib::Params *par, int page, int sample ) {
  
  std::string position_name = get_channel_stream_name( par, page, sample );

  //v 1.0.0.0 - Stream00007=Storage00001/probe_C001.tif (use filename_)
  std::string file_name = par->oifFolderName + "/" + par->oifFileName + "_" + position_name; 
  xstring stream_name = par->oib_info_hash.get_key( file_name );

  //v 2.0.0.0 - Stream00006=Storage00001/s_C001Z001.tif (use s_)
  if (stream_name.size()<=0) {
    file_name = par->oifFolderName + "/s_" + position_name; 
    stream_name = par->oib_info_hash.get_key( file_name );
  }

  //v X.X.X.X slow gessing if we did not find the stream, hope that _C001Z001 construct will be found
  if (stream_name.size()<=0) {
    stream_name = par->oib_info_hash.get_key_where_value_endsWith( position_name );
  }

  //strip initial "OibSaveInfo/" from the key
  std::string info_header("OibSaveInfo/");
  stream_name.erase( 0, info_header.size() );
  stream_name = "/" + par->oifFolderName + "/" + stream_name;
  return stream_name;
}

int read_oib_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  ole::Params *parOle = (ole::Params *) fmtHndl->internalParams;
  oib::Params *par = &parOle->oib_params;
  ImageInfo *info = &parOle->i; 
  if (parOle->storage == NULL) return 1;
  if (fmtHndl->pageNumber >= info->number_pages) fmtHndl->pageNumber = info->number_pages-1;

  ImageBitmap *img = fmtHndl->image;
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;

  TIFFSetWarningHandler(0);
  TIFFSetErrorHandler(0);
  for (unsigned int sample=0; sample<img->i.samples; ++sample) {
    std::string stream_name = get_stream_name( par, fmtHndl->pageNumber, sample );
    POLE::Stream *stream = new POLE::Stream( parOle->storage, stream_name );
    if (!stream || stream->size() <= 0) return 1;
    if( stream->fail() ) return 1;
    fmtHndl->stream = stream;

    TIFF *tiff = TIFFClientOpen( fmtHndl->fileName, "rm", (thandle_t) fmtHndl, 
      stream_tiff_read, stream_tiff_write, stream_tiff_seek, stream_tiff_close, stream_tiff_size, stream_tiff_mmap, stream_tiff_unmap );

    if (!tiff) return 1;

    // since i'm not sure if metadata for depth is correct, we'll read actual tiff settings first
    bim::uint16 bitspersample = 1;
    bim::uint32 height = 0; 
    bim::uint32 width = 0; 
    bim::uint16 samplesperpixel = 1;

    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    
    if (info->depth != bitspersample) return 1;
    if (info->width != width) return 1;
    if (info->height != height) return 1;
    if (samplesperpixel != 1) return 1;

    uchar *p = (uchar *) img->bits[sample];
    bim::uint lineSize = getLineSizeInBytes( img );

    for(unsigned int y=0; y<img->i.height; y++) {
      xprogress( fmtHndl, y*(sample+1), img->i.height*img->i.samples, "Reading OIB" );
      if ( xtestAbort( fmtHndl ) == 1) break;  
      TIFFReadScanline(tiff, p, y, 0);
      p += lineSize;
    } // for y

    fmtHndl->stream = NULL;
    delete stream;
    TIFFClose( tiff );
  }  // for sample

  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint oib_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  ole::Params *parOle = (ole::Params *) fmtHndl->internalParams;
  oib::Params *par = &parOle->oib_params;
  if (!parOle->storage) return 1;

  hash->set_value( bim::PIXEL_RESOLUTION_X, par->pixel_resolution[0] );
  hash->set_value( bim::PIXEL_RESOLUTION_Y, par->pixel_resolution[1] );
  hash->set_value( bim::PIXEL_RESOLUTION_Z, par->pixel_resolution[2] );
  hash->set_value( bim::PIXEL_RESOLUTION_T, par->pixel_resolution[3] );

  if (par->pixel_resolution[0]>0) hash->set_value( bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  if (par->pixel_resolution[1]>0) hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  if (par->pixel_resolution[2]>0) hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS );
  if (par->pixel_resolution[3]>0) hash->set_value( bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS );

  //date
  std::string date = par->oif_metadata_hash.get_value("Acquisition Parameters Common/ImageCaputreDate", "" );
  if (date.size()>0) {
    if ( *date.begin() == '\'' && *(date.end()-1) == '\'' ) date = date.substr( 1, date.size()-2);
    hash->set_value( bim::IMAGE_DATE_TIME, date );
  }

  // channel names
  xstring tag;
  for (int i=0; i<par->channel_names.size(); ++i) {
    tag.sprintf( bim::CHANNEL_NAME_TEMPLATE.c_str(), i);
    hash->set_value( tag.c_str(), par->channel_names[i] );
  }

  // preferred lut mapping
  if (par->display_lut.size() >= bim::NumberDisplayChannels) {
    hash->set_value( bim::DISPLAY_CHANNEL_RED,     par->display_lut[bim::Red] );
    hash->set_value( bim::DISPLAY_CHANNEL_GREEN,   par->display_lut[bim::Green] );
    hash->set_value( bim::DISPLAY_CHANNEL_BLUE,    par->display_lut[bim::Blue] );
    hash->set_value( bim::DISPLAY_CHANNEL_YELLOW,  par->display_lut[bim::Yellow] );
    hash->set_value( bim::DISPLAY_CHANNEL_MAGENTA, par->display_lut[bim::Magenta] );
    hash->set_value( bim::DISPLAY_CHANNEL_CYAN,    par->display_lut[bim::Cyan] );
    hash->set_value( bim::DISPLAY_CHANNEL_GRAY,    par->display_lut[bim::Gray] );
  }

  for (int i=0; i<par->channel_mapping.size(); ++i) {
    xstring tag_name  = xstring::xprintf( bim::CHANNEL_COLOR_TEMPLATE.c_str(), i );
    xstring tag_value = xstring::xprintf( "%d,%d,%d", par->channel_mapping[i].r, par->channel_mapping[i].g, par->channel_mapping[i].b );
    hash->append_tag( tag_name, tag_value );
  }

  // include all other tags from the hash into custom tag location
  bim::TagMap::const_iterator it = par->oif_metadata_hash.begin();
  while (it != par->oif_metadata_hash.end() ) {
    xstring key = it->first;
    if ( !key.startsWith("Sequential Group") && 
         !key.startsWith("ProfileSaveInfo") )
         hash->set_value(xstring(bim::CUSTOM_TAGS_PREFIX) + key, par->oif_metadata_hash.get_value(key));
    ++it;
  }

  return 0;
}

