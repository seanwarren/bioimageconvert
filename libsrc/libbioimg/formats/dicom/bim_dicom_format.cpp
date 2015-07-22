/*****************************************************************************
  DICOM support
  Copyright (c) 2015, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2015, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    2013-01-12 14:13:40 - First creation
        
  ver : 1
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

#include <xtypes.h>
#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_dicom_format.h"

//#include <gdcmReader.h>
//#include <gdcmImageReader.h>
#include <gdcmImageRegionReader.h>
#include <gdcmImageHelper.h>
#include <gdcmBoxRegion.h>

#include <gdcmGlobal.h>
#include <gdcmDicts.h>
#include <gdcmDict.h>
#include <gdcmValue.h>
//#include <gdcmAttribute.h>
//#include <gdcmStringFilter.h>


using namespace bim;

//****************************************************************************
// Misc
//****************************************************************************

bim::DICOMParams::DICOMParams() {
    i = initImageInfo(); 
    //image = NULL;
    file = NULL;
    //reader = new gdcm::ImageReader();
    reader = new gdcm::ImageRegionReader();
}

bim::DICOMParams::~DICOMParams() {
    if (reader) delete reader;
    if (file) delete file;
}

//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_DICOM_MAGIC_SIZE 132

int dicomValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_DICOM_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *) magic;

    if (memcmp(mag_num+128, "DICM", 4) == 0 ) return 0;
    if (memcmp(mag_num, "DICM", 4) == 0) return 0;
    //bool Reader::CanRead() const // gdcm reader based function, requires file name
    return -1;
}

FormatHandle dicomAquireFormatProc( void ) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void dicomCloseImageProc (FormatHandle *fmtHndl);
void dicomReleaseFormatProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    dicomCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

inline bool find_tag(const gdcm::DataSet& ds, uint16_t group, uint16_t element) {
    const gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return false;
    //const gdcm::DataElement& de = ds.GetDataElement(tag);
    //if (de.IsEmpty()) return false;
    return true;
}

inline void set_lut(ImageInfo *info, gdcm::SmartPointer<gdcm::LookupTable> lut, 
                                     gdcm::LookupTable::LookupTableType type_r, 
                                     gdcm::LookupTable::LookupTableType type_g, 
                                     gdcm::LookupTable::LookupTableType type_b) {
    unsigned int num_colors_r = lut->GetLUTLength(type_r);
    unsigned int num_colors_g = lut->GetLUTLength(type_g);
    unsigned int num_colors_b = lut->GetLUTLength(type_b);
    if (num_colors_r == 0) return;
    if (num_colors_r != num_colors_g && num_colors_g != num_colors_g && num_colors_r <= 0) return;
    int sz = bim::min<int>(num_colors_r, 256);
    info->lut.count = sz;

    if (lut->GetBitSample() == 8) {
        const unsigned char *p = lut->GetPointer();
        for (bim::uint i = 0; i < sz; i++) {
            info->lut.rgba[i] = xRGB(p[0], p[1], p[2]);
            p += 3;
        }
    } else if (lut->GetBitSample() == 16) {
        const unsigned short *p = (const unsigned short *)lut->GetPointer();
        for (bim::uint i = 0; i < sz; i++) {
            info->lut.rgba[i] = xRGB(p[0] / 256, p[1] / 256, p[2] / 256);
            p+=3;
        }
    }
}

void dicomGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;  

    *info = initImageInfo();

    gdcm::File & f = par->reader->GetFile();
    //gdcm::Image & img = par->reader->GetImage();

    const gdcm::DataSet& ds = f.GetDataSet();
    bool slice_thickness = find_tag(ds, 0x0018, 0x0050);
    bool slice_spacing   = find_tag(ds, 0x0018, 0x0088);
    bool frame_time      = find_tag(ds, 0x0018, 0x1063);
    bool num_temporal    = find_tag(ds, 0x0020, 0x0105);
    bool number_frames   = find_tag(ds, 0x0028, 0x0008);

    std::vector<unsigned int> dims = gdcm::ImageHelper::GetDimensionsValue(f);
    std::vector<double> spacing = gdcm::ImageHelper::GetSpacingValue(f);

    // some of this might have to be updated while decoding the image
    info->width = dims[0]; // img.GetColumns();
    info->height = dims[1]; // img.GetRows();
    unsigned int nd = dims.size(); //img.GetNumberOfDimensions();
    if (nd == 2) {
        info->number_z = 1;
        info->number_t = 1;
    } else if (nd == 3) {
        if (number_frames || num_temporal || frame_time) {
            info->number_z = 1;
            info->number_t = dims[2]; //img.GetDimension(2);
        } else {
            info->number_z = dims[2]; //img.GetDimension(2);
            info->number_t = 1;
        }
    } else if (nd >= 4) {
        info->number_z = dims[2]; //img.GetDimension(2);
        info->number_t = dims[3]; //img.GetDimension(3);
    }
    
    info->number_pages = info->number_z * info->number_t;

    // set XY scale
    info->resUnits = RES_mm;
    info->xRes = spacing[0]; //img.GetSpacing(0);
    info->yRes = spacing[1]; //img.GetSpacing(1);

    //---------------------------------------------------------------
    // define dims
    //---------------------------------------------------------------
    if (info->number_z>1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_Z;
    }

    if (info->number_t>1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_T;
    }

    if (info->number_z>1 && info->number_t>1) {
        info->number_dims = 5;
        info->dimensions[3].dim = DIM_Z;
        info->dimensions[4].dim = DIM_T;
    }

    //---------------------------------------------------------------
    // define pixels
    //---------------------------------------------------------------
   
    // set pixel depth and format
    //gdcm::PixelFormat pf = img.GetPixelFormat();
    gdcm::PixelFormat pf = gdcm::ImageHelper::GetPixelFormatValue(f);
    info->samples = pf.GetSamplesPerPixel();
    gdcm::PixelFormat::ScalarType st = pf.GetScalarType();
    
    if (st == gdcm::PixelFormat::UINT8) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::INT8) {
        info->depth = 8;
        info->pixelType = FMT_SIGNED;
    } else if (st == gdcm::PixelFormat::UINT12) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::INT12) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    } else if (st == gdcm::PixelFormat::UINT16) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::INT16) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    } else if (st == gdcm::PixelFormat::UINT32) {
        info->depth = 32;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::INT32) {
        info->depth = 32;
        info->pixelType = FMT_SIGNED;
    } else if (st == gdcm::PixelFormat::UINT64) {
        info->depth = 64;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::INT64) {
        info->depth = 64;
        info->pixelType = FMT_SIGNED;
    } else if (st == gdcm::PixelFormat::FLOAT16) {
        info->depth = 16;
        info->pixelType = FMT_FLOAT;
    } else if (st == gdcm::PixelFormat::FLOAT32) {
        info->depth = 32;
        info->pixelType = FMT_FLOAT;
    } else if (st == gdcm::PixelFormat::FLOAT64) {
        info->depth = 64;
        info->pixelType = FMT_FLOAT;
    } else if (st == gdcm::PixelFormat::SINGLEBIT) {
        info->depth = 1;
        info->pixelType = FMT_UNSIGNED;
    } else if (st == gdcm::PixelFormat::UNKNOWN) {
        //info->depth = 16;
        //info->pixelType = FMT_UNSIGNED;
    }

    //---------------------------------------------------------------
    // photometric mode
    //---------------------------------------------------------------

    info->imageMode = IM_GRAYSCALE;
    gdcm::PhotometricInterpretation photometric = gdcm::ImageHelper::GetPhotometricInterpretationValue(f);
    if (photometric == gdcm::PhotometricInterpretation::RGB) {
        info->imageMode = IM_RGB;
    }
    else if (photometric == gdcm::PhotometricInterpretation::ARGB) {
        info->imageMode = IM_RGBA;
    }
    else if (photometric == gdcm::PhotometricInterpretation::PALETTE_COLOR) {
        info->imageMode = IM_INDEXED;
    }

    //---------------------------------------------------------------
    // read LUT
    //---------------------------------------------------------------
    if (photometric == gdcm::PhotometricInterpretation::PALETTE_COLOR) {
        info->imageMode = IM_INDEXED;

        // init LUT
        info->lut.count = 0;
        for (bim::uint i = 0; i<256; i++) {
            info->lut.rgba[i] = xRGB(i, i, i);
        }

        gdcm::SmartPointer<gdcm::LookupTable> lut = gdcm::ImageHelper::GetLUT(f);
        if (lut) {
            unsigned int num_colors_y = lut->GetLUTLength(gdcm::LookupTable::GRAY);
            if (num_colors_y > 0) {
                set_lut(info, lut, gdcm::LookupTable::GRAY, gdcm::LookupTable::GRAY, gdcm::LookupTable::GRAY);
            } else {
                set_lut(info, lut, gdcm::LookupTable::RED, gdcm::LookupTable::GREEN, gdcm::LookupTable::BLUE);
            }
        }
    } // if paletted
}

void dicomCloseImageProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose ( fmtHndl );
    bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint dicomOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) dicomCloseImageProc(fmtHndl);
    bim::DICOMParams *par = new bim::DICOMParams();
    fmtHndl->internalParams = (void *)par;

    if (io_mode == IO_READ) {
        #if defined(BIM_WIN)
        bim::xstring fn(fmtHndl->fileName);
        par->file = new std::fstream((const wchar_t *)fn.toUTF16().c_str(), std::fstream::in | std::fstream::binary);
        par->reader->SetStream(*par->file);
        #else
        par->reader->SetFileName(fmtHndl->fileName);
        #endif       
        try {
            //par->reader->Read();
            if (!par->reader->ReadInformation()) return 1;
            dicomGetImageInfo(fmtHndl);
        } catch (...) {
            dicomCloseImageProc(fmtHndl);
            return 1;
        }
    }
    else return 1;
    return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint dicomGetNumPagesProc ( FormatHandle *fmtHndl ) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    return info->number_pages;
}


ImageInfo dicomGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    fmtHndl->pageNumber = page_num;  
    bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint dicomAddMetaDataProc (FormatHandle *fmtHndl) {
    fmtHndl=fmtHndl;
    return 1;
}


bim::uint dicomReadMetaDataProc (FormatHandle *fmtHndl, bim::uint page, int group, int tag, int type) {
    if (fmtHndl == NULL) return 1;
    return 1;
}

char* dicomReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
    return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out, int stride = 0) {
    T *raw = (T *)in + sample;
    T *p = (T *)out;
    int step = samples;
    size_t inrowsz = stride == 0 ? samples*W : stride;
    size_t ourowsz = W;

    #pragma omp parallel for default(shared)
    for (bim::int64 y = 0; y < H; ++y) {
        T *lin = raw + y*inrowsz;
        T *lou = p + y*ourowsz;
        for (bim::int64 x = 0; x < W; ++x) {
            *lou = *lin;
            lou++;
            lin += step;
        } // for x
    } // for y
}

bim::uint dicomReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;

    bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;  

    // allocate output image
    ImageBitmap *bmp = fmtHndl->image;
    if ( allocImg( fmtHndl, info, bmp) != 0 ) return 1;
    uint64 plane_sz = getImgSizeInBytes(bmp);

    gdcm::File & f = par->reader->GetFile();
    gdcm::PhotometricInterpretation photometric = gdcm::ImageHelper::GetPhotometricInterpretationValue(f);
    DICOMParams::PlanarConfig planar_config = (DICOMParams::PlanarConfig) gdcm::ImageHelper::GetPlanarConfigurationValue(f);
    gdcm::PixelFormat pf = gdcm::ImageHelper::GetPixelFormatValue(f);
    int pixelsize = pf.GetPixelSize();
    uint64 buffer_sz = info->width * info->height * pixelsize;

    if (buffer_sz != plane_sz*info->samples) return 1;

    std::vector<char> buffer(buffer_sz);
    gdcm::BoxRegion box;
    box.SetDomain(0, info->width - 1, 0, info->height - 1, page, page);
    par->reader->SetRegion(box);
    char *buf = (char *)&buffer[0];
    if (!par->reader->ReadIntoBuffer(buf, buffer_sz)) return 1;

    // simplest one channel case, read data directly into the image buffer
    if (info->samples == 1) {
        memcpy(bmp->bits[0], buf, plane_sz);
    } else if (info->samples == 3 && planar_config == DICOMParams::RRRGGGBBB) {
        for (int s = 0; s < info->samples; ++s) {
            memcpy(bmp->bits[s], buf + plane_sz*s, plane_sz);
        }
    } else {
        // in multi-channel interleaved case read into appropriate channels
        for (int s = 0; s < info->samples; ++s) {
            if (bmp->i.depth == 8)
                copy_channel<bim::uint8>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else if (bmp->i.depth == 16)
                copy_channel<bim::uint16>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else if (bmp->i.depth == 32)
                copy_channel<bim::uint32>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
            else if (bmp->i.depth == 64)
                copy_channel<bim::uint64>(info->width, info->height, info->samples, s, buf, bmp->bits[s]);
        } // for sample

    }
    return 0;
}

bim::uint dicomWriteImageProc ( FormatHandle *fmtHndl ) {
    return 1;
    fmtHndl;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

inline std::string read_tag(const gdcm::DataSet& ds, uint16_t group, uint16_t element) {
    const gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return "";

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    if (de.IsEmpty()) return "";
    //const gdcm::ByteValue * bv = de.GetByteValue();

    std::stringstream strm;
    de.GetValue().Print(strm);
    std::string value = strm.str();
    return value;
}


bim::uint dicom_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return 1;
  if (isCustomReading (fmtHndl)) return 1;
  bim::DICOMParams *par = (bim::DICOMParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;

  //gdcm::Image & img = par->reader->GetImage();
  const gdcm::DataSet& ds = par->reader->GetFile().GetDataSet();
  gdcm::File & f = par->reader->GetFile();

  //-------------------------------------------
  // scale
  //-------------------------------------------

  hash->set_value(bim::PIXEL_RESOLUTION_X, info->xRes);
  hash->set_value(bim::PIXEL_RESOLUTION_Y, info->yRes);
  hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, "mm");
  hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, "mm");


  xstring slice_thickness_mm = read_tag(ds, 0x0018, 0x0050);
  if (slice_thickness_mm.size() > 0) {
      hash->set_value(bim::PIXEL_RESOLUTION_Z, slice_thickness_mm);
      hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, "mm");
  }

  xstring temporal_resolution = read_tag(ds, 0x0020, 0x0110);
  if (temporal_resolution.size() > 0) {
      hash->set_value(bim::PIXEL_RESOLUTION_T, temporal_resolution);
      hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "seconds");
  }

  if (info->number_z > 1 && info->number_t == 1) {
      std::vector<double> spacing = gdcm::ImageHelper::GetSpacingValue(f);
      hash->set_value(bim::PIXEL_RESOLUTION_Z, spacing[2]);
      hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, "mm");
  } else if (info->number_z == 1 && info->number_t > 1) {
      std::vector<double> spacing = gdcm::ImageHelper::GetSpacingValue(f);
      hash->set_value(bim::PIXEL_RESOLUTION_T, spacing[2]);
      hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "seconds");
  }

  //-------------------------------------------
  //date: Thu Jun 22 08:27:13 2006
  //-------------------------------------------

  xstring d = read_tag(ds, 0x0008, 0x0012);
  xstring t = read_tag(ds, 0x0008, 0x0013);
  if (d.size()>=8 && t.size()>=6) {
      xstring dt = xstring::xprintf("%s-%s-%s %s:%s:%s", d.substr(0, 4).c_str(), d.substr(4, 2).c_str(), d.substr(6, 2).c_str(), t.substr(0, 2).c_str(), t.substr(2, 2).c_str(), t.substr(4, 2).c_str());
      hash->set_value(bim::IMAGE_DATE_TIME, dt);
  }

  //-------------------------------------------
  // channel names
  //-------------------------------------------
  //for (unsigned int i = 0; i<z->channels(); ++i) {
  //    if (z->info()->hasKey(xstring::xprintf("channel_name_%d", i)))
  //        hash->set_value(xstring::xprintf(bim::CHANNEL_NAME_TEMPLATE.c_str(), i), z->info()->get_value(xstring::xprintf("channel_name_%d", i)));
  //}

  //-------------------------------------------
  // preferred lut mapping
  //-------------------------------------------
  // now write tags based on the order
  /*std::map<int, std::string> display_channel_names;
  display_channel_names[bim::Red] = "red";
  display_channel_names[bim::Green] = "green";
  display_channel_names[bim::Blue] = "blue";
  display_channel_names[bim::Yellow] = "yellow";
  display_channel_names[bim::Magenta] = "magenta";
  display_channel_names[bim::Cyan] = "cyan";
  display_channel_names[bim::Gray] = "gray";

  hash->set_value(bim::DISPLAY_CHANNEL_RED, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Red].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_GREEN, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Green].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_BLUE, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Blue].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_YELLOW, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Yellow].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_MAGENTA, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Magenta].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_CYAN, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Cyan].c_str())));
  hash->set_value(bim::DISPLAY_CHANNEL_GRAY, z->info()->get_value(xstring::xprintf("display_channel_%s", display_channel_names[bim::Gray].c_str())));
  */

  //-------------------------------------------
  // include all other tags into custom tag location
  //-------------------------------------------
  const gdcm::Global& g = gdcm::Global::GetInstance();
  const gdcm::Dicts &dicts = g.GetDicts();
  const gdcm::Dict &pubdict = dicts.GetPublicDict();

  gdcm::DataSet::ConstIterator it = ds.Begin();
  while (it != ds.End()) {
      const gdcm::DataElement &de = (*it);

      if (!de.IsEmpty()) {
          std::stringstream strm;
          const gdcm::Tag tag = de.GetTag();

          const gdcm::DictEntry entry = pubdict.GetDictEntry(tag);
          std::string name = entry.GetName();
          name += xstring::xprintf(" (%04x,%04x)", tag.GetGroup(), tag.GetElement());

          de.GetValue().Print(strm);
          std::string value = strm.str();
          hash->set_value(xstring("DICOM/") + name, value);
      }

      ++it;
  }

  return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_DICOM_NUM_FORMATS 1

FormatItem dicomItems[BIM_DICOM_NUM_FORMATS] = {
  { //0
    "DICOM",            // short name, no spaces
    "Digital Imaging and Communications in Medicine (DICOM)", // Long format name
    "dcm|dic|dicom",   // pipe "|" separated supported extension list
    1, //canRead;      // 0 - NO, 1 - YES
    0, //canWrite;     // 0 - NO, 1 - YES
    1, //canReadMeta;  // 0 - NO, 1 - YES
    0, //canWriteMeta; // 0 - NO, 1 - YES
    1, //canWriteMultiPage;   // 0 - NO, 1 - YES
    //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 0, 0, 0, 0, 0 } 
  }
};


FormatHeader dicomHeader = {

  sizeof(FormatHeader),
  "2.4.4",
  "DICOM",
  "Digital Imaging and Communications in Medicine (DICOM)",
  
  BIM_FORMAT_DICOM_MAGIC_SIZE,
  {1, BIM_DICOM_NUM_FORMATS, dicomItems},
  
  dicomValidateFormatProc,
  // begin
  dicomAquireFormatProc, //AquireFormatProc
  // end
  dicomReleaseFormatProc, //ReleaseFormatProc
  
  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  dicomOpenImageProc, //OpenImageProc
  dicomCloseImageProc, //CloseImageProc 

  // info
  dicomGetNumPagesProc, //GetNumPagesProc
  dicomGetImageInfoProc, //GetImageInfoProc


  // read/write
  dicomReadImageProc, //ReadImageProc 
  NULL, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc
  
  // meta data
  dicomReadMetaDataProc, //ReadMetaDataProc
  dicomAddMetaDataProc,  //AddMetaDataProc
  dicomReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  dicom_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

FormatHeader* dicomGetFormatHeader(void)
{
  return &dicomHeader;
}

} // extern C


