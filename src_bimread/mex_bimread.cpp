/*******************************************************************************
  Matlab interface for bioImage formats library

  Input:
   fname    - string with file name of image to decode 
   page_num - the number of the page to decode, 0 if ommited
              in time series pages are time points
              in the z series pages are depth points

  Output:
   im       - matrix of the image in the native format with channels in the 3d dimension
   format   - string with format name used to decode the image
   pages    - number of pages in the image
   xyzres   - pixel size on x,y,z dim in microns double[3]
   metatxt  - string with all meta-data extracted from the image

  ex:
   [im, format, pages, xyzr, metatxt] = bimread( fname, page_num );
   
   [im] = bimread( fname );

  Notes: 
    Currently only creates 8 or 16 bit arrays!!! 
    Extend if you want to read other type of data.


  Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    10/13/2006 16:00 - First creation

  Ver : 4
*******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <xstring.h>

#include <dim_img_format_interface.h>
#include <dim_img_format_utils.h>
#include <dim_image.h>
#include <dim_format_manager.h>
#include <meta_format_manager.h>

extern "C" {
#include "mex.h"
}

template <typename T>
void copy_data ( const TDimImage &img, mxArray *plhs[] ) {
  T *p = (T *) mxGetData(plhs[0]);
  for (unsigned int c=0; c<img.samples(); ++c)
  for (unsigned int x=0; x<img.width(); ++x)
  for (unsigned int y=0; y<img.height(); ++y) {
    T *ip = (T *) img.scanLine( c, y );
    *p = ip[x];
    ++p;
  } // y
}


/*
#if !defined(MX_API_VER) || (MX_API_VER < 0x07000000)
  #ifdef MX_COMPAT_32
  typedef int mwSize;
  #else
  typedef size_t    mwSize;
  #endif
#endif
*/
//------------------------------------------------------------------------------
// function: mex_bimread - entry point from Matlab via mexFucntion()
// INPUTS:
//   nlhs - number of left hand side arguments (outputs)
//   plhs[] - pointer to table where created matrix pointers are
//            to be placed
//   nrhs - number of right hand side arguments (inputs)
//   prhs[] - pointer to table of input matrices
//------------------------------------------------------------------------------

void mex_bimread( int nlhs, mxArray *plhs[], int nrhs, const mxArray  *prhs[] )
{
	// Check for proper number of input and output arguments
	if (nrhs < 1) mexErrMsgTxt("At least 1 input arguments required. Usage: bimread( fname, page_num ).");
	if (nlhs < 1) mexErrMsgTxt("At least 1 output argument required. Usage: [ im, format, pages, xyzres, metatxt ] = bimread.");
  if (nrhs > 2) mexErrMsgTxt("Too many input arguments. Usage: bimread( fname, page_num ).");
  if (nlhs > 5) mexErrMsgTxt("Too many output arguments. Usage: [ im, format, pages, xyzres, metatxt ] = bimread.");

  // input
  char *fname;
  int  page=0;

  // output
  std::string fmt_name = "";
  int num_pages=0;
  double pixel_size[3] = {0.0, 0.0, 0.0};

  //-----------------------------------------------------------
  // get file name
  //-----------------------------------------------------------
  // Input must be a string
  if (mxIsChar(prhs[0]) != 1)
    mexErrMsgTxt("Input 1 must be a string.");

  // Input must be a row vector
  if (mxGetM(prhs[0]) != 1)
    mexErrMsgTxt("Input 1 must be a row vector.");
    
  // Get the length of the input string.
  int buflen = (mxGetM(prhs[0]) * mxGetN(prhs[0])) + 1;

  // Allocate memory for input and output strings
  fname = (char*) mxCalloc(buflen, sizeof(char));

  // Copy the string data from prhs[0] into a C string input_buf
  if ( mxGetString(prhs[0], fname, buflen) != 0 ) 
    mexWarnMsgTxt("Not enough space. String is truncated.");


  //-----------------------------------------------------------
  // if page is provided
  //-----------------------------------------------------------

  if (nrhs > 1) {
    int mrows = mxGetM(prhs[1]);
    int ncols = mxGetN(prhs[1]);
    if (!mxIsDouble(prhs[1]) || mxIsComplex(prhs[1]) ||
        !(mrows == 1 && ncols == 1)) {
      mexErrMsgTxt("Input must be a noncomplex scalar double.");
    }
    double *dv = mxGetPr(prhs[1]);
    page = (int) *dv;
    page--; // to bring to c convention
  } // if page is provided

  #ifdef _DEBUG
  mexPrintf("Filename to load: %s\n", fname);
  mexPrintf("Page to load: %d\n", page);
  #endif

  //-----------------------------------------------------------
  // now read image and metadata
  //-----------------------------------------------------------
  
  TMetaFormatManager fm;
  TDimImage img;

  if (fm.sessionStartRead( fname ) != 0)
    mexErrMsgTxt("Input format is not supported\n");    
  
  num_pages = fm.sessionGetNumberOfPages();
  fmt_name = fm.sessionGetFormatName();

  #if defined (DEBUG) || defined (_DEBUG)
  mexPrintf("Number of pages: %d\n", num_pages);  
  mexPrintf("Format name: %s\n", fmt_name.c_str());  
  #endif

  if (page<0) {
    page=0;
    mexPrintf("Requested page number is invalid, used %d.\n", page+1);  
  }

  if (page>=num_pages) {
    page=num_pages-1;
    mexPrintf("Requested page number is invalid, used %d.\n", page+1);  
  }

  fm.sessionReadImage( img.imageBitmap(), page );

  // getting metadata fields
  fm.sessionParseMetaData(page);
  pixel_size[0] = fm.getPixelSizeX();
  pixel_size[1] = fm.getPixelSizeY();
  pixel_size[2] = fm.getPixelSizeZ();

  #if defined (DEBUG) || defined (_DEBUG)
  mexPrintf("Pixel resolution: %.8f, %.8f, %.8f\n", pixel_size[0], pixel_size[1], pixel_size[2]);  
  #endif

  // get meta text if required
  std::string meta_data_text = "";    
  if (nlhs > 4) {
    DTagMap meta_data = fm.get_metadata();
    meta_data_text = meta_data.join( "; " );
  }

  fm.sessionEnd(); 


  //-----------------------------------------------------------
  // pre-poc input image
  //-----------------------------------------------------------
  
  // make sure red image is in supported pixel format, e.g. will convert 12 bit to 16 bit
  img = img.ensureTypedDepth();

  //-----------------------------------------------------------
  // create output image
  //-----------------------------------------------------------
  #if defined(MX_API_VER) && (MX_API_VER > 0x07000000)
  const mwSize dims[] = { img.height(), img.width(), img.samples() };
  #else
  const int dims[] = { img.height(), img.width(), img.samples() };
  #endif

  mxClassID t = mxUINT8_CLASS;
  if (img.depth()==8  && img.pixelType()==D_FMT_UNSIGNED) t = mxUINT8_CLASS;
  else
  if (img.depth()==16 && img.pixelType()==D_FMT_UNSIGNED) t = mxUINT16_CLASS;
  else
  if (img.depth()==32 && img.pixelType()==D_FMT_UNSIGNED) t = mxUINT32_CLASS;
  else // SIGNED
  if (img.depth()==8  && img.pixelType()==D_FMT_SIGNED)   t = mxINT8_CLASS; 
  else
  if (img.depth()==16 && img.pixelType()==D_FMT_SIGNED)   t = mxINT16_CLASS; 
  else
  if (img.depth()==32 && img.pixelType()==D_FMT_SIGNED)   t = mxINT32_CLASS; 
  else // FLOAT
  if (img.depth()==32 && img.pixelType()==D_FMT_FLOAT)    t = mxSINGLE_CLASS;
  else
  if (img.depth()==64 && img.pixelType()==D_FMT_FLOAT)    t = mxDOUBLE_CLASS;

  plhs[0] = mxCreateNumericArray( 3, dims, t, mxREAL);

  // UNSIGNED
  if (img.depth()==8  && img.pixelType()==D_FMT_UNSIGNED) copy_data<DIM_UINT8>   ( img, plhs );
  else
  if (img.depth()==16 && img.pixelType()==D_FMT_UNSIGNED) copy_data<DIM_UINT16>  ( img, plhs );
  else
  if (img.depth()==32 && img.pixelType()==D_FMT_UNSIGNED) copy_data<DIM_UINT32>  ( img, plhs );
  else // SIGNED
  if (img.depth()==8  && img.pixelType()==D_FMT_SIGNED)   copy_data<DIM_INT8>    ( img, plhs );
  else
  if (img.depth()==16 && img.pixelType()==D_FMT_SIGNED)   copy_data<DIM_INT16>   ( img, plhs );
  else
  if (img.depth()==32 && img.pixelType()==D_FMT_SIGNED)   copy_data<DIM_INT32>   ( img, plhs );
  else // FLOAT
  if (img.depth()==32 && img.pixelType()==D_FMT_FLOAT)    copy_data<DIM_FLOAT32> ( img, plhs );
  else
  if (img.depth()==64 && img.pixelType()==D_FMT_FLOAT)    copy_data<DIM_FLOAT64> ( img, plhs );

  //-----------------------------------------------------------
  // create output meta-data
  //-----------------------------------------------------------

  // if need to return format
  if (nlhs > 1) {
    char *output_buf = (char*) mxCalloc( fmt_name.size()+1, sizeof(char) );
    output_buf[fmt_name.size()] = 0;
    memcpy( output_buf, fmt_name.c_str(), fmt_name.size() );
    plhs[1] = mxCreateString(output_buf);
  }

  // if need to return num_pages
  if (nlhs > 2) {
    plhs[2] = mxCreateDoubleMatrix(1, 1, mxREAL);  
    double *v = mxGetPr(plhs[2]);
    *v = num_pages;
  }

  // if need to return xyzres
  if (nlhs > 3) {
    plhs[3] = mxCreateDoubleMatrix(3, 1, mxREAL);  
    double *v = mxGetPr(plhs[3]);
    v[0] = pixel_size[0];
    v[1] = pixel_size[1];
    v[2] = pixel_size[2];
  }

  // if need to return format
  if (nlhs > 4) {
    char *output_buf = (char*) mxCalloc( meta_data_text.size()+1, sizeof(char) );
    output_buf[meta_data_text.size()] = 0;
    memcpy( output_buf, meta_data_text.c_str(), meta_data_text.size() );
    plhs[4] = mxCreateString(output_buf);
  }
}


extern "C" {
  //--------------------------------------------------------------
  // mexFunction - Entry point from Matlab. From this C function,
  //   simply call the C++ application function, above.
  //--------------------------------------------------------------
  void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray  *prhs[] )
  {
    mex_bimread(nlhs, plhs, nrhs, prhs);
  }
}