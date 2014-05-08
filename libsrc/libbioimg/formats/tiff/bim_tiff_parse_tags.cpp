/*****************************************************************************
  TIFF TAG PARSING FUNCTIONS 
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    04/19/2004 14:31 - First creation
        
  Ver : 1
*****************************************************************************/

#include "bi_ifs.h"

#include <stdio.h>
#include <string.h>

#include "formats/bim_img_format_interface.h"
#include "formats/tiff/bim_tiff_format.h"

//------------------------------------------------------------------------------
// FLUOVIEW
//------------------------------------------------------------------------------

void parse_fluoview_285 (char *buf, meta_data* md)
{
  int i=0;
  char *line;
  float xr=0.0, yr=0.0;

  line = strstr( buf, "Resolution" );

  if (line != NULL) {
    sscanf( line, "Resolution\t%f\t%f", &xr, &yr );
    md->pixel_size_x = xr;
    md->pixel_size_y = yr;
  }
}

void parse_fluoview_270_dateTime (char *buf, meta_data* md)
{
  int i=0;
  char *line;
  int d=0, m=0, y=0, h=0, mi=0, s=0;
  
  //Date=02-17-2004
  line = strstr( buf, "Date=" );
  if (line != NULL)
    sscanf( line, "Date=%d-%d-%d", &m, &d, &y );

  //Time=11:54:50
  line = strstr( buf, "Time=" );
  if (line != NULL)
    sscanf( line, "Time=%d:%d:%d", &h, &mi, &s );

  md->imaging_time = new char [20];
  sprintf(md->imaging_time, "%.2d-%.2d-%.4d %.2d:%.2d:%.2d", d, m, y, h, mi, s);
}

void read_meta_data(FormatHandle *fmtParams, meta_data* md)
{
  TagList *tagList = &fmtParams->metaData;
  int pos;

  if (tagList == NULL) return;
  if (tagList->count == 0) return;
  if (tagList->tags == NULL) return;  
  
  //FILE *in_stream = fopen("tag270.txt", "wb");
  //fwrite(tagList->tags[pos].tagData, tagList->tags[pos].tagLength, 1, in_stream);
  //fclose(in_stream);


  //----------------------------------------------------------------------
  // STK
  //----------------------------------------------------------------------
  if (isTagPresent( tagList, 33628 ) == true)
  {
    int n = 1;

    // read tag 306 - Date/Time
    if ((pos = tagPos(tagList, 306)) != -1)
    {
      md->imaging_time = new char [tagList->tags[pos].tagLength+1];
      strncpy(md->imaging_time, (char *) tagList->tags[pos].tagData, tagList->tags[pos].tagLength);
      md->imaging_time[tagList->tags[pos].tagLength] = 0;
    }

    // read tag 270 - image description
    if ((pos = tagPos(tagList, 33628)) != -1)
    {
      //md->imaging_time = new char [tagList->tags[pos].tagLength+1];
      //strncpy(md->imaging_time, (char *) tagList->tags[pos].tagData, tagList->tags[pos].tagLength);

      //FILE *in_stream = fopen("meta.txt", "wb");
      //fwrite(tagList->tags[pos].tagData, tagList->tags[pos].tagLength, 1, in_stream);
      //fclose(in_stream);
      //parse_stk_uic1 ( (unsigned char *) tagList->tags[pos].tagData, tagList->tags[pos].tagLength, 1, md);

      md->pixel_size_x = 0;
      md->pixel_size_y = 0;
    }    

    return;
  }

  //----------------------------------------------------------------------
  // PSIA
  //----------------------------------------------------------------------
  if (isTagPresent( tagList, 50432 ) == true)
  {
    if ((pos = tagPos(tagList, 50435)) != -1)
    {
      psiaImageHeader *psia_head = (psiaImageHeader *) tagList->tags[pos].tagData;

      md->pixel_size_x = psia_head->dfXScanSize;
      md->pixel_size_y = psia_head->dfYScanSize;
    }

    return;
  }

  //----------------------------------------------------------------------
  // Fluoview
  //----------------------------------------------------------------------
  if (isTagPresent( tagList, 34361 ) == true)
  {
    if ((pos = tagPos(tagList, 270)) != -1)
      parse_fluoview_270 ( (char *) tagList->tags[pos].tagData, md );

    if ((pos = tagPos(tagList, 285)) != -1)
      parse_fluoview_285 ( (char *) tagList->tags[pos].tagData, md );

    return;
  }

  // regular tiff
  // read tag 306 - Date/Time
  if ((pos = tagPos(tagList, 306)) != -1)
  {
    md->imaging_time = new char [tagList->tags[pos].tagLength+1];
    strncpy(md->imaging_time, (char *) tagList->tags[pos].tagData, tagList->tags[pos].tagLength);
    md->imaging_time[tagList->tags[pos].tagLength] = 0;
  }

  
}



