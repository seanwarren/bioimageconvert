/*******************************************************************************

  Simple dynamic memory buffer
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    10/20/2006 20:21 - First creation
      
  ver: 1
        
*******************************************************************************/

#include "bim_buffer.h"

DimBuffer::DimBuffer( unsigned int size ) {
  buf = 0;
  buf_size = 0;
  allocate( size );
}

DimBuffer::DimBuffer( unsigned int size, unsigned char fill_byte ) {
  buf = 0;
  buf_size = 0;
  allocate( size );
  fill( fill_byte );
}

DimBuffer::~DimBuffer() {
  free();
}

bool DimBuffer::allocate( unsigned int size ) {
  if (size == buf_size) return true;
  free();
  if (size <= 0) return true;
  buf = new unsigned char [size];
  if (buf != 0)
    buf_size = size;
  else
    buf_size = 0;
  return (buf != 0);
}

void DimBuffer::free( ) {
  if (buf != 0) delete [] buf;
  buf = 0;
  buf_size = 0;
}

void DimBuffer::fill( unsigned char fill_byte ) {
  if (size()==0) return;
  for (unsigned int i=0; i<buf_size; ++i) 
    buf[i] = fill_byte;
}


