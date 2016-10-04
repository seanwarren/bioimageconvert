/*****************************************************************************
LCMS2 metadata parsing generic functions
Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
2015-08-07 10:12:40 - First creation

ver : 1
*****************************************************************************/

#include <xstring.h>
#include <tag_map.h>
#include <bim_metatags.h>

#include "bim_lcms_parse.h"

#include <lcms2.h>

using namespace bim;

std::string lcms_ver_to_string(unsigned char v[4]) {
    return xstring::xprintf("%d.%d.%d", v[0], v[1], v[2]);

}

void lcms_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return;
    if (isCustomReading(fmtHndl)) return;
    if (!hash) return;

    try {
        if (hash->hasKey(bim::RAW_TAGS_ICC)) {
            // read profile from buffer
            cmsHPROFILE ph = cmsOpenProfileFromMem(hash->get_value_bin(bim::RAW_TAGS_ICC), hash->get_size(bim::RAW_TAGS_ICC));
            if (ph) {
                hash->set_value(bim::ICC_TAGS_DEFINITION, bim::ICC_TAGS_DEFINITION_EMBEDDED);

                cmsFloat64Number ver = cmsGetProfileVersion(ph);
                hash->set_value(bim::ICC_TAGS_VERSION, xstring::xprintf("%.1f", ver));

                bim::uint32 sz = cmsGetProfileInfo(ph, cmsInfoDescription, "en", "US", NULL, 0);
                if (sz > 0) {
                    std::string descr("\0", sz);
                    cmsGetProfileInfoASCII(ph, cmsInfoDescription, "en", "US", &descr[0], sz);
                    hash->set_value(bim::ICC_TAGS_DESCRIPTION, descr);
                }

                cmsColorSpaceSignature sig = cmsGetColorSpace(ph);
                std::string cs = bim::ICC_TAGS_COLORSPACE_MULTICHANNEL;
                if (sig == cmsSigXYZData) cs = bim::ICC_TAGS_COLORSPACE_XYZ;
                else if (sig == cmsSigLabData) cs = bim::ICC_TAGS_COLORSPACE_LAB;
                else if (sig == cmsSigLuvData) cs = bim::ICC_TAGS_COLORSPACE_LUV;
                else if (sig == cmsSigYCbCrData) cs = bim::ICC_TAGS_COLORSPACE_YCBCR;
                else if (sig == cmsSigRgbData) cs = bim::ICC_TAGS_COLORSPACE_RGB;
                else if (sig == cmsSigGrayData) cs = bim::ICC_TAGS_COLORSPACE_GRAY;
                else if (sig == cmsSigHsvData) cs = bim::ICC_TAGS_COLORSPACE_HSV;
                else if (sig == cmsSigHlsData) cs = bim::ICC_TAGS_COLORSPACE_HSL;
                else if (sig == cmsSigCmykData) cs = bim::ICC_TAGS_COLORSPACE_CMYK;
                else if (sig == cmsSigCmyData) cs = bim::ICC_TAGS_COLORSPACE_CMY;
                hash->set_value(bim::ICC_TAGS_COLORSPACE, cs);
                if (cs != bim::ICC_TAGS_COLORSPACE_MULTICHANNEL)
                    hash->set_value(bim::IMAGE_MODE, cs);

                hash->set_value(bim::ICC_TAGS_SIZE, hash->get_size(bim::RAW_TAGS_ICC));

                cmsCloseProfile(ph);
            }
        }
    }
    catch (...) {
        return;
    }

}

bim::ImageModes lcms_image_mode(const std::string &s) {
    if (s == bim::ICC_TAGS_COLORSPACE_XYZ) return bim::IM_XYZ;
    else if (s == bim::ICC_TAGS_COLORSPACE_LAB) return bim::IM_LAB;
    else if (s == bim::ICC_TAGS_COLORSPACE_LUV) return bim::IM_LUV;
    else if (s == bim::ICC_TAGS_COLORSPACE_YCBCR) return bim::IM_YCbCr;
    else if (s == bim::ICC_TAGS_COLORSPACE_RGB) return bim::IM_RGB;
    else if (s == bim::ICC_TAGS_COLORSPACE_GRAY) return bim::IM_GRAYSCALE;
    else if (s == bim::ICC_TAGS_COLORSPACE_HSV) return bim::IM_HSV;
    else if (s == bim::ICC_TAGS_COLORSPACE_HSL) return bim::IM_HSL;
    else if (s == bim::ICC_TAGS_COLORSPACE_CMYK) return bim::IM_CMYK;
    else if (s == bim::ICC_TAGS_COLORSPACE_CMY) return bim::IM_CMY;
    return bim::IM_MULTI;
}


