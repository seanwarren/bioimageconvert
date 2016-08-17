/*****************************************************************************
 Registration defs

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 OUTPUT:
  m, v - pointers to matrix, m-2x2, c-2x1 SHOULD BE INITIALIZED!!!
  xyA, XYA - pointers to TPointA, PASS INITED STRUCTURE but DO NOT INIT
             ARRAYS!!!!
  goodbad - pointer to int

  Undefine REG_DELETE_INPUT_IMAGES if you want input images not to be deleted

 History:
   10/20/2001 13:05:33 - First creation
   01/31/2002 19:09:37 - TRegParams changed, it receives TPointList now
   03/18/2002 16:15:17 - Resize of QImage, memory economy to the top!
                         Input images are deleted to economize memory
   03/21/2002 15:54:53 - added avaluateRegistration
   04/17/2002 13:27:20 - new: Translation transformation added
   28/08/2003 19:31:00 - scaleRatioSecond added 
   05/09/2003 16:29:00 - image swap added
   12/11/2003 18:41:00 - degradeST4Affine

 Ver : 11
*****************************************************************************/

#ifndef IMAGE_REGISTRATION_H
#define IMAGE_REGISTRATION_H

#include <cstdlib>

#include <string>
#include <list>
#include <deque>
#include <sstream>
#include <iostream>
#include <fstream>

#include <BioImageCore>
#include <BioImage>

#include "points.h"
#include "algebra_matrix.h"

#define REG_VERSION "0.1.23"

// define this to delete the input images inside the function, memory economy
#define REG_DELETE_INPUT_IMAGES 

// values of registration quality - points
#define REG_Q_EXTRA_FAST   64
#define REG_Q_FAST         80
#define REG_Q_NORMAL       128
#define REG_Q_GOOD_QUALITY 255
#define REG_Q_HIGH_QUALITY 360
#define REG_Q_EXAGGERATED  512

// default values
#define REG_MAX_SIZE 1024 // max image size; if g and G are bigger then they are subsampled
#define REG_NUM_POINTS REG_Q_NORMAL

// ERRORS
#define REG_OK 0
#define REG_ER_INPUT_IMG_INVALID 1
#define REG_ER_INPUT_PARAM_INVALID 2
#define REG_ER_NOT_ENOUGH_MEMORY 3
#define REG_ER_NOT_ENOUGH_POINTS 4
#define REG_ER_INTERRUPTED_BY_USER 5
#define REG_ER_DURING_PROCESS 6

namespace reg {

//****************************************************************************
// Params
//****************************************************************************

enum Transformation { RST, Affine, Translation, ST, ProjectiveNS  };
enum ResultQuality { Bad=-1, Uncertain=0, Good=1, Excellent=2 };

class Params {
public:
  typedef float image_type;
  typedef float point_type;
  typedef float work_type;

public:
    Params();
    ~Params();

public:
  //*** IN
  int numpoints; // number of points to start with
  //int maxsize; // maximum image size, larger are downsampled
  Transformation transformation;

  //*** IN/OUT (initialized parameters)
  work_type **m; // transformation matrix (scale/rotation) 2x2 - affine, 3x3 - projective
  work_type **v; // transformation matrix (translation) 2x1
  
  // reg uses this lists to return tie points, other functions might
  // use to get points and recalculate transformation
  std::deque< Point<point_type> > tiePoints1;
  std::deque< Point<point_type> > tiePoints2;

  //*** OUT
  ResultQuality goodbad; // output registration quality
  double rmse;
};

// Main driver
int register_image_pair(bim::Image *img, bim::Image *imG, Params *regParams);


//****************************************************************************
// Matrix
//****************************************************************************

template <typename T>
class Matrix {
public:
  int cols;
  int rows;
  T **data; // pointer to data matrix like: int**, double**...

public:
  Matrix(): cols(0), rows(0), data(0)   {}
  Matrix(int rows, int cols): cols(0), rows(0), data(0)   { init(rows, cols); }
  Matrix(const bim::Image *img): cols(0), rows(0), data(0) { fromImage(img); }
  Matrix(const Matrix<T> *m): cols(0), rows(0), data(0)   { fromMatrix(m); }
  ~Matrix() { free(); }

public:
  bool init(int rows, int cols);
  bool free();

  bool      fromMatrix(const Matrix<T> *m);
  bool      fromImage(const bim::Image *img);
  bim::Image toImage();
  bool      toFile(const std::string &filename, const std::string &format);
};

//****************************************************************************
// reg::Matrix
//****************************************************************************

template <typename T>
bool reg::Matrix<T>::init(int rows, int cols) {
  // return pointer to array of pointers to rows
  if (this->cols==cols && this->rows==rows) return true;
  this->free();
  this->data = m_init<T>(rows, cols);
  this->cols = cols;
  this->rows = rows;
  return true;
}

template <typename T>
bool reg::Matrix<T>::free() {
  if (!this->data) return false;
  m_free(this->data);
  this->data = 0;
  this->rows = 0;
  this->cols = 0;
  return true;
}

template <typename T>
bool reg::Matrix<T>::fromMatrix(const Matrix<T> *m) {
  this->init(m->rows, m->cols);
  m_copy(m->data, this->data, m->rows, m->cols);
  return true;
}

template <typename Tm, typename Ti>
void copyImageToMatrix(const bim::Image *img, const double &minpv, const double &maxpv, reg::Matrix<Tm> *m) {
  Tm **g = m->data;
  double range = maxpv-minpv;
  for (unsigned int i=1; i<=img->height(); i++) {
    Ti *p = (Ti *) img->scanLine( 0, i-1 );
    for (unsigned int j=1; j<=img->width(); j++) {
      Ti v = *(p+(j-1));
      g[i][j] = (Tm)((v - minpv) / range);
    }
  }
}

template <typename T>
bool reg::Matrix<T>::fromImage(const bim::Image *img) {
  this->init( (int) img->height(), (int) img->width() );

  bim::ImageHistogram h(*img);
  double minpv = h[0]->min_value();
  double maxpv = h[0]->max_value(); 

  if (img->depth()==8 && img->pixelType()==bim::FMT_UNSIGNED)
    copyImageToMatrix<T, bim::uint8>( img, minpv, maxpv, this );
  else
  if (img->depth()==16 && img->pixelType()==bim::FMT_UNSIGNED)
    copyImageToMatrix<T, bim::uint16>( img, minpv, maxpv, this );
  else
  if (img->depth()==32 && img->pixelType()==bim::FMT_UNSIGNED)
    copyImageToMatrix<T, bim::uint32>( img, minpv, maxpv, this );
  else
  if (img->depth()==8 && img->pixelType()==bim::FMT_SIGNED)
    copyImageToMatrix<T, bim::int8>( img, minpv, maxpv, this );
  else
  if (img->depth()==16 && img->pixelType()==bim::FMT_SIGNED)
    copyImageToMatrix<T, bim::int16>( img, minpv, maxpv, this );
  else
  if (img->depth()==32 && img->pixelType()==bim::FMT_SIGNED)
    copyImageToMatrix<T, bim::int32>( img, minpv, maxpv, this );
  else
  if (img->depth()==32 && img->pixelType()==bim::FMT_FLOAT)
    copyImageToMatrix<T, bim::float32>( img, minpv, maxpv, this );
  else
  if (img->depth()==64 && img->pixelType()==bim::FMT_FLOAT)
    copyImageToMatrix<T, bim::float64>( img, minpv, maxpv, this );

  return 0;
}

template <typename T>
bim::Image reg::Matrix<T>::toImage() {
  bim::Image img(this->cols, this->rows, 8, 1);
  T **g = this->data;
  T minval = m_min(g, this->rows, this->cols);
  T maxval = m_max(g, this->rows, this->cols);
  double range_mult = 255.0 /(maxval - minval);
  for (unsigned int i=1; i<=img.height(); i++) {
    unsigned char *p = (unsigned char *) img.scanLine( 0, i-1 );
    for (unsigned int j=1; j<=img.width(); j++) {
      *(p+(j-1)) = (unsigned char)(g[i][j]*range_mult);
    }
  }
  return img;
}

template <typename T>
bool reg::Matrix<T>::toFile(const std::string &filename, const std::string &format) {
  bim::Image img = this->toImage();
  return img.toFile(filename, format);
}

//****************************************************************************
// Functions
//****************************************************************************

// error counted using tie point lists and transformation passed in params
double rmsError(reg::Params *regParams); // RMSE

// This functions recalculate transformation using tie point lists 
//int getTransformation(Params *regParams);
//int getInvTransformation(Params *regParams);
//int getInvTransformation(Params *regParams, REAL **m, REAL **v);
//int getTransformation(Transformation regType, TPointA xyA, TPointA XYA, REAL **m, REAL **v);

//REAL **getInverse(REAL **m);
//REAL **getInverse(TRegParams *regParams);
//REAL **getInverse(REAL **m);

// degrade scale and translation for affine transformation
//int degradeST4Affine(Params *regParams, REAL **md, REAL **vd);

// returns transformed point from senced image
/*
Point transformSensedPoint(Params *regParams, unsigned int index);

TPoint trSens2Uni(TRegParams *regParams, uint index);
TPoint trBase2Uni(TRegParams *regParams, uint index);
TPoint trUni2Sens(TRegParams *regParams, uint index);
TPoint trUni2Base(TRegParams *regParams, uint index);

Point trSens2Base(REAL **m, REAL **v, Params *regParams, const Point &p);
Point trBase2Sens(REAL **m, REAL **v, Params *regParams, const Point &p);
Point trSens2Base(REAL **m, REAL **v, const Point &p);
Point trSens2Base(Params *regParams, const Point &p);
Point trBase2Sens(Params *regParams, const Point &p);
*/

// evaluate Registration
//IN: gray scale images and m and v matrixes
//OUT: goodbad parameter
/*
int evaluateRegistration(QImage *img, QImage *imG, Params *regParams);


void optimizedRANSAC(QImage *img, QImage *imG, Params *regParams);

void geometry(TMatrix *mg, TMatrix *MG, TPointA *xyA, TPointA *XYA, 
              double *r, double *c, double *s, double *dx, double *dy,
              TPointA *xysA, TPointA *XYsA);


// transformations
void getSTparameters(TPointA *xyA, TPointA *XYA, 
                      double *r, double *dx, double *dy);

void getRSTparameters(TPointA *xyA, TPointA *XYA, 
                      double *r, double *c, double *s, double *dx, double *dy);

void getAFFINEparameters(TPointA *xyA, TPointA *XYA, REAL **m, REAL **vo);

void getPROJECTIVENSparameters(TPointA *xyA, TPointA *XYA, REAL **m, REAL **vo);
*/


//****************************************************************************
// UTILS
//****************************************************************************
template <typename T>
void rst2aff(double r, double a, double dx, double dy, T **m, T **v) {
  T c, s;

  v[1][1] = (T) dx;
  v[2][1] = (T) dy;

  c = (T) cos(a*REG_PI/180.0);
  s = (T) sin(a*REG_PI/180.0);

  m[1][1] = (T) r*c;
  m[1][2] = (T) r*s;
  m[2][1] = (T) -r*s;
  m[2][2] = (T) r*c;
}

template <typename T>
void aff2rst(T **m, T **v, double& r, double& a, double& dx, double& dy) {
  dx = v[1][1];
  dy = v[2][1];

  a = atan(m[1][2]/m[1][1]) * (180.0/REG_PI);
  r = sqrt(m[1][1]*m[1][1] + m[1][2]*m[1][2]);
}

template <typename T>
void aff2pj(T **m, T **v) {
  v[3][1] = 0;

  m[3][1] = 0.0;
  m[3][2] = 0.0;

  m[1][3] = v[1][1];
  m[2][3] = v[2][1];
  m[3][3] = 1.0;
}



} // namespace reg

#endif // IMAGE_REGISTRATION_H


