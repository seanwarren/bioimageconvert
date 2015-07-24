/*****************************************************************************
  NIFTI support
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

#include "bim_nifti_format.h"

#include <nifti1.h>
#include <nifti1_io.h>
#include <fslio.h>

#include <pugixml.hpp>

using namespace bim;

//****************************************************************************
// defs
//****************************************************************************

std::map< int, xstring > init_intents() {
    std::map<int, xstring > v;
    v.insert(std::make_pair(0, "None"));
    v.insert(std::make_pair(2, "Correlation"));
    v.insert(std::make_pair(3, "T-Test"));
    v.insert(std::make_pair(4, "F-Test"));
    v.insert(std::make_pair(5, "Standard normal"));
    v.insert(std::make_pair(6, "Chi-squared"));
    v.insert(std::make_pair(7, "Beta distribution"));
    v.insert(std::make_pair(8, "Binomial distribution"));
    v.insert(std::make_pair(9, "Gamma distribution"));
    v.insert(std::make_pair(10, "Poisson distribution"));
    v.insert(std::make_pair(11, "Normal distribution"));
    v.insert(std::make_pair(12, "Noncentral F statistic"));
    v.insert(std::make_pair(13, "Noncentral chi-squared statistic"));
    v.insert(std::make_pair(14, "Logistic distribution"));
    v.insert(std::make_pair(15, "Laplace distribution"));
    v.insert(std::make_pair(16, "Uniform distribution"));
    v.insert(std::make_pair(17, "Noncentral t statistic"));
    v.insert(std::make_pair(18, "Weibull distribution"));
    v.insert(std::make_pair(19, "Chi distribution"));
    v.insert(std::make_pair(20, "Inverse Gaussian"));
    v.insert(std::make_pair(21, "Extreme value"));
    v.insert(std::make_pair(22, "p-value"));
    v.insert(std::make_pair(23, "ln (p-value)"));
    v.insert(std::make_pair(24, "log10 (p-value)"));
    v.insert(std::make_pair(1001, "Estimate"));
    v.insert(std::make_pair(1002, "Label"));
    v.insert(std::make_pair(1003, "NeuroNames"));
    v.insert(std::make_pair(1004, "Matrix"));
    v.insert(std::make_pair(1005, "Symmetric matrix"));
    v.insert(std::make_pair(1006, "Displacement field"));
    v.insert(std::make_pair(1007, "Vector"));
    v.insert(std::make_pair(1008, "Pointset"));
    v.insert(std::make_pair(1009, "Triangle"));
    v.insert(std::make_pair(1010, "Quaternion"));
    v.insert(std::make_pair(1011, "Dimensionless value"));
    v.insert(std::make_pair(2001, "Time series"));
    v.insert(std::make_pair(2002, "Node Index"));
    v.insert(std::make_pair(2003, "RGB Vector"));
    v.insert(std::make_pair(2004, "RGBA Vector"));
    v.insert(std::make_pair(2005, "Shape"));
    return v;
}

std::map< int, xstring > init_xform() {
    std::map<int, xstring > v;
    v.insert(std::make_pair(0, "Unknown"));
    v.insert(std::make_pair(1, "Scanner-based anatomical coordinates"));
    v.insert(std::make_pair(2, "Aligned to anatomical ground-truth"));
    v.insert(std::make_pair(3, "Aligned to Talairach-Tournoux Atlas"));
    v.insert(std::make_pair(4, "MNI 152 normalized"));
    return v;
}


//****************************************************************************
// Misc
//****************************************************************************

bim::NIFTIParams::NIFTIParams() {
    i = initImageInfo(); 
    header = NULL;
    nim = NULL;
}

bim::NIFTIParams::~NIFTIParams() {
    if (header) free(header);
    if (nim) nifti_image_free(nim);
}

std::map<int, xstring> NIFTIParams::intents = init_intents();
std::map<int, xstring> NIFTIParams::xforms = init_xform();

//****************************************************************************
// required funcs
//****************************************************************************

#define BIM_FORMAT_NIFTI_MAGIC_SIZE 348

int niftiValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
    if (length < BIM_FORMAT_NIFTI_MAGIC_SIZE) return -1;
    unsigned char *mag_num = (unsigned char *) magic;

    if (memcmp(mag_num+344, "ni1", 3) == 0 ) return 0;
    if (memcmp(mag_num+344, "n+1", 3) == 0) return 0;
    return -1;
}

FormatHandle niftiAquireFormatProc( void ) {
    FormatHandle fp = initFormatHandle();
    return fp;
}

void niftiCloseImageProc (FormatHandle *fmtHndl);
void niftiReleaseFormatProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    niftiCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------

void niftiGetImageInfo(FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    if (fmtHndl->internalParams == NULL) return;
    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    if (par->header == NULL) return;
    nifti_1_header *h = par->header;

    ImageInfo *info = &par->i;  
    *info = initImageInfo();

    info->width = h->dim[1];
    info->height = h->dim[2];
    info->number_z = h->dim[3];
    info->number_t = h->dim[4];
    info->number_pages = info->number_z * info->number_t;

    // set XY scale
    info->resUnits = RES_mm;
    info->xRes = h->pixdim[1];
    info->yRes = h->pixdim[2];

    if (h->dim[0] <= 3 && h->xyzt_units == NIFTI_UNITS_METER) {
        info->resUnits = RES_m;
    } else if (h->dim[0] <= 3 && h->xyzt_units == NIFTI_UNITS_MICRON) {
        info->resUnits = RES_um;
    }

    //---------------------------------------------------------------
    // define dims
    //---------------------------------------------------------------
    if (info->number_z>1 && info->number_t<=1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_Z;
    } else if (info->number_t>1 && info->number_z <= 1) {
        info->number_dims = 4;
        info->dimensions[3].dim = DIM_T;
    } else if (info->number_z>1 && info->number_t>1) {
        info->number_dims = 5;
        info->dimensions[3].dim = DIM_Z;
        info->dimensions[4].dim = DIM_T;
    }

    //---------------------------------------------------------------
    // pixel depth and format
    //---------------------------------------------------------------

    info->samples = h->dim[5];
    if (info->samples>1)
        info->imageMode = IM_MULTI;
    else
        info->imageMode = IM_GRAYSCALE;

    if (h->datatype == DT_UINT8) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
    } else if (h->datatype == DT_INT8) {
        info->depth = 8;
        info->pixelType = FMT_SIGNED;
    } else if (h->datatype == DT_UINT16) {
        info->depth = 16;
        info->pixelType = FMT_UNSIGNED;
    } else if (h->datatype == DT_INT16) {
        info->depth = 16;
        info->pixelType = FMT_SIGNED;
    } else if (h->datatype == DT_UINT32) {
        info->depth = 32;
        info->pixelType = FMT_UNSIGNED;
    } else if (h->datatype == DT_INT32) {
        info->depth = 32;
        info->pixelType = FMT_SIGNED;
    } else if (h->datatype == DT_UINT64) {
        info->depth = 64;
        info->pixelType = FMT_UNSIGNED;
    } else if (h->datatype == DT_INT64) {
        info->depth = 64;
        info->pixelType = FMT_SIGNED;
    } else if (h->datatype == DT_FLOAT32) {
        info->depth = 32;
        info->pixelType = FMT_FLOAT;
    } else if (h->datatype == DT_FLOAT64) {
        info->depth = 64;
        info->pixelType = FMT_FLOAT;
    } else if (h->datatype == DT_FLOAT128) {
        info->depth = 128;
        info->pixelType = FMT_FLOAT;
    } else if (h->datatype == DT_COMPLEX64) {
        info->depth = 64;
        info->pixelType = FMT_COMPLEX;
    } else if (h->datatype == DT_COMPLEX128) {
        info->depth = 128;
        info->pixelType = FMT_COMPLEX;
    } else if (h->datatype == DT_COMPLEX256) {
        info->depth = 256;
        info->pixelType = FMT_COMPLEX;
    } else if (h->datatype == DT_RGB24) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
        info->samples = 3;
        info->imageMode = IM_RGB;
    } else if (h->datatype == DT_RGBA32) {
        info->depth = 8;
        info->pixelType = FMT_UNSIGNED;
        info->samples = 4;
        info->imageMode = IM_RGBA;
    }
}

void niftiCloseImageProc (FormatHandle *fmtHndl) {
    if (fmtHndl == NULL) return;
    xclose ( fmtHndl );
    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    fmtHndl->internalParams = 0;
    delete par;
}

bim::uint niftiOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
    if (fmtHndl == NULL) return 1;
    if (fmtHndl->internalParams != NULL) niftiCloseImageProc(fmtHndl);
    bim::NIFTIParams *par = new bim::NIFTIParams();
    fmtHndl->internalParams = (void *)par;

    if (io_mode == IO_READ) {
        par->header = nifti_read_header(fmtHndl->fileName, NULL, 1);
        if (par->header == NULL) return 1;
        try {
            niftiGetImageInfo(fmtHndl);
        } catch (...) {
            niftiCloseImageProc(fmtHndl);
            return 1;
        }
    }
    else return 1;
    return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint niftiGetNumPagesProc ( FormatHandle *fmtHndl ) {
    if (fmtHndl == NULL) return 0;
    if (fmtHndl->internalParams == NULL) return 0;
    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;
    return info->number_pages;
}


ImageInfo niftiGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint page_num ) {
    ImageInfo ii = initImageInfo();
    if (fmtHndl == NULL) return ii;
    fmtHndl->pageNumber = page_num;  
    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint niftiAddMetaDataProc (FormatHandle *fmtHndl) {
    fmtHndl=fmtHndl;
    return 1;
}


bim::uint niftiReadMetaDataProc (FormatHandle *fmtHndl, bim::uint page, int group, int tag, int type) {
    if (fmtHndl == NULL) return 1;
    return 1;
}

char* niftiReadMetaDataAsTextProc ( FormatHandle *fmtHndl ) {
    return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

template <typename T>
void copy_channel(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *) in;
    T *p = (T *) out;
    raw += sample;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        T *pp = p + x;
        T *rr = raw + x*samples;
        *pp = *rr;
    } // for x
}

template <typename T>
void scale_channel(bim::uint64 W, bim::uint64 H, const void *in, nifti_1_header *h) {
    T *raw = (T *)in;
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W*H>BIM_OMP_FOR1)
    for (bim::int64 x = 0; x < W*H; ++x) {
        raw[x] = (raw[x] * h->scl_slope) + h->scl_inter;
    } // for x
}

bim::uint niftiReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
    if (fmtHndl == NULL) return 1;
    fmtHndl->pageNumber = page;

    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    nifti_1_header *h = par->header;
    ImageInfo *info = &par->i;  

    // allocate output image
    ImageBitmap *bmp = fmtHndl->image;
    if (allocImg(fmtHndl, info, bmp) != 0) return 1;

    if (!par->nim)
        par->nim = nifti_image_read((char*)fmtHndl->fileName, 1);

    uint64 plane_sz = info->width * info->height * (info->depth / 8);
    uint64 buffer_sz = plane_sz * info->samples;
    int offset = page*buffer_sz;
    unsigned char *buf = (unsigned char *) par->nim->data;

    // simplest one channel case, read data directly into the image buffer
    if (h->datatype != DT_RGB24 && h->datatype != DT_RGBA32) {
        for (int s = 0; s < info->samples; ++s)
            memcpy(bmp->bits[s], buf + offset + s*plane_sz, buffer_sz);
    } else {
        // in multi-channel interleaved case read into appropriate channels
        for (int s = 0; s < info->samples; ++s) {
            if (bmp->i.depth == 8 && bmp->i.pixelType == FMT_UNSIGNED)
                copy_channel<uint8>(info->width, info->height, info->samples, s, buf + offset, bmp->bits[s]);
        } // for sample
    }

    //scale_channel(bim::uint64 W, bim::uint64 H, const void *in, nifti_1_header *h) {
    return 0;
}

bim::uint niftiWriteImageProc ( FormatHandle *fmtHndl ) {
    return 1;
    fmtHndl;
}

//----------------------------------------------------------------------------
// Metadata hash
//----------------------------------------------------------------------------

void nifti_parse_extension_text(TagMap *hash, nifti1_extension *ex) {
    xstring s;
    s.resize(ex->esize);
    memcpy(&s[0], ex->edata, ex->esize);
    hash->set_value("NIFTI/extension_text", s);
}

void nifti_xml_add_tag_attr(TagMap *hash, pugi::xml_document *doc, const std::string &key, const std::string &xpath, const std::string &attr) {
    try {
        pugi::xpath_node node = doc->select_node(xpath.c_str());
        bim::xstring v = node.node().attribute(attr.c_str()).value();
        if (v.size()>0) hash->set_value(key, v);
    } catch (pugi::xpath_exception& e) {
        // do nothing
    }
}

void nifti_xml_add_tag_text(TagMap *hash, pugi::xml_document *doc, const std::string &key, const std::string &xpath) {
    try {
        pugi::xpath_node node = doc->select_node(xpath.c_str());
        bim::xstring v = node.node().first_child().value();
        if (v.size()>0) hash->set_value(key, v);
    }
    catch (pugi::xpath_exception& e) {
        // do nothing
    }
}

void nifti_xml_add_subtag_text(TagMap *hash, const pugi::xpath_node &parent, const std::string &key, const std::string &xpath) {
    try {
        pugi::xpath_node node = parent.node().select_node(xpath.c_str());
        bim::xstring v = node.node().first_child().value();
        if (v.size()>0) hash->set_value(key, v);
    }
    catch (pugi::xpath_exception& e) {
        // do nothing
    }
}

void nifti_parse_extension_xcede(TagMap *hash, nifti1_extension *ex) {
    pugi::xml_document doc;
    if (doc.load_buffer(ex->edata, ex->esize)) {
        nifti_xml_add_tag_text(hash, &doc, "XCEDE/subject/id", "/project/subject/subjectData/ID");
        // read assesments
        try {
            pugi::xpath_node_set assessments = doc.select_nodes("/project/subject/visit/subjectVar/assessment");
            int i = 0;
            for (pugi::xpath_node_set::const_iterator it = assessments.begin(); it != assessments.end(); ++it) {
                pugi::xpath_node node = *it;
                xstring key = xstring::xprintf("XCEDE/subject/visit/assessment/%.4d", ++i);

                nifti_xml_add_subtag_text(hash, node, key + "/name", "name");
                nifti_xml_add_subtag_text(hash, node, key+"/description", "description");

                std::vector<xstring> v;
                pugi::xpath_node_set values = node.node().select_nodes("assessmentValue/summaryValue/actualValue");
                for (pugi::xpath_node_set::const_iterator it = values.begin(); it != values.end(); ++it) {
                    pugi::xpath_node node = *it;
                    v.push_back(node.node().first_child().value());
                }
                hash->set_value(key + "/values", xstring::join(v, ","));
            }
        }
        catch (pugi::xpath_exception& e) {
            // do nothing
        }

        nifti_xml_add_tag_text(hash, &doc, "XCEDE/study/series/id", "/project/subject/visit/study/series/ID");
        nifti_xml_add_tag_text(hash, &doc, "XCEDE/study/series/scanner/manufacturer", "/project/subject/visit/study/series/seriesData/scanner/manufacturer");
        nifti_xml_add_tag_text(hash, &doc, "XCEDE/study/series/scanner/model", "/project/subject/visit/study/series/seriesData/scanner/model");
        
        nifti_xml_add_tag_text(hash, &doc, "XCEDE/study/series/experimental_protocol/name", "/project/subject/visit/study/series/expProtocol/name");
        nifti_xml_add_tag_text(hash, &doc, "XCEDE/study/series/experimental_protocol/annotation", "/project/subject/visit/study/series/expProtocol/annotation/text");

        // read acquisition protocol
        try {
            pugi::xpath_node_set nodes = doc.select_nodes("/project/subject/visit/study/series/acqProtocol/acqParam");
            for (pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
                pugi::xpath_node node = *it;
                bim::xstring n = node.node().attribute("name").value();
                bim::xstring v = node.node().first_child().value();
                hash->set_value(xstring("XCEDE/study/series/acquisition_protocol/parameters/") + n, v);
            }
        } catch (pugi::xpath_exception& e) {
            // do nothing
        }
    }
}

void nifti_parse_extension_afni(TagMap *hash, nifti1_extension *ex) {
    pugi::xml_document doc;
    if (doc.load_buffer(ex->edata, ex->esize)) {
        try {
            pugi::xpath_node_set nodes = doc.select_nodes("/AFNI_attributes/AFNI_atr");
            for (pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
                pugi::xpath_node node = *it;
                bim::xstring n = node.node().attribute("atr_name").value();
                bim::xstring v = node.node().first_child().value();
                hash->set_value(xstring("AFNI/") + n, v);
            }
        }
        catch (pugi::xpath_exception& e) {
            // do nothing
        }
    }
}

void nifti_parse_extension_dicom(TagMap *hash, nifti1_extension *ex) {

}

bim::uint nifti_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
    if (fmtHndl == NULL) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    bim::NIFTIParams *par = (bim::NIFTIParams *) fmtHndl->internalParams;
    ImageInfo *info = &par->i;

    if (par->header == NULL) return 1;
    nifti_1_header *h = par->header;

    //-------------------------------------------
    // scale
    //-------------------------------------------

    hash->set_value(bim::PIXEL_RESOLUTION_X, h->pixdim[1]);
    hash->set_value(bim::PIXEL_RESOLUTION_Y, h->pixdim[2]);
    hash->set_value(bim::PIXEL_RESOLUTION_Z, h->pixdim[3]);
    hash->set_value(bim::PIXEL_RESOLUTION_T, h->pixdim[4]);

    if (h->xyzt_units | NIFTI_UNITS_METER) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, "m");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, "m");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, "m");
    } else if (h->xyzt_units | NIFTI_UNITS_MICRON) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, "um");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, "um");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, "um");
    } else {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, "mm");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, "mm");
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, "mm");
    }

    if (h->xyzt_units | NIFTI_UNITS_MSEC) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "ms");
    } else if (h->xyzt_units | NIFTI_UNITS_USEC) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "us");
    } else {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "s");
    }

    if (h->xyzt_units | NIFTI_UNITS_MSEC) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "ms");
    } else if (h->xyzt_units | NIFTI_UNITS_USEC) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "us");
    } else {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_T, "s");
    }

    if (h->xyzt_units | NIFTI_UNITS_PPM) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_C, "ppm");
    } else if (h->xyzt_units | NIFTI_UNITS_RADS) {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_C, "rads");
    } else {
        hash->set_value(bim::PIXEL_RESOLUTION_UNIT_C, "hz");
    }

    //-------------------------------------------
    // include all other tags into custom tag location
    //-------------------------------------------
    hash->set_value("NIFTI/description", h->descrip);
    try {
        hash->set_value("NIFTI/intent", NIFTIParams::intents[h->intent_code]);
    } catch (...) {}
    hash->set_value("NIFTI/slice_start", h->slice_start);
    hash->set_value("NIFTI/scaling_slope", h->scl_slope);
    hash->set_value("NIFTI/scaling_offset", h->scl_inter);
    hash->set_value("NIFTI/slice_end", h->slice_end);
    hash->set_value("NIFTI/slice_timing_order", h->slice_code);
    hash->set_value("NIFTI/display_intensity_max", h->cal_max);
    hash->set_value("NIFTI/display_intensity_min", h->cal_min);
    hash->set_value("NIFTI/slice_duration", h->slice_duration);
    hash->set_value("NIFTI/time_axis_shift", h->toffset);
    hash->set_value("NIFTI/description", h->descrip);
    hash->set_value("NIFTI/aux_file", h->aux_file);
    try {
        hash->set_value("NIFTI/qform coordinates", NIFTIParams::xforms[h->qform_code]);
        hash->set_value("NIFTI/sform coordinates", NIFTIParams::xforms[h->sform_code]);
    }
    catch (...) {}
    hash->set_value("NIFTI/quaternion", xstring::xprintf("%f,%f,%f;%f,%f,%f", h->quatern_b, h->quatern_c, h->quatern_d, \
        h->qoffset_x, h->qoffset_y, h->qoffset_z));
    hash->set_value("NIFTI/affine_transform", xstring::xprintf("%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f", 
                                                         h->srow_x[0], h->srow_x[1], h->srow_x[2], h->srow_x[3], \
                                                         h->srow_y[0], h->srow_y[1], h->srow_y[2], h->srow_y[3], \
                                                         h->srow_z[0], h->srow_z[1], h->srow_z[2], h->srow_z[3]));
    hash->set_value("NIFTI/intent_name", h->intent_name);


    // read extensions
    if (!par->nim)
        par->nim = nifti_image_read((char*)fmtHndl->fileName, 1);

    for (int i = 0; i < par->nim->num_ext; ++i) {
        nifti1_extension *ex = &par->nim->ext_list[i];
        if (ex->ecode == NIFTI_ECODE_XCEDE) nifti_parse_extension_xcede(hash, ex);
        else if (ex->ecode == NIFTI_ECODE_AFNI) nifti_parse_extension_afni(hash, ex);
        else if (ex->ecode == NIFTI_ECODE_DICOM) nifti_parse_extension_dicom(hash, ex);
        else if (ex->ecode == NIFTI_ECODE_COMMENT) nifti_parse_extension_text(hash, ex);
    }

    return 0;
}

//****************************************************************************
// exported
//****************************************************************************

#define BIM_NIFTI_NUM_FORMATS 1

FormatItem niftiItems[BIM_NIFTI_NUM_FORMATS] = {
    { //0
        "NIFTI",            // short name, no spaces
        "NIfTI-1", // Long format name
        "nii|hdr|img",   // pipe "|" separated supported extension list
        1, //canRead;      // 0 - NO, 1 - YES
        0, //canWrite;     // 0 - NO, 1 - YES
        1, //canReadMeta;  // 0 - NO, 1 - YES
        0, //canWriteMeta; // 0 - NO, 1 - YES
        1, //canWriteMultiPage;   // 0 - NO, 1 - YES
        //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
        { 0, 0, 0, 0, 0, 0, 0, 0 }
    }
};


FormatHeader niftiHeader = {
    sizeof(FormatHeader),
    "2.0.0",
    "NIFTI",
    "Neuroimaging Informatics Technology Initiative (NIFTI)",

    BIM_FORMAT_NIFTI_MAGIC_SIZE,
    { 1, BIM_NIFTI_NUM_FORMATS, niftiItems },

    niftiValidateFormatProc,
    // begin
    niftiAquireFormatProc, //AquireFormatProc
    // end
    niftiReleaseFormatProc, //ReleaseFormatProc

    // params
    NULL, //AquireIntParamsProc
    NULL, //LoadFormatParamsProc
    NULL, //StoreFormatParamsProc

    // image begin
    niftiOpenImageProc, //OpenImageProc
    niftiCloseImageProc, //CloseImageProc 

    // info
    niftiGetNumPagesProc, //GetNumPagesProc
    niftiGetImageInfoProc, //GetImageInfoProc


    // read/write
    niftiReadImageProc, //ReadImageProc 
    NULL, //WriteImageProc
    NULL, //ReadImageTileProc
    NULL, //WriteImageTileProc
    NULL, //ReadImageLineProc
    NULL, //WriteImageLineProc
    NULL, //ReadImageThumbProc
    NULL, //WriteImageThumbProc
    NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc

    // meta data
    niftiReadMetaDataProc, //ReadMetaDataProc
    niftiAddMetaDataProc,  //AddMetaDataProc
    niftiReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
    nifti_append_metadata, //AppendMetaDataProc

    NULL,
    NULL,
    ""
};

extern "C" {

    FormatHeader* niftiGetFormatHeader(void) {
        return &niftiHeader;
    }

} // extern C

