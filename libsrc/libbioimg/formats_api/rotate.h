/*******************************************************************************

  Rotation functions

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2007-07-06 17:02 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_ROTATE_H
#define BIM_ROTATE_H

#include <string>
#include <iostream>
#include <fstream>
#include <limits>
#include <vector>

#include "xtypes.h"
#include "bim_buffer.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

//------------------------------------------------------------------------------------
// Simple Rotate functions, templated type is the pixel data type
//------------------------------------------------------------------------------------

template <typename T>
void image_rotate_right ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

template <typename T>
void image_rotate_left ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

template <typename T>
void image_flip ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );

template <typename T>
void image_mirror ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                        const T *psrc,  unsigned int w_in, unsigned int h_in, unsigned int offset_in );


//************************************************************************************
// IMPLEMENTATION PART
//************************************************************************************

template <typename T>
void image_rotate_right ( T *pdest, unsigned int /*w_to*/, unsigned int /*h_to*/, unsigned int offset_to,
                         const T *psrc, unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {

  register unsigned int x, y;
  register unsigned int xn, yn;
  const T *p = (T*) psrc;
  T *q = (T*) pdest;

  for (y=0; y<h_in; y++) {
    for (x=0; x<w_in; x++) {
      xn = (h_in-1)-y;
      yn = x;
      q = pdest + (offset_to * yn);
      q[xn] = p[x];      
    } // x
    p += offset_in;
  } // y
}


template <typename T>
void image_rotate_left ( T *pdest, unsigned int /*w_to*/, unsigned int h_to, unsigned int offset_to,
                         const T *psrc, unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {

  register unsigned int x, y;
  register unsigned int xn, yn;
  const T *p = (T*) psrc;
  T *q = (T*) pdest;

  for (y=0; y<h_in; y++) {
    for (x=0; x<w_in; x++) {
      xn = y;
      yn = (h_to-1)-x;
      q = pdest + (offset_to * yn);
      q[xn] = p[x];      
    } // x
    p += offset_in;
    //q += offset_to;
  } // y
}

template <typename T>
void image_flip ( T *pdest, unsigned int /*w_to*/, unsigned int h_to, unsigned int offset_to,
                  const T *psrc, unsigned int w_in, unsigned int h_in, unsigned int offset_in ) {

  register unsigned int y;
  const T *p = (T*) psrc + (offset_in * (h_in-1));
  T *q = (T*) pdest;

  for (y=0; y<h_to; y++) {
    memcpy( q, p, sizeof(T)*w_in );  
    q += offset_to;
    p -= offset_in;
  } // y
}

template <typename T>
void image_mirror ( T *pdest, unsigned int w_to, unsigned int h_to, unsigned int offset_to,
                  const T *psrc, unsigned int /*w_in*/, unsigned int /*h_in*/, unsigned int offset_in ) {

  const T *p = (T*) psrc;
  T *q = (T*) pdest;
  DTypedBuffer<T> buf( w_to );

  for (unsigned int y=0; y<h_to; y++) {
    
    T *qt = buf.buffer() + (w_to-1);
    for (unsigned int x=0; x<w_to; x++) {
      *qt = p[x];  
      --qt;
    } // x
    memcpy( q, buf.bytes(), sizeof(T)*w_to );

    q += offset_to;
    p += offset_in;
  } // y
}

#endif //BIM_ROTATE_H
