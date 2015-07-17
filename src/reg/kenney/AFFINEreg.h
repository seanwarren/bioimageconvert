/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg, mG - pointers to TMatrix, images to mosaic, gray scale, in REAL
OUTPUT:
  m, v - pointers to matrix, m-2x2, c-2x1 SHOULD BE INITIALIZED!!!
  dx, dy - pointers to double
  xyA, XYA - pointers to TPoint, DO NOT INIT THEM!!!!
  goodbad - pointer to int

Notes: 
  It's not an exact copy of matlab code:
  string output, time counting and input variables should be treated from GUI
  program. 

Data structures considerations:
  * All arrays and matrixes starts in 1 for Matlab code compatibility.
  * In/Out Pointers are not initialized then it is specified in function header!
    Take care to follow instructions to prevent junk in the memory!


 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/27/2001 15:09:30 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 2
*****************************************************************************/

#ifndef REG_AFFINE_REGISTRATION
#define REG_AFFINE_REGISTRATION

template< typename Timg, typename Tpoint, typename Tw >
int AFFINEreg(reg::Matrix<Timg> *mg, reg::Matrix<Timg> *mG, int numpoints,
               Tw **m, Tw **v, double *fit0, double *fit1, reg::Params *regParams) {

  double r, a, dx, dy;
  int Error = RSTreg<Timg, Tpoint, Tw>(mg, mG, numpoints, &r, &a, &dx, &dy, regParams);
  reg::rst2aff(r, a, dx, dy, m, v);

  //tdfShow(regParams->filtParams, "Affine: get parameters\0", 0);
  // check the RST fit
  double temp;
  evalfitAFFINE(mg, mG, m, v, fit0, &temp, &temp, &temp, &temp, &temp);

  // Get the Affine transform from the points xy and xxyy
  Tw **ma = m_init<Tw>(2, 2);
  Tw **va = m_init<Tw>(2, 1);
  std::deque< reg::Point<Tpoint> > *points1 = &regParams->tiePoints1; 
  std::deque< reg::Point<Tpoint> > *points2 = &regParams->tiePoints2;
  getAFFINEparameters(points1, points2, ma, va);

  //tdfShow(regParams->filtParams, "Affine: evaluate\0", 50);
  // check the Affine fit
  evalfitAFFINE(mg, mG, ma, va, fit1, &temp, &temp, &temp, &temp, &temp);

  if (*fit0 > *fit1) {
    m_copy(ma, m, 2, 2);
    m_copy(va, v, 2, 1);    
  }
  m_free(ma);
  m_free(va);
  
  //tdfShow(regParams->filtParams, "Affine: Done!\0", 100);

  return Error;
}

#endif // REG_AFFINE_REGISTRATION
