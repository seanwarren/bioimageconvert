/*****************************************************************************
 Tag names for metadata

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2010 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   2010-07-29 17:18:22 - First creation

 Ver : 1
*****************************************************************************/

#ifndef BIM_IMG_META_TAGS
#define BIM_IMG_META_TAGS

#include <string>

// define the declaration macro in order to get external const vars in the namespace
#ifndef DECLARE_STR
#define DECLARE_STR(S,V) extern const std::string S;
#endif

namespace bim {

#include "bim_metatags.def" // include actual string data

} // namespace bim

#endif // BIM_IMG_META_TAGS
