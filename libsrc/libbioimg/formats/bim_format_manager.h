/*******************************************************************************

  Manager for Image Formats

  Uses DimFiSDK version: 1.2
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  Notes:
    Session: during session any session wide operation will be performed
    with current session!!!
    If you want to start simultaneous sessions then create new manager
    using = operator. It will copy all necessary initialization data
    to the new manager.

    Non-session wide operation might be performed simultaneously with 
    an open session.

  History:
    03/23/2004 18:03 - First creation
    08/04/2004 18:22 - custom stream managment compliant
    
  ver: 2
        
*******************************************************************************/

#ifndef BIM_FORMAT_MANAGER_H
#define BIM_FORMAT_MANAGER_H

#include <string>
#include <vector>

#include <bim_img_format_interface.h>
#include <bim_image.h>

namespace bim {

class FormatManager {
public:
  FormatManager();
  ~FormatManager();
  FormatManager &operator=( FormatManager fm );

  void              addNewFormatHeader (FormatHeader *nfh);
  FormatHeader *getFormatHeader(int i);
  unsigned int      countInstalledFormats();
  //bool              canWriteMulti (const char *formatName);
  bool              isFormatSupported (const char *formatName);
  bool              isFormatSupportsR (const char *formatName);
  bool              isFormatSupportsW (const char *formatName);  
  bool              isFormatSupportsWMP (const char *formatName);

  bool              isFormatSupportsBpcW (const char *formatName, int bpc); 

  void              printAllFormats();
  void              printAllFormatsXML();
  void              printAllFormatsHTML();
  std::string       getAllFormatsHTML();

  std::string       getAllExtensions();
  std::vector<std::string> getReadFormats();
  std::vector<std::string> getWriteFormats();
  std::vector<std::string> getWriteMPFormats();
  std::string       getFormatFilter( const char *formatName );
  std::string       getFilterExtensions( const char *formatName );
  std::string       getFilterExtensionFirst( const char *formatName );

  std::string       getQtFilters();
  std::string       getQtReadFilters();
  std::string       getQtWriteFilters();
  std::string       getQtWriteMPFilters();

  // Simple Read/Write
  void loadImage  (const bim::Filename fileName, ImageBitmap *bmp);
  void loadImage  (const bim::Filename fileName, Image &img) { loadImage(fileName, img.imageBitmap()); }
  void loadImage  (const bim::Filename fileName, ImageBitmap *bmp, int page);
  void loadImage  (const bim::Filename fileName, Image &img, int page) { loadImage(fileName, img.imageBitmap(), page); }


  void loadImage  ( BIM_STREAM_CLASS *stream,
                    ReadProc readProc, SeekProc seekProc, SizeProc sizeProc, 
                    TellProc  tellProc,  EofProc eofProc, CloseProc closeProc,   
                    const bim::Filename fileName, ImageBitmap *bmp, int page );
  //void loadBuffer (void *p, int buf_size, ImageBitmap *bmp);
  
  void readImagePreview (const bim::Filename fileName, ImageBitmap *bmp, 
                         bim::uint roiX, bim::uint roiY, bim::uint roiW, bim::uint roiH, 
                         bim::uint w, bim::uint h);
  void readImageThumb   (const bim::Filename fileName, ImageBitmap *bmp, bim::uint w, bim::uint h);


  void writeImage ( BIM_STREAM_CLASS *stream, ReadProc readProc, WriteProc writeProc, FlushProc flushProc, 
                    SeekProc seekProc, SizeProc sizeProc, TellProc  tellProc,  
                    EofProc eofProc, CloseProc closeProc, const bim::Filename fileName, 
                    ImageBitmap *bmp, const char *formatName, int quality, TagMap *meta=NULL, const char *options = NULL);
  void writeImage (const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, 
      int quality, TagMap *meta);
  void writeImage (const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, int quality);
  void writeImage (const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, const char *options = NULL);
  void writeImage (const bim::Filename fileName, Image &img, const char *formatName, const char *options = NULL) {
    writeImage (fileName, img.imageBitmap(), formatName, options);
  }

  //unsigned char *writeBuffer ( ImageBitmap *bmp, const char *formatName, int quality, TagList *meta, int &buf_size );
  //unsigned char *writeBuffer ( ImageBitmap *bmp, const char *formatName, int quality, int &buf_size );
  //unsigned char *writeBuffer ( ImageBitmap *bmp, const char *formatName, int &buf_size );

  //--------------------------------------------------------------------------------------
  // Callbacks
  //--------------------------------------------------------------------------------------

  ProgressProc progress_proc;
  ErrorProc error_proc;
  TestAbortProc test_abort_proc;
  void setCallbacks( ProgressProc _progress_proc, 
                     ErrorProc _error_proc, 
                     TestAbortProc _test_abort_proc ) {
    progress_proc = _progress_proc;
    error_proc = _error_proc;
    test_abort_proc = _test_abort_proc;
  }

  //--------------------------------------------------------------------------------------
  // begin: session-wide operations
  //--------------------------------------------------------------------------------------
  bool sessionActive() const { return session_active; }

  std::string sessionFilename() const { return sessionFileName; }
  bool  sessionIsReading  (const std::string &fileName="") const;
  bool  sessionIsWriting  (const std::string &fileName="") const;

  int   sessionStartRead  ( BIM_STREAM_CLASS *stream, ReadProc readProc, SeekProc seekProc, 
                            SizeProc sizeProc, TellProc tellProc, EofProc eofProc, 
                            CloseProc closeProc, const bim::Filename fileName, const char *formatName=NULL);
  
  int   sessionStartWrite ( BIM_STREAM_CLASS *stream, WriteProc writeProc, FlushProc flushProc, 
          SeekProc seekProc, SizeProc sizeProc, TellProc  tellProc, EofProc eofProc,
          CloseProc closeProc, const bim::Filename fileName, const char *formatName, const char *options = NULL );

  // if formatName spesified then forces specific format, used for RAW
  int   sessionStartRead    (const bim::Filename fileName, const char *formatName=NULL);
  int   sessionStartReadRAW(const bim::Filename fileName, unsigned int header_offset = 0, bool big_endian = false, bool interleaved=false);
  int   sessionStartWrite   (const bim::Filename fileName, const char *formatName, const char *options = NULL);

  int   sessionGetFormat ();
  int   sessionGetSubFormat ();
  char* sessionGetCodecName ();
  char* sessionGetFormatName ();
  bool  sessionIsCurrentCodec ( const char *name );
  bool  sessionIsCurrentFormat ( const char *name );
  int   sessionGetNumberOfPages ();
  ImageInfo sessionGetInfo () const { return this->info; }
  int   sessionGetCurrentPage ();
  //TagList* sessionReadMetaData ( bim::uint page, int group, int tag, int type);
  int   sessionReadImage  ( ImageBitmap *bmp, bim::uint page );
  int   sessionReadImage  ( Image &img, bim::uint page ) { return sessionReadImage(img.imageBitmap(), page); }

  void  sessionReadImagePreview ( ImageBitmap *bmp, 
                                  bim::uint roiX, bim::uint roiY, bim::uint roiW, bim::uint roiH, 
                                  bim::uint w, bim::uint h);
  void  sessionReadImageThumb   ( ImageBitmap *bmp, bim::uint w, bim::uint h);


  int   sessionReadLevel(ImageBitmap *bmp, bim::uint page, uint level);
  int   sessionReadLevel(Image &img, bim::uint page, uint level) { return sessionReadLevel(img.imageBitmap(), page, level); }

  int   sessionReadTile(ImageBitmap *bmp, bim::uint page, uint64 xid, uint64 yid, uint level);
  int   sessionReadTile(Image &img, bim::uint page, uint64 xid, uint64 yid, uint level) { return sessionReadTile(img.imageBitmap(), page, xid, yid, level); }


  void  sessionSetQuality ( int quality );
  int   sessionWriteImage ( ImageBitmap *bmp, bim::uint page );
  int   sessionWriteImage ( Image &img, bim::uint page ) 
  { return sessionWriteImage(img.imageBitmap(), page); }

  void  sessionEnd();
  //--------------------------------------------------------------------------------------
  // end: session-wide operations
  //--------------------------------------------------------------------------------------


protected:
  unsigned char *magic_number; // magic number loaded to suit correct format search
  unsigned int max_magic_size; // maximum size needed to establish format
  ImageInfo info;

  // session vars
  bool session_active;
  FormatHandle sessionHandle;
  int sessionFormatIndex, sessionSubIndex;
  int sessionCurrentPage;
  std::string sessionFileName;

  std::vector<FormatHeader *> formatList;

  void setMaxMagicSize();
  bool loadMagic( const bim::Filename fileName );
  bool loadMagic( BIM_STREAM_CLASS *stream, SeekProc seekProc, ReadProc readProc );
  void getNeededFormatByMagic(const bim::Filename fileName, int &format_index, int &sub_index);
  void getNeededFormatByName    (const char *formatName, int &format_index, int &sub_index);
  void getNeededFormatByFileExt (const bim::Filename fileName, int &format_index, int &sub_index);
  
  FormatItem *getFormatItem (const char *formatName);
};

} // namespace bim

#endif // BIM_FORMAT_MANAGER_H
