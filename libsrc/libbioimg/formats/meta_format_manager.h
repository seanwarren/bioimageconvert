/*******************************************************************************

  Manager for Image Formats with MetaData parsing

  Uses DimFiSDK version: 1
  
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
    01/25/2007 21:00 - added QImaging TIFF
      
  ver: 2
        
*******************************************************************************/

#ifndef META_FORMAT_MANAGER_H
#define META_FORMAT_MANAGER_H

#include <vector>
#include <map>

#include "bim_format_manager.h"
#include <xstring.h>
#include <tag_map.h>


namespace bim {

// this is REALLY suboptimal but static vectors in c++ are a pain
std::vector<DisplayColor> defaultChannelColors();

class MetaFormatManager : public FormatManager {
public:
  MetaFormatManager();
  ~MetaFormatManager();
  int  sessionStartRead  (const bim::Filename fileName);
  int  sessionReadImage  ( ImageBitmap *bmp, bim::uint page );
  int  sessionWriteImage ( ImageBitmap *bmp, bim::uint page );

  void sessionParseMetaData ( bim::uint page );
  ImageBitmap *sessionImage();
  void sessionEnd();

  void sessionWriteSetMetadata( const TagMap &hash );
  void sessionWriteSetOMEXML( const std::string &omexml );

  void writeImage(BIM_STREAM_CLASS *stream, ReadProc readProc, WriteProc writeProc, FlushProc flushProc,
      SeekProc seekProc, SizeProc sizeProc, TellProc  tellProc,
      EofProc eofProc, CloseProc closeProc, const bim::Filename fileName,
      ImageBitmap *bmp, const char *formatName, int quality, TagMap *meta = NULL, const char *options = NULL) {
      return FormatManager::writeImage(stream, readProc,
          writeProc, flushProc, seekProc,
          sizeProc, tellProc, eofProc, closeProc,
          fileName, bmp, formatName, quality, meta == NULL ? &this->metadata : meta, options);
  }

  void writeImage(const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, int quality, TagMap *meta) {
      writeImage(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, quality, meta);
  }
  void writeImage(const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, int quality) {
      writeImage(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, quality, NULL);
  }
  void writeImage(const bim::Filename fileName, ImageBitmap *bmp, const char *formatName, const char *options = NULL) {
      writeImage(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, fileName, bmp, formatName, 100, NULL, options);
  }
  void writeImage(const bim::Filename fileName, Image &img, const char *formatName, const char *options = NULL) {
      writeImage(fileName, img.imageBitmap(), formatName, options);
  }

  // META
  double       getPixelSizeX();
  double       getPixelSizeY();
  double       getPixelSizeZ();
  double       getPixelSizeT();
  const char*  getImagingTime();

  bool         display_lut_needs_fusion() const;

  inline const TagMap                      &get_metadata() const      { return metadata; }
  inline const std::vector< int >          &get_display_lut() const   { return display_lut; }
  inline const std::vector< std::string >  &get_channel_names() const { return channel_names; }

  inline std::string get_metadata_tag( const std::string &key, const std::string &def ) const { return metadata.get_value( key, def ); }
  inline int         get_metadata_tag_int( const std::string &key, const int &def ) const { return metadata.get_value_int( key, def ); }
  inline double      get_metadata_tag_double( const std::string &key, const double &def ) const { return metadata.get_value_double( key, def ); }

  void               delete_metadata_tag(const std::string &key) { metadata.delete_tag(key); }
  
  void               set_metadata_tag(const std::string &key, const int &v) { metadata.set_value(key, v); }
  void               set_metadata_tag(const std::string &key, const double &v) { metadata.set_value(key, v); }
  void               set_metadata_tag(const std::string &key, const std::string &v) { metadata.set_value(key, v); }

private:
  int           got_meta_for_session;
  //TagList  *tagList;
  //std::string   meta_data_text;
  //ImageInfo info;
  //TagItem   writeTagItem;

  std::vector<std::string> display_channel_tag_names;
  std::vector<bim::DisplayColor> channel_colors_default;
  std::vector<std::string> pixel_format_strings;

  // key-value pairs for metadata
  TagMap metadata;
  inline void appendMetadata( const std::string &key, const std::string &value ) { metadata.append_tag( key, value ); }
  inline void appendMetadata( const std::string &key, const int &value ) { metadata.append_tag( key, value ); }
  inline void appendMetadata( const std::string &key, const double &value ) { metadata.append_tag( key, value ); }

  // META
  double pixel_size[4]; //XYZ in microns, T in seconds
  xstring imaging_time; // "YYYY-MM-DD HH:MM:SS" ANSI date and 24h time
  std::vector< std::string > channel_names;
  std::vector< int > display_lut;

  void fill_static_metadata_from_map();
};

} // namespace bim

#endif // META_FORMAT_MANAGER_H

