/*****************************************************************************
  JPEG2000 support 
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION
  
  Programmer: Mario Emmenlauer <mario@emmenlauer.de>

  History:
    04/19/2015 14:20 - First creation
        
  Ver : 1

******************************************************************************
  This code is inspired by the code in file opj_decompress.c of the
  openjpeg library version 2.1.0. To comply with the original copyright,
  the following terms apply to this file:
******************************************************************************

  The copyright in this software is being made available under the 2-clauses 
  BSD License, included below. This software may be subject to other third 
  party and contributor rights, including patent rights, and no such rights
  are granted under this license.

  Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
  Copyright (c) 2002-2014, Professor Benoit Macq
  Copyright (c) 2001-2003, David Janssens
  Copyright (c) 2002-2003, Yannick Verschueren
  Copyright (c) 2003-2007, Francois-Olivier Devaux 
  Copyright (c) 2003-2014, Antonin Descampe
  Copyright (c) 2005, Herve Drolon, FreeImage Team
  Copyright (c) 2006-2007, Parvatha Elangovan
  Copyright (c) 2008, 2011-2012, Centre National d'Etudes Spatiales (CNES), FR 
  Copyright (c) 2012, CS Systemes d'Information, France
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <bim_image.h>

#include "bim_jp2_decompress.h"
#include "bim_jp2_format.h"

#include <openjpeg.h>

#ifdef OPJ_HAVE_LIBLCMS2
#include <lcms2.h>
#endif
#ifdef OPJ_HAVE_LIBLCMS1
#include <lcms.h>
#endif
#include "bim_jp2_color.h"

#include <format_defs.h>



int openjpeg_image_to_bim_image(opj_image_t* opj_image, bim::ImageBitmap* bim_image)
{
  size_t width, height, imgsize;
  size_t index;
  int bps, sgnd, sgndadjust;
  int ushift, dshift, has_alpha, force16;

  ushift = dshift = force16 = has_alpha = 0;
  bps = (int)opj_image->comps[0].prec;

  if (bps > 8 && bps < 16) {
    ushift = 16 - bps;
    dshift = bps - ushift;
    bps = 16;
    force16 = 1;
  }

  if (bps != 8 && bps != 16) {
    std::cerr << "openjpeg_image_to_bim_image(): Found bits=" << bps
            << ", but only 8 and 16 bits implemented. Aborted." << std::endl;
    return 1;
  }

  sgnd = (int)opj_image->comps[0].sgnd;
  sgndadjust = sgnd ? 1 << (opj_image->comps[0].prec - 1) : 0;

  width = (int)opj_image->comps[0].w;
  height = (int)opj_image->comps[0].h;
  imgsize = width * height;


/*****************************************************
  // TODO FIXME FINISH HERE:
  if (cinfo->output_components == 1) {
    while (cinfo->output_scanline < cinfo->output_height) {
      row_pointer[0] =  ((uchar *) bim_image->bits[0]) + (cinfo->output_width*cinfo->output_scanline);
      (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
    }
  }

  if ( cinfo->output_components == 3 ) {
    row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];
    while (cinfo->output_scanline < cinfo->output_height) {
      register unsigned int i;        
      (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
      uchar *row = row_pointer[0];        
      uchar* pix1 = ((uchar *) bim_image->bits[0]) + cinfo->output_width * (cinfo->output_scanline-1);
      uchar* pix2 = ((uchar *) bim_image->bits[1]) + cinfo->output_width * (cinfo->output_scanline-1);
      uchar* pix3 = ((uchar *) bim_image->bits[2]) + cinfo->output_width * (cinfo->output_scanline-1);

      for (size_t i=0; i<cinfo->output_width; i++) {
        *pix1++ = *row++;
        *pix2++ = *row++;
        *pix3++ = *row++;
      }
    } // while scanlines
    delete row_pointer[0];
  }

  if ( cinfo->output_components == 4 ) {
    row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];
    while (cinfo->output_scanline < cinfo->output_height) {
      register unsigned int i;        
      (void) jpeg_read_scanlines( cinfo, row_pointer, 1 );
      uchar *row = row_pointer[0];        
      uchar* pix1 = ((uchar *) bim_image->bits[0]) + cinfo->output_width * (cinfo->output_scanline-1);
      uchar* pix2 = ((uchar *) bim_image->bits[1]) + cinfo->output_width * (cinfo->output_scanline-1);
      uchar* pix3 = ((uchar *) bim_image->bits[2]) + cinfo->output_width * (cinfo->output_scanline-1);
      uchar* pix4 = ((uchar *) bim_image->bits[3]) + cinfo->output_width * (cinfo->output_scanline-1);

      for (size_t i=0; i<cinfo->output_width; i++) {
        *pix1++ = *row++;
        *pix2++ = *row++;
        *pix3++ = *row++;
        *pix4++ = *row++;
      }
    } // while scanlines
    delete row_pointer[0];
  }

****************************************************

  // TODO FIXME FINISH HERE:
  if (opj_image->numcomps >= 3
      && opj_image->comps[0].dx == opj_image->comps[1].dx
      && opj_image->comps[1].dx == opj_image->comps[2].dx
      && opj_image->comps[0].dy == opj_image->comps[1].dy
      && opj_image->comps[1].dy == opj_image->comps[2].dy
      && opj_image->comps[0].prec == opj_image->comps[1].prec
      && opj_image->comps[1].prec == opj_image->comps[2].prec) {
    has_alpha = (opj_image->numcomps == 4);

    if (bim::allocImg( bim_image, width, height, opj_image->numcomps, bps ) != 0) {
      return 1;
    }


    buf = _TIFFmalloc(strip_size);
    index = 0;

    for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++) {
      unsigned char* dat8;
      tsize_t i, ssize, last_i = 0;
      int step, restx;
      ssize = TIFFStripSize(tif);
      dat8 = (unsigned char*)buf;

      if (bps == 8) {
	step = 3 + has_alpha;
	restx = step - 1;

	for (size_t i = 0; i < ssize - restx; i += step) {
	  int r, g, b, a = 0;

	  if (index < imgsize) {
	    r = opj_image->comps[0].data[index];
	    g = opj_image->comps[1].data[index];
	    b = opj_image->comps[2].data[index];
	    if (has_alpha)
	      a = opj_image->comps[3].data[index];

	    if (sgnd) {
	      r += sgndadjust;
	      g += sgndadjust;
	      b += sgndadjust;
	      if (has_alpha)
		a += sgndadjust;
	    }
	    if (r > 255)
	      r = 255;
	    else if (r < 0)
	      r = 0;
	    dat8[i + 0] = (unsigned char)r;
	    if (g > 255)
	      g = 255;
	    else if (g < 0)
	      g = 0;
	    dat8[i + 1] = (unsigned char)g;
	    if (b > 255)
	      b = 255;
	    else if (b < 0)
	      b = 0;
	    dat8[i + 2] = (unsigned char)b;
	    if (has_alpha) {
	      if (a > 255)
		a = 255;
	      else if (a < 0)
		a = 0;
	      dat8[i + 3] = (unsigned char)a;
	    }

	    index++;
	    last_i = i + step;
	  } else
	    break;
	} // for(i = 0;)

	if (last_i < ssize) {
	  for (size_t i = last_i; i < ssize; i += step) {
	    int r, g, b, a = 0;

	    if (index < imgsize) {
	      r = opj_image->comps[0].data[index];
	      g = opj_image->comps[1].data[index];
	      b = opj_image->comps[2].data[index];
	      if (has_alpha)
		a = opj_image->comps[3].data[index];

	      if (sgnd) {
		r += sgndadjust;
		g += sgndadjust;
		b += sgndadjust;
		if (has_alpha)
		  a += sgndadjust;
	      }
	      if (r > 255)
		r = 255;
	      else if (r < 0)
		r = 0;
	      if (g > 255)
		g = 255;
	      else if (g < 0)
		g = 0;
	      if (b > 255)
		b = 255;
	      else if (b < 0)
		b = 0;

	      dat8[i + 0] = (unsigned char)r;
	      if (i + 1 < ssize)
		dat8[i + 1] = (unsigned char)g;
	      else
		break;
	      if (i + 2 < ssize)
		dat8[i + 2] = (unsigned char)b;
	      else
		break;
	      if (has_alpha) {
		if (a > 255)
		  a = 255;
		else if (a < 0)
		  a = 0;

		if (i + 3 < ssize)
		  dat8[i + 3] = (unsigned char)a;
		else
		  break;
	      }
	      index++;
	    } else
	      break;
	  } // for(i)
	} // if(last_i < ssize)
      }   // if(bps == 8)
      else if (bps == 16) {
	step = 6 + has_alpha + has_alpha;
	restx = step - 1;

	for (size_t i = 0; i < ssize - restx; i += step) {
	  int r, g, b, a = 0;

	  if (index < imgsize) {
	    r = opj_image->comps[0].data[index];
	    g = opj_image->comps[1].data[index];
	    b = opj_image->comps[2].data[index];
	    if (has_alpha)
	      a = opj_image->comps[3].data[index];

	    if (sgnd) {
	      r += sgndadjust;
	      g += sgndadjust;
	      b += sgndadjust;
	      if (has_alpha)
		a += sgndadjust;
	    }
	    if (force16) {
	      r = (r << ushift) + (r >> dshift);
	      g = (g << ushift) + (g >> dshift);
	      b = (b << ushift) + (b >> dshift);
	      if (has_alpha)
		a = (a << ushift) + (a >> dshift);
	    }
	    if (r > 65535)
	      r = 65535;
	    else if (r < 0)
	      r = 0;
	    if (g > 65535)
	      g = 65535;
	    else if (g < 0)
	      g = 0;
	    if (b > 65535)
	      b = 65535;
	    else if (b < 0)
	      b = 0;

	    dat8[i + 0] = (unsigned char)r; // LSB
	    dat8[i + 1] = (unsigned char)(r >> 8); // MSB
	    dat8[i + 2] = (unsigned char)g;
	    dat8[i + 3] = (unsigned char)(g >> 8);
	    dat8[i + 4] = (unsigned char)b;
	    dat8[i + 5] = (unsigned char)(b >> 8);
	    if (has_alpha) {
	      if (a > 65535)
		a = 65535;
	      else if (a < 0)
		a = 0;
	      dat8[i + 6] = (unsigned char)a;
	      dat8[i + 7] = (unsigned char)(a >> 8);
	    }
	    index++;
	    last_i = i + step;
	  } else {
	    break;
          }
	} // for(i = 0;)

	if (last_i < ssize) {
	  for (size_t i = last_i; i < ssize; i += step) {
	    int r, g, b, a = 0;

	    if (index < imgsize) {
	      r = opj_image->comps[0].data[index];
	      g = opj_image->comps[1].data[index];
	      b = opj_image->comps[2].data[index];
	      if (has_alpha)
		a = opj_image->comps[3].data[index];

	      if (sgnd) {
		r += sgndadjust;
		g += sgndadjust;
		b += sgndadjust;
		if (has_alpha)
		  a += sgndadjust;
	      }
	      if (force16) {
		r = (r << ushift) + (r >> dshift);
		g = (g << ushift) + (g >> dshift);
		b = (b << ushift) + (b >> dshift);
		if (has_alpha)
		  a = (a << ushift) + (a >> dshift);
	      }
	      if (r > 65535)
		r = 65535;
	      else if (r < 0)
		r = 0;
	      if (g > 65535)
		g = 65535;
	      else if (g < 0)
		g = 0;
	      if (b > 65535)
		b = 65535;
	      else if (b < 0)
		b = 0;

	      dat8[i + 0] = (unsigned char)r; // LSB
	      if (i + 1 < ssize)
		dat8[i + 1] = (unsigned char)(r >> 8);
	      else
		break; // MSB
	      if (i + 2 < ssize)
		dat8[i + 2] = (unsigned char)g;
	      else
		break;
	      if (i + 3 < ssize)
		dat8[i + 3] = (unsigned char)(g >> 8);
	      else
		break;
	      if (i + 4 < ssize)
		dat8[i + 4] = (unsigned char)b;
	      else
		break;
	      if (i + 5 < ssize)
		dat8[i + 5] = (unsigned char)(b >> 8);
	      else
		break;

	      if (has_alpha) {
		if (a > 65535)
		  a = 65535;
		else if (a < 0)
		  a = 0;
		if (i + 6 < ssize)
		  dat8[i + 6] = (unsigned char)a;
		else
		  break;
		if (i + 7 < ssize)
		  dat8[i + 7] = (unsigned char)(a >> 8);
		else
		  break;
	      }
	      index++;
	    } else
	      break;
	  } // for(i)
	} // if(last_i < ssize)
      } // if(bps == 16)
      (void)TIFFWriteEncodedStrip(tif, strip, (void*)buf, strip_size);
    } // for(strip = 0; )

    return 0;
  } // RGB(A)
*/

  if (opj_image->numcomps == 1 // GRAY 
      || (opj_image->numcomps == 2 // GRAY_ALPHA 
          && opj_image->comps[0].dx == opj_image->comps[1].dx
          && opj_image->comps[0].dy == opj_image->comps[1].dy
          && opj_image->comps[0].prec == opj_image->comps[1].prec)) {

    has_alpha = (opj_image->numcomps == 2);


#ifdef DEBUG
    if (has_alpha)
      std::cerr << "Detected GRAY ALPHA image." << std::endl;
    else
      std::cerr << "Detected GRAY image." << std::endl;
#endif


    if (bim::allocImg( bim_image, width, height, opj_image->numcomps, bps ) != 0) {
#ifdef DEBUG
      std::cerr << "Failed to reserve memory for bim::image of size "
              << width << "x" << height << "x" << opj_image->numcomps
              << " in " << bps << "bit precision." << std::endl;
#endif
      return 1;
    }
    bim::uchar* bim_image_ptr = (bim::uchar*)bim_image->bits[0];


    index = 0;
    if (bps == 8) {
      if (has_alpha) {
        const int step = 2;
        for (size_t i = 0; i < imgsize; i += step) {
          int r = opj_image->comps[0].data[i];
          int a = opj_image->comps[1].data[i];

          if (sgnd) {
            r += sgndadjust;
            a += sgndadjust;

            if (r > 255) r = 255;
            if (r < 0)   r = 0;
            if (a > 255) a = 255;
            if (a < 0)   a = 0;
          }

          bim_image_ptr[index++] = (unsigned char)r;
          bim_image_ptr[index++] = (unsigned char)a;
        }
      } else {
        for (size_t i = 0; i < imgsize; ++i) {
          int r = opj_image->comps[0].data[i];

          if (sgnd) {
            r += sgndadjust;

            if (r > 255) r = 255;
            if (r < 0)   r = 0;
          }

          bim_image_ptr[index++] = (unsigned char)r;
        }
      }
    } else if (bps == 16) {
      if (has_alpha) {
        for (size_t i = 0; i < imgsize; ++i) {
          int r = opj_image->comps[0].data[i];
          int a = opj_image->comps[1].data[i];

          if (sgnd) {
            r += sgndadjust;
            a += sgndadjust;
          }
          if (force16) {
            r = (r << ushift) + (r >> dshift);
            a = (a << ushift) + (a >> dshift);
          }

          if (r > 65535) r = 65535;
          if (r < 0)     r = 0;
          if (a > 65535) a = 65535;
          if (a < 0)     a = 0;

          bim_image_ptr[index++] = (unsigned char)r; // LSB
          bim_image_ptr[index++] = (unsigned char)(r >> 8); // MSB
          bim_image_ptr[index++] = (unsigned char)a;
          bim_image_ptr[index++] = (unsigned char)(a >> 8);
	}
      } else {
        for (size_t i = 0; i < imgsize; ++i) {
          int r = opj_image->comps[0].data[i];

          if (sgnd) {
            r += sgndadjust;
          }
          if (force16) {
            r = (r << ushift) + (r >> dshift);
          }
          if (r > 65535) r = 65535;
          if (r < 0)     r = 0;

          bim_image_ptr[index++] = (unsigned char)r; // LSB
          bim_image_ptr[index++] = (unsigned char)(r >> 8); // MSB
        }
      }
    }

    return 0;
  }

  std::cerr << "openjpeg_image_to_bim_image(): Bad color format. " << std::endl
          << "Only RGB(A) and GRAY(A) has been implemented. " << std::endl
          << "Found number of components to be " << opj_image->numcomps
          << ". Aborting" << std::endl;

  return 1;
}


static int infile_format_magic(const char* fname) {
  FILE* reader;
  unsigned char buf[12];
  OPJ_SIZE_T l_nb_read;

  reader = fopen(fname, "rb");
  if (reader == NULL)
    return -2;

  memset(buf, 0, 12);
  l_nb_read = fread(buf, 1, 12, reader);
  fclose(reader);
  if (l_nb_read != 12)
    return -1;


  if (memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(buf, JP2_MAGIC, 4) == 0) {
    return JP2_CFMT;
  } else if (memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0) {
    return J2K_CFMT;
  }

  return -1;
}

static int infile_format(const char* fname) {
  static const char* extension[] = { "j2k", "jp2", "jpt", "j2c", "jpc" };
  static const int format[] = { J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT, J2K_CFMT };
  const char* ext = strrchr(fname, '.');
  if (ext == NULL) {
    return -1;
  }

  ++ext;
  if (*ext) {
    for (size_t i = 0; i < sizeof(format)/sizeof(*format); i++) {
      if (strcasecmp(ext, extension[i]) == 0) {
        return format[i];
      }
    }
  }

  return -1;
}

// sample error callback expecting a FILE* client object
static void jp2_error_callback(const char* msg, void* client_data) {
  (void)client_data;
  std::cerr << msg;
}

// sample warning callback expecting a FILE* client object
static void jp2_warning_callback(const char* msg, void* client_data) {
  (void)client_data;
#ifdef DEBUG
  std::cerr << msg;
#else
  (void)msg;
#endif
}

// sample debug callback expecting no client object
static void jp2_info_callback(const char* msg, void* client_data) {
  (void)client_data;
#ifdef DEBUG
  std::cout << msg;
#else
  (void)msg;
#endif
}




int read_jp2_image( bim::FormatHandle *fmtHndl ) {
#ifdef DEBUG
  std::cerr << "read_jp2_image() called" << std::endl;
#endif

  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;

  // openjpeg variables:
  opj_dparameters_t parameters;           // decompression parameters 
  opj_image_t*      opj_image = NULL;
  opj_stream_t*     l_stream  = NULL;      // Stream 
  opj_codec_t*      l_codec   = NULL;      // Handle to a decompressor

  // TODO FIXME: There would be an option to read an index from the file.
  // I do not know what that is good for. Currently not implemented.
  //opj_codestream_index_t* cstr_index = NULL;



  //*********************************************************************
  // SETTING DECODING PARAMETERS
  //*********************************************************************
  // initialize decoding parameters to default values 
  opj_set_default_decoder_parameters(&parameters);

  // Set input file name. Currently accepts J2K-files, JP2-files
  // and JPT-files. The file type is identified based on magic number.
  // Use infile_format_ext(fmtHndl->fileName) to check file extension.
  parameters.decod_format = infile_format_magic(fmtHndl->fileName);
  strncpy(parameters.infile, fmtHndl->fileName, sizeof(parameters.infile) - 1);
  if (parameters.decod_format < 0) {
    std::cerr << "read_jp2_image(): Unknown input file format for file "
            << parameters.infile << "." << std::endl << "Known file formats are"
            << " *.j2k, *.jp2, *.jpc or *.jpt." << std::endl;
    return 1;
  }

  // Set the reduce factor: Set the number of highest resolution levels
  // to be discarded. The image resolution is effectively divided by 2 to
  // the power of the number of discarded levels. The reduce factor is
  // limited by the smallest total number of decomposition levels among
  // tiles.
  //parameters.cp_reduce = 3;

  // Set the maximum number of quality layers to decode. If there are
  // less quality layers than the specified number, all the quality layers
  // are decoded.
  //parameters.cp_layer = 4;

  // Set a decoding area. By default all the image is decoded.
  //parameters.DA_x0 = 0;
  //parameters.DA_y0 = 0;
  //parameters.DA_x1 = 1024;
  //parameters.DA_y1 = 1024;

  // Set the tile_number of the decoded tile. Follow the JPEG2000 convention
  // from left-up to bottom-up. By default all tiles are decoded.
  //parameters.tile_index = 2;
  // Combine the above option (tile number) with the following option, to set
  // how many tiles should be decoded:
  //parameters.nb_tile_to_decode = 1;

#ifdef USE_JPWL
  std::cerr << "WARNING: read_jp2_image(): you are using untested JPWL code."
          << " Use at your own discretion." << std::endl;
  // TODO FIXME: Currently JPWL is not active and untested. The code should
  // work but please check carefully before activating/using.
  //
  //  Activate the JPWL correction capability, if the codestream complies.
  //  Options can be a comma separated list of <param=val> tokens:
  //  c, c=numcomps
  //     numcomps is the number of expected components in the codestream
  //     (search of first EPB rely upon this, default is %d)" << std::endl, JPWL_EXPECTED_COMPONENTS);
  char* token = NULL;

  // search expected number of components 
  if (*token == 'c') {
    static int compno;
    compno = JPWL_EXPECTED_COMPONENTS; // predefined no. of components 

    if (sscanf(token, "c=%d", &compno) == 1) {
      // Specified 
      if ((compno < 1) || (compno > 256)) {
        std::cerr << "Invalid number of components c = " << compno << std::endl;
        return 1;
      }
      parameters.jpwl_exp_comps = compno;
    } else if (!strcmp(token, "c")) {
      // default 
      parameters.jpwl_exp_comps = compno; // auto for default size 
    } else {
      std::cerr << "Invalid components specified = " << token << std::endl;
      return 1;
    }
  }

  // search maximum number of tiles 
  if (*token == 't') {
    static int tileno;
    tileno = JPWL_MAXIMUM_TILES; // maximum no. of tiles 

    if (sscanf(token, "t=%d", &tileno) == 1) {
      // Specified 
      if ((tileno < 1) || (tileno > JPWL_MAXIMUM_TILES)) {
        std::cerr << "invalid number of tiles t = " << tileno << std::endl;
        return 1;
      }
      parameters.jpwl_max_tiles = tileno;
    } else if (!strcmp(token, "t")) {
      // default 
      parameters.jpwl_max_tiles = tileno; // auto for default size 
    } else {
      std::cerr << "invalid tiles specified = " << token << std::endl;
      return 1;
    };
  }

  parameters.jpwl_correct = OPJ_TRUE;
  std::cout << "JPWL correction capability activated. Expecting "
          << parameters.jpwl_exp_comps << " components." << std::endl;
#endif // USE_JPWL 



  //*********************************************************************
  // READ THE JPEG2000 IMAGE INTO OPENJPEG IMAGE FORMAT
  //*********************************************************************
  // TODO FIXME: This effectively opens the file again. Better would be
  // to use the file handle from fmtHndl->stream.
  // read the input file and put it in memory
  l_stream = opj_stream_create_default_file_stream(parameters.infile, 1);
  if (!l_stream) {
    std::cerr << "Failed to create a file stream for file "
            << parameters.infile << "." << std::endl;
    return 1;
  }


  // Create a decoder for the JPEG2000 stream
  switch (parameters.decod_format) {
  case J2K_CFMT:
    {
      // Get a decoder handle for JPEG-2000 codestream
#ifdef DEBUG
      std::cerr << "Setting file decompressor to OPJ_CODEC_J2K." << std::endl;
#endif
      l_codec = opj_create_decompress(OPJ_CODEC_J2K);
      break;
    }
  case JP2_CFMT:
    {
      // Get a decoder handle for JPEG 2000 compressed image data
#ifdef DEBUG
      std::cerr << "Setting file decompressor to OPJ_CODEC_JP2." << std::endl;
#endif
      l_codec = opj_create_decompress(OPJ_CODEC_JP2);
      break;
    }
  case JPT_CFMT:
    {
      // Get a decoder handle for JPEG 2000, JPIP
#ifdef DEBUG
      std::cerr << "Setting file decompressor to OPJ_CODEC_JPT." << std::endl;
#endif
      l_codec = opj_create_decompress(OPJ_CODEC_JPT);
      break;
    }
  default:
    {
      std::cerr << "Failed to parse the decode-format index " << parameters.decod_format << "." << std::endl
              << "This indicates that infile_format_magic() could not parse the file magic number." << std::endl;
      opj_stream_destroy(l_stream);
      return 1;
    }
  }

  // Catch events using our own callbacks, and give a local context 
  opj_set_info_handler(l_codec, jp2_info_callback, 00);
  opj_set_warning_handler(l_codec, jp2_warning_callback, 00);
  opj_set_error_handler(l_codec, jp2_error_callback, 00);

  // Setup the decoding parameters using the parameters set above:
  if (!opj_setup_decoder(l_codec, &parameters)) {
    std::cerr << "Failed to setup the decoder" << std::endl;
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    return 1;
  }

  // Read the main header of the codestream, and if necessary the JP2 boxes
  if (!opj_read_header(l_stream, l_codec, &opj_image)) {
    std::cerr << "Failed to read the file header" << std::endl;
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    opj_image_destroy(opj_image);
    return 1;
  }

  // NOTE: the following implementation of reading comments from the JP2
  // header is a patch implemented by Mario Emmenlauer. It is not part of
  // the official openjpeg library.
  if (opj_image->cp_comment) {
#ifdef DEBUG
    std::cerr << "Found comment(s) decoded from the image header." << std::endl;
#endif

    size_t endpos = 0;
    size_t startpos = 0;
    const char* comment;
    while (1) {
      ++endpos;
      if (opj_image->cp_comment[endpos] == 0 && opj_image->cp_comment[endpos+1] != 0) {
        // A single zero character indicates the end of one comment. So here
        // is a break between two concatenated comments:
        comment = (opj_image->cp_comment + startpos);
        par->comments.push_back(comment);
        startpos = endpos+1;
      } else if (opj_image->cp_comment[endpos] == 0 && opj_image->cp_comment[endpos+1] == 0 && opj_image->cp_comment[endpos+2] == 0) {
        // Three consecutive zeros indicate the end of the comments list.
        // So here is the end of all string comments:
        comment = (opj_image->cp_comment + startpos);
        par->comments.push_back(comment);
        break;
      }
    }
  }




  if (!parameters.nb_tile_to_decode) {
#ifdef DEBUG
    std::cerr << "Setting the area that should be decoded to "
            << (OPJ_INT32)parameters.DA_x0 << ", "
            << (OPJ_INT32)parameters.DA_y0 << ", "
            << (OPJ_INT32)parameters.DA_x1 << ", "
            << (OPJ_INT32)parameters.DA_y1 << std::endl;
#endif

    // Optional if you want decode the entire image 
    if (!opj_set_decode_area(l_codec, opj_image, (OPJ_INT32)parameters.DA_x0, (OPJ_INT32)parameters.DA_y0, (OPJ_INT32)parameters.DA_x1, (OPJ_INT32)parameters.DA_y1)) {
      std::cerr << "Failed to set the area to be decoded." << std::endl;
      opj_stream_destroy(l_stream);
      opj_destroy_codec(l_codec);
      opj_image_destroy(opj_image);
      return 1;
    }

    // Get the decoded image 
    if (!(opj_decode(l_codec, l_stream, opj_image) && opj_end_decompress(l_codec, l_stream))) {
      std::cerr << "Failed to decode the image." << std::endl;
      opj_destroy_codec(l_codec);
      opj_stream_destroy(l_stream);
      opj_image_destroy(opj_image);
      return 1;
    }
  } else {
    // This is just here to illustrate how to use the resolution after set parameters
    /*
#ifdef DEBUG
    std::cerr << "Setting resolution factor." << std::endl;
#endif
    if (!opj_set_decoded_resolution_factor(l_codec, 5)) {
      std::cerr << "failed to set the resolution factor tile!" << std::endl;
      opj_destroy_codec(l_codec);
      opj_stream_destroy(l_stream);
      opj_image_destroy(opj_image);
      return 1;
    }
    */

#ifdef DEBUG
    std::cerr << "Will decode only the single tile number "
            << parameters.tile_index << "." << std::endl;
#endif
    if (!opj_get_decoded_tile(l_codec, l_stream, opj_image, parameters.tile_index)) {
      std::cerr << "Failed to decode the tile." << std::endl;
      opj_destroy_codec(l_codec);
      opj_stream_destroy(l_stream);
      opj_image_destroy(opj_image);
      return 1;
    }
#ifdef DEBUG
    std::cout << "Single tile number " << parameters.tile_index << " has been decoded." << std::endl;
#endif
  }

  // Close the byte stream 
  opj_stream_destroy(l_stream);

  // free remaining structures 
  if (l_codec) {
    opj_destroy_codec(l_codec);
  }




  //*********************************************************************
  // OPENJPEG COLOR SPACE CONVERSION
  //*********************************************************************
  if (opj_image->color_space == OPJ_CLRSPC_SYCC) {
#ifdef DEBUG
    std::cerr << "Will convert from color space SYCC to RGB." << std::endl;
#endif
    color_sycc_to_rgb(opj_image); // FIXME 
  }

  if (opj_image->color_space != OPJ_CLRSPC_SYCC && opj_image->numcomps == 3 && opj_image->comps[0].dx == opj_image->comps[0].dy && opj_image->comps[1].dx != 1) {
    opj_image->color_space = OPJ_CLRSPC_SYCC;
  } else if (opj_image->numcomps <= 2) {
    opj_image->color_space = OPJ_CLRSPC_GRAY;
  }

  if (opj_image->icc_profile_buf) {
#ifdef DEBUG
    std::cerr << "Detected the image has an ICC profile embedded." << std::endl;
#endif
#if defined(OPJ_HAVE_LIBLCMS1) || defined(OPJ_HAVE_LIBLCMS2)
    color_apply_icc_profile(opj_image); // FIXME 
#endif
    free(opj_image->icc_profile_buf);
    opj_image->icc_profile_buf = NULL;
    opj_image->icc_profile_len = 0;
  }




  //*********************************************************************
  // CONVERT THE OPENJPEG IMAGE FORMAT INTO BIM::IMAGE
  //*********************************************************************
  // create output image
  if (openjpeg_image_to_bim_image(opj_image, fmtHndl->image) != 0) {
    std::cerr << "Failed to convert the openjpeg image to bim::image." << std::endl;
    opj_image_destroy(opj_image);
    return 1;
  }



  // free image data structure 
  opj_image_destroy(opj_image);

  // destroy the codestream index 
  //opj_destroy_cstr_index(&cstr_index);

  return 0;
}
