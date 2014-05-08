/*****************************************************************************
 Registration implementation

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  Undefine REG_DELETE_INPUT_IMAGES in .h if you don't want 
  input images to be deleted

 History:
   10/20/2001 13:05:33 - First creation
   11/21/2001 13:52:14 - Big image resizing
   12/13/2001 18:07:35 - Support for selected rectangular area
   01/31/2002 19:09:37 - TPointA changed to TPointList
   03/18/2002 16:15:17 - Resize of QImage, memory economy to the top!
                         Input images are deleted to economize memory
   03/21/2002 15:54:53 - added avaluateRegistration
   04/07/2002 02:10:04 - bug corrected in getTransformation
   04/16/2002 19:37:25 - bug corrected in resizing
   04/17/2002 13:27:20 - new: Translation transformation added
   05/16/2002 13:16:12 - mse goodbad additional evaluation
   07/11/2002 02:58:39 - memory leaks corrected
   28/08/2003 19:31:00 - scaleRatioSecond added
   05/09/2003 16:29:00 - image swap added
   12/11/2003 18:41:00 - degradeST4Affine

 Ver : 16
*****************************************************************************/

#include <cmath>

#include <BioImage>

#include "algebra_matrix.h"
#include "algebra_array.h"

#include "registration.h"

#include "smooth.h"
#include "getAFFINEparameters.h"
#include "getRSTparameters.h"
#include "getSTparameters.h"
#include "getPROJECTIVENSparameters.h"

#include "kenney/reg_kenney.h"


//****************************************************************************
// UTILS
//****************************************************************************

template <typename T>
void set_to_identiry(T **m, T **v) {
  m[1][1] = 1.0;
  m[1][2] = 0;
  m[1][3] = 0;
  m[2][1] = 0;
  m[2][2] = 1.0;
  m[2][3] = 0;
  m[3][1] = 0;
  m[3][2] = 0;
  m[3][3] = 1.0;

  v[1][1] = m[1][3];
  v[2][1] = m[2][3];
  v[3][1] = 0.0;
}

//****************************************************************************
// reg::Params
//****************************************************************************

reg::Params::Params() {
  this->numpoints = REG_NUM_POINTS;
  this->transformation = reg::Affine;
  this->goodbad = reg::Bad;

  this->m = m_init<float>(3, 3);
  this->v = m_init<float>(3, 1);
  set_to_identiry(this->m, this->v);
}

reg::Params::~Params() {
  m_free(this->m);
  m_free(this->v);
}

//****************************************************************************
// REGISTRATION
//****************************************************************************
int reg::register_image_pair(bim::Image *img, bim::Image *imG, reg::Params *regParams) {

  //if ( (img->depth() != 8) && (img->depth() != imG->depth()) ) return REG_ER_INPUT_IMG_INVALID; 
  if (!regParams) return REG_ER_INPUT_PARAM_INVALID;

  //*****************************************************************************
  // Start the registration
  //*****************************************************************************
  //tdfShow(regParams->filtParams, "Start\0", 3);
  set_to_identiry(regParams->m, regParams->v);

  int Error = REG_OK; 
  Error = registerImgKenney<reg::Params::image_type, reg::Params::point_type, reg::Params::work_type>(img, imG, regParams);

  // Recompute the transformation for corrected tie points
  //getTransformation(regParams);

  // now avaluate godbad with RMSE
  regParams->rmse = reg::rmsError(regParams);

  if (regParams->goodbad==reg::Bad && regParams->rmse<1.0 && regParams->tiePoints1.size()>5) 
    regParams->goodbad = reg::Uncertain;
  else
  if (regParams->goodbad==reg::Bad && regParams->rmse<2.0 && regParams->tiePoints1.size()>10) 
    regParams->goodbad = reg::Uncertain;

  return Error;
}

//****************************************************************************
// SUPPORT FUNCTIONS
//****************************************************************************

// counts RMSE
double reg::rmsError(reg::Params *regParams) {
  typedef reg::Params::work_type Tw;
  typedef reg::Params::point_type Tpoint;

  if (!regParams) return -1.0;
  if (regParams->tiePoints1.size()==0) return -1.0;
  double error=0.0;

  Tw **m = regParams->m;
  Tw **v = regParams->v;

  Tw **TempM = m_init<Tw>(3, 3);
  m_copy(m, TempM, 3, 3);
  Tw **invm = m_init<Tw>(3, 3);

  if ( regParams->transformation < reg::ProjectiveNS ) {
    pinv(TempM, 2, 2, invm);
    for (int i=0; i<regParams->tiePoints1.size(); ++i) {
      reg::Point<Tpoint> p1 = regParams->tiePoints1[i];
      reg::Point<Tpoint> p2 = regParams->tiePoints2[i];
      reg::Point<Tpoint> p2t;
      p2t.x = invm[2][1]*(p2.y-v[1][1]) + invm[2][2]*(p2.x-v[2][1]);
      p2t.y = invm[1][1]*(p2.y-v[1][1]) + invm[1][2]*(p2.x-v[2][1]);
      error = error + (sqr(p1.x-p2t.x) + sqr(p1.y-p2t.y));
    }
    error = sqrt (error / (double)(regParams->tiePoints1.size()) );
  }
  else // if the case is 3x3 matrix, we'll go by generic method
  {
    pinv(TempM, 3, 3, invm);

    for (int i=0; i<regParams->tiePoints1.size(); ++i) {
      reg::Point<Tpoint> p1 = regParams->tiePoints1[i];
      reg::Point<Tpoint> p2 = regParams->tiePoints2[i];
      reg::Point<Tpoint> p2t;
      p2t.x = (invm[2][1]*(p2.y) + invm[2][2]*(p2.x) + invm[2][3]*(1) ) / 
              (invm[3][1]*(p2.y) + invm[3][2]*(p2.x) + invm[3][3]*(1) );
      p2t.y = (invm[1][1]*(p2.y) + invm[1][2]*(p2.x) + invm[1][3]*(1) ) / 
              (invm[3][1]*(p2.y) + invm[3][2]*(p2.x) + invm[3][3]*(1) );
      error = error + (sqr(p1.x-p2t.x) + sqr(p1.y-p2t.y));
    }
    error = sqrt (error / (double)(regParams->tiePoints1.size()) );
  }

  m_free(TempM);
  m_free(invm);
  return error;
}

/*
int getTransformation(TRegParams *regParams) {
  if (regParams == NULL) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->tpList1==NULL) || (regParams->tpList2==NULL) ) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->m==NULL) || (regParams->v==NULL) ) return REG_ER_INPUT_PARAM_INVALID; 

  int Result;
  REAL **m, **v;  
  TPointA xyA, XYA;
  xyA.A=NULL; XYA.A=NULL;
  pointListToArray(regParams->tpList1, &xyA);
  pointListToArray(regParams->tpList2, &XYA);
  m = regParams->m;
  v = regParams->v;

  Result = getTransformation(regParams->regType, xyA, XYA, m, v);

  freePointArray(&xyA);
  freePointArray(&XYA);
  return Result;
}

int getInvTransformation(TRegParams *regParams)
{
  if (regParams == NULL) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->tpList1==NULL) || (regParams->tpList2==NULL) ) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->m==NULL) || (regParams->v==NULL) ) return REG_ER_INPUT_PARAM_INVALID; 

  int Result;
  REAL **m, **v;  
  TPointA xyA, XYA;
  xyA.A=NULL; XYA.A=NULL;
  pointListToArray(regParams->tpList1, &xyA);
  pointListToArray(regParams->tpList2, &XYA);
  m = regParams->m;
  v = regParams->v;

  Result = getTransformation(regParams->regType, XYA, xyA, m, v);

  freePointArray(&xyA);
  freePointArray(&XYA);
  return Result;
}

int getInvTransformation(TRegParams *regParams, REAL **m, REAL **v)
{
  if (regParams == NULL) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->tpList1==NULL) || (regParams->tpList2==NULL) ) return REG_ER_INPUT_PARAM_INVALID;

  int Result;
  TPointA xyA, XYA;
  xyA.A=NULL; XYA.A=NULL;
  pointListToArray(regParams->tpList1, &xyA);
  pointListToArray(regParams->tpList2, &XYA);

  Result = getTransformation(regParams->regType, XYA, xyA, m, v);

  freePointArray(&xyA);
  freePointArray(&XYA);
  return Result;
}

void getTparameters(TPointA *xyA, TPointA *XYA, double &dx, double &dy)
{
  double dxS=0, dyS=0;

  for (int i=0; i<xyA->length; i++)
  {
    dxS = dxS + (XYA->A[i].x - xyA->A[i].x);
    dyS = dyS + (XYA->A[i].y - xyA->A[i].y);      
  }
  //dx = lround(dxS / (double)(xyA->length-1));
  //dy = lround(dyS / (double)(xyA->length-1));
  dx = dxS / (double)(xyA->length-1);
  dy = dyS / (double)(xyA->length-1);
}

int getTransformation(TRegistrationType regType, TPointA xyA, TPointA XYA, REAL **m, REAL **v)
{
  double r, a, c, s, dx, dy;

  if (xyA.length<2) return REG_ER_NOT_ENOUGH_POINTS;
  
  if ( (regType==rtAffine) && (xyA.length>3) )
  {
    getAFFINEparameters(&xyA, &XYA, m, v);
    aff2pj(m, v);
  }
  
  if ( (regType==rtRST) || ((xyA.length<=3) && (xyA.length>=2)) )
  {
    getRSTparameters(&xyA, &XYA, &r, &c, &s, &dx, &dy);
    a = atan2(s, c)*180.0/PI;
    rst2aff(r, a, dx, dy, m, v);
    aff2pj(m, v);
  }

  if (regType==rtST)
  {
    getSTparameters(&xyA, &XYA, &r, &dx, &dy);
    rst2aff(r, 0, dx, dy, m, v);
    aff2pj(m, v);
  }

  if ( (regType==rtProjectiveNS) && (xyA.length>4) )
  {
    int i;
    double ex=0, ey=0;
    getPROJECTIVENSparameters(&xyA, &XYA, m, v);


    ex = 0; ey = 0;
    double erx, ery;
    for (i=1; i<=XYA.length; i++)
    {
      TPoint tp = initPoint( XYA.A[i].y, XYA.A[i].x );
      tp = trSens2Base( m, v, tp );
      erx = (tp.x - xyA.A[i].y);
      ery = (tp.y - xyA.A[i].x);
      ex += erx;
      ey += ery;
    }
    ex = ex / (double) XYA.length;  
    ey = ey / (double) XYA.length;
  }

  if ( (regType==rtTranslation) || (xyA.length<2) )
  {
    getTparameters(&xyA, &XYA, dx, dy);
    rst2aff(1, 0, dx, dy, m, v);
    aff2pj(m, v);
  }

  return REG_OK;
}

REAL **getInverse(REAL **m, TRegParams *regParams)
{
  REAL **invm;
  REAL **TempM;
  int s=2; 

  if (regParams->regType == rtProjectiveNS) s=3;

  TempM = mf_init(s, s);
  mf_copy(m, TempM, s, s);
  invm = mf_init(s, s);
  pinv(TempM, s, s, invm);
  mf_free(TempM);

  return invm;
}

REAL **getInverse(REAL **m)
{
  REAL **invm;
  REAL **TempM;
  int s=3; 

  TempM = mf_init(s, s);
  mf_copy(m, TempM, s, s);
  invm = mf_init(s, s);
  pinv(TempM, s, s, invm);
  mf_free(TempM);

  return invm;
}

REAL **getInverse(TRegParams *regParams)
{
  return getInverse(regParams->m, regParams);
}

int degradeST4Affine(TRegParams *regParams, REAL **md, REAL **vd)
{
  int Result=0;
  float r;
  uint i,j;
  TPointA xyA, XYA;
  REAL **m;

  
  if (regParams == NULL) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->tpList1==NULL) || (regParams->tpList2==NULL) ) return REG_ER_INPUT_PARAM_INVALID;
  if ( (regParams->m==NULL) || (regParams->v==NULL) ) return REG_ER_INPUT_PARAM_INVALID; 

  xyA.A=NULL; XYA.A=NULL;
  m = regParams->m;
  pointListToArray(regParams->tpList1, &xyA);
  pointListToArray(regParams->tpList2, &XYA);
  if (md == NULL) { md = mf_init(3, 3); for (i=1;i<=3;i++) for (j=1;j<=3;j++) md[i][j] = 0; }
  if (vd == NULL) { vd = mf_init(3, 1); for (i=1;i<=3;i++) vd[i][1] = 0; }
  
  // now get scale parameter
  r = sqrt(m[1][1]*m[1][1] + m[1][2]*m[1][2]);
  
  // now remove scale from tie points
  for (i=0; i<(uint)XYA.length; i++) {
    XYA.A[i].x = (long) ((float) XYA.A[i].x) / r;
    XYA.A[i].y = (long) ((float) XYA.A[i].y) / r;
  }
  
  // recalculate new transformation with no scale
  Result = getTransformation(regParams->regType, xyA, XYA, md, vd);
  // remove translation
  for (i=1;i<=3;i++) vd[i][1] = 0;

  freePointArray(&xyA);
  freePointArray(&XYA);
  return Result;
}


TPoint transformSensedPoint(TRegParams *regParams, uint index)
{
  TPoint tp = initPoint(-1,-1);
  if (regParams == NULL) return tp;
  if ( (regParams->tpList1==NULL) || (regParams->tpList2==NULL) ) return tp;
  if ( (regParams->m==NULL) || (regParams->v==NULL) ) return tp;
  if (index>=regParams->tpList2->count()) return tp;

  //REAL **m, **v;  
  //REAL **invm;
  //m = regParams->m;
  //v = regParams->v;
  //double I, J;
  //invm = getInverse(regParams);
  
  //I = regParams->tpList2->at(index)->y;
  //J = regParams->tpList2->at(index)->x;

  //tp.x = (long int)lround( invm[2][1]*(I-v[1][1]) + invm[2][2]*(J-v[2][1]) );
  //tp.y = (long int)lround( invm[1][1]*(I-v[1][1]) + invm[1][2]*(J-v[2][1]) );

  tp = trSens2Base(regParams->m, regParams->v, regParams, *regParams->tpList2->at(index) );

  //mf_free(invm);

  return tp;
}

/*
TPoint trSens2Uni(TRegParams *regParams, uint index)
{ 
  return initPoint(0,0);
}

TPoint trBase2Uni(TRegParams *regParams, uint index)
{ 
  return initPoint(0,0);
}

TPoint trUni2Sens(TRegParams *regParams, uint index)
{ 
  return initPoint(0,0);
}

TPoint trUni2Base(TRegParams *regParams, uint index)
{ 
  return initPoint(0,0);
}
*/

// -------------------------------------------------------------------
// | X |            | x |
// | Y | = invm * ( | y | - v )
// -------------------------------------------------------------------
/*
TPoint trSens2Base(REAL **m, REAL **v, TRegParams *regParams, TPoint p)
{
  TPoint tp = initPoint(0, 0);
  REAL **invm = getInverse(m, regParams);
  double I = p.y;
  double J = p.x;

  if (regParams->regType != rtProjectiveNS) 
  {
    tp.x = lround( invm[2][1]*(I-v[1][1]) + invm[2][2]*(J-v[2][1]) );
    tp.y = lround( invm[1][1]*(I-v[1][1]) + invm[1][2]*(J-v[2][1]) );
  }
  else 
  {
    tp.x = lround( (invm[2][1]*I + invm[2][2]*J + invm[2][3] ) / 
                   (invm[3][1]*I + invm[3][2]*J + invm[3][3] ) );

    tp.y = lround( (invm[1][1]*I + invm[1][2]*J + invm[1][3] ) /
                   (invm[3][1]*I + invm[3][2]*J + invm[3][3] ) );
  }
 
  mf_free(invm);
  return tp;  
}

TPoint trSens2Base(TRegParams *regParams, TPoint p)
{ 
  return trSens2Base(regParams->m, regParams->v, regParams, p);
}

TPoint trSens2Base(REAL **m, REAL **v, TPoint p)
{
  TPoint tp = initPoint(0, 0);
  REAL **invm = getInverse(m);
  double I = p.y;
  double J = p.x;

  tp.x = lround( (invm[2][1]*I + invm[2][2]*J + invm[2][3] ) / 
                 (invm[3][1]*I + invm[3][2]*J + invm[3][3] ) );

  tp.y = lround( (invm[1][1]*I + invm[1][2]*J + invm[1][3] ) /
                 (invm[3][1]*I + invm[3][2]*J + invm[3][3] ) );
 
  mf_free(invm);
  return tp;  
}

// -------------------------------------------------------------------
// | X |       | x |
// | Y | = m * | y | + v
// -------------------------------------------------------------------
TPoint trBase2Sens(REAL **m, REAL **v, TRegParams *regParams, TPoint p)
{
  TPoint tp = initPoint(0, 0);
  double I = p.y;
  double J = p.x;
  
  if (regParams->regType != rtProjectiveNS) 
  {
    tp.x = lround( m[2][1]*I + m[2][2]*J + v[2][1] );
    tp.y = lround( m[1][1]*I + m[1][2]*J + v[1][1] );
  }
  else
  {
    tp.x = lround( (m[2][1]*I + m[2][2]*J + m[2][3]) /
                   (m[3][1]*I + m[3][2]*J + m[3][1]) );
    tp.y = lround( (m[1][1]*I + m[1][2]*J + v[1][3]) /
                   (m[3][1]*I + m[3][2]*J + m[3][1]) );
  }
  
  return tp;
}

TPoint trBase2Sens(TRegParams *regParams, TPoint p)
{ 
  return trBase2Sens(regParams->m, regParams->v, regParams, p);
}
// -------------------------------------------------------------------

//****************************************************************************
// EVALUATION
//****************************************************************************
/*
int evaluateRegistration(TMatrix *mg, TMatrix *mG, TRegParams *regParams)
{
  int numrand=16;    // number of random pertrubations to determine good fit- bad fit
  double maxerror=60.0;
  
  int *goodbad;
  REAL **m, **v;
  double fit, mv, sv, mV, sV;
  double overlap;
  double bf, bs, gf, gs;
  double good, bad;
  int numx;

  double bim_goodbad;
 
  m = regParams->m;
  v = regParams->v;
  goodbad = &regParams->goodbad;
  
  
  //tdfShow(regParams->filtParams, "Evaluate transformation\0", 0);

  // evaluate the fit for the TPA parameter estimates
  evalfit(mg, mG, m, v, &fit, &mv, &sv, &mV, &sV, &overlap);       

  // estimate fit values corresponding to bad parameters
  // the max allowable index error is maxerror
  //tdfShow(regParams->filtParams, "Evaluate transformation\0", 40);
  maxerror=60.0;
  fiterror(mg, mG, m, v, numrand, maxerror, &bf, &bs);


  // estimate fit values corresponding to good parameters
  //tdfShow(regParams->filtParams, "Evaluate transformation\0", 80);
  maxerror=2.0;
  fiterror(mg, mg, 1.0, 0.0, 0.0, 0.0, numrand, maxerror, &gf, &gs);   

  // decide if we have a good fit or a bad fit or can't tell
  //numx = xyA->length;
  numx = regParams->tpList1->count();

  // find distance from computed fit to good fit in terms of good fit standard devs
  good = (fit-gf)/(gs+DIM_EPS);
  // find distance from computed fit to bad fit in terms of bad fit standard devs
  bad = fabs(fit-bf)/(bs+DIM_EPS);         

  bim_goodbad = 100.0 - ((fit-gf)*100.0 / bf);

  *goodbad=0;

  // GOOD FIT TEST
  //if ( (good<2.0) && (bad>2.0) )
  if ( (good<2.0) && (bad>2.0) || (bim_goodbad > 50) )
  //if ( (good<2.0) && (bad>2.0) || (good < bad) )
    if (numx>2) *goodbad=1;

  // EXCELLENT FIT TEST
  if ( (good<1.0) && (bad>3.0) )
    if (numx>2) *goodbad=2;

  //  BAD FIT TEST
  if ( (good>2.0) && (bad<2.0) ) *goodbad=-1;

  //tdfShow(regParams->filtParams, "Evaluation complete!\0", 100);

  return REG_OK;
}

int evaluateRegistration(QImage *img, QImage *imG, TRegParams *regParams)
{
  TMatrix rbmp1, rbmp2;  
  
  if ( (img->depth() != 8) && (img->depth() != imG->depth()) ) return REG_ER_INPUT_PARAM_INVALID; 
  if ( (regParams == NULL) || (regParams == 0) ) return REG_ER_INPUT_PARAM_INVALID;

  qtImageToRealM(img, &rbmp1, NULL);
  qtImageToRealM(imG, &rbmp2, NULL);
  
  double rmse = rmsError(regParams);

  avaluateRegistration(&rbmp1, &rbmp2, regParams);
  if ( (regParams->goodbad == REG_BAD) && (regParams->tpList1->count() >= 3) &&
       (rmse <= 2)
     )
     regParams->goodbad = REG_UNCERTAIN;

  freeRealMatrix(&rbmp1);
  freeRealMatrix(&rbmp2);

  return REG_OK;
}

void optimizedRANSAC(QImage *img, QImage *imG, TRegParams *regParams)
{
  TPointA xyA, XYA;
  TPointA txyA, tXYA;
  TMatrix mg, mG;
  double c=0, s=0;
  double r, dx, dy;
  uint i;

  if ( (img->depth() != 8) && (img->depth() != imG->depth()) ) return; 
  if ( (regParams == NULL) || (regParams == 0) ) return;

  initPointArray(&xyA, regParams->tpList1->count());
  initPointArray(&XYA, regParams->tpList2->count());
  initPointArray(&txyA, regParams->tpList1->count());
  initPointArray(&tXYA, regParams->tpList2->count());

  for (i=0; i<regParams->tpList1->count(); i++)
  {
    xyA.A[i+1].x = regParams->tpList1->at(i)->x; 
    xyA.A[i+1].y = regParams->tpList1->at(i)->y;
    XYA.A[i+1].x = regParams->tpList2->at(i)->x;
    XYA.A[i+1].y = regParams->tpList2->at(i)->y;
  }

  qtImageToRealM(img, &mg, NULL);
  qtImageToRealM(imG, &mG, NULL);


  #ifdef DIM_REG_KENNEY
  geometry( &mg, &mG, &xyA, &XYA, &r, &c, &s, &dx, &dy, &txyA, &tXYA);
  #endif

  // remove the matrixes from the memory
  freeRealMatrix(&mg);
  freeRealMatrix(&mG);

  pointArrayToList(&txyA, regParams->tpList1);
  pointArrayToList(&tXYA, regParams->tpList2);

  freePointArray(&xyA);
  freePointArray(&XYA);
  freePointArray(&txyA);
  freePointArray(&tXYA);
}
*/
