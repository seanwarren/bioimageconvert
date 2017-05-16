/*******************************************************************************

  Interpolation filters and application functions, templated and parallelized
  This code is inspired by resize code from imagemagick library

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2007-07-06 17:02 - First creation
    2013-06-15 14:40 - Parallel implementation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_RESIZE_H
#define BIM_RESIZE_H

#include <string>
#include <iostream>
#include <fstream>
#include <limits>
#include <vector>

#include "xtypes.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

//------------------------------------------------------------------------------------
// Filters implemented
//------------------------------------------------------------------------------------

// base class for interpolation filters
template<typename T> class DInterpolationFilter;

// actual filters
template<typename T> class DPointFilter;
template<typename T> class DBoxFilter;
template<typename T> class DTriangleFilter;
template<typename T> class DHermiteFilter;
template<typename T> class DHanningFilter;
template<typename T> class DHammingFilter;
template<typename T> class DBlackmanFilter;
template<typename T> class DGaussianFilter;
template<typename T> class DCubicFilter;
template<typename T> class DQuadraticFilter;
template<typename T> class DCatromFilter;
template<typename T> class DMitchellFilter;
template<typename T> class DLanczosFilter;
template<typename T> class DBesselFilter;
template<typename T> class DSincFilter;
template<typename T> class DBlackmanBesselFilter;
template<typename T> class DBlackmanSincFilter;

//************************************************************************************
// Vector resize functions
//************************************************************************************

// nearest neighbor ------------------------------------------------------------------
template <typename T>
std::vector<T> vector_resample_NN ( const std::vector<T> &src, unsigned int new_size );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
std::vector<T> vector_resample_NN ( const std::vector<T> &src, unsigned int new_size );

// Bilinear ---------------------------------------------------------------------------
template <typename T>
std::vector<T> vector_resample_BL ( const std::vector<T> &src, unsigned int new_size );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
std::vector<T> vector_resample_BL ( const std::vector<T> &src, unsigned int new_size );

// Bicubic ----------------------------------------------------------------------------
template <typename T>
std::vector<T> vector_resample_BC ( const std::vector<T> &src, unsigned int new_size );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
std::vector<T> vector_resample_BC ( const std::vector<T> &src, unsigned int new_size );

//------------------------------------------------------------------------------------
// Generic resize function, 
// Types:
//   o Td is the pixel data type (uchar, short, ...)
//   o Tw is the working type, suggested values are float or double
// Params:
//   o filter: filter to use
//   o blur: The blur factor where > 1 is blurry, < 1 is sharp
//------------------------------------------------------------------------------------

template <typename Td, typename Tw>
std::vector<Td> ResizeVector( const std::vector<Td> &src, unsigned int new_size,
                              DInterpolationFilter<Tw> &filter, const Tw blur );



//************************************************************************************
// Simple Image Resize functions, templated type is the pixel data type
//************************************************************************************

// nearest neighbor ------------------------------------------------------------------
template <typename T>
void image_resample_NN ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
void image_resample_BL ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                         const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

// Bilinear --------------------------------------------------------------------------
template <typename T>
void image_resample_BL ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
void image_resample_BL ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

// Bicubic ---------------------------------------------------------------------------
template <typename T>
void image_resample_BC ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

// templating Tw - work type, potentially giving higher quality or faster speed
template <typename T, typename Tw>
void image_resample_BC ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );


//------------------------------------------------------------------------------------
// Generic resize function, 
// Types:
//   o Td is the pixel data type (uchar, short, ...)
//   o Tw is the working type, suggested values are float or double
// Params:
//   o filter: filter to use
//   o blur: The blur factor where > 1 is blurry, < 1 is sharp
//------------------------------------------------------------------------------------
template <typename Td, typename Tw>
void ResizeImage( Td *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                         const Td *psrc, unsigned int w_in, unsigned int h_in, unsigned int offset_in, 
                         DInterpolationFilter<Tw> &filter, const Tw blur );







//************************************************************************************
// IMPLEMENTATION PART
//************************************************************************************

//------------------------------------------------------------------------------------
// BesselOrderOne from ImageMagick
//------------------------------------------------------------------------------------
/*
%  BesselOrderOne() computes the Bessel function of x of the first kind of
%  order 0:
%
%    Reduce x to |x| since j1(x)= -j1(-x), and for x in (0,8]
%
%       j1(x) = x*j1(x);
%
%    For x in (8,inf)
%
%       j1(x) = sqrt(2/(pi*x))*(p1(x)*cos(x1)-q1(x)*sin(x1))
%
%    where x1 = x-3*pi/4. Compute sin(x1) and cos(x1) as follow:
%
%       cos(x1) =  cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
%               =  1/sqrt(2) * (sin(x) - cos(x))
%       sin(x1) =  sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
%               = -1/sqrt(2) * (sin(x) + cos(x))
%
%  The format of the BesselOrderOne method is:
%
%      MagickRealType BesselOrderOne(MagickRealType x)
%
%  A description of each parameter follows:
%
%    o value: Method BesselOrderOne returns the Bessel function of x of the
%      first kind of orders 1.
%
%    o x: MagickRealType value.
*/

template<typename T> 
inline T J1(const T &x) {
  T p, q;

  register long i;

  static const double
    Pone[] =
    {
       0.581199354001606143928050809e+21,
      -0.6672106568924916298020941484e+20,
       0.2316433580634002297931815435e+19,
      -0.3588817569910106050743641413e+17,
       0.2908795263834775409737601689e+15,
      -0.1322983480332126453125473247e+13,
       0.3413234182301700539091292655e+10,
      -0.4695753530642995859767162166e+7,
       0.270112271089232341485679099e+4
    },
    Qone[] =
    {
      0.11623987080032122878585294e+22,
      0.1185770712190320999837113348e+20,
      0.6092061398917521746105196863e+17,
      0.2081661221307607351240184229e+15,
      0.5243710262167649715406728642e+12,
      0.1013863514358673989967045588e+10,
      0.1501793594998585505921097578e+7,
      0.1606931573481487801970916749e+4,
      0.1e+1
    };

  p=Pone[8];
  q=Qone[8];
  for (i=7; i >= 0; --i) {
    p=p*x*x+Pone[i];
    q=q*x*x+Qone[i];
  }
  return p/q;
}

template<typename T> 
inline T P1(const T &x) {
  T p, q;
  register long i;
  static const double
    Pone[] =
    {
      0.352246649133679798341724373e+5,
      0.62758845247161281269005675e+5,
      0.313539631109159574238669888e+5,
      0.49854832060594338434500455e+4,
      0.2111529182853962382105718e+3,
      0.12571716929145341558495e+1
    },
    Qone[] =
    {
      0.352246649133679798068390431e+5,
      0.626943469593560511888833731e+5,
      0.312404063819041039923015703e+5,
      0.4930396490181088979386097e+4,
      0.2030775189134759322293574e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; --i) {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return p/q;
}

template<typename T> 
inline T Q1(const T &x) {
  T p, q;
  register long i;
  static const double
    Pone[] =
    {
      0.3511751914303552822533318e+3,
      0.7210391804904475039280863e+3,
      0.4259873011654442389886993e+3,
      0.831898957673850827325226e+2,
      0.45681716295512267064405e+1,
      0.3532840052740123642735e-1
    },
    Qone[] =
    {
      0.74917374171809127714519505e+4,
      0.154141773392650970499848051e+5,
      0.91522317015169922705904727e+4,
      0.18111867005523513506724158e+4,
      0.1038187585462133728776636e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; --i) {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return p/q;
}

template<typename T> 
inline T BesselOrderOne(const T &xx) {
  T p,q;

  if (xx == 0.0) return(0.0);
  T x=xx;
  p=x;
  if (x < 0.0) x=(-x);
  if (x < 8.0) return(p*J1<T>(x));

  q=sqrt((double) (2.0/(bim::Pi*x)))*(P1<T>(x)*(1.0/sqrt(2.0)*(sin((double) x)-
    cos((double) x)))-8.0/x*Q1<T>(x)*(-1.0/sqrt(2.0)*(sin((double) x)+
    cos((double) x))));
  if (p < 0.0) q=-q;
  return q;
}




//------------------------------------------------------------------------------------
// filters - definition
//------------------------------------------------------------------------------------

template<typename T> 
class DFilterContribution {
public:
  DFilterContribution() { weight = (T)0.0f; pixel=0; }
  DFilterContribution( const DFilterContribution &c ) { weight=c.weight; pixel=c.pixel; }
public:
  T weight;
  int pixel;
};

template<typename T> 
class DInterpolationFilter {
public:
  DInterpolationFilter() { }
  virtual inline T function(const T &x) const { return x; }
  inline const T& support() const { return _support; }
  void setSupport( const T &s ) {
    _support = s;
    contribution.resize( (unsigned int) ceil(2.0*bim::max<T>(_support,0.5)+5) );
  }

public:
  std::vector<DFilterContribution<T> > contribution;

protected:
  T _support;
};

//------------------------------------------------------------------------------------
// filters - implementation
//------------------------------------------------------------------------------------

template<typename T> 
class DBesselFilter: public DInterpolationFilter<T> {
public:
  DBesselFilter() { this->setSupport((T)0.0); }

  inline T function(const T &x) const {
    if (x == 0.0)
      return((T) (bim::Pi/4.0));
    return (T)(BesselOrderOne(bim::Pi*x)/(2.0*x));  
  }
};

template<typename T> 
class DSincFilter: public DInterpolationFilter<T> {
public:
  DSincFilter() { this->setSupport((T)0.0); }

  inline T function(const T &x) const {
    if (x == 0.0) return 1.0;
    return (T)(sin(bim::Pi*(double) x)/(bim::Pi*(double) x));
  }
};

template<typename T> 
class DBlackmanFilter: public DInterpolationFilter<T> {
public:
  DBlackmanFilter() { this->setSupport((T)1.0); }

  inline T function(const T &x) const {
    return (T)(0.42+0.5*cos(bim::Pi*(double) x)+0.08*cos(2.0*bim::Pi*(double) x));
  }
};

template<typename T> 
class DBlackmanBesselFilter: public DInterpolationFilter<T> {
public:
  DBlackmanBesselFilter() { this->setSupport((T)3.2383); }

  inline T function(const T &x) const {
    T v = (T)( (x/this->support())*Bessel.function(x) );
    return Blackman.function(v);
  }
protected:
  DBlackmanFilter<T> Blackman;
  DBesselFilter<T> Bessel;
};

template<typename T> 
class DBlackmanSincFilter: public DInterpolationFilter<T> {
public:
  DBlackmanSincFilter() { this->setSupport((T)4.0); }

  inline T function(const T &x) const {
    return (T)(Blackman.function(x/this->support())*Sinc.function(x));
  }
protected:
  DBlackmanFilter<T> Blackman;
  DSincFilter<T> Sinc;
};

template<typename T> 
class DBoxFilter: public DInterpolationFilter<T> {
public:
  DBoxFilter() { this->setSupport((T)0.5); }

  inline T function(const T &x) const {
    if (x < -0.5) return (T)(0.0);
    if (x < 0.5)  return (T)(1.0);
    return (T)(0.0);
  }
};

template<typename T> 
class DPointFilter: public DBoxFilter<T> {
public:
  DPointFilter() { this->setSupport((T)0.0); }
};

template<typename T> 
class DCatromFilter: public DInterpolationFilter<T> {
public:
  DCatromFilter() { this->setSupport((T)2.0); }

  inline T function(const T &x) const {
    if (x < -2.0) return (T)(0.0);
    if (x < -1.0) return (T)(0.5*(4.0+x*(8.0+x*(5.0+x))));
    if (x < 0.0)  return (T)(0.5*(2.0+x*x*(-5.0-3.0*x)));
    if (x < 1.0)  return (T)(0.5*(2.0+x*x*(-5.0+3.0*x)));
    if (x < 2.0)  return (T)(0.5*(4.0+x*(-8.0+x*(5.0-x))));
    return(0.0);
  }
};

template<typename T> 
class DCubicFilter: public DInterpolationFilter<T> {
public:
  DCubicFilter() { this->setSupport((T)2.0); }

  inline T function(const T &x) const {
    if (x < -2.0) return (T)(0.0);
    if (x < -1.0) return (T)((2.0+x)*(2.0+x)*(2.0+x)/6.0);
    if (x < 0.0)  return (T)((4.0+x*x*(-6.0-3.0*x))/6.0);
    if (x < 1.0)  return (T)((4.0+x*x*(-6.0+3.0*x))/6.0);
    if (x < 2.0)  return (T)((2.0-x)*(2.0-x)*(2.0-x)/6.0);
    return(0.0);
  }
};

template<typename T> 
class DGaussianFilter: public DInterpolationFilter<T> {
public:
  DGaussianFilter() { this->setSupport((T)1.25); }

  inline T function(const T &x) const {
    return (T)( exp((double) (-2.0*x*x))*sqrt(2.0/bim::Pi) );
  }
};

template<typename T> 
class DHanningFilter: public DInterpolationFilter<T> {
public:
  DHanningFilter() { this->setSupport((T)1.0); }

  inline T function(const T &x) const {
    return (T)(0.5+0.5*cos(bim::Pi*(double) x));
  }
};

template<typename T> 
class DHammingFilter: public DInterpolationFilter<T> {
public:
  DHammingFilter() { this->setSupport((T)1.0); }

  inline T function(const T &x) const {
    return (T)(0.54+0.46*cos(bim::Pi*(double) x));
  }
};

template<typename T> 
class DHermiteFilter: public DInterpolationFilter<T> {
public:
  DHermiteFilter() { this->setSupport((T)1.0); }

  inline T function(const T &x) const {
    if (x < -1.0) return (T)(0.0);
    if (x < 0.0)  return (T)((2.0*(-x)-3.0)*(-x)*(-x)+1.0);
    if (x < 1.0)  return (T)((2.0*x-3.0)*x*x+1.0);
    return (T)(0.0);
  }
};

template<typename T> 
class DLanczosFilter: public DInterpolationFilter<T> {
public:
  DLanczosFilter() { this->setSupport((T)3.0); }

  inline T function(const T &x) const {
    if (x < -3.0) return (T)(0.0);
    if (x < 0.0)  return (T)(Sinc.function(-x)*Sinc.function((T)(-x/3.0)));
    if (x < 3.0)  return (T)(Sinc.function(x)*Sinc.function((T)(x/3.0)));
    return (T)(0.0);
  }
protected:
  DSincFilter<T> Sinc;
};

template<typename T> 
class DMitchellFilter: public DInterpolationFilter<T> {
public:
#define B   (1.0/3.0)
#define C   (1.0/3.0)
#define P0  ((  6.0- 2.0*B       )/6.0)
#define P2  ((-18.0+12.0*B+ 6.0*C)/6.0)
#define P3  (( 12.0- 9.0*B- 6.0*C)/6.0)
#define Q0  ((       8.0*B+24.0*C)/6.0)
#define Q1  ((     -12.0*B-48.0*C)/6.0)
#define Q2  ((       6.0*B+30.0*C)/6.0)
#define Q3  ((     - 1.0*B- 6.0*C)/6.0)

  DMitchellFilter() { this->setSupport((T)2.0); }

  inline T function(const T &x) const {
    if (x < -2.0) return (T)(0.0);
    if (x < -1.0) return (T)(Q0-x*(Q1-x*(Q2-x*Q3)));
    if (x < 0.0)  return (T)(P0+x*x*(P2-x*P3));
    if (x < 1.0)  return (T)(P0+x*x*(P2+x*P3));
    if (x < 2.0)  return (T)(Q0+x*(Q1+x*(Q2+x*Q3)));
    return (T)(0.0);
  }
};

template<typename T> 
class DQuadraticFilter: public DInterpolationFilter<T> {
public:
  DQuadraticFilter() { this->setSupport((T)1.5); }

  inline T function(const T &x) const {
    if (x < -1.5) return (T)(0.0);
    if (x < -0.5) return (T)(0.5*(x+1.5)*(x+1.5));
    if (x < 0.5)  return (T)(0.75-x*x);
    if (x < 1.5)  return (T)(0.5*(x-1.5)*(x-1.5));
    return (T)(0.0);
  }
};

template<typename T> 
class DTriangleFilter: public DInterpolationFilter<T> {
public:
  DTriangleFilter() { this->setSupport((T)1.0); }

  inline T function(const T &x) const {
    if (x < -1.0) return (T)(0.0);
    if (x < 0.0)  return (T)(1.0+x);
    if (x < 1.0)  return (T)(1.0-x);
    return (T)(0.0);
  }
};

//------------------------------------------------------------------------------------
// filter application
//------------------------------------------------------------------------------------

template <typename Td, typename Tw>
static void HorizontalFilter( const Td *psrc, unsigned int w_in, unsigned int /*h_in*/, unsigned int offset_in, 
                              Td *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                              DInterpolationFilter<Tw> &filter_in, const Tw blur )
{
    Td TdMin = bim::lowest<Td>();
  Td TdMax = std::numeric_limits<Td>::max();
  Tw TwEps = std::numeric_limits<Tw>::epsilon();
  Tw x_factor = (Tw) w_to / (Tw) w_in;

  //Apply filter to resize horizontally from source to destination
  Tw scale = (Tw)( blur * bim::max<double>( 1.0/x_factor, 1.0 ) );
  Tw support = scale * filter_in.support();

  if (support <= 0.5) {
    // Reduce to point sampling
    support = (Tw) (0.5+TwEps);
    scale = 1.0;
  }
  scale = (Tw)(1.0/scale);
  if (support*2+2 > filter_in.contribution.size()) filter_in.contribution.resize( (unsigned int)(support*2+2) );
  
  #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (w_to>BIM_OMP_FOR2)
  for (bim::int64 x=0; x<w_to; ++x) {
    DInterpolationFilter<Tw> filter = filter_in;
    Tw center  = (Tw)  (x+0.5)/x_factor;
    Tw density = 0.0;
    unsigned int start = (int) (bim::max<double>(center-support,0.0) + 0.5);
    unsigned int stop  = (int) (bim::min<double>(center+support,w_in+0.5));
    unsigned int n;
    for (n=0; n<(stop-start); ++n) {
      filter.contribution[n].pixel=start+n;
      filter.contribution[n].weight = filter_in.function( (Tw)(scale * ((start+n)-center+0.5)) );
      density += filter.contribution[n].weight;
    }
    
    if ((density != 0.0) && (density != 1.0)) {
      // normalize
      density = (Tw)(1.0/density);
      for (unsigned int i=0; i<n; ++i)
        filter.contribution[i].weight *= density;
    }

    const Td *p = psrc + filter.contribution[0].pixel;
    Td *q = pdest + x;
    for (unsigned int y=0; y<h_to; ++y) {
      Tw pixel = 0;
      unsigned int yl = y*offset_in;
      for (unsigned int i=0; i<n; ++i) {
        unsigned int j = yl + (filter.contribution[i].pixel - filter.contribution[0].pixel);
        Tw alpha = filter.contribution[i].weight;
        pixel += alpha*p[j];
      } // for i to n

      // we have to round if data is integer and work is float
      if (std::numeric_limits<Td>::is_integer && !std::numeric_limits<Tw>::is_integer)
        *q = bim::trim<Td, Tw>( bim::round<Tw>(pixel), TdMin, TdMax );
      else
        *q = bim::trim<Td, Tw>( pixel, TdMin, TdMax );

      q += offset_to;
    } // for y
  } // for x
}

template <typename Td, typename Tw>
static void VerticalFilter( const Td *psrc, unsigned int /*w_in*/, unsigned int h_in, unsigned int offset_in,  
                            Td *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                            DInterpolationFilter<Tw> &filter_in, const Tw blur )
{
    Td TdMin = bim::lowest<Td>();
  Td TdMax = std::numeric_limits<Td>::max();
  Tw TwEps = std::numeric_limits<Tw>::epsilon();
  Tw y_factor = (Tw) h_to / (Tw) h_in;

  // Apply filter to resize vertically from source to destination
  Tw scale = (Tw)(blur * bim::max<double>( 1.0/y_factor, 1.0 ) );
  Tw support = scale * filter_in.support();

  if (support <= 0.5) {
    // Reduce to point sampling
    support = (Tw) (0.5+TwEps);
    scale = 1.0;
  }
  scale = (Tw)(1.0/scale);
  if (support*2+2 > filter_in.contribution.size()) filter_in.contribution.resize( (unsigned int)(support*2+2) );

  #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (h_to>BIM_OMP_FOR2)
  for (bim::int64 y=0; y<h_to; ++y) {
    DInterpolationFilter<Tw> filter = filter_in;
    Tw center  = (Tw)  (y+0.5)/y_factor;
    Tw density = 0.0;
    unsigned int start = (int) (bim::max<double>(center-support,0.0) + 0.5);
    unsigned int stop  = (int) (bim::min<double>(center+support, h_in+0.5));
    unsigned int n;
    for (n=0; n<(stop-start); ++n) {
      filter.contribution[n].pixel=start+n;
      filter.contribution[n].weight = filter_in.function( (Tw)(scale * ((start+n)-center+0.5)) );
      density += filter.contribution[n].weight;
    }
    
    if ((density != 0.0) && (density != 1.0)) {
      // normalize
      density = (Tw)(1.0/density);
      for (unsigned int i=0; i<n; ++i)
        filter.contribution[i].weight *= density;
    }

    const Td *p = psrc + (offset_to * (filter.contribution[0].pixel));
    Td *q = pdest + (offset_to * y);
    for (unsigned int x=0; x<w_to; ++x) {
      Tw pixel = 0;
      for (unsigned int i=0; i<n; ++i) {
        unsigned int j = ((filter.contribution[i].pixel-filter.contribution[0].pixel)*offset_in + x);
        Tw alpha = filter.contribution[i].weight;
        pixel += alpha*p[j];
      } // for i to n
      
      // we have to round if data is integer and work is float
      if (std::numeric_limits<Td>::is_integer && !std::numeric_limits<Tw>::is_integer)
        *q = bim::trim<Td, Tw>( bim::round<Tw>(pixel), TdMin, TdMax );
      else
        *q = bim::trim<Td, Tw>( pixel, TdMin, TdMax );

      q++;
    } // for x
  } // for y
}

//------------------------------------------------------------------------------------
//  o filter: filter to use
//  o blur: The blur factor where > 1 is blurry, < 1 is sharp
//------------------------------------------------------------------------------------
template <typename Td, typename Tw>
void ResizeImage( Td *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                         const Td *psrc, unsigned int w_in, unsigned int h_in, unsigned int offset_in, 
                         DInterpolationFilter<Tw> &filter, const Tw blur )
{
  Tw x_factor = (Tw) w_to /(Tw) w_in;
  Tw y_factor = (Tw) h_to /(Tw) h_in;
  Tw x_support = (Tw)( blur * bim::max<double>( 1.0/x_factor, 1.0*filter.support() ) );
  Tw y_support = (Tw)( blur * bim::max<double>( 1.0/y_factor, 1.0*filter.support() ) );
  if (bim::max<Tw>(x_support,y_support) > filter.support() )
    filter.setSupport( bim::max<Tw>(x_support,y_support) );

  Td *source_image = NULL;
  if ((w_to*(h_in+h_to)) > (h_to*(w_in+w_to))) {
    source_image = new Td [ w_to*h_in ];
    HorizontalFilter<Td,Tw>( psrc, w_in, h_in, offset_in, source_image, w_to, h_in, w_to, filter, blur);
    VerticalFilter<Td,Tw>( source_image, w_to, h_in, w_to, pdest, w_to, h_to, offset_to, filter, blur );
  } else {
    source_image = new Td [ w_in*h_to ];
    VerticalFilter<Td,Tw>( psrc, w_in, h_in, offset_in, source_image, w_in, h_to, w_in, filter, blur );
    HorizontalFilter<Td,Tw>( source_image, w_in, h_to, w_in, pdest, w_to, h_to, offset_to, filter, blur);
  }
  if (source_image) delete [] source_image;

}

//------------------------------------------------------------------------------------
// resize
//------------------------------------------------------------------------------------

template <typename T, typename Tw>
void image_resample_NN ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                         const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {

    Tw ratio_h = w_in  / (Tw) w_to;
    Tw ratio_v = h_in / (Tw) h_to;

    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (h_to>BIM_OMP_FOR2)
    for (bim::int64 y=0; y<h_to; y++) {
        T *q = ((T*) pdest) + y*offset_to;
        for (unsigned int x=0; x<w_to; x++) {
            unsigned int xn = bim::trim<unsigned int, Tw>(ceil(x*ratio_h), 0, w_in-1 );
            unsigned int yn = bim::trim<unsigned int, Tw>(ceil(y*ratio_v), 0, h_in-1 );
            const T *p = psrc + (offset_in * yn);
            q[x] = p[xn];      
        } // x
    } // y
}

template <typename T>
void image_resample_NN ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {
  image_resample_NN<T, float> ( pdest, w_to, h_to, offset_to, psrc,  w_in, h_in, offset_in );
}


template <typename T, typename Tw>
void image_resample_BL ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                         const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {
  DTriangleFilter<Tw> filter;
  ResizeImage<T, Tw>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in, filter, 0.85f );
}

template <typename T>
void image_resample_BL ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {
  image_resample_BL<T, float> ( pdest, w_to, h_to, offset_to, psrc,  w_in, h_in, offset_in );
}


template <typename T, typename Tw>
void image_resample_BC ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {
  DCubicFilter<Tw> filter;
  ResizeImage<T, Tw>( pdest, w_to, h_to, offset_to, psrc, w_in, h_in, offset_in, filter, 0.95f );
}

template <typename T>
void image_resample_BC ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {
  image_resample_BC<T, float> ( pdest, w_to, h_to, offset_to, psrc,  w_in, h_in, offset_in );
}


//************************************************************************************
// Vector interpolation
//************************************************************************************

//------------------------------------------------------------------------------------
//  o filter: filter to use
//  o blur: The blur factor where > 1 is blurry, < 1 is sharp
//------------------------------------------------------------------------------------
template <typename Td, typename Tw>
std::vector<Td> ResizeVector( const std::vector<Td> &src, unsigned int new_size,
                              DInterpolationFilter<Tw> &filter, const Tw blur )
{
  unsigned int j, n;
  unsigned int start, stop;
  Tw alpha, center, density, scale, support;
  register unsigned int i, x;
  Tw pixel;

  std::vector<Td> q(new_size);  

  Td TdMin = bim::lowest<Td>();
  Td TdMax = std::numeric_limits<Td>::max();
  Tw TwEps = std::numeric_limits<Tw>::epsilon();

  Tw x_factor = (Tw) new_size /(Tw) src.size();
  Tw x_support = (Tw)( blur * bim::max<double>( 1.0/x_factor, 1.0*filter.support() ) );
  if (x_support > filter.support() )
    filter.setSupport( x_support );

  //Apply filter to resize horizontally from source to destination
  scale = (Tw)( blur * bim::max<double>( 1.0/x_factor, 1.0 ) );
  support = scale * filter.support();

  if (support <= 0.5) {
    // Reduce to point sampling
    support = (Tw) (0.5+TwEps);
    scale = 1.0;
  }
  scale = (Tw)(1.0/scale);
  if (support*2+2 > filter.contribution.size()) 
    filter.contribution.resize( (unsigned int)(support*2+2) );

  for (x=0; x<new_size; ++x) {
    center  = (Tw)  (x+0.5)/x_factor;
    start   = (int) (bim::max<double>(center-support,0.0) + 0.5);
    stop    = (int) (bim::min<double>(center+support,src.size()+0.5));
    density = 0.0;

    for (n=0; n<(stop-start); ++n) {
      filter.contribution[n].pixel=start+n;
      filter.contribution[n].weight = filter.function( (Tw)(scale * ((start+n)-center+0.5)) );
      density += filter.contribution[n].weight;
    }
    
    if ((density != 0.0) && (density != 1.0)) {
      // normalize
      density = (Tw)(1.0/density);
      for (i=0; i<n; ++i)
        filter.contribution[i].weight *= density;
    }

    pixel = 0;
    for (i=0; i<n; ++i) {
      j = (filter.contribution[i].pixel - filter.contribution[0].pixel);
      alpha = filter.contribution[i].weight;
      pixel += alpha * src[j+filter.contribution[0].pixel];
     } // for i to n
    q[x] = bim::trim<Td, Tw>( pixel, TdMin, TdMax );

  } // for x

  return q;
}

//------------------------------------------------------------------------------------
// resize
//------------------------------------------------------------------------------------

template <typename T, typename Tw>
std::vector<T> vector_resample_NN ( const std::vector<T> &src, unsigned int new_size ) {

  std::vector<T> q(new_size);
  Tw ratio = src.size() / (Tw) new_size;
  register unsigned int x, xn;

  for (x=0; x<new_size; x++) {
    xn = bim::trim<unsigned int, Tw>(ceil(x*ratio), 0, src.size()-1 );
    q[x] = src[xn];      
  } // x
  return q;
}

template <typename T>
std::vector<T> vector_resample_NN ( const std::vector<T> &src, unsigned int new_size ) {
  return vector_resample_NN<T, double>( src, new_size );
}


template <typename T, typename Tw>
std::vector<T> vector_resample_BL ( const std::vector<T> &src, unsigned int new_size ) {
  DTriangleFilter<Tw> filter;
  return ResizeVector<T, Tw>( src, new_size, filter, 0.85f );
}

template <typename T>
std::vector<T> vector_resample_BL ( const std::vector<T> &src, unsigned int new_size ) {
  return vector_resample_BL<T, double>( src, new_size );
}


template <typename T, typename Tw>
std::vector<T> vector_resample_BC ( const std::vector<T> &src, unsigned int new_size ) {
  DCubicFilter<Tw> filter;
  return ResizeVector<T, Tw>( src, new_size, filter, 0.95f );
}

template <typename T>
std::vector<T> vector_resample_BC ( const std::vector<T> &src, unsigned int new_size ) {
  return vector_resample_BC<T, double>( src, new_size );
}

#endif //BIM_RESIZE_H
