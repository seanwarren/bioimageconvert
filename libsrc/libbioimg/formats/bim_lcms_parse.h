/*****************************************************************************
LCMS2 metadata parsing generic functions
Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
2015-08-07 10:12:40 - First creation

ver : 1
*****************************************************************************/

#ifndef BIM_LCMS_PARSE_H
#define BIM_LCMS_PARSE_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

void lcms_append_metadata (bim::FormatHandle *fmtHndl, bim::TagMap *hash );

bim::ImageModes lcms_image_mode(const std::string &s);

#endif // BIM_LCMS_PARSE_H
