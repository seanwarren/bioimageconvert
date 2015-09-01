/*****************************************************************************
EXIV2 metadata parsing generic functions
Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
2013-01-12 14:13:40 - First creation

ver : 1
*****************************************************************************/

#ifndef BIM_EXIV_PARSE_H
#define BIM_EXIV_PARSE_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

void exiv_append_metadata (bim::FormatHandle *fmtHndl, bim::TagMap *hash );
void exiv_write_metadata(bim::TagMap *hash, bim::FormatHandle *fmtHndl);

//-----------------------------------------------------------------------------
// Support for TIFF blocks with EXIF and GPS IFDs
//-----------------------------------------------------------------------------

void create_tiff_exif_block(const std::vector<char> &exif, unsigned int offset_exif, 
                            const std::vector<char> &gps, unsigned int offset_gps, 
                            bim::TagMap *hash);

void extract_exif_gps_blocks(bim::TagMap *hash, std::vector<char> &exif, std::vector<char> &gps );

typedef struct tiff TIFF;

void tiff_exif_to_buffer(TIFF *tif, bim::TagMap *hash);
// unfortunately not used due to libtiff not supporting tag namespaces
// although it is possible to write EXIF it is not possible to write EXIF-GPS tags
// due to namespace clashes, use EXIV2 instead after the file is closed
// this function simply pre-sets EXIF and GPS tags poiting to 0
void buffer_to_tiff_exif(bim::TagMap *hash, TIFF *tif);

#endif // BIM_EXIV_PARSE_H
