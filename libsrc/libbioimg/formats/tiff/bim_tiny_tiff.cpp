/*****************************************************************************
  TINY TIFF READER - an extension for libtiff to read any tags as is
  Copyright (c) 2004-2012 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION
  
  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
    
  History:
    03/29/2004 22:23:00 - First creation
    09/28/2005 23:10 - fixed bug in swabData   
    2012-10-10 11:08:18 - Rewrite and support for BigTIFF
    2013-09-23 12:23:00 - Lazy parsing of directory IFDs, faster for very large files
        
  Ver : 4
*****************************************************************************/

#include "bim_tiny_tiff.h"

#include <xtypes.h>

using namespace TinyTiff;


//-------------------------------------------------------------------------------------
// TIFF in-memory reading
//-------------------------------------------------------------------------------------

tsize_t MemoryStream::read(tdata_t buffer, tsize_t _size) {
    _size = bim::min<tsize_t>(_size, this->sz - this->pos);
    if (_size > 0) {
        memcpy(buffer, this->data + this->pos, _size);
        this->pos += _size;
    }
    return _size;
}

tsize_t MemoryStream::write(tdata_t buffer, tsize_t _size) {
    _size = bim::min<tsize_t>(_size, this->sz - this->pos);
    if (_size > 0) {
        memcpy(this->data + this->pos, buffer, _size);
        this->pos += _size;
    }
    return _size;
}

toff_t MemoryStream::seek(toff_t pos, int whence) {
    unsigned int offset = pos;
    if (whence == SEEK_CUR)
        offset = this->pos + pos;
    else if (whence == SEEK_END)
        offset = this->sz - pos;
    this->pos = bim::trim<toff_t>(offset, 0, this->sz - 1);
    return this->pos;
}

toff_t MemoryStream::size() {
    return this->sz;
}

// libtiff io functions

tsize_t TinyTiff::mem_read(thandle_t st, tdata_t buffer, tsize_t size) {
    MemoryStream *s = (MemoryStream *) st;
    if (!s) return 0;
    return s->read(buffer, size);
};

tsize_t TinyTiff::mem_write(thandle_t st, tdata_t buffer, tsize_t size) {
    MemoryStream *s = (MemoryStream *)st;
    if (!s) return 0;
    return s->write(buffer, size);
};

int TinyTiff::mem_close(thandle_t) {
    return 0;
};

toff_t TinyTiff::mem_seek(thandle_t st, toff_t pos, int whence) {
    MemoryStream *s = (MemoryStream *)st;
    if (!s) return 0;
    return s->seek(pos, whence);
};

toff_t TinyTiff::mem_size(thandle_t st) {
    MemoryStream *s = (MemoryStream *)st;
    if (!s) return 0;
    return s->size();
};

int TinyTiff::mem_map(thandle_t, tdata_t*, toff_t*) {
    return 0;
};

void TinyTiff::mem_unmap(thandle_t, tdata_t, toff_t) {
    return;
};

//-------------------------------------------------------------------------------------
// misc
//-------------------------------------------------------------------------------------

inline void swabData(int type, uint64 size, void* data, bool needswab) {
    if (!needswab) return;
  
    if ( (type == TIFF_SHORT) || (type == TIFF_SSHORT) )
        TIFFSwabArrayOfShort( (uint16*) data, size/2 );
    else
    if ( (type == TIFF_LONG) || (type == TIFF_SLONG) || (type == TIFF_FLOAT) )
        TIFFSwabArrayOfLong( (uint32*) data, size/4 );
    else
    if (type == TIFF_RATIONAL)
        TIFFSwabArrayOfLong( (uint32*) data, size/4 );
    else
    if (type == TIFF_DOUBLE)
        TIFFSwabArrayOfDouble( (double*) data, size/8 );
    else
    if (type==TIFF_LONG8 || type==TIFF_SLONG8 || type==TIFF_IFD8)
        TIFFSwabArrayOfLong8( (uint64*) data, size/8 );
}

//-------------------------------------------------------------------------------------
// Entry
//-------------------------------------------------------------------------------------

void Entry::read(TIFF *tif, bool needswab) {
    unsigned char buffer[20]; // Tiff entry is 12 bytes, BigTiff entry is 20 bytes
    unsigned int size = (tif->tif_flags & TIFF_BIGTIFF) ? 20 : 12;
    init();
    thandle_t h = tif->tif_fd != 0 ? (thandle_t)tif->tif_fd : (thandle_t)tif->tif_clientdata;
    if (tif->tif_readproc(h, buffer, size) < size) return;

    if (needswab) {
        if (tif->tif_flags & TIFF_BIGTIFF) {
		        TIFFSwabShort( (uint16*) &buffer[0] );
		        TIFFSwabShort( (uint16*) &buffer[2] );
		        TIFFSwabLong8( (uint64*) &buffer[4] );
		        TIFFSwabLong8( (uint64*) &buffer[12] );
        } else {
		        TIFFSwabShort( (uint16*) &buffer[0] );
		        TIFFSwabShort( (uint16*) &buffer[2] );
		        TIFFSwabLong ( (uint32*) &buffer[4] );
		        TIFFSwabLong ( (uint32*) &buffer[8] );
        }
    }

    if (tif->tif_flags & TIFF_BIGTIFF) {
        tag    = *(uint16*)(&buffer[0]);
        type   = *(uint16*)(&buffer[2]);
        count  = *(uint64*)(&buffer[4]);
        offset = *(uint64*)(&buffer[12]);
    } else {
        tag    = *(uint16*)(&buffer[0]);
        type   = *(uint16*)(&buffer[2]);
        count  = *(uint32*)(&buffer[4]);
        offset = *(uint32*)(&buffer[8]);
    }
}



//-------------------------------------------------------------------------------------
// IFD
//-------------------------------------------------------------------------------------

void IFD::read(toff_t offset) {
    init();
    thandle_t h = tif->tif_fd != 0 ? (thandle_t)tif->tif_fd : (thandle_t)tif->tif_clientdata;
    tif->tif_seekproc(h, offset, SEEK_SET);

    // read count of tags in the IFD
    if (tif->tif_flags & TIFF_BIGTIFF) {
        if (tif->tif_readproc(h, &count, 8) < 8) return;
        if (needswab) 
            TIFFSwabLong8(&count);
    } else {
        uint16 countsm;
        if (tif->tif_readproc(h, &countsm, 2) < 2) return;
        if (needswab)
            TIFFSwabShort(&countsm);
        count = countsm;
    }

    // read tags
    entries.clear();
    entries.reserve(count);
    for (unsigned int i=0; i<count; i++) {
        Entry entry(tif, needswab);
        if (!entry.isValid()) return;
        entries.push_back(entry);
    }

    // read offset to the next IFD
    if (tif->tif_flags & TIFF_BIGTIFF) {
        if (tif->tif_readproc(h, &next, 8) < 8) return;
        if (needswab) { TIFFSwabLong8(&next); }
    } else {
        uint32 nextsm;
        if (tif->tif_readproc(h, &nextsm, 4) < 4) return;
        if (needswab) { TIFFSwabLong(&nextsm); }
        next = nextsm;
    }
}

int IFD::tagPosition( uint16 tag ) {
    if (!isValid()) return -1;
    for (unsigned int i=0; i<entries.size(); ++i)
        if (entries[i].tag == tag) return i;
    return -1;
}

Entry* IFD::getTag(uint16 tag) {
    int pos = tagPosition(tag);
    if (pos<0) return NULL;
    return &entries[pos];
}

int IFD::readBufNoAlloc (toff_t offset, uint64 size, uint16 type, uint8 *buf) {
  if (!buf) return 1;
  thandle_t h = tif->tif_fd != 0 ? (thandle_t)tif->tif_fd : (thandle_t)tif->tif_clientdata;
  tif->tif_seekproc(h, offset, SEEK_SET);
  if (tif->tif_readproc(h, buf, size) < (tmsize_t)size) return 1;
  swabData(type, size, buf, needswab);
  return 0;
}

void IFD::readBuf (toff_t offset, uint64 size, uint16 type, uint8 **buf) {
    //if (*buf) _TIFFfree( *buf );
    *buf = (uint8*) _TIFFmalloc( size );
    if (this->readBufNoAlloc (offset, size, type, *buf) != 0) {
        _TIFFfree( *buf );
        *buf = NULL;
    }
}

// this function reads tif tag using IFD values
void IFD::readTag (uint16 tag, uint64 &size, uint16 &type, uint8 **buf) {
    size = 0; type = 0;
    Entry* entry = this->getTag(tag);
    if (!entry) return;

    type = entry->type;
    size = entry->count * tag_size_bytes[entry->type];

    if ((tif->tif_flags & TIFF_BIGTIFF) && size<=4 || size<=8) { // if tag contain data instead of offset
        if (*buf != NULL) _TIFFfree( *buf );
        *buf = (uint8*) _TIFFmalloc( size );
        _TIFFmemcpy(*buf, &entry->offset, size);
        swabData(type, size, *buf, needswab);
    } else { // if tag contain offset
        this->readBuf ( entry->offset, size, type, buf);
        if (!*buf) size = 0;
    }
}

// this function reads tif tag using provided size and type instead of IFD values
void IFD::readTagCustom (uint16 tag, uint64 size, uint16 type, uint8 **buf) {
    Entry* entry = this->getTag(tag);
    if (!entry) return;
    this->readBuf ( entry->offset, size, type, buf);
}

void IFD::readTag (uint16 tag, std::vector<uint8> *v) {
    if (!this->tagPresent(tag )) return;
    uint8 *buf = NULL; uint64 size; uint16 type;
    this->readTag (tag, size, type, &buf);
    v->resize(size);
    memcpy( &v->at(0), buf, size );
    _TIFFfree(buf);
}

std::string IFD::readTagString(uint16 tag) {
    std::string s;
    if (!this->tagPresent(tag)) return s;

    std::vector<uint8> buf;
    this->readTag(tag, &buf);
    uint64 size = buf.size();
    while (size > 0 && buf[size-1] == 0) // remove trailing zeros
        --size;
    s.resize(size + 1, '\0');
    memcpy(&s[0], &buf[0], size);
    return s;
}

int IFD::readTagInt(uint16 tag) {
    if (!this->tagPresent(tag)) return 0;
    std::vector<uint8> buf;
    this->readTag(tag, &buf);
    
    uint64 size = buf.size();
    Entry *tt = this->getTag(tag);
    
    if (tt->type == TIFF_SHORT && size >= 2)
        return *(short*)&buf[0];
    else if (tt->type == TIFF_LONG && size >= 4)
        return *(long*)&buf[0];

    return 0;
}

//-------------------------------------------------------------------------------------
// IFDs
//-------------------------------------------------------------------------------------

void Tiff::read(TIFF *tif) {
    this->tif = tif;
    init();
    thandle_t h = tif->tif_fd != 0 ? (thandle_t) tif->tif_fd : (thandle_t)tif->tif_clientdata;
    tif->tif_seekproc(h, 0, SEEK_SET);
    
    uint64 ifd_offset=8;
    if (tif->tif_flags & TIFF_BIGTIFF) {
        HeaderBig hdr;
        if (tif->tif_readproc(h, &hdr, sizeof(hdr)) < (int) sizeof(hdr)) return;
        hdr.magic == TIFF_BIGENDIAN ? needswab = static_cast<bool>(!bigendian) : needswab = static_cast<bool>(bigendian);
        if (needswab) {
            TIFFSwabShort(&hdr.version);
            TIFFSwabLong8(&hdr.diroffset);
        }
        if (hdr.version != 0x002B) return;
        ifd_offset = hdr.diroffset;
    } else {
        Header hdr;
        if (tif->tif_readproc(h, &hdr, sizeof(hdr)) < (int) sizeof(hdr)) return;
        hdr.magic == TIFF_BIGENDIAN ? needswab = static_cast<bool>(!bigendian) : needswab = static_cast<bool>(bigendian);
        if (needswab) {
            TIFFSwabShort(&hdr.version);
            TIFFSwabLong(&hdr.diroffset);
        }
        if (hdr.version != 0x002A) return;
        ifd_offset = hdr.diroffset;
    }
  
    IFD ifd(tif, ifd_offset, needswab);
    ifds.push_back(ifd);
    // delay reading all ifds untile they are actually needed
    /*
    while (ifd.next != 0) {
        ifd = IFD(tif, ifd.next, needswab);
        ifds.push_back(ifd);
    }*/
}

bool Tiff::tagPresentInFirstIFD ( uint16 tag ) {
    if (ifds.size()<=0) return false;
    return ifds[0].tagPresent(tag);
}

IFD* Tiff::getIfd(uint32 i) { 
    if (this->ifds.size()>=i) 
        return &this->ifds[i];
    
    uint32 ii=this->ifds.size()-1;
    IFD ifd = this->ifds[ii];
    while (ifd.next!=0 && ii<=i) {
        ifd = IFD(this->tif, ifd.next, needswab);
        ifds.push_back(ifd);
        ++ii;
    }
    if (this->ifds.size()>=i) 
        return &this->ifds[i];
    else
        return NULL;
}

