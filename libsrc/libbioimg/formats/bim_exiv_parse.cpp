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
#include <exiv2/tiffimage.hpp>

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_exiv_parse.h"

#include <tiffio.h>
#include <tiffiop.h>

using namespace bim;


static double datum2coord(const Exiv2::Exifdatum &coord, const Exiv2::Exifdatum &ref) {
    double d = coord.toRational(0).first / (double)coord.toRational(0).second;
    double m = coord.toRational(1).first / (double)coord.toRational(1).second;
    double s = coord.toRational(2).first / (double)coord.toRational(2).second;
    std::string r = ref.toString();
    double h = (r == "W" || r == "S") ? -1.0 : 1.0;
    return (d + m / 60.0 + s / 3600.0) * h;
}

static double datum2alt(const Exiv2::Exifdatum &coord, const Exiv2::Exifdatum &ref) {
    if (coord.count() < 1) return 0.0;
    double h = coord.toRational(0).first / (double)coord.toRational(0).second;
    int r = ref.toLong();
    double l = (r == 1) ? -1.0 : 1.0;
    return h * l;
}

static void exif2hash(Exiv2::ExifData &exifData, TagMap *hash) {
    if (exifData.empty()) return;

    Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
        if (i->typeId() == Exiv2::undefined) continue;
        if (i->typeId() == Exiv2::unsignedByte && i->size() > 1024) continue;
        if (i->typeId() >= Exiv2::unsignedShort && i->typeId() <= Exiv2::tiffIfd && i->size() > 1024) continue;
        xstring mykey = i->key();
        mykey = mykey.replace(".", "/");
        xstring myval = i->print();
        hash->set_value(mykey, myval);
    }

    // write Geo coordinates
    Exiv2::Exifdatum lat = exifData["Exif.GPSInfo.GPSLatitude"];
    Exiv2::Exifdatum lon = exifData["Exif.GPSInfo.GPSLongitude"];
    if (lat.count() >= 3 && lon.count() >= 3) {
        double c1 = datum2coord(lat, exifData["Exif.GPSInfo.GPSLatitudeRef"]);
        double c2 = datum2coord(lon, exifData["Exif.GPSInfo.GPSLongitudeRef"]);
        double c3 = datum2alt(exifData["Exif.GPSInfo.GPSAltitude"], exifData["Exif.GPSInfo.GPSAltitudeRef"]);
        xstring v = xstring::xprintf("%f,%f,%f", c1, c2, c3);
        hash->set_value("Geo/Coordinates/center", v);
    }
}

static void iptc2hash(const Exiv2::IptcData &iptcData, TagMap *hash) {
    if (!iptcData.empty()) {
        Exiv2::IptcData::const_iterator end = iptcData.end();
        for (Exiv2::IptcData::const_iterator i = iptcData.begin(); i != end; ++i) {
            if (i->typeId() == Exiv2::undefined) continue;
            if (i->typeId() == Exiv2::unsignedByte && i->size() > 1024) continue;
            if (i->typeId() >= Exiv2::unsignedShort && i->typeId() <= Exiv2::tiffIfd && i->size() > 1024) continue;
            xstring mykey = i->key();
            mykey = mykey.replace(".", "/");
            xstring myval = i->print();
            hash->set_value(mykey, myval);
        }
    }
}

static void xmp2hash(const Exiv2::XmpData &xmpData, TagMap *hash) {
    if (!xmpData.empty()) {
        Exiv2::XmpData::const_iterator end = xmpData.end();
        for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
            if (i->typeId() == Exiv2::undefined) continue;
            if (i->typeId() == Exiv2::unsignedByte && i->size() > 1024) continue;
            if (i->typeId() >= Exiv2::unsignedShort && i->typeId() <= Exiv2::tiffIfd && i->size() > 1024) continue;
            xstring mykey = i->key();
            mykey = mykey.replace(".", "/");
            xstring myval = i->print();
            hash->set_value(mykey, myval);
        }
    }
}

static Exiv2::Image::AutoPtr init_image(FormatHandle *fmtHndl) {
    Exiv2::Image::AutoPtr image;
    if (!fmtHndl->fileName) return image;
    try {
#ifdef BIM_WIN
        bim::xstring fn(fmtHndl->fileName);
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fn.toUTF16());
#else
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fmtHndl->fileName);
#endif

        if (image.get() == 0) return Exiv2::Image::AutoPtr();
        image->readMetadata();
        return image;
    }
    catch (...) {
        return Exiv2::Image::AutoPtr();
    }
}

void exiv_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return;
    if (isCustomReading(fmtHndl)) return;
    if (!hash) return;

    if (hash->hasKey(bim::RAW_TAGS_EXIF) || hash->hasKey(bim::RAW_TAGS_IPTC) || hash->hasKey(bim::RAW_TAGS_XMP)) {
        // metadata block were decoded form the image

        // write Exif metadata
        if (hash->hasKey(bim::RAW_TAGS_EXIF) && hash->get_type(bim::RAW_TAGS_EXIF) == bim::RAW_TYPES_EXIF) {
            Exiv2::ExifData exifData;
            try {
                Exiv2::ExifParser::decode(exifData,
                    (const Exiv2::byte *) hash->get_value_bin(bim::RAW_TAGS_EXIF),
                    hash->get_size(bim::RAW_TAGS_EXIF));
                exif2hash(exifData, hash);
            } 
            catch (...) {}
        }

        // write IPTC metadata
        if (hash->hasKey(bim::RAW_TAGS_IPTC) && hash->get_type(bim::RAW_TAGS_IPTC) == bim::RAW_TYPES_IPTC) {
            Exiv2::IptcData iptcData;
            try {
                Exiv2::IptcParser::decode(iptcData,
                    (const Exiv2::byte *) hash->get_value_bin(bim::RAW_TAGS_IPTC),
                    hash->get_size(bim::RAW_TAGS_IPTC));
                iptc2hash(iptcData, hash);
            }
            catch (...) {}
        }

        // write XMP metadata
        if (hash->hasKey(bim::RAW_TAGS_XMP) && hash->get_type(bim::RAW_TAGS_XMP) == bim::RAW_TYPES_XMP) {
            Exiv2::XmpData xmpData;
            try {
                Exiv2::XmpParser::decode(xmpData, hash->get_value(bim::RAW_TAGS_XMP));
                xmp2hash(xmpData, hash);
            }
            catch (...) {}
        }

        // write Photoshop metadata
        /*if (hash->hasKey(bim::RAW_TAGS_PHOTOSHOP) && hash->get_type(bim::RAW_TAGS_PHOTOSHOP) == bim::RAW_TYPES_PHOTOSHOP) {
            error_code = PKImageEncode_SetPhotoshopMetadata_WMP(pEncoder, (U8*)hash->get_value_bin(bim::RAW_TAGS_PHOTOSHOP), hash->get_size(bim::RAW_TAGS_PHOTOSHOP));
        }*/
    } else {
        // blocks were not decoded form the image, try to read the file and find blocks
        Exiv2::Image::AutoPtr image = init_image(fmtHndl);
        if (image.get() == 0) return;
        exif2hash(image->exifData(), hash);
        iptc2hash(image->iptcData(), hash);
        xmp2hash(image->xmpData(), hash);
    }

    // fill in date time from exif
    if (hash->hasKey("Exif/Image/DateTime")) {
        xstring v = hash->get_value("Exif/Image/DateTime");
        // Exif export date/time as: YYYY:MM:DD hh:mm:ss
        if (v[4] == ':') v.replace(4, 1, "-");
        if (v[7] == ':') v.replace(7, 1, "-");
        hash->set_value(bim::IMAGE_DATE_TIME, v);
    }

}

//-----------------------------------------------------------------------------
// Support for TIFF blocks with EXIF and GPS IFDs
//-----------------------------------------------------------------------------

const int tiff_tag_sizes[19] = { 1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 0, 0, 0, 8, 8, 8 };

#define BIM_EXIF_NUM_TAGS 2

#pragma pack(push, 1)
typedef struct {
    bim::uint16 tag;    // tag identifying information for entry
    bim::uint16 type;   // data type of entry (for BigTIFF 17 types are defined, 0-13, 16-18)
    bim::uint32 count;  // count of elements for entry, in TIFF stored as uint32
    bim::uint32 offset; // data itself (if <= 64-bits) or offset to data for entry, in TIFF stored as uint32
} tiff_ifd_entry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    bim::uint16 tag;    // tag identifying information for entry
    bim::uint16 type;   // data type of entry (for BigTIFF 17 types are defined, 0-13, 16-18)
    bim::uint64 count;  // count of elements for entry, in TIFF stored as uint32
    bim::uint64 offset; // data itself (if <= 64-bits) or offset to data for entry, in TIFF stored as uint32
} bigtiff_ifd_entry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    bim::uint16 magic;     // 0x4D4D constan 
    bim::uint16 version;   // 0x002A version = standard TIFF
    bim::uint32 diroffset; // offset to first directory
    bim::uint16 count;     // number of directory entries, uint16 in TIFF and uint64 in BigTIFF
    tiff_ifd_entry entries[BIM_EXIF_NUM_TAGS];
    bim::uint32 next;  // offset to next directory or zero, uint32 in TIFF and uint64 in BigTIFF
} tiff_buffer;
#pragma pack(pop)

const void tiff_parse_ifd_update_offsets(unsigned char *buf, unsigned int old_offset, unsigned int new_offset) {
    bim::uint16 count = *(bim::uint16*) buf;
    buf += sizeof(bim::uint16);
    for (int i = 0; i < count; ++i) {
        tiff_ifd_entry *e = (tiff_ifd_entry*)buf;
        if (e->count * tiff_tag_sizes[e->type] > 4) {
            // if offset is stored in the offset field, then update
            e->offset = e->offset - old_offset + new_offset;
        }
        buf += sizeof(tiff_ifd_entry);
    }
}

void create_tiff_exif_block(const std::vector<char> &exif, unsigned int offset_exif, const std::vector<char> &gps, unsigned int offset_gps, TagMap *hash) {
    if (exif.size() == 0 && gps.size() == 0) return;

    unsigned int bufsz = sizeof(tiff_buffer);
    unsigned int offset_exif_new = 0;
    unsigned int offset_exifgps_new = 0;

    if (exif.size() > 0) {
        bufsz += exif.size();
        offset_exif_new = sizeof(tiff_buffer);
        tiff_parse_ifd_update_offsets((unsigned char *)&exif[0], offset_exif, offset_exif_new);
    }

    if (gps.size() > 0) {
        int align = (bufsz % 2) ? 1 : 0; // offset must be even, word aligned
        offset_exifgps_new = bufsz + align;
        bufsz += gps.size() + align;
        tiff_parse_ifd_update_offsets((unsigned char *)&gps[0], offset_gps, offset_exifgps_new);
    }

    // create mini TIFF stream
    tiff_buffer header;
    header.magic = 0x4949;
    header.version = 0x002A;
    header.diroffset = 8;
    header.count = BIM_EXIF_NUM_TAGS;
    header.entries[0].tag = TIFFTAG_EXIFIFD;
    header.entries[0].type = TIFF_LONG;
    header.entries[0].count = 1;
    header.entries[0].offset = offset_exif_new;
    header.entries[1].tag = TIFFTAG_GPSIFD;
    header.entries[1].type = TIFF_LONG;
    header.entries[1].count = 1;
    header.entries[1].offset = offset_exifgps_new;
    header.next = 0;

    std::vector<char> buffer(bufsz, 0);
    memcpy(&buffer[0], &header, sizeof(header));

    if (exif.size() > 0)
        memcpy(&buffer[0] + offset_exif_new, &exif[0], exif.size());

    if (gps.size() > 0)
        memcpy(&buffer[0] + offset_exifgps_new, &gps[0], gps.size());

    hash->set_value(bim::RAW_TAGS_EXIF, buffer, bim::RAW_TYPES_EXIF);
}


template <typename Count, class Entry, typename next>
void sub_ifd_to_buffer(toff_t offset, TIFF *tif, std::vector<char> &buffer) {
    Count count = 0; 
    tif->tif_seekproc((thandle_t)tif->tif_fd, offset, SEEK_SET);
    if (tif->tif_readproc((thandle_t)tif->tif_fd, &count, sizeof(Count)) < sizeof(Count)) return;
    if (tif->tif_flags & TIFF_SWAB) {
        if (tif->tif_flags & TIFF_BIGTIFF)
            TIFFSwabLong8((bim::uint64*)&count);
        else
            TIFFSwabShort((bim::uint16*) &count);
    }

    unsigned int header_size = sizeof(Count)+sizeof(next)+sizeof(Entry)*count;
    unsigned int payload_begin = offset + header_size;
    unsigned int payload_end = payload_begin;

    std::vector<Entry> entries(count);
    std::vector<tiff_ifd_entry> oentries(count);
    if (tif->tif_readproc((thandle_t)tif->tif_fd, &entries[0], sizeof(Entry)*count) < sizeof(Entry)*count) return;
    for (int i = 0; i < count; ++i) {
        Entry *e = &entries[i];
        tiff_ifd_entry *o = &oentries[i];
        // swab if needed
        if (tif->tif_flags & TIFF_SWAB) {
            if (tif->tif_flags & TIFF_BIGTIFF) {
                TIFFSwabShort((bim::uint16*)&e->tag);
                TIFFSwabShort((bim::uint16*)&e->type);
                TIFFSwabLong8((bim::uint64*) &e->count);
                TIFFSwabLong8((bim::uint64*)&e->offset);
            } else {
                TIFFSwabShort((bim::uint16*)&e->tag);
                TIFFSwabShort((bim::uint16*)&e->type);
                TIFFSwabLong((bim::uint32*)&e->count);
                TIFFSwabLong((bim::uint32*)&e->offset);
            }
        }

        // find size of payload area
        unsigned int sz = e->count * tiff_tag_sizes[e->type];
        unsigned int off = sz > 4 ? e->offset + sz: 0;
        if (off > payload_end) payload_end = off;
        o->tag = e->tag;
        o->type = e->type;
        o->count = e->count;
        o->offset = e->offset - offset;
    }

    unsigned int payload_size = payload_end - payload_begin;

    bim::uint16 out_count = count;
    bim::uint32 out_next = 0;
    unsigned int outsz = sizeof(bim::uint16) + sizeof(tiff_ifd_entry)*count + sizeof(bim::uint32) + payload_size;
    buffer.resize(outsz);
    unsigned int pos = 0;
    memcpy(&buffer[0] + pos, &out_count, sizeof(bim::uint16));
    pos += sizeof(bim::uint16);
    memcpy(&buffer[0] + pos, &oentries[0], sizeof(tiff_ifd_entry)*count);
    pos += sizeof(tiff_ifd_entry)*count;
    memcpy(&buffer[0] + pos, &out_next, sizeof(bim::uint32));
    pos += sizeof(bim::uint32);

    tif->tif_seekproc((thandle_t)tif->tif_fd, payload_begin, SEEK_SET);
    if (tif->tif_readproc((thandle_t)tif->tif_fd, &buffer[0] + pos, payload_size) < payload_size) {
        buffer.resize(0);
    }
}

void tiff_exif_to_buffer(TIFF *tif, bim::TagMap *hash) {
    std::vector<char> exif;
    std::vector<char> gps;
    toff_t offset_exif = 0;
    toff_t offset_gps = 0;

    if (TIFFGetField(tif, TIFFTAG_EXIFIFD, &offset_exif)) {
        if ((tif->tif_flags & TIFF_BIGTIFF))
            sub_ifd_to_buffer<bim::uint64, bigtiff_ifd_entry, bim::uint64>(offset_exif, tif, exif);
        else
            sub_ifd_to_buffer<bim::uint16, tiff_ifd_entry, bim::uint32>(offset_exif, tif, exif);
    }

    if (TIFFGetField(tif, TIFFTAG_GPSIFD, &offset_gps)) {
        if ((tif->tif_flags & TIFF_BIGTIFF))
            sub_ifd_to_buffer<bim::uint64, bigtiff_ifd_entry, bim::uint64>(offset_gps, tif, gps);
        else
            sub_ifd_to_buffer<bim::uint16, tiff_ifd_entry, bim::uint32>(offset_gps, tif, gps);
    }

    // create metadata TIFF buffer
    create_tiff_exif_block(exif, 0, gps, 0, hash);
}

void buffer_to_tiff_exif(bim::TagMap *hash, TIFF *tif) {

}
