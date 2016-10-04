#ifndef IVIDEO_H
#define IVIDEO_H

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

#include <string>
#include <vector>
#include <math.h>
#include <limits> 
#include <memory>
#include <map> 
#include "parse.h"

namespace VideoIO
{

  /**
  * This pure abstract class defines a simple interface for reading videos 
  * from files.  Individual implementations such as DirectShowIVideo and 
  * FfmpegIVideo implement these methods as appropriate.  For the most part, 
  * its methods are self-explanatory, with the following caveats:
  *    1) The first frame is 0 (not 1).
  *    2) Any method may throw a VrFatalError or VrRecoverableException on 
  *       error.
  *    3) An implementation may behave arbitrarily if any methods are called
  *       after construction and before a successful call to open.  Preferably
  *       a VrRecoverableException is thrown if the call cannot be completed.
  *    4) An implementation may behave arbitrarily if any methods other than 
  *       open are called after close is called.  Preferably a 
  *       VrRecoverableException is thrown if the call cannot be completed.
  *    5) After opening a video, it is *not* pointing to a valid frame.  A call
  *       must first be made to next, step, or seek.  An implementation may 
  *       behave arbitrarily if any method other than next, step, seek, or 
  *       close is called immediately after open succeeds.  Preferably a 
  *       VrRecoverableException is thrown if the call cannot be completed.
  *    6) If the user attempts to seek past the end or before the beginning of 
  *       the video using a call to next, step, or seek, they must return false
  *       and not throw an exception.  These methods must only throw exceptions
  *       under catastrophic conditions (generally only when a VrFatalError
  *       would be appropriate).
  *    7) The user must be allowed to call close an arbitrary number of times
  *       (i.e. close should do nothing if the video is already closed; it must
  *       not throw an exception).
  *
  * The following is a typical simple usage example, assuming the class
  * FooIVideo is a concrete subclass of IVideo:
  *    FooIVideo myVid;
  *    myVid.open(...);
  *    while (myVid.next()) {
  *      IVideo::Frame const &frame = myVid.currFrame();
  *      ...do something with the frame...
  *    }
  *    myVid.close();
  */

  class IVideo 
  {
  public:
    typedef unsigned FourCC;

    /**
    * A video frame consists of unsigned char data with bytes ordered the
    * way Matlab likes them.  For an xRGB image, index a frame as:
    *     myFrame[y + (x + c*myVid->width())*myVid->height()]
    * where myVid is a IVideo object, (x,y) are the 0-indexed pixel coordinates,
    * and c is the color channel (0==red, 1==green, 2==blue).
    */
    typedef std::vector<unsigned char> Frame;

    // Constructors/Destructors
    virtual ~IVideo() { };

    // I/O Operations
    virtual void         open(KeyValueMap &kvm)        = 0;
    virtual void         close()                       = 0;
    virtual bool         next()                        = 0;
    virtual bool         step(int numFrames=1)         = 0;
    virtual bool         seek(int toFrame)             = 0;
    virtual int          currFrameNum()          const = 0;
    virtual Frame const &currFrame()             const = 0; 

    // video stats
    virtual std::string  filename()             const = 0;
    virtual int          width()                const = 0;
    virtual int          height()               const = 0;
    virtual int          depth()                const = 0;
    virtual double       fps()                  const = 0;
    virtual int          numFrames()            const = 0;
    virtual FourCC       fourcc()               const = 0;
    virtual int          numHiddenFinalFrames() const = 0;
    
    // provide a mechanism for IVideo implementations to return video
    // statistics and/or parameter settings that are not exported by 
    // the IVideo interface.  TODO: in the future, it would be nice for
    // the value part to contain data type information so that
    // videoReaderWrapper could convert to numeric matrices, as 
    // appropriate.
    typedef std::map<std::string, std::string> ExtraParamsAndStats;
    virtual ExtraParamsAndStats extraParamsAndStats() const = 0;
  };

  /// Converts a 4-character string to a FourCC code 
  inline IVideo::FourCC stringToFourCC(std::string s) 
  { 
    return (((IVideo::FourCC)s[3]) << 24) | (((IVideo::FourCC)s[2]) << 16) | 
      (((IVideo::FourCC)s[1]) <<  8) | (((IVideo::FourCC)s[0]) <<  0);
  }

  inline std::string fourCCToString(IVideo::FourCC fcc)
  {
    char s[5];
    for (int i=0; i<5/*5->null byte at end*/; i++) {
      s[i] = (char)(fcc & 0xff);
      fcc >>= 8;
    }
    return s;
  }

  /**
  * Unfortunately, for videos encoded with some codecs it is difficult 
  * (impossible?) to read the last few frames.  This function attempts to guess
  * how many of the last few frames cannot be read given an encoder's FourCC
  * value.  These hard-coded numbers were obtained from tests performed with
  *     tests/analyzeNumFinalHiddenFrames.m
  */
  inline int guessNumHiddenFinalFrames(IVideo::FourCC fcc)
  {
    static std::map<IVideo::FourCC, int> nHidden;
    if (nHidden.size() == 0) {
      nHidden[stringToFourCC("DIVX")] = 1;
      nHidden[stringToFourCC("WMV3")] = 0;
      nHidden[0]                      = 0; // uncompressed
      nHidden[stringToFourCC("    ")] = 0; // uncompressed
      nHidden[stringToFourCC("H264")] = 3;
#if defined(WIN32) || defined(WIN64)
      nHidden[stringToFourCC("XVID")] = 3; 
#else
      nHidden[stringToFourCC("XVID")] = 1;
#endif
      nHidden[stringToFourCC("xvid")] = 0;
      nHidden[stringToFourCC("3IV2")] = 1;
      nHidden[stringToFourCC("DX50")] = 1;
      nHidden[stringToFourCC("YUY2")] = 0; // uncompressed
    }
    if (nHidden.find(fcc) != nHidden.end()) {
      return nHidden[fcc];
    } else {
      return 3; // Be pessimistic if we don't know about a given codec
    }
  }

}; /* namespace VideoIO */

#endif
