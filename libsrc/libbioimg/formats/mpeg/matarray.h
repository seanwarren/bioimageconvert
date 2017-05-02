#ifndef MATARRAY_H
#define MATARRAY_H

// $Date: 2008-11-29 20:58:23 -0500 (Sat, 29 Nov 2008) $
// $Revision: 713 $

/*
videoIO: granting easy, flexible, and efficient read/write access to video 
                 files in Matlab on Windows and GNU/Linux platforms.
    
Copyright (c) 2006 Gerald Dalley
  
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

    Portions of this software link to code licensed under the Gnu General 
    Public License (GPL).  As such, they must be licensed by the more 
    restrictive GPL license rather than this MIT license.  If you compile 
    those files, this library and any code of yours that uses it automatically
    becomes subject to the GPL conditions.  Any source files supplied by 
    this library that bear this restriction are clearly marked with internal
    comments.

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#include <vector>
#include <limits>
#include "debug.h"
#ifdef MATLAB_MEX_FILE
#  include <mex.h>
#  include <matrix.h>
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

/* 
 * This header file contains a quick-and-dirty implementation of a C++ 
 * container (MatArray) for data in the form of Matlab's mxArray.  MatArray
 * allows C++ programs to manipulate simple Matlab-style arrays without
 * needing to include the Matlab header files and without needing to link
 * to the Matlab binary libraries.  Dense numeric arrays and cell arrays
 * are supported.  Sparse matrices, structs, and other Matlab data types
 * are not (yet) supported.
 */

namespace VideoIO {

  //------ Cross-platform typedefs -------------------------------------------

  typedef signed   char      int8;
  typedef unsigned char      uint8;
  typedef signed   short     int16;
  typedef unsigned short     uint16;
  typedef signed   int       int32;
  typedef unsigned int       uint32;
#if defined(WIN32) || defined(WIN64)
  typedef __int64            int64;
  typedef unsigned __int64   uint64;
#else
  typedef long long          int64;
  typedef unsigned long long uint64;
#endif

  //------ mxArray type constants --------------------------------------------

  // Format specification:
  //  http://www.mathworks.com/access/helpdesk/help/pdf_doc/matlab/matfile_format.pdf
  class MatDataTypeConstants {
  public:
    static const uint8 mxNONE_CLASS    = 0; // NOT an official type
    static const uint8 mxCELL_CLASS    = 1;
    //static const uint8 mxLOGICAL_CLASS = 3; // not defined because this one has changed over time
    static const uint8 mxCHAR_CLASS    = 4;
    static const uint8 mxDOUBLE_CLASS  = 6;
    static const uint8 mxSINGLE_CLASS  = 7;
    static const uint8 mxINT8_CLASS    = 8;
    static const uint8 mxUINT8_CLASS   = 9;
    static const uint8 mxINT16_CLASS   = 10;
    static const uint8 mxUINT16_CLASS  = 11;
    static const uint8 mxINT32_CLASS   = 12;
    static const uint8 mxUINT32_CLASS  = 13;
    static const uint8 mxINT64_CLASS   = 14; // Matlab 7+ only (?)
    static const uint8 mxUINT64_CLASS  = 15; // Matlab 7+ only (?)

    inline static size_t elmSize(uint8 matMxType) {
      switch(matMxType) {
      case mxCELL_CLASS:    return sizeof(void*);  break;
      //case mxLOGICAL_CLASS: return sizeof(uint8);  break;
      case mxCHAR_CLASS:    return sizeof(uint16); break;  
      case mxDOUBLE_CLASS:  return sizeof(double); break;
      case mxSINGLE_CLASS:  return sizeof(float);  break;
      case mxINT8_CLASS:    return sizeof(int8);   break;  
      case mxUINT8_CLASS:   return sizeof(uint8);  break; 
      case mxINT16_CLASS:   return sizeof(int16);  break; 
      case mxUINT16_CLASS:  return sizeof(uint16); break;
      case mxINT32_CLASS:   return sizeof(int32);  break; 
      case mxUINT32_CLASS:  return sizeof(uint32); break;
      case mxINT64_CLASS:   return sizeof(int64);  break; 
      case mxUINT64_CLASS:  return sizeof(uint64); break;
      default: VrRecoverableThrow("Unsupported matrix class (" << 
                                  (int)matMxType << ")");
      }
    }    

    inline static const char *name(uint8 matMxType) {
#define NC(n) case n: return #n; break
      switch(matMxType) {
        NC(mxCELL_CLASS);
        //NC(mxLOGICAL_CLASS);
        NC(mxCHAR_CLASS);
        NC(mxDOUBLE_CLASS);
        NC(mxSINGLE_CLASS);
        NC(mxINT8_CLASS);  
        NC(mxUINT8_CLASS); 
        NC(mxINT16_CLASS); 
        NC(mxUINT16_CLASS);
        NC(mxINT32_CLASS); 
        NC(mxUINT32_CLASS);
        NC(mxINT64_CLASS); 
        NC(mxUINT64_CLASS);
      default: return "UnsupportedClass";
      }
#undef NC
    }
  };

  //------ C++ type to mxArray type constants --------------------------------

  template <class T> class MatType {
  public:
    /// Returns the Matlab class ID / storate type for a standard matrix of 
    /// type T.  Corresponds to the mxXXX constants.
    static uint8 mx() { return 0; }
  };

  template <> class MatType<int8  >  { public: static uint8 mx() { return MatDataTypeConstants::mxINT8_CLASS;   } };
  template <> class MatType<uint8 >  { public: static uint8 mx() { return MatDataTypeConstants::mxUINT8_CLASS;  } };
  template <> class MatType<int16 >  { public: static uint8 mx() { return MatDataTypeConstants::mxINT16_CLASS;  } };
  template <> class MatType<uint16>  { public: static uint8 mx() { return MatDataTypeConstants::mxUINT16_CLASS; } };
  template <> class MatType<int32 >  { public: static uint8 mx() { return MatDataTypeConstants::mxINT32_CLASS;  } };
  template <> class MatType<uint32>  { public: static uint8 mx() { return MatDataTypeConstants::mxUINT32_CLASS; } };
  template <> class MatType<int64 >  { public: static uint8 mx() { return MatDataTypeConstants::mxINT64_CLASS;  } };
  template <> class MatType<uint64>  { public: static uint8 mx() { return MatDataTypeConstants::mxUINT64_CLASS; } };
  template <> class MatType<float >  { public: static uint8 mx() { return MatDataTypeConstants::mxSINGLE_CLASS; } };
  template <> class MatType<double>  { public: static uint8 mx() { return MatDataTypeConstants::mxDOUBLE_CLASS; } };

  //------ MatArray class ----------------------------------------------------

  /////////////////////////////////////////////////////////////////////////////
  // Our wrapper class for Matlab-style data.  It exists so that we can avoid
  // linking to any matlab libraries when marshalling matlab-style data around.
  class MatArray {
  public:
    inline MatArray(MatArray const *other);
    inline MatArray(uint8 mx, int nrows, int ncols);
    inline MatArray(uint8 mx, std::vector<int> const &dims);
    inline virtual ~MatArray() { freeData(); }

#ifdef MATLAB_MEX_FILE
    inline MatArray(mxArray const *mat);
    inline void transferToMat(mxArray *&mat);
#endif    

    inline uint8                   mx()     const { return _mx; }
    inline std::vector<int> const &dims()   const { return _dims; }
    inline size_t                  numElm() const;
    inline void const             *data()   const { return _data; }
    inline void                   *data()         { return _data; }

  private:
    inline void allocData();
    inline void freeData();

    uint8             _mx;
    std::vector<int>  _dims;
    void             *_data;
  };
  /////////////////////////////////////////////////////////////////////////////

  //------ MatArray auto-deleting container ----------------------------------

  class MatArrayVector : public std::vector<MatArray*> {
  public:
    virtual ~MatArrayVector() {
      for (size_t i=0; i<size(); i++) {
        delete (*this)[i];
      }
    }
  };

  //------ MatArray conversion functions -------------------------------------

  inline std::string mat2string(MatArray const *m) {
    TRACE;
    VrRecoverableCheckMsg(m->mx() == MatDataTypeConstants::mxCHAR_CLASS, 
                          "The MatArray must be a character array if it is "
                          "being converted to a string.");
    std::string s;
    for (size_t i=0; i<m->numElm(); i++) {
      s.push_back((char)((int16*)m->data())[i]);
    }
    return s;
  }

  template <class T>
  inline T mat2scalar(MatArray const *m) {
    TRACE;
    if (m->mx() != MatType<T>::mx()) {
      if (m->mx() == MatDataTypeConstants::mxCHAR_CLASS) {
        VrRecoverableThrow("Data type mismatch: " << " expected " <<
                           MatDataTypeConstants::name(MatType<T>::mx()) <<
                           ", but a string \"" << mat2string(m) << "\" "
                           "was found instead.");
      } else {
        VrRecoverableThrow("Data type mismatch: " << " expected " << 
                           MatDataTypeConstants::name(MatType<T>::mx()) <<
                           ", but " << MatDataTypeConstants::name(m->mx()) <<
                           " was found instead.");
      }
    }
    VrRecoverableCheckMsg(m->numElm() == 1, 
                          "Expected a scalar variable, not a matrix");
    return *(T*)m->data();
  }

  template <class T>
  inline std::auto_ptr<MatArray> scalar2mat(T val) {
    TRACE;
    std::auto_ptr<MatArray> mat(new MatArray(MatType<T>::mx(), 1, 1));
    *((T*)mat->data()) = val;
    return mat;
  }

  inline std::auto_ptr<MatArray> string2mat(std::string const &s) {
    TRACE;
    VrRecoverableCheck(s.size() < std::numeric_limits<int>::max());
    std::auto_ptr<MatArray> mat(new MatArray(MatDataTypeConstants::mxCHAR_CLASS,
                                             1, (int)s.size()));
    for (size_t i=0; i<s.size(); i++) {
      ((int16*)mat->data())[i] = s[i];
    }
    return mat;
  }

  //------ MatArray implementation -------------------------------------------

  MatArray::MatArray(MatArray const *other) :
    _mx(other->_mx), _dims(other->_dims), _data(NULL)
  { 
    TRACE;
    allocData();
    if (_mx == MatDataTypeConstants::mxCELL_CLASS) {
      for (size_t i=0; i<numElm(); i++) {
        ((MatArray**)_data)[i] = new MatArray(((MatArray**)other->data())[i]);
      }
    } else {
      size_t sz = numElm() * MatDataTypeConstants::elmSize(mx());
      PRINTINFO("Copying " << sz << " bytes from " << other->_data << 
                " to " << _data);
      memcpy(_data, other->_data, sz);
    }
  }

  MatArray::MatArray(uint8 mx, int nrows, int ncols) :
    _mx(mx), _data(NULL)
  { TRACE; _dims.resize(2); _dims[0] = nrows; _dims[1] = ncols; allocData(); }

  MatArray::MatArray(uint8 mx, std::vector<int> const &dims) :
    _mx(mx), _dims(dims), _data(NULL)
  { TRACE; allocData(); }

#ifdef MATLAB_MEX_FILE
  // Creates an alias to a Matlab matrix
  MatArray::MatArray(mxArray const *mat) :
    _mx(mxGetClassID(mat)), _dims(), _data(NULL)
  {
    TRACE;
    VrRecoverableCheck(!mxIsComplex(mat) && !mxIsSparse(mat));
    VrRecoverableCheck(mxIsNumeric(mat) || mxIsChar(mat) || mxIsCell(mat));

    _dims.resize(mxGetNumberOfDimensions(mat));
    int const *matDims = mxGetDimensions(mat);
    for (size_t i=0; i<_dims.size(); i++) {
      _dims[i] = matDims[i];
    }

    allocData();

    const size_t nElm = mxGetNumberOfElements(mat);
    VrRecoverableCheck(nElm == numElm());
    if (mxIsCell(mat)) {
      VrRecoverableCheck(nElm < std::numeric_limits<int>::max());
      for (size_t i=0; i<nElm; i++) {
        ((MatArray**)_data)[i] = new MatArray(mxGetCell(mat, (int)i));
      }
    } else {
      size_t sz = nElm * MatDataTypeConstants::elmSize(mx());
      void *matData = mxGetPr(mat);
      PRINTINFO("Copying " << sz << " bytes from " << matData << 
              " to " << _data);
      memcpy(_data, matData, sz);
    }
  } 

  void MatArray::transferToMat(mxArray *&mat) { // TODO: change to "mxArray *MatArray::toMat()" 
    TRACE;
    if (mx() == MatDataTypeConstants::mxCELL_CLASS) {
      // cell arrays contain embedded mxArrays, so we must do the 
      // transformation here.
      VrRecoverableCheck(_dims.size() < std::numeric_limits<int>::max());
      mat = mxCreateCellArray((int)_dims.size(), &_dims[0]);
      const size_t N = numElm();
      MatArray **src = (MatArray **)_data;
      VrRecoverableCheck(N < std::numeric_limits<int>::max());
      for (size_t i=0; i<N; i++) {
        mxArray *tmp;
        src[i]->transferToMat(tmp);
        mxSetCell(mat, (int)i, tmp);
      }
    } else {
      // Avoid the calloc penalty 
      // (see http://ioalinux1.epfl.ch/~mleutene/MATLABToolbox/CmexWrapper.html)
      mat = mxCreateNumericArray(0, 0, (mxClassID)_mx, mxREAL);
      VrRecoverableCheck(_dims.size() < std::numeric_limits<int>::max());
      VrRecoverableCheck(mxSetDimensions(mat, &_dims[0], (int)_dims.size()) == 0);
      mxSetPr(mat, (double*)_data);
      _data = NULL;
    }
    freeData(); 
  }
#endif

  void MatArray::allocData() {
    TRACE;
    std::vector<int> dimsBackup(_dims);
    freeData();
    _dims = dimsBackup;

    const size_t sz = numElm() * MatDataTypeConstants::elmSize(mx());
    PRINTINFO("allocating " << sz << " bytes for " << numElm() <<" elements.");

    if (sz > 0) {
#ifdef MATLAB_MEX_FILE
      VrRecoverableCheck((_data = mxMalloc(sz)) != NULL);
#else
      VrRecoverableCheck((_data = malloc(sz)) != NULL);
#endif
    }
  }

  void MatArray::freeData() {
    TRACE;
    if (_data) {
      if (mx() == MatDataTypeConstants::mxCELL_CLASS) {
        MatArray **a = (MatArray**)_data;
        for (size_t i=0; i<numElm(); i++) {
          delete *a++;
        }
      }
#ifdef MATLAB_MEX_FILE
      mxFree(_data);
#else
      free(_data);
#endif      
      _data = NULL;
    } 
    _dims.resize(0);
  }

  size_t MatArray::numElm() const {
    size_t n = 1;
    for (size_t i=0; i<_dims.size(); i++) {
      n *= _dims[i];
    }
    return n;
  }

}; /* namespace VideoIO */

#endif
