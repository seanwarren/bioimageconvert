/*****************************************************************************
FFMPEG support 
Copyright (c) 2008-2012 by Dmitry V. Fedorov <www.dimin.net> <dima@dimin.net>

IMPLEMENTATION

Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
2008-02-01 14:45 - First creation

Ver : 2
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <cstring>

#include <string>
#include <set>
#include <list>
#include <utility>
#include <algorithm>

#include <xstring.h>
#include "tag_map.h"
#include <bim_metatags.h>

// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
#pragma warning(disable:4996)
#endif 

#include "bim_ffmpeg_format.h"
//#include "bim_ffmpeg_format_io.cpp"

using namespace bim;

#define BIM_FORMAT_FFMPEG_MPEG       0
#define BIM_FORMAT_FFMPEG_VCD        0
#define BIM_FORMAT_FFMPEG_MPEG1      0
#define BIM_FORMAT_FFMPEG_MPEG2      1
#define BIM_FORMAT_FFMPEG_MPEG4      2
#define BIM_FORMAT_FFMPEG_MJPEG      3
#define BIM_FORMAT_FFMPEG_MJ2        4
#define BIM_FORMAT_FFMPEG_AVI        5
#define BIM_FORMAT_FFMPEG_WMV        6
#define BIM_FORMAT_FFMPEG_QT         7
#define BIM_FORMAT_FFMPEG_FLASH      8
#define BIM_FORMAT_FFMPEG_OGG        9
#define BIM_FORMAT_FFMPEG_MATROSKA   10
#define BIM_FORMAT_FFMPEG_DV         11
#define BIM_FORMAT_FFMPEG_FLV        12
#define BIM_FORMAT_FFMPEG_H264       13
#define BIM_FORMAT_FFMPEG_WEBM       14
#define BIM_FORMAT_FFMPEG_H265       15


//****************************************************************************
// internals
//****************************************************************************

struct HeaderClass {
    const int  offset;
    const int  size;
    const char *magic;
}; 

inline bool headcmp( const HeaderClass &h, unsigned char *magic ) {
  return memcmp(h.magic, &magic[h.offset], h.size)==0;
}

std::vector< std::string > init_formats_write() {
    std::vector< std::string > v;
    v.push_back( "mpeg" );       // 0 "MPEG",  - MPEG1VIDEO
    v.push_back( "mpeg2video" ); // 1 "MPEG2", - MPEG2VIDEO m2v
    v.push_back( "mp4" );        // 2 "MPEG4", - MPEG4
    v.push_back( "mjpeg" );      // 3 "MJEPG", - MJPEG
    v.push_back( "mjpeg2000" );  // 4 "MJEPG2000",
    v.push_back( "avi" );        // 5 "AVI",   - XVID
    v.push_back( "asf" );        // 6 "WMV",   - WMV3
    v.push_back( "mov" );        // 7 "QT", // "mov,mp4,m4a,3gp,3g2,mj2"
    v.push_back( "swf" );        // 8 "FLASH",
    v.push_back( "ogg" );        // 9 "Ogg format",
    v.push_back( "matroska" );   // 10 "Matroska File Format",
    v.push_back( "dv" );         // 11 "DV video format",
    v.push_back( "flv" );        // 12 "FLV",
    v.push_back( "mp4" );        // 13 "H264",
    v.push_back( "webm" );       // 14 "WebM",
    v.push_back( "mp4" );        // 15 "H265",
    return v;
}

std::vector< std::string > init_formats_fourcc() {
    std::vector< std::string > v;
    v.push_back( "" );     // 0 "MPEG",  - MPEG1VIDEO
    v.push_back( "" );     // 1 "MPEG2", - MPEG2VIDEO m2v
    v.push_back( "" );     // 2 "MPEG4", - MPEG4
    v.push_back( "" );     // 3 "MJEPG", - MJPEG
    v.push_back( "" );     // 4 "MJEPG2000",
    v.push_back( "" );     // 5 "AVI",   - AV_CODEC_ID_MPEG4 // formats_fourcc.push_back( "XVID" );
    v.push_back( "" );     // 6 "WMV",   - WMV3
    v.push_back( "" );     // 7 "QT", // "mov,mp4,m4a,3gp,3g2,mj2"
    v.push_back( "" );     // 8 "FLASH",
    v.push_back( "" );     // 9 "Ogg format",
    v.push_back( "" );     // 10 "Matroska File Format", // formats_fourcc.push_back( "XVID" );
    v.push_back( "" );     // 11 "DV video format",
    v.push_back( "" );     // 12 "Flash video",
    v.push_back( "" );     // 13 "H264",
    v.push_back( "" );     // 14 "WebM",
    v.push_back( "" );     // 15 "H265",
    return v;
}

std::vector< AVCodecID > init_encoder_ids() {
    std::vector< AVCodecID > v;
    // AV_CODEC_ID_NONE indicates use of ffmpeg default codec
    // for some formats we would like to force the codec
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_MPEG1VIDEO ); // 0 "MPEG",  - CODEC_ID_MPEG1VIDEO
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_MPEG2VIDEO ); // 1 "MPEG2", - MPEG2VIDEO
    v.push_back( AV_CODEC_ID_MPEG4 );  //v.push_back( CODEC_ID_MPEG4 );      // 2 "MPEG4", - AV_CODEC_ID_MPEG4
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_MJPEG );      // 3 "MJEPG", - MJPEG
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( AV_CODEC_ID_NONE );       // 4 "MJEPG2000",
    v.push_back( AV_CODEC_ID_NONE );  // 5 "AVI", // v.push_back( CODEC_ID_MPEG4 );
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_MSMPEG4V3 );  // 6 "WMV",   - WMV3
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_MPEG4 );      // 7 "QT",
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_FLV1 );       // 8 "FLASH",
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_THEORA );     // 9 "Ogg format",
    v.push_back( AV_CODEC_ID_NONE );  // 10 "Matroska File Format", //v.push_back( CODEC_ID_MPEG4 );
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_DVVIDEO );    // 11 "DV video format",
    v.push_back( AV_CODEC_ID_NONE );  //v.push_back( CODEC_ID_FLV1 );       // 12 "Flash Video",
    v.push_back( AV_CODEC_ID_H264 );  // 13 "MPEG4 H264",
    v.push_back( AV_CODEC_ID_NONE );  // 14 "WebM default VP8 or VP9
    v.push_back( AV_CODEC_ID_H265 );  // 15 "MPEG4 H265",
    return v;
}

std::map< std::string, int > init_formats() {
    std::map< std::string, int > v;
    v.insert( std::make_pair( "mpeg", 0 ) );
    v.insert( std::make_pair( "mpegvideo", 0 ) );
    v.insert( std::make_pair( "mpeg1video", 0 ) );
    v.insert( std::make_pair( "mpegts", 1 ) );
    v.insert( std::make_pair( "mpegtsraw", 1 ) );
    v.insert( std::make_pair( "mpeg2video", 1 ) );
    v.insert( std::make_pair( "m4v", 2 ) );
    v.insert( std::make_pair( "h264", 2 ) );
    v.insert( std::make_pair( "mjpeg", 3 ) );
    v.insert( std::make_pair( "ingenient", 3 ) );
    v.insert( std::make_pair( "avi", 5 ) );
    v.insert( std::make_pair( "asf", 6 ) );
    v.insert( std::make_pair( "mov,mp4,m4a,3gp,3g2,mj2", 7 ) ); 
    v.insert( std::make_pair( "swf", 8 ) );
    v.insert( std::make_pair( "ogg", 9 ) );
    v.insert( std::make_pair( "matroska", 10 ) );
    v.insert( std::make_pair( "dv", 11 ) );
    v.insert( std::make_pair( "flv", 12 ) );
    v.insert( std::make_pair( "h264", 13 ) );
    v.insert( std::make_pair( "webm", 14 ) );
    v.insert( std::make_pair( "h265", 15 ) );
    return v;
}

std::vector<int> init_bitrate_scaler() {
    std::vector<int> v;
    // define bitrate scaler in an attemp to equalize formats
    v.push_back(1);    // 0 "MPEG"
    v.push_back(1);    // 1 "MPEG2"
    v.push_back(10);   // 2 "MPEG4"
    v.push_back(1);    // 3 "MJEPG"
    v.push_back(1);    // 4 "MJEPG2000"
    v.push_back(10);   // 5 "AVI"
    v.push_back(10);   // 6 "WMV"
    v.push_back(100);  // 7 "QT",
    v.push_back(10);   // 8 "FLASH",
    v.push_back(10);   // 9 "Ogg format"
    v.push_back(100);  // 10 "Matroska File Format"
    v.push_back(1);    // 11 "DV video format"
    v.push_back(10);   // 12 "Flash Video"
    v.push_back(100);  // 13 "MPEG4 H264"
    v.push_back(10);   // 14 "WebM VP8"
    v.push_back(100);  // 15 "MPEG4 H265"
    return v;
}

std::vector<int> init_default_bitrates() {
    std::vector<int> v;
    // define bitrate scaler in an attemp to equalize formats
    v.push_back(0);    // 0 "MPEG"
    v.push_back(0);    // 1 "MPEG2"
    v.push_back(0);    // 2 "MPEG4"
    v.push_back(0);    // 3 "MJEPG"
    v.push_back(0);    // 4 "MJEPG2000"
    v.push_back(0);    // 5 "AVI"
    v.push_back(0);    // 6 "WMV"
    v.push_back(1000000);  // 7 "QT",
    v.push_back(0);    // 8 "FLASH",
    v.push_back(0);   // 9 "Ogg format"
    v.push_back(1000000);  // 10 "Matroska File Format"
    v.push_back(0);    // 11 "DV video format"
    v.push_back(0);    // 12 "Flash Video"
    v.push_back(1000000);  // 13 "MPEG4 H264"
    v.push_back(0);   // 14 "WebM VP8"
    v.push_back(1000000);  // 15 "MPEG4 H265"
    return v;
}

std::map< int, float > init_fps_overrides() {
    std::map<int, float > v;
    v.insert(std::make_pair(7, 0.0));
    v.insert(std::make_pair(13, 0.0));
    v.insert(std::make_pair(15, 0.0));
    return v;
}

std::map< std::string, int > init_codecs() {
    std::map< std::string, int > v;
    // if format case is ambiguous, use codecs
    v.insert( std::make_pair( "mpeg1video", 0 ) );
    v.insert( std::make_pair( "mpeg2video", 1 ) );
    v.insert( std::make_pair( "mpeg4", 2 ) );
    return v;
}

std::vector< std::string >   FFMpegParams::formats_write  = init_formats_write();
std::vector< std::string >   FFMpegParams::formats_fourcc = init_formats_fourcc();
std::vector< AVCodecID >     FFMpegParams::encoder_ids    = init_encoder_ids();
std::map< std::string, int > FFMpegParams::formats        = init_formats();
std::map< std::string, int > FFMpegParams::codecs         = init_codecs();
std::vector<int>             FFMpegParams::format_bitrate_scaler = init_bitrate_scaler();
std::vector<int>             FFMpegParams::default_bitrates = init_default_bitrates();
std::map< int, float >       FFMpegParams::fps_overrides = init_fps_overrides();

//----------------------------------------------------------------------------
// supported headers
//----------------------------------------------------------------------------

//#define BIM_FORMAT_FFMPEG_MAGIC_SIZE 40
#define BIM_FORMAT_FFMPEG_MAGIC_SIZE 200


static const HeaderClass header_mpeg_1 = { 0, 3, "\x00\x00\x00" };
static const HeaderClass header_mpeg_2 = { 4, 4, "ftyp" };

static const HeaderClass header_vob = { 0, 3, "\x00\x00\x01" };

static const HeaderClass header_m2t_1a = { 0, 1, "\x47" };
static const HeaderClass header_m2t_1b = { 188, 1, "\x47" };

static const HeaderClass header_m2t_2a = { 4, 1, "\x47" };
static const HeaderClass header_m2t_2b = { 196, 1, "\x47" };

static const HeaderClass header_avi_1 = { 0, 4, "RIFF" };
static const HeaderClass header_avi_2 = { 8, 3, "AVI" };
static const HeaderClass headers_avi[] = {
    { 0, 8, "RIFFAVI " },
    { 0, 8, "RIFFAVIX" },
    { 0, 8, "RIFFAVI\x19" },
    { 0, 8, "ON2 ON2f" },
    { 0, 8, "RIFFAMV " },
};

static const HeaderClass headers_qt[] = {
    { 4, 4, "moov" },
    { 4, 4, "free" },
    { 4, 4, "mdat" },
    { 4, 4, "wide" },
    { 4, 4, "pnot" },
    { 4, 4, "skip" },
    { 4, 6, "ftypqt" },
    { 24, 4, "wide" }
};

static const HeaderClass header_vcd = { 0, 8, "ENTRYVCD" };

static const HeaderClass header_wmv = { 0, 16, "\x30\x26\xb2\x75\x8e\x66\xcf\x11\xa6\xd9\x00\xaa\x00\x62\xce\x6c" };

static const HeaderClass header_mj2 = { 0, 12, "\x00\x00\x00\x0C\x6a\x50\x20\x20\x0D\x0A\x87\x0A" };

static const HeaderClass header_ogg = { 0, 4, "OggS" };

static const HeaderClass header_matroska = { 0, 4, "\x1a\x45\xdf\xa3" };

static const HeaderClass header_webm = { 31, 4, "webm" };

static const HeaderClass headers_flash[] = {
    { 0, 3, "FWS" },
    { 0, 3, "CWS" },
};

static const HeaderClass headers_flv[] = {
    { 0, 3, "FLV" },
    { 0, 8, "\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1" }
};


//****************************************************************************
// FORMAT DEMANDED FUNTIONS
//****************************************************************************

int ffMpegValidateFormatProc (BIM_MAGIC_STREAM *magic, bim::uint length, const bim::Filename fileName) {
  if (length < BIM_FORMAT_FFMPEG_MAGIC_SIZE) return -1;
  unsigned char *mag_num = (unsigned char *) magic;

  // BIM_FORMAT_FFMPEG_MPEG - BIM_FORMAT_FFMPEG_MPEG4
  //Hex: 00 00 01 Bx : VOB
  if (headcmp(header_vob, mag_num) && mag_num[3]>>4==0xB) return BIM_FORMAT_FFMPEG_MPEG;

  //00 00 00 XX 66 74 79 70 : MPEG4, M4V
  if (headcmp(header_mpeg_1, mag_num) && 
      headcmp(header_mpeg_2, mag_num)) return BIM_FORMAT_FFMPEG_MPEG4;

  // VCD
  if (headcmp(header_vcd, mag_num)) return BIM_FORMAT_FFMPEG_VCD;

  // AVI
  for (int i=0; i<5; i++)
    if (headcmp(headers_avi[i], mag_num)) return BIM_FORMAT_FFMPEG_AVI;

  if (headcmp(header_avi_1, mag_num) &&
      headcmp(header_avi_2, mag_num)) return BIM_FORMAT_FFMPEG_AVI;

  // WMV
  if (headcmp(header_wmv, mag_num)) return BIM_FORMAT_FFMPEG_WMV;

  // Quick Time
  for (int i=0; i<8; i++)
    if (headcmp(headers_qt[i], mag_num)) return BIM_FORMAT_FFMPEG_QT;

  // Adobe Flash
  for (int i=0; i<2; i++)
    if (headcmp(headers_flash[i], mag_num)) return BIM_FORMAT_FFMPEG_FLASH;
  
  // Adobe Flash video
  for (int i=0; i<2; i++)
    if (headcmp(headers_flv[i], mag_num)) return BIM_FORMAT_FFMPEG_FLV;

  // MJ2
  if (headcmp(header_mj2, mag_num)) return BIM_FORMAT_FFMPEG_MJ2;

  // OGG
  if (headcmp(header_ogg, mag_num)) return BIM_FORMAT_FFMPEG_OGG;

  // Matroska
  if (headcmp(header_matroska, mag_num)) return BIM_FORMAT_FFMPEG_MATROSKA;

  // DV ?


  //2E 52 4D 46	 	.RMF
  //RM, RMVB	 	RealMedia streaming media file

  // WebM 
  if (headcmp(header_webm, mag_num)) return BIM_FORMAT_FFMPEG_WEBM;

  // mpeg2 stream M2T
  if (headcmp(header_m2t_1a, mag_num) && headcmp(header_m2t_1b, mag_num)) return BIM_FORMAT_FFMPEG_MPEG2;
  if (headcmp(header_m2t_2a, mag_num) && headcmp(header_m2t_2b, mag_num)) return BIM_FORMAT_FFMPEG_MPEG2;

  return -1;
}

bool ffmpegGetImageInfo( FormatHandle *fmtHndl ) {

  if (fmtHndl == NULL) return false;
  if (fmtHndl->internalParams == NULL) return false;
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  
  *info = initImageInfo();

  if (par->ff_in.numFrames() <= 0) return false;

  info->width        = par->ff_in.width();
  info->height       = par->ff_in.height();
  info->depth        = 8;
  info->samples      = par->ff_in.depth();
  info->number_pages = par->ff_in.numFrames();
  info->number_t     = info->number_pages;
  info->number_z     = 1;
  info->imageMode    = IM_RGB;
  info->pixelType    = FMT_UNSIGNED;

  const char *fmt_name = par->ff_in.formatName();
  const char *cdc_name = par->ff_in.codecName();  

  std::map<std::string, int>::const_iterator it = FFMpegParams::formats.find( fmt_name );
  if (it != FFMpegParams::formats.end()) {
    fmtHndl->subFormat = (*it).second;
  }

  return true;
}

FormatHandle ffMpegAquireFormatProc( void ) {
  FormatHandle fp = initFormatHandle();
  return fp;
}

void ffMpegCloseImageProc (FormatHandle *fmtHndl);
void ffMpegReleaseFormatProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  ffMpegCloseImageProc ( fmtHndl );  
}


//----------------------------------------------------------------------------
// OPEN/CLOSE
//----------------------------------------------------------------------------
void ffMpegSetWriteParameters  (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;  
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;

  par->frame_sizes_set = false;
  par->fps = 0.0f;
  par->bitrate = 0;

  if (!fmtHndl->options) return;
  xstring str = fmtHndl->options;
  std::vector<xstring> options = str.split( " " );
  if (options.size() < 1) return;
  
  int i = -1;
  while (i<(int)options.size()-1) {
    i++;

    if ( options[i]=="fps" && options.size()-i>0 ) {
      i++;
      par->fps = (float) options[i].toDouble( 0.0 );
      continue;
    }

    if ( options[i]=="bitrate" && options.size()-i>0 ) {
      i++;
      par->bitrate = options[i].toInt( 0 );
      continue;
    }
  } // while

}

void ffMpegCloseImageProc (FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return;
  if (fmtHndl->internalParams == NULL) return;  
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  fmtHndl->internalParams = NULL;
  par->ff_in.close();
  par->ff_out.close();
  delete par;
}

bim::uint ffMpegOpenImageProc  (FormatHandle *fmtHndl, ImageIOModes io_mode) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams != NULL) ffMpegCloseImageProc (fmtHndl);  
  
  FFMpegParams *par = new FFMpegParams();
  fmtHndl->internalParams = (void *) par;

  if ( io_mode == IO_READ ) {
    VideoIO::KeyValueMap kvm;
    std::string filename = fmtHndl->fileName; // ffmpeg understands utf-8 encoded unicode file names
    kvm["filename"] = filename;
    par->ff_in.setConvertToMatlab(false);
    try {
        par->ff_in.open( kvm );
    } catch(...) {
        ffMpegCloseImageProc (fmtHndl); 
        return 1;
    }
    if ( !ffmpegGetImageInfo( fmtHndl ) ) { 
        ffMpegCloseImageProc (fmtHndl); 
        return 1; 
    };
  }

  if ( io_mode == IO_WRITE ) {
    ffMpegSetWriteParameters(fmtHndl);
  }

  return 0;
}


//----------------------------------------------------------------------------
// INFO for OPEN image
//----------------------------------------------------------------------------

bim::uint ffMpegGetNumPagesProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 0;
  if (fmtHndl->internalParams == NULL) return 0;
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;    
  return (bim::uint) info->number_pages;
}

ImageInfo ffMpegGetImageInfoProc ( FormatHandle *fmtHndl, bim::uint /*page_num*/ ) {
  ImageInfo ii = initImageInfo();
  if (fmtHndl == NULL) return ii;
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  return par->i;
}

//----------------------------------------------------------------------------
// METADATA
//----------------------------------------------------------------------------

bim::uint ffMpegAddMetaDataProc (FormatHandle *fmtHndl) {
  fmtHndl=fmtHndl;
  return 1;
}


bim::uint ffMpegReadMetaDataProc (FormatHandle *fmtHndl, bim::uint page, int group, int tag, int type) {
  fmtHndl; page; group; tag; type;
  return 1;
}

char* ffMpegReadMetaDataAsTextProc ( FormatHandle * /*fmtHndl*/ ) {
  return NULL;
}


//----------------------------------------------------------------------------
// READ/WRITE
//----------------------------------------------------------------------------

int read_ffmpeg_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  ImageInfo *info = &par->i;  
  ImageBitmap *img = fmtHndl->image;

  // init the image
  if ( allocImg( fmtHndl, info, img) != 0 ) return 1;
  unsigned int page_size = getImgSizeInBytes(img);

  bool res = false;
  try {
    res = par->ff_in.seek(fmtHndl->pageNumber);
  } catch (...) {
    return 1;
  }

  if ( res ) {
    const std::vector<unsigned char> *frame = par->ff_in.currBGR();
    const unsigned char *fbuf = &(*frame)[0];

    if (frame->size() >= page_size)
    for ( bim::uint c=0; c<info->samples; ++c ) {
      const unsigned char *pI = fbuf + (info->samples-c-1);
      unsigned char *pO = (unsigned char *) img->bits[c];

      for ( int y=0; y<info->height; ++y ) {

        xprogress( fmtHndl, y, info->height, "Reading Video" );
        if ( xtestAbort( fmtHndl ) == 1) break;  

        for ( unsigned long x=0; x<info->width; ++x ) {
          *pO = *pI;
          ++pO;
          pI+=info->samples;
        } // for x
      } // for y
    }// c
  } // if could seek to the page

  return 0;
}

int write_ffmpeg_image(FormatHandle *fmtHndl) {
  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  ImageBitmap *img = fmtHndl->image;
  ImageInfo *info = &img->i;

  if (!par->frame_sizes_set) {

    std::string filename(fmtHndl->fileName);  // ffmpeg understands utf-8 encoded unicode file names
    par->ff_out.setWidth( (int) info->width );
    par->ff_out.setHeight( (int) info->height );

    par->ff_out.setFormat( FFMpegParams::formats_write[ fmtHndl->subFormat ] );

    // use our default codec
    par->ff_out.setCodec( FFMpegParams::encoder_ids[ fmtHndl->subFormat ] );

    // for some formats set fourcc to non default one, e.g. XVID instead of default FMP4 for better compatibility of AVI
    par->ff_out.setFourCC( FFMpegParams::formats_fourcc[ fmtHndl->subFormat ] );

    // some formats do not support fps
    if (FFMpegParams::fps_overrides.count(fmtHndl->subFormat) > 0) {
        std::map<int, float>::const_iterator it = FFMpegParams::fps_overrides.find(fmtHndl->subFormat);
        if (it != FFMpegParams::fps_overrides.end())
            par->fps = (*it).second;
    }
    if (par->fps > 0) 
        par->ff_out.setFramesPerSecond(par->fps);

    // for some formats default bitrate produces poor output, adjust
    if (par->bitrate <= 0)
        par->bitrate = FFMpegParams::default_bitrates[fmtHndl->subFormat];
    if (par->bitrate > 0) 
        par->ff_out.setBitRate(par->bitrate * FFMpegParams::format_bitrate_scaler[fmtHndl->subFormat]);

    VideoIO::KeyValueMap kvm;
    kvm["filename"] = filename; // ffmpeg understands utf-8 encoded unicode file names
    kvm["depth"]    = "3";
    
    /*
    assnString(kvm["bitRate"],          getBitRate());
    assnString(kvm["bitRateTolerance"], getBitRateTolerance());
    assnString(kvm["gopSize"],          getGopSize());
    assnString(kvm["maxBFrames"],       getMaxBFrames());
    */
    par->ff_out.setup( kvm );    
    //par->ff_out.open( filename ); // dima: open is done in setup
    if (!par->ff_out.isOpen()) return 1;

    // finalize opening file and init raw frame
    try {
        par->ff_out.initFromRawFrame((int)info->width, (int)info->height, 3);
    }
    catch (...) {
        return 1;
    }

    par->frame_sizes_set = true;
  }

  AVFrame *frame = par->ff_out.rawFrame();
  unsigned char *fbuf = frame->data[0];

  // write data into the raw frame
  int channels = std::min<int>( info->samples, 3 );
  if (channels<3)
    memset(fbuf, 0, info->width*info->height*3);
  for ( int c=0; c<channels; ++c ) {
    unsigned char *pI = fbuf + c;
    unsigned char *pO = (unsigned char *) img->bits[c];

    for ( int y=0; y<info->height; ++y ) {

      xprogress( fmtHndl, y, info->height, "Writing video" );
      if ( xtestAbort( fmtHndl ) == 1) break;  

      for ( unsigned long x=0; x<info->width; ++x ) {
        *pI = *pO;
        ++pO;
        pI+=3;
      } // for x
    } // for y
  }// c

  try {
    par->ff_out.addFromRawFrame( (int) info->width, (int) info->height, 3 );
  } catch (...) {
    return 1;
  }

  return 0;
}


bim::uint ffMpegReadImageProc  ( FormatHandle *fmtHndl, bim::uint page ) {
  if (fmtHndl == NULL) return 1;
  fmtHndl->pageNumber = page;
  return read_ffmpeg_image( fmtHndl );
}

bim::uint ffMpegWriteImageProc ( FormatHandle *fmtHndl ) {
  if (fmtHndl == NULL) return 1;
  return write_ffmpeg_image( fmtHndl );
}

//----------------------------------------------------------------------------
// META DATA PROC
//----------------------------------------------------------------------------

bim::uint ffmpeg_append_metadata (FormatHandle *fmtHndl, TagMap *hash ) {

  if (fmtHndl == NULL) return 1;
  if (fmtHndl->internalParams == NULL) return 1;
  if (!hash) return 1;

  FFMpegParams *par = (FFMpegParams *) fmtHndl->internalParams;
  hash->append_tag( bim::VIDEO_FORMAT_NAME,             par->ff_in.formatName() );
  hash->append_tag( bim::VIDEO_CODEC_NAME,              par->ff_in.codecName() );
  hash->append_tag( bim::VIDEO_FRAMES_PER_SECOND,       par->ff_in.fps() );
  hash->append_tag( bim::PIXEL_RESOLUTION_T,      (1.0/(double)par->ff_in.fps()) );
  hash->append_tag( bim::PIXEL_RESOLUTION_UNIT_T, bim::PIXEL_RESOLUTION_UNIT_SECONDS );

  return 0;
}


//****************************************************************************
//
// EXPORTED FUNCTION
//
//****************************************************************************

#define BIM_FFMPEG_NUM_FORMATS 16

FormatItem ffMpegItems[BIM_FFMPEG_NUM_FORMATS] = {
  {
    "MPEG",      // short name, no spaces
      "MPEG I", // Long format name
      "mpg|mpeg|m1v",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "MPEG2",            // short name, no spaces
      "MPEG 2", // Long format name
      "mpg|mpeg|m2v|m2t",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "MPEG4",            // short name, no spaces
      "MPEG4 H.263", // Long format name
      "m4v",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "MJPEG",            // short name, no spaces
      "Motion JPEG", // Long format name
      "mjpeg|mjp|mjpg",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "MJPEG2000",            // short name, no spaces
      "Motion JPEG-2000", // Long format name
      "mj2|mjp2",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      0, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      0, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "AVI",            // short name, no spaces
      "Microsoft Windows AVI", // Long format name
      "avi",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "WMV",            // short name, no spaces
      "Microsoft Windows Media Video", // Long format name
      "wmv|asf",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "QuickTime",            // short name, no spaces
      "Apple QuickTime", // Long format name
      "mov",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "Flash",            // short name, no spaces
      "Adobe Flash", // Long format name
      "swf",        // pipe "|" separated supported extension list
      0, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "OGG",            // short name, no spaces
      "OGG Theora Video Format", // Long format name
      "ogv|ogg",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "Matroska",            // short name, no spaces
      "Matroska Multimedia Container", // Long format name
      "mkv",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "DV",            // short name, no spaces
      "Digital Video", // Long format name
      "dv",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      0, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      0, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "FLV",            // short name, no spaces
      "Adobe Flash Video", // Long format name
      "flv",        // pipe "|" separated supported extension list
      0, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "H264",            // short name, no spaces
      "MPEG4 AVC H.264", // Long format name
      "mp4",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }, {
    "WEBM",            // short name, no spaces
      "WebM VP8", // Long format name
      "webm",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 }  
  }, {
    "H265",            // short name, no spaces
      "MPEG4 AVC H.265", // Long format name
      "mp4",        // pipe "|" separated supported extension list
      1, //canRead;      // 0 - NO, 1 - YES
      1, //canWrite;     // 0 - NO, 1 - YES
      0, //canReadMeta;  // 0 - NO, 1 - YES
      0, //canWriteMeta; // 0 - NO, 1 - YES
      1, //canWriteMultiPage;   // 0 - NO, 1 - YES
      //TDivFormatConstrains constrains ( w, h, pages, minsampl, maxsampl, minbitsampl, maxbitsampl, noLut )
    { 0, 0, 0, 3, 3, 8, 8, 1 } 
  }                
};

FormatHeader ffMpegHeader = {

  sizeof(FormatHeader),
  "2.5.3",
  "FFMPEG",
  "FFMPEG codecs",

  BIM_FORMAT_FFMPEG_MAGIC_SIZE,                  // 0 or more, specify number of bytes needed to identify the file
  {1, BIM_FFMPEG_NUM_FORMATS, ffMpegItems},   // 

  ffMpegValidateFormatProc,
  // begin
  ffMpegAquireFormatProc, //AquireFormatProc
  // end
  ffMpegReleaseFormatProc, //ReleaseFormatProc

  // params
  NULL, //AquireIntParamsProc
  NULL, //LoadFormatParamsProc
  NULL, //StoreFormatParamsProc

  // image begin
  ffMpegOpenImageProc, //OpenImageProc
  ffMpegCloseImageProc, //CloseImageProc 

  // info
  ffMpegGetNumPagesProc, //GetNumPagesProc
  ffMpegGetImageInfoProc, //GetImageInfoProc


  // read/write
  ffMpegReadImageProc, //ReadImageProc 
  ffMpegWriteImageProc, //WriteImageProc
  NULL, //ReadImageTileProc
  NULL, //WriteImageTileProc
  NULL, //ReadImageLineProc
  NULL, //WriteImageLineProc
  NULL, //ReadImageThumbProc
  NULL, //WriteImageThumbProc
  NULL, //dimJpegReadImagePreviewProc, //ReadImagePreviewProc

  // meta data
  ffMpegReadMetaDataProc, //ReadMetaDataProc
  ffMpegAddMetaDataProc,  //AddMetaDataProc
  ffMpegReadMetaDataAsTextProc, //ReadMetaDataAsTextProc
  ffmpeg_append_metadata, //AppendMetaDataProc

  NULL,
  NULL,
  ""

};

extern "C" {

  FormatHeader* ffMpegGetFormatHeader(void)
  {
    return &ffMpegHeader;
  }

} // extern C


