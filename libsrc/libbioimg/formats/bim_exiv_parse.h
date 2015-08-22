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

// appends all tags found by EXIV2
void exiv_append_metadata (bim::FormatHandle *fmtHndl, bim::TagMap *hash );

#endif // BIM_EXIV_PARSE_H
