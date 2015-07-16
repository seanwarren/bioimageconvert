/*****************************************************************************
  JPEG2000 support 
  Copyright (c) 2015 by Mario Emmenlauer <mario@emmenlauer.de>

  IMPLEMENTATION

  Programmer: Mario Emmenlauer <mario@emmenlauer.de>

  History:
    04/19/2015 14:20 - First creation
        
  Ver : 1
*****************************************************************************/
#ifndef BIM_JP2_FORMAT_IO_H
#define BIM_JP2_FORMAT_IO_H BIM_JP2_FORMAT_IO_H

#include <openjpeg.h>

template<class T>
void print_jp2_encode_parameters(const T& parameters) {
  std::cout << "parameters.tile_size_on        = " << parameters.tile_size_on         << std::endl;
  std::cout << "parameters.cp_tx0              = " << parameters.cp_tx0               << std::endl;
  std::cout << "parameters.cp_ty0              = " << parameters.cp_ty0               << std::endl;
  std::cout << "parameters.cp_tdx              = " << parameters.cp_tdx               << std::endl;
  std::cout << "parameters.cp_tdy              = " << parameters.cp_tdy               << std::endl;
  std::cout << "parameters.cp_disto_alloc      = " << parameters.cp_disto_alloc       << std::endl;
  std::cout << "parameters.cp_fixed_alloc      = " << parameters.cp_fixed_alloc       << std::endl;
  std::cout << "parameters.cp_fixed_quality    = " << parameters.cp_fixed_quality     << std::endl;
  std::cout << "parameters.cp_matrice          = " << parameters.cp_matrice           << std::endl;
  if (parameters.cp_comment != NULL)
    std::cout << "parameters.cp_comment          = " << parameters.cp_comment         << std::endl;
  else
    std::cout << "parameters.cp_comment          = " << "NULL"                        << std::endl;
  std::cout << "parameters.csty                = " << parameters.csty                 << std::endl;
  std::cout << "parameters.prog_order          = " << parameters.prog_order           << std::endl;
  //std::cout << "parameters.POC                 = " << parameters.POC                  << std::endl;
  std::cout << "parameters.numpocs             = " << parameters.numpocs              << std::endl;
  std::cout << "parameters.tcp_numlayers       = " << parameters.tcp_numlayers        << std::endl;
  //std::cout << "parameters.tcp_rates           = " << parameters.tcp_rates            << std::endl;
  //std::cout << "parameters.tcp_distoratio      = " << parameters.tcp_distoratio       << std::endl;
  std::cout << "parameters.numresolution       = " << parameters.numresolution        << std::endl;
  std::cout << "parameters.cblockw_init        = " << parameters.cblockw_init         << std::endl;
  std::cout << "parameters.cblockh_init        = " << parameters.cblockh_init         << std::endl;
  std::cout << "parameters.mode                = " << parameters.mode                 << std::endl;
  std::cout << "parameters.irreversible        = " << parameters.irreversible         << std::endl;
  std::cout << "parameters.roi_compno          = " << parameters.roi_compno           << std::endl;
  std::cout << "parameters.roi_shift           = " << parameters.roi_shift            << std::endl;
  std::cout << "parameters.res_spec            = " << parameters.res_spec             << std::endl;
  //std::cout << "parameters.prcw_init           = " << parameters.prcw_init            << std::endl;
  //std::cout << "parameters.prch_init           = " << parameters.prch_init            << std::endl;
  if (parameters.infile != NULL)
    std::cout << "parameters.infile              = " << parameters.infile               << std::endl;
  else
    std::cout << "parameters.infile              = " << "NULL"                          << std::endl;
  if (parameters.outfile != NULL)
    std::cout << "parameters.outfile             = " << parameters.outfile              << std::endl;
  else
    std::cout << "parameters.outfile             = " << "NULL"                          << std::endl;
  std::cout << "parameters.image_offset_x0     = " << parameters.image_offset_x0      << std::endl;
  std::cout << "parameters.image_offset_y0     = " << parameters.image_offset_y0      << std::endl;
  std::cout << "parameters.subsampling_dx      = " << parameters.subsampling_dx       << std::endl;
  std::cout << "parameters.subsampling_dy      = " << parameters.subsampling_dy       << std::endl;
  std::cout << "parameters.decod_format        = " << parameters.decod_format         << std::endl;
  std::cout << "parameters.cod_format          = " << parameters.cod_format           << std::endl;
  std::cout << "parameters.max_comp_size       = " << parameters.max_comp_size        << std::endl;
  std::cout << "parameters.tp_on               = " << (int)parameters.tp_on           << std::endl;
  std::cout << "parameters.tp_flag             = " << (int)parameters.tp_flag         << std::endl;
  std::cout << "parameters.tcp_mct             = " << (int)parameters.tcp_mct         << std::endl;
  std::cout << "parameters.jpip_on             = " << parameters.jpip_on              << std::endl;
  //std::cout << "parameters.mct_data          = " << parameters.mct_data               << std::endl;
  std::cout << "parameters.max_cs_size         = " << parameters.max_cs_size          << std::endl;
  std::cout << "parameters.rsiz                = " << parameters.rsiz                 << std::endl;
}

template<class T>
void print_jp2_decode_parameters(const T& parameters) {
  if (parameters.infile != NULL)
    std::cout << "parameters.infile              = " << parameters.infile               << std::endl;
  else
    std::cout << "parameters.infile              = " << "NULL"                          << std::endl;
  if (parameters.outfile != NULL)
    std::cout << "parameters.outfile             = " << parameters.outfile              << std::endl;
  else
    std::cout << "parameters.outfile             = " << "NULL"                          << std::endl;
  std::cout << "parameters.decod_format        = " << parameters.decod_format         << std::endl;
  std::cout << "parameters.cod_format          = " << parameters.cod_format           << std::endl;
}

#endif
