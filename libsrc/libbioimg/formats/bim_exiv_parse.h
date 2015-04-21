/*******************************************************************************

  Memory IO
 
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
  
  Based on memio used by GDAL and libtiff by:
      Mike Johnson - Banctec AB
      Frank Warmerdam, warmerdam@pobox.com

  History:
    08/05/2004 13:57 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_EXIV_PARSE_H
#define BIM_EXIV_PARSE_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// appends all tags found by EXIV2
void exiv_append_metadata (bim::FormatHandle *fmtHndl, bim::TagMap *hash );

#endif // BIM_EXIV_PARSE_H
