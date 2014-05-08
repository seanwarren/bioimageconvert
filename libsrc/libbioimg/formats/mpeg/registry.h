#ifndef REGISTRY_H
#define REGISTRY_H

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

#include "handle.h"
#include "IVideo.h"
#include "OVideo.h"
#include <set>

namespace VideoIO 
{

  /**
   * the VideoManager class is used to be a factory for new video readers or
   * writers and to maintain a registry of all valid instances.  We use this
   * so that we can keep track of valid handle -> instance pointer mappings
   * in the backend.  The present architecture assumes that a given .mex* file
   * will only have a single VideoManager type (so if you want to have a 
   * factory for DirectShowIVideo and DirectShowOVideo, you'll need to create
   * two VideoManager subclasses and a .mex* file for each of them.
   */
  template<class VideoType>
  class VideoManager
  {
  public:
    ~VideoManager() { deleteAllVideos(); };

    virtual VideoType *createVideo() throw() = 0;

    VideoType *lookupVideo(Handle handle);
    Handle     registerVideo(VideoType *vid);
    void       deleteVideo(Handle handle);
    void       deleteAllVideos();
    
  private:
    std::map<Handle, VideoType*> openVideos;
  };
  
  /** 
   * Generic input video manager
   */
  typedef VideoManager<IVideo> IVideoManager;
  
  /**
   * Generic output video manager.  Includes methods that function as the
   * equivalent to static methods for the Matlab objects.
   */
  class OVideoManager : public VideoManager<OVideo>
  {
  public:
    virtual std::set<std::string> getcodecs() throw() = 0; 
  };
  
  /** For modules where only a single manager will be used, call these 
   *  functions in the implementation file to automatically setup the
   *  manager.  Returns the old registered manager.
   */
  extern IVideoManager *registerIVideoManager(IVideoManager *mgr) throw();
  extern OVideoManager *registerOVideoManager(OVideoManager *mgr) throw();
  
  /** Get the current singleton video manager */
  extern IVideoManager *iVideoManager();
  extern OVideoManager *oVideoManager();

  extern void freeAllVideoManagers();
    
}; /* namespace VideoIO */

#endif
