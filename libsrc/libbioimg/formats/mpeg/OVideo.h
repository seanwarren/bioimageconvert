#ifndef OVIDEO_H
#define OVIDEO_H

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
#include "IVideo.h"
#include "parse.h"

namespace VideoIO
{
  /**
  * This pure abstract class defines a simple interface for writing videos 
  * to files.  Individual implementations such as DirectShowOVideo and 
  * FfmpegOVideo implement these methods as appropriate.  For the most part, 
  * its methods are self-explanatory, with the following caveats:
  *    1) Any method may throw a VrFatalError or VrRecoverableException on 
  *       error.
  *    2) An implementation may behave arbitrarily if any methods are called
  *       after construction and before a successful call to open.  Preferably
  *       a VrRecoverableException is thrown if the call cannot be completed.
  *    3) An implementation may behave arbitrarily if any methods other than 
  *       open are called after close is called.  Preferably a 
  *       VrRecoverableException is thrown if the call cannot be completed.
  *    4) The user must be allowed to call close an arbitrary number of times
  *       (i.e. close should do nothing if the video is already closed; it must
  *       not throw an exception).
  *    5) The user may call the setup method any time.  Any illegal keys, 
  *       illegal values may cause the implementation to throw a 
  *       VrRecoverableException or in extreme cases a VrFatalError.
  *       The implementation is free to reject any setup calls while the
  *       video is open by throwing a VrRecoverableException.
  *
  * The following is a typical simple usage example, assuming the class
  * FooOVideo is a concrete subclass of OVideo:
  *    FooOVideo myVid;
  *    ...do any setup for myVid such as selecting the code and/or bitrate...
  *    myVid.open(...);
  *    for (int i=0; i<10; i++) {
  *      IVideo::Frame frame;
  *      ...populate the current frame with an image...
  *      myVid.addframe(frame);
  *    }
  *    myVid.close();
  */

  class OVideo 
  {
  public:
    typedef unsigned FourCC;    
    
    // Constructors/Destructors
    virtual ~OVideo() { };

    virtual void setup(KeyValueMap &kvm) = 0;
    virtual KeyValueMap getSetupAndStats() const = 0; 

    // required frame width/height/depth for addframe calls
    virtual int  getWidth()  const = 0; 
    virtual int  getHeight() const = 0; 
    virtual int  getDepth()  const { return 3; }

    // I/O Operations
    virtual void open(std::string const &fname)    = 0;
    virtual void close()                           = 0;
    virtual void addframe(int w, int h, int d, 
                          IVideo::Frame const &f)  = 0;
    virtual bool isOpen() const = 0;
  };

}; /* namespace VideoIO */

#endif
