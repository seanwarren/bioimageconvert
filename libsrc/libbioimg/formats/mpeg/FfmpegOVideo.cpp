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

#include <errno.h>
#include "FfmpegOVideo.h"
#include "registry.h"
#include "parse.h"
#include "FfmpegCommon.h"
#include <iostream>

using namespace std;

#if (LIBAVUTIL_VERSION_MAJOR >= 51)
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

namespace VideoIO 
{
 
  /////////////////////////////////////////////////////////////////////////////
  // defaults

  // For a default, assume 256-QAM encoding is used (as of Oct 2008, QAM 
  // encoding is the most common DRM-free cable-based transmission standard. 
  // 256-QAM is the highest-quality mpeg2 standard).
  static const int     DEFAULT_BITRATE_PER_PIX = (int)(38.8*1024*1024/1920/1080); // 
  static const AVCodecID CODEC_ID_DEFAULT = CODEC_ID_MPEG1VIDEO; 
  static const int     DEFAULT_BITRATE  = 4000000;
  static const int     DEFAULT_WIDTH    =    352;
  static const int     DEFAULT_HEIGHT   =    288;
  static const int     USE_DEFAULT_VAL  =     -1;

  /////////////////////////////////////////////////////////////////////////////
  // codec id <-> codec name mappings

  class stringcaseless {
  public:
    inline bool operator() (string const &a, string const &b) const {
      return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
  };

  static map<AVCodecID,string> codecNameFromId;
  static map<string,AVCodecID,stringcaseless> codecIdFromName;

  static void buildCodecMaps()
  {
#define PC(cn) \
      codecIdFromName[#cn]           = CODEC_ID_##cn; \
      codecNameFromId[CODEC_ID_##cn] = #cn
    PC(DEFAULT);
    

#if (LIBAVCODEC_VERSION_MAJOR >= 54)
    PC(MPEG1VIDEO);     PC(MPEG2VIDEO);    PC(MPEG2VIDEO_XVMC);    PC(H261);    PC(H263);    PC(RV10);
    PC(RV20);    PC(MJPEG);    PC(MJPEGB);    PC(LJPEG);    PC(SP5X);    PC(JPEGLS);    PC(MPEG4);
    PC(RAWVIDEO);    PC(MSMPEG4V1);    PC(MSMPEG4V2);    PC(MSMPEG4V3);    PC(WMV1);    PC(WMV2);
    PC(H263P);    PC(H263I);    PC(FLV1);    PC(SVQ1);    PC(SVQ3);    PC(DVVIDEO);    PC(HUFFYUV);
    PC(CYUV);    PC(H264);    PC(INDEO3);    PC(VP3);    PC(THEORA);    PC(ASV1);    PC(ASV2);
    PC(FFV1);    PC(4XM);    PC(VCR1);    PC(CLJR);    PC(MDEC);    PC(ROQ);    PC(INTERPLAY_VIDEO);
    PC(XAN_WC3);    PC(XAN_WC4);    PC(RPZA);    PC(CINEPAK);    PC(WS_VQA);    PC(MSRLE);
    PC(MSVIDEO1);    PC(IDCIN);    PC(8BPS);    PC(SMC);    PC(FLIC);    PC(TRUEMOTION1);
    PC(VMDVIDEO);    PC(MSZH);    PC(ZLIB);    PC(QTRLE);    PC(SNOW);    PC(TSCC);
    PC(ULTI);    PC(QDRAW);    PC(VIXL);    PC(QPEG);    PC(PNG);    PC(PPM);    PC(PBM);    PC(PGM);
    PC(PGMYUV);    PC(PAM);    PC(FFVHUFF);    PC(RV30);    PC(RV40);    PC(VC1);    PC(WMV3);    PC(LOCO);
    PC(WNV1);    PC(AASC);    PC(INDEO2);    PC(FRAPS);    PC(TRUEMOTION2);    PC(BMP);    PC(CSCD);    PC(MMVIDEO);
    PC(ZMBV);    PC(AVS);    PC(SMACKVIDEO);    PC(NUV);    PC(KMVC);    PC(FLASHSV);    PC(CAVS);
    PC(JPEG2000);    PC(VMNC);    PC(VP5);    PC(VP6);    PC(VP6F);    PC(TARGA);    PC(DSICINVIDEO);
    PC(TIERTEXSEQVIDEO);    PC(TIFF);    PC(GIF);    PC(DXA);    PC(DNXHD);    PC(THP);    PC(SGI);
    PC(C93);    PC(BETHSOFTVID);    PC(PTX);    PC(TXD);    PC(VP6A);    PC(AMV);    PC(VB);    PC(PCX);
    PC(SUNRAST);    PC(INDEO4);    PC(INDEO5);    PC(MIMIC);    PC(RL2);    PC(ESCAPE124);    PC(DIRAC);
    PC(BFI);    PC(CMV);    PC(MOTIONPIXELS);    PC(TGV);    PC(TGQ);    PC(TQI);    PC(AURA);    PC(AURA2);
    PC(V210X);    PC(TMV);    PC(V210);    PC(DPX);    PC(MAD);    PC(FRWU);    PC(FLASHSV2);    PC(CDGRAPHICS);
    PC(R210);    PC(ANM);    PC(BINKVIDEO);    PC(IFF_ILBM);    PC(IFF_BYTERUN1);    PC(KGV1);    PC(YOP);
    PC(VP8);    PC(PICTOR);    PC(ANSI);    PC(A64_MULTI);    PC(A64_MULTI5);    PC(R10K);    PC(MXPEG);
    PC(LAGARITH);    PC(PRORES);    PC(JV);    PC(DFA);    PC(WMV3IMAGE);    PC(VC1IMAGE);    PC(UTVIDEO);
    PC(BMV_VIDEO);    PC(VBLE);    PC(DXTORY);    PC(V410);    PC(XWD);    PC(CDXL);    PC(XBM);
    PC(ZEROCODEC);    PC(MSS1);    PC(MSA1);    PC(TSCC2);    PC(MTS2);    PC(CLLC);
    //PC(MSS2);
#else
    PC(MPEG1VIDEO);     PC(MPEG2VIDEO);     PC(MPEG2VIDEO_XVMC);      PC(H261);    PC(H263);           PC(RV10);       
    PC(RV20);    PC(MJPEG);    PC(MJPEGB);         PC(LJPEG);          PC(SP5X);                 PC(MPEG4);    PC(RAWVIDEO);   
    PC(MSMPEG4V1);      PC(MSMPEG4V2);            PC(MSMPEG4V3);    PC(WMV1);           PC(WMV2);           PC(H263P);            
    PC(H263I);    PC(FLV1);           PC(SVQ1);           PC(SVQ3);                 PC(DVVIDEO);    PC(HUFFYUV);    
    PC(CYUV);           PC(H264);                 PC(INDEO3);    PC(VP3);            PC(THEORA);         PC(ASV1);             
    PC(ASV2);    PC(FFV1);           PC(4XM);            PC(VCR1);                 PC(CLJR);    PC(MDEC);       
    PC(ROQ);            PC(INTERPLAY_VIDEO);      PC(XAN_WC3);    PC(XAN_WC4);        PC(RPZA);           PC(CINEPAK);          
    PC(WS_VQA);    PC(MSRLE);          PC(MSVIDEO1);       PC(IDCIN);                PC(8BPS);    PC(SMC);        
    PC(FLIC);           PC(TRUEMOTION1);          PC(VMDVIDEO);    PC(MSZH);           PC(ZLIB);           PC(QTRLE);            
    PC(SNOW);    PC(TSCC);           PC(ULTI);           PC(QDRAW);                PC(VIXL);    PC(QPEG);           //PC(XVID);
    PC(PNG);                  PC(PPM);    PC(PBM);            PC(PGM);            PC(PGMYUV);               PC(PAM);
    PC(FFVHUFF);        PC(RV30);           PC(RV40);                 PC(WMV3);           PC(LOCO);           PC(VC1);
    PC(WNV1);           PC(AASC);    PC(INDEO2);         PC(FRAPS);          PC(TRUEMOTION2);
#endif

#undef PC
  }

  static AVCodecID parseCodecId(string const &cn) {
    if (codecIdFromName.size() == 0) buildCodecMaps();

    const char *codecName = cn.c_str();
    if (codecName == NULL) return AV_CODEC_ID_NONE;

    map<string,AVCodecID,stringcaseless>::const_iterator i = codecIdFromName.find(cn);
    if (i == codecIdFromName.end()) return AV_CODEC_ID_NONE;
    return i->second;
  }

  static string getCodecName(AVCodecID id) {
    if (codecIdFromName.size() == 0) buildCodecMaps();

    map<AVCodecID,string>::const_iterator i = codecNameFromId.find(id);
    if (i == codecNameFromId.end()) return "NONE";
    return i->second;
  }

  /////////////////////////////////////////////////////////////////////////////
  // video manager

  class FfmpegOVideoManager : public OVideoManager {
  public:
    virtual OVideo *createVideo() throw() { 
      return new(nothrow) FfmpegOVideo(); 
    }
    
    virtual std::set<std::string> getcodecs() throw() {
      return getFfmpegOutputCodecNames();
    }
  };
  
  static auto_ptr<OVideoManager> oldManager(
    registerOVideoManager(new FfmpegOVideoManager()));
  
/////////////////////////////////////////////////////////////////////////////
// FfmpegOVideo

  FfmpegOVideo::FfmpegOVideo() : 
    fpsNum(30000), fpsDenom(1001),
    bitRate(USE_DEFAULT_VAL), bitRateTolerance(USE_DEFAULT_VAL),
    gopSize(USE_DEFAULT_VAL), maxBFrames(USE_DEFAULT_VAL), 
    width(USE_DEFAULT_VAL), height(USE_DEFAULT_VAL), 
    codecId(CODEC_ID_DEFAULT), 
    fmt(NULL), oc(NULL), videoStream(NULL), currFrameNum(-1), rgbPicture(NULL),
    codecPicture(NULL), outputBuffer(NULL), 
    urlOpened(false), codecOpened(false)
#ifdef VIDEO_READER_USE_SWSCALER
    , imgConvertCtx(NULL)
#endif
  { }

// (S)et (P)arameter if file is (C)onfigurable
#define SPC(pa) if (kvm.hasKey(#pa)) { pa = kvm.parseInt<int>(#pa); }

  void FfmpegOVideo::setup(KeyValueMap &kvm)
  {
     TRACE;
     avcodec_register_all();
     if (!isConfigurable()) close();
     
     int fpsNum, fpsDenom;
     if (kvm.fpsParse(fpsNum, fpsDenom)) {
       setFramesPerSecond(fpsNum, fpsDenom);
     }

     SPC(bitRate);
     SPC(bitRateTolerance);
     SPC(width);
     SPC(height);
     SPC(gopSize);
     SPC(maxBFrames);

     KeyValueMap::const_iterator cdc = kvm.find("codec");
     if (cdc != kvm.end()) {
       AVCodecID newCodecId = parseCodecId(cdc->second);
       VrRecoverableCheckMsg(newCodecId != AV_CODEC_ID_NONE,
                             "The \"" << cdc->second << "\" codec is not "
                             "recognized by your version of ffmpeg.");
       if (newCodecId != AV_CODEC_ID_NONE) {
         setCodec(newCodecId);
       }
     }
       
     KeyValueMap::const_iterator fname = kvm.find("filename");
     if (fname != kvm.end()) {
       open(fname->second);
     }

     //kvm.alertUncheckedKeys("Unrecognized arguments: "); // dima, probably not very important
  }

#define assnString(lval, x) \
  { stringstream ss; ss << x; lval = ss.str(); }

  KeyValueMap FfmpegOVideo::getSetupAndStats() const {
    TRACE;
    KeyValueMap kvm;
    // setup
    assnString(kvm["fps"],              getFramesPerSecond());
    assnString(kvm["bitRate"],          getBitRate());
    assnString(kvm["bitRateTolerance"], getBitRateTolerance());
    assnString(kvm["width"],            getWidth());
    assnString(kvm["height"],           getHeight());
    assnString(kvm["depth"],            getDepth());
    assnString(kvm["gopSize"],          getGopSize());
    assnString(kvm["maxBFrames"],       getMaxBFrames());
    assnString(kvm["codec"],            getCodecName());
               kvm["filename"]        = oc ? oc->filename : "";

    // stats
    assnString(kvm["currFrameNum"], currFrameNum); 

    return kvm;
  }

  string FfmpegOVideo::getCodecName() const {
    return VideoIO::getCodecName(getCodec());
  }

  static inline int64_t frameToTimestamp(AVCodecContext const *pCodecCtx, 
                                         int64_t frame)
  {
  #if LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0)
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
  #if LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0)
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

  /** Converts Matlab's preferred byte layout for images to C-style xRGB images.
   *  This code should be kept in sync with bgr2Matlab in FfmpegIVideo.cpp.
   */
  static inline void matlab2rgb(unsigned char *rgb, unsigned char const *mat, 
    int w, int h, int d)
  {
    for (int c=0; c<d; c++) {
      for (int x=0; x<w; x++) {
        for (int y=0; y<h; y++) {
          // We need to transpose from row-major to column-major
          rgb[(x+y*w)*d + c] = *mat++;
        }
      }
    }
  }

  void FfmpegOVideo::open(std::string const &fname)
  {
    TRACE;
    // initialize libavcodec, and register all codecs and formats 
    ffmpegInitIfNeeded();

    if (isOpen()) close();

    VERBOSE("Opening video file (" << fname << ") for writing...");

    try {
      // auto detect the output format from the name. default is mpeg. 
      #if (LIBAVCODEC_VERSION_MAJOR >= 54)
      if (formatName.size() > 0)
        fmt = av_guess_format(formatName.c_str(), NULL, NULL);
      else
        fmt = av_guess_format(NULL, fname.c_str(), NULL); 
      #else
      if (formatName.size() > 0)
        fmt = guess_format(formatName.c_str(), NULL, NULL);
      else
        fmt = guess_format(NULL, fname.c_str(), NULL);
      #endif
      if (!fmt) {
        // Could not deduce output format from file extension: using MPEG.
        #if (LIBAVCODEC_VERSION_MAJOR >= 54)
        fmt = av_guess_format("mpeg", NULL, NULL);
        #else
        fmt = guess_format("mpeg", NULL, NULL);
        #endif
      }
      VrRecoverableCheckMsg(fmt != NULL, 
        "Could not allocate even the default file format descriptor for \"" << 
        fname << "\".  Is your memory corrupted?");

      if (codecId == AV_CODEC_ID_NONE)
        codecId = fmt->video_codec;

      // allocate the output media context 
      #if (LIBAVCODEC_VERSION_MAJOR >= 54)
      VrRecoverableCheckMsg(oc = avformat_alloc_context(), 
        "Could not allocate the output media descriptor for \"" << fname <<
        "\".  Are you out of memory?");
      #else
      VrRecoverableCheckMsg(oc = av_alloc_format_context(), 
        "Could not allocate the output media descriptor for \"" << fname <<
        "\".  Are you out of memory?");
      #endif

      oc->oformat = fmt;
      snprintf(oc->filename, sizeof(oc->filename), "%s", fname.c_str());

      // add the video streams using the default format codecs
      // and initialize the codecs 
      VrRecoverableCheckMsg(fmt->video_codec != AV_CODEC_ID_NONE, 
        "Unable to determine the system default codec for \"" << fname << 
        "\".");
      
      if (codecId != AV_CODEC_ID_NONE)
        fmt->video_codec = codecId;

      currFrameNum = -1;
    } catch (VrRecoverableException const &e) {
      close();
      throw;
    } catch (VrFatalError const &e) {
      close();
      throw;
    }
  }

  /* Attempts to encode the frame pointed to by codecPicture.
   * If codecPicture==NULL, this can be used to drain the encoder
   * in preparation of closing the file.  Upon success, returns
   * the number of encoded bytes written to oc.  If 0, this
   * means that the encoder has buffered data that can be 
   * drained later by subsequent calls to encodeFrame.  On
   * error, a negative value is returned.
   */
  static int encodeFrame(AVFormatContext *oc, AVCodecContext *c, AVStream *st,
                         uint8 *outputBuffer, int OutputBufferSize, 
                         AVFrame *codecPicture)
  {
    TRACE;
    VERBOSE("c=" << c << ", outputBuffer=" << outputBuffer << " (" << 
            OutputBufferSize << " bytes), codecPicture=" << 
            codecPicture);

    /* encode the image */
    int nEncodedBytes = avcodec_encode_video(c, outputBuffer, 
                                             OutputBufferSize, codecPicture);
    VERBOSE("nEncodedBytes = " << nEncodedBytes);

    /* if zero size, it means the image was buffered */
    if (nEncodedBytes > 0) {
      AVPacket pkt;
      av_init_packet(&pkt);

#if LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0)
      if (st->r_frame_rate_base==0) {
        pkt.pts = 0; 
      } else {
        AVRational bb = {c->frame_rate_base, c->frame_rate};
        AVRational cc =  {st->r_frame_rate_base, st->r_frame_rate};
        pkt.pts = av_rescale_q(c->coded_frame->pts, bb, cc);
      }
#else
      pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, 
                             st->time_base);
#endif
      if(c->coded_frame->key_frame) pkt.flags |= PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data         = outputBuffer;
      pkt.size         = nEncodedBytes; 
      
      /* write the compressed frame in the media file */
      if (av_write_frame(oc, &pkt) < 0) return -1;
    }
    
    return nEncodedBytes;
  }

  void FfmpegOVideo::close() 
  {
    TRACE;

    VERBOSE("Draining the encoder...");
    if (oc && videoStream && getCodecFromStream(videoStream) && outputBuffer) {
      // Do I need a check for (oc->oformat->flags & AVFMT_RAWPICTURE)?
      while (encodeFrame(oc, getCodecFromStream(videoStream), videoStream, 
                         outputBuffer, OutputBufferSize, NULL) > 0);
    }

    VERBOSE("Closing video codec...");
    if (videoStream && codecOpened) {
      AVCodecContext *c = getCodecFromStream(videoStream);
      if ((c != NULL) && (avcodec_find_encoder(c->codec_id) != NULL)) {
        avcodec_close(getCodecFromStream(videoStream));
      }
    }
    videoStream = NULL;
    codecOpened = false;

    VERBOSE("Freeing the pictures and buffers...");
    if (codecPicture) {
      av_free(codecPicture->data[0]);
      av_free(codecPicture);
      codecPicture = NULL;
    }
    if (rgbPicture) {
      av_free(rgbPicture->data[0]);
      av_free(rgbPicture);
      rgbPicture = NULL;
    }
    if (outputBuffer) {
      av_free(outputBuffer);
      outputBuffer = NULL;
    }
    
    if (oc && urlOpened) {
      VERBOSE("Writing the trailer (if applicable)...");
      av_write_trailer(oc);

      VERBOSE("Freeing the streams...");
      for (int i = 0; i < oc->nb_streams; i++) {
#if LIBAVCODEC_VERSION_INT >= ((51<<16)+(7<<8)+0)
        av_freep(&oc->streams[i]->codec);
#endif
        av_freep(&oc->streams[i]);
      }
      
      if (!(fmt->flags & AVFMT_NOFILE)) {
        VERBOSE("Closing the output file...");
#if LIBAVFORMAT_VERSION_MAJOR >= 54
        avio_close(oc->pb);
#elif LIBAVFORMAT_VERSION_INT >= ((52<<16)+(0<<8)+0)
        url_fclose(oc->pb);
#else
        url_fclose(&oc->pb);
#endif
      }

      VERBOSE("Freeing the format context...");
      av_free(oc);
      oc = NULL;
    }
    urlOpened = false;
    
    currFrameNum = -1;

#ifdef VIDEO_READER_USE_SWSCALER
    VERBOSE("Cleaning up the image converter...");
    if (imgConvertCtx != NULL) {
      sws_freeContext(imgConvertCtx);
      imgConvertCtx = NULL;
    }
#endif
  }


  static void writeVideoFrame(
#ifdef VIDEO_READER_USE_SWSCALER
                              struct SwsContext *&imgConvertCtx, 
                              int rgbW, int rgbH,
#endif
                              AVFormatContext *oc, AVStream *st, 
                              int currFrameNum, AVFrame *rgbPicture,
                              AVFrame *codecPicture, 
                              uint8 *outputBuffer, int OutputBufferSize)
  {
    TRACE;
    AVCodecContext *c = getCodecFromStream(st);

    // rgbPicture is in RGB24, so we must convert it to the codec pixel format
#ifdef VIDEO_READER_USE_SWSCALER
    imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                         rgbW, rgbH, PIX_FMT_RGB24,
                                         c->width, c->height, c->pix_fmt,
                                         SWS_POINT, NULL, NULL, NULL);
    VrRecoverableCheckMsg(imgConvertCtx, 
      "Could not initialize the colorspace converter to convert from xRGB to "
      "the codec's colorspace.");
    FfRecoverableCheckMsg(
      sws_scale(imgConvertCtx, 
                rgbPicture->data, rgbPicture->linesize, 0, rgbH, 
                codecPicture->data, codecPicture->linesize),
      "Could not convert from xRGB to the stream's pixel format.");
#else
    FfRecoverableCheck(img_convert((AVPicture*)codecPicture, c->pix_fmt, 
                       (AVPicture*)rgbPicture, PIX_FMT_RGB24,
                       c->width, c->height));
#endif
    
    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
      /* raw video case. The API will change slightly in the near
         futur for that */
      AVPacket pkt;
      av_init_packet(&pkt);

      pkt.flags |= PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data         = (uint8_t *)codecPicture;
      pkt.size         = sizeof(AVPicture);

      FfRecoverableCheckMsg(av_write_frame(oc, &pkt), 
        "Could not write frame " << currFrameNum << ".  "
        "Perhaps you are out of disk space or have lost write permissions.");
    } else {
      const int nEncodedBytes = encodeFrame(oc, c, st, outputBuffer, 
                                            OutputBufferSize, codecPicture);
      /*
      VrRecoverableCheckMsg(nEncodedBytes >= 0, 
        "No bytes were encoded for frame " << currFrameNum << ".");
      */
    }
  }

  void FfmpegOVideo::addframe(int w, int h, int d,
                              IVideo::Frame const &f) 
  {
    TRACE;
    VrRecoverableCheck(isOpen());
    
    if (getWidth() == USE_DEFAULT_VAL)  setWidth(w);
    if (getHeight() == USE_DEFAULT_VAL) setHeight(h);
    
    VrRecoverableCheck(w == getWidth());
    VrRecoverableCheck(h == getHeight());
    VrRecoverableCheck(d == getDepth());
    VrRecoverableCheck(f.size() == w*h*d);

    if (isConfigurable()) finalizeOpen();

    try {
      matlab2rgb(rgbPicture->data[0], &f[0], w, h, d);

      writeVideoFrame(
#ifdef VIDEO_READER_USE_SWSCALER
                      imgConvertCtx, w, h,
#endif
                      oc, videoStream, ++currFrameNum, 
                      rgbPicture, codecPicture, 
                      outputBuffer, OutputBufferSize);
    } catch (VrRecoverableException const &e) {
      close();
      throw;
    }
  }


  // dima: writing without matlab stuff - addframe split into two functions
  void FfmpegOVideo::initFromRawFrame(int w, int h, int d) {
    TRACE;
    VrRecoverableCheck(isOpen());
    
    if (getWidth() == USE_DEFAULT_VAL)  setWidth(w);
    if (getHeight() == USE_DEFAULT_VAL) setHeight(h);
    
    VrRecoverableCheck(w == getWidth());
    VrRecoverableCheck(h == getHeight());
    VrRecoverableCheck(d == getDepth());

    if (isConfigurable()) 
      finalizeOpen();
  }

  void FfmpegOVideo::addFromRawFrame(int w, int h, int d) {
    try {
      writeVideoFrame(
#ifdef VIDEO_READER_USE_SWSCALER
                      imgConvertCtx, w, h,
#endif
                      oc, videoStream, ++currFrameNum, 
                      rgbPicture, codecPicture, 
                      outputBuffer, OutputBufferSize);
    } catch (VrRecoverableException const &e) {
      close();
      throw;
    }
  }
  // dima: end, writing without matlab stuff - split into two functions

  void FfmpegOVideo::setFramesPerSecond(double newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    VrRecoverableCheckMsg(newVal > 0, 
      "frame rate values must be positive (" << newVal << " requested).");
    if (newVal == 29.97) { // ffmpeg is really picky about NTSC rates
      fpsNum   = 30000;
      fpsDenom =  1001;
    } else {
      fpsNum   = (int)(newVal * AV_TIME_BASE);
      fpsDenom = AV_TIME_BASE;
    }
  }

  void FfmpegOVideo::setFramesPerSecond(int num, int denom)
  {
    VrRecoverableCheck(isConfigurable());
    VrRecoverableCheckMsg(num > 0 && denom > 0,
      "frame rate values must be positive, but " << num << " / " << denom << 
      " was supplied.");
    fpsNum   = num;
    fpsDenom = denom;
  }

  void FfmpegOVideo::setBitRate(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    bitRate = newVal; 
  }

  void FfmpegOVideo::setBitRateTolerance(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    bitRateTolerance = newVal; 
  }

  void FfmpegOVideo::setWidth(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    width = newVal; 
  }

  void FfmpegOVideo::setHeight(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    height = newVal; 
  }

  void FfmpegOVideo::setGopSize(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    gopSize = newVal; 
  }

  void FfmpegOVideo::setMaxBFrames(int newVal)
  { 
    VrRecoverableCheck(isConfigurable()); 
    maxBFrames = newVal; 
  }

  void FfmpegOVideo::setCodec(AVCodecID newCodecId)
  {
    TRACE;
    VrRecoverableCheck(isConfigurable());
    if ((newCodecId == CODEC_ID_ASV1) || (newCodecId == CODEC_ID_ASV2) || 
        (newCodecId == CODEC_ID_SVQ1)) {
      VrRecoverableThrow("The " << VideoIO::getCodecName(newCodecId) << 
                         " codec segfaults (crashes) in some versions of "
                         "ffmpeg.  If you believe your version of ffmpeg "
                         "has fixed this problem, please find the first "
                         "ffmpeg version that supports it and add proper "
                         "LIBAVCODEC_VERSION_INT ifdefs to "
                         "FfmpegOVideo::setCodec(AVCodecID).\n");
    }
    codecId = newCodecId;
  }

  void FfmpegOVideo::setCodec(string const &codecName)
  { 
    AVCodecID newCodecId = parseCodecId(codecName);
    VrRecoverableCheckMsg(newCodecId != AV_CODEC_ID_NONE, 
      "No \"" << codecName << "\" codec could be found.");
    setCodec(newCodecId);
  }

  bool FfmpegOVideo::isOpen() const 
  { 
    return fmt && oc;// && videoStream && rgbPicture && codecPicture && 
    //outputBuffer && urlOpened && codecOpened;
  }
  
  bool FfmpegOVideo::isConfigurable() const
  {
    return currFrameNum < 0;
  }

  #if (LIBAVUTIL_VERSION_MAJOR >= 50)
  static AVFrame *allocPicture(enum PixelFormat pixFmt, int width, int height)
  #else
  static AVFrame *allocPicture(int pixFmt, int width, int height)
  #endif
  {
    TRACE;
    AVFrame *codecPicture = avcodec_alloc_frame();
    if (!codecPicture) return NULL;
    const int size = avpicture_get_size(pixFmt, width, height);
    uint8_t *codecPictureBuf = (uint8_t*)av_malloc(size);
    if (!codecPictureBuf) {
      av_free(codecPicture);
      return NULL;
    }
    VrRecoverableCheck(
      avpicture_fill((AVPicture *)codecPicture, codecPictureBuf,
                     pixFmt, width, height) == size);

    return codecPicture;
  }

  void FfmpegOVideo::finalizeOpen()
  {
    TRACE;
    VrRecoverableCheck(isConfigurable());

    try {
      fmt->video_codec = codecId;
      VrRecoverableCheck(videoStream = addVideoStream());

      // set the output parameters (must be done even if no
      // parameters). 
      //FfRecoverableCheck(av_set_parameters(oc, NULL)); // dima: depricated, now parameters are passed directly to avformat_write_header 

#ifdef PRINT_INFOS
      dump_format(oc, 0, fname.c_str(), 1);
#endif

      // now that all the parameters are set, we can open the 
      // video codecs and allocate the necessary encode buffers 
      openVideo();

      // open the output file, if needed 
      if (!(fmt->flags & AVFMT_NOFILE)) {
        #if LIBAVFORMAT_VERSION_MAJOR >= 54
        FfRecoverableCheckMsg(
          avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE), 
          "Could not open \"" << oc->filename << "\" for writing.");
        #else
        FfRecoverableCheckMsg(
          url_fopen(&oc->pb, oc->filename, URL_WRONLY), 
          "Could not open \"" << oc->filename << "\" for writing.");
        #endif
        urlOpened = true;
      }

      // write the stream header, if any 
      FfRecoverableCheckMsg(avformat_write_header(oc, NULL), 
                            "Could not write file header for \"" << 
                            oc->filename);
      
    } catch (VrRecoverableException const &e) {
      close();
      throw;
    } catch (VrFatalError const &e) {
      close();
      throw;
    }
  }

  void FfmpegOVideo::openVideo()
  {
    TRACE;
    AVCodecContext *c = getCodecFromStream(videoStream);
    VrRecoverableCheck(c != NULL);

    //dima: attempt to fix h264 encoding
    if (c->codec_id == AV_CODEC_ID_H264) {
      c->me_range = 16;
      c->max_qdiff = 4;
      c->qmin = 10;
      c->qmax = 51;
      c->qcompress = 0.6; 
    }
    //dima: attempt to fix h264 encoding

    VERBOSE("Finding the video encoder...");
    AVCodec *codec = avcodec_find_encoder(c->codec_id);
    VrRecoverableCheckMsg(codec != NULL, 
                          "Could not find a(n) " << getCodecName() << 
                          " encoder");

    VERBOSE("Selecting the pixel format...");
    if (codec && codec->pix_fmts){
      const enum PixelFormat *p= codec->pix_fmts;
      for(; *p!=-1; p++){
        if (*p == c->pix_fmt) break;
      }
      if (*p == -1) c->pix_fmt = codec->pix_fmts[0];
    }

    VERBOSE("Opening the codec...");
    #if (LIBAVCODEC_VERSION_MAJOR >= 54)
    FfRecoverableCheckMsg(avcodec_open2(c, codec, NULL),
                          "Could not initialize the codec.  "
                          "Perhaps you've chosen an unsupported frame rate.");
    #else
    FfRecoverableCheckMsg(avcodec_open(c, codec),
                          "Could not initialize the codec.  "
                          "Perhaps you've chosen an unsupported frame rate.");
    #endif

    // When avcodec_open fails, it tends to leave a mess, so we use a flag
    // to tell us if it succeeded.  There a chance that our close method will 
    // leak memory on failed avcodec_open attempts.
    codecOpened = true;

    outputBuffer = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
      VERBOSE("Allocating the output buffer...");
      VrRecoverableCheck(outputBuffer = (uint8*)av_malloc(OutputBufferSize));
    }

    VERBOSE("Allocating the encoded raw codec picture...");
    VrRecoverableCheck(
      codecPicture = allocPicture(c->pix_fmt, c->width, c->height));
    
    VERBOSE("Allocating input image wrapper...");
    VrRecoverableCheckMsg(
      rgbPicture = allocPicture(PIX_FMT_RGB24, c->width, c->height),
      "Could not allocate a " << c->width << "x" << c->height << 
      " xRGB image.  Perhaps you are out of memory.");    
  }

#define SETINTVAL(varname, fieldname) \
  if (varname != USE_DEFAULT_VAL) {   \
    c->fieldname = varname;           \
  } 

#define SETINTVALDEFAULT(varname, fieldname, defaultVal) \
  if (varname != USE_DEFAULT_VAL) {                      \
    c->fieldname = varname;                              \
  } else {                                               \
    c->fieldname = defaultVal;                           \
  }

  AVStream *FfmpegOVideo::addVideoStream()
  {
    TRACE;
    AVCodecContext *c;
    AVStream *st;
    
    if (fpsNum   <= 0) return NULL;
    if (fpsDenom <= 0) return NULL;
    if (getWidth()  <= 0) return NULL;
    if (getHeight() <= 0) return NULL;
    
    #if LIBAVFORMAT_VERSION_MAJOR >= 56
    st = avformat_new_stream(oc, NULL);
    #else
    const int videoStreamIdx = 0;
    st = av_new_stream(oc, videoStreamIdx);
    #endif

    if (!st) return NULL;

    c = getCodecFromStream(st);
    c->codec_id   = codecId;
    c->codec_type = CODEC_TYPE_VIDEO;
#if (LIBAVCODEC_VERSION_INT > 0x000409) || (LIBAVCODEC_VERSION_INT == 0x000409 && LIBAVCODEC_BUILD >= 4753)
    c->pix_fmt    = PIX_FMT_NONE; // let the codec decide
#else
    // PIX_FMT_NONE wasn't introduced until svn revision 4161
    c->pix_fmt    = (PixelFormat)-1;
#endif

    // dima, set particular fourcc
    if (video_codec_tag)
      c->codec_tag = video_codec_tag;

    // Set encoder params.  See AVCodecContext docs in avcodec.h for details.
    // We could expand the set of supported params further.
    SETINTVAL       (width,            width);
    SETINTVAL       (height,           height);
    SETINTVAL       (gopSize,          gop_size); 
    SETINTVAL       (maxBFrames,       max_b_frames);
    SETINTVALDEFAULT(bitRate,          bit_rate, width*height*DEFAULT_BITRATE_PER_PIX);
    SETINTVAL       (bitRateTolerance, bit_rate_tolerance);
    
#if LIBAVCODEC_VERSION_INT < ((51<<16)+(7<<8)+0)
    c->frame_rate_base = fpsDenom;
    c->frame_rate      = fpsNum;
#else
    c->time_base.num   = fpsDenom;
    c->time_base.den   = fpsNum;
#endif

    if (c->codec_id == CODEC_ID_MPEG1VIDEO){
      // needed to avoid using macroblocks in which some coeffs overflow
      // this doesnt happen with normal video, it just happens here as the
      // motion of the chroma plane doesnt match the luma plane 
      c->mb_decision = FF_MB_DECISION_RD;
    }
    
    // some formats want stream headers to be seperate
    if (strcmp(oc->oformat->name, "mp4")==0 || 
        strcmp(oc->oformat->name, "mov")==0 || 
        strcmp(oc->oformat->name, "3gp")==0) {
      c->flags |= CODEC_FLAG_GLOBAL_HEADER; 
    }
    
    return st;
  }
};
