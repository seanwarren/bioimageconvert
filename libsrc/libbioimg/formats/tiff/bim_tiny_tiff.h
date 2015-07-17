/*****************************************************************************
  TINY TIFF READER - an extension for libtiff to read any tags as is
  Copyright (c) 2004-2012 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23:00 - First creation
    2012-10-10 11:08:18 - Rewrite and support for BigTIFF
    2013-09-23 12:23:00 - Lazy parsing of directory IFDs, faster for very large files
        
  Ver : 4
*****************************************************************************/

#ifndef BIM_TINY_TIFF_H
#define BIM_TINY_TIFF_H

#include <tiffio.h>
#include <tiffiop.h>

#include <vector>
#include <string>

namespace TinyTiff {

// test if the current platform is bigendian
static int one = 1;
static int bigendian = (*(char *)&one == 0);

// tiff type sizes in bytes 
static const int tag_size_bytes[19] = { 1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 0, 0, 0, 8, 8, 8 }; 

// headers must be POD-types, they are loaded from disk as is!
typedef struct {
  uint16 magic;     // 0x4D4D constan 
  uint16 version;   // 0x002A version = standard TIFF
  uint32 diroffset; // offset to first directory
} Header;

// headers must be POD-types, they are loaded from disk as is!
typedef struct {
  uint16 magic;     // 0x4D4D constan 
  uint16 version;   // 0x002B version = BigTIFF
  uint16 bytesize;  // 0x0008 bytesize of offsets
  uint16 constant;  // 0x0000 constant
  uint64 diroffset; // offset to first directory
} HeaderBig;


// unified entry for Tiff and BigTiff, reads both and creates BigTIFF entry in memory
class Entry {
public:
    Entry(TIFF *tif, bool needswab) { read(tif, needswab); }
    void init() { tag=0; type=0; count=0; offset=0; } 
    void read(TIFF *tif, bool needswab);
    bool isValid() const { return tag>0 && count>0; }

public:
    uint16 tag;    // tag identifying information for entry
    uint16 type;   // data type of entry (for BigTIFF 17 types are defined, 0-13, 16-18)
    uint64 count;  // count of elements for entry, in TIFF stored as uint32
    uint64 offset; // data itself (if <= 64-bits) or offset to data for entry, in TIFF stored as uint32
    
    friend class IFD;
};

// unified IFD for Tiff and BigTiff, reads both and creates BigTIFF IFD in memory
class IFD {
public:
    IFD(TIFF *tif, toff_t offset, bool needswab) { this->tif = tif; this->needswab=needswab; read(offset); }
    void init() { count=0; next=0; entries.clear(); } 

    bool   isValid()  const { return count>0 && entries.size()==count; }
    size_t size()     const { return entries.size(); }
    bool   needSwab() const { return needswab; }
    
    int  tagPosition( uint16 tag );
    bool tagPresent ( uint16 tag ) { return tagPosition(tag)>=0; }
    void read(toff_t offset);

    Entry* getEntry(size_t pos) { if (pos>=entries.size()) return NULL; return &entries[pos]; }
    Entry* getTag(uint16 tag);
    int64  tagOffset(uint16 tag) { Entry *e = getTag(tag); return e!=NULL?e->offset:-1; }
    int64  tagCount(uint16 tag) { Entry *e = getTag(tag); return e!=NULL?e->count:-1; }

    // reads to buffer data of size in bytes from determant offset and do necessary convertion
    void readBuf (toff_t offset, uint64 size, uint16 type, uint8 **buf);
    int  readBufNoAlloc (toff_t offset, uint64 size, uint16 type, uint8 *buf);
    
    std::string readTagString (uint16 tag);
    void readTag (uint16 tag, std::vector<uint8> *buf);
    void readTag (uint16 tag, uint64 &size, uint16 &type, uint8 **buf);
    // this function reads tif tag using provided size and type instead of IFD values
    void readTagCustom (uint16 tag, uint64 size, uint16 type, uint8 **buf);

private:
    uint64 count; // number of directory entries, uint16 in TIFF and uint64 in BigTIFF
    std::vector<Entry> entries;
    uint64 next;  // offset to next directory or zero, uint32 in TIFF and uint64 in BigTIFF
    
    TIFF *tif;
    bool needswab;
    friend class Tiff;
};


// unified IFDs for Tiff and BigTiff, reads both and creates BigTIFF IFD in memory
class Tiff {
public:
    void   init() { ifds.clear(); needswab=false; }
    void   read(TIFF *tif);

    IFD*   firstIfd() { return ifds.size()>0?&ifds[0]:NULL; }
    IFD*   getIfd(uint32 i);// { return ifds.size()<i?&ifds[i]:NULL; }

    bool   isValid()  const { return ifds.size()>0; }
    bool   needSwab() const { return needswab; }
    //size_t count()    const { return ifds.size(); } // because we are lazy it is harder to know the true ifd count
    bool   tagPresentInFirstIFD ( uint16 tag );

private:
    std::vector<IFD> ifds;
    TIFF *tif;
    bool needswab;
};


} // namespace TinyTiff

#endif //BIM_TINY_TIFF_H
