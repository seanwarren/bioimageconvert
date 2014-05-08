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

#include <iostream>
#include <errno.h>
#include "FfmpegIVideo.h"
#include "registry.h"
#include "parse.h"

using namespace std;

namespace VideoIO
{
  class FfmpegIVideoManager : public IVideoManager
  {
  public:
    virtual IVideo *createVideo() throw() {
      return new(nothrow) FfmpegIVideo();
    }
  };
  static auto_ptr<IVideoManager> oldManager(
    registerIVideoManager(new FfmpegIVideoManager()));
  
  template <class T>
  static void squeeze(vector<T> &v) {
    vector<T>(v).swap(v);
  }

  static inline int64_t frameToTimestamp(AVCodecContext const *pCodecCtx, 
                                         int64_t frame)
  {
  #if (LIBAVCODEC_VERSION_MAJOR < 52) && (LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0))
    return (int64_t)(1000 * frame * 
                   pCodecCtx->frame_rate_base / pCodecCtx->frame_rate);
  #else
    return (int64_t)(1000 * frame * 
                   pCodecCtx->time_base.num / pCodecCtx->time_base.den);
  #endif
  }
  
  static inline int64_t timestampToFrame(AVCodecContext const *pCodecCtx, 
                                         int64_t ts)
  {
  #if (LIBAVCODEC_VERSION_MAJOR < 52) && (LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0))
    int64_t f = (int64_t)(ts * pCodecCtx->frame_rate / 
                      1000 / pCodecCtx->frame_rate_base);
  #else
    int64_t f = (int64_t)(ts * pCodecCtx->time_base.den /
                      1000 / pCodecCtx->time_base.num);
  #endif
    // Provide consistent handling of roundoff errors.  Example: at 30fps,
    // frame 10 --> 333.333...ms, which gets rounded down to 333ms.
    // If we then say timestampToFrame(...,333), we get frame 9, not
    // frame 10.  We choose to make the frame numbers authoritative 
    // and we make timestamp rounding consistent with the frame numbers.
    if (ts == frameToTimestamp(pCodecCtx, f+1)) f++;
    return f;
  }


  FfmpegIVideo::FfmpegIVideo() :
    fname(""), 
    currentFrameNumber(-1), currentTimestamp(-1), pFormatCtx(NULL), 
    videoStream(-1), pCodecCtx(NULL), pFrame(NULL), pFrameBGR(NULL), 
    buffPosition(0), 
#ifdef VIDEO_READER_USE_SWSCALER
    imgConvertCtx(NULL), 
#endif
    nHiddenFinalFrames(0), dropBadPackets(true)
  { 
    TRACE;
    packet.data = NULL;
    // Register all formats and codecs
    ffmpegInitIfNeeded();
    convert_to_matlab = true;
  }

  /** Converts C-style BGR images to Matlab's preferred byte layout for images.
   * I've optimized it as much as I thought I reasonably could.  It turns out
   * that using the dy pointer arithmetic on bgrData yields substantial 
   * performance benefits (15-25% faster reading benchmark speeds), but 
   * additional arithmetic with c or x yields no benefits or even negative
   * benefits.  These tests were run using Matlab 2007a on machines with the
   * following specs:
   *   2.2GHz Xeon (P4-based),     32-bit Linux 2.6.15, gcc 3.3.5
   *   2.4GHz Core 2 Duo (Conroe), 64-bit Linux 2.6.17, gcc 3.3.5
   *   2.33GHz Xeon (Woodcrest),   64-bit Linux 2.6.18, gcc 4.1.2
   */
  static inline void bgrToMatlab(unsigned char *out, 
    unsigned char const *bgrData, 
    int w, int h, int d)
  {    
    const int dy = w*d;
    for (int c=0; c<d; c++) {
      for (int x=0; x<w; x++) {
        unsigned char const *b = &bgrData[x*d + (d-1-c)];
        for (int y=0; y<h; y++) {
          // We need to transpose from row-major to column-major, and do 
          // BGR->xRGB conversion.
          //*out++ = bgrData[(x+y*w)*d + (d-1-c)]; // without unrolling
          *out++ = *b; 
          b += dy;
        }
      }
    }
  }

  /*
  int64_t FfmpegIVideo::currentTimestamp() const {
    return frameToTimestamp(pCodecCtx, currentFrameNumber);
  }
  */

  bool FfmpegIVideo::next() 
  {
    TRACE;
    VrRecoverableCheckMsg(isOpen(), "No video file is open.");
    
    VERBOSE("About to get frame " << currentFrameNumber+1);
    if (!getNextFrame()) return false;

    VERBOSE("About to convert frame"); 

#ifdef VIDEO_READER_USE_SWSCALER
    imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                         // what the decoder gives us
                                         pCodecCtx->width, pCodecCtx->height, 
                                         pCodecCtx->pix_fmt,
                                         // what we want
                                         width(), height(), 
                                         PIX_FMT_BGR24,
                                         // how we want it
                                         SWS_POINT, NULL, NULL, NULL);
    VrRecoverableCheckMsg(imgConvertCtx, 
      "Could not initialize the colorspace converter to produce BGR output");
    FfRecoverableCheckMsg(
      sws_scale(imgConvertCtx, 
                pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
                pFrameBGR->data, pFrameBGR->linesize),
      "Could not convert from the stream's pixel format to BGR.");
#else
    FfRecoverableCheck(
      img_convert((AVPicture *)pFrameBGR, PIX_FMT_BGR24, (AVPicture*)pFrame, 
                  pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height));
#endif
    VERBOSE("Frame gotten and converted.");

    #ifndef FFMPEG_VIDEO_DISABLE_MATLAB
    if (convert_to_matlab)
      bgrToMatlab(&currentFrame[0], &bgrData[0], width(), height(), depth());
    #endif
      
    currentFrameNumber++;
    currentTimestamp = frameToTimestamp(pCodecCtx, currentFrameNumber);
    
    return true;
  }

  bool FfmpegIVideo::step(int frameDelta) 
  { 
    TRACE;
    VrRecoverableCheckMsg(isOpen(), "No video file is open.");

    // Early exits
    if (!(frameDelta + currentFrameNumber >= 0)) return false;
    if (frameDelta == 0) return true;

    // Restart from the beginning for backward steps
    string const filename(pFormatCtx->filename);
    if (frameDelta < 0) {
      frameDelta = currentFrameNumber + frameDelta + 1;
      open(filename.c_str());
    }

    // Speedy advance that avoids decoding frames
    const int origFrame = currentFrameNumber;
    if (!stepLowLevel(frameDelta)) {
      // If the step failed, try to go back to the frame we were previously
      // observing.  If that fails (e.g. because the file was corrupted or
      // deleted), then give up.
      open(filename.c_str());
      if (origFrame >= 0) {
        stepLowLevel(origFrame);
      }
      return false;
    }
    return true;  
  }

  bool FfmpegIVideo::stepLowLevel(int frameDelta)
  {
    TRACE;
    //VrRecoverableCheck(frameDelta > 0);
    frameDelta--;
    while (frameDelta-- > 0) {
      if (!getNextFrame()) return false;
      currentFrameNumber++;
    }
    return next();
  }

  bool FfmpegIVideo::seek(int toFrame) 
  { 
    TRACE;
    return step(toFrame - currentFrameNumber);
  }

  IVideo::ExtraParamsAndStats FfmpegIVideo::extraParamsAndStats() const 
  {
    TRACE;
    IVideo::ExtraParamsAndStats params;
    params["preciseFrames"]  = "-1";
    params["dropBadPackets"] = toString((int)dropBadPackets);
    return params;
  }

  void FfmpegIVideo::open(KeyValueMap &kvm) 
  {
    TRACE;

    std::string fname;

    // Extract arguments
    VrRecoverableCheckMsg(kvm.hasKey("filename"), 
                          "The filename must be specified");
    for (KeyValueMap::const_iterator i=kvm.begin(); i!=kvm.end(); i++) {
      if (strcasecmp("filename", i->first.c_str())==0) {
        fname = i->second; // no type conversion necessary
      } else if (strcasecmp("preciseFrames", i->first.c_str())==0) {
        // for now, all ffmpeg seeks are precise, so ignore this option.
      } else if (strcasecmp("dropBadPackets", i->first.c_str())==0) {
        dropBadPackets = (bool)kvm.parseInt<int>("dropBadPackets");
      } else {
        VrRecoverableThrow("Unrecognnized argument name: " << i->first);
      }
    }

    // Do all the work to open the file.
    open(fname);
  }

  void FfmpegIVideo::open(std::string const &fname) {
    TRACE;
    if (isOpen()) close();

    try {
      this->fname = fname;

      // Open video file
      VERBOSE("opening video file (" << fname.c_str() << ") for reading...");
      #if (LIBAVFORMAT_VERSION_MAJOR >= 54)
      FfRecoverableCheckMsg(
        avformat_open_input(&pFormatCtx, fname.c_str(), NULL, NULL), 
        "Could not open \"" << fname << "\".  Make sure the filename is "
        "correct, that the file is not corrupted, and that ffmpeg can "
        "play the file.");
      #else
      FfRecoverableCheckMsg(
        av_open_input_file(&pFormatCtx, fname.c_str(), NULL, 0, NULL), 
        "Could not open \"" << fname << "\".  Make sure the filename is "
        "correct, that the file is not corrupted, and that ffmpeg can "
        "play the file.");
      #endif

      VrRecoverableCheck(pFormatCtx != NULL);
      PRINTINFO("There are " << pFormatCtx->nb_streams << " streams.");
      
      // Retrieve stream information
      PRINTINFO("finding stream info...");
      FfRecoverableCheckMsg(av_find_stream_info(pFormatCtx),
                            "Could not find the stream info for \"" << 
                            fname << "\".");
  
      // Dump information about file onto standard error
#ifdef PRINT_VERBOSES      
      dump_format(pFormatCtx, 0, fname.c_str(), false);
#endif      
  
      // Find the first video stream
      PRINTINFO("Finding first video stream among " << 
                (int)pFormatCtx->nb_streams << " streams...");  
      videoStream = -1;
      for (int i=0; i<pFormatCtx->nb_streams; i++) {
        if (getCodecFromStream(pFormatCtx->streams[i])->codec_type == 
            CODEC_TYPE_VIDEO) {
          videoStream = i;
          break;
        }
      }
#ifdef PRINT_VERBOSES
      if (videoStream == -1) VERBOSE("Did not find any video streams");
#endif
      // Make sure we found the video stream
      VrRecoverableCheckMsg(videoStream != -1, 
                            "Could not find any video streams in \"" << fname 
                            << "\".  Is this an audio file?");  
  
      // Get a pointer to the codec context for the video stream
      PRINTINFO("getting codec context...");
      VrRecoverableCheckMsg(
        pCodecCtx = getCodecFromStream(pFormatCtx->streams[videoStream]),
        "Unable to find a codec for " << fname);
      
      // Find the decoder for the video stream
      PRINTINFO("finding decoder...");
#if (LIBAVCODEC_VERSION_MAJOR < 52) && (LIBAVCODEC_VERSION_INT == 0x0409) 
      if ((pCodecCtx->codec_id == CODEC_ID_WMV3) ||
          (pCodecCtx->codec_id == CODEC_ID_VC9)) {
        pCodecCtx->codec_id = CODEC_ID_NONE;
        VrRecoverableThrow("Unfortunately, the WMV3/VC1/VC9 (Windows Media 9) "
                           "codec is buggy in your version of ffmpeg.  It "
                           "doesn't decode chroma information correctly.");
      }
#endif
      AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
      VrRecoverableCheckMsg(pCodec != NULL,
                 "Could not find a video decoder for stream " << videoStream <<
                 " in \"" << fname << "\".");
  
      pCodecCtx->debug_mv          = 0;
      pCodecCtx->debug             = 0;
      pCodecCtx->workaround_bugs   = 1;
      pCodecCtx->lowres            = 0;
      if(pCodecCtx->lowres) pCodecCtx->flags |= CODEC_FLAG_EMU_EDGE;
      pCodecCtx->idct_algo         = FF_IDCT_AUTO;
      //if(fast) pCodecCtx->flags2  |= CODEC_FLAG2_FAST;
#if (LIBAVCODEC_VERSION_MAJOR >= 52) || (LIBAVCODEC_VERSION_INT > 0x000409) || (LIBAVCODEC_VERSION_INT == 0x000409 && LIBAVCODEC_BUILD >= 4758)
      // These three fields were introduced after 0.4.9-pre1 used for the 
      // Debian Sarge build, but before the version number was changed from 
      // 0.4.9 (svn revision 4440).
      pCodecCtx->skip_frame        = AVDISCARD_DEFAULT;
      pCodecCtx->skip_idct         = AVDISCARD_DEFAULT;
      pCodecCtx->skip_loop_filter  = AVDISCARD_DEFAULT;
#endif


#if (LIBAVCODEC_VERSION_MAJOR >= 54) 
      pCodecCtx->err_recognition  = AV_EF_CAREFUL;
#elif (LIBAVCODEC_VERSION_MAJOR >= 52) 
      pCodecCtx->error_recognition  = FF_ER_CAREFUL;
#elif LIBAVCODEC_VERSION_INT >= ((50<<16)+(0<<8)+0)
      pCodecCtx->error_resilience  = FF_ER_CAREFUL;
#else
      pCodecCtx->error_resilience  = FF_ER_CAREFULL; // typo in 0.4.9
#endif
      pCodecCtx->error_concealment = 3;
      
      // Open codec
      PRINTINFO("opening codec...");
      FfRecoverableCheckMsg(avcodec_open2(pCodecCtx, pCodec, NULL),
                            "Could not open the codec for \"" << 
                            fname << "\".");
    
      // Hack to correct wrong frame rates that seem to be generated by some 
      // codecs 
#if (LIBAVCODEC_VERSION_MAJOR < 52) && (LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0))
      if (pCodecCtx->frame_rate>1000 && pCodecCtx->frame_rate_base==1) {
        PRINTINFO("correcting frame rate");
        pCodecCtx->frame_rate_base=1000;
      }
#else
      if (pCodecCtx->time_base.den>1000 && pCodecCtx->time_base.num==1) {
        PRINTINFO("correcting frame rate");
        pCodecCtx->time_base.num=1000;
      }
#endif
  
      // Allocate video frame
      PRINTINFO("allocating video frame...");
      VrRecoverableCheck((pFrame = avcodec_alloc_frame()) != NULL);  
    
      // Allocate an AVFrame structure
      PRINTINFO("allocating avframe struct...");
      VrRecoverableCheck((pFrameBGR = avcodec_alloc_frame()) != NULL);
    
      // Allocate BGR adn Matlab buffers
      PRINTINFO("Creating bitmap image ("<<pCodecCtx->width<<"x"<<
                pCodecCtx->height<<")...");
      bgrData.resize(pCodecCtx->width * pCodecCtx->height * depth());
      currentFrame.resize(pCodecCtx->width * pCodecCtx->height * depth());
  
      // Assign appropriate parts of buffer to image planes in pFrameBGR
      PRINTINFO("Setting up pixel transfer structure...");
      VrRecoverableCheck(3 * pCodecCtx->width * pCodecCtx->height ==
        avpicture_fill((AVPicture *)pFrameBGR, (uint8_t*)&bgrData[0], 
                       PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height));
  
      packet.data = NULL;
      
      nHiddenFinalFrames = guessNumHiddenFinalFrames(fourcc());
        
      PRINTINFO("done.");
    } catch (...) {
      close();
      throw;
    }
  }

  void FfmpegIVideo::close() 
  {
    TRACE;
    // Clean up partially-decoded streams
    buffPosition = 0;
    dataBuffer.resize(0);
    squeeze(dataBuffer);
    if (packet.data != NULL) {
      av_free_packet(&packet);
    }
  
    // Free the xRGB image
    if (pFrameBGR != NULL) { 
      PRINTINFO("About to free pFrameBGR");
      av_free(pFrameBGR); 
      pFrameBGR = NULL; 
    }
    
    // Free the YUV frame
    if (pFrame != NULL) { 
      PRINTINFO("About to free pFrame");
      av_free(pFrame); 
      pFrame = NULL; 
    }
    
    // Close the codec
    if (pCodecCtx != NULL) { 
      PRINTINFO("About to free pCodecCtx");
      PRINTINFO("Codec ID: " <<  pCodecCtx->codec_id << ""); 
      if (pCodecCtx->codec_id != AV_CODEC_ID_NONE) avcodec_close(pCodecCtx); 
      pCodecCtx = NULL; 
    }
  
    videoStream = -1;
    
    // Close the video file
    if (pFormatCtx != NULL) { 
      PRINTINFO("About to free pFormatCtx");
      av_close_input_file(pFormatCtx); 
      pFormatCtx = NULL; 
    }
  
    currentFrame.resize(0);
    bgrData.resize(0);
    currentTimestamp   = -1;
    currentFrameNumber = -1;
    fname              = "";

    squeeze(currentFrame);
    squeeze(bgrData);

#ifdef VIDEO_READER_USE_SWSCALER
    // Clean up image converter
    if (imgConvertCtx != NULL) {
      sws_freeContext(imgConvertCtx);
      imgConvertCtx = NULL;
    }
#endif

    nHiddenFinalFrames = 0;  
  }
  
  bool FfmpegIVideo::getNextFrame()
  {
    TRACE;
    size_t          totalBytesDecoded = 0;
    int             bytesRemaining = dataBuffer.size() - (int)buffPosition;
  
    try {
      // Decode packets until we have decoded a complete frame
      pCodecCtx->debug = 1;
      while (true) {
        // Work on the current packet until we have decoded all of it
        while (bytesRemaining > 0) {
          // Decode the next chunk of data
          int       frameFinished = 0;
          #if (LIBAVCODEC_VERSION_MAJOR >= 54)
          //AVPacket avpkt;
          //avpkt.data = &dataBuffer[buffPosition];
          //avpkt.size = bytesRemaining;
          int const bytesDecoded = 
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
          #else
          int const bytesDecoded = 
            avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, 
                                 &dataBuffer[buffPosition], bytesRemaining);
          #endif

          if (bytesDecoded >= 0) {
            totalBytesDecoded += bytesDecoded;
          }
          VERBOSE("  decoded "<<bytesDecoded<<" bytes ("<<totalBytesDecoded<<
                  " total), frame "<<(frameFinished ? "" : "not ")<<
                  "finished");
            
          if (bytesDecoded >= 0) {
          bytesRemaining -= bytesDecoded;
          buffPosition   += bytesDecoded;
          } else { 
            // Error decoding the packet... 
            if (dropBadPackets) {
              // just discard the packet and try the next one (what ffmpeg.c 
              // and ffplay.c do)
              if (packet.data != NULL) av_free_packet(&packet);
              break;
            } else {
              // play it safe and complain
              VrRecoverableCheckMsg(false, // using *Check* to embed context 
                                    "Error decoding packet.\n" <<
                                    (ffBuffer[0] ? ffBuffer : "\n"));
            }
          }
            
          // Did we finish the current frame? Then we can return
          if (frameFinished) {
            VERBOSE("  codec says the frame is finished (frame "<<
                    pCodecCtx->frame_number<<", "<<
                    pCodecCtx->frame_skip_factor<<") (bytesRemaining="<<
                    bytesRemaining<<").");
            VrRecoverableCheck(totalBytesDecoded > 0);
            return true;
          }
        }
        
        // Read the next packet, skipping all packets that aren't for this 
        // stream
        do {
          if (packet.data != NULL) av_free_packet(&packet);
          if (av_read_frame(pFormatCtx, &packet) < 0) {
            // Decode the rest of the last frame
            int       frameFinished = 0;
            #if (LIBAVCODEC_VERSION_MAJOR >= 54)
            int const bytesDecoded = 
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            #else
            int const bytesDecoded = avcodec_decode_video(pCodecCtx, pFrame, 
              &frameFinished, &dataBuffer[buffPosition], bytesRemaining);
            #endif
            if (bytesDecoded >= 0) {
              totalBytesDecoded += bytesDecoded;
            }
            VERBOSE("  decoded "<<bytesDecoded<<" bytes ("<<totalBytesDecoded<<
              " total) from final packet");
            if (packet.data != NULL) av_free_packet(&packet);
            buffPosition = 0;
            dataBuffer.resize(0);

            VrRecoverableCheck(frameFinished);
            VrRecoverableCheck(totalBytesDecoded > 0);
            return true;
          }
          VERBOSE("  "<<((packet.stream_index==videoStream) ? 
                         "Found":"skipping")<<" packet with "<<
                  packet.size<<" bytes");
        } while (packet.stream_index != videoStream);
      
        bytesRemaining = packet.size;
        buffPosition = 0;
        dataBuffer.resize(bytesRemaining);
        #if (LIBAVCODEC_VERSION_MAJOR < 54)
        memcpy(&dataBuffer[0], packet.data, bytesRemaining);
        #endif
      }
    } catch (VrRecoverableException const &e) {
      if (packet.data != NULL) av_free_packet(&packet);
      close();
      return false;
    } catch (VrFatalError const &e) {
      if (packet.data != NULL) av_free_packet(&packet);
      close();
      throw;
    }
  }
  
  AVRational FfmpegIVideo::fpsRational() const
  {
    if (!isOpen()) {
      AVRational err = {-1, 0};
      return err;
    }
#if (LIBAVCODEC_VERSION_MAJOR < 52) && (LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0))
    AVRational fps;
    fps.num = pCodecCtx->frame_rate;
    fps.den = pCodecCtx->frame_rate_base * 1000;
    return fps;
#else
    AVStream *st = pFormatCtx->streams[videoStream];
    if (st->r_frame_rate.den && st->r_frame_rate.num) {
      return st->r_frame_rate;
      // } else if(st->time_base.den && st->time_base.num) {
      //   AVRational fps;
      //   fps.num = st->time_base.den;
      //   fps.den = st->time_base.num;
      //   return fps;
    } else {
      AVRational fps;
      fps.num = pCodecCtx->time_base.den;
      fps.den = pCodecCtx->time_base.num;
      return fps;
    }
#endif    
  }

  double FfmpegIVideo::fps() const 
  {
    AVRational const fps = fpsRational();
    return av_q2d(fps);
  }
  
  int FfmpegIVideo::numFrames() const 
  { 
    VrRecoverableCheck(isOpen()); 
#if (LIBAVCODEC_VERSION_MAJOR >= 52) || (LIBAVCODEC_VERSION_INT > 0x000409) || (LIBAVCODEC_VERSION_INT == 0x000409 && LIBAVCODEC_BUILD >= 4758)
    const int nReported = (int)pFormatCtx->streams[videoStream]->nb_frames;
    if (nReported == 0) {
      AVRational const fps        = fpsRational();    

      int64_t nFrames = ((int64_t)fps.num * pFormatCtx->duration) / 
                        ((int64_t)fps.den * AV_TIME_BASE);
      if (nFrames <= 0) return -1;
      VrRecoverableCheckMsg(nFrames <= numeric_limits<int>::max(), 
                            "Overflow when computing the number of frames");
      return (int)nFrames - nHiddenFinalFrames;
    }
    return nReported - nHiddenFinalFrames; 
#else
    // nb_frames was introduced in svn revision 4390. 
    return -1;
#endif
  }

};
