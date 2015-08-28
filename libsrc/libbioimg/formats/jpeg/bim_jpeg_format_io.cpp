/*****************************************************************************
  JPEG IO
  Copyright (c) 2004 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

  IMPLEMENTATION

  Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>


  History:
  03/29/2004 22:23 - First creation
  08/04/2004 22:25 - Update to FMT_IFS 1.2, support for io protorypes
  2010-06-24 15:11 - EXIF/IPTC extraction

  Ver : 3
  *****************************************************************************/

#include "bim_jpeg_format.h"

#include <tag_map.h>
#include <bim_metatags.h>
#include <bim_exiv_parse.h>
#include <bim_lcms_parse.h>
#include <bim_format_misc.h>

static const int max_buf = 4096;

#define EXIF_MARKER		(JPEG_APP0+1)	// EXIF marker / Adobe XMP marker
#define ICC_MARKER		(JPEG_APP0+2)	// ICC profile marker
#define IPTC_MARKER		(JPEG_APP0+13)	// IPTC marker / BIM marker 

#define ICC_HEADER_SIZE 14				// size of non-profile data in APP2
#define MAX_BYTES_IN_MARKER 65533L		// maximum data length of a JPEG marker
#define MAX_DATA_BYTES_IN_MARKER 65519L	// maximum data length of a JPEG APP2 marker

#define MAX_JFXX_THUMB_SIZE (MAX_BYTES_IN_MARKER - 5 - 1)

#define JFXX_TYPE_JPEG 	0x10	// JFIF extension marker: JPEG-compressed thumbnail image
#define JFXX_TYPE_8bit 	0x11	// JFIF extension marker: palette thumbnail image
#define JFXX_TYPE_24bit	0x13	// JFIF extension marker: RGB thumbnail image

#define ICC_OVERHEAD_LEN  14		// size of non-profile data in APP2
#define MAX_SEQ_NO  255		        // sufficient since marker numbers are bytes

struct my_error_mgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};


extern "C" {
    static void my_error_exit(j_common_ptr cinfo)
    {
        my_error_mgr* myerr = (my_error_mgr*)cinfo->err;
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, buffer);
        longjmp(myerr->setjmp_buffer, 1);
    }
}

//****************************************************************************
// READ STUFF
//****************************************************************************

struct my_jpeg_source_mgr : public jpeg_source_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    FormatHandle *fmtHndl;
    JOCTET buffer[max_buf];

public:
    my_jpeg_source_mgr(FormatHandle *new_hndl);
};

extern "C" {

    static void dimjpeg_init_source(j_decompress_ptr)
    {
    }

    static boolean dimjpeg_fill_input_buffer(j_decompress_ptr cinfo) {
        int num_read;
        my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;
        src->next_input_byte = src->buffer;

        //num_read = fread( src->buffer, 1, max_buf, src->stream);
        num_read = xread(src->fmtHndl, src->buffer, 1, max_buf);

        if (num_read <= 0) {
            // Insert a fake EOI marker - as per jpeglib recommendation
            src->buffer[0] = (JOCTET)0xFF;
            src->buffer[1] = (JOCTET)JPEG_EOI;
            src->bytes_in_buffer = 2;
        }
        else
            src->bytes_in_buffer = num_read;

        return (boolean)true;
    }

    static void dimjpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        my_jpeg_source_mgr* src = (my_jpeg_source_mgr*)cinfo->src;

        // `dumb' implementation from jpeglib

        // Just a dumb implementation for now.  Could use fseek() except
        // it doesn't work on pipes.  Not clear that being smart is worth
        // any trouble anyway --- large skips are infrequent.
        if (num_bytes > 0)
        {
            while (num_bytes > (long)src->bytes_in_buffer)
            {
                num_bytes -= (long)src->bytes_in_buffer;
                (void)dimjpeg_fill_input_buffer(cinfo);
                // note we assume that qt_fill_input_buffer will never return false,
                // so suspension need not be handled.
            }
            src->next_input_byte += (size_t)num_bytes;
            src->bytes_in_buffer -= (size_t)num_bytes;
        }
    }

    static void dimjpeg_term_source(j_decompress_ptr)
    {
    }

} // extern C


inline my_jpeg_source_mgr::my_jpeg_source_mgr(FormatHandle *new_hndl)
{
    jpeg_source_mgr::init_source = dimjpeg_init_source;
    jpeg_source_mgr::fill_input_buffer = dimjpeg_fill_input_buffer;
    jpeg_source_mgr::skip_input_data = dimjpeg_skip_input_data;
    jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
    jpeg_source_mgr::term_source = dimjpeg_term_source;
    fmtHndl = new_hndl;
    bytes_in_buffer = 0;
    next_input_byte = buffer;
}

//----------------------------------------------------------------------------
// READ PROC
//----------------------------------------------------------------------------

const unsigned char icc_signature[12] = { 0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45, 0x00 };

static inline boolean marker_is_icc(jpeg_saved_marker_ptr marker) {
    return marker->marker == ICC_MARKER &&
           marker->data_length >= ICC_HEADER_SIZE &&
           (memcmp(icc_signature, marker->data, sizeof(icc_signature)) == 0);
}

static bool jpeg_read_icc(bim::JpegParams *par) {
    jpeg_decompress_struct *cinfo = par->cinfo;
    jpeg_saved_marker_ptr marker;

    char marker_present[MAX_SEQ_NO + 1];	  // 1 if marker found
    unsigned int data_length[MAX_SEQ_NO + 1]; // size of profile data in marker
    unsigned int data_offset[MAX_SEQ_NO + 1]; // offset for data in marker

    // This first pass over the saved markers discovers whether there are
    // any ICC markers and verifies the consistency of the marker numbering.
    int num_markers = 0;
    int seq_no;
    for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
        marker_present[seq_no] = 0;

    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
        if (marker_is_icc(marker)) {
            if (num_markers == 0)
                num_markers = GETJOCTET(marker->data[13]);
            else if (num_markers != GETJOCTET(marker->data[13]))
                return FALSE;		// inconsistent num_markers fields
            seq_no = GETJOCTET(marker->data[12]);
            if (seq_no <= 0 || seq_no > num_markers)
                return FALSE;		// bogus sequence number
            if (marker_present[seq_no])
                return FALSE;		// duplicate sequence numbers
            marker_present[seq_no] = 1;
            data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
        }
    }

    if (num_markers == 0)
        return FALSE;

    // Check for missing markers, count total space needed,
    // compute offset of each marker's part of the data.
    unsigned int total_length = 0;
    for (seq_no = 1; seq_no <= num_markers; seq_no++) {
        if (marker_present[seq_no] == 0)
            return FALSE;		// missing sequence number
        data_offset[seq_no] = total_length;
        total_length += data_length[seq_no];
    }

    if (total_length == 0)
        return FALSE;		// found only empty markers?

    // Allocate space for assembled data
    par->buffer_icc.resize(total_length * sizeof(JOCTET));
    
    // and fill it in
    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
        if (marker_is_icc(marker)) {
            seq_no = GETJOCTET(marker->data[12]);
            JOCTET *dst_ptr = ((unsigned char*)&par->buffer_icc[0]) + data_offset[seq_no];
            JOCTET FAR *src_ptr = marker->data + ICC_OVERHEAD_LEN;
            memcpy(dst_ptr, src_ptr, data_length[seq_no]);
        }
    }
}

const unsigned char exif_signature[6] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };

static void jpeg_read_exif(bim::JpegParams *par, const unsigned char *buffer, unsigned int size) {
    if (size <= sizeof(exif_signature)) return;
    if (memcmp(exif_signature, buffer, sizeof(exif_signature)) != 0) return;

    par->buffer_exif.resize(size - sizeof(exif_signature));
    memcpy(&par->buffer_exif[0], buffer + sizeof(exif_signature), size - sizeof(exif_signature));
}

const char *adobecm_signature = "Adobe_CM";
const char *adobe2_signature = "Adobe_Photoshop2.5";
const char *adobe3_signature = "Photoshop 3.0\x0"; // 14
const char *iptc_signature = "8BIM\x04\x04\x0\x0\x0\x0"; // 10
const unsigned char iptc_magic[2] = { 0x1C, 0x01 };

static void jpeg_read_iptc(bim::JpegParams *par, const unsigned char *buffer, unsigned int size) {
    if (size < 8) return;
    if (memcmp(buffer, adobecm_signature, strlen(adobecm_signature)) == 0) return;
    //if (memcmp(buffer, adobe2_signature, strlen(adobe2_signature)) != 0) return;
    //if (memcmp(buffer, adobe3_signature, strlen(adobe3_signature)) != 0) return;

    size_t offset = 0;
    bool found = false;
    while (offset < size - 1) {
        if (memcmp(buffer + offset, iptc_magic, sizeof(iptc_magic)) == 0) {
            found = true;
            break;
        }
        offset++;
    }
    if (!found) return;

    par->buffer_iptc.resize(size - offset);
    memcpy(&par->buffer_iptc[0], buffer + offset, size - offset);
}

const char *xmp_signature = "http://ns.adobe.com/xap/1.0/";

static void jpeg_read_xmp(bim::JpegParams *par, const unsigned char *buffer, unsigned int size) {
    const size_t xmp_signature_size = strlen(xmp_signature) + 1;
    if (size <= xmp_signature_size) return;
    if (memcmp(xmp_signature, buffer, xmp_signature_size-1) != 0) return;

    par->buffer_xmp.resize(size - xmp_signature_size);
    memcpy(&par->buffer_xmp[0], buffer + xmp_signature_size, size - xmp_signature_size);
}

static void jpeg_read_comment(bim::JpegParams *par, const unsigned char *buffer, unsigned int size) {
    std::string c;
    c.resize(size+1, 0);
    memcpy(&c[0], buffer, size);
    par->comments.push_back(c);
}

static void ReadMetadata(bim::JpegParams *par) {
    jpeg_read_icc(par);

    jpeg_decompress_struct *cinfo = par->cinfo;
    jpeg_saved_marker_ptr marker;
    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
        if (marker->marker == JPEG_APP0) {
            if (memcmp(marker->data, "JFIF", 5) == 0) {
                // JFIF is handled by libjpeg already, handle JFXX
            } else if (memcmp(marker->data, "JFXX", 5) == 0) {
                //if (cinfo->saw_JFIF_marker && cinfo->JFIF_minor_version >= 2)
                //    jpeg_read_jfxx(par, marker->data, marker->data_length);
            }
        } else if (marker->marker == JPEG_COM) {
            jpeg_read_comment(par, marker->data, marker->data_length);
        } else if (marker->marker == EXIF_MARKER) {
            jpeg_read_exif(par, marker->data, marker->data_length);
            jpeg_read_xmp(par, marker->data, marker->data_length);
        } else if (marker->marker == IPTC_MARKER) {
            // IPTC/NAA or Adobe Photoshop profile
            jpeg_read_iptc(par, marker->data, marker->data_length);
        }
    }

}

bool jpegGetImageInfo(FormatHandle *fmtHndl) {
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;

    par->iod_src = new my_jpeg_source_mgr(fmtHndl);
    par->cinfo = new jpeg_decompress_struct;
    jpeg_create_decompress(par->cinfo);
    par->cinfo->src = par->iod_src;
    par->cinfo->err = jpeg_std_error(par->jerr);
    par->jerr->error_exit = my_error_exit;

    for (int m = 0; m < 16; m++)
        jpeg_save_markers(par->cinfo, JPEG_APP0 + m, 0xFFFF);

    if (!setjmp(par->jerr->setjmp_buffer)) {
        jpeg_read_header(par->cinfo, (boolean)true);
    }

    // set some image parameters
    par->i.number_pages = 1;
    par->i.depth = 8;
    par->i.width = par->cinfo->image_width;
    par->i.height = par->cinfo->image_height;
    par->i.samples = par->cinfo->num_components;

    ReadMetadata(par);
    return true;
}

static int read_jpeg_image(FormatHandle *fmtHndl) {
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;
    JSAMPROW row_pointer[1];
    ImageBitmap *image = fmtHndl->image;
    jpeg_decompress_struct *cinfo = par->cinfo;

    if (!setjmp(par->jerr->setjmp_buffer)) {
        jpeg_start_decompress(cinfo);

        if (allocImg(image, cinfo->output_width, cinfo->output_height, cinfo->output_components, 8) != 0) {
            jpeg_destroy_decompress(cinfo);
            return 1;
        }

        if (cinfo->output_components == 1) {
            while (cinfo->output_scanline < cinfo->output_height) {
                xprogress(fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG");
                if (xtestAbort(fmtHndl) == 1) break;

                row_pointer[0] = ((uchar *)image->bits[0]) + (cinfo->output_width*cinfo->output_scanline);
                (void)jpeg_read_scanlines(cinfo, row_pointer, 1);
            }
        }  else if (cinfo->output_components == 3) {
            row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];
            while (cinfo->output_scanline < cinfo->output_height) {
                xprogress(fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG");
                if (xtestAbort(fmtHndl) == 1) break;

                register unsigned int i;
                (void)jpeg_read_scanlines(cinfo, row_pointer, 1);
                uchar *row = row_pointer[0];
                uchar* pix1 = ((uchar *)image->bits[0]) + cinfo->output_width * (cinfo->output_scanline - 1);
                uchar* pix2 = ((uchar *)image->bits[1]) + cinfo->output_width * (cinfo->output_scanline - 1);
                uchar* pix3 = ((uchar *)image->bits[2]) + cinfo->output_width * (cinfo->output_scanline - 1);

                for (i = 0; i < cinfo->output_width; i++) {
                    *pix1++ = *row++;
                    *pix2++ = *row++;
                    *pix3++ = *row++;
                }

            } // while scanlines
            delete row_pointer[0];
        }
        else if (cinfo->output_components == 4) {

            row_pointer[0] = new uchar[cinfo->output_width*cinfo->output_components];

            while (cinfo->output_scanline < cinfo->output_height) {
                xprogress(fmtHndl, cinfo->output_scanline, cinfo->output_height, "Reading JPEG");
                if (xtestAbort(fmtHndl) == 1) break;

                register unsigned int i;
                (void)jpeg_read_scanlines(cinfo, row_pointer, 1);
                uchar *row = row_pointer[0];
                uchar* pix1 = ((uchar *)image->bits[0]) + cinfo->output_width * (cinfo->output_scanline - 1);
                uchar* pix2 = ((uchar *)image->bits[1]) + cinfo->output_width * (cinfo->output_scanline - 1);
                uchar* pix3 = ((uchar *)image->bits[2]) + cinfo->output_width * (cinfo->output_scanline - 1);
                uchar* pix4 = ((uchar *)image->bits[3]) + cinfo->output_width * (cinfo->output_scanline - 1);

                for (i = 0; i < cinfo->output_width; i++) {
                    *pix1++ = *row++;
                    *pix2++ = *row++;
                    *pix3++ = *row++;
                    *pix4++ = *row++;
                }

            } // while scanlines
            delete row_pointer[0];
        } // if 4 components

        jpeg_finish_decompress(cinfo);
    }

    return 0;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint jpeg_append_metadata(FormatHandle *fmtHndl, TagMap *hash) {
    if (fmtHndl == NULL) return 1;
    if (!hash) return 1;
    if (isCustomReading(fmtHndl)) return 1;
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;

    if (par->buffer_icc.size() > 0) {
        hash->set_value(bim::RAW_TAGS_ICC, par->buffer_icc, bim::RAW_TYPES_ICC);
        lcms_append_metadata(fmtHndl, hash);
    }

    // comments
    for (int i = 0; i<par->comments.size(); ++i) {
        hash->set_value(bim::CUSTOM_TAGS_PREFIX + xstring::xprintf("comment_%.3d", i), par->comments[i]);
    }

    if (par->buffer_exif.size()>0)
        hash->set_value(bim::RAW_TAGS_EXIF, par->buffer_exif, bim::RAW_TYPES_EXIF);
    if (par->buffer_xmp.size()>0)
        hash->set_value(bim::RAW_TAGS_XMP, par->buffer_xmp, bim::RAW_TYPES_XMP);
    if (par->buffer_iptc.size()>0)
        hash->set_value(bim::RAW_TAGS_IPTC, par->buffer_iptc, bim::RAW_TYPES_IPTC);
    //if (par->buffer_photoshop.size()>0)
    //    hash->set_value(bim::RAW_TAGS_PHOTOSHOP, par->buffer_photoshop, bim::RAW_TYPES_PHOTOSHOP);

    // use EXIV2 to read metadata
    exiv_append_metadata(fmtHndl, hash);

    return 0;
}

//****************************************************************************
// WRITE STUFF
//****************************************************************************

struct my_jpeg_destination_mgr : public jpeg_destination_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    FormatHandle *fmtHndl;
    JOCTET buffer[max_buf];

public:
    my_jpeg_destination_mgr(FormatHandle *new_hndl);
};

extern "C" {

    static void dimjpeg_init_destination(j_compress_ptr)
    {
    }

    static void dimjpeg_exit_on_error(j_compress_ptr cinfo, FormatHandle *fmtHndl)
    {
        // cinfo->err->msg_code = JERR_FILE_WRITE;
        xflush(fmtHndl);
        (*cinfo->err->error_exit)((j_common_ptr)cinfo);
    }

    static boolean dimjpeg_empty_output_buffer(j_compress_ptr cinfo)
    {
        my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;

        if (xwrite(dest->fmtHndl, (char*)dest->buffer, 1, max_buf) != max_buf)
            dimjpeg_exit_on_error(cinfo, dest->fmtHndl);

        xflush(dest->fmtHndl);
        dest->next_output_byte = dest->buffer;
        dest->free_in_buffer = max_buf;

        return (boolean)true;
    }

    static void dimjpeg_term_destination(j_compress_ptr cinfo)
    {
        my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
        unsigned int n = max_buf - dest->free_in_buffer;

        if (xwrite(dest->fmtHndl, (char*)dest->buffer, 1, n) != n)
            dimjpeg_exit_on_error(cinfo, dest->fmtHndl);

        xflush(dest->fmtHndl);
        dimjpeg_exit_on_error(cinfo, dest->fmtHndl);
    }

} // extern C

inline my_jpeg_destination_mgr::my_jpeg_destination_mgr(FormatHandle *new_hndl)
{
    jpeg_destination_mgr::init_destination = dimjpeg_init_destination;
    jpeg_destination_mgr::empty_output_buffer = dimjpeg_empty_output_buffer;
    jpeg_destination_mgr::term_destination = dimjpeg_term_destination;
    fmtHndl = new_hndl;
    next_output_byte = buffer;
    free_in_buffer = max_buf;
}

//----------------------------------------------------------------------------
// WRITE PROC
//----------------------------------------------------------------------------

#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

static void jpeg_write_icc(j_compress_ptr cinfo, TagMap *hash) {
    // write ICC profile
    if (hash->hasKey(bim::RAW_TAGS_ICC) && hash->get_type(bim::RAW_TAGS_ICC) == bim::RAW_TYPES_ICC) {
        unsigned int icc_data_len = hash->get_size(bim::RAW_TAGS_ICC);
        JOCTET *icc_data_ptr = (JOCTET *)hash->get_value_bin(bim::RAW_TAGS_ICC);

        int cur_marker = 1;  // per spec, counting starts at 1
        unsigned int length; // number of bytes to write in this marker

        // Calculate the number of markers we'll need, rounding up of course
        unsigned int num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;

        if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len)
            num_markers++;

        while (icc_data_len > 0) {
            // length of profile to put in this marker
            length = icc_data_len;
            if (length > MAX_DATA_BYTES_IN_MARKER)
                length = MAX_DATA_BYTES_IN_MARKER;
            icc_data_len -= length;

            // Write the JPEG marker header (APP2 code and marker length)
            jpeg_write_m_header(cinfo, ICC_MARKER,
                (unsigned int)(length + ICC_OVERHEAD_LEN));

            // Write the marker identifying string "ICC_PROFILE" (null-terminated).
            // We code it in this less-than-transparent way so that the code works
            // even if the local character set is not ASCII.
            jpeg_write_m_byte(cinfo, 0x49);
            jpeg_write_m_byte(cinfo, 0x43);
            jpeg_write_m_byte(cinfo, 0x43);
            jpeg_write_m_byte(cinfo, 0x5F);
            jpeg_write_m_byte(cinfo, 0x50);
            jpeg_write_m_byte(cinfo, 0x52);
            jpeg_write_m_byte(cinfo, 0x4F);
            jpeg_write_m_byte(cinfo, 0x46);
            jpeg_write_m_byte(cinfo, 0x49);
            jpeg_write_m_byte(cinfo, 0x4C);
            jpeg_write_m_byte(cinfo, 0x45);
            jpeg_write_m_byte(cinfo, 0x0);

            // Add the sequencing info
            jpeg_write_m_byte(cinfo, cur_marker);
            jpeg_write_m_byte(cinfo, (int)num_markers);

            // Add the profile data
            while (length--) {
                jpeg_write_m_byte(cinfo, *icc_data_ptr);
                icc_data_ptr++;
            }
            cur_marker++;
        }
    }
}

static void WriteMetadata(j_compress_ptr cinfo, TagMap *hash) {
    jpeg_write_icc(cinfo, hash);

}

static int write_jpeg_image(FormatHandle *fmtHndl) {
    ImageBitmap *image = fmtHndl->image;
    bim::JpegParams *par = (bim::JpegParams *) fmtHndl->internalParams;

    struct jpeg_compress_struct cinfo;
    JSAMPROW row_pointer[1];
    row_pointer[0] = 0;

    struct my_jpeg_destination_mgr *iod_dest = new my_jpeg_destination_mgr(fmtHndl);
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);

    jerr.error_exit = my_error_exit;

    if (!setjmp(jerr.setjmp_buffer))
    {
        jpeg_create_compress(&cinfo);
        cinfo.dest = iod_dest;

        cinfo.image_width = image->i.width;
        cinfo.image_height = image->i.height;


        LUT* cmap = 0;
        bool gray = true;

        if (image->i.samples == 1)
        {
            cinfo.input_components = 1;
            cinfo.in_color_space = JCS_GRAYSCALE;
            gray = true;
        }
        else
        {
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
            gray = false;
        }

        if (image->i.depth < 8)
        {
            cmap = &image->i.lut;
            gray = true;
            if (cmap->count > 0) gray = false;
            cinfo.input_components = gray ? 1 : 3;
            cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
        }

        jpeg_set_defaults(&cinfo);
        int quality = fmtHndl->quality;
        if (quality < 1) quality = 1;
        if (quality > 100) quality = 100;
        jpeg_set_quality(&cinfo, quality, (boolean)true); // limit to baseline-JPEG values );

        if (fmtHndl->order == 1) jpeg_simple_progression(&cinfo);

        jpeg_start_compress(&cinfo, (boolean)true);

        WriteMetadata(&cinfo, fmtHndl->metaData);

        row_pointer[0] = new uchar[cinfo.image_width*cinfo.input_components];
        int w = cinfo.image_width;
        long lineSizeBytes = getLineSizeInBytes(image);

        while (cinfo.next_scanline < cinfo.image_height)
        {
            uchar *row = row_pointer[0];

            /*
            switch ( image.depth() )
            {
            case 1:
            if (gray)
            {
            uchar* data = image.scanLine(cinfo.next_scanline);
            if ( image.bitOrder() == QImage::LittleEndian )
            {
            for (int i=0; i<w; i++)
            {
            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
            row[i] = qRed(cmap[bit]);
            }
            }
            else
            {
            for (int i=0; i<w; i++)
            {
            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
            row[i] = qRed(cmap[bit]);
            }
            }
            }
            else
            {
            uchar* data = image.scanLine(cinfo.next_scanline);
            if ( image.bitOrder() == QImage::LittleEndian )
            {
            for (int i=0; i<w; i++)
            {
            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
            *row++ = qRed(cmap[bit]);
            *row++ = qGreen(cmap[bit]);
            *row++ = qBlue(cmap[bit]);
            }
            }
            else
            {
            for (int i=0; i<w; i++)
            {
            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
            *row++ = qRed(cmap[bit]);
            *row++ = qGreen(cmap[bit]);
            *row++ = qBlue(cmap[bit]);
            }
            }
            }

            break;
            */


            //if 4 bits per sample, there should be only one sample
            if (image->i.depth == 4)
            {
                if (gray)
                {
                    uchar* pix = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
                    uchar pixH, pixL;
                    for (int i = 0; i < lineSizeBytes; i++)
                    {
                        pixH = (unsigned char)((*pix) << 4);
                        pixL = (unsigned char)((*pix) >> 4);
                        *row++ = pixH;
                        if (i + 1 < w) *row++ = pixL;
                        pix++;
                    }
                } // if one sample with 8 bits
                else
                {
                    uchar pixH, pixL;
                    uchar* pix = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;

                    for (int i = 0; i < lineSizeBytes; i++)
                    {
                        pixH = (unsigned char)((*pix) << 4);
                        pixL = (unsigned char)((*pix) >> 4);
                        *row++ = (unsigned char)xR(cmap->rgba[pixH]);
                        *row++ = (unsigned char)xG(cmap->rgba[pixH]);
                        *row++ = (unsigned char)xB(cmap->rgba[pixH]);
                        if (i + 1 < w)
                        {
                            *row++ = (unsigned char)xR(cmap->rgba[pixL]);
                            *row++ = (unsigned char)xG(cmap->rgba[pixL]);
                            *row++ = (unsigned char)xB(cmap->rgba[pixL]);
                        }
                        pix++;
                    }
                } // if paletted image
            } // 4 bits per sample


            //if 8 bits per sample
            if (image->i.depth == 8)
            {
                if (gray)
                {
                    uchar* pix = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;

                    memcpy(row, pix, w);
                    row += w;
                } // if one sample with 8 bits
                else
                {
                    if (image->i.samples == 1)
                    {
                        uchar* pix = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
                        for (int i = 0; i < w; i++)
                        {
                            *row++ = (unsigned char)xR(cmap->rgba[*pix]);
                            *row++ = (unsigned char)xG(cmap->rgba[*pix]);
                            *row++ = (unsigned char)xB(cmap->rgba[*pix]);
                            pix++;
                        }
                    } // if paletted image

                    if (image->i.samples == 2)
                    {
                        uchar* pix1 = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
                        uchar* pix2 = ((uchar *)image->bits[1]) + lineSizeBytes * cinfo.next_scanline;
                        for (int i = 0; i < w; i++)
                        {
                            *row++ = *pix1;
                            *row++ = *pix2;
                            *row++ = 0;
                            pix1++; pix2++;
                        }
                    } // if 2 samples

                    if (image->i.samples >= 3)
                    {
                        uchar* pix1 = ((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline;
                        uchar* pix2 = ((uchar *)image->bits[1]) + lineSizeBytes * cinfo.next_scanline;
                        uchar* pix3 = ((uchar *)image->bits[2]) + lineSizeBytes * cinfo.next_scanline;
                        for (int i = 0; i < w; i++)
                        {
                            *row++ = *pix1;
                            *row++ = *pix2;
                            *row++ = *pix3;
                            pix1++; pix2++; pix3++;
                        }
                    } // if 3 or more samples
                } // if not gray
            } // 8 bits per sample


            //if 16 bits per sample
            if (image->i.depth == 16)
            {
                if (image->i.samples == 1)
                {
                    uint16* pix = (uint16*)(((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline);

                    for (int i = 0; i < w; i++)
                    {
                        *row++ = (uchar)(*pix / 256);
                        ++pix;
                    }
                } // if paletted image

                if (image->i.samples == 2)
                {
                    uint16* pix1 = (uint16*)(((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline);
                    uint16* pix2 = (uint16*)(((uchar *)image->bits[1]) + lineSizeBytes * cinfo.next_scanline);
                    for (int i = 0; i < w; i++)
                    {
                        *row++ = (uchar)(*pix1 / 256);
                        *row++ = (uchar)(*pix2 / 256);
                        *row++ = 0;
                        ++pix1; ++pix2;
                    }
                } // if 2 samples

                if (image->i.samples >= 3)
                {
                    uint16* pix1 = (uint16*)(((uchar *)image->bits[0]) + lineSizeBytes * cinfo.next_scanline);
                    uint16* pix2 = (uint16*)(((uchar *)image->bits[1]) + lineSizeBytes * cinfo.next_scanline);
                    uint16* pix3 = (uint16*)(((uchar *)image->bits[2]) + lineSizeBytes * cinfo.next_scanline);
                    for (int i = 0; i < w; i++)
                    {
                        *row++ = (uchar)(*pix1 / 256);
                        *row++ = (uchar)(*pix2 / 256);
                        *row++ = (uchar)(*pix3 / 256);
                        ++pix1; ++pix2; ++pix3;
                    }
                } // if 3 or more samples

            } // 16 bits per sample

            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
    }

    delete iod_dest;
    delete row_pointer[0];

    return 0;
}


