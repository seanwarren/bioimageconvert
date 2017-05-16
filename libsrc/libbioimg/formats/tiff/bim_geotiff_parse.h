/*******************************************************************************

  GeoTIFF metadata extraction
 
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    12/11/2014 5:38:23 PM - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_GEOTIFF_PARSE_H
#define BIM_GEOTIFF_PARSE_H

#include <vector>

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>

// appends all tags found by GeoTIFF
void geotiff_append_metadata (bim::FormatHandle *fmtHndl, bim::TagMap *hash );

typedef struct tiff TIFF;

bool isGeoTiff(TIFF *tif);
int GTIFFromBuffer(const std::vector<char> &buffer, TIFF *out);
int BufferFromGTIF(TIFF *in, std::vector<char> &buffer);

#endif // BIM_GEOTIFF_PARSE_H
