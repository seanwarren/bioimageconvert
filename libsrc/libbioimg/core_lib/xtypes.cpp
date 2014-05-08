/*****************************************************************************
 Base typing and type conversion definitions

 IMPLEMENTATION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2006 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   04/19/2006 16:20 - First creation

 Ver : 1
*****************************************************************************/

#include "xtypes.h"

using namespace bim;

//------------------------------------------------------------------------------
// SWAP TYPES
//------------------------------------------------------------------------------

void bim::swapArrayOfShort(uint16* wp, register uint64 n) {
  register uchar* cp;
  register uchar t;

  while (n-- > 0) {
    cp = (uchar*) wp;
    t = cp[1]; cp[1] = cp[0]; cp[0] = t;
    wp++;
  }
}

void bim::swapArrayOfLong(register uint32* lp, register uint64 n) {
  register uchar *cp;
  register uchar t;

  while (n-- > 0) {
    cp = (uchar *)lp;
    t = cp[3]; cp[3] = cp[0]; cp[0] = t;
    t = cp[2]; cp[2] = cp[1]; cp[1] = t;
    lp++;
  }
}

void bim::swapArrayOfFloat(register float32* lp, register uint64 n) {
  register uchar *cp;
  register uchar t;

  while (n-- > 0) {
    cp = (uchar *)lp;
    t = cp[3]; cp[3] = cp[0]; cp[0] = t;
    t = cp[2]; cp[2] = cp[1]; cp[1] = t;
    lp++;
  }
}

void bim::swapDouble(float64 *dp) {
  register uint32* lp = (uint32*) dp;
  uint32 t;
  bim::swapArrayOfLong(lp, 2);
  t = lp[0]; lp[0] = lp[1]; lp[1] = t;
}

void bim::swapArrayOfDouble(float64* dp, register uint64 n) {
  register uint32* lp = (uint32*) dp;
  register uint32 t;

  bim::swapArrayOfLong(lp, n + n);
  while (n-- > 0) {
    t = lp[0]; lp[0] = lp[1]; lp[1] = t;
    lp += 2;
  }
}

