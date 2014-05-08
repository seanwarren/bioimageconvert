#ifndef FFMPEGCOMMON_H
#define FFMPEGCOMMON_H

// $Date: 2008-11-17 17:39:15 -0500 (Mon, 17 Nov 2008) $
// $Revision: 706 $

/*
videoIO: granting easy, flexible, and efficient read/write access to video 
                 files in Matlab on Windows and GNU/Linux platforms.
    
Copyright (c) 2006 Gerald Dalley
  
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

    Portions of this software link to code licensed under the Gnu General 
    Public License (GPL).  As such, they must be licensed by the more 
    restrictive GPL license rather than this MIT license.  If you compile 
    those files, this library and any code of yours that uses it automatically
    becomes subject to the GPL conditions.  Any source files supplied by 
    this library that bear this restriction are clearly marked with internal
    comments.

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/

#include "debug.h"

// FFMPEG Includes
extern "C" {
#ifdef FFMPEG_2007_VERSION
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#else
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#endif

#if (LIBAVCODEC_VERSION_MAJOR >= 52) || (LIBAVCODEC_VERSION_INT>((51<<16)+(11<<8)+0))
#  define VIDEO_READER_USE_SWSCALER
#ifdef FFMPEG_2007_VERSION
#include <ffmpeg/swscale.h>
#else
#include <libswscale/swscale.h>
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#endif
#endif
};

#include <string>
#include <set>

// dima: microsoft compiler
#ifdef _MSC_VER
#define snprintf _snprintf
#define strcasecmp _stricmp
#endif

namespace VideoIO {

  // Get a short description of standard ffmpeg error codes
  // (note that most ffmpeg functions do not return informative
  // error codes).
  extern char const *averror(int errCode);

  // Call this before calling *any* ffmpeg functions.  It may be safely called
  // as many times as you'd like.
  extern void ffmpegInitIfNeeded();

  // Like VrRecoverableCheckMsg, but used when calling functions that return
  // ffmpeg error codes (negative means failure).  Also grabs ffmpeg logger
  // information.
#define FfRecoverableCheckMsg(testCond, msg)                             \
{ int ffmpegRc = 0;                                                      \
  ffBuffer[0] = '\0';                                                    \
  VrGenericCheckMsg((ffmpegRc = (testCond)) >= 0,                        \
                    VrRecoverableException,                              \
                    msg,                                                 \
                    "      ffmpeg error " << ffmpegRc << ": " <<         \
                         averror(ffmpegRc) << "\n" <<                    \
                    (ffBuffer[0] ? ffBuffer : "\n")) }

#define FfRecoverableCheck(testCond) \
  FfRecoverableCheckMsg(testCond, #testCond)

  // Internal buffer: do not use directly.  Note: it's not threadsafe. 
  extern char * const ffBuffer;

  // Different versions of ffmpeg have the codec context as either a 
  // part of the AVStream struct or as a pointer from AVStream.  This
  // function abstracts the version incompatibilities.
  extern AVCodecContext *getCodecFromStream(AVStream *s);

#if (!((LIBAVCODEC_VERSION_MAJOR >= 52) || (LIBAVCODEC_VERSION_INT > 0x000409) || (LIBAVCODEC_VERSION_INT == 0x000409 && LIBAVCODEC_BUILD >= 4754)))
  // av_rescale_q was introduced in svn revision 4168
  // safe a*b/c computation
  extern int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq);
#endif

  typedef struct FfmpegFormatName {
    FfmpegFormatName(std::string const &n, std::string const &l) : 
      name(n), longName(l) {};
    std::string name;
    std::string longName;
  };
  extern bool operator< (FfmpegFormatName const &a, FfmpegFormatName const &b);

  extern std::set<FfmpegFormatName>  const &getFfmpegInputFileFormats();
  extern std::set<std::string> const &getFfmpegInputFileFormatShortNames();
  extern std::set<std::string> const &getFfmpegInputCodecNames();  
  extern std::set<FfmpegFormatName>  const &getFfmpegOutputFileFormats();
  extern std::set<std::string> const &getFfmpegOutputFileFormatShortNames();
  extern std::set<std::string> const &getFfmpegOutputCodecNames();
  extern std::set<std::string> const &getFfmpegProtocols();
};
#endif
