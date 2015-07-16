/*****************************************************************************
  JPEG2000 support
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION

  Programmer: Mario Emmenlauer <mario@emmenlauer.de>

  History:
  04/19/2015 14:20 - First creation

  Ver : 1
  *****************************************************************************/
#ifndef BIM_JP2_DECOMPRESS_H
#define BIM_JP2_DECOMPRESS_H BIM_JP2_DECOMPRESS_H

#include "bim_jp2_format.h"

// JPEG2000 magic numbers, taken from openjpeg-2.1.0
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
// position 45: "\xff\x52" 
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"


int read_jp2_image(bim::FormatHandle *fmtHndl);

#endif
