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

#include <iostream>
#include <fstream>

#include <Jzon.h>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "xtiffio.h"
#include "bim_tiny_tiff.h"
#include "bim_tiff_format.h"

unsigned int tiffGetNumberOfPages( bim::TiffParams *par );

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

  if( TIFFIsTiled(tif) ) return 1;

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
    else
    if (sampleformat == SAMPLEFORMAT_IEEEFP)
      info->pixelType = bim::FMT_FLOAT;

    if ( allocImg( fmtHndl, info, img) != 0 ) return 1;
  }


  //--------------------------------------------------------------------
  // read data
  //--------------------------------------------------------------------
  bim::uint lineSize = getLineSizeInBytes( img );
  for (unsigned int sample=0; sample<(unsigned int)info->samples; ++sample) {

    tiff_page = computeTiffDirectory( fmtHndl, fmtHndl->pageNumber, sample );
    TIFFSetDirectory( tif, (bim::uint16) tiff_page );

    // small safeguard
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    if (img->i.depth != bitspersample || img->i.width != width || img->i.height != height) continue;

    bim::uchar *p = (bim::uchar *) img->bits[ sample ];
    register bim::uint y = 0;

    for(y=0; y<img->i.height; y++) {
      xprogress( fmtHndl, y*(sample+1), img->i.height*img->i.samples, "Reading OME-TIFF" );
      if ( xtestAbort( fmtHndl ) == 1) break;  
      TIFFReadScanline(tif, p, y, 0);
      p += lineSize;
    } // for y
  }  // for sample

  //TIFFSetDirectory(tif, fmtHndl->pageNumber);
  return 0;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

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

        p += 15;
    }

    // channel names may also be stored in Logical channel in the Image
    // v >=5
    p = tag_270.find("<Channel ");
    if (p != std::string::npos)
    for (unsigned int i = 0; i < (bim::uint)ome->channels; ++i) {

        bim::xstring tag = tag_270.section("<Channel", ">", p);
        if (tag.size() <= 0) continue;
        tag = ometiff_normalize_xml_spaces(tag);
        bim::xstring medium = tag.section(" Name=\"", "\"");
        if (medium.size() <= 0) continue;
        bim::xstring index = tag.section(" ID=\"Channel:0:", "\"");
        int chan = index.toInt(i);
        bim::xstring wavelength = tag.section(" ExcitationWavelength=\"", "\"");

        if (wavelength.size() > 0)
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium + " - " + wavelength + "nm");
        else
            hash->append_tag(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), chan), medium);

        p += 8;
    }


    // the preferred mapping seems to be the default order
    /*
    if ( fvi->display_lut.size() == 3 ) {
    hash->append_tag( "display_channel_red",   fvi->display_lut[0] );
    hash->append_tag( "display_channel_green", fvi->display_lut[1] );
    hash->append_tag( "display_channel_blue",  fvi->display_lut[2] );
    }
    */

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
  str += bim::xstring::xprintf("<Image ID=\"Image:0\" Name=\"%s\">", "bioimage.ome.tif" );

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
          if (hash->hasKey(key)) {
              str += bim::xstring::xprintf("<Channel ID=\"Channel:0:%d\" Name=\"%s\" SamplesPerPixel=\"1\">",
                  i, hash->get_value(key).c_str());
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
      std::map<std::string, std::string>::const_iterator it;
      for (it = hash->begin(); it != hash->end(); ++it) {
          bim::xstring key = (*it).first;
          if (key.startsWith(bim::CUSTOM_TAGS_PREFIX) && key != "custom/Image Description") {
              str += bim::xstring::xprintf("<XMLAnnotation ID=\"Annotation:%d\" Namespace=\"openmicroscopy.org/OriginalMetadata\">", i);
              str += "<Value xmlns=\"\"><OriginalMetadata>";
              // dima: here we should be encoding strings into utf-8, though bioformats seems to expect latin1, skip encoding for now
              str += bim::xstring::xprintf("<Key>%s</Key>", key.right(7).c_str());
              str += bim::xstring::xprintf("<Value>%s</Value>", (*it).second.c_str());
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
  bim::TagList *tagList = &fmtHndl->metaData;

  if (tagList->count>0 && tagList->tags)
  for (bim::uint i=0; i<tagList->count; i++) {
    bim::TagItem *tagItem = &tagList->tags[i];
    
    if (tagItem->tagGroup == bim::META_GENERIC && tagItem->tagId == bim::METADATA_TAGS ) {
      bim::TagMap *hash = (bim::TagMap *) tagItem->tagData;
      std::string xml = constructOMEXML( fmtHndl, hash );
      TIFFSetField( tif, TIFFTAG_IMAGEDESCRIPTION, xml.c_str() );
      return 0;
    }

    if (tagItem->tagGroup == bim::META_GENERIC && tagItem->tagId == bim::METADATA_OMEXML ) {
      std::string *xml = (std::string *) tagItem->tagData;
      TIFFSetField( tif, TIFFTAG_IMAGEDESCRIPTION, xml->c_str() );
      return 0;
    }
  }

  std::string xml = constructOMEXML( fmtHndl, 0 );
  TIFFSetField( tif, TIFFTAG_IMAGEDESCRIPTION, xml.c_str());

  return 0;
}

//****************************************************************************
// OME-TIFF WRITER
//****************************************************************************

int omeTiffWritePlane(bim::FormatHandle *fmtHndl, bim::TiffParams *tifParams) {
  TIFF *out = tifParams->tiff;
  bim::ImageBitmap *img = fmtHndl->image;
 
  bim::uint32 height;
  bim::uint32 width;
  bim::uint32 rowsperstrip = (bim::uint32) -1;
  bim::uint16 bitspersample;
  bim::uint16 samplesperpixel = 1;
  bim::uint16 photometric = PHOTOMETRIC_MINISBLACK;
  bim::uint16 compression;
  bim::uint16 planarConfig = PLANARCONFIG_SEPARATE;	// separated planes 


  // samples in OME-TIFF are stored in separate IFDs 
  for (bim::uint sample=0; sample<img->i.samples; sample++) {

    width = (bim::uint32) img->i.width;
    height = (bim::uint32) img->i.height;
    bitspersample = img->i.depth;
    //samplesperpixel = img->i.samples;

    if ( bitspersample==1 && samplesperpixel==1 ) photometric = PHOTOMETRIC_MINISWHITE;

    // handle standard width/height/bpp stuff
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitspersample);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, planarConfig);	// separated planes
    TIFFSetField(out, TIFFTAG_SOFTWARE, "DIMIN TIFF WRAPPER <www.dimin.net>");

    // set pixel format
    bim::uint16 sampleformat = SAMPLEFORMAT_UINT;
    if (img->i.pixelType == bim::FMT_SIGNED) sampleformat = SAMPLEFORMAT_INT;
    if (img->i.pixelType == bim::FMT_FLOAT)  sampleformat = SAMPLEFORMAT_IEEEFP;
    TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, sampleformat);

    //if( TIFFGetField( out, TIFFTAG_DOCUMENTNAME, &pszText ) )
    //if( TIFFGetField( out, TIFFTAG_DATETIME, &pszText ) )

    //------------------------------------------------------------------------------  
    // compression
    //------------------------------------------------------------------------------  

    compression = fmtHndl->compression;
    if (compression == 0) compression = COMPRESSION_NONE; 

    switch(bitspersample) {
    case 1  :
      if (compression != COMPRESSION_CCITTFAX4) compression = COMPRESSION_NONE;
      break;

    case 8  :
    case 16 :
    case 32 :
    case 64 :
      if ( (compression != COMPRESSION_LZW) && (compression != COMPRESSION_PACKBITS) )
        compression = COMPRESSION_NONE;  
      break;
    
    default :
      compression = COMPRESSION_NONE;
      break;
    }

    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);

    unsigned long strip_size = bim::max<unsigned long>( TIFFDefaultStripSize(out,-1), 1 );
    switch ( compression ) {
      case COMPRESSION_JPEG: {
        TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size+(16-(strip_size % 16)) );
        break;
      }
      case COMPRESSION_ADOBE_DEFLATE: {
        TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
        if ( (photometric == PHOTOMETRIC_RGB) ||
             ((photometric == PHOTOMETRIC_MINISBLACK) && (bitspersample >= 8)) )
          TIFFSetField( out, TIFFTAG_PREDICTOR, 2 );
        TIFFSetField( out, TIFFTAG_ZIPQUALITY, 9 );
        break;
      }
      case COMPRESSION_CCITTFAX4: {
        TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, height );
        break;
      }
      case COMPRESSION_LZW: {
        TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
        if (planarConfig == PLANARCONFIG_SEPARATE)
           TIFFSetField( out, TIFFTAG_PREDICTOR, PREDICTOR_NONE );
        else
           TIFFSetField( out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL );
        break;
      }
      default: {
        TIFFSetField( out, TIFFTAG_ROWSPERSTRIP, strip_size );
        break;
      }
    }

    //------------------------------------------------------------------------------
    // writing meta data
    //------------------------------------------------------------------------------
    if (fmtHndl->pageNumber == 0 && sample == 0 )
      write_omeTiff_metadata (fmtHndl, tifParams);

    //------------------------------------------------------------------------------
    // writing image
    //------------------------------------------------------------------------------
    bim::uchar *bits = (bim::uchar *) img->bits[sample];
    bim::uint line_size = getLineSizeInBytes( img );
    for (bim::uint32 y=0; y<height; y++) {
      xprogress( fmtHndl, y*(sample+1), height*img->i.samples, "Writing OME-TIFF" );
      if ( xtestAbort( fmtHndl ) == 1) break;  
      TIFFWriteScanline(out, bits, y, 0);
      bits += line_size;
    } // for y

    TIFFWriteDirectory( out );
  } // for sample

  TIFFFlushData(out);
  TIFFFlush(out);
  return 0;
}


