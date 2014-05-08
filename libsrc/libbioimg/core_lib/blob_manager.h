/*****************************************************************************
 Cached blob memory access

 DEFINITION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2006 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   11/07/2006 17:41 - First creation

 Ver : 1
*****************************************************************************/

#ifndef BIM_CACHE_BLOB_MANAGER
#define BIM_CACHE_BLOB_MANAGER

namespace bim {

using namespace std;
/*

class CCacheBlob {
  static uint _num_blobs;
public:
  CCacheBlob( int id ) { ++_num_blobs; init(); _id = id; }
  ~CCacheBlob() { --_num_blobs; }
  void init();

  bool store ( ofstream *fs );
  bool retrieve ( ifstream *fs );

  // must have anough space to copy data into
  void get_buffer_data ( void *fill_in );
  
  // gives the pointer to the buffer, you MUST lock this memory address
  // while you're accessing the buffer so it would not be chached and removed from memory 
  void *get_buffer_ptr(); 
  void setLock( bool lock );

  bool is_in_memory() { return _buffer != NULL; }
  // is_chached
  // 
private:
  uint        _id;
  uint        _size;
  void*       _buffer;
  uint        _cache_address;
  bool        _cached;
  bool        _locked;
  //uint        _last_access_time;

  // hide copy-constructor
  CCacheBlob(const CCacheBlob& o) {
    ++_num_blobs;
  }

};

uint CCacheBlob::_num_blobs = 0;

//std::list
*/

} // namespace dim

#endif // BIM_CACHE_BLOB_MANAGER
