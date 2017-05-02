/*****************************************************************************
  GeoTIFF metadata parsing generic functions
  Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
  Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>
  
  Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

  History:
    12/11/2014 5:38:23 PM - First creation
        
  ver : 1
*****************************************************************************/

#include <sstream>

#include <tiffio.h>
#include <tiffiop.h>
#include <tiffio.hxx>

#include <geotiff.h> // public interface
#include <geo_tiffp.h> // external TIFF interface
#include <geo_keyp.h> // private interface
#include <geokeys.h>
#include <geovalues.h>
#include <geo_normalize.h>
//#include <geo_simpletags.h>
//#include <cpl_serv.h>

#include <xstring.h>
#include <tag_map.h>
#include <tiff/bim_tiny_tiff.h>

//----------------------------------------------------------------------------
// formatting metadata - rewrite of geotiff functions
//----------------------------------------------------------------------------

#define BIM_GEOTIFF_PREFIX     "Geo/"
#define BIM_GEOTIFF_FMT_DOUBLE "%.15g"
#define BIM_GEOTIFF_FMT_SHORT  "%11hd"

static void PrintTag(int tag, int nrows, double *dptr, int ncols, bim::TagMap *hash, const std::string &path) {
    double *data = dptr;
    bim::xstring value;
    for (int i=0; i<nrows; i++) {
        for (int j=0; j<ncols; j++) {
            value += bim::xstring::xprintf(BIM_GEOTIFF_FMT_DOUBLE, *data++);
            if (j < ncols-1) value += ',';
        }
        if (i < nrows-1) value += ';';
    }
    hash->set_value(path + GTIFTagName(tag), value);
    _GTIFFree(dptr); // free up the allocated memory
}

static void PrintKey(GeoKey *key, bim::TagMap *hash, const std::string &path) {
    geokey_t keyid = (geokey_t)key->gk_key;
    int count = key->gk_count;
    int vals_now;
    bim::xstring value;

    if (key->gk_type == TYPE_ASCII) {
        char *data = key->gk_data;
        int in_char=0;
        while (in_char < count - 1) {
            char ch = ((char *)data)[in_char++];
            value += ch;
        }
    } else if (key->gk_type == TYPE_DOUBLE) {
        char *data = key->gk_data;
        for (double *dptr = (double *)data; count > 0; count -= vals_now) {
            vals_now = count > 3 ? 3 : count;
            for (int i = 0; i < vals_now; i++, dptr++) {
                value += bim::xstring::xprintf(BIM_GEOTIFF_FMT_DOUBLE, *dptr);
                if (i < vals_now - 1)
                    value += ",";
            }
        }
    } else if (key->gk_type == TYPE_SHORT) {
        if (count == 1) {
            pinfo_t *sptr = (pinfo_t *)&key->gk_data;
            value = GTIFValueName(keyid, *sptr);
        } else {
            pinfo_t *sptr = (pinfo_t *)key->gk_data;
            for (; count > 0; count -= vals_now) {
                vals_now = count > 3 ? 3 : count;
                for (int i = 0; i < vals_now; i++, sptr++) {
                    value += bim::xstring::xprintf(BIM_GEOTIFF_FMT_SHORT, *sptr);
                    if (i < vals_now - 1)
                        value += ",";

                }
            }
        }
    }

    hash->set_value(path + GTIFKeyName(keyid), value);
}


static int GTIFReportACorner(GTIF *gtif, GTIFDefn *defn, bim::TagMap *hash, const std::string &corner_name,
                             double x, double y, int inv_flag, int dec_flag)
{
    double x_saved=x, y_saved=y;

    // Try to transform the coordinate into PCS space
    if (!GTIFImageToPCS(gtif, &x, &y))
        return FALSE;

    bim::xstring value;

    if (defn->Model == ModelTypeGeographic) {
        if (dec_flag)
            value = bim::xstring::xprintf("%.7f,%.7f", x, y);
        else
            value = bim::xstring::xprintf("%s,%s", GTIFDecToDMS(x, "Long", 2), GTIFDecToDMS(y, "Lat", 2));
    } else {
        value = bim::xstring::xprintf("%.3f,%.3f", x, y);
        hash->set_value(corner_name+"_model", value);

        if (GTIFProj4ToLatLong(defn, 1, &x, &y)) {
            if (dec_flag)
                value = bim::xstring::xprintf("%.7f,%.7f", y, x);
            else
                value = bim::xstring::xprintf("%s,%s", GTIFDecToDMS(x, "Long", 2), GTIFDecToDMS(y, "Lat", 2));
        }
    }

    hash->set_value(corner_name, value);

    if (inv_flag && GTIFPCSToImage(gtif, &x_saved, &y_saved)) {
        value = bim::xstring::xprintf("%.3f,%.3f", x_saved, y_saved);
        hash->set_value(corner_name + "_inverse", value);
    }

    return TRUE;
}

static void GTIFPrintCorners(GTIF *gtif, GTIFDefn *defn, bim::TagMap *hash, const std::string &path,
    int xsize, int ysize, int inv_flag, int dec_flag)
{
    if (!GTIFReportACorner(gtif, defn, hash, path+"upper_left", 0.0, 0.0, inv_flag, dec_flag))
        return;
    GTIFReportACorner(gtif, defn, hash, path + "lower_left", 0.0, ysize, inv_flag, dec_flag);
    GTIFReportACorner(gtif, defn, hash, path + "upper_right", xsize, 0.0, inv_flag, dec_flag);
    GTIFReportACorner(gtif, defn, hash, path + "lower_right", xsize, ysize, inv_flag, dec_flag);
    GTIFReportACorner(gtif, defn, hash, path + "center", xsize / 2.0, ysize / 2.0, inv_flag, dec_flag);
}



static void GTIFPrintDefn(GTIFDefn *psDefn, bim::TagMap *hash, const std::string &path) {
    if (!psDefn->DefnSet)
        return;

    if (psDefn->Model) {
        std::string model = "Projected";
        if (psDefn->Model == ModelTypeGeographic)
            model = "Geographic";
        else if (psDefn->Model == ModelTypeGeocentric)
            model = "Geocentric";
        hash->set_value(path + "type", model);
    }

    char *s = GTIFGetProj4Defn(psDefn);
    if (s) {
        hash->set_value(path + "proj4_definition", s);
        free(s);
    }

    // Get the PCS name if possible
    if (psDefn->PCS != KvUserDefined) {
        char *pszPCSName = NULL;

        GTIFGetPCSInfo(psDefn->PCS, &pszPCSName, NULL, NULL, NULL);
        if (pszPCSName == NULL)
            pszPCSName = CPLStrdup("name unknown");
        
        hash->set_value(path + "PCS", 
            bim::xstring::xprintf("%d (%s)", psDefn->PCS, pszPCSName));

        CPLFree(pszPCSName);
    }

    // Dump the projection code if possible
    if (psDefn->ProjCode != KvUserDefined) {
        char *pszTRFName = NULL;

        GTIFGetProjTRFInfo(psDefn->ProjCode, &pszTRFName, NULL, NULL);
        if (pszTRFName == NULL)
            pszTRFName = CPLStrdup("");

        hash->set_value(path + "projection", 
            bim::xstring::xprintf("%d (%s)", psDefn->ProjCode, pszTRFName));

        CPLFree(pszTRFName);
    }

    // Try to dump the projection method name, and parameters if possible
    if (psDefn->CTProjection != KvUserDefined) {
        char *pszName = GTIFValueName(ProjCoordTransGeoKey, psDefn->CTProjection);
        if (pszName == NULL)
            pszName = "(unknown)";

        hash->set_value(path + "method", pszName);

        for (int i = 0; i < psDefn->nParms; i++) {
            if (psDefn->ProjParmId[i] == 0)
                continue;

            pszName = GTIFKeyName((geokey_t)psDefn->ProjParmId[i]);
            if (pszName == NULL)
                pszName = "(unknown)";

            if (i < 4) {
                char *pszAxisName;

                if (strstr(pszName, "Long") != NULL)
                    pszAxisName = "Long";
                else if (strstr(pszName, "Lat") != NULL)
                    pszAxisName = "Lat";
                else
                    pszAxisName = "?";

                hash->set_value(path + pszName,
                    bim::xstring::xprintf("%f (%s)", psDefn->ProjParm[i], 
                    GTIFDecToDMS(psDefn->ProjParm[i], pszAxisName, 2)));
            }
            else if (i == 4)
                hash->set_value(path + pszName, bim::xstring::xprintf("%f", psDefn->ProjParm[i]));
            else
                hash->set_value(path + pszName, bim::xstring::xprintf("%f m", psDefn->ProjParm[i]));
        }
    }

    // Report the GCS name, and number
    if (psDefn->GCS != KvUserDefined) {
        char *pszName = NULL;

        GTIFGetGCSInfo(psDefn->GCS, &pszName, NULL, NULL, NULL);
        if (pszName == NULL)
            pszName = CPLStrdup("(unknown)");

        hash->set_value(path + "GCS",
            bim::xstring::xprintf("%d/%s", psDefn->GCS, pszName));
        CPLFree(pszName);
    }

    // Report the datum name
    if (psDefn->Datum != KvUserDefined) {
        char *pszName = NULL;

        GTIFGetDatumInfo(psDefn->Datum, &pszName, NULL);
        if (pszName == NULL)
            pszName = CPLStrdup("(unknown)");

        hash->set_value(path + "datum",
            bim::xstring::xprintf("%d/%s", psDefn->Datum, pszName));
        CPLFree(pszName);
    }

    // Report the ellipsoid
    if (psDefn->Ellipsoid != KvUserDefined) {
        char *pszName = NULL;

        GTIFGetEllipsoidInfo(psDefn->Ellipsoid, &pszName, NULL, NULL);
        if (pszName == NULL)
            pszName = CPLStrdup("(unknown)");

        hash->set_value(path + "ellipsoid",
            bim::xstring::xprintf("%d/%s (%.2f,%.2f)", psDefn->Ellipsoid, pszName,
            psDefn->SemiMajor, psDefn->SemiMinor));
        CPLFree(pszName);
    }

    // Report the prime meridian
    if (psDefn->PM != KvUserDefined) {
        char *pszName = NULL;

        GTIFGetPMInfo(psDefn->PM, &pszName, NULL);

        if (pszName == NULL)
            pszName = CPLStrdup("(unknown)");

        hash->set_value(path + "prime_meridian",
            bim::xstring::xprintf("%d/%s (%f/%s)", psDefn->PM, pszName,
            psDefn->PMLongToGreenwich,
            GTIFDecToDMS(psDefn->PMLongToGreenwich, "Long", 2)));
        CPLFree(pszName);
    }

    // Report TOWGS84 parameters
    if (psDefn->TOWGS84Count > 0) {
        bim::xstring value;
        for (int i = 0; i < psDefn->TOWGS84Count; i++) {
            value += bim::xstring::xprintf("%g", psDefn->TOWGS84[i]);
            if (i < psDefn->TOWGS84Count-1) 
                value += ",";
        }
        hash->set_value(path + "TOWGS84", value);
    }

    // Report the projection units of measure (currently just linear)
    if (psDefn->UOMLength != KvUserDefined) {
        char *pszName = NULL;

        GTIFGetUOMLengthInfo(psDefn->UOMLength, &pszName, NULL);
        if (pszName == NULL)
            pszName = CPLStrdup("(unknown)");

        hash->set_value(path + "units",
            bim::xstring::xprintf("%d/%s (%fm)", psDefn->UOMLength, pszName, psDefn->UOMLengthInMeters));

        CPLFree(pszName);
    }
}


//----------------------------------------------------------------------------
// Appending metadata
//----------------------------------------------------------------------------

#include <bim_metatags.h>
#include "bim_tiff_format.h"
#include "bim_geotiff_parse.h"

using namespace bim;

void geotiff_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {
  if (fmtHndl == NULL) return;
  if (isCustomReading (fmtHndl)) return;
  if (!hash) return;

  TiffParams *par = (TiffParams *)fmtHndl->internalParams;
  if (!par->tiff) return;
  
  // skip if no geotiff tags are present
  if (par->ifds.isValid()) {
      if (!par->ifds.tagPresentInFirstIFD(TIFFTAG_GEOPIXELSCALE) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_INTERGRAPH_MATRIX) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_GEOTIEPOINTS) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_GEOTRANSMATRIX) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_GEOKEYDIRECTORY) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_GEODOUBLEPARAMS) &&
          !par->ifds.tagPresentInFirstIFD(TIFFTAG_GEOASCIIPARAMS)) return;
  }

  //hash->set_value(bim::RAW_TAGS_GEOTIFF, buf, sz, bim::RAW_TYPES_GEOTIFF);

  // use GeoTIFF to read metadata
  try {
      TIFF *tif = par->tiff;
      GTIF *gtif = GTIFNew(tif);
      if (!gtif) return;

      // print header
      hash->set_value(xstring::xprintf("%sType", BIM_GEOTIFF_PREFIX), "GeoTIFF");
      hash->set_value(xstring::xprintf("%sVersion", BIM_GEOTIFF_PREFIX), gtif->gt_version);
      hash->set_value(xstring::xprintf("%sKey_Revision", BIM_GEOTIFF_PREFIX), 
                      xstring::xprintf("%1hd.%hd", gtif->gt_rev_major, gtif->gt_rev_minor));

      // print tags
      {
          xstring path = xstring::xprintf("%sTags/", BIM_GEOTIFF_PREFIX);
          double *data;
          int count;

          if ((gtif->gt_methods.get)(tif, GTIFF_TIEPOINTS, &count, &data))
              PrintTag(GTIFF_TIEPOINTS, count / 3, data, 3, hash, path);
          
          if ((gtif->gt_methods.get)(tif, GTIFF_TRANSMATRIX, &count, &data))
              PrintTag(GTIFF_TRANSMATRIX, count / 4, data, 4, hash, path);
          
          if ((gtif->gt_methods.get)(tif, GTIFF_PIXELSCALE, &count, &data)) {
              // store pixel resolution and units
              hash->append_tag(bim::PIXEL_RESOLUTION_X, bim::xstring::xprintf(BIM_GEOTIFF_FMT_DOUBLE, data[0]));
              hash->append_tag(bim::PIXEL_RESOLUTION_Y, bim::xstring::xprintf(BIM_GEOTIFF_FMT_DOUBLE, data[1]));
              hash->append_tag(bim::PIXEL_RESOLUTION_Z, bim::xstring::xprintf(BIM_GEOTIFF_FMT_DOUBLE, data[2]));
              hash->set_value(bim::PIXEL_RESOLUTION_UNIT_X, bim::PIXEL_RESOLUTION_UNIT_METERS);
              hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Y, bim::PIXEL_RESOLUTION_UNIT_METERS);
              hash->set_value(bim::PIXEL_RESOLUTION_UNIT_Z, bim::PIXEL_RESOLUTION_UNIT_METERS);
              
              PrintTag(GTIFF_PIXELSCALE, count / 3, data, 3, hash, path);
          }
      }

      // print keys
      {
          int numkeys = gtif->gt_num_keys;
          GeoKey *key = gtif->gt_keys;
          xstring path = xstring::xprintf("%sKeys/", BIM_GEOTIFF_PREFIX);
          for (int i=0; i<numkeys; i++)
              PrintKey(++key, hash, path);
      }

      
      // print PCS and corners
      GTIFDefn defn;
      if (GTIFGetDefn(gtif, &defn)) {
          GTIFPrintDefn(&defn, hash, xstring::xprintf("%sModel/", BIM_GEOTIFF_PREFIX) );

          xstring path = xstring::xprintf("%sCoordinates/", BIM_GEOTIFF_PREFIX);
          int xsize, ysize;
          TIFFGetField(par->tiff, TIFFTAG_IMAGEWIDTH, &xsize);
          TIFFGetField(par->tiff, TIFFTAG_IMAGELENGTH, &ysize);
          GTIFPrintCorners(gtif, &defn, hash, path, xsize, ysize, 0, 1);
      }

      GTIFFree(gtif);

  } catch(...) {
      return;
  }
}

//----------------------------------------------------------------------------
// GeoTIFF buffer support
//----------------------------------------------------------------------------

bool isGeoTiff(TIFF *tif) {
    double *d_list = NULL;
    bim::int16   d_list_count;

    if (!TIFFGetField(tif, GTIFF_TIEPOINTS, &d_list_count, &d_list) &&
        !TIFFGetField(tif, GTIFF_PIXELSCALE, &d_list_count, &d_list) &&
        !TIFFGetField(tif, GTIFF_TRANSMATRIX, &d_list_count, &d_list))
        return false;
    else
        return true;
}

static void CopyGeoTIFF(TIFF *in, TIFF *out) {
    double *d_list = NULL;
    bim::int16   d_list_count;

    // read definition from source file
    GTIF *gtif = gtif = GTIFNew(in);
    if (!gtif) return;

    // TAG 33922
    if (TIFFGetField(in, GTIFF_TIEPOINTS, &d_list_count, &d_list))
        TIFFSetField(out, GTIFF_TIEPOINTS, d_list_count, d_list);

    // TAG 33550
    if (TIFFGetField(in, GTIFF_PIXELSCALE, &d_list_count, &d_list))
        TIFFSetField(out, GTIFF_PIXELSCALE, d_list_count, d_list);

    // 34264
    if (TIFFGetField(in, GTIFF_TRANSMATRIX, &d_list_count, &d_list))
        TIFFSetField(out, GTIFF_TRANSMATRIX, d_list_count, d_list);

    // Here we violate the GTIF abstraction to retarget on another file.
    //   We should just have a function for copying tags from one GTIF object
    //   to another
    gtif->gt_tif = out;
    gtif->gt_flags |= FLAG_FILE_MODIFIED;

    // Install keys and tags
    GTIFWriteKeys(gtif);
    GTIFFree(gtif);
    return;
};

int GTIFFromBuffer(const std::vector<char> &buffer, TIFF *out) {
    TinyTiff::MemoryStream stream((char*)&buffer[0], buffer.size());
    TIFF* in = XTIFFClientOpen("MemoryTIFF", "r", (thandle_t)&stream,
        TinyTiff::mem_read, TinyTiff::mem_write, TinyTiff::mem_seek,
        TinyTiff::mem_close, TinyTiff::mem_size, TinyTiff::mem_map, TinyTiff::mem_unmap);
    if (!in) return -1;
    CopyGeoTIFF(in, out);
    XTIFFClose(in);
    return 0;
}

int BufferFromGTIF(TIFF *in, std::vector<char> &buffer ) {
    if (!in || !isGeoTiff(in)) return -1;
    std::stringstream stream;
    TIFF* out = TIFFStreamOpen("MemTIFF", (std::ostream *) &stream);
    if (!out) return -1;

    // Write some minimal set of image parameters.                    
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

    // Copy GEO params
    CopyGeoTIFF(in, out);

    // Finish tiff buffer        
    unsigned char tiny_image = 0;
    TIFFWriteEncodedStrip(out, 0, (char *)&tiny_image, 1);
    TIFFWriteCheck(out, TIFFIsTiled(out), "MemBufFromGTIF");
    TIFFWriteDirectory(out);
    XTIFFClose(out);

    // copy data into output buffer
    buffer.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return 0;
}
