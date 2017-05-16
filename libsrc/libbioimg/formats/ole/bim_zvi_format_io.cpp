/*****************************************************************************
  Zeiss ZVI format support
  Copyright (c) 2010, Center for Bio Image Informatics, UCSB
  
  Authors: 
    Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

    Michel Boudinot <Michel.Boudinot@inaf.cnrs-gif.fr>

  History:
    2010-08-26 17:13:22 - First creation

  Ver : 1
*****************************************************************************/

#include <cstring>
#include <string>
#include <algorithm>

#include <bim_img_format_interface.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_img_format_utils.h>

#include <pole.h>

#include "bim_ole_format.h"
#include "zvi.h"

namespace bim {
namespace zvi {
const char months[13][4] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int getMonthNum(const std::string &ms) {
  for (int i=1; i<=12; ++i)
    if (ms == months[i]) return i;
  return 0;
}
} // namespace ZVI
} // namespace bim

using namespace bim;

int zviGetImageInfo( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  ImageInfo *info = &par->i; 
  bim::zvi::Directory *z = &par->zvi_dir;
  
  info->ver = sizeof(ImageInfo);
  info->imageMode = IM_GRAYSCALE;
  info->tileWidth = 0;
  info->tileHeight = 0; 
  info->transparentIndex = 0;
  info->transparencyMatting = 0;
  info->lut.count = 0;

  z->fromStorage(par->storage);
  if (!z->isValid()) return 1;

  info->width   = z->imageWidth();
  info->height  = z->imageHeight();
  info->samples = z->channels();
  info->depth   = z->pixelBitDepth();
  info->pixelType = FMT_UNSIGNED;

  info->number_pages = z->pages() / z->channels();
  info->number_z     = z->zPlanes();
  info->number_t     = z->timePoints();

  if (z->pixelFormat()==bim::zvi::PT_FLOAT32 || z->pixelFormat()==bim::zvi::PT_FLOAT64) 
    info->pixelType = FMT_FLOAT;


  // set XY scale
  info->resUnits = RES_um;
  if (z->meta_scale()->hasKey("ScalingFactorX") && z->meta_scale()->hasKey("ScalingUnitX") &&
      z->meta_scale()->get_value("ScalingUnitX") == "Micrometer" ) {
    info->xRes = z->meta_scale()->get_value_double("ScalingFactorX", 0.0);
  }
  if (z->meta_scale()->hasKey("ScalingFactorY") && z->meta_scale()->hasKey("ScalingUnitY") &&
      z->meta_scale()->get_value("ScalingUnitY") == "Micrometer" ) {
    info->yRes = z->meta_scale()->get_value_double("ScalingFactorY", 0.0);
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

void get_z_t( bim::zvi::Directory *zvi_dir, int page, int sample, int &z, int &t ) {
  int nz = std::max<int>(zvi_dir->zPlanes(), 1);
  int nt = std::max<int>(zvi_dir->timePoints(), 1);
  int nc = std::max<int>(zvi_dir->channels(), 1);

  //int c = sample;
  int l = (page/nz)/nt;
  t = (page - l*nt*nz) / nz;
  z = page - nt*l - nz*t;
}

int zvi_read_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  ImageInfo *info = &par->i; 
  if (par->storage == NULL) return 1;
  if (fmtHndl->pageNumber >= par->i.number_pages) fmtHndl->pageNumber = par->i.number_pages-1;
  bim::zvi::Directory *zvi_dir = &par->zvi_dir;

  ImageBitmap *img = fmtHndl->image;
  if ( allocImg( fmtHndl, &par->i, img) != 0 ) return 1;

  for (unsigned int sample=0; sample<img->i.samples; ++sample) {
    xprogress( fmtHndl, sample+1, img->i.samples, "Reading ZVI" );
    if ( xtestAbort( fmtHndl ) == 1) break;  

    int c=sample, z=0, t=0;
    get_z_t( zvi_dir, fmtHndl->pageNumber, sample, z, t );
    if (!zvi_dir->readImagePixels( c, z, t, getImgSizeInBytes(img), (unsigned char *) img->bits[sample] )) return 1;
  }  // for sample

  return 0;
}


//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

std::string get_zvi_resolution_unit( const std::string &unit ) {
  if ( unit == "Micrometer") 
    return bim::PIXEL_RESOLUTION_UNIT_MICRONS;
  else return unit;
}

bim::uint zvi_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;
  ole::Params *par = (ole::Params *) fmtHndl->internalParams;
  if (!par->storage) return 1;
  bim::zvi::Directory *z = &par->zvi_dir;

  //-------------------------------------------
  // scale
  //-------------------------------------------
  if (z->meta_scale()->hasKey("ScalingFactorX"))
    hash->set_value( bim::PIXEL_RESOLUTION_X, z->meta_scale()->get_value("ScalingFactorX") );
  if (z->meta_scale()->hasKey("ScalingFactorY"))
    hash->set_value( bim::PIXEL_RESOLUTION_Y, z->meta_scale()->get_value("ScalingFactorY") );

  if (z->meta_scale()->hasKey("ScalingUnitX"))
    hash->set_value( bim::PIXEL_RESOLUTION_UNIT_X, get_zvi_resolution_unit(z->meta_scale()->get_value("ScalingUnitX")) );
  if (z->meta_scale()->hasKey("ScalingUnitY"))
    hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Y, get_zvi_resolution_unit(z->meta_scale()->get_value("ScalingUnitY")) );

  // ZVI has only 3 resolution axis, where the Z can probably be either time or depth
  if (z->meta_scale()->hasKey("ScalingFactorZ") && (z->zPlanes()>1 || z->timePoints()>1)) {
    std::string unit = z->meta_scale()->get_value("ScalingUnitZ", bim::PIXEL_RESOLUTION_UNIT_MICRONS );
    if (unit=="Second" || unit=="Millisecond" || unit=="Microsecond" || unit=="Minute" || unit=="Hour" ) {
      hash->set_value( bim::PIXEL_RESOLUTION_T, z->meta_scale()->get_value("ScalingFactorZ") );
      hash->set_value( bim::PIXEL_RESOLUTION_UNIT_T, unit );
    } else {
      hash->set_value( bim::PIXEL_RESOLUTION_Z, z->meta_scale()->get_value("ScalingFactorZ") );
      hash->set_value( bim::PIXEL_RESOLUTION_UNIT_Z, get_zvi_resolution_unit(z->meta_scale()->get_value("ScalingUnitZ")) );
    }
  }

  //-------------------------------------------
  //date: Thu Jun 22 08:27:13 2006
  //-------------------------------------------
  if (z->meta()->hasKey("Acquisition Date")) {
    xstring zvi_date = z->meta()->get_value("Acquisition Date");
    char t[1024], month[3];
    int m=0, y, d, h, mi, s;
    sscanf( zvi_date.c_str(), "%s %3s %d %d:%d:%d %d", t, month, &d, &h, &mi, &s, &y ); 
    m = zvi::getMonthNum( month );
    hash->set_value( bim::IMAGE_DATE_TIME, xstring::xprintf("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", y, m, d, h, mi, s) );
  }

  //-------------------------------------------
  // channel names
  //-------------------------------------------
  for (unsigned int i=0; i<z->channels(); ++i) {
    if (z->info()->hasKey(xstring::xprintf("channel_name_%d", i)))
      hash->set_value( xstring::xprintf( bim::CHANNEL_NAME_TEMPLATE.c_str(), i), z->info()->get_value(xstring::xprintf("channel_name_%d", i)) );
  }

  //-------------------------------------------
  // preferred lut mapping
  //-------------------------------------------
  // now write tags based on the order
  std::map<int, std::string> display_channel_names;
  display_channel_names[bim::Red]     = "red";
  display_channel_names[bim::Green]   = "green";
  display_channel_names[bim::Blue]    = "blue";
  display_channel_names[bim::Yellow]  = "yellow";
  display_channel_names[bim::Magenta] = "magenta";
  display_channel_names[bim::Cyan]    = "cyan";
  display_channel_names[bim::Gray]    = "gray";
  
  hash->set_value( bim::DISPLAY_CHANNEL_RED,     z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Red].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_GREEN,   z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Green].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_BLUE,    z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Blue].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_YELLOW,  z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Yellow].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_MAGENTA, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Magenta].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_CYAN,    z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Cyan].c_str())) );
  hash->set_value( bim::DISPLAY_CHANNEL_GRAY,    z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Gray].c_str())) );


  //-------------------------------------------
  // objective
  //-------------------------------------------
  if (z->meta()->hasKey("Objective Name")) {
      //hash->set_value( bim::OBJECTIVE_DESCRIPTION, z->meta()->get_value("Objective Name") );
      bim::parse_objective_from_string(z->meta()->get_value("Objective Name"), hash);
  }

  if (z->meta()->hasKey("Objective Magnification")) {
      double mag = bim::objective_parse_magnification(z->meta()->get_value("Objective Magnification")+"X");
      if (mag>0) hash->set_value(bim::OBJECTIVE_MAGNIFICATION, mag);
  }

  if (z->meta()->hasKey("Objective N.A.")) {
      double na = bim::objective_parse_num_aperture(z->meta()->get_value("Objective N.A."));
      if (na>0) hash->set_value(bim::OBJECTIVE_NUM_APERTURE, na);
  }

  //-------------------------------------------
  // include all other tags into custom tag location
  //-------------------------------------------
  bim::TagMap::const_iterator it = z->meta()->begin();
  while (it != z->meta()->end() ) {
      hash->set_value(xstring(bim::CUSTOM_TAGS_PREFIX) + it->first, z->meta()->get_value(it->first));
    ++it;
  }

  return 0;
}

