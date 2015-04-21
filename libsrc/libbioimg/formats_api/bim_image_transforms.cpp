/*******************************************************************************

  Implementation of the Image Transforms
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2011-05-11 08:32:12 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_TRANSFORMS
#pragma message("bim::Image: Transforms")

#include "xtypes.h"
#include "bim_image.h"

#include <algorithm>
#include <limits>
#include <cstring>

#include <xstring.h>
#include <bim_metatags.h>

#include <fftw3.h>
#include "../transforms/FuzzyCalc.h"
#include "../transforms/chebyshev.h"
#include "../transforms/wavelet/Symlet5.h"
#include "../transforms/wavelet/DataGrid2D.h"
#include "../transforms/radon.h"

using namespace bim;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

//------------------------------------------------------------------------------------
// Transforms
//------------------------------------------------------------------------------------

Image fft2 (const Image &matrix_IN) {
    Image im = matrix_IN.convertToDepth(64, bim::Lut::ltTypecast, bim::FMT_FLOAT);
    unsigned int width  = (unsigned int) im.width();
    unsigned int height = (unsigned int) im.height();
    unsigned int half_height = (unsigned int) im.height()/2+1;
  
    double *in = (double*) fftw_malloc(sizeof(double) * width*height);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * width*height);
    fftw_plan p = fftw_plan_dft_r2c_2d(width,height,in,out, FFTW_MEASURE); // FFTW_ESTIMATE: deterministic
  
    for (unsigned int sample=0; sample<im.samples(); sample++) {
        double *in_plane = (double*) im.bits(sample);
        for (unsigned int y=0; y<height; y++)
        for (unsigned int x=0; x<width; x++) {
            in[height*x+y] = *in_plane;
            in_plane++;
        }
        fftw_execute(p);

        // The resultant image uses the modulus (sqrt(nrm)) of the complex numbers for pixel values
        double *out_plane = (double*) im.bits(sample);
        for (unsigned int y=0; y<half_height; y++)
        for (unsigned int x=0; x<width; x++) {
            unsigned int idx = half_height*x+y;
            *out_plane = sqrt(pow(out[idx][0],2)+pow(out[idx][1],2));    // sqrt(real(X).^2 + imag(X).^2)
            out_plane++;
        }

        // complete the first column
        out_plane = (double*) im.bits(sample);
        #pragma omp parallel for default(shared)
        for (int y=half_height; y<height; y++) {
            double *pO = out_plane + width*y;
            double *pI = out_plane + width*(height-y);
            *pO = *pI;
        }

        // complete the rest of the columns
        out_plane = (double*) im.bits(sample);
        #pragma omp parallel for default(shared)
        for (int y=half_height; y<height; y++)
        for (unsigned int x=1; x<width; x++) { // 1 because the first column is already completed
            double *pO = out_plane + width*y + x;
            double *pI = out_plane + width*(height-y) + (width-x);
            *pO = *pI;
        }
    } // samples

    // clean up
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    return im;
}

// dima: This implementation comes from WndChrm and is not the best, rewrite when possible
Image chebyshev2 (const Image &matrix_IN) {
    Image in = matrix_IN.convertToDepth(64, bim::Lut::ltTypecast, bim::FMT_FLOAT);
    bim::uint64 N = std::min<bim::uint64>(in.width(), in.height());
	bim::uint64 width = N;
	bim::uint64 height = std::min<bim::uint64>( in.height(), N ); 
    bim::uint64 samples = in.samples();

    Image out(width, height, 64, samples, FMT_FLOAT );
    for (unsigned int sample=0; sample<in.samples(); sample++) {
        double *inp = (double *) in.bits(sample);
        double *oup = (double *) out.bits(sample);
        Chebyshev2D(inp, oup, N, in.width(), in.height());
    }
    return out;
}

//------------------------------------------------------------------------------------
// wavelet
//------------------------------------------------------------------------------------

template <typename Ti, typename To>
void do_wavelet2 (const Image &in, Image &out) {
    unsigned int width  = (unsigned int) in.width();
    unsigned int height = (unsigned int) in.height();
    bim::uint64 samples = in.samples();

    for (unsigned int sample=0; sample<in.samples(); sample++) {
	    DataGrid *grid = new DataGrid2D(width,height,-1);
        //Ti *p = (Ti*) in.bits(sample);
        #pragma omp parallel for default(shared)
        for (int y=0;y<height;y++) {
            Ti *p = (Ti*) in.scanLine(sample, y);
		    for(unsigned int x=0; x<width; x++) {
			    grid->setData(x, y, -1, (double) *p);
                p++;
            }
        }
	    Symlet5 *Sym5 = new Symlet5(0, 1); // dima: here the number of levels is only 1, we probably should estimate appropriate number here
	    Sym5->transform2D(grid);
	    
        unsigned int ow = std::min<unsigned int>(grid->getX(), out.width());
        unsigned int oh = std::min<unsigned int>(grid->getY(), out.height());
        //To *po = (To*) out.bits(sample);
        #pragma omp parallel for default(shared)
        for (int y=0;y<oh;y++) {
            To *po = (To*) out.scanLine(sample, y);
		    for(unsigned int x=0; x<ow; x++) {
			    *po = (To) grid->getData(x, y, -1);
                po++;
            }
        }

	    delete Sym5;
	    delete grid;
    } // sample
}
 
Image wavelet2 (const Image &in) {
    unsigned int width  = (unsigned int) in.width();
    unsigned int height = (unsigned int) in.height();
    bim::uint64 samples = in.samples();
    // wavelet code adds borders between outputs
    Image out(width+8, height+8, 64, samples, FMT_FLOAT );

    if (in.depth()==8 && in.pixelType()==FMT_UNSIGNED)
        do_wavelet2<bim::uint8, double>  ( in, out );
    else
    if (in.depth()==16 && in.pixelType()==FMT_UNSIGNED)
        do_wavelet2<bim::uint16, double> ( in, out );
    else
    if (in.depth()==32 && in.pixelType()==FMT_UNSIGNED)
        do_wavelet2<bim::uint32, double> ( in, out );
    else
    if (in.depth()==64 && in.pixelType()==FMT_UNSIGNED)
        do_wavelet2<bim::uint64, double> ( in, out );
    else
    if (in.depth()==8 && in.pixelType()==FMT_SIGNED)
        do_wavelet2<bim::int8, double>   ( in, out );
    else
    if (in.depth()==16 && in.pixelType()==FMT_SIGNED)
        do_wavelet2<bim::int16, double>  ( in, out );
    else
    if (in.depth()==32 && in.pixelType()==FMT_SIGNED)
        do_wavelet2<bim::int32, double>  ( in, out );
    else
    if (in.depth()==64 && in.pixelType()==FMT_SIGNED)
        do_wavelet2<bim::int64, double>  ( in, out );
    else
    if (in.depth()==32 && in.pixelType()==FMT_FLOAT)
        do_wavelet2<bim::float32, double> ( in, out );
    else
    if (in.depth()==64 && in.pixelType()==FMT_FLOAT)
        do_wavelet2<bim::float64, double> ( in, out );

    return out;
}

//------------------------------------------------------------------------------------
// radon
//------------------------------------------------------------------------------------

Image radon2 (const Image &matrix_IN) {
    Image in = matrix_IN.convertToDepth(64, bim::Lut::ltTypecast, bim::FMT_FLOAT);
	bim::uint64 width = in.width();
	bim::uint64 height = in.height(); 
    bim::uint64 samples = in.samples();

    //int num_angles=4;
	//double theta[4]={0,45,90,135};

    double theta[181];
    for (int a=0; a<181; a++) theta[a] = a;
    int num_angles=180;

	int rLast = (int) ceil(sqrt(pow( (double)(width-1-(width-1)/2),2)+pow( (double)(height-1-(height-1)/2),2))) + 1;
	int rFirst = -rLast;
	unsigned int output_size = rLast-rFirst+1;

    Image out(output_size, num_angles, 64, samples, FMT_FLOAT );
    out.fill(0);

    for (unsigned int sample=0; sample<in.samples(); sample++) {
        double *ptr = (double *) out.bits(sample);
        double *pixels = (double *) in.bits(sample);
        radon(ptr, pixels, theta, height, width, (width-1)/2, (height-1)/2, num_angles, rFirst, output_size);
	}

    out = out.rotate(90);
    return out;
}

//------------------------------------------------------------------------------------
// image transforms
// inverse transforms are not implemented yet
//------------------------------------------------------------------------------------

Image Image::transform( Image::TransformMethod type ) const {
    if (type==Image::tmFFT)
        return fft2 (*this);
    else if (type==Image::tmChebyshev)
        return chebyshev2 (*this);    
    else if (type==Image::tmWavelet)
        return wavelet2 (*this);    
    else if (type==Image::tmRadon)
        return radon2 (*this);    

    return Image();  
}


Image operation_transform(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::TransformMethod transform = Image::tmNone;
    if (arguments.toLowerCase() == "chebyshev") transform = Image::tmChebyshev;
    if (arguments.toLowerCase() == "fft")       transform = Image::tmFFT;
    if (arguments.toLowerCase() == "radon")     transform = Image::tmRadon;
    if (arguments.toLowerCase() == "wavelet")   transform = Image::tmWavelet;

    if (transform != Image::tmcNone)
        return img.transform(transform);
    return img;
};

//------------------------------------------------------------------------------------
// 3c Color Transforms 
//------------------------------------------------------------------------------------

inline void rgb2hsv ( const double &i1, const double &i2, const double &i3, double &o1, double &o2, double &o3, const double &tmax, const double &tmin, const double &range ) {
    double r = (i1 - tmin) / range;
    double g = (i2 - tmin) / range;
    double b = (i3 - tmin) / range;

    double maxv = std::max (r, std::max (g, b));
    double minv = std::min (r, std::min (g, b));
    double delta = maxv - minv;

    double v = maxv*240.0;
    double s = 0;
    if (maxv != 0.0)
        s = (delta / maxv)*240.0;

    double h = 0;
    if (s != 0) {
        if (r == maxv)
            h = (g - b) / delta;
        else if (g == maxv)
            h = 2 + (b - r) / delta;
        else if (b == maxv)
            h = 4 + (r - g) / delta;
        h *= 60.0;
        if (h >= 360) h -= 360.0;
        if (h < 0.0) h += 360.0;
        h *= (240.0/360.0);
    }
  
    o1 = h;
    o2 = s;
    o3 = v;
}

inline void hsv2rgb ( const double &i1, const double &i2, const double &i3, double &o1, double &o2, double &o3, const double &tmax, const double &tmin, const double &range ) {
	  double R=0, G=0, B=0;
	  double H = i1;
	  double S = i2/240.0;
	  double V = i3/240.0;
	  if (S == 0 && H == 0) {R=G=B=V;}  /*if S=0 and H is undefined*/
	  H = H*(360.0/240.0);
	  if (H == 360) H=0;
	  H = H/60;
	  double i = floor(H);
	  double f = H-i;
	  double p = V*(1-S);
	  double q = V*(1-(S*f));
	  double t = V*(1-(S*(1-f)));

	  if (i==0) {R=V;  G=t;  B=p;}
	  if (i==1) {R=q;  G=V;  B=p;}
	  if (i==2) {R=p;  G=V;  B=t;}
	  if (i==3) {R=p;  G=q;  B=V;}
	  if (i==4) {R=t;  G=p;  B=V;}
	  if (i==5) {R=V;  G=p;  B=q;}

	  o1 = R*range + tmin;
	  o2 = G*range + tmin;
	  o3 = B*range + tmin;
}

inline void hsv2wndchrmcolor ( const double &h, const double &s, const double &v, double &o1, double &o2, double &o3, const double &tmax, const double &tmin, const double &range ) {
    long color_index = FindColor( h, s, v );
    o1 = ( range * color_index ) / COLORS_NUM;
    o2 = 0;
    o3 = 0;
}


inline void rgb2xyz ( const double &i1, const double &i2, const double &i3, double &o1, double &o2, double &o3, const double &tmax, const double &tmin, const double &range ) {
/*
void RGB2XYZ(
	const int&		sR,
	const int&		sG,
	const int&		sB,
	double&			X,
	double&			Y,
	double&			Z)
{
	double R = sR/255.0;
	double G = sG/255.0;
	double B = sB/255.0;

	double r, g, b;

	if(R <= 0.04045)	r = R/12.92;
	else				r = pow((R+0.055)/1.055,2.4);
	if(G <= 0.04045)	g = G/12.92;
	else				g = pow((G+0.055)/1.055,2.4);
	if(B <= 0.04045)	b = B/12.92;
	else				b = pow((B+0.055)/1.055,2.4);

	X = r*0.4124564 + g*0.3575761 + b*0.1804375;
	Y = r*0.2126729 + g*0.7151522 + b*0.0721750;
	Z = r*0.0193339 + g*0.1191920 + b*0.9503041;
}
*/
}

inline void rgb2lab ( const double &i1, const double &i2, const double &i3, double &o1, double &o2, double &o3, const double &tmax, const double &tmin, const double &range ) {
/*
void RGB2LAB(const int& sR, const int& sG, const int& sB, double& lval, double& aval, double& bval)
{
	//------------------------
	// sRGB to XYZ conversion
	//------------------------
	double X, Y, Z;
	RGB2XYZ(sR, sG, sB, X, Y, Z);

	//------------------------
	// XYZ to LAB conversion
	//------------------------
	double epsilon = 0.008856;	//actual CIE standard
	double kappa   = 903.3;		//actual CIE standard

	double Xr = 0.950456;	//reference white
	double Yr = 1.0;		//reference white
	double Zr = 1.088754;	//reference white

	double xr = X/Xr;
	double yr = Y/Yr;
	double zr = Z/Zr;

	double fx, fy, fz;
	if(xr > epsilon)	fx = pow(xr, 1.0/3.0);
	else				fx = (kappa*xr + 16.0)/116.0;
	if(yr > epsilon)	fy = pow(yr, 1.0/3.0);
	else				fy = (kappa*yr + 16.0)/116.0;
	if(zr > epsilon)	fz = pow(zr, 1.0/3.0);
	else				fz = (kappa*zr + 16.0)/116.0;

	lval = 116.0*fy-16.0;
	aval = 500.0*(fx-fy);
	bval = 200.0*(fy-fz);
}
*/
}

//------------------------------------------------------------------------------------
// Color converters
//------------------------------------------------------------------------------------

template <typename T, typename F>
bool converter_3c( const Image &in, Image &out, F func ) {
    if (in.width()   != out.width())   return false;
    if (in.height()  != out.height())  return false;
    if (in.samples() != out.samples()) return false;
    if (in.depth()   != out.depth())   return false;
    if (in.samples() < 3) return false;

    bim::uint64 w = (bim::uint64) in.width();
    bim::uint64 h = (bim::uint64) in.height();

    double tmax  = (double) std::numeric_limits<T>::max();
    double tmin = (double) bim::lowest<T>();
    double range = (double) std::numeric_limits<T>::max() - bim::lowest<T>();

    #pragma omp parallel for default(shared)
    for (int y=0; y<h; ++y ) {
        double d1, d2, d3;
        T *src_1 = (T *) in.scanLine ( bim::Red,   y ); 
        T *src_2 = (T *) in.scanLine ( bim::Green, y ); 
        T *src_3 = (T *) in.scanLine ( bim::Blue,  y ); 
        T *dst_1 = (T *) out.scanLine( bim::Red,   y ); 
        T *dst_2 = (T *) out.scanLine( bim::Green, y ); 
        T *dst_3 = (T *) out.scanLine( bim::Blue,  y ); 
        for (unsigned int x=0; x<w; ++x) {
            func ( (double)src_1[x], (double)src_2[x], (double)src_3[x], d1, d2, d3, tmax, tmin, range );
            dst_1[x] = (T) d1;
            dst_2[x] = (T) d2;
            dst_3[x] = (T) d3;
        }
    }
    return true;
}

template <typename F>
bool convert_colors( const Image &in, Image &out, F func ) {
    if (in.depth()==8 && in.pixelType()==FMT_UNSIGNED)
      return converter_3c<bim::uint8>  ( in, out, func );
    else
    if (in.depth()==16 && in.pixelType()==FMT_UNSIGNED)
      return converter_3c<bim::uint16> ( in, out, func );
    else
    if (in.depth()==32 && in.pixelType()==FMT_UNSIGNED)
      return converter_3c<bim::uint32> ( in, out, func );
    else
    if (in.depth()==64 && in.pixelType()==FMT_UNSIGNED)
      return converter_3c<bim::uint64> ( in, out, func );
    else
    if (in.depth()==8 && in.pixelType()==FMT_SIGNED)
      return converter_3c<bim::int8>   ( in, out, func );
    else
    if (in.depth()==16 && in.pixelType()==FMT_SIGNED)
      return converter_3c<bim::int16>  ( in, out, func );
    else
    if (in.depth()==32 && in.pixelType()==FMT_SIGNED)
      return converter_3c<bim::int32>  ( in, out, func );
    else
    if (in.depth()==64 && in.pixelType()==FMT_SIGNED)
      return converter_3c<bim::int64>  ( in, out, func );
    else
    if (in.depth()==32 && in.pixelType()==FMT_FLOAT)
      return converter_3c<bim::float32> ( in, out, func );
    else
    if (in.depth()==64 && in.pixelType()==FMT_FLOAT)
      return converter_3c<bim::float64> ( in, out, func );
    else
    return false;
}

Image Image::transform_color( Image::TransformColorMethod type ) const {
    Image out = this->deepCopy();

    if (type==Image::tmcRGB2HSV) {
        convert_colors( *this, out, rgb2hsv );
        out.bmp->i.imageMode = bim::IM_HSV;
    } else if (type==Image::tmcHSV2RGB) {
        convert_colors( *this, out, hsv2rgb );
        out.bmp->i.imageMode = bim::IM_RGB;
    } else if (type==Image::tmcRGB2WndChrmColor) {
        convert_colors( *this, out, rgb2hsv );
        convert_colors( out, out, hsv2wndchrmcolor );
        out.extractChannel(bim::Red);
        out.bmp->i.imageMode = bim::IM_GRAYSCALE;
    }
    return out;
}

Image operation_transform_color(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    Image::TransformColorMethod transform_color = Image::tmcNone;
    if (arguments.toLowerCase() == "rgb2hsv") transform_color = Image::tmcRGB2HSV;
    if (arguments.toLowerCase() == "hsv2rgb") transform_color = Image::tmcHSV2RGB;

    if (transform_color != Image::tmcNone)
        return img.transform_color(transform_color);
    return img;
};

Image operation_filter(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    if (arguments.toLowerCase() == "edge")
        return img.filter_edge();
    else if (arguments.toLowerCase() == "wndchrmcolor")
        return img.transform_color(Image::tmcRGB2WndChrmColor);
    else if (arguments.toLowerCase() == "otsu")
        return img.filter_edge();
    return img;
};

//------------------------------------------------------------------------------------
// Hounsfield Units - used for CT (CAT) data
// provided conversion maps from device dependent to HU (device independent) scale
// typically this conversion will only make sense for 1 sample per pixel images with signed 16 bit pixels or floating point
// most devices use slope == 1.0 and intercept == -1024.0
//------------------------------------------------------------------------------------

template <typename T, typename Tw>
bool converter_hounsfield(const Image &in, Image &out, const Tw &slope, const Tw &intercept) {
    if (in.width() != out.width())   return false;
    if (in.height() != out.height())  return false;
    if (in.samples() != out.samples()) return false;
    if (in.depth() != out.depth())   return false;

    bim::uint64 w = (bim::uint64) in.width();
    bim::uint64 h = (bim::uint64) in.height();
    double tmax = (double)std::numeric_limits<T>::max();
    double tmin = (double)bim::lowest<T>();

    for (int s = 0; s < in.samples(); ++s) {
        #pragma omp parallel for default(shared)
        for (bim::int64 y = 0; y < h; ++y) {
            T *src = (T *)in.scanLine(s, y);
            T *dst = (T *)out.scanLine(s, y);
            for (unsigned int x = 0; x < w; ++x) {
                //dst[x] = (T)((Tw)src[x] * slope + intercept);
                dst[x] = bim::trim<T, Tw>(src[x] * slope + intercept, tmin, tmax);
            }
        }
    }
    return true;
}

Image Image::transform_hounsfield(const double &slope, const double &intercept ) const {
    Image out;

    if (this->pixelType() == FMT_SIGNED || this->pixelType() == FMT_FLOAT) {
        out = this->deepCopy();
    } else {
        // convert to signed data type keeping image depth, typically this should be 16 bit per sample
        out = this->convertToDepth(32, Lut::ltTypecast, FMT_FLOAT);
    }

    if (out.depth() == 8 && out.pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int8, double>(out, out, slope, intercept);
    else
    if (out.depth() == 16 && out.pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int16, double>(out, out, slope, intercept);
    else
    if (out.depth() == 32 && out.pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int32, double>(out, out, slope, intercept);
    else
    if (out.depth() == 64 && out.pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int64, double>(out, out, slope, intercept);
    else
    if (out.depth() == 32 && out.pixelType() == FMT_FLOAT)
        converter_hounsfield<bim::float32, double>(out, out, slope, intercept);
    else
    if (out.depth() == 64 && out.pixelType() == FMT_FLOAT)
        converter_hounsfield<bim::float64, double>(out, out, slope, intercept);
    
    return out;
}

// mutable version of same operation, more memory efficient, only valid for float and signed images
bool Image::transform_hounsfield_inplace(const double &slope, const double &intercept) {
    if (this->pixelType() == FMT_UNSIGNED) return false;
    if (this->depth() < 16) return false;

    if (this->depth() == 16 && this->pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int16, double>(*this, *this, slope, intercept);
    else
    if (this->depth() == 32 && this->pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int32, double>(*this, *this, slope, intercept);
    else
    if (this->depth() == 64 && this->pixelType() == FMT_SIGNED)
        converter_hounsfield<bim::int64, double>(*this, *this, slope, intercept);
    else
    if (this->depth() == 32 && this->pixelType() == FMT_FLOAT)
        converter_hounsfield<bim::float32, double>(*this, *this, slope, intercept);
    else
    if (this->depth() == 64 && this->pixelType() == FMT_FLOAT)
        converter_hounsfield<bim::float64, double>(*this, *this, slope, intercept);

    return true;
}

//-----------------------------------------------------------------------------------
// enhance_hounsfield
//-----------------------------------------------------------------------------------

template <typename T>
bool enhancer_hounsfield(const Image &in, const T &min_val, const T &max_val, const T &min_set, const T &max_set) {
    bim::uint64 w = (bim::uint64) in.width();
    bim::uint64 h = (bim::uint64) in.height();

    for (int s = 0; s < in.samples(); ++s) {
        #pragma omp parallel for default(shared)
        for (bim::int64 y = 0; y < h; ++y) {
            T *src = (T *)in.scanLine(s, y);
            for (unsigned int x = 0; x < w; ++x) {
                if (src[x] < min_val) src[x] = min_set;
                if (src[x] > max_val) src[x] = max_set;
            }
        }
    }
    return true;
}

Image compute_hounsfield(const Image &in, int depth, DataFormat pxtype, const double &minv, const double &maxv, const double &maxs) {
    Image img = in.deepCopy();

    if (in.depth() == 8 && in.pixelType() == FMT_SIGNED)
        enhancer_hounsfield<bim::int8>(img, minv, maxv, minv, maxs);
    else
    if (in.depth() == 16 && in.pixelType() == FMT_SIGNED)
        enhancer_hounsfield<bim::int16>(img, minv, maxv, minv, maxs);
    else
    if (in.depth() == 32 && in.pixelType() == FMT_SIGNED)
        enhancer_hounsfield<bim::int32>(img, minv, maxv, minv, maxs);
    else
    if (in.depth() == 64 && in.pixelType() == FMT_SIGNED)
        enhancer_hounsfield<bim::int64>(img, minv, maxv, minv, maxs);
    else
    if (in.depth() == 32 && in.pixelType() == FMT_FLOAT)
        enhancer_hounsfield<bim::float32>(img, minv, maxv, minv, maxs);
    else
    if (in.depth() == 64 && in.pixelType() == FMT_FLOAT)
        enhancer_hounsfield<bim::float64>(img, minv, maxv, minv, maxs);

    if (depth == img.depth() && pxtype == img.pixelType())
        return img;
    return img.convertToDepth(depth, Lut::ltLinearDataRange, pxtype, Histogram::cmSeparate);
}

// typical enhancement of CT images using Hounsfield scale, where pixels are normalized using
// min and max computed from window center and window width given in Hounsfield Units
// image MUST be previously converted to HU using transform_hounsfield or transform_hounsfield_inplace
// Typical values of center/width:
//    HeadSFT:          40 / 80  head soft tissue
//    Brain             30 / 110
//    NeckSFT :         60 / 300
//    Bone :            400 / 2000 
//    Temporal bones:   400 / 4000  (bones of the scull)
//    Bone body:        350 / 2500 
//    Soft Tissue :     40 / 500
//    SoftTissue(PEDS): 40 / 400   just soft tissue CT (pediatric )
//    Mediastinum:      400/1800
//    Bronchial:        -180 / 2600
//    Lung :            -350 / 2000
//    Lung 2:           -700 / 1200
//    Abdomen           -20 / 400
//    Liver:            60 / 180
//    Liver W/O:        40 / 150 without contrast
//    Liver W/C:        100 / 150 with contrast
//    P Fossa :         30 / 180
//    CSpineSFT w/o :   40 / 250   Cervical spine without contrast
//    TLSpineSFT w/o:   40 / 500   Thoracic and Lumbar spine
//    INFARCT :         40 / 60
//    OBLIQUE MIP :     200 / 700
//    MYELOGRAM W/L:    60 / 650
Image Image::enhance_hounsfield(int depth, DataFormat pxtype, const double &wnd_center, const double &wnd_width, bool empty_outside_range) const {
    double minv = wnd_center - (wnd_width/2.0);
    double maxv = wnd_center + (wnd_width/2.0);
    double maxs = empty_outside_range ? minv : maxv;
    return compute_hounsfield(*this, depth, pxtype, minv, maxv, maxs);
}

// produces multi channel image with different ranges as separate channels
// image MUST be previously converted to HU using transform_hounsfield or transform_hounsfield_inplace
// defined 5 channels:
//   1 : -inf to -100 : Lungs
//   2 : -100 to -50  : Fat
//   3 : -50  to 50   : Brain
//   4 :  50  to 250  : Organs
//   5 :  250 to inf  : Bones
Image Image::multi_hounsfield() const {
    // Lungs
    Image img = compute_hounsfield(*this, 8, FMT_UNSIGNED, -1300, -100, -1300);
    // Fat
    img = img.appendChannels(compute_hounsfield(*this, 8, FMT_UNSIGNED, -150, -25, -150));
    // Brain
    img = img.appendChannels(compute_hounsfield(*this, 8, FMT_UNSIGNED, -30, 90, -30));
    // Organs
    img = img.appendChannels(compute_hounsfield(*this, 8, FMT_UNSIGNED, 50, 250, 50));
    // Bones
    img = img.appendChannels(compute_hounsfield(*this, 8, FMT_UNSIGNED, 200, 2000, 2000));

    TagMap meta = img.get_metadata();
    
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "255,255,255");
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0), "Lungs");

    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "0,255,255");
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0), "Fat");

    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "0,255,0");
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0), "Brain");

    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "255,0,0");
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0), "Organs");

    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_COLOR_TEMPLATE.c_str(), 0), "255,255,0");
    meta.set_value(bim::xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), 0), "Bones");

    img.set_metadata(meta);
    return img;
}

Image operation_hounsfield(Image &img, const bim::xstring &arguments, const xoperations &operations, ImageHistogram *hist, XConf *c) {
    std::vector<xstring> strl = arguments.split(",");
    if (strl.size() < 2) {
        c->print("Number of arguments for -hounsfield is less then 2, skipping hounsfield...", 2);
        return img;
    }
    int depth = strl[0].toInt(0);
    DataFormat pf = FMT_UNSIGNED;
    if (strl[1].toLowerCase() == "s") pf = FMT_SIGNED;
    if (strl[1].toLowerCase() == "f") pf = FMT_FLOAT;
    double wnd_center = 0.0;
    double wnd_width = 0.0;
    if (strl.size() >= 4) {
        wnd_center = strl[2].toDouble(0.0);
        wnd_width = strl[3].toDouble(0.0);
    }
    if (strl.size() < 4 || wnd_width == 0) {
        // if window center and width were not provided, read from metadata
        wnd_center = img.get_metadata_tag_double("DICOM/Window Center (0028,1050)", 0.0);
        wnd_width = img.get_metadata_tag_double("DICOM/Window Width (0028,1051)", 0.0);
    }
    if (wnd_width == 0) {
        c->print("Window center and width were not provied and could not be red from metadata, skipping hounsfield...", 2);
        return img;
    }

    double slope = 1.0;
    double intercept = -1024.0;
    if (strl.size() > 5) {
        // if slope and intercept provided in the command line
        slope = strl[4].toDouble(1.0);
        intercept = strl[5].toDouble(-1024.0);
    }
    else {
        // try to read from the metadata
        if (img.get_metadata_tag("DICOM/Rescale Type (0028,1054)", "") == "HU") {
            slope = img.get_metadata_tag_double("DICOM/Rescale Slope (0028,1053)", 1.0);
            intercept = img.get_metadata_tag_double("DICOM/Rescale Intercept (0028,1052)", -1024.0);
        }
    }

    if (depth != 8 && depth != 16 && depth != 32 && depth != 64) {
        std::cout << xstring::xprintf("Hounsfield output depth (%s bpp) is not supported, skipping...\n", depth);
        return img;
    }

    if (!img.transform_hounsfield_inplace(slope, intercept)) {
        img = img.transform_hounsfield(slope, intercept);
    }
    return img.enhance_hounsfield(depth, pf, wnd_center, wnd_width);
    //img = img.multi_hounsfield();
};

#endif //BIM_USE_TRANSFORMS
