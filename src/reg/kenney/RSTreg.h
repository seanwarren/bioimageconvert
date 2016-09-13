/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg, mG - pointers to TMatrix, images to mosaic, gray scale, in REAL
OUTPUT:
  r, a, dx, dy - pointers to double
  xyA, XYA - pointers to TPoint, DO NOT INIT the array inside THEM!!!!
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
   08/04/2001 16:40:30 - First creation
   08/08/2001 18:01:48 - OK, it worked, now let's do it better!
   08/10/2001 15:06:32 - use of TPointA
   08/15/2001 22:31:19 - Corrected good/bad fit test, 
                         code passed to beta version !
   10/02/2001 19:12:00 - GCC warnings and Linux treatment
   07/11/2002 02:03:17 - Memory leak corrected

 Ver : 6
*****************************************************************************/

#ifndef REG_RST_REGISTRATION
#define REG_RST_REGISTRATION

#include <algorithm>

template< typename Timg, typename Tpoint >
void correctMaxArray(const reg::Matrix<Timg> *mg, std::deque< reg::Point<Tpoint> > *points, int wind) {
  int rg = mg->rows;
  int cg = mg->cols;
  typename std::deque< reg::Point<Tpoint> >::iterator it = points->begin();
  while (it != points->end()) {
    if (it->y < wind+1)  it->y = (Tpoint) wind+1;
    if (it->y > rg-wind) it->y = (Tpoint) rg-wind;
    if (it->x < wind+1)  it->x = (Tpoint) wind+1;
    if (it->x > cg-wind) it->x = (Tpoint) cg-wind;
    ++it;
  }
}

template< typename Timg, typename Tpoint, typename Tw >
int RSTreg(const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *mG, int numpoints, 
           double *r, double *a, double *dx, double *dy, reg::Params *regParams )

// get the rotation-scale-translation (RST) estimates for
//     r = scale factor (zoom in or zoom out, nominal value is r=1)
//     a = rotation angle in degrees
//    dx = x translation
//    dy = y translation
// (x,y) = tiepoints in the image g 
// (X,Y) = tiepoints in the image G 

// input:
// g and G are images of the same size to be registered
{
  // set the internal parameters
  int wind=10;       // window radius for tiepoint feature matching 
  int numrand=16;    // number of random pertrubations to determine good fit- bad fit
  double maxerror=60.0;
  int nummax = std::max<int>(numpoints, 32); // number of tiepoints selected for each feature surface

  double fit, mv, sv, mV, sV;
  double bf, bs, gf, gs;
  double overlap;

  int Error = REG_OK;
  
  *r=0; *a=0; *dx=0; *dy=0;
  int rg = mg->rows;
  int cg = mg->cols;
  int rG = mG->rows;
  int cG = mG->cols;

  std::deque< reg::Point<Tpoint> > *points1 = &regParams->tiePoints1; 
  std::deque< reg::Point<Tpoint> > *points2 = &regParams->tiePoints2;

  // for the feature surface we select the maxima of the surface as tiepoints
  // these tiepoints are culled using windowed matching and then culled again
  // using geometric fitting to arrive at an estimate of r a dx and dy
  std::deque< reg::Point<Tpoint> > candidates1;
  std::deque< reg::Point<Tpoint> > candidates2;

  //tdfShow(regParams->filtParams, "RST: get features 1\0", 0);

  reg::Matrix<Timg> mres(rg, cg);

  reg::Matrix<Timg> mgT(mg);
  resolvent(&mgT, &mres); 
  mgT.free();
  // find the maxima on the LSO surface for g
  findmaxima(&mres, nummax, &candidates1);

  //tdfShow(regParams->filtParams, "RST: get features 2\0", 25);
  mgT.fromMatrix(mG);
  resolvent(&mgT, &mres); 
  // find the maxima on the LSO surface for G
  findmaxima(&mres, nummax, &candidates2);
  mgT.free();

  mres.free();


  // correct some Matlab code errors
  correctMaxArray(mg, &candidates1, wind);
  correctMaxArray(mG, &candidates2, wind);

  // get the rotated windows for each tiepoint in g and G
  reg::Matrix<Timg> mw, mW;
  getfeatures<Tw>(mg, &candidates1, wind, &mw);  
  getfeatures<Tw>(mG, &candidates2, wind, &mW);

  //tdfShow(regParams->filtParams, "RST: match features\0", 50);
  // match the feature vectors in g and G
  std::deque< reg::Point<Tpoint> > matched1;
  std::deque< reg::Point<Tpoint> > matched2;
  matchfeatures(&mw, &mW, &candidates1, &candidates2, numpoints, &matched1, &matched2);

  //tdfShow(regParams->filtParams, "RST: geometry\0", 75);  
  // use the geometry of the tiepoints to find a good match
  double c=0, s=0;
  geometry<Timg, Tpoint, Tw>( mg, mG, &matched1, &matched2, r, &c, &s, dx, dy, points1, points2);

  // get the rotation angle
  *a = atan2( s, c )*180.0/REG_PI;                

  //tdfShow(regParams->filtParams, "RST: evaluate\0", 90);
  // evaluate the fit for the TPA parameter estimates
  evalfit<Timg, float>(mg, mG, *r, c, s, *dx, *dy, &fit, &mv, &sv, &mV, &sV, &overlap);       


  // estimate fit values corresponding to bad parameters
  // the max allowable index error is maxerror
  maxerror=60.0;
  fiterror<Timg, float>(mg, mG, *r, *a, *dx, *dy, numrand, maxerror, &bf, &bs);

  // estimate fit values corresponding to good parameters
  maxerror=2.0;
  fiterror<Timg, float>(mg, mg, 1.0, 0.0, 0.0, 0.0, numrand, maxerror, &gf, &gs);   

  // decide if we have a good fit or a bad fit or can't tell
  int numx = (int) points1->size();


  // find distance from computed fit to good fit in terms of good fit standard devs
  double good = (fit-gf)/(gs+REG_EPS);
  // find distance from computed fit to bad fit in terms of bad fit standard devs
  double bad = fabs(fit-bf)/(bs+REG_EPS);         

  double bim_goodbad = 100.0 - ((fit-gf)*100.0 / bf);

  regParams->goodbad = reg::Uncertain;

  //  BAD FIT TEST
  if ( (good>2.0) && (bad<2.0) ) 
    regParams->goodbad = reg::Bad;

  if (bim_goodbad>40 && numx>4)
    regParams->goodbad = reg::Uncertain;

  // GOOD FIT TEST
  if ( (good<2.0) && (numx>2) && (bad>2.0) )
    regParams->goodbad = reg::Good;

  if (bim_goodbad>48 && numx>6)
    regParams->goodbad = reg::Good;

  // EXCELLENT FIT TEST
  if ( (good<1.0) && (bad>3.0) && (numx>2) )
    regParams->goodbad = reg::Excellent;


  //tdfShow(regParams->filtParams, "RST: Done!\0", 100);

  return Error;
}

#endif // REG_RST_REGISTRATION
