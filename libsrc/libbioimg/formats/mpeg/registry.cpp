// $Date: 2008-11-17 17:39:15 -0500 (Mon, 17 Nov 2008) $
// $Revision: 706 $

/*
videoIO: granting easy, flexible, and efficient read/write access to video 
                 files in Matlab on Windows and GNU/Linux platforms.
    
Copyright (c) 2006 Gerald Dalley
  
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

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
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <time.h>
#include <map>
#include "registry.h"
#include "debug.h"

using namespace std;

namespace VideoIO 
{

  // We maintain a registry of all open videos here.  To lower the probability
  // of mixing up handles if the MEX function gets cleared, we use a hash on
  // the current timestamp for our first handle.
  static Handle nextHandle = (Handle)time(NULL);
  
  template<class VideoType>
  VideoType *VideoManager<VideoType>::lookupVideo(Handle handle)
  {
    TRACE;    
    typename map<Handle, VideoType*>::iterator i = openVideos.find(handle);
    if (i == openVideos.end()) {
      VrRecoverableThrow("Handle " << handle << 
                         " is not a valid video handle.");
    }
    VERBOSE("Retrieved video #" << i->first << "'s pointer: " << i->second);
    VrRecoverableCheck(i->second != NULL);
    return i->second;
  }

  template<class VideoType>
  Handle VideoManager<VideoType>::registerVideo(VideoType *vid)
  {
    TRACE;
    Handle newHandle = nextHandle++;
    openVideos[newHandle] = vid;
    return newHandle;
  }

  template<class VideoType>
  void VideoManager<VideoType>::deleteVideo(Handle handle)
  {
    TRACE;
    VideoType *vid = lookupVideo(handle);
    if (vid) {
      openVideos.erase(handle);
      delete vid;
    }
  }

  template<class VideoType>
  void VideoManager<VideoType>::deleteAllVideos()
  {
    TRACE;
    typename map<Handle, VideoType*>::iterator i;
    for (i = openVideos.begin(); i != openVideos.end(); i++) {
      delete i->second;
    }
    openVideos.clear();
  }
  
  template class VideoManager<IVideo>;  
  static IVideoManager *ivm = NULL;
  IVideoManager *registerIVideoManager(IVideoManager *mgr) throw() { 
    IVideoManager *old = ivm;
    ivm = mgr; 
    return old;
  }
  IVideoManager *iVideoManager() { 
    VrRecoverableCheckMsg(ivm != NULL, "iVideoManager is NULL");
    return ivm; 
  }

  template class VideoManager<OVideo>;  
  static OVideoManager *ovm = NULL;
  OVideoManager *registerOVideoManager(OVideoManager *mgr) throw() { 
    OVideoManager *old = ovm;
    ovm = mgr; 
    return old;
  }
  OVideoManager *oVideoManager() { 
    VrRecoverableCheckMsg(ovm != NULL, "oVideoManager is NULL");
    return ovm; 
  }

  void freeAllVideoManagers() {
    if (ovm != NULL) { delete ovm; ovm = NULL; }
    if (ivm != NULL) { delete ivm; ivm = NULL; }
  }
 
}; /* namespace VideoIO */
