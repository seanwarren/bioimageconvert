/*****************************************************************************
  EXIV2 metadata parsing generic functions
  Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_exiv_parse.h"

using namespace bim;

void exiv_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return;
  if (isCustomReading (fmtHndl)) return;

  // use EXIV2 to read metadata
  if (!hash) return;
  if (!fmtHndl->fileName) return;
  try {
#ifdef BIM_WIN
      bim::xstring fn(fmtHndl->fileName);
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fn.toUTF16());
#else
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fmtHndl->fileName);
#endif

      if (image.get()==0) return;
      image->readMetadata();

      Exiv2::ExifData &exifData = image->exifData();
      if (!exifData.empty()) {
          Exiv2::ExifData::const_iterator end = exifData.end();
          for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
              if (i->typeId()==Exiv2::undefined) continue;
              if (i->typeId()==Exiv2::unsignedByte && i->size()>1024) continue;
              if (i->typeId()>=Exiv2::unsignedShort && i->typeId()<=Exiv2::tiffIfd && i->size()>1024) continue;
              xstring mykey = i->key();
              mykey = mykey.replace(".", "/");
              xstring myval = i->print();
              hash->set_value( mykey, myval );
          }
      }

      Exiv2::IptcData &iptcData = image->iptcData();
      if (!iptcData.empty()) {
          Exiv2::IptcData::iterator end = iptcData.end();
          for (Exiv2::IptcData::iterator i = iptcData.begin(); i != end; ++i) {
              if (i->typeId()==Exiv2::undefined) continue;
              if (i->typeId()==Exiv2::unsignedByte && i->size()>1024) continue;
              if (i->typeId()>=Exiv2::unsignedShort && i->typeId()<=Exiv2::tiffIfd && i->size()>1024) continue;
              xstring mykey = i->key();
              mykey = mykey.replace(".", "/");
              xstring myval = i->print();
              hash->set_value( mykey, myval );
          }
      }

      Exiv2::XmpData &xmpData = image->xmpData();
      if (!xmpData.empty()) {
          Exiv2::XmpData::iterator end = xmpData.end();
          for (Exiv2::XmpData::iterator i = xmpData.begin(); i != end; ++i) {
              if (i->typeId()==Exiv2::undefined) continue;
              if (i->typeId()==Exiv2::unsignedByte && i->size()>1024) continue;
              if (i->typeId()>=Exiv2::unsignedShort && i->typeId()<=Exiv2::tiffIfd && i->size()>1024) continue;
              xstring mykey = i->key();
              mykey = mykey.replace(".", "/");
              xstring myval = i->print();
              hash->set_value( mykey, myval );
          }
      }

  } catch(...) {
      return;
  }

  // fill in date time from exif
  if (hash->hasKey("Exif/Image/DateTime")) {
      xstring v = hash->get_value( "Exif/Image/DateTime" );
      // Exif export date/time as: YYYY:MM:DD hh:mm:ss
      if (v[4]==':') v.replace(4, 1, "-");
      if (v[7]==':') v.replace(7, 1, "-");
      hash->set_value( bim::IMAGE_DATE_TIME, v );
  }

}


