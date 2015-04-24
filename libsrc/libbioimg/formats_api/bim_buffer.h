/*******************************************************************************

  Simple dynamic memory buffer
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    10/20/2006 20:21 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_BUFFER_H
#define BIM_BUFFER_H

#include <stddef.h>

// simple buffer with storage entity byte, allocates and deallocated memory space
class DimBuffer {
public:
  DimBuffer( size_t size=0 );
  DimBuffer( size_t size, unsigned char fill_byte );
  ~DimBuffer();

  virtual bool allocate( size_t size );
  virtual void fill( unsigned char fill_byte = 0 );
  void free( );

  inline size_t size() const { return buf_size; }
  inline unsigned char *buffer() const { return buf; }
  inline unsigned char *bytes() const { return buf; }
  inline unsigned char &operator[](ptrdiff_t i) const { return *(buf + i); }

private:
  unsigned char *buf;
  size_t buf_size;

};


// typed buffer with storage entity defined by the user
template <typename T>
class DTypedBuffer : public DimBuffer {
public:
  
  DTypedBuffer( size_t size=0 ): DimBuffer( sizeof(T)*size ) { }

  DTypedBuffer( size_t size, const T &fill_value ): DimBuffer( sizeof(T)*size ) {
    fill( fill_value );
  }

  inline T* buffer() const { return (T*) bytes(); }

  bool allocate( size_t size ) {
    return DimBuffer::allocate( sizeof(T)*size );
  }
  
  void fill( const T &fill_value ) {
    if (size()==0) return;
    T *p = (T*) bytes();
    for (size_t i=0; i<size(); ++i) 
      p[i] = fill_value;
  }

  inline size_t size() const { 
    return ( DimBuffer::size()/sizeof(T) ); 
  }

  inline T &operator[](ptrdiff_t i) const { 
    T *p = (T*) bytes();
    return *(p + i); 
  }

};


#endif //BIM_BUFFER_H


