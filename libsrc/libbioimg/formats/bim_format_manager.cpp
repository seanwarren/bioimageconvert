/*******************************************************************************

  Manager for Image Formats

  Uses DimFiSDK version: 1.2

  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
    08/04/2004 18:22 - custom stream managment compliant

  ver: 2

*******************************************************************************/

#include <iostream>
#include <cstring>
#include <cerrno>

#include "bim_format_manager.h"
#include "xstring.h"


// Disables Visual Studio 2005 warnings for deprecated code
#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
  #pragma warning(disable:4996)
#endif

// formats
#include "png/bim_png_format.h"
#include "jpeg/bim_jpeg_format.h"
#include "tiff/bim_tiff_format.h"
#include "biorad_pic/bim_biorad_pic_format.h"
#include "bmp/bim_bmp_format.h"
#include "nanoscope/bim_nanoscope_format.h"
#include "jp2/bim_jp2_format.h"
#include "ibw/bim_ibw_format.h"
#include "ome/bim_ome_format.h"
#include "raw/bim_raw_format.h"
#ifdef BIM_FFMPEG_FORMAT
#include "mpeg/bim_ffmpeg_format.h"
#endif
#include "ole/bim_ole_format.h"
#include "dcraw/bim_dcraw_format.h"
#ifdef BIM_GDCM_FORMAT
#include "dicom/bim_dicom_format.h"
#endif
#include "nifti/bim_nifti_format.h"
#include "jxr/bim_jxr_format.h"
#include "webp/bim_webp_format.h"
#include "mrc/bim_mrc_format.h"

using namespace bim;

FormatManager::FormatManager() {
  progress_proc = NULL;
  error_proc = NULL;
  test_abort_proc = NULL;

  session_active = false;
  sessionCurrentPage = 0;
  max_magic_size = 0;
  magic_number = NULL;
  sessionHandle = initFormatHandle();
  sessionFormatIndex = -1;
  sessionSubIndex = 0;

  // add static formats, jpeg first updated: dima 07/21/2005 17:11
  addNewFormatHeader ( jpegGetFormatHeader( ) );
  addNewFormatHeader ( tiffGetFormatHeader( ) );
  addNewFormatHeader ( bioRadPicGetFormatHeader() );
  addNewFormatHeader ( bmpGetFormatHeader() );
  addNewFormatHeader ( pngGetFormatHeader() );
  addNewFormatHeader ( nanoscopeGetFormatHeader() );
  addNewFormatHeader ( jp2GetFormatHeader() );
  addNewFormatHeader ( ibwGetFormatHeader() );
  addNewFormatHeader ( omeGetFormatHeader() );
  addNewFormatHeader ( rawGetFormatHeader() );
  addNewFormatHeader ( oleGetFormatHeader() );
  addNewFormatHeader ( dcrawGetFormatHeader() );
  #ifdef BIM_FFMPEG_FORMAT
  addNewFormatHeader ( ffMpegGetFormatHeader() );
  #endif
  #ifdef BIM_GDCM_FORMAT
  addNewFormatHeader ( dicomGetFormatHeader() );
  #endif
  #ifdef BIM_NIFTI_FORMAT
  addNewFormatHeader(niftiGetFormatHeader());
  #endif
  #ifdef BIM_JXRLIB_FORMAT
  addNewFormatHeader(jxrGetFormatHeader());
  #endif
  #ifdef BIM_LIBWEBP_FORMAT
  addNewFormatHeader(webpGetFormatHeader());
  #endif
  addNewFormatHeader(mrcGetFormatHeader());
}


FormatManager::~FormatManager()
{
  sessionEnd();
  formatList.clear();
}

inline FormatManager &FormatManager::operator=( FormatManager fm )
{
  unsigned int i;

  if (fm.countInstalledFormats() > this->countInstalledFormats()) {
    for (i=this->countInstalledFormats(); i<fm.countInstalledFormats(); i++) {
      this->addNewFormatHeader ( fm.getFormatHeader(i) );
    }
  }

  return *this;
}

FormatHeader *FormatManager::getFormatHeader(int i)
{
  if (i >= (int) formatList.size()) return NULL;
  return formatList.at( i );
}

unsigned int FormatManager::countInstalledFormats() {
  return formatList.size();
}

void FormatManager::addNewFormatHeader (FormatHeader *nfh)
{
  formatList.push_back( nfh );
  setMaxMagicSize();
}

void FormatManager::setMaxMagicSize()
{
  unsigned int i;

  for (i=0; i<formatList.size(); i++)
  {
    if (formatList.at(i)->neededMagicSize > max_magic_size)
      max_magic_size = formatList.at(i)->neededMagicSize;
  }

  if (magic_number != NULL) delete magic_number;
  magic_number = new unsigned char [ max_magic_size ];
}

bool FormatManager::loadMagic(const bim::Filename fileName) {
    memset( magic_number, 0, max_magic_size );

#ifdef BIM_WIN
    xstring fn(fileName);
    FILE *in_stream = _wfopen(fn.toUTF16().c_str(), L"rb");
#else
    FILE *in_stream = fopen(fileName, "rb");
#endif

    if (in_stream == NULL) {
#ifdef DEBUG
        std::cerr << "Error opening '" << fileName << "', error '" << std::strerror(errno) << "'." << std::endl;
#endif
        return false;
    }
    fread(magic_number, sizeof(unsigned char), max_magic_size, in_stream);
    fclose(in_stream);
    return true;
}

bool FormatManager::loadMagic( BIM_STREAM_CLASS *stream, SeekProc seekProc, ReadProc readProc )
{
  if ( (stream == NULL) || ( seekProc == NULL ) || ( readProc == NULL ) ) return false;
  if ( seekProc ( stream, 0, SEEK_SET ) != 0 ) return false;
  readProc ( magic_number, sizeof(unsigned char), max_magic_size, stream );
  return true;
}

void FormatManager::getNeededFormatByMagic(const bim::Filename fileName, int &format_index, int &sub_index) {
    format_index = -1;
    sub_index = -1;
    for (unsigned int i = 0; i < formatList.size(); i++) {
        int s = formatList.at(i)->validateFormatProc(magic_number, max_magic_size, fileName);
        if (s > -1) {
            format_index = i;
            sub_index = s;
            break;
        }
    }
}

#ifdef WIN32
#define strncmp strnicmp
#define strcmp stricmp
#else
#define strncmp strncasecmp
#define strcmp strcasecmp
#endif
void FormatManager::getNeededFormatByName(const char *formatName, int &format_index, int &sub_index) {
  unsigned int i, s;
  format_index = -1;
  sub_index = -1;

  for (i=0; i<formatList.size(); i++)
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
      char *fmt_short_name = formatList.at(i)->supportedFormats.item[s].formatNameShort;
      if (strcmp(fmt_short_name, formatName) == 0) {
        format_index = i;
        sub_index = s;
        break;
      } // if strcmp
    } // for s

  return;
}
#undef strncmp
#undef strcmp

FormatItem *FormatManager::getFormatItem (const char *formatName)
{
  int format_index, sub_index;
  FormatHeader *selectedFmt;
  FormatItem   *subItem;

  getNeededFormatByName(formatName, format_index, sub_index);
  if (format_index < 0) return NULL;
  if (sub_index < 0) return NULL;

  selectedFmt = formatList.at( format_index );
  if (sub_index >= (int)formatList.at(format_index)->supportedFormats.count) return NULL;
  subItem = &formatList.at(format_index)->supportedFormats.item[sub_index];
  return subItem;
}

bool FormatManager::isFormatSupported (const char *formatName)
{
  FormatItem *fi = getFormatItem (formatName);
  if (fi == NULL) return false;
  return true;
}

bool FormatManager::isFormatSupportsWMP (const char *formatName)
{
  FormatItem *fi = getFormatItem (formatName);
  if (fi == NULL) return false;
  if (fi->canWriteMultiPage == 1) return true;
  return false;
}

bool FormatManager::isFormatSupportsR (const char *formatName)
{
  FormatItem *fi = getFormatItem (formatName);
  if (fi == NULL) return false;
  if (fi->canRead == 1) return true;
  return false;
}

bool FormatManager::isFormatSupportsW (const char *formatName)
{
  FormatItem *fi = getFormatItem (formatName);
  if (fi == NULL) return false;
  if (fi->canWrite == 1) return true;
  return false;
}

bool FormatManager::isFormatSupportsBpcW (const char *formatName, int bpc) {
  FormatItem *fi = getFormatItem (formatName);
  if (fi == NULL) return false;
  if (fi->canWrite == 1) {
    if (fi->constrains.maxBitsPerSample == 0) return true;
    if ((int)fi->constrains.maxBitsPerSample >= bpc) return true;
  }
  return false;
}

void FormatManager::getNeededFormatByFileExt(const bim::Filename fileName, int &format_index, int &sub_index)
{
  format_index = 0;
  sub_index = 0;
  fileName;
}

void FormatManager::printAllFormats() {
  unsigned int i, s;

  for (i=0; i<formatList.size(); i++) {
    std::cout << xstring::xprintf("Format %d: ""%s"" ver: %s\n", i, formatList.at(i)->name, formatList.at(i)->version );
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
        std::cout << xstring::xprintf("  %d: %s [", s, formatList.at(i)->supportedFormats.item[s].formatNameShort);

        if (formatList.at(i)->supportedFormats.item[s].canRead)  std::cout << xstring::xprintf("R ");
        if (formatList.at(i)->supportedFormats.item[s].canWrite) std::cout << xstring::xprintf("W ");
        if (formatList.at(i)->supportedFormats.item[s].canReadMeta) std::cout << xstring::xprintf("RM ");
        if (formatList.at(i)->supportedFormats.item[s].canWriteMeta) std::cout << xstring::xprintf("WM ");
        if (formatList.at(i)->supportedFormats.item[s].canWriteMultiPage) std::cout << xstring::xprintf("WMP ");

        std::cout << xstring::xprintf("] <%s>\n", formatList.at(i)->supportedFormats.item[s].extensions);
    }
    std::cout << xstring::xprintf("\n");
  }
}

void FormatManager::printAllFormatsXML() {
  unsigned int i, s;

  for (i=0; i<formatList.size(); i++) {
      std::cout << xstring::xprintf("<format index=\"%d\" name=\"%s\" version=\"%s\" >\n", i, formatList.at(i)->name, formatList.at(i)->version);

    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
        std::cout << xstring::xprintf("  <codec index=\"%d\" name=\"%s\" >\n", s, formatList.at(i)->supportedFormats.item[s].formatNameShort);

      if ( formatList.at(i)->supportedFormats.item[s].canRead )
          std::cout << xstring::xprintf("    <tag name=\"support\" value=\"reading\" />\n");

      if ( formatList.at(i)->supportedFormats.item[s].canWrite )
          std::cout << xstring::xprintf("    <tag name=\"support\" value=\"writing\" />\n");

      if ( formatList.at(i)->supportedFormats.item[s].canReadMeta )
          std::cout << xstring::xprintf("    <tag name=\"support\" value=\"reading metadata\" />\n");

      if ( formatList.at(i)->supportedFormats.item[s].canWriteMeta )
          std::cout << xstring::xprintf("    <tag name=\"support\" value=\"writing metadata\" />\n");

      if ( formatList.at(i)->supportedFormats.item[s].canWriteMultiPage )
          std::cout << xstring::xprintf("    <tag name=\"support\" value=\"writing multiple pages\" />\n");

      std::cout << xstring::xprintf("    <tag name=\"extensions\" value=\"%s\" />\n", formatList.at(i)->supportedFormats.item[s].extensions);
      std::cout << xstring::xprintf("    <tag name=\"fullname\" value=\"%s\" />\n", formatList.at(i)->supportedFormats.item[s].formatNameLong);
      std::cout << xstring::xprintf("    <tag name=\"min-samples-per-pixel\" value=\"%d\" />\n", formatList.at(i)->supportedFormats.item[s].constrains.minSamplesPerPixel);
      std::cout << xstring::xprintf("    <tag name=\"max-samples-per-pixel\" value=\"%d\" />\n", formatList.at(i)->supportedFormats.item[s].constrains.maxSamplesPerPixel);
      std::cout << xstring::xprintf("    <tag name=\"min-bits-per-sample\" value=\"%d\" />\n", formatList.at(i)->supportedFormats.item[s].constrains.minBitsPerSample);
      std::cout << xstring::xprintf("    <tag name=\"max-bits-per-sample\" value=\"%d\" />\n", formatList.at(i)->supportedFormats.item[s].constrains.maxBitsPerSample);


      std::cout << xstring::xprintf("  </codec>\n");
    }
    std::cout << xstring::xprintf("</format>\n");
  }
}

std::string FormatManager::getAllFormatsHTML() {
  unsigned int i, s;
  std::string fmts;
  xstring fmt;

  fmts += "<table>\n";
  fmts += "  <tr><th>Codec</th><th>Name</th><th>Features</th><th>Extensions</th></tr>\n";
  for (i=0; i<formatList.size(); i++) {

    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
      std::string codec = formatList.at(i)->supportedFormats.item[s].formatNameShort;
      std::string name  = formatList.at(i)->supportedFormats.item[s].formatNameLong;
      xstring ext       = formatList.at(i)->supportedFormats.item[s].extensions;
      xstring features;

      if ( formatList.at(i)->supportedFormats.item[s].canRead ) features += " R";
      if ( formatList.at(i)->supportedFormats.item[s].canWrite ) features += " W";
      if ( formatList.at(i)->supportedFormats.item[s].canReadMeta ) features += " RM";
      if ( formatList.at(i)->supportedFormats.item[s].canWriteMeta ) features += " WM";
      if ( formatList.at(i)->supportedFormats.item[s].canWriteMultiPage ) features += " WMP";
      fmt.sprintf("  <tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", codec.c_str(), name.c_str(), features.c_str(), ext.c_str() );
      fmts += fmt;
    }
  }
  fmts += "</table>\n";
  return fmts;
}

void FormatManager::printAllFormatsHTML() {
  std::string str = getAllFormatsHTML();
  std::cout << str;
}

std::string FormatManager::getAllExtensions() {

  unsigned int i, s;
  std::string str;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
      str += formatList.at(i)->supportedFormats.item[s].extensions;
      str += "|";
    }
  }
  return str;
}

std::string filterizeExtensions( const std::string &_exts ) {
  std::string exts = _exts;
  std::string::size_type pos;
  pos = exts.find ( "|" );
  while (pos != std::string::npos) {
    exts.replace( pos, 1, " *.");
    pos = exts.find ( "|" );
  }
  return exts;
}

std::string FormatManager::getQtFilters() {

  unsigned int i, s;
  std::string str;
  std::string t;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++) {
      str += formatList.at(i)->supportedFormats.item[s].formatNameLong;
      str += " (*.";
      std::string exts = formatList.at(i)->supportedFormats.item[s].extensions;
      str += filterizeExtensions( exts );

      str += ");;";
    }
  }
  return str;
}

std::string FormatManager::getQtReadFilters() {
  unsigned int i, s;
  std::string str;
  std::string t;

  str += "All images (*.";
  std::string exts = getAllExtensions();
  str += filterizeExtensions( exts );
  str += ");;";

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canRead) {
        str += formatList.at(i)->supportedFormats.item[s].formatNameLong;
        str += " (*.";
        std::string exts = formatList.at(i)->supportedFormats.item[s].extensions;
        str += filterizeExtensions( exts );
        str += ");;";
      }
  }
  str += "All files (*.*)";
  return str;
}

std::string FormatManager::getQtWriteFilters() {
  unsigned int i, s;
  std::string str;
  std::string t;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canWrite) {
        str += formatList.at(i)->supportedFormats.item[s].formatNameLong;
        str += " (*.";
        std::string exts = formatList.at(i)->supportedFormats.item[s].extensions;
        str += filterizeExtensions( exts );
        str += ");;";
      }
  }
  return str;
}

std::string FormatManager::getQtWriteMPFilters() {
  unsigned int i, s;
  std::string str;
  std::string t;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canWriteMultiPage) {
        str += formatList.at(i)->supportedFormats.item[s].formatNameLong;
        str += " (*.";
        std::string exts = formatList.at(i)->supportedFormats.item[s].extensions;
        str += filterizeExtensions( exts );
        str += ");;";
      }
  }
  return str;
}

std::vector<std::string> FormatManager::getReadFormats() {
  unsigned int i, s;
  std::string str;
  std::vector<std::string> fmts;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canRead) {
        str = formatList.at(i)->supportedFormats.item[s].formatNameShort;
        fmts.push_back( str );
      }
  }
  return fmts;
}

std::vector<std::string> FormatManager::getWriteFormats() {
  unsigned int i, s;
  std::string str;
  std::vector<std::string> fmts;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canWrite) {
        str = formatList.at(i)->supportedFormats.item[s].formatNameShort;
        fmts.push_back( str );
      }
  }
  return fmts;
}

std::vector<std::string> FormatManager::getWriteMPFormats() {
  unsigned int i, s;
  std::string str;
  std::vector<std::string> fmts;

  for (i=0; i<formatList.size(); i++) {
    for (s=0; s<formatList.at(i)->supportedFormats.count; s++)
      if (formatList.at(i)->supportedFormats.item[s].canWriteMultiPage) {
        str = formatList.at(i)->supportedFormats.item[s].formatNameShort;
        fmts.push_back( str );
      }
  }
  return fmts;
}

std::string FormatManager::getFormatFilter( const char *formatName ) {
  FormatItem *fi = getFormatItem (formatName);
  std::string str;
  if (fi == NULL) return str;

  //tr("Images (*.png *.xpm *.jpg)"));
  str += fi->formatNameLong;
  str += " (*.";
  std::string exts = fi->extensions;
  str += filterizeExtensions( exts );
  str += ")";
  return str;
}

std::string FormatManager::getFilterExtensions( const char *formatName ) {
  FormatItem *fi = getFormatItem (formatName);
  std::string str;
  if (fi == NULL) return str;
  str += fi->extensions;
  return str;
}

std::string FormatManager::getFilterExtensionFirst( const char *formatName ) {
  std::string str = getFilterExtensions( formatName );

  std::string::size_type pos;
  pos = str.find ( "|" );
  if (pos != std::string::npos)
    str.resize(pos);
  return str;
}

//--------------------------------------------------------------------------------------
// simple read/write operations
//--------------------------------------------------------------------------------------

int FormatManager::loadImage ( BIM_STREAM_CLASS *stream,
                  ReadProc readProc, SeekProc seekProc, SizeProc sizeProc,
                  TellProc  tellProc,  EofProc eofProc, CloseProc closeProc,
                  const bim::Filename fileName, ImageBitmap *bmp, int page  )
{
  int format_index, format_sub;
  FormatHeader *selectedFmt;

  if ( ( stream != NULL ) && (seekProc != NULL) && (readProc != NULL) ) {
    if (!loadMagic( stream, seekProc, readProc )) return 1;
  } else {
    if (!loadMagic(fileName)) return 1;
  }

  getNeededFormatByMagic(fileName, format_index, format_sub);
  if (format_index == -1) return 1;
  selectedFmt = formatList.at( format_index );

  FormatHandle fmtParams = selectedFmt->aquireFormatProc();

  format_sub = selectedFmt->validateFormatProc(magic_number, max_magic_size, fileName);
  if (format_sub == -1) return 1;
  sessionHandle.subFormat = format_sub; // initial guess

  fmtParams.showProgressProc = progress_proc;
  fmtParams.showErrorProc    = error_proc;
  fmtParams.testAbortProc    = test_abort_proc;

  fmtParams.stream    = stream;
  fmtParams.readProc  = readProc;
  fmtParams.writeProc = NULL;
  fmtParams.flushProc = NULL;
  fmtParams.seekProc  = seekProc;
  fmtParams.sizeProc  = sizeProc;
  fmtParams.tellProc  = tellProc;
  fmtParams.eofProc   = eofProc;
  fmtParams.closeProc = closeProc;

  fmtParams.fileName = fileName;
  fmtParams.image = bmp;
  fmtParams.magic = magic_number;
  fmtParams.io_mode = IO_READ;
  fmtParams.pageNumber = page;

  if ( selectedFmt->openImageProc ( &fmtParams, IO_READ ) != 0) return 1;

  //bmpInfo = selectedFmt->getImageInfoProc ( &fmtParams, 0 );
  //selectedFmt->readMetaDataProc ( &fmtParams, 0, -1, -1, -1);
  selectedFmt->readImageProc ( &fmtParams, page );

  selectedFmt->closeImageProc ( &fmtParams );

  // RELEASE FORMAT
  selectedFmt->releaseFormatProc ( &fmtParams );
  return 0;
}

int FormatManager::loadImage(const bim::Filename fileName, ImageBitmap *bmp, int page)
{
  return loadImage ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, page );
}

int FormatManager::loadImage(const bim::Filename fileName, ImageBitmap *bmp)
{
  return loadImage ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, 0 );
}

/*
void FormatManager::loadBuffer (void *p, int buf_size, ImageBitmap *bmp) {
  MemIOBuf memBuf;
  MemIO_InitBuf( &memBuf, buf_size, (unsigned char *) p );
  loadImage ( &memBuf, MemIO_ReadProc, MemIO_SeekProc, MemIO_SizeProc, MemIO_TellProc, MemIO_EofProc, MemIO_CloseProc, "buffer.dat", bmp, 0 );
  MemIO_DeinitBuf( &memBuf );
}
*/

int FormatManager::writeImage ( BIM_STREAM_CLASS *stream, ReadProc readProc,
                  WriteProc writeProc, FlushProc flushProc, SeekProc seekProc,
                  SizeProc sizeProc, TellProc  tellProc,  EofProc eofProc, CloseProc closeProc,
                  const bim::Filename fileName, ImageBitmap *bmp, const char *formatName,
                  int quality, TagMap *meta, const char *options )
{
  int format_index, sub_index;
  FormatHeader *selectedFmt;

  getNeededFormatByName(formatName, format_index, sub_index);
  selectedFmt = formatList.at( format_index );

  FormatHandle fmtParams = selectedFmt->aquireFormatProc();

  fmtParams.showProgressProc = progress_proc;
  fmtParams.showErrorProc    = error_proc;
  fmtParams.testAbortProc    = test_abort_proc;

  fmtParams.stream    = stream;
  fmtParams.writeProc = writeProc;
  fmtParams.readProc  = readProc;
  fmtParams.flushProc = flushProc;
  fmtParams.seekProc  = seekProc;
  fmtParams.sizeProc  = sizeProc;
  fmtParams.tellProc  = tellProc;
  fmtParams.eofProc   = eofProc;
  fmtParams.closeProc = closeProc;

  fmtParams.subFormat = sub_index;
  fmtParams.fileName = fileName;
  fmtParams.image = bmp;
  fmtParams.quality = (unsigned char) quality;
  fmtParams.io_mode = IO_WRITE;
  if (meta) fmtParams.metaData = meta;

  if (options != NULL && options[0] != 0) fmtParams.options = (char *) options;

  if ( selectedFmt->openImageProc ( &fmtParams, IO_WRITE ) != 0) return 1;

  selectedFmt->writeImageProc ( &fmtParams );

  selectedFmt->closeImageProc ( &fmtParams );

  // RELEASE FORMAT
  selectedFmt->releaseFormatProc ( &fmtParams );
  return 0;
}

int FormatManager::writeImage (const bim::Filename fileName, ImageBitmap *bmp, const char *formatName,
    int quality, TagMap *meta)
{
  return writeImage ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, quality, meta );
}

int FormatManager::writeImage (const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, int quality)
{
  return writeImage ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, quality, NULL );
}

int FormatManager::writeImage(const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, const char *options)
{
  return writeImage ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, 100, NULL, options );
}

/*
unsigned char *FormatManager::writeBuffer ( ImageBitmap *bmp, const char *formatName, int quality,
                                               TagList *meta, int &buf_size ) {
  DMemIO memBuf;
  unsigned char *p;
  dMemIO_Init( &memBuf, 0, NULL );
  writeImage ( &memBuf, dMemIO_Read, dMemIO_Write, dMemIO_Flush, dMemIO_Seek,
               dMemIO_Size, dMemIO_Tell, dMemIO_Eof, dMemIO_Close,
               "buffer.dat", bmp, formatName, quality, meta );

  p = new unsigned char [ memBuf.size ];
  buf_size = memBuf.size;
  memcpy( p, memBuf.data, memBuf.size );
  dMemIO_Destroy( &memBuf );
  return p;
}

unsigned char *FormatManager::writeBuffer ( ImageBitmap *bmp, const char *formatName, int quality, int &buf_size )
{
  return writeBuffer ( bmp, formatName, quality, NULL, buf_size );
}

unsigned char *FormatManager::writeBuffer ( ImageBitmap *bmp, const char *formatName, int &buf_size )
{
  return writeBuffer ( bmp, formatName, 100, NULL, buf_size );
}
*/

void FormatManager::readImagePreview (const bim::Filename fileName, ImageBitmap *bmp,
                         bim::uint roiX, bim::uint roiY, bim::uint roiW, bim::uint roiH, bim::uint w, bim::uint h)
{
/*

  int format_index;
  FormatHeader *selectedFmt;

  if (!loadMagic(fileName)) return;

  format_index = getNeededFormatByMagic(fileName);
  selectedFmt = formatList.at( format_index );

  FormatHandle fmtParams = selectedFmt->aquireFormatProc();

  if ( selectedFmt->validateFormatProc(magic_number, max_magic_size, fileName) == -1 ) return;

  fmtParams.roiX = roiX;
  fmtParams.roiY = roiY;
  fmtParams.roiW = roiW;
  fmtParams.roiH = roiH;

  fmtParams.fileName = fileName;
  fmtParams.image = bmp;
  fmtParams.magic = magic_number;
  fmtParams.io_mode = IO_READ;
  if ( selectedFmt->openImageProc ( &fmtParams, IO_READ ) != 0) return;


  //----------------------------------------------------
  // now if function is not implemented, run standard
  // implementation
  //----------------------------------------------------
  if ( selectedFmt->readImagePreviewProc != NULL )
    selectedFmt->readImagePreviewProc ( &fmtParams, w, h );
  else
  {
    // read whole image and resize if needed
    selectedFmt->readImageProc ( &fmtParams, 0 );

    // first extract ROI from full image
    //if ( roiX < 0) roiX = 0; if ( roiY < 0) roiY = 0;
    if ( roiX >= bmp->i.width) roiX = bmp->i.width-1;
    if ( roiY >= bmp->i.height) roiY = bmp->i.height-1;
    if ( roiW >= bmp->i.width-roiX) roiW = bmp->i.width-roiX-1;
    if ( roiY >= bmp->i.height-roiY) roiH = bmp->i.height-roiY-1;

    if ( (roiW>0) && (roiH>0) )
      retreiveImgROI( bmp, roiX, roiY, roiW, roiH );

    // now resize to desired size
    if ( (w>0) && (h>0) )
      resizeImgNearNeighbor( bmp, w, h);


  }
  //----------------------------------------------------

  selectedFmt->closeImageProc ( &fmtParams );

  // RELEASE FORMAT
  selectedFmt->releaseFormatProc ( &fmtParams );

*/
}

void FormatManager::readImageThumb (const bim::Filename fileName, ImageBitmap *bmp, bim::uint w, bim::uint h) {
/*
  int format_index;
  FormatHeader *selectedFmt;

  if (!loadMagic(fileName)) return;

  format_index = getNeededFormatByMagic(fileName);
  selectedFmt = formatList.at( format_index );

  FormatHandle fmtParams = selectedFmt->aquireFormatProc();

  if ( selectedFmt->validateFormatProc(magic_number, max_magic_size, fileName) == -1 ) return;

  fmtParams.fileName = fileName;
  fmtParams.image = bmp;
  fmtParams.magic = magic_number;
  fmtParams.io_mode = IO_READ;
  if ( selectedFmt->openImageProc ( &fmtParams, IO_READ ) != 0) return;


  //----------------------------------------------------
  // now if function is not implemented, run standard
  // implementation
  //----------------------------------------------------
  if ( selectedFmt->readImagePreviewProc != NULL )
    selectedFmt->readImagePreviewProc ( &fmtParams, w, h );
  else
  {
    // read whole image and resize if needed
    selectedFmt->readImageProc ( &fmtParams, 0 );

    // now resize to desired size
    if ( (w>0) && (h>0) )
      resizeImgNearNeighbor( bmp, w, h);

  }
  //----------------------------------------------------

  selectedFmt->closeImageProc ( &fmtParams );

  // RELEASE FORMAT
  selectedFmt->releaseFormatProc ( &fmtParams );
*/
}


//--------------------------------------------------------------------------------------
// begin: session-wide operations
//--------------------------------------------------------------------------------------

bool FormatManager::sessionIsReading(const std::string &fileName) const {
  if (!session_active) return false;
  if (fileName.size()>0)
    if (fileName != sessionFileName) return false;
  if (sessionHandle.io_mode == IO_READ) return true;
  return false;
}

bool FormatManager::sessionIsWriting(const std::string &fileName) const {
  if (!session_active) return false;
  if (fileName.size()>0)
    if (fileName != sessionFileName) return false;
  if (sessionHandle.io_mode == IO_WRITE) return true;
  return false;
}

int FormatManager::sessionStartRead ( BIM_STREAM_CLASS *stream, ReadProc readProc, SeekProc seekProc,
                         SizeProc sizeProc, TellProc tellProc, EofProc eofProc,
                         CloseProc closeProc, const bim::Filename fileName, const char *formatName)
{
  if (session_active) sessionEnd();
  sessionCurrentPage = 0;

  if (formatName == NULL) {
    if ( ( stream != NULL ) && (seekProc != NULL) && (readProc != NULL) ) {
      if (!loadMagic( stream, seekProc, readProc )) {
#ifdef DEBUG
          std::cerr << "FormatManager::sessionStartRead(): Failed to access file magic from stream. Aborted." << std::endl;
#endif
          return 1;
      }
    }
    else {
      if (!loadMagic(fileName)) {
#ifdef DEBUG
          std::cerr << "FormatManager::sessionStartRead(): Failed to access file magic from file. Aborted." << std::endl;
#endif
          return 1;
      }
    }
    getNeededFormatByMagic(fileName, sessionFormatIndex, sessionSubIndex);
  } else {
    // force specific format
    getNeededFormatByName(formatName, sessionFormatIndex, sessionSubIndex);
  }

  if (sessionFormatIndex < 0) {
#ifdef DEBUG
      std::cerr << "FormatManager::sessionStartRead(): sessionFormatIndex is not set. Aborted." << std::endl;
#endif
      return 1;
  }
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );

  sessionHandle = selectedFmt->aquireFormatProc();
  sessionHandle.subFormat = sessionSubIndex; // initial guess

  //if ( selectedFmt->validateFormatProc(magic_number, max_magic_size)==-1 ) return 1;

  sessionHandle.showProgressProc = progress_proc;
  sessionHandle.showErrorProc    = error_proc;
  sessionHandle.testAbortProc    = test_abort_proc;

  sessionHandle.stream    = stream;
  sessionHandle.readProc  = readProc;
  sessionHandle.writeProc = NULL;
  sessionHandle.flushProc = NULL;
  sessionHandle.seekProc  = seekProc;
  sessionHandle.sizeProc  = sizeProc;
  sessionHandle.tellProc  = tellProc;
  sessionHandle.eofProc   = eofProc;
  sessionHandle.closeProc = closeProc;

  sessionFileName = fileName;
  sessionHandle.fileName = &sessionFileName[0];
  sessionHandle.magic    = magic_number;
  sessionHandle.io_mode  = IO_READ;
  const int res = selectedFmt->openImageProc ( &sessionHandle, IO_READ );
  sessionSubIndex = sessionHandle.subFormat;
  if (res == 0) {
      session_active = true;
      this->info = selectedFmt->getImageInfoProc ( &sessionHandle, 0 );
  } else {
#ifdef DEBUG
      std::cerr << "FormatManager::sessionStartRead(): Failed to open file with openImageProc(). Aborted." << std::endl;
#endif
  }
  return res;
}

int FormatManager::sessionStartRead  (const bim::Filename fileName, const char *formatName) {
  return sessionStartRead ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, formatName );
}

int FormatManager::sessionStartReadRAW(const bim::Filename fileName, unsigned int header_offset, bool big_endian, bool interleaved) {
  int res = sessionStartRead(fileName, "RAW");
  if (res == 0) {
    RawParams *rp = (RawParams *) sessionHandle.internalParams;
    rp->header_offset = header_offset;
    rp->big_endian = big_endian;
    rp->interleaved = interleaved;
  }
  return res;
}

int FormatManager::sessionStartWrite (const bim::Filename fileName, const char *formatName, const char *options ) {
  return sessionStartWrite ( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, formatName, options );
}

int FormatManager::sessionStartWrite ( BIM_STREAM_CLASS *stream,
                  WriteProc writeProc, FlushProc flushProc, SeekProc seekProc,
                  SizeProc sizeProc, TellProc  tellProc,  EofProc eofProc, CloseProc closeProc,
                  const bim::Filename fileName, const char *formatName, const char *options ) {
  int res;
  if (session_active == true) sessionEnd();
  sessionCurrentPage = 0;

  getNeededFormatByName(formatName, sessionFormatIndex, sessionSubIndex);
  if (sessionFormatIndex < 0) {
    std::cerr << "Error: selected format '" << formatName << "' not found. Formats are not case sensitive, but must be given with the precise string (i.e. 'tif' is not the same as 'tiff'). Aborted." << std::endl;
    return -1;
  }

  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );

  sessionHandle = selectedFmt->aquireFormatProc();

  sessionHandle.showProgressProc = progress_proc;
  sessionHandle.showErrorProc    = error_proc;
  sessionHandle.testAbortProc    = test_abort_proc;

  sessionHandle.stream    = stream;
  sessionHandle.writeProc = writeProc;
  sessionHandle.readProc  = NULL;
  sessionHandle.flushProc = flushProc;
  sessionHandle.seekProc  = seekProc;
  sessionHandle.sizeProc  = sizeProc;
  sessionHandle.tellProc  = tellProc;
  sessionHandle.eofProc   = eofProc;
  sessionHandle.closeProc = closeProc;

  sessionFileName = fileName;
  sessionHandle.fileName = &sessionFileName[0];
  sessionHandle.io_mode  = IO_WRITE;
  sessionHandle.subFormat = sessionSubIndex;

  if (options != NULL && options[0] != 0) sessionHandle.options = (char *) options;

  res = selectedFmt->openImageProc ( &sessionHandle, IO_WRITE );

  if (res == 0) session_active = true;
  return res;
}

void FormatManager::sessionEnd() {
  if ( (sessionFormatIndex>=0) && (sessionFormatIndex<(int)formatList.size()) ) {
    FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
    selectedFmt->closeImageProc    ( &sessionHandle );
    selectedFmt->releaseFormatProc ( &sessionHandle );
  }
  sessionFormatIndex = -1;
  session_active = false;
  sessionCurrentPage = 0;
}

void  FormatManager::sessionSetQuality  (int quality) {
  sessionHandle.quality = (unsigned char) quality;
}

int FormatManager::sessionGetFormat ()
{
  if (session_active != true) return -1;
  return sessionFormatIndex;
}

int FormatManager::sessionGetSubFormat ()
{
  if (session_active != true) return -1;
  return sessionSubIndex;
}

char* FormatManager::sessionGetCodecName () {
  if (session_active != true) return NULL;
  return formatList.at(sessionFormatIndex)->name;
}

bool FormatManager::sessionIsCurrentCodec ( const char *name ) {
  char* fmt = sessionGetCodecName();
  return ( strcmp( fmt, name ) == 0 );
}

char* FormatManager::sessionGetFormatName () {
  if (session_active != true) return NULL;
  return formatList.at(sessionFormatIndex)->supportedFormats.item[sessionSubIndex].formatNameShort;
}

bool FormatManager::sessionIsCurrentFormat ( const char *name ) {
  char* fmt = sessionGetFormatName();
  return ( strcmp( fmt, name ) == 0 );
}

int FormatManager::sessionGetNumberOfPages ()
{
  if (session_active != true) return 0;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  return selectedFmt->getNumPagesProc ( &sessionHandle );
}

int FormatManager::sessionGetCurrentPage ()
{
  if (session_active != true) return 0;
  return sessionCurrentPage;
}

/*
TagList* FormatManager::sessionReadMetaData ( bim::uint page, int group, int tag, int type) {
  if (session_active != true) return NULL;
  sessionCurrentPage = page;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  if (selectedFmt->readMetaDataProc)
      selectedFmt->readMetaDataProc ( &sessionHandle, page, group, tag, type);
  return &sessionHandle.metaData;
}
*/

int FormatManager::sessionReadImage ( ImageBitmap *bmp, bim::uint page ) {
  if (session_active != true) return 1;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  sessionCurrentPage = page;
  sessionHandle.image = bmp;
  sessionHandle.pageNumber = page;
  int r = selectedFmt->readImageProc ( &sessionHandle, page );
  sessionHandle.image = NULL;
  return r;
}

int FormatManager::sessionWriteImage ( ImageBitmap *bmp, bim::uint page) {
  if (session_active != true) return 1;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  sessionCurrentPage = page;
  sessionHandle.image = bmp;
  sessionHandle.pageNumber = page;
  return selectedFmt->writeImageProc ( &sessionHandle );
}



int FormatManager::sessionReadLevel(ImageBitmap *bmp, bim::uint page, uint level) {
    if (session_active != true) return 1;
    FormatHeader *selectedFmt = formatList.at(sessionFormatIndex);
    if (!selectedFmt->readImageLevelProc) return 1;
    sessionHandle.image = bmp;
    int r = selectedFmt->readImageLevelProc(&sessionHandle, page, level);
    sessionHandle.image = NULL;
    return r;
}

int FormatManager::sessionReadTile(ImageBitmap *bmp, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level) {
    if (session_active != true) return 1;
    FormatHeader *selectedFmt = formatList.at(sessionFormatIndex);
    if (!selectedFmt->readImageTileProc) return 1;
    sessionHandle.image = bmp;
    int r = selectedFmt->readImageTileProc(&sessionHandle, page, xid, yid, level);
    sessionHandle.image = NULL;
    return r;
}





void  FormatManager::sessionReadImagePreview ( ImageBitmap *bmp,
                                bim::uint roiX, bim::uint roiY, bim::uint roiW, bim::uint roiH,
                                bim::uint w, bim::uint h) {
/*
  if (session_active != true) return;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  sessionCurrentPage = 0;
  sessionHandle.image = bmp;
  sessionHandle.pageNumber = 0;
  sessionHandle.roiX = roiX;
  sessionHandle.roiY = roiY;
  sessionHandle.roiW = roiW;
  sessionHandle.roiH = roiH;

  //----------------------------------------------------
  // now if function is not implemented, run standard
  // implementation
  //----------------------------------------------------
  if ( selectedFmt->readImagePreviewProc != NULL )
    selectedFmt->readImagePreviewProc ( &sessionHandle, w, h );
  else
  {
    // read whole image and resize if needed
    selectedFmt->readImageProc ( &sessionHandle, 0 );

    // first extract ROI from full image
    //if ( roiX < 0) roiX = 0; if ( roiY < 0) roiY = 0;
    if ( roiX >= bmp->i.width) roiX = bmp->i.width-1;
    if ( roiY >= bmp->i.height) roiY = bmp->i.height-1;
    if ( roiW >= bmp->i.width-roiX) roiW = bmp->i.width-roiX-1;
    if ( roiY >= bmp->i.height-roiY) roiH = bmp->i.height-roiY-1;

    if ( (roiW>0) && (roiH>0) )
      retreiveImgROI( bmp, roiX, roiY, roiW, roiH );

    // now resize to desired size
    if ( (w>0) && (h>0) )
      resizeImgNearNeighbor( bmp, w, h);
  }
*/
}

void  FormatManager::sessionReadImageThumb   ( ImageBitmap *bmp, bim::uint w, bim::uint h) {
/*
  if (session_active != true) return;
  FormatHeader *selectedFmt = formatList.at( sessionFormatIndex );
  sessionCurrentPage = 0;
  sessionHandle.image = bmp;
  sessionHandle.pageNumber = 0;

  //----------------------------------------------------
  // now if function is not implemented, run standard
  // implementation
  //----------------------------------------------------
  if ( selectedFmt->readImagePreviewProc != NULL )
    selectedFmt->readImagePreviewProc ( &sessionHandle, w, h );
  else
  {
    // read whole image and resize if needed
    selectedFmt->readImageProc ( &sessionHandle, 0 );

    // now resize to desired size
    if ( (w>0) && (h>0) )
      resizeImgNearNeighbor( bmp, w, h);

  }
*/
}

//--------------------------------------------------------------------------------------
// end: session-wide operations
//--------------------------------------------------------------------------------------















