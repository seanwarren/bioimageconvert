/*****************************************************************************
 Kenney Registration Method Implementation

 Original idea and Matlab Code by: C. Kenney
 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 This code is under the copyright of:
   Visual Lab, Department of Electrical and Computer Engineering
   University of California, Santa Barbara, CA, USA.
   
   Image Processing Division,
   National Institute for Space Research (INPE), Brazil. 

 This project is supported in part by: 
   CAPES, SELPER, Office of Naval Research, 
   China Lake Naval Air Warfare Center and CalTrans.

 People: 
   Dmitry V. Fedorov, Leila M. G. Fonseca, Charles Kenney, B.S. Manjunath.

 History:
   10/20/2001 13:05:33 - First creation

 Ver : 17
*****************************************************************************/

#ifndef REG_REGISTRATION_KENNEY
#define REG_REGISTRATION_KENNEY

#include <cmath>

#include <list>
#include <vector>
#include <algorithm>

#include "resolvent.h"
#include "findmaxima.h"
#include "getfeatures.h"
#include "matchfeatures.h"
#include "harmonicdiff.h"
#include "evalfit.h"
#include "geometry.h"

#include "fiterror.h"

#include "RSTreg.h"

#include "evalfitAFFINE.h"
#include "AFFINEreg.h"

template< typename Timg, typename Tpoint, typename Tw >
int registerImgKenney(bim::Image *img, bim::Image *imG, reg::Params *regParams) {

  reg::Matrix<Timg> rbmp1(img);
  reg::Matrix<Timg> rbmp2(imG);

  int numpoints = regParams->numpoints;
  Tw **m = regParams->m;
  Tw **v = regParams->v;

  int Error = REG_OK;  
  try {
    if ( (regParams->transformation == reg::RST) || (regParams->transformation == reg::ST) ) {
      double r, a, dx, dy;  
      Error = RSTreg<Timg, Tpoint, Tw>(&rbmp1, &rbmp2, numpoints, &r, &a, &dx, &dy, regParams);
      reg::rst2aff(r, a, dx, dy, m, v);
    } else
    if (regParams->transformation == reg::Translation) {
      double r, a, dx, dy;  
      Error = RSTreg<Timg, Tpoint, Tw>(&rbmp1, &rbmp2, numpoints, &r, &a, &dx, &dy, regParams);
      reg::rst2aff(1, 0, dx, dy, m, v);
    } else
    if ( (regParams->transformation == reg::Affine) || (regParams->transformation == reg::ProjectiveNS) ) {
      double fit0, fit1;
      Error = AFFINEreg<Timg, Tpoint, Tw>(&rbmp1, &rbmp2, numpoints, m, v, &fit0, &fit1, regParams);
    }
  } catch (...) {
    return REG_ER_DURING_PROCESS;
  }

  return Error;
}

#endif // REG_REGISTRATION_KENNEY

