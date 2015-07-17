/*****************************************************************************
 Cached blob memory access

 IMPLEMENTATION

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2006 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   11/07/2006 17:41 - First creation

 Ver : 1
*****************************************************************************/

#include "blob_manager.h"

using namespace bim;

void CCacheBlob::init() {
  _size = 0;
  _buffer = NULL;
  _cache_address = 0;
  _cached = false;
  _locked = false;
}

bool CCacheBlob::store(std::ofstream *fs) {
  if (fs == NULL) return -1;
  for (unsigned int i=0; i<this->size(); ++i) {
    T v = (*this)[i];
    fs->write((char*)&v, sizeof(T));
    if (fs->fail()) return -1; 
  }
  return (int) this->size() * sizeof(T);
}

bool CCacheBlob::retrieve(std::ifstream *fs) {
  if (fs == NULL) return -1;
  for (unsigned int i=0; i<this->size(); ++i) {
    T v; 
    fs->read((char*)&v, sizeof(T));
    if (fs->fail()) return -1; 
    (*this)[i] = v;
  }
  return (int) this->size() * sizeof(T);
}



