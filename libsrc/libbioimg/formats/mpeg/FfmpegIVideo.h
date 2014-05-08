#ifndef FFMPEGIVIDEO_H
#define FFMPEGIVIDEO_H

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
#include "IVideo.h"
#include "FfmpegCommon.h"

// Normal includes
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <math.h>
#include <limits>
#include <memory>

namespace VideoIO 
{

  // AO = "Assert Open"
#undef AO
#define AO VrRecoverableCheck(isOpen())

  /** The FfmpegIVideo class is used to read videos using the ffmpeg library
  *  hosted at http://ffmpeg.mplayerhq.hu.  This class was designed to be
  *  used by the videoIO library for Matlab, but was written in a way that
  *  it can be easily used in other contexts as a consise abstraction to 
  *  ffmpeg.
  *
  *  Copyright (C) 2006 Gerald Dalley
  *
  *  This code is released under the MIT license (see the accompanying MIT.txt
  *  file for details), unless the ffmpeg library requires a more restrictive
  *  license.  
  */
  class FfmpegIVideo : public IVideo
  {
  public:
    // Constructors/Destructors
    FfmpegIVideo();
    virtual ~FfmpegIVideo() { TRACE; close(); };

    // I/O Operations
    virtual void         open(KeyValueMap &kvm);
    virtual void         close();
    virtual bool         next();
    virtual bool         step(int numFrames=1);
    virtual bool         seek(int toFrame);
    virtual int          currFrameNum()   const {AO; return currentFrameNumber;}
    virtual Frame const &currFrame()      const {AO; return currentFrame;} 
    virtual const std::vector<unsigned char> *currBGR()        const {AO; return &bgrData; }     

    // video stats
    virtual std::string filename()             const { AO; return fname; } 
    virtual int         width()                const { AO; return pCodecCtx->width; }
    virtual int         height()               const { AO; return pCodecCtx->height; }
    virtual int         depth()                const { AO; return 3; } 
            AVRational  fpsRational()          const;
    virtual double      fps()                  const;
    virtual int         numFrames()            const;
    virtual FourCC      fourcc()               const { AO; return pCodecCtx->codec_tag; /* may not work on old versions of ffmpeg */ }
    virtual int         numHiddenFinalFrames() const { AO; return nHiddenFinalFrames; }
    
    virtual ExtraParamsAndStats extraParamsAndStats() const;
    
    AVCodecContext*     codec()                const { AO; return pCodecCtx; }
    AVFormatContext*    format()               const { AO; return pFormatCtx; }
    const char*         codecName()            const { AO; return pCodecCtx->codec->name; }
    const char*         formatName()           const { AO; return pFormatCtx->iformat->name; }
    void                setConvertToMatlab(bool v)  { convert_to_matlab = v; }

  private:
    void        open(std::string const &fname);
    inline bool isOpen() const { return (pCodecCtx != NULL); }    
    bool getNextFrame();
    bool stepLowLevel(int numFrames);

    std::string                fname;
    
    int                        currentFrameNumber;
    int                        currentTimestamp;

    AVFormatContext            *pFormatCtx;
    int                        videoStream;
    AVCodecContext             *pCodecCtx;
    
    /** The codec stuffs the data here */
    AVFrame                    *pFrame; 
    /** We then tell it to decode to a 24-bit BGR wrapper */
    AVFrame                    *pFrameBGR;
    /** Where the actual BGR data is stored here */
    std::vector<unsigned char> bgrData;
    /** Which we then manually rearrange to Matlab's format */
    Frame                      currentFrame;       
  
    AVPacket                   packet;  
    std::vector<unsigned char> dataBuffer;
    size_t                     buffPosition;

    bool                       convert_to_matlab;

#ifdef VIDEO_READER_USE_SWSCALER
    struct SwsContext         *imgConvertCtx;
#endif 

    int                        nHiddenFinalFrames; 

    bool                       dropBadPackets;
  };

#undef AO

}; /* namespace VideoIO */

#endif 
