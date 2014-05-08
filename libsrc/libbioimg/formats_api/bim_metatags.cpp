/*****************************************************************************
 Tag names for metadata

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2010 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   2010-07-29 17:18:22 - First creation

 Ver : 1
*****************************************************************************/

#include "bim_metatags.h"

// redefine the declaration macro in order to get proper initialization
#undef DECLARE_STR
#define DECLARE_STR(S,V) const std::string bim::S = V;

#include "bim_metatags.def" // include actual string data

