/*******************************************************************************

  Manager for Image Formats with MetaData parsing

  Uses DimFiSDK version: 1
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  Notes:
    Session: during session any session wide operation will be performed
    with current session!!!
    If you want to start simultaneous sessions then create new manager
    using = operator. It will copy all necessary initialization data
    to the new manager.

    Non-session wide operation might be performed simultaneously with 
    an open session.

  History:
    03/23/2004 18:03 - First creation
    01/25/2007 21:00 - added QImaging TIFF
      
  ver: 3
        

*******************************************************************************/

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

#include <cstring>

#include <xtypes.h>
#include <xstring.h>
#include <bim_metatags.h>
#include <bim_lcms_parse.h>

#include "meta_format_manager.h"

using namespace bim;

std::vector<bim::DisplayColor> bim::defaultChannelColors() {
    std::vector<bim::DisplayColor> c;
    c.push_back( bim::DisplayColor(255,   0,   0) ); // red
    c.push_back( bim::DisplayColor(0,   255,   0) ); // green
    c.push_back( bim::DisplayColor(0,     0, 255) ); // blue
    c.push_back( bim::DisplayColor(255, 255, 255) ); // gray
    c.push_back( bim::DisplayColor(255,   0, 255) ); // magenta
    c.push_back( bim::DisplayColor(  0, 255, 255) ); // cyan
    c.push_back( bim::DisplayColor(255, 255,   0) ); // yellow
    c.push_back( bim::DisplayColor(255, 128,   0) ); // CUSTOM - special case
    return c;
}

MetaFormatManager::MetaFormatManager() : FormatManager() {

    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_RED);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_GREEN);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_BLUE);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_GRAY);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_CYAN);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_MAGENTA);
    display_channel_tag_names.push_back(bim::DISPLAY_CHANNEL_YELLOW);

    channel_colors_default = bim::defaultChannelColors();

    pixel_format_strings.push_back("undefined");
    pixel_format_strings.push_back("unsigned integer");
    pixel_format_strings.push_back("signed integer");
    pixel_format_strings.push_back("floating point");
    pixel_format_strings.push_back("complex");

    // color spaces and image modes
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_MONO);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_GRAY);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_INDEXED);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_RGB);
    image_mode_strings.push_back("");
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_HSL);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_HSV);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_RGBA);
    image_mode_strings.push_back("");
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_CMYK);
    image_mode_strings.push_back("");
    image_mode_strings.push_back("");
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_MULTICHANNEL);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_RGBE);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_YUV);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_XYZ);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_LAB);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_CMY);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_LUV);
    image_mode_strings.push_back(bim::ICC_TAGS_COLORSPACE_YCBCR);
}

MetaFormatManager::~MetaFormatManager()
{

}

bool MetaFormatManager::display_lut_needs_fusion() const {
  for (int i=bim::Blue+1; i<(int) bim::NumberDisplayChannels; ++i) 
    if (display_lut[i] != -1) return true;
  return false;
}

int MetaFormatManager::sessionStartRead(const bim::Filename fileName) {
  got_meta_for_session = -1;
  imaging_time = "0000-00-00 00:00:00";
  pixel_size[0] = 0;
  pixel_size[1] = 0;
  pixel_size[2] = 0;
  pixel_size[3] = 0;
  channel_names.clear();
  display_lut.clear();
  metadata.clear();
  info = initImageInfo();
  return FormatManager::sessionStartRead(fileName);
}

void MetaFormatManager::sessionEnd() {
  got_meta_for_session = -1;
  FormatManager::sessionEnd();
}

int MetaFormatManager::sessionWriteImage ( ImageBitmap *bmp, bim::uint page ) {
  if (session_active != true) return 1;
  sessionHandle.metaData = &this->metadata;
  return FormatManager::sessionWriteImage( bmp, page );
}

int MetaFormatManager::sessionReadImage ( ImageBitmap *bmp, bim::uint page ) {
  if (session_active != true) return 1;
  int res = FormatManager::sessionReadImage( bmp, page );
  info = bmp->i;

  channel_names.clear();
  display_lut.clear();
  //metadata.clear();

  display_lut.resize((int) bim::NumberDisplayChannels);
  for (int i=0; i<(int) bim::NumberDisplayChannels; ++i) display_lut[i] = -1;

  if (info.samples == 1)
    for (int i=0; i<3; ++i) 
      display_lut[i] = 0;
  else
    for (int i=0; i<bim::min<int>((int) bim::NumberDisplayChannels, info.samples); ++i) 
      display_lut[i] = i;

  switch ( info.resUnits ) {
    case RES_um:
      pixel_size[0] = info.xRes;
      pixel_size[1] = info.xRes;
      pixel_size[2] = 0;
      pixel_size[3] = 0;
      break;

    case RES_nm:
      pixel_size[0] = info.xRes / 1000.0;
      pixel_size[1] = info.xRes / 1000.0;
      pixel_size[2] = 0;
      pixel_size[3] = 0;
      break;

    case RES_mm:
      pixel_size[0] = info.xRes * 1000.0;
      pixel_size[1] = info.xRes * 1000.0;
      pixel_size[2] = 0;
      pixel_size[3] = 0;
      break;

    default:
      pixel_size[0] = 0;
      pixel_size[1] = 0;
      pixel_size[2] = 0;
      pixel_size[3] = 0;
  }

  return res;
}


void MetaFormatManager::sessionWriteSetMetadata( const TagMap &hash ) {
    metadata = hash;
    if (session_active)
        sessionHandle.metaData = &this->metadata;
}

void MetaFormatManager::sessionWriteSetOMEXML( const std::string &omexml ) {
    metadata.set_value(bim::RAW_TAGS_OMEXML, omexml, bim::RAW_TYPES_OMEXML);
}

void MetaFormatManager::sessionParseMetaData(bim::uint page) {
    if (got_meta_for_session == page) return;

    channel_names.clear();
    display_lut.clear();
    metadata.clear();

    //tagList = sessionReadMetaData(page, -1, -1, -1);
    got_meta_for_session = page;

    { // parsing stuff from red image

        display_lut.resize((int)bim::NumberDisplayChannels);
        for (int i = 0; i < (int)bim::NumberDisplayChannels; ++i) display_lut[i] = -1;

        if (info.samples == 1)
        for (int i = 0; i < 3; ++i)
            display_lut[i] = 0;
        else
        for (int i = 0; i < bim::min<int>((int)bim::NumberDisplayChannels, info.samples); ++i)
            display_lut[i] = i;

        switch (info.resUnits) {
        case RES_um:
            pixel_size[0] = info.xRes;
            pixel_size[1] = info.xRes;
            pixel_size[2] = 0;
            pixel_size[3] = 0;
            break;

        case RES_nm:
            pixel_size[0] = info.xRes / 1000.0;
            pixel_size[1] = info.xRes / 1000.0;
            pixel_size[2] = 0;
            pixel_size[3] = 0;
            break;

        case RES_mm:
            pixel_size[0] = info.xRes * 1000.0;
            pixel_size[1] = info.xRes * 1000.0;
            pixel_size[2] = 0;
            pixel_size[3] = 0;
            break;

        default:
            pixel_size[0] = 0;
            pixel_size[1] = 0;
            pixel_size[2] = 0;
            pixel_size[3] = 0;
        }
    }

    // API v1.7: first check if format has appand metadata function, if yes, run
    if (session_active && sessionFormatIndex >= 0 && sessionFormatIndex < formatList.size()) {
        FormatHeader *selectedFmt = formatList.at(sessionFormatIndex);
        if (selectedFmt->appendMetaDataProc)
            selectedFmt->appendMetaDataProc(&sessionHandle, &metadata);
    }

    // hack towards new system: in case the format loaded all data directly into the map, refresh static values
    fill_static_metadata_from_map();

    // All following will only be inserted if tags do not exist

    appendMetadata(bim::IMAGE_FORMAT, this->sessionGetFormatName());
    if (this->imaging_time != "0000-00-00 00:00:00")
        appendMetadata(bim::IMAGE_DATE_TIME, this->imaging_time);

    if (this->pixel_size[0] > 0) {
        appendMetadata(bim::PIXEL_RESOLUTION_X, this->pixel_size[0]);
        appendMetadata(bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    }

    if (this->pixel_size[1] > 0) {
        appendMetadata(bim::PIXEL_RESOLUTION_Y, this->pixel_size[1]);
        appendMetadata(bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    }

    if (this->pixel_size[2] > 0) {
        appendMetadata(bim::PIXEL_RESOLUTION_Z, this->pixel_size[2]);
        appendMetadata(bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    }

    if (this->pixel_size[3] > 0) {
        appendMetadata(bim::PIXEL_RESOLUTION_T, this->pixel_size[3]);
        appendMetadata(bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS);
    }

    // -------------------------------------------------------
    // Image info

    appendMetadata(bim::IMAGE_NUM_X, (int)info.width);
    appendMetadata(bim::IMAGE_NUM_Y, (int)info.height);
    appendMetadata(bim::IMAGE_NUM_Z, (int)info.number_z);
    appendMetadata(bim::IMAGE_NUM_T, (int)info.number_t);
    appendMetadata(bim::IMAGE_NUM_C, (int)info.samples);
    appendMetadata(bim::IMAGE_NUM_P, (int)info.number_pages);
    appendMetadata(bim::PIXEL_DEPTH, (int)info.depth);
    appendMetadata(bim::PIXEL_FORMAT, pixel_format_strings[info.pixelType]);
    appendMetadata(bim::RAW_ENDIAN, (bim::bigendian) ? "big" : "little");
    // overwrite imageMode from metadata colorspace
    bim::ImageModes mode = lcms_image_mode(metadata.get_value(bim::ICC_TAGS_COLORSPACE));
    if (mode != bim::IM_MULTI) 
        info.imageMode = mode;
    appendMetadata(bim::IMAGE_MODE, image_mode_strings[info.imageMode]);
    appendMetadata(bim::ICC_TAGS_COLORSPACE, image_mode_strings[info.imageMode]);


    appendMetadata(bim::IMAGE_NUM_RES_L, (int)info.number_levels);
    if (info.tileWidth > 0 && info.tileHeight > 0) {
        appendMetadata(bim::TILE_NUM_X, (int)info.tileWidth);
        appendMetadata(bim::TILE_NUM_Y, (int)info.tileHeight);
    }

    const char *dimNames[6] = { "none", "X", "Y", "C", "Z", "T" };
    xstring dimensions;
    try {
        if (info.number_dims >= BIM_MAX_DIMS) info.number_dims = BIM_MAX_DIMS - 1;
        for (unsigned int i = 0; i < info.number_dims; ++i) {
            dimensions += xstring::xprintf("%s", dimNames[info.dimensions[i].dim]);
        }
    }
    catch (...) {
    }
    appendMetadata(bim::IMAGE_DIMENSIONS, dimensions);


    for (unsigned int i = 0; i < channel_names.size(); ++i) {
        xstring tag_name;
        tag_name.sprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i);
        appendMetadata(tag_name.c_str(), channel_names[i].c_str());
    }

    for (int i = 0; i < (int)bim::min<size_t>(display_lut.size(), display_channel_tag_names.size()); ++i)
        appendMetadata(display_channel_tag_names[i], display_lut[i]);

    if (info.samples == 1)
        appendMetadata(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "255,255,255");
    else
    for (int i = 0; i < (int)bim::min<size_t>(info.samples, channel_colors_default.size()); ++i)
        appendMetadata(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), i),
        xstring::xprintf("%d,%d,%d", channel_colors_default[i].r, channel_colors_default[i].g, channel_colors_default[i].b));
}

double MetaFormatManager::getPixelSizeX()
{
  if (got_meta_for_session != sessionGetCurrentPage() ) 
    sessionParseMetaData ( sessionGetCurrentPage() );
  return pixel_size[0];
}

double MetaFormatManager::getPixelSizeY()
{
  if (got_meta_for_session != sessionGetCurrentPage() ) 
    sessionParseMetaData ( sessionGetCurrentPage() );
  return pixel_size[1];
}

double MetaFormatManager::getPixelSizeZ()
{
  if (got_meta_for_session != sessionGetCurrentPage() ) 
    sessionParseMetaData ( sessionGetCurrentPage() );
  return pixel_size[2];
}

double MetaFormatManager::getPixelSizeT()
{
  if (got_meta_for_session != sessionGetCurrentPage() ) 
    sessionParseMetaData ( sessionGetCurrentPage() );
  return pixel_size[3];
}

const char* MetaFormatManager::getImagingTime() {
  if (got_meta_for_session != sessionGetCurrentPage() ) 
    sessionParseMetaData ( sessionGetCurrentPage() );
  return imaging_time.c_str();
}


ImageBitmap *MetaFormatManager::sessionImage() {
  return NULL;
}

//---------------------------------------------------------------------------------------
// MISC
//---------------------------------------------------------------------------------------

void MetaFormatManager::fill_static_metadata_from_map() {
  std::string s;
  double d;
  int v;

  // Date time
  s = get_metadata_tag( bim::IMAGE_DATE_TIME, "" );
  if (s.size()>0) imaging_time = s;
  
  // Resolution
  d = get_metadata_tag_double( bim::PIXEL_RESOLUTION_X, 0 );
  if (d>0) pixel_size[0] = d;

  d = get_metadata_tag_double( bim::PIXEL_RESOLUTION_Y, 0 );
  if (d>0) pixel_size[1] = d;

  d = get_metadata_tag_double( bim::PIXEL_RESOLUTION_Z, 0 );
  if (d>0) pixel_size[2] = d;

  d = get_metadata_tag_double( bim::PIXEL_RESOLUTION_T, 0 );
  if (d>0) pixel_size[3] = d;


  // LUT
  for (int i=0; i<display_channel_tag_names.size(); ++i) {
    v = get_metadata_tag_int( display_channel_tag_names[i], -2 );
    if (v>-2 && display_lut.size()>i) display_lut[i] = v;  
  }


  // ------------------------------------------
  // Channel Names
 
  if (channel_names.size()<info.samples) channel_names.resize( info.samples );
  for (unsigned int i=0; i<channel_names.size(); ++i) {
    xstring tag_name;
    tag_name.sprintf( bim::CHANNEL_NAME_TEMPLATE.c_str(), i );

    s = get_metadata_tag( tag_name, "" );
    if (s.size()>0) channel_names[i] = s;
  }
}



