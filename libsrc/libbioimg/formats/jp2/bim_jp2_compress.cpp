/*****************************************************************************
  JPEG2000 support
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION

  Programmer: Mario Emmenlauer <mario@emmenlauer.de>

  History:
  05/19/2015 14:20 - First creation

  Ver : 1

  ******************************************************************************
  This code is inspired by the code in file opj_compress.c of the
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
  Copyright (c) 2008, Jerome Fimes, Communications & Systemes <jerome.fimes@c-s.fr>
  Copyright (c) 2011-2012, Centre National d'Etudes Spatiales (CNES), France
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

#include "bim_jp2_format.h"
#include "bim_jp2_format_io.h"
#include "bim_jp2_compress.h"

#include <openjpeg.h>

#include <format_defs.h>


template<class T>
void copy_memory_generic_bim_image_to_openjpeg_image(const bim::ImageBitmap* const bim_image, opj_image_t* opj_image, const size_t numcomps, const size_t imgsize)
{
}


int bim_image_to_openjpeg_image(const bim::ImageBitmap* const bim_image, opj_image_t* opj_image)
{
    /*

        int subsampling_dx = parameters->subsampling_dx;
        int subsampling_dy = parameters->subsampling_dy;
        TIFF *tif;
        tdata_t buf;
        tstrip_t strip;
        tsize_t strip_size;
        int j, numcomps, w, h,index;
        OPJ_COLOR_SPACE color_space;
        opj_image_cmptparm_t cmptparm[4]; // RGBA
        opj_image_t *image = NULL;
        int imgsize = 0;
        int has_alpha = 0;
        unsigned short tiBps, tiPhoto, tiSf, tiSpp, tiPC;
        unsigned int tiWidth, tiHeight;
        OPJ_BOOL is_cinema = OPJ_IS_CINEMA(parameters->rsiz);

        tif = TIFFOpen(filename, "r");

        if(!tif)
        {
        fprintf(stderr, "tiftoimage:Failed to open %s for reading\n", filename);
        return 0;
        }
        tiBps = tiPhoto = tiSf = tiSpp = tiPC = 0;
        tiWidth = tiHeight = 0;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tiWidth);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tiHeight);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &tiBps);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &tiSf);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tiSpp);
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &tiPhoto);
        TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &tiPC);
        w= (int)tiWidth;
        h= (int)tiHeight;

        if(tiBps != 8 && tiBps != 16 && tiBps != 12) tiBps = 0;
        if(tiPhoto != 1 && tiPhoto != 2) tiPhoto = 0;

        if( !tiBps || !tiPhoto)
        {
        if( !tiBps)
        fprintf(stderr,"tiftoimage: Bits=%d, Only 8 and 16 bits"
        " implemented\n",tiBps);
        else
        if( !tiPhoto)
        fprintf(stderr,"tiftoimage: Bad color format %d.\n\tOnly RGB(A)"
        " and GRAY(A) has been implemented\n",(int) tiPhoto);

        fprintf(stderr,"\tAborting\n");
        TIFFClose(tif);

        return NULL;
        }

        {// From: tiff-4.0.x/libtiff/tif_getimage.c :
        uint16* sampleinfo;
        uint16 extrasamples;

        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
        &extrasamples, &sampleinfo);

        if(extrasamples >= 1)
        {
        switch(sampleinfo[0])
        {
        case EXTRASAMPLE_UNSPECIFIED:
        // Workaround for some images without correct info about alpha channel
        if(tiSpp > 3)
        has_alpha = 1;
        break;

        case EXTRASAMPLE_ASSOCALPHA: // data pre-multiplied
        case EXTRASAMPLE_UNASSALPHA: // data not pre-multiplied
        has_alpha = 1;
        break;
        }
        }
        else // extrasamples == 0
        if(tiSpp == 4 || tiSpp == 2) has_alpha = 1;
        }

        // initialize image components
        memset(&cmptparm[0], 0, 4 * sizeof(opj_image_cmptparm_t));

        if ((tiPhoto == PHOTOMETRIC_RGB) && (is_cinema)) {
        fprintf(stdout,"WARNING:\n"
        "Input image bitdepth is %d bits\n"
        "TIF conversion has automatically rescaled to 12-bits\n"
        "to comply with cinema profiles.\n",
        tiBps);
        }

        if(tiPhoto == PHOTOMETRIC_RGB) // RGB(A)
        {
        numcomps = 3 + has_alpha;
        color_space = OPJ_CLRSPC_SRGB;

        //#define USETILEMODE
        for(j = 0; j < numcomps; j++)
        {
        if(is_cinema)
        {
        cmptparm[j].prec = 12;
        cmptparm[j].bpp = 12;
        }
        else
        {
        cmptparm[j].prec = tiBps;
        cmptparm[j].bpp = tiBps;
        }
        cmptparm[j].dx = (OPJ_UINT32)subsampling_dx;
        cmptparm[j].dy = (OPJ_UINT32)subsampling_dy;
        cmptparm[j].w = (OPJ_UINT32)w;
        cmptparm[j].h = (OPJ_UINT32)h;
        #ifdef USETILEMODE
        cmptparm[j].x0 = 0;
        cmptparm[j].y0 = 0;
        #endif
        }

        #ifdef USETILEMODE
        image = opj_image_tile_create(numcomps,&cmptparm[0],color_space);
        #else
        image = opj_image_create((OPJ_UINT32)numcomps, &cmptparm[0], color_space);
        #endif

        if(!image)
        {
        TIFFClose(tif);
        return NULL;
        }
        // set image offset and reference grid
        image->x0 = (OPJ_UINT32)parameters->image_offset_x0;
        image->y0 = (OPJ_UINT32)parameters->image_offset_y0;
        image->x1 =	!image->x0 ? (OPJ_UINT32)(w - 1) * (OPJ_UINT32)subsampling_dx + 1 :
        image->x0 + (OPJ_UINT32)(w - 1) * (OPJ_UINT32)subsampling_dx + 1;
        image->y1 =	!image->y0 ? (OPJ_UINT32)(h - 1) * (OPJ_UINT32)subsampling_dy + 1 :
        image->y0 + (OPJ_UINT32)(h - 1) * (OPJ_UINT32)subsampling_dy + 1;

        buf = _TIFFmalloc(TIFFStripSize(tif));

        strip_size=TIFFStripSize(tif);
        index = 0;
        imgsize = (int)(image->comps[0].w * image->comps[0].h);
        // Read the Image components
        for(strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
        {
        unsigned char *dat8;
        int step;
        tsize_t i, ssize;
        ssize = TIFFReadEncodedStrip(tif, strip, buf, strip_size);
        dat8 = (unsigned char*)buf;

        if(tiBps == 16)
        {
        step = 6 + has_alpha + has_alpha;

        for(i = 0; i < ssize; i += step)
        {
        if(index < imgsize)
        {
        image->comps[0].data[index] = ( dat8[i+1] << 8 ) | dat8[i+0]; // R
        image->comps[1].data[index] = ( dat8[i+3] << 8 ) | dat8[i+2]; // G
        image->comps[2].data[index] = ( dat8[i+5] << 8 ) | dat8[i+4]; // B
        if(has_alpha)
        image->comps[3].data[index] = ( dat8[i+7] << 8 ) | dat8[i+6];

        if(is_cinema)
        {
        // Rounding 16 to 12 bits
        image->comps[0].data[index] =
        (image->comps[0].data[index] + 0x08) >> 4 ;
        image->comps[1].data[index] =
        (image->comps[1].data[index] + 0x08) >> 4 ;
        image->comps[2].data[index] =
        (image->comps[2].data[index] + 0x08) >> 4 ;
        if(has_alpha)
        image->comps[3].data[index] =
        (image->comps[3].data[index] + 0x08) >> 4 ;
        }
        index++;
        }
        else
        break;
        }//for(i = 0)
        }//if(tiBps == 16)
        else
        if(tiBps == 8)
        {
        step = 3 + has_alpha;

        for(i = 0; i < ssize; i += step)
        {
        if(index < imgsize)
        {
        #ifndef USETILEMODE
        image->comps[0].data[index] = dat8[i+0];// R
        image->comps[1].data[index] = dat8[i+1];// G
        image->comps[2].data[index] = dat8[i+2];// B
        if(has_alpha)
        image->comps[3].data[index] = dat8[i+3];
        #endif

        if(is_cinema)
        {
        // Rounding 8 to 12 bits
        #ifndef USETILEMODE
        image->comps[0].data[index] = image->comps[0].data[index] << 4 ;
        image->comps[1].data[index] = image->comps[1].data[index] << 4 ;
        image->comps[2].data[index] = image->comps[2].data[index] << 4 ;
        if(has_alpha)
        image->comps[3].data[index] = image->comps[3].data[index] << 4 ;
        #endif
        }
        index++;
        }//if(index
        else
        break;
        }//for(i )
        }//if( tiBps == 8)
        else
        if(tiBps == 12)// CINEMA file
        {
        step = 9;

        for(i = 0; i < ssize; i += step)
        {
        if((index < imgsize)&(index+1 < imgsize))
        {
        image->comps[0].data[index]   = ( dat8[i+0]<<4 )        |(dat8[i+1]>>4);
        image->comps[1].data[index]   = ((dat8[i+1]& 0x0f)<< 8) | dat8[i+2];

        image->comps[2].data[index]   = ( dat8[i+3]<<4)         |(dat8[i+4]>>4);
        image->comps[0].data[index+1] = ((dat8[i+4]& 0x0f)<< 8) | dat8[i+5];

        image->comps[1].data[index+1] = ( dat8[i+6] <<4)        |(dat8[i+7]>>4);
        image->comps[2].data[index+1] = ((dat8[i+7]& 0x0f)<< 8) | dat8[i+8];

        index += 2;
        }
        else
        break;
        }//for(i )
        }
        }//for(strip = 0; )

        _TIFFfree(buf);
        TIFFClose(tif);

        return image;
        }//RGB(A)

        if(tiPhoto == PHOTOMETRIC_MINISBLACK) // GRAY(A)
        {
        numcomps = 1 + has_alpha;
        color_space = OPJ_CLRSPC_GRAY;

        for(j = 0; j < numcomps; ++j)
        {
        cmptparm[j].prec = tiBps;
        cmptparm[j].bpp = tiBps;
        cmptparm[j].dx = (OPJ_UINT32)subsampling_dx;
        cmptparm[j].dy = (OPJ_UINT32)subsampling_dy;
        cmptparm[j].w = (OPJ_UINT32)w;
        cmptparm[j].h = (OPJ_UINT32)h;
        }
        #ifdef USETILEMODE
        image = opj_image_tile_create(numcomps,&cmptparm[0],color_space);
        #else
        image = opj_image_create((OPJ_UINT32)numcomps, &cmptparm[0], color_space);
        #endif

        if(!image)
        {
        TIFFClose(tif);
        return NULL;
        }
        // set image offset and reference grid
        image->x0 = (OPJ_UINT32)parameters->image_offset_x0;
        image->y0 = (OPJ_UINT32)parameters->image_offset_y0;
        image->x1 =	!image->x0 ? (OPJ_UINT32)(w - 1) * (OPJ_UINT32)subsampling_dx + 1 :
        image->x0 + (OPJ_UINT32)(w - 1) * (OPJ_UINT32)subsampling_dx + 1;
        image->y1 =	!image->y0 ? (OPJ_UINT32)(h - 1) * (OPJ_UINT32)subsampling_dy + 1 :
        image->y0 + (OPJ_UINT32)(h - 1) * (OPJ_UINT32)subsampling_dy + 1;

        buf = _TIFFmalloc(TIFFStripSize(tif));

        strip_size = TIFFStripSize(tif);
        index = 0;
        imgsize = (int)(image->comps[0].w * image->comps[0].h);
        // Read the Image components
        for(strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
        {
        unsigned char *dat8;
        tsize_t i, ssize;
        int step;

        ssize = TIFFReadEncodedStrip(tif, strip, buf, strip_size);
        dat8 = (unsigned char*)buf;

        if(tiBps == 16)
        {
        step = 2 + has_alpha + has_alpha;

        for(i = 0; i < ssize; i += step)
        {
        if(index < imgsize)
        {
        image->comps[0].data[index] = ( dat8[i+1] << 8 ) | dat8[i+0];
        if(has_alpha)
        image->comps[1].data[index] = ( dat8[i+3] << 8 ) | dat8[i+2];
        index++;
        }
        else
        break;
        }//for(i )
        }
        else
        if(tiBps == 8)
        {
        step = 1 + has_alpha;

        for(i = 0; i < ssize; i += step)
        {
        if(index < imgsize)
        {
        image->comps[0].data[index] = dat8[i+0];
        if(has_alpha)
        image->comps[1].data[index] = dat8[i+1];
        index++;
        }
        else
        break;
        }//for(i )
        }
        }//for(strip = 0;

        _TIFFfree(buf);
        TIFFClose(tif);

        }//GRAY(A)

        */
    return 0;
}

static int outfile_format_magic(char* filename) {
    static const char* extension[] = {
        "pgx", "pnm", "pgm", "ppm", "pbm", "pam", "bmp", "tif", "raw", "rawl", "tga", "png", "j2k", "jp2", "j2c", "jpc"
    };
    static const int format[] = {
        PGX_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, BMP_DFMT, TIF_DFMT, RAW_DFMT, RAWL_DFMT, TGA_DFMT, PNG_DFMT, J2K_CFMT, JP2_CFMT, J2K_CFMT, J2K_CFMT
    };

    std::vector<bim::xstring> exts = bim::xstring(filename).split(".");
    if (exts.size() < 2) return -1;
    bim::xstring ext = exts[exts.size() - 1].toLowerCase();

    for (size_t i = 0; i < sizeof(format) / sizeof(*format); i++) {
        if (ext == extension[i]) {
            return format[i];
        }
    }

    return -1;
}

// sample error callback
static void jp2_error_callback(const char* msg, void* client_data) {
    (void)client_data;
    std::cerr << msg;
}

// sample warning callback
static void jp2_warning_callback(const char* msg, void* client_data) {
    (void)client_data;
#ifdef DEBUG
    std::cerr << msg;
#else
    (void)msg;
#endif
}

// sample debug callback
static void jp2_info_callback(const char* msg, void* client_data) {
    (void)client_data;
#ifdef DEBUG
    std::cout << msg;
#else
    (void)msg;
#endif
}


int write_jp2_image(bim::FormatHandle *fmtHndl)
{
#ifdef DEBUG
    std::cerr << "write_jp2_image() called" << std::endl;
#endif

    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams == NULL) return 1;
    bim::Jp2Params *par = (bim::Jp2Params *) fmtHndl->internalParams;

    // openjpeg variables:
    opj_cparameters_t parameters;
    opj_image_t*      opj_image = NULL;
    opj_stream_t*     l_stream = NULL;
    opj_codec_t*      l_codec = NULL;

    // TODO FIXME: There would be an option to write an index to a file.
    // I do not know what that is good for. Currently not implemented.
    //char indexfilename[OPJ_PATH_LEN];
    //indexfilename[0] = 0;

    OPJ_BOOL bSuccess;


    //*********************************************************************
    // CONVERT THE BIM::IMAGE INTO OPENJPEG IMAGE FORMAT
    //*********************************************************************
    //image = pngtoimage(parameters.infile, &parameters);
    if (bim_image_to_openjpeg_image(fmtHndl->image, opj_image) != 0) {
        fprintf(stderr, "Error: could not convert bim::ImageBitmap to openjpeg image.\n");
        return 1;
    }



    //*********************************************************************
    // SETTING ENCODING PARAMETERS
    //*********************************************************************
    // initialize encoding parameters to default values 
    opj_set_default_encoder_parameters(&parameters);

    // Set output file name. Currently accepts J2K-files, JP2-files
    // and J2C-files. The file type is identified based on magic number.

    parameters.cod_format = outfile_format_magic(fmtHndl->fileName);
    strncpy(parameters.outfile, fmtHndl->fileName, sizeof(parameters.outfile) - 1);
    if (parameters.cod_format < 0) {
        std::cerr << "write_jp2_image(): Unknown output format for file "
            << parameters.outfile << "." << std::endl << "Known file formats are"
            << " *.j2k, *.j2c or *.jp2." << std::endl;
        return 1;
    }


    // Different compression ratios for successive layers. The rate specified
    // for each quality level is the desired compression factor. Can not be
    // combined with setting psnr. Decreasing ratios required.
    // Example: 20,10,1 means
    //   quality layer 1: compress 20x,
    //   quality layer 2: compress 10x
    //   quality layer 3: compress lossless
    //
    // set number of layers to a single layer:
    //parameters.tcp_numlayers = 3;
    parameters.tcp_numlayers = 1;

    // set allocation by rate/distortion:
    parameters.cp_disto_alloc = 1;

    // compression rates of the layers:
    //parameters.tcp_rates[0] = 20;
    //parameters.tcp_rates[1] = 10;
    //parameters.tcp_rates[2] = 1;
    // set lossless compression:
    parameters.tcp_rates[0] = 0;



    // Different psnr for successive layers (30,40,50). Can not be combined
    // with setting compression ratios. Increasing PSNR values required.
    /*
    parameters.tcp_numlayers = 3;
    parameters.cp_fixed_quality = 1;
    parameters.tcp_distoratio[0] = 30;
    parameters.tcp_distoratio[1] = 40;
    parameters.tcp_distoratio[2] = 50;
    */


    // mod fixed_quality (before : -q)
    /* TODO FIXME: is this correct?
    int* row = NULL, *col = NULL;
    OPJ_UINT32 numlayers = 0, numresolution = 0, matrix_width = 0;

    char* s = opj_optarg;
    sscanf(s, "%ud", &numlayers);
    s++;
    if (numlayers > 9)
    s++;

    parameters.tcp_numlayers = (int)numlayers;
    numresolution = (OPJ_UINT32)parameters.numresolution;
    matrix_width = numresolution * 3;
    parameters.cp_matrice = (int*)malloc(numlayers * matrix_width * sizeof(int));
    s = s + 2;

    for (int i = 0; i < numlayers; i++) {
    row = &parameters.cp_matrice[i * matrix_width];
    col = row;
    parameters.tcp_rates[i] = 1;
    sscanf(s, "%d,", &col[0]);
    s += 2;
    if (col[0] > 9)
    s++;
    col[1] = 0;
    col[2] = 0;
    for (j = 1; j < numresolution; j++) {
    col += 3;
    sscanf(s, "%d,%d,%d", &col[0], &col[1], &col[2]);
    s += 6;
    if (col[0] > 9)
    s++;
    if (col[1] > 9)
    s++;
    if (col[2] > 9)
    s++;
    }
    if (i < numlayers - 1)
    s++;
    }
    parameters.cp_fixed_alloc = 1;
    */

    // check parameters that only a single quality setting is active:
    if ((parameters.cp_disto_alloc || parameters.cp_fixed_alloc || parameters.cp_fixed_quality) && (!(parameters.cp_disto_alloc ^ parameters.cp_fixed_alloc ^ parameters.cp_fixed_quality))) {
        fprintf(stderr, "Error, exactly one of cp_disto_alloc, cp_fixed_alloc and cp_fixed_quality should be used..\n");
        return 1;
    }


    // Tile offset
    // -TP <R|L|C>
    //     Divide packets of every tile into tile-parts.
    //     Division is made by grouping Resolutions (R), Layers (L)
    //     or Components (C).
    //
    // Tile size. <tile width>,<tile height>
    // Default: the dimension of the whole image, thus only one tile.
    parameters.tile_size_on = OPJ_FALSE;
    /*
    //parameters.tile_size_on = OPJ_TRUE;
    if (parameters.tile_size_on) {
    // cp_tx0, cp_ty0 = tile offset X, tile offset Y, the offset of the origin of the tiles:
    parameters.cp_tx0 = 0;
    parameters.cp_ty0 = 0;
    parameters.cp_tdx = 512;
    parameters.cp_tdy = 512;
    }
    */

    // Coordonnate of the reference grid, image offset X and Y. Offset of the
    // origin of the image.
    /*
    parameters.image_offset_x0 = ;
    parameters.image_offset_y0 = ;
    */

    if ((parameters.cp_tx0 > parameters.image_offset_x0) || (parameters.cp_ty0 > parameters.image_offset_y0)) {
        fprintf(stderr, "Error: the tile offset dimensions are incorrect. tx0 (%d) should be smaller or equal image width (%d), tyO (%d) smaller or equal image height (%d).\n", parameters.cp_tx0, parameters.image_offset_x0, parameters.cp_ty0, parameters.image_offset_y0);
        return 1;
    }



    // Number of resolutions. It corresponds to the number of DWT
    // decompositions +1.
    //parameters.numresolution = 6;
    parameters.numresolution = 8;


    // Precinct size. Values specified must be power of 2. Multiple records
    // may be supplied, in which case the first record refers to the highest
    // resolution level and subsequent records to lower resolution levels.
    // The last specified record is right-shifted for each remaining lower
    // resolution levels. Default: 215x215 at each resolution.
    /*
    parameters.prcw_init[0] = 512;
    parameters.prch_init[0] = 512;
    parameters.prcw_init[0] = 256;
    parameters.prch_init[0] = 256;
    parameters.prcw_init[0] = 128;
    parameters.prch_init[0] = 128;
    parameters.res_spec = 3;
    parameters.csty |= 0x01;
    */


    // Code-block size. The dimension must respect the constraint defined in
    // the JPEG-2000 standard (no dimension smaller than 4 or greater than 1024,
    // no code-block with more than 4096 coefficients). The maximum value
    // authorized is 64x64. Default: 64x64.
    /*
    int cblockw_init = 64;
    int cblockh_init = 64;
    if (cblockw_init * cblockh_init > 4096 || cblockw_init > 1024 || cblockw_init < 4 || cblockh_init > 1024 || cblockh_init < 4) {
    fprintf(stderr, "Error: Size of code_block error. Restriction: width*height <= 4096, 4 <= width, height <= 1024.\n");
    return 1;
    }
    parameters.cblockw_init = cblockw_init;
    parameters.cblockh_init = cblockh_init;
    */


#ifdef FIXME_INDEX
    // Create an index file.
    // Index structure:
    // ----------------
    //
    // Image_height Image_width
    // progression order
    // Tiles_size_X Tiles_size_Y
    // Tiles_nb_X Tiles_nb_Y
    // Components_nb
    // Layers_nb
    // decomposition_levels
    // [Precincts_size_X_res_Nr Precincts_size_Y_res_Nr]...
    //    [Precincts_size_X_res_0 Precincts_size_Y_res_0]
    // Main_header_start_position
    // Main_header_end_position
    // Codestream_size
    //
    // INFO ON TILES
    // tileno start_pos end_hd end_tile nbparts disto nbpix disto/nbpix
    // Tile_0 start_pos end_Theader end_pos NumParts TotalDisto NumPix MaxMSE
    // Tile_1   ''           ''        ''        ''       ''    ''      ''
    // ...
    // Tile_Nt   ''           ''        ''        ''       ''    ''     ''
    // ...
    // TILE 0 DETAILS
    // part_nb tileno num_packs start_pos end_tph_pos end_pos
    // ...
    // Progression_string
    // pack_nb tileno layno resno compno precno start_pos end_ph_pos end_pos disto
    // Tpacket_0 Tile layer res. comp. prec. start_pos end_pos disto
    // ...
    // Tpacket_Np ''   ''    ''   ''    ''       ''       ''     ''
    // MaxDisto
    // TotalDisto
    const char* index = "/some/path/to/indexfile";
    strncpy(indexfilename, index, OPJ_PATH_LEN);
    fprintf(stderr, "Error: Index file generation is currently broken, please do not use.\n");
    return 1;
#endif


    // Progression order. One of LRCP|RLCP|RPCL|PCRL|CPRL. Default: LRCP.
    // The characters in the text strings represent the following: L = layer, R = resolution, C = component and P = position.
    const char progression[5] = "LRCP";

    // Progression order change.
    // Example:  T1=0,0,1,5,3,CPRL/T1=5,0,1,6,3,CPRL
    /*
    // number of progression order change (POC), default 0:
    parameters.numpocs = 2;

    parameters.POC[0].tile = 1;
    parameters.POC[0].resno0 = 0;
    parameters.POC[0].compno0 = 0;
    parameters.POC[0].layno1 = 1;
    parameters.POC[0].resno1 = 5;
    parameters.POC[0].compno1 = 3;
    parameters.POC[0].progorder = "CPRL";

    parameters.POC[1].tile = 1;
    parameters.POC[1].resno0 = 5;
    parameters.POC[1].compno0 = 0;
    parameters.POC[1].layno1 = 1;
    parameters.POC[1].resno1 = 6;
    parameters.POC[1].compno1 = 3;
    parameters.POC[1].progorder = "CPRL";

    for (int i = 0; i < parameters.numpocs; ++i) {
    if        (strncmp(parameters.POC[i].progorder, "LRCP", 4) == 0) {
    parameters.POC[i].prg1 = OPJ_LRCP;
    } else if (strncmp(parameters.POC[i].progorder, "RLCP", 4) == 0) {
    parameters.POC[i].prg1 = OPJ_RLCP;
    } else if (strncmp(parameters.POC[i].progorder, "RPCL", 4) == 0) {
    parameters.POC[i].prg1 = OPJ_RPCL;
    } else if (strncmp(parameters.POC[i].progorder, "PCRL", 4) == 0) {
    parameters.POC[i].prg1 = OPJ_PCRL;
    } else if (strncmp(parameters.POC[i].progorder, "CPRL", 4) == 0) {
    parameters.POC[i].prg1 = OPJ_CPRL;
    } else {
    fprintf(stderr, "Unrecognized progression order %s, should be one of [LRCP, RLCP, RPCL, PCRL, CPRL].\n", parameters.POC[i].progorder);
    return 1;
    }
    }
    */


    // Subsampling factor. Note that subsampling bigger than 2 can produce an
    // error. Default: no subsampling.
    /*
    parameters.subsampling_dx = ;
    parameters.subsampling_dy = ;
    */




    // Write SOP marker before each packet.
    /*
    parameters.csty |= 0x02;
    */


    // Write EPH marker after each header packet.
    /*
    parameters.csty |= 0x04;
    */


    // Mode switch. Indicate multiple modes by adding their values.
    //  1 = BYPASS(LAZY)
    //  2 = RESET
    //  4 = RESTART(TERMALL)
    //  8 = VSC
    // 16 = ERTERM(SEGTERM)
    // 32 = SEGMARK(SEGSYM)
    /*
    parameters.mode = 12;
    */


    // ROI component index, ROI upshifting value. Quantization indices upshifted
    // for a component. Warning: This option does not implement the usual ROI
    // (Region of Interest). It should be understood as a 'Component of Interest'.
    // It offers the possibility to upshift the value of a component during the
    // quantization step. Component number is [0, 1, 2, ...], upshifting value
    // must be in the range [0, 37].
    /*
    parameters.roi_compno = 1;
    parameters.roi_shift = 5;
    */


    // TODO FIXME: get the comment from the image metadata:
    // Add <comment> in the comment marker segment.
    /*
    parameters.cp_comment = (char*)malloc(strlen(opj_optarg) + 1);
    if (parameters.cp_comment) {
    strcpy(parameters.cp_comment, opj_optarg);
    }

    // Create a default comment if none was given:
    if (parameters.cp_comment == NULL) {
    const char comment[] = "Created by OpenJPEG version ";
    const size_t clen = strlen(comment);
    const char* version = opj_version();
    #ifdef USE_JPWL
    parameters.cp_comment = (char*)malloc(clen + strlen(version) + 11);
    sprintf(parameters.cp_comment, "%s%s with JPWL", comment, version);
    #else
    parameters.cp_comment = (char*)malloc(clen + strlen(version) + 1);
    sprintf(parameters.cp_comment, "%s%s", comment, version);
    #endif
    }
    */


    // Use the irreversible DWT 9-7.
    /*
    parameters.irreversible = 1;
    */


    // Tile part generation
    //parameters.tp_flag = ...;
    //parameters.tp_on = 1;


    // Digital Cinema 2K profile compliant codestream.
    // Need to specify the frames per second for a 2K resolution. Only
    // 24 or 48 fps are currently allowed.
    /*
    const int fps = 24;
    //const int fps = 48;
    if (fps == 24) {
    parameters.rsiz          = OPJ_PROFILE_CINEMA_2K;
    parameters.max_comp_size = OPJ_CINEMA_24_COMP;
    parameters.max_cs_size   = OPJ_CINEMA_24_CS;
    } else if (fps == 48) {
    parameters.rsiz          = OPJ_PROFILE_CINEMA_2K;
    parameters.max_comp_size = OPJ_CINEMA_48_COMP;
    parameters.max_cs_size   = OPJ_CINEMA_48_CS;
    } else {
    fprintf(stderr, "Incorrect value for frames per second in Cinema 2K profile. Must be 24 or 48, given %d.\n", fps);
    return 1;
    }
    fprintf(stdout, "CINEMA 2K profile activated. Other options might be overriden by this profile.\n");
    */


    // Digital Cinema 4K profile compliant codestream.
    // Frames per second not required. Default value is 24fps.
    /*
    parameters.rsiz = OPJ_PROFILE_CINEMA_4K;
    fprintf(stdout, "CINEMA 4K profile activated. Other options might be overriden by this profile.\n");
    */



    // Explicitely specifies if a Multiple Component Transform has to be used.
    //  - 0: no MCT
    //  - 1: RGB->YCC conversion
    //  - 2: custom MCT
    // HINT: if the number of image components is >= 3, it is a good idea
    // to enable Multiple Component Transform.
    if (opj_image->numcomps >= 3) {
        parameters.tcp_mct = 1;
    }
    else {
        parameters.tcp_mct = 0;
    }

    if (parameters.tcp_mct < 0 || parameters.tcp_mct > 2) {
        fprintf(stderr, "MCT incorrect value. Accepted values are 0, 1 or 2.\n");
        return 1;
    }
    if ((parameters.tcp_mct == 1) && (opj_image->numcomps < 3)) {
        fprintf(stderr, "RGB->YCC Multiple Component Transform cannot be used, the image has less than 3 components.\n");
        return 1;
    }


    // Use a custom Multiple Component Transform from a file.
    // Use array-based MCT, values are coma separated, line by line
    // No specific separators between lines, no space allowed between values.
    if (parameters.tcp_mct == 2) {
        char* lFilename = "/some/path/to/mct-file-spec";
        char* lMatrix;
        char* lCurrentPtr;
        float* lCurrentDoublePtr;
        float* lSpace;
        int* l_int_ptr;
        int lNbComp = 0, lTotalComp, lMctComp, i2;
        size_t lStrLen, lStrFread;

        FILE* lFile = fopen(lFilename, "r");
        if (lFile == NULL) {
            fprintf(stderr, "Error opening MCT file '%s'.\n", lFilename);
            return 1;
        }

        // Set size of file and read its content
        fseek(lFile, 0, SEEK_END);
        lStrLen = (size_t)ftell(lFile);
        fseek(lFile, 0, SEEK_SET);
        lMatrix = (char*)malloc(lStrLen + 1);
        lStrFread = fread(lMatrix, 1, lStrLen, lFile);
        fclose(lFile);
        if (lStrLen != lStrFread) {
            fprintf(stderr, "Error reading MCT file '%s'.\n", lFilename);
            return 1;
        }

        lMatrix[lStrLen] = 0;
        lCurrentPtr = lMatrix;

        // replace ',' by 0
        while (*lCurrentPtr != 0) {
            if (*lCurrentPtr == ' ') {
                *lCurrentPtr = 0;
                ++lNbComp;
            }
            ++lCurrentPtr;
        }
        ++lNbComp;
        lCurrentPtr = lMatrix;

        lNbComp = (int)(sqrt(4 * lNbComp + 1) / 2. - 0.5);
        lMctComp = lNbComp * lNbComp;
        lTotalComp = lMctComp + lNbComp;
        lSpace = (float*)malloc((size_t)lTotalComp * sizeof(float));
        lCurrentDoublePtr = lSpace;
        for (i2 = 0; i2 < lMctComp; ++i2) {
            lStrLen = strlen(lCurrentPtr) + 1;
            *lCurrentDoublePtr++ = (float)atof(lCurrentPtr);
            lCurrentPtr += lStrLen;
        }

        l_int_ptr = (int*)lCurrentDoublePtr;
        for (i2 = 0; i2 < lNbComp; ++i2) {
            lStrLen = strlen(lCurrentPtr) + 1;
            *l_int_ptr++ = atoi(lCurrentPtr);
            lCurrentPtr += lStrLen;
        }

        // TODO should not be here
        opj_set_MCT(&parameters, lSpace, (int*)(lSpace + lMctComp), (OPJ_UINT32)lNbComp);

        // Free memory
        free(lSpace);
        free(lMatrix);

        if (!parameters.mct_data) {
            fprintf(stderr, "Custom MCT has been set but no array-based MCT has been provided. Aborting.\n");
            return 1;
        }
    }



    // Write jpip codestream index box in JP2 output file. Currently supports
    // only RPCL order.
    /*
    parameters.jpip_on = OPJ_TRUE;
    */





#ifdef USE_JPWL
    /*
    std::cerr << "WARNING: write_jp2_image(): you are using untested JPWL code."
    << " Use at your own discretion." << std::endl;
    // TODO FIXME: Currently JPWL is not active and untested. The code will most
    // likely NOT work, so please check carefully before activating/using.
    //
    // JPWL capabilities switched on
    // -W <params>
    //     Adoption of JPWL (Part 11) capabilities (-W params)
    //     The <params> field can be written and repeated in any order:
    //     [h<tilepart><=type>,s<tilepart><=method>,a=<addr>,...
    //     ...,z=<size>,g=<range>,p<tilepart:pack><=type>]
    //      h selects the header error protection (EPB): 'type' can be
    //        [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]
    //        if 'tilepart' is absent, it is for main and tile headers
    //        if 'tilepart' is present, it applies from that tile
    //          onwards, up to the next h<> spec, or to the last tilepart
    //          in the codestream (max. JPWL_MAX_NO_TILESPECS specs
    //      p selects the packet error protection (EEP/UEP with EPBs)
    //       to be applied to raw data: 'type' can be
    //        [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]
    //        if 'tilepart:pack' is absent, it is from tile 0, packet 0
    //        if 'tilepart:pack' is present, it applies from that tile
    //          and that packet onwards, up to the next packet spec
    //          or to the last packet in the last tilepart in the stream
    //          (max. JPWL_MAX_NO_PACKSPECS specs)
    //      s enables sensitivity data insertion (ESD): 'method' can be
    //        [-1=NO ESD 0=RELATIVE ERROR 1=MSE 2=MSE REDUCTION 3=PSNR
    //         4=PSNR INCREMENT 5=MAXERR 6=TSE 7=RESERVED]
    //        if 'tilepart' is absent, it is for main header only
    //        if 'tilepart' is present, it applies from that tile
    //          onwards, up to the next s<> spec, or to the last tilepart
    //          in the codestream (max. JPWL_MAX_NO_TILESPECS specs)
    //      g determines the addressing mode: <range> can be
    //        [0=PACKET 1=BYTE RANGE 2=PACKET RANGE]
    //      a determines the size of data addressing: <addr> can be
    //        2/4 bytes (small/large codestreams). If not set, auto-mode
    //      z determines the size of sensitivity values: <size> can be
    //        1/2 bytes, for the transformed pseudo-floating point value
    //      ex.:
    //        h,h0=64,h3=16,h5=32,p0=78,p0:24=56,p1,p3:0=0,p3:20=32,s=0,
    //          s0=6,s3=-1,a=0,g=1,z=1
    //      means
    //        predefined EPB in MH, rs(64,32) from TPH 0 to TPH 2,
    //        CRC-16 in TPH 3 and TPH 4, CRC-32 in remaining TPHs,
    //        UEP rs(78,32) for packets 0 to 23 of tile 0,
    //        UEP rs(56,32) for packs. 24 to the last of tilepart 0,
    //        UEP rs default for packets of tilepart 1,
    //        no UEP for packets 0 to 19 of tilepart 3,
    //        UEP CRC-32 for packs. 20 of tilepart 3 to last tilepart,
    //        relative sensitivity ESD for MH,
    //        TSE ESD from TPH 0 to TPH 2, byte range with automatic
    //        size of addresses and 1 byte for each sensitivity value
    //      ex.:
    //            h,s,p
    //      means
    //        default protection to headers (MH and TPHs) as well as
    //        data packets, one ESD in MH
    //      N.B.: use the following recommendations when specifying
    //            the JPWL parameters list
    //        - when you use UEP, always pair the 'p' option with 'h'
    char* token = NULL;
    int hprot, pprot, sens, addr, size, range;

    // we need to enable indexing
    if (!indexfilename || !*indexfilename) {
    strncpy(indexfilename, JPWL_PRIVATEINDEX_NAME, OPJ_PATH_LEN);
    }

    // search for different protection methods

    // break the option in comma points and parse the result
    token = strtok(opj_optarg, ",");
    while (token != NULL) {
    // search header error protection method
    if (*token == 'h') {
    static int tile = 0, tilespec = 0, lasttileno = 0;

    hprot = 1; // predefined method

    if (sscanf(token, "h=%d", &hprot) == 1) {
    // Main header, specified
    if (!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
    ((hprot >= 37) && (hprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid main header protection method h = %d\n", hprot);
    return 1;
    }
    parameters.jpwl_hprot_MH = hprot;
    } else if (sscanf(token, "h%d=%d", &tile, &hprot) == 2) {
    // Tile part header, specified
    if (!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
    ((hprot >= 37) && (hprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid tile part header protection method h = %d\n", hprot);
    return 1;
    }
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method t = %d\n", tile);
    return 1;
    }
    if (tilespec < JPWL_MAX_NO_TILESPECS) {
    parameters.jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
    parameters.jpwl_hprot_TPH[tilespec++] = hprot;
    }
    } else if (sscanf(token, "h%d", &tile) == 1) {
    // Tile part header, unspecified
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method t = %d\n", tile);
    return 1;
    }
    if (tilespec < JPWL_MAX_NO_TILESPECS) {
    parameters.jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
    parameters.jpwl_hprot_TPH[tilespec++] = hprot;
    }
    } else if (!strcmp(token, "h")) {
    // Main header, unspecified
    parameters.jpwl_hprot_MH = hprot;

    } else {
    fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
    return 1;
    };
    }

    // search packet error protection method
    if (*token == 'p') {
    static int pack = 0, tile = 0, packspec = 0;

    pprot = 1; // predefined method

    if (sscanf(token, "p=%d", &pprot) == 1) {
    // Method for all tiles and all packets
    if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
    ((pprot >= 37) && (pprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid default packet protection method p = %d\n", pprot);
    return 1;
    }
    parameters.jpwl_pprot_tileno[0] = 0;
    parameters.jpwl_pprot_packno[0] = 0;
    parameters.jpwl_pprot[0] = pprot;

    } else if (sscanf(token, "p%d=%d", &tile, &pprot) == 2) {
    // method specified from that tile on
    if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
    ((pprot >= 37) && (pprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
    return 1;
    }
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
    return 1;
    }
    if (packspec < JPWL_MAX_NO_PACKSPECS) {
    parameters.jpwl_pprot_tileno[packspec] = tile;
    parameters.jpwl_pprot_packno[packspec] = 0;
    parameters.jpwl_pprot[packspec++] = pprot;
    }
    } else if (sscanf(token, "p%d:%d=%d", &tile, &pack, &pprot) == 3) {
    // method fully specified from that tile and that packet on
    if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
    ((pprot >= 37) && (pprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
    return 1;
    }
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
    return 1;
    }
    if (pack < 0) {
    fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n", pack);
    return 1;
    }
    if (packspec < JPWL_MAX_NO_PACKSPECS) {
    parameters.jpwl_pprot_tileno[packspec] = tile;
    parameters.jpwl_pprot_packno[packspec] = pack;
    parameters.jpwl_pprot[packspec++] = pprot;
    }
    } else if (sscanf(token, "p%d:%d", &tile, &pack) == 2) {
    // default method from that tile and that packet on
    if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
    ((pprot >= 37) && (pprot <= 128)))) {
    fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
    return 1;
    }
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
    return 1;
    }
    if (pack < 0) {
    fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n", pack);
    return 1;
    }
    if (packspec < JPWL_MAX_NO_PACKSPECS) {
    parameters.jpwl_pprot_tileno[packspec] = tile;
    parameters.jpwl_pprot_packno[packspec] = pack;
    parameters.jpwl_pprot[packspec++] = pprot;
    }
    } else if (sscanf(token, "p%d", &tile) == 1) {
    // default from a tile on
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
    return 1;
    }
    if (packspec < JPWL_MAX_NO_PACKSPECS) {
    parameters.jpwl_pprot_tileno[packspec] = tile;
    parameters.jpwl_pprot_packno[packspec] = 0;
    parameters.jpwl_pprot[packspec++] = pprot;
    }
    } else if (!strcmp(token, "p")) {
    // all default
    parameters.jpwl_pprot_tileno[0] = 0;
    parameters.jpwl_pprot_packno[0] = 0;
    parameters.jpwl_pprot[0] = pprot;

    } else {
    fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
    return 1;
    };
    }

    // search sensitivity method
    if (*token == 's') {
    static int tile = 0, tilespec = 0, lasttileno = 0;

    sens = 0; // predefined: relative error

    if (sscanf(token, "s=%d", &sens) == 1) {
    // Main header, specified
    if ((sens < -1) || (sens > 7)) {
    fprintf(stderr, "ERROR -> invalid main header sensitivity method s = %d\n", sens);
    return 1;
    }
    parameters.jpwl_sens_MH = sens;
    } else if (sscanf(token, "s%d=%d", &tile, &sens) == 2) {
    // Tile part header, specified
    if ((sens < -1) || (sens > 7)) {
    fprintf(stderr, "ERROR -> invalid tile part header sensitivity method s = %d\n", sens);
    return 1;
    }
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
    return 1;
    }
    if (tilespec < JPWL_MAX_NO_TILESPECS) {
    parameters.jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
    parameters.jpwl_sens_TPH[tilespec++] = sens;
    }
    } else if (sscanf(token, "s%d", &tile) == 1) {
    // Tile part header, unspecified
    if (tile < 0) {
    fprintf(stderr, "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
    return 1;
    }
    if (tilespec < JPWL_MAX_NO_TILESPECS) {
    parameters.jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
    parameters.jpwl_sens_TPH[tilespec++] = hprot;
    }
    } else if (!strcmp(token, "s")) {
    // Main header, unspecified
    parameters.jpwl_sens_MH = sens;
    } else {
    fprintf(stderr, "ERROR -> invalid sensitivity method selection = %s\n", token);
    return 1;
    };

    parameters.jpwl_sens_size = 2; // 2 bytes for default size
    }

    // search addressing size
    if (*token == 'a') {
    addr = 0; // predefined: auto

    if (sscanf(token, "a=%d", &addr) == 1) {
    // Specified
    if ((addr != 0) && (addr != 2) && (addr != 4)) {
    fprintf(stderr, "ERROR -> invalid addressing size a = %d\n", addr);
    return 1;
    }
    parameters.jpwl_sens_addr = addr;
    } else if (!strcmp(token, "a")) {
    // default
    parameters.jpwl_sens_addr = addr; // auto for default size
    } else {
    fprintf(stderr, "ERROR -> invalid addressing selection = %s\n", token);
    return 1;
    };
    }

    // search sensitivity size
    if (*token == 'z') {
    size = 1; // predefined: 1 byte

    if (sscanf(token, "z=%d", &size) == 1) {
    // Specified
    if ((size != 0) && (size != 1) && (size != 2)) {
    fprintf(stderr, "ERROR -> invalid sensitivity size z = %d\n", size);
    return 1;
    }
    parameters.jpwl_sens_size = size;
    } else if (!strcmp(token, "a")) {
    // default
    parameters.jpwl_sens_size = size; // 1 for default size
    } else {
    fprintf(stderr, "ERROR -> invalid size selection = %s\n", token);
    return 1;
    };
    }

    // search range method
    if (*token == 'g') {
    range = 0; // predefined: 0 (packet)

    if (sscanf(token, "g=%d", &range) == 1) {
    // Specified
    if ((range < 0) || (range > 3)) {
    fprintf(stderr, "ERROR -> invalid sensitivity range method g = %d\n", range);
    return 1;
    }
    parameters.jpwl_sens_range = range;

    } else if (!strcmp(token, "g")) {
    // default
    parameters.jpwl_sens_range = range;

    } else {
    fprintf(stderr, "ERROR -> invalid range selection = %s\n", token);
    return 1;
    };
    }

    // next token or bust
    token = strtok(NULL, ",");
    };

    // some info
    fprintf(stdout, "Info: JPWL capabilities enabled\n");
    parameters.jpwl_epc_on = OPJ_TRUE;
    */
#endif



    // Debugging output:
#ifdef DEBUG
    print_jp2_encode_parameters(parameters);
#endif



    //*********************************************************************
    // OPEN THE FILE
    //*********************************************************************
    // TODO FIXME: This effectively opens the file again. Better would be
    // to use the file handle from fmtHndl->stream.
    // open a byte stream for writing and allocate memory for all tiles
    l_stream = opj_stream_create_default_file_stream(parameters.outfile, OPJ_FALSE);
    if (!l_stream) {
        std::cerr << "Failed to create a file stream for file "
            << parameters.outfile << "." << std::endl;
        return 1;
    }


    //*********************************************************************
    // SETUP THE JPEG2000 DECODER
    //*********************************************************************
    // Create an encoder for the JPEG2000 stream
    switch (parameters.cod_format) {
    case J2K_CFMT:
        // JPEG-2000 codestream
#ifdef DEBUG
        std::cerr << "Setting file compressor to OPJ_CODEC_J2K." << std::endl;
#endif
        l_codec = opj_create_compress(OPJ_CODEC_J2K);
        break;
    case JP2_CFMT:
        // JPEG 2000 compressed image data
#ifdef DEBUG
        std::cerr << "Setting file compressor to OPJ_CODEC_JP2." << std::endl;
#endif
        l_codec = opj_create_compress(OPJ_CODEC_JP2);
        break;
    default:
        fprintf(stderr, "Could not parse encoding format.\n");
        opj_stream_destroy(l_stream);
        return 1;
    }

    // Catch events using our own callbacks, and give a local context
    opj_set_info_handler(l_codec, jp2_info_callback, 00);
    opj_set_warning_handler(l_codec, jp2_warning_callback, 00);
    opj_set_error_handler(l_codec, jp2_error_callback, 00);

    // Setup the encoding parameters using the parameters set above:
    if (!opj_setup_encoder(l_codec, &parameters, opj_image)) {
        std::cerr << "Failed to setup the encoder" << std::endl;
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
    }


    // encode the image
    bSuccess = opj_start_compress(l_codec, opj_image, l_stream);
    if (!bSuccess) {
        fprintf(stderr, "Error: Failed to encode image in opj_start_compress().\n");
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(opj_image);
        remove(parameters.outfile);
        return 1;
    }

    if (parameters.tile_size_on) {
        OPJ_BYTE* l_data;
        OPJ_UINT32 l_data_size = 512 * 512 * 3;
        OPJ_UINT32 l_nb_tiles = 4;
        l_data = (OPJ_BYTE*)malloc(l_data_size * sizeof(OPJ_BYTE));
        memset(l_data, 0, l_data_size);
        for (int i = 0; i < l_nb_tiles; ++i) {
            bSuccess = opj_write_tile(l_codec, i, l_data, l_data_size, l_stream);
            if (!bSuccess) {
                fprintf(stderr, "Error: Failed to write tile %d of %d. Aborted.\n", i, l_nb_tiles);
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(opj_image);
                free(l_data);
                remove(parameters.outfile);
                return 1;
            }
        }
        free(l_data);
    }
    else {
        bSuccess = opj_encode(l_codec, l_stream);
        if (!bSuccess) {
            fprintf(stderr, "Error: Failed to encode image in opj_encode().\n");
            opj_stream_destroy(l_stream);
            opj_destroy_codec(l_codec);
            opj_image_destroy(opj_image);
            remove(parameters.outfile);
            return 1;
        }
    }

    bSuccess = opj_end_compress(l_codec, l_stream);
    if (!bSuccess) {
        fprintf(stderr, "Error: Failed to encode image in opj_end_compress().\n");
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(opj_image);
        remove(parameters.outfile);
        return 1;
    }

#ifdef DEBUG
    fprintf(stdout, "Generated compressed outfile '%s'.\n", parameters.outfile);
#endif

    // close and free the byte stream
    opj_stream_destroy(l_stream);

    // free remaining compression structures
    opj_destroy_codec(l_codec);

    // free image data
    opj_image_destroy(opj_image);


    // free user parameters structure
    if (parameters.cp_comment)
        free(parameters.cp_comment);
    if (parameters.cp_matrice)
        free(parameters.cp_matrice);

    return 0;
}
