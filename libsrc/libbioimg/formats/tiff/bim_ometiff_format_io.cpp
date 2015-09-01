/*****************************************************************************
  OME-TIFF definitions 
  Copyright (c) 2009, Center for Bio-Image Informatics, UCSB
 
  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    2009-07-09 12:01 - First creation

  Ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <Jzon.h>
#include <pugixml.hpp>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_image.h>

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"

using namespace bim;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

unsigned int tiffGetNumberOfPages( bim::TiffParams *par );
int tiff_update_subifd_next_pointer(TIFF* tif, bim::uint64 dir_offset, bim::uint64 to_offset);
void detectTiffPyramid(bim::TiffParams *tiffParams);
int read_tiff_image_level(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams, bim::uint page, bim::uint level);
int read_tiff_image_tile(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level);
void pyramid_append_metadata(bim::FormatHandle *fmtHndl, bim::TagMap *hash);
void generic_append_metadata(FormatHandle *fmtHndl, TagMap *hash);
void generic_write_metadata(FormatHandle *fmtHndl, TagMap *hash);

//----------------------------------------------------------------------------
// OME-TIFF MISC FUNCTIONS
//----------------------------------------------------------------------------

bool omeTiffIsValid(bim::TiffParams *par) {
  if (!par) return false;
  TinyTiff::IFD *ifd = par->ifds.firstIfd();
  if (!ifd) return false;
  if (!ifd->tagPresent(TIFFTAG_IMAGEDESCRIPTION )) return false;
  bim::xstring tag_270 = ifd->readTagString(TIFFTAG_IMAGEDESCRIPTION );
  if ( tag_270.contains("<OME") && tag_270.contains("<Image") && tag_270.contains("<Pixels") ) return true;
  return false;
}

bim::xstring ometiff_normalize_xml_spaces( const bim::xstring &s ) {
  bim::xstring o = s;
  bim::xstring::size_type b=0;

  while (b != std::string::npos) {
    b = o.find ( "=" , b );
    if (b != std::string::npos ) {
      while (o[b+1] == ' ')
        o = o.erase(b+1, 1);

      while (b>0 && o[b-1] == ' ') {
        o = o.erase(b-1, 1);
        --b;
      }

      ++b;
    }
  }

  return o;
}

int omeTiffGetInfo (bim::TiffParams *par) {
  if (!par) return 1;
  if (!par->tiff) return 1;
  TinyTiff::IFD *ifd = par->ifds.firstIfd();
  if (!ifd) return false;

  TIFF *tif = par->tiff;
  bim::ImageInfo *info = &par->info;
  bim::OMETiffInfo *ome = &par->omeTiffInfo;

  // Read OME-XML from image description tag
  if (!ifd->tagPresent(TIFFTAG_IMAGEDESCRIPTION )) return false;
  bim::xstring tag_270 = ifd->readTagString(TIFFTAG_IMAGEDESCRIPTION );
  if (tag_270.size()<=0) return false;

  //---------------------------------------------------------------
  // image geometry
  //---------------------------------------------------------------
  bim::xstring tag_pixels = tag_270.section("<Pixels", ">");
  if (tag_pixels.size()<=0) return false;
  tag_pixels = ometiff_normalize_xml_spaces( tag_pixels );

  //info->number_pages = tiffGetNumberOfPages( par ); // dima: enumerating all pages in an ome-tiff file with many pages is very slow
  //bim::uint64 real_tiff_pages = info->number_pages; 
  ome->channels  = tag_pixels.section(" SizeC=\"", "\"").toInt(1);
  ome->number_t  = tag_pixels.section(" SizeT=\"", "\"").toInt(1);
  ome->number_z  = tag_pixels.section(" SizeZ=\"", "\"").toInt(1);
  ome->width     = tag_pixels.section(" SizeX=\"", "\"").toInt(1);
  ome->height    = tag_pixels.section(" SizeY=\"", "\"").toInt(1);
  ome->bim_order = tag_pixels.section(" DimensionOrder=\"", "\"");
  ome->pages     = ome->number_t*ome->number_z;  
  bim::uint64 real_tiff_pages =  ome->pages * ome->channels; // dima: enumerating all pages in an ome-tiff file with many pages is very slow

  //info->width    = ome->width;    // tiff image will have the most correct width and height parameters
  //info->height   = ome->height;   // tiff image will have the most correct width and height parameters 
  info->samples  = ome->channels; // no way to estimate number of channels, they come as spearate pages
  info->number_pages = ome->pages;
  info->number_t = ome->number_t;
  info->number_z = ome->number_z;

  if (info->samples > 1) 
    info->imageMode = bim::IM_MULTI;
  else
    info->imageMode = bim::IM_GRAYSCALE;    

  bim::uint16 bitspersample = 1;  
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);  
  info->depth = bitspersample;

  //---------------------------------------------------------------
  // fix for the case of multiple files, with many channels defined 
  // but not provided in the particular TIFF
  //---------------------------------------------------------------
  bim::uint16 samplesperpixel = 1;
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  if (samplesperpixel == 1) {
    // test if number of channels is correct given the number of pages
    if (real_tiff_pages < ome->pages * ome->channels) {
      info->samples = 1;
      ome->channels = 1;
    }

    if (real_tiff_pages < ome->pages) {
      ome->pages = real_tiff_pages;
      info->number_pages = real_tiff_pages;
    }

    if (info->number_t*info->number_z < ome->pages) {
      if (info->number_z > info->number_t) 
        info->number_z = ome->pages;
      else
        info->number_t = ome->pages;
    }
  }



  //---------------------------------------------------------------
  // define dims
  //---------------------------------------------------------------
  if (info->number_z > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = bim::DIM_Z;
  }
  if (info->number_t > 1) {
    info->number_dims = 4;
    info->dimensions[3].dim = bim::DIM_T;
  }
  if (info->number_z>1 && info->number_t>1) {
    info->number_dims = 5;
    info->dimensions[3].dim = bim::DIM_Z;        
    info->dimensions[4].dim = bim::DIM_T;
  }

  //--------------------------------------------------------------------  
  // pixel resolution
  //--------------------------------------------------------------------

  ome->pixel_resolution[0] = tag_pixels.section(" PhysicalSizeX=\"", "\"").toDouble(0.0);
  ome->pixel_resolution[1] = tag_pixels.section(" PhysicalSizeY=\"", "\"").toDouble(0.0);
  ome->pixel_resolution[2] = tag_pixels.section(" PhysicalSizeZ=\"", "\"").toDouble(0.0);
  ome->pixel_resolution[3] = tag_pixels.section(" TimeIncrement=\"", "\"").toDouble(0.0);

  // Fix for old style OME-TIFF images
  bim::xstring tag_image = tag_270.section("<Image", ">");
  if (tag_image.size()>0) {
    tag_image = ometiff_normalize_xml_spaces( tag_image );
    
    if (ome->pixel_resolution[0]==0.0)
      ome->pixel_resolution[0] = tag_image.section(" PixelSizeX=\"", "\"").toDouble(0.0);
    if (ome->pixel_resolution[1]==0.0)
      ome->pixel_resolution[1] = tag_image.section(" PixelSizeY=\"", "\"").toDouble(0.0);
    if (ome->pixel_resolution[2]==0.0)
      ome->pixel_resolution[2] = tag_image.section(" PixelSizeZ=\"", "\"").toDouble(0.0);
    if (ome->pixel_resolution[3]==0.0)
      ome->pixel_resolution[3] = tag_image.section(" PixelSizeT=\"", "\"").toDouble(0.0);
  }

  //std::ofstream myfile;
  //myfile.open ("D:\\dima_media\\images\\_BIO\\_dima_ome_tiff\\oib.xml");
  //myfile << tag_270;
  //myfile.close();  

  return 0;
}

void omeTiffGetCurrentPageInfo(bim::TiffParams *par) {
  if (!par) return;
  bim::ImageInfo *info = &par->info;
  bim::OMETiffInfo *ome = &par->omeTiffInfo;

  info->samples = ome->channels;
  info->number_pages = ome->pages;

  info->resUnits = bim::RES_um;
  info->xRes = ome->pixel_resolution[0];
  info->yRes = ome->pixel_resolution[1];
}


//----------------------------------------------------------------------------
// READ/WRITE FUNCTIONS
//----------------------------------------------------------------------------

bim::uint64 computeTiffDirectory( bim::FormatHandle *fmtHndl, int page, int sample ) {
  bim::TiffParams *par = (bim::TiffParams *) fmtHndl->internalParams;
  bim::ImageInfo *info = &par->info;
  bim::OMETiffInfo *ome = &par->omeTiffInfo;

  bim::uint64 nz = std::max<bim::uint64>(info->number_z, 1);
  bim::uint64 nt = std::max<bim::uint64>(info->number_t, 1);
  bim::uint64 nc = std::max<bim::uint64>(info->samples, 1);

  //XYLZT file will be named as “xxx_C00mL00qZ00nT00p.tif”
  bim::uint64 c = sample;
  bim::uint64 l = (page/nz)/nt;
  bim::uint64 t = (page - l*nt*nz) / nz;
  bim::uint64 z = page - nt*l - nz*t;

  // compute directory position based on order: XYCZT
  bim::uint64 dirNum = (t*nz + z)*nc + c;

  // Possible orders: XYCZT XYCTZ XYZCT XYZTC XYTCZ XYTZC
  if (ome->bim_order.startsWith("XYCZ") ) dirNum = (t*nz + z)*nc + c;
  if (ome->bim_order.startsWith("XYCT") ) dirNum = (z*nt + t)*nc + c;
  if (ome->bim_order.startsWith("XYZC") ) dirNum = (t*nc + c)*nz + z;
  if (ome->bim_order.startsWith("XYZT") ) dirNum = (c*nt + t)*nz + z;
  if (ome->bim_order.startsWith("XYTC") ) dirNum = (z*nc + c)*nt + t;
  if (ome->bim_order.startsWith("XYTZ") ) dirNum = (t*nz + z)*nt + t;

  return dirNum;
}

// this is here due to some OME-TIFF do not conform with the standard and come with all channels in the same IFD
bim::uint64 computeTiffDirectoryNoChannels( bim::FormatHandle *fmtHndl, int page, int sample ) {
  bim::TiffParams *par = (bim::TiffParams *) fmtHndl->internalParams;
  bim::ImageInfo *info = &par->info;
  bim::OMETiffInfo *ome = &par->omeTiffInfo;

  bim::uint64 nz = std::max<bim::uint64>(info->number_z, 1);
  bim::uint64 nt = std::max<bim::uint64>(info->number_t, 1);
  bim::uint64 nc = 1;

  //XYLZT file will be named as “xxx_C00mL00qZ00nT00p.tif”
  bim::uint64 c = 0;
  bim::uint64 l = (page/nz)/nt;
  bim::uint64 t = (page - l*nt*nz) / nz;
  bim::uint64 z = page - nt*l - nz*t;

  // compute directory position based on order: XYCZT
  bim::uint64 dirNum = (t*nz + z)*nc + c;

  // Possible orders: XYCZT XYCTZ XYZCT XYZTC XYTCZ XYTZC
  if (ome->bim_order.startsWith("XYCZ") ) dirNum = (t*nz + z)*nc + c;
  if (ome->bim_order.startsWith("XYCT") ) dirNum = (z*nt + t)*nc + c;
  if (ome->bim_order.startsWith("XYZC") ) dirNum = (t*nc + c)*nz + z;
  if (ome->bim_order.startsWith("XYZT") ) dirNum = (c*nt + t)*nz + z;
  if (ome->bim_order.startsWith("XYTC") ) dirNum = (z*nc + c)*nt + t;
  if (ome->bim_order.startsWith("XYTZ") ) dirNum = (t*nz + z)*nt + t;

  return dirNum;
}

bool ometiff_read_striped(TIFF *tif, bim::ImageBitmap *img, bim::FormatHandle *fmtHndl, const unsigned int &sample) {
    bim::uint lineSize = getLineSizeInBytes(img);
    bim::uchar *p = (bim::uchar *) img->bits[sample];
    for (register bim::uint64 y = 0; y < img->i.height; y++) {
        xprogress(fmtHndl, y*(sample + 1), img->i.height*img->i.samples, "Reading OME-TIFF");
        if (xtestAbort(fmtHndl) == 1) return false;
        if (TIFFReadScanline(tif, p, y, 0) < 0) return false;
        p += lineSize;
    } // for y
    return true;
}

bool ometiff_read_tiled(TIFF *tif, bim::ImageBitmap *img, bim::FormatHandle *fmtHndl, const int &sample) {
    if (!tif || !img) return false;

    bim::uint lineSize = getLineSizeInBytes(img);
    bim::uint bpp = ceil((double)img->i.depth / 8.0);
    bim::uint32 columns, rows;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &columns);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &rows);

    std::vector<bim::uchar> buffer(TIFFTileSize(tif));
    bim::uchar *buf = &buffer[0];

    for (bim::uint y = 0; y<img->i.height; y += rows) {
        xprogress(fmtHndl, y, img->i.height, "Reading TIFF");
        if (xtestAbort(fmtHndl) == 1) return false;

        bim::uint tile_height = (img->i.height - y >= rows) ? rows : (bim::uint) img->i.height - y;
        for (bim::uint x = 0; x<(bim::uint)img->i.width; x += columns) {
            bim::uint tile_width = (img->i.width - x < columns) ? (bim::uint) img->i.width - x : (bim::uint) columns;
            if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0) return false;
            for (bim::uint yi = 0; yi < tile_height; yi++) {
                bim::uchar *p = (bim::uchar *) img->bits[sample] + (lineSize * (y + yi));
                _TIFFmemcpy(p + (x*bpp), buf + (yi*columns*bpp), tile_width*bpp);
            }
        } // for x
    } // for y
    return true;
}


bim::uint omeTiffReadPlane( bim::FormatHandle *fmtHndl, bim::TiffParams *par, int plane ) {
  if (!par) return 1;
  if (!par->tiff) return 1;
  if (par->subType != bim::tstOmeTiff && par->subType != bim::tstOmeBigTiff) return 1;  
  TinyTiff::IFD *ifd = par->ifds.firstIfd();
  if (!ifd) return 1;
  TIFF *tif = par->tiff;
  if (!tif) return false;

  bim::ImageInfo *info = &par->info;
  bim::OMETiffInfo *ome = &par->omeTiffInfo;
  
  //--------------------------------------------------------------------  
  // read image parameters
  //--------------------------------------------------------------------
  bim::uint64 tiff_page = computeTiffDirectory( fmtHndl, fmtHndl->pageNumber, 0 );
  bim::ImageBitmap *img = fmtHndl->image;
  TIFFSetDirectory( tif, (bim::uint16) tiff_page );

  bim::uint16 bitspersample = 1;
  bim::uint32 height = 0; 
  bim::uint32 width = 0; 
  bim::uint16 samplesperpixel = 1;
  bim::uint16 sampleformat = 1;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
  TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleformat);

  if (samplesperpixel > 1) {
    tiff_page = computeTiffDirectoryNoChannels( fmtHndl, fmtHndl->pageNumber, 0 );
    TIFFSetDirectory( tif, (bim::uint16) tiff_page );
    return 2;
  }

  if (img->i.depth != bitspersample || img->i.width != width || img->i.height != height) {
      //info->samples = ome->ch;
      info->depth = bitspersample;
      info->width = width;
      info->height = height;
      info->pixelType = bim::FMT_UNSIGNED;
      if (sampleformat == SAMPLEFORMAT_INT)
          info->pixelType = bim::FMT_SIGNED;
      else if (sampleformat == SAMPLEFORMAT_IEEEFP)
          info->pixelType = bim::FMT_FLOAT;

      if (allocImg(fmtHndl, info, img) != 0) return 1;
  }


  //--------------------------------------------------------------------
  // read data
  //--------------------------------------------------------------------
  for (unsigned int sample = 0; sample < (unsigned int)info->samples; ++sample) {
      tiff_page = computeTiffDirectory(fmtHndl, fmtHndl->pageNumber, sample);
      TIFFSetDirectory(tif, (bim::uint16) tiff_page);
      if (!TIFFIsTiled(tif)) {
          if (!ometiff_read_striped(tif, img, fmtHndl, sample)) return 1;
      } else {
          if (!ometiff_read_tiled(tif, img, fmtHndl, sample)) return 1;
      }
  }  // for sample

  //TIFFSetDirectory(tif, fmtHndl->pageNumber);
  return 0;
}


//--------------------------------------------------------------------------------------------
// Levels and Tiles functions
//--------------------------------------------------------------------------------------------

int ometiff_read_image_level(bim::FormatHandle *fmtHndl, bim::TiffParams *par, bim::uint page, bim::uint level) {
    if (!par) return 1;
    if (!par->tiff) return 1;
    if (par->subType != bim::tstOmeTiff && par->subType != bim::tstOmeBigTiff) return 1;
    TIFF *tif = par->tiff;
    if (!tif) return 1;

    bim::ImageInfo *info = &par->info;
    bim::OMETiffInfo *ome = &par->omeTiffInfo;
    bim::PyramidInfo *pyramid = &par->pyramid;
    bim::ImageBitmap *img = fmtHndl->image;

    
    //-------------------------------------------------------------------- 
    // detect levels
    //-------------------------------------------------------------------- 

    bim::uint64 tiff_page = computeTiffDirectory(fmtHndl, fmtHndl->pageNumber, 0);
    TIFFSetDirectory(tif, (bim::uint16) tiff_page);
    detectTiffPyramid(par);
    if (pyramid->number_levels > 0 && pyramid->number_levels <= level) return 1;
    bim::uint64 subdiroffset = pyramid->directory_offsets[level];
    if (TIFFSetSubDirectory(tif, subdiroffset) == 0) return 1;

    //--------------------------------------------------------------------  
    // read image parameters
    //--------------------------------------------------------------------

    bim::uint16 bitspersample = 1;
    bim::uint32 height = 0;
    bim::uint32 width = 0;
    bim::uint16 samplesperpixel = 1;
    bim::uint16 sampleformat = 1;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleformat);

    // fix for ome-tiff files with all channels within one image
    if (samplesperpixel > 1) {
        tiff_page = computeTiffDirectoryNoChannels(fmtHndl, fmtHndl->pageNumber, 0);
        TIFFSetDirectory(tif, (bim::uint16) tiff_page);
        detectTiffPyramid(par);
        return read_tiff_image_level(fmtHndl, par, page, level);
    }

    if (img->i.depth != bitspersample || img->i.width != width || img->i.height != height) {
        //info->samples = ome->channels;
        info->depth = bitspersample;
        info->width = width;
        info->height = height;
        info->pixelType = bim::FMT_UNSIGNED;
        if (sampleformat == SAMPLEFORMAT_INT)
            info->pixelType = bim::FMT_SIGNED;
        else if (sampleformat == SAMPLEFORMAT_IEEEFP)
            info->pixelType = bim::FMT_FLOAT;

        if (allocImg(fmtHndl, info, img) != 0) return 1;
    }


    //--------------------------------------------------------------------
    // read data
    //--------------------------------------------------------------------
    for (unsigned int sample = 0; sample < (unsigned int)info->samples; ++sample) {
        tiff_page = computeTiffDirectory(fmtHndl, fmtHndl->pageNumber, sample);
        if (TIFFSetDirectory(tif, (bim::uint16) tiff_page) == 0) return 1;
        detectTiffPyramid(par);
        if (pyramid->number_levels > 0 && pyramid->number_levels <= level) return 1;
        bim::uint64 subdiroffset = pyramid->directory_offsets[level];
        if (TIFFSetSubDirectory(tif, subdiroffset) == 0) return 1;

        if (!TIFFIsTiled(tif)) {
            if (!ometiff_read_striped(tif, img, fmtHndl, sample)) return 1;
        } else {
            if (!ometiff_read_tiled(tif, img, fmtHndl, sample)) return 1;
        }
    }  // for sample

    TIFFSetDirectory(tif, tiff_page);
    return 0;
}

int ometiff_read_image_tile(bim::FormatHandle *fmtHndl, bim::TiffParams *par, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level) {
    if (!par) return 1;
    if (!par->tiff) return 1;
    if (par->subType != bim::tstOmeTiff && par->subType != bim::tstOmeBigTiff) return 1;
    TIFF *tif = par->tiff;
    if (!tif) return 1;

    bim::OMETiffInfo *ome = &par->omeTiffInfo;
    bim::PyramidInfo *pyramid = &par->pyramid;
    bim::ImageBitmap *img = fmtHndl->image;

    if (!TIFFIsTiled(tif)) return 1;

    // set correct level
    bim::uint64 tiff_page = computeTiffDirectory(fmtHndl, fmtHndl->pageNumber, 0);
    TIFFSetDirectory(tif, (bim::uint16) tiff_page);
    detectTiffPyramid(par);
    if (pyramid->number_levels > 0 && pyramid->number_levels <= level) return 1;
    bim::uint64 subdiroffset = pyramid->directory_offsets[level];
    if (TIFFSetSubDirectory(tif, subdiroffset) == 0) return 1;

    // set tile parameters
    bim::uint32 height = 0;
    bim::uint32 width = 0;
    bim::uint16 planarConfig;
    bim::uint32 columns, rows;
    bim::uint16 photometric = PHOTOMETRIC_MINISWHITE;
    bim::uint16 bitspersample = 1;
    bim::uint16 samplesperpixel = 1;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &columns);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &rows);
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);

    // fix for ome-tiff files with all channels within one image
    if (samplesperpixel > 1) {
        tiff_page = computeTiffDirectoryNoChannels(fmtHndl, fmtHndl->pageNumber, 0);
        TIFFSetDirectory(tif, (bim::uint16) tiff_page);
        detectTiffPyramid(par);
        return read_tiff_image_tile(fmtHndl, par, page, xid, yid, level);
    }

    // tile sizes may be smaller at the border
    bim::uint64 x = xid * columns;
    bim::uint64 y = yid * rows;
    bim::uint tile_width = (width - x >= columns) ? columns : (bim::uint) width - x;
    bim::uint tile_height = (height - y >= rows) ? rows : (bim::uint) height - y;

    //img->i.samples = samplesperpixel;
    img->i.samples = ome->channels;
    img->i.depth = bitspersample;
    img->i.width = tile_width;
    img->i.height = tile_height;
    bim::uint bpp = ceil((double)img->i.depth / 8.0);
    if (allocImg(fmtHndl, &img->i, img) != 0) return 1;

    std::vector<bim::uint8> buffer(TIFFTileSize(tif));
    bim::uint8 *buf = &buffer[0];

    // libtiff tiles will always have columns hight with empty pixels
    // we need to copy only the usable portion
    for (bim::uint sample = 0; sample < img->i.samples; sample++) {
        tiff_page = computeTiffDirectory(fmtHndl, fmtHndl->pageNumber, sample);
        if (TIFFSetDirectory(tif, (bim::uint16) tiff_page) == 0) return 1;
        detectTiffPyramid(par);
        if (pyramid->number_levels > 0 && pyramid->number_levels <= level) return 1;
        bim::uint64 subdiroffset = pyramid->directory_offsets[level];
        if (TIFFSetSubDirectory(tif, subdiroffset) == 0) return 1;

        if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0) return 1;

        #pragma omp parallel for default(shared) 
        for (bim::int64 y = 0; y < tile_height; ++y) {
            bim::uint8 *from = buf + y*bpp*columns;
            bim::uint8 *to = ((bim::uint8 *)img->bits[sample]) + y*bpp*tile_width;
            memcpy(to, from, bpp*tile_width);
        }
    }  // for sample

    TIFFSetDirectory(tif, tiff_page);
    return 0;
}



//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

// OME Color class from Bioformats Color.h
class OmeColor {
public:
    // The type of an individual color component (R, G, B, A)
    typedef bim::uint8  component_type;
    // The type of all components composed as a single RGBA value (unsigned)
    typedef bim::uint32 composed_type;
    // The type of all components composed as a single RGBA value (signed)
    typedef bim::int32  signed_type;

    inline OmeColor() : value(0x000000FFU) {}

    inline OmeColor(composed_type value) : value(value) {
    }

    inline OmeColor(signed_type value) :
        value(static_cast<composed_type>(value)) {
    }

    inline OmeColor(component_type r, component_type g, component_type b, component_type a = std::numeric_limits<component_type>::max()) :
        value(static_cast<composed_type>(r) << 24U |
              static_cast<composed_type>(g) << 16U |
              static_cast<composed_type>(b) << 8U |
              static_cast<composed_type>(a) << 0U) {
    }

    inline ~OmeColor() {
    }

    // Get the red component of this color.
    inline component_type getRed() const {
        return static_cast<component_type>((this->value >> 24U) & 0xffU);
    }

    // Set the red component of this color.
    inline void setRed(component_type red) {
        this->value &= ~(0xFFU << 24U);
        this->value |= static_cast<composed_type>(red) << 24U;
    }

    // Get the green component of this color.
    inline component_type getGreen() const {
        return static_cast<component_type>((this->value >> 16U) & 0xffU);
    }

    // Set the green component of this color.
    inline void setGreen(component_type green) {
        this->value &= ~(0xffU << 16U);
        this->value |= static_cast<composed_type>(green) << 16U;
    }

    // Get the blue component of this color.
    inline component_type getBlue() const {
        return static_cast<component_type>((this->value >> 8U) & 0xffU);
    }

    // Set the blue component of this color.
    inline void setBlue(component_type blue) {
        this->value &= ~(0xffU << 8U);
        this->value |= static_cast<composed_type>(blue) << 8U;
    }

    // Get the alpha component of this color.
    inline component_type getAlpha() const {
        return static_cast<component_type>((this->value >> 0) & 0xffU);
    }

    // Set the alpha component of this color.
    inline void setAlpha(component_type alpha) {
        this->value &= ~(0xffU << 0);
        this->value |= static_cast<composed_type>(alpha) << 0;
    }

    // Get the *signed* integer value of this color.
    inline signed_type getValue() const {
        return static_cast<signed_type>(this->value);
    }

    // Set the integer value of this color from an unsigned integer
    inline void setValue(composed_type value) {
        this->value = value;
    }

    // Set the integer value of this color from a *signed* integer.
    inline void setValue(signed_type value) {
        this->value = static_cast<composed_type>(value);
    }

    // Cast the color to its value as an unsigned integer.
    inline operator composed_type () const {
        return this->value;
    }

    // Cast the color to its value as a signed integer.
    inline operator signed_type () const {
        return static_cast<signed_type>(this->value);
    }

    inline std::string toString() const {
        std::stringstream s;
        s << this->getValue();
        return s.str();
    }

private:
    composed_type value;
};

inline std::string colorRGBStringToOMEString(const xstring &c) {
    std::vector<int> v = c.splitInt(",");
    OmeColor omec;
    if (v.size()>0) omec.setRed(v[0]);
    if (v.size()>1) omec.setGreen(v[1]);
    if (v.size()>2) omec.setBlue(v[2]);
    if (v.size()>3) omec.setAlpha(v[3]);
    return omec.toString();
}

inline std::string colorOMEStringToRGBString(const xstring &c) {
    OmeColor omec(c.toInt());
    return xstring::xprintf("%d,%d,%d", omec.getRed(), omec.getGreen(), omec.getBlue());
}


void parse_json_object (bim::TagMap *hash, Jzon::Object &parent_node, const std::string &path) {
  for (Jzon::Object::iterator it = parent_node.begin(); it != parent_node.end(); ++it) {
    std::string name = (*it).first;
    Jzon::Node &node = (*it).second;
    if (node.IsObject()) {
        parse_json_object (hash, node.AsObject(), path+name+'/');
    }
    std::string value = node.ToString();
    hash->set_value( path+name, value );
  }
}

bim::uint append_metadata_omeTiff(bim::FormatHandle *fmtHndl, bim::TagMap *hash) {

    if (!fmtHndl) return 1;
    if (!fmtHndl->internalParams) return 1;
    if (!hash) return 1;

    bim::TiffParams *par = (bim::TiffParams *) fmtHndl->internalParams;
    bim::ImageInfo *info = &par->info;
    bim::OMETiffInfo *ome = &par->omeTiffInfo;


    //----------------------------------------------------------------------------
    // Dimensions
    //----------------------------------------------------------------------------
    hash->append_tag(bim::IMAGE_NUM_Z, (const unsigned int)info->number_z);
    hash->append_tag(bim::IMAGE_NUM_T, (const unsigned int)info->number_t);
    hash->append_tag(bim::IMAGE_NUM_C, ome->channels);

    //----------------------------------------------------------------------------
    // Resolution
    //----------------------------------------------------------------------------
    hash->set_value(bim::PIXEL_RESOLUTION_X, ome->pixel_resolution[0]);
    hash->set_value(bim::PIXEL_RESOLUTION_Y, ome->pixel_resolution[1]);
    hash->set_value(bim::PIXEL_RESOLUTION_Z, ome->pixel_resolution[2]);
    hash->set_value(bim::PIXEL_RESOLUTION_T, ome->pixel_resolution[3]);

    hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_MICRONS);
    hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS);

    //----------------------------------------------------------------------------
    // Reading OME-TIFF tag
    //----------------------------------------------------------------------------

    TinyTiff::IFD *ifd = par->ifds.firstIfd();
    if (!ifd) return 1;
    TIFF *tif = par->tiff;

    bim::xstring tag_270 = ifd->readTagString(TIFFTAG_IMAGEDESCRIPTION);
    if (tag_270.size() <= 0) return 0;
    hash->append_tag(bim::RAW_TAGS_PREFIX + "ome-tiff", tag_270);

    //----------------------------------------------------------------------------
    // Channel names and preferred mapping
    //----------------------------------------------------------------------------
    for (unsigned int i = 0; i<(bim::uint)ome->channels; ++i) {
        bim::xstring tag = bim::xstring::xprintf("<LightSource ID=\"LightSource:%d\">", i);
        if (tag == "")
            tag = bim::xstring::xprintf("<LightSource ID=\"LightSource:0:%d\">", i);

        std::string::size_type p = tag_270.find(tag);
        if (p == std::string::npos) continue;
        bim::xstring tag_laser = tag_270.section("<Laser", ">", p);
        if (tag_laser.size() <= 0) continue;
        bim::xstring medium = tag_laser.section(" LaserMedium=\"", "\"");
        if (medium.size() <= 0) continue;
        bim::xstring wavelength = tag_laser.section(" Wavelength=\"", "\"");
        if (wavelength.size()>0)
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), medium + " - " + wavelength + "nm");
        else
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), medium);
    }

    // channel names may also be stored in Logical channel in the Image
    // v <=4
    std::string::size_type p = tag_270.find("<LogicalChannel ");
    if (p != std::string::npos)
    for (unsigned int i = 0; i < (bim::uint)ome->channels; ++i) {

        //if (p == std::string::npos) continue;
        bim::xstring tag = tag_270.section("<LogicalChannel", ">", p);
        if (tag.size() <= 0) continue;
        tag = ometiff_normalize_xml_spaces(tag);
        bim::xstring medium = tag.section(" Name=\"", "\"");
        if (medium.size() <= 0) continue;
        bim::xstring wavelength = tag.section(" ExWave=\"", "\"");
        bim::xstring color      = tag.section(" Color=\"", "\"");
        int chan = i;
        p = tag_270.find("<ChannelComponent", p);
        tag = tag_270.section("<ChannelComponent", ">", p);
        if (tag.size() > 0) {
            tag = ometiff_normalize_xml_spaces(tag);
            bim::xstring index = tag.section(" Index=\"", "\"");
            chan = index.toInt(i);
        }

        if (wavelength.size() > 0)
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium + " - " + wavelength + "nm");
        else
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium);

        if (color.size() > 0)
            hash->append_tag(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), chan), colorOMEStringToRGBString(color));

        p += 15;
    }

    // channel names may also be stored in Logical channel in the Image
    // v >=5
    // read using pugixml
    int red_channels = 0;
    pugi::xml_document doc;
    if (doc.load_buffer(tag_270.c_str(), tag_270.size())) {
        try {
            pugi::xpath_node_set channels = doc.select_nodes("/OME/Image/Pixels/Channel");
            for (pugi::xpath_node_set::const_iterator it = channels.begin(); it != channels.end(); ++it) {
                pugi::xpath_node node = *it;
                bim::xstring medium     = node.node().attribute("Name").value();
                bim::xstring index      = node.node().attribute("ID").value();
                bim::xstring wavelength = node.node().attribute("ExcitationWavelength").value();
                bim::xstring color      = node.node().attribute("Color").value();

                // parse index
                std::vector<bim::xstring> ids = index.split(":");
                if (ids.size() > 2) {
                    int chan = ids[2].toInt(0);
                    if (wavelength.size() > 0)
                        hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium + " - " + wavelength + "nm");
                    else
                        hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium);

                    if (color.size() > 0)
                        hash->append_tag(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), chan), colorOMEStringToRGBString(color));

                    ++red_channels;
                }
            }
        }
        catch (pugi::xpath_exception& e) {
            // do nothing
        }
    }

    // OME-XML may contain errors, try old way of reading
    if (red_channels < ome->channels) {
        p = tag_270.find("<Channel ");
        if (p != std::string::npos)
        for (unsigned int i = 0; i < (bim::uint)ome->channels; ++i) {

            bim::xstring heading = bim::xstring::xprintf("<Channel ID=\"Channel:0:%d\"", i);
            bim::xstring tag = tag_270.section(heading, ">", p);
            if (tag.size() <= 0) continue;
            tag = ometiff_normalize_xml_spaces(tag);
            bim::xstring medium = tag.section(" Name=\"", "\"");
            if (medium.size() <= 0) continue;

            int chan = i;
            bim::xstring wavelength = tag.section(" ExcitationWavelength=\"", "\"");
            bim::xstring color      = tag.section(" Color=\"", "\"");

            if (wavelength.size() > 0)
                hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium + " - " + wavelength + "nm");
            else
                hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium);

            if (color.size() > 0)
                hash->append_tag(xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), chan), colorOMEStringToRGBString(color));

        }
    }


    //----------------------------------------------------------------------------
    // pyramid info
    //----------------------------------------------------------------------------

    pyramid_append_metadata(fmtHndl, hash);

    //----------------------------------------------------------------------------
    // stage position
    //----------------------------------------------------------------------------
    p = tag_270.find("<Plane ");
    while (p != std::string::npos) {
        bim::xstring tag = tag_270.section("<Plane", ">", p);
        if (tag.size() > 0) {
            tag = ometiff_normalize_xml_spaces(tag);
            int c = tag.section(" TheC=\"", "\"").toInt(0);
            int t = tag.section(" TheT=\"", "\"").toInt(0);
            int z = tag.section(" TheZ=\"", "\"").toInt(0);

            tag = tag_270.section("<StagePosition", ">", p);
            if (tag.size() > 0) {
                tag = ometiff_normalize_xml_spaces(tag);
                double sx = tag.section(" PositionX=\"", "\"").toDouble(0);
                double sy = tag.section(" PositionY=\"", "\"").toDouble(0);
                double sz = tag.section(" PositionZ=\"", "\"").toDouble(0);
                bim::uint64 page = t*info->number_z + z;
                hash->append_tag(bim::xstring::xprintf(bim::STAGE_POSITION_TEMPLATE_X.c_str(), page), sx);
                hash->append_tag(bim::xstring::xprintf(bim::STAGE_POSITION_TEMPLATE_Y.c_str(), page), sy);
                hash->append_tag(bim::xstring::xprintf(bim::STAGE_POSITION_TEMPLATE_Z.c_str(), page), sz);
            }

        }
        p = tag_270.find("<Plane ", p + 5);
    }


    //----------------------------------------------------------------------------
    // more stuff
    //----------------------------------------------------------------------------

    // v <=4
    bim::xstring tag = tag_270.section("<CreationDate>", "</CreationDate>");
    if (tag.size() >= 19) {
        tag[10] = ' ';
        hash->append_tag(bim::IMAGE_DATE_TIME, tag);
    }

    // v 5
    tag = tag_270.section("<AcquisitionDate>", "</AcquisitionDate>");
    if (tag.size() >= 19) {
        tag[10] = ' ';
        hash->append_tag(bim::IMAGE_DATE_TIME, tag);
    }

    p = tag_270.find("<Instrument ID=\"Instrument:0\">");

    bim::xstring tag_objective = tag_270.section("<Objective", ">", p);
    tag_objective = ometiff_normalize_xml_spaces(tag_objective);
    if (tag_objective.size() > 0) {
        bim::xstring model = tag_objective.section(" Model=\"", "\"");
        if (model.size() > 0)
            hash->append_tag(bim::OBJECTIVE_DESCRIPTION, model);
    }

    bim::xstring tag_magnification = tag_270.section("<NominalMagnification>", "</NominalMagnification>", p);
    if (tag_magnification.size() > 0)
        hash->append_tag(bim::OBJECTIVE_MAGNIFICATION, tag_magnification + "X");


    //----------------------------------------------------------------------------
    // read all custom attributes
    //----------------------------------------------------------------------------

    if (tag_270.contains("<StructuredAnnotations")) {
        // new OME-TIFF annotations format >= 5
        p = tag_270.find("<StructuredAnnotations");
        p = tag_270.find("<OriginalMetadata", p);
        bim::xstring tag_original_meta = tag_270.section("<OriginalMetadata>", "</OriginalMetadata>", p);
        while (tag_original_meta.size() > 0) {
            tag_original_meta = ometiff_normalize_xml_spaces(tag_original_meta);
            bim::xstring name = tag_original_meta.section("<Key>", "</Key>");
            bim::xstring val = tag_original_meta.section("<Value>", "</Value>");
            if (name.size() > 0 && val.size() > 0) {
                // replace all / here with some other character
                hash->append_tag(bim::CUSTOM_TAGS_PREFIX + name, val);
            }
            p += tag_original_meta.size();
            tag_original_meta = tag_270.section("<OriginalMetadata>", "</OriginalMetadata>", p);
        }
    }  else {
        // old format <=4
        p = tag_270.find("<CustomAttributes");
        p = tag_270.find("<OriginalMetadata", p);
        bim::xstring tag_original_meta = tag_270.section("<OriginalMetadata", ">", p);
        while (tag_original_meta.size() > 0) {
            tag_original_meta = ometiff_normalize_xml_spaces(tag_original_meta);
            bim::xstring name = tag_original_meta.section(" Name=\"", "\"");
            bim::xstring val = tag_original_meta.section(" Value=\"", "\"");
            if (name.size() > 0 && val.size() > 0) {
                // replace all / here with some other character
                hash->append_tag(bim::CUSTOM_TAGS_PREFIX + name, val);
            }
            p += tag_original_meta.size();
            tag_original_meta = tag_270.section("<OriginalMetadata", ">", p);
        }
    }

  //----------------------------------------------------------------------------
  // Reading Micro-Manager tag
  //----------------------------------------------------------------------------

  bim::xstring tag_MM = ifd->readTagString(TIFFTAG_MICROMANAGER);
  if (tag_MM.size()<=0) return 0;
  hash->append_tag( bim::RAW_TAGS_PREFIX+"micro-manager-raw", tag_MM );

  // parse micro-manager tags and append
  Jzon::Object rootNode;
  Jzon::Parser jparser(tag_MM);
  if (jparser.Parse(rootNode)) {
    std::string path = "MicroManager/";
    parse_json_object (hash, rootNode, path);
  }

  //----------------------------------------------------------------------------
  // Reading Micro-Manager tag
  //----------------------------------------------------------------------------

  generic_append_metadata(fmtHndl, hash);

  return 0;
}


//----------------------------------------------------------------------------
// Write METADATA
//----------------------------------------------------------------------------

std::string omeTiffPixelType( bim::ImageBitmap *img ) {
    std::string pt = "uint8";
    if (img->i.depth==16 && img->i.pixelType==bim::FMT_UNSIGNED) pt = "uint16";
    if (img->i.depth==32 && img->i.pixelType==bim::FMT_UNSIGNED) pt = "uint32";
    if (img->i.depth==8  && img->i.pixelType==bim::FMT_SIGNED)   pt = "int8";
    if (img->i.depth==16 && img->i.pixelType==bim::FMT_SIGNED)   pt = "int16";
    if (img->i.depth==32 && img->i.pixelType==bim::FMT_SIGNED)   pt = "int32";
    if (img->i.depth==32 && img->i.pixelType==bim::FMT_FLOAT)    pt = "float";
    if (img->i.depth==64 && img->i.pixelType==bim::FMT_FLOAT)    pt = "double"; 
    return pt;
}

std::string constructOMEXML( bim::FormatHandle *fmtHndl, bim::TagMap *hash ) {
  bim::ImageBitmap *img = fmtHndl->image; 
  
  // Header
  std::string str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  str += "<!-- Warning: this comment is an OME-XML metadata block, which contains crucial dimensional parameters and other important metadata. Please edit cautiously (if at all), and back up the original data before doing so. For more information, see the OME-TIFF web site: http://loci.wisc.edu/ome/ome-tiff.html. -->";
  // version <=4
  //str += "<OME xmlns=\"http://www.openmicroscopy.org/XMLschemas/OME/FC/ome.xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.openmicroscopy.org/XMLschemas/OME/FC/ome.xsd http://www.openmicroscopy.org/XMLschemas/OME/FC/ome.xsd\">";
  // version >= 5
  str += "<OME xmlns=\"http://www.openmicroscopy.org/Schemas/OME/2013-06\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.openmicroscopy.org/Schemas/OME/2013-06 http://www.openmicroscopy.org/Schemas/OME/2013-06/ome.xsd\">";

  // Image tag
  str += "<Image ID=\"Image:0\" Name=\"bioimage.ome.tif\">";

  if (hash->hasKey(bim::IMAGE_DATE_TIME))
      str += bim::xstring::xprintf("<AcquisitionDate>%s</AcquisitionDate>", hash->get_value(bim::IMAGE_DATE_TIME).c_str());

  str += bim::xstring::xprintf("<Description>Constructed by libbioimage ome-tiff encoder v4.0.3</Description>");


  str += "<Pixels ID=\"Pixels:0\"";
  str += " DimensionOrder=\"XYCZT\"";
  str += bim::xstring::xprintf(" Type=\"%s\"", omeTiffPixelType(img).c_str() );
  str += bim::xstring::xprintf(" SignificantBits=\"%d\"", img->i.depth);
  str += " BigEndian=\"false\" Interleaved=\"false\"";

  str += bim::xstring::xprintf(" SizeX=\"%d\"", img->i.width );
  str += bim::xstring::xprintf(" SizeY=\"%d\"", img->i.height ); 
  str += bim::xstring::xprintf(" SizeC=\"%d\"", img->i.samples ); 
  if (hash && hash->size()>0) {
      str += bim::xstring::xprintf(" SizeZ=\"%d\"", hash->get_value_int(bim::IMAGE_NUM_Z, (int) img->i.number_z) ); 
      str += bim::xstring::xprintf(" SizeT=\"%d\"", hash->get_value_int(bim::IMAGE_NUM_T, (int) img->i.number_t) );
  } else {
      str += bim::xstring::xprintf(" SizeZ=\"%d\"", img->i.number_z ); 
      str += bim::xstring::xprintf(" SizeT=\"%d\"", img->i.number_t );
  }

  // writing physical sizes
  if (hash && hash->size()>0) {
    if ( hash->hasKey(bim::PIXEL_RESOLUTION_X) )
      str += bim::xstring::xprintf(" PhysicalSizeX=\"%s\"", hash->get_value(bim::PIXEL_RESOLUTION_X).c_str() );
    if ( hash->hasKey(bim::PIXEL_RESOLUTION_Y) )
      str += bim::xstring::xprintf(" PhysicalSizeY=\"%s\"", hash->get_value(bim::PIXEL_RESOLUTION_Y).c_str() );
    if ( hash->hasKey(bim::PIXEL_RESOLUTION_Z) )
      str += bim::xstring::xprintf(" PhysicalSizeZ=\"%s\"", hash->get_value(bim::PIXEL_RESOLUTION_Z).c_str() );
    if ( hash->hasKey(bim::PIXEL_RESOLUTION_T) )
      str += bim::xstring::xprintf(" TimeIncrement=\"%s\"", hash->get_value(bim::PIXEL_RESOLUTION_T).c_str() );
  }

  str += " >";

  // channel names
  if (hash && hash->size()>0 && hash->hasKey(bim::CHANNEL_NAME_0)) {
      for (bim::uint i = 0; i<img->i.samples; ++i) {
          bim::xstring key = bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i);
          bim::xstring color = bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), i);
          if (hash->hasKey(key)) {
              str += bim::xstring::xprintf("<Channel ID=\"Channel:0:%d\" Name=\"%s\" SamplesPerPixel=\"1\"",
                  i, hash->get_value(key).c_str());
              if (hash->hasKey(color)) {
                  str += bim::xstring::xprintf(" Color=\"%s\"", colorRGBStringToOMEString(hash->get_value(color)).c_str());
              }
              str += ">";
              str += bim::xstring::xprintf("<DetectorSettings ID=\"Detector:0:%d\" /><LightPath/>", i);
              str += "</Channel>";
          }
      }
  }


  str += "<TiffData/></Pixels>";
  str += "</Image>";

  // custom attributes
  if (hash && hash->size() > 0) {
      str += "<StructuredAnnotations xmlns=\"http://www.openmicroscopy.org/Schemas/SA/2013-06\">";
      int i = 0;
      bim::TagMap::const_iterator it;
      for (it = hash->begin(); it != hash->end(); ++it) {
          bim::xstring key = (*it).first;
          if (key.startsWith(bim::CUSTOM_TAGS_PREFIX) && key != "custom/Image Description") {
              str += bim::xstring::xprintf("<XMLAnnotation ID=\"Annotation:%d\" Namespace=\"openmicroscopy.org/OriginalMetadata\">", i);
              str += "<Value xmlns=\"\"><OriginalMetadata>";
              // dima: here we should be encoding strings into utf-8, though bioformats seems to expect latin1, skip encoding for now
              str += bim::xstring::xprintf("<Key>%s</Key>", key.right(7).c_str());
              str += bim::xstring::xprintf("<Value>%s</Value>", hash->get_value(key));
              str += "</OriginalMetadata></Value></XMLAnnotation>";
              i++;
          }
      }
      str += "</StructuredAnnotations>";
  }

  str += "</OME>";
  return str;
}

bim::uint write_omeTiff_metadata (bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams) {
    TIFF *tif = tifParams->tiff;
    bim::TagMap *hash = fmtHndl->metaData;

    if (hash && hash->hasKey(bim::RAW_TAGS_OMEXML)) {
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, hash->get_value_bin(bim::RAW_TAGS_OMEXML));
    } else if (hash) {
        std::string xml = constructOMEXML( fmtHndl, hash );
        TIFFSetField( tif, TIFFTAG_IMAGEDESCRIPTION, xml.c_str() );
    } else {
        std::string xml = constructOMEXML(fmtHndl, 0);
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, xml.c_str());
    }

    generic_write_metadata(fmtHndl, hash);

    return 0;
}

//****************************************************************************
// OME-TIFF WRITER
//****************************************************************************

void ometiff_write_striped_plane(TIFF *out, bim::ImageBitmap *img, bim::FormatHandle *fmtHndl, const bim::uint &sample) {
    bim::uint64 height = (bim::uint32) img->i.height;
    bim::uchar *bits = (bim::uchar *) img->bits[sample];
    bim::uint line_size = getLineSizeInBytes(img);
    for (bim::uint64 y = 0; y < height; y++) {
        xprogress(fmtHndl, y*(sample + 1), height*img->i.samples, "Writing OME-TIFF");
        if (xtestAbort(fmtHndl) == 1) break;
        TIFFWriteScanline(out, bits, y, 0);
        bits += line_size;
    } // for y
}

void ometiff_write_tiled_plane(TIFF *tif, bim::ImageBitmap *img, bim::FormatHandle *fmtHndl, const bim::uint &sample) {
    bim::TiffParams *par = (bim::TiffParams *)fmtHndl->internalParams;

    bim::uint32 width = (bim::uint32) img->i.width;
    bim::uint32 height = (bim::uint32) img->i.height;
    bim::uint32 columns = par->info.tileWidth, rows = par->info.tileHeight;
    bim::uint bpp = ceil((double)img->i.depth / 8.0);

    TIFFSetField(tif, TIFFTAG_TILEWIDTH, columns);
    TIFFSetField(tif, TIFFTAG_TILELENGTH, rows);

    std::vector<bim::uint8> buffer(TIFFTileSize(tif));
    bim::uint8 *buf = &buffer[0];

    for (bim::uint y = 0; y<img->i.height; y += rows) {
        xprogress(fmtHndl, y, img->i.height, "Writing OME-TIFF");
        if (xtestAbort(fmtHndl) == 1) break;
        for (bim::uint x = 0; x<(bim::uint)img->i.width; x += columns) {
            bim::uint tile_width = (width - x >= columns) ? columns : (bim::uint) width - x;
            bim::uint tile_height = (height - y >= rows) ? rows : (bim::uint) height - y;

            // libtiff tiles will always have columns hight with empty pixels
            // we need to copy only the usable portion
            #pragma omp parallel for default(shared) 
            for (bim::int64 i = 0; i < tile_height; ++i) {
                bim::uint8 *to = buf + i*bpp*columns;
                bim::uint8 *from = ((bim::uint8 *)img->bits[sample]) + (y + i)*bpp*width + x*bpp;
                memcpy(to, from, bpp*tile_width);
            }
            if (TIFFWriteTile(tif, buf, x, y, 0, 0) < 0) break;
        } // for x
    } // for y
}

int omeTiffWritePlane(bim::FormatHandle *fmtHndl, bim::TiffParams *par, bim::ImageBitmap *img = NULL, bool subscale = false) {
  TIFF *out = par->tiff;
  if (!img) img = fmtHndl->image;
 
  bim::uint32 height = img->i.height;
  bim::uint32 width = img->i.width;
  bim::uint32 rowsperstrip = (bim::uint32) -1;
  bim::uint16 bitspersample = img->i.depth;
  bim::uint16 samplesperpixel = 1;
  bim::uint16 photometric = PHOTOMETRIC_MINISBLACK;
  if (bitspersample == 1 && samplesperpixel == 1) photometric = PHOTOMETRIC_MINISWHITE;
  bim::uint16 compression;
  bim::uint16 planarConfig = PLANARCONFIG_SEPARATE;	// separated planes 

  // ignore storing pyramid for small images
  bim::int64 sz = bim::max<bim::int64>(width, height);
  if (!subscale && par->pyramid.format != PyramidInfo::pyrFmtNone && sz<PyramidInfo::min_level_size) {
      par->pyramid.format = PyramidInfo::pyrFmtNone;
  }

  // only allow sub-dirs in ome-tiff for pyramid storage to maximize compatibility with other readers
  if (par->pyramid.format == PyramidInfo::pyrFmtTopDirs) {
      par->pyramid.format = PyramidInfo::pyrFmtSubDirs;
  }

  // samples in OME-TIFF are stored in separate IFDs 
  for (bim::uint sample = 0; sample < img->i.samples; sample++) {

      // handle standard width/height/bpp stuff
      TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
      TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
      TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
      TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitspersample);
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
      TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
      TIFFSetField(out, TIFFTAG_PLANARCONFIG, planarConfig);	// separated planes
      TIFFSetField(out, TIFFTAG_SOFTWARE, "libbioimage");

      // set pixel format
      bim::uint16 sampleformat = SAMPLEFORMAT_UINT;
      if (img->i.pixelType == bim::FMT_SIGNED) sampleformat = SAMPLEFORMAT_INT;
      if (img->i.pixelType == bim::FMT_FLOAT)  sampleformat = SAMPLEFORMAT_IEEEFP;
      TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, sampleformat);

      //if( TIFFGetField( out, TIFFTAG_DOCUMENTNAME, &pszText ) )
      //if( TIFFGetField( out, TIFFTAG_DATETIME, &pszText ) )


      //------------------------------------------------------------------------------  
      // resolution pyramid
      //------------------------------------------------------------------------------  

      if (par->info.tileWidth > 0 && par->pyramid.format != PyramidInfo::pyrFmtNone) {
          TIFFSetField(out, TIFFTAG_SUBFILETYPE, subscale ? FILETYPE_REDUCEDIMAGE : 0);

          if (!subscale) {
              par->pyramid.directory_offsets.resize(0);
              bim::int64 num_levels = ceil(bim::log2<double>(sz)) - ceil(bim::log2<double>(PyramidInfo::min_level_size)) + 1;
              if (par->pyramid.format == PyramidInfo::pyrFmtSubDirs) {
                  // if pyramid levels are to be written into SUBIFDs, write the tag and indicate to libtiff how many subifds are coming
                  bim::uint16 num_sub_ifds = num_levels - 1; // number of pyramidal levels - 1
                  std::vector<bim::uint64> offsets_sub_ifds(num_levels - 1, 0UL);
                  TIFFSetField(out, TIFFTAG_SUBIFD, num_sub_ifds, &offsets_sub_ifds[0]);
              }
          }
      }

      //------------------------------------------------------------------------------  
      // compression
      //------------------------------------------------------------------------------  

      compression = fmtHndl->compression;
      if (compression == 0) compression = COMPRESSION_NONE;

      if (compression == COMPRESSION_CCITTFAX4 && bitspersample != 1) {
          compression = COMPRESSION_NONE;
      } if (compression == COMPRESSION_JPEG && (bitspersample != 8 && bitspersample != 16)) {
          compression = COMPRESSION_NONE;
      }
      TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

      // set compression parameters
      bim::uint32 strip_size = bim::max<bim::uint32>(TIFFDefaultStripSize(out, -1), 1);
      if (compression == COMPRESSION_JPEG) {
          // rowsperstrip must be multiple of 8 for JPEG
          TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, strip_size + (8 - (strip_size % 8)));
          TIFFSetField(out, TIFFTAG_JPEGQUALITY, fmtHndl->quality);
      } else if (compression == COMPRESSION_ADOBE_DEFLATE) {
          //TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
          if (planarConfig == PLANARCONFIG_SEPARATE || samplesperpixel == 1)
              TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_NONE);
          else
              TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
          TIFFSetField(out, TIFFTAG_ZIPQUALITY, 9);
      } else if (compression == COMPRESSION_CCITTFAX4) {
          //TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
      } else if (compression == COMPRESSION_LZW) {
          //TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
          if (planarConfig == PLANARCONFIG_SEPARATE || samplesperpixel == 1)
              TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_NONE);
          else
              TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
      } else {
          //TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
      }

      //------------------------------------------------------------------------------
      // writing meta data
      //------------------------------------------------------------------------------
      if (fmtHndl->pageNumber == 0 && sample == 0 && !subscale) {
          generic_write_metadata(fmtHndl, fmtHndl->metaData);
          write_omeTiff_metadata(fmtHndl, par);
      }

      //------------------------------------------------------------------------------
      // writing image
      //------------------------------------------------------------------------------

      if (par->info.tileWidth < 1 || par->pyramid.format == PyramidInfo::pyrFmtNone) {
          ometiff_write_striped_plane(out, img, fmtHndl, sample);
      } else {
          ometiff_write_tiled_plane(out, img, fmtHndl, sample);
      }

      // correct libtiff writing of subifds by linking sibling ifds through nextifd offset
      if (subscale && par->pyramid.format == PyramidInfo::pyrFmtSubDirs) {
          bim::uint64 dir_offset = (TIFFSeekFile(out, 0, SEEK_END) + 1) &~1;
          par->pyramid.directory_offsets.push_back(dir_offset);
      }


      TIFFWriteDirectory(out);

      //------------------------------------------------------------------------------
      // write pyramid levels
      //------------------------------------------------------------------------------

      bim::ImageBitmap temp;
      if (!subscale && par->info.tileWidth >0 && par->pyramid.format != PyramidInfo::pyrFmtNone) {
          //bim::ImageBitmap temp;
          temp.i = img->i;
          temp.i.samples = 1;
          temp.bits[0] = img->bits[sample];
          bim::Image image(&temp);
          int i = 0;
          while (bim::max<unsigned int>(image.width(), image.height()) > PyramidInfo::min_level_size) {
              image = image.downSampleBy2x();
              if (omeTiffWritePlane(fmtHndl, par, image.imageBitmap(), true) != 0) break;
              ++i;
          }

          // correct libtiff writing of subifds by linking sibling ifds through nextifd offset
          if (par->pyramid.format == PyramidInfo::pyrFmtSubDirs)
          for (int i = 0; i < par->pyramid.directory_offsets.size() - 1; ++i) {
              if (!tiff_update_subifd_next_pointer(out, par->pyramid.directory_offsets[i], par->pyramid.directory_offsets[i + 1])) break;
          }
      }

  } // for sample

  if (!subscale) {
      TIFFFlushData(out);
      TIFFFlush(out);
  }
  return 0;
}


