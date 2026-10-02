//
// Created by bkg2k on 14/05/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <SDL_audio.h>
#include <utils/datetime/HighResolutionTimer.h>
#include "VideoEngine.h"

#include "Renderer.h"
#include "rendering/textures/TextureManager.h"

#define PIXEL_FORMAT AV_PIX_FMT_RGBA

#define RETURN_ERROR(x, y) do{ LOG(LogError) << "[Video Engine]" << x; return y; }while(false)

VideoEngine* VideoEngine::mSoundToken;
VideoEngine* VideoEngine::mAudioDeviceOwner;
Mutex VideoEngine::mStaticSyncer;
Mutex VideoEngine::mAudioSyncer;
Array<VideoEngine*> VideoEngine::mSleepingEngines;

unsigned char VideoEngine::mConvertedData[(192000 * 3) / 2];
unsigned char* VideoEngine::mConvertedDataPointer = &VideoEngine::mConvertedData[0];
unsigned char VideoEngine::mAudioBuffer[(192000 * 3) / 2];
unsigned int VideoEngine::mAvailableAudio = 0;
unsigned int VideoEngine::mConsumedAudio = 0;

void VideoEngine::ClearAudioBuffers()
{
  memset(mConvertedData, 0, sizeof(mConvertedData));
  memset(mAudioBuffer, 0, sizeof(mAudioBuffer));
}

bool VideoEngine::OpenAudioDevice(const AVCodecContext& audioContext)
{
  #ifdef OPTION_RECALBOX_NO_AUDIO
  (void)audioContext;
  return false;
  #else
  // mStaticSyncer - not mAudioSyncer - guards the device handover: SDL_CloseAudio() waits for the
  // running callback, and that callback takes mAudioSyncer.
  Mutex::AutoLock locker(mStaticSyncer);

  // Only the token holder is allowed on the device, so the shared audio buffers never see two
  // decoders at once. Tested inside the lock: the token can change under us otherwise.
  if (mSoundToken != this) return false;

  // The legacy SDL audio API drives a single device: take it back from whoever holds it
  if (mAudioDeviceOwner != nullptr)
  {
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    mAudioDeviceOwner = nullptr;
  }

  SDL_AudioSpec wanted;
  SDL_AudioSpec obtained;
  wanted.channels = audioContext.channels;
  wanted.freq = audioContext.sample_rate;
  wanted.format = AUDIO_F32;
  wanted.silence = 0;
  wanted.samples = SDL_AUDIO_BUFFER_SIZE;
  wanted.userdata = this;
  wanted.callback = AudioCallback;

  if (SDL_OpenAudio(&wanted, &obtained) < 0)
  { LOG(LogError) << "[Video Engine] Cannot open audio device: " << SDL_GetError(); return false; }

  // Start from clean buffers so the new stream does not replay the tail of the previous one
  { Mutex::AutoLock audioLocker(mAudioSyncer); ClearAudioBuffers(); mConsumedAudio = 0; mAvailableAudio = 0; }

  mAudioDeviceOwner = this;
  SDL_PauseAudio(0);
  return true;
  #endif
}

void VideoEngine::CloseAudioDevice()
{
  #ifndef OPTION_RECALBOX_NO_AUDIO
  Mutex::AutoLock locker(mStaticSyncer);
  if (mAudioDeviceOwner != this) return;

  // Pause first: SDL_CloseAudio() then only has to join a callback that is already winding down.
  // Neither call may run under mAudioSyncer, they wait for the callback that takes it.
  SDL_PauseAudio(1);
  SDL_CloseAudio();
  mAudioDeviceOwner = nullptr;
  #endif
}

int VideoEngine::NanoSleep(long long nanoseconds)
{
  static timespec remaining;
  if (nanoseconds < 1000000000LL)
  {
    if (nanoseconds < 0) return 0;
    timespec delay = { 0, (int)nanoseconds };
    nanosleep(&delay, &remaining);
  }
  else
  {
    timespec delay = { (int)(nanoseconds / 1000000000LL), (int)(nanoseconds % 1000000000LL) };
    nanosleep(&delay, &remaining);
  }
  // Return fractional or remaining time
  return (int)remaining.tv_nsec;
}

char* VideoEngine::FourCCToString(unsigned int fourcc)
{
  static char FCC[5] = { 0, 0, 0, 0, 0 };
  FCC[0] = (char)fourcc;
  FCC[1] = (char)(fourcc >> 8U);
  FCC[2] = (char)(fourcc >> 16U);
  FCC[3] = (char)(fourcc >> 24U);
  FCC[4] = 0;
  return FCC;
}

void VideoEngine::AudioPacketQueue::Enqueue(const AVPacket* packet)
{
  AVPacket pkt;
  if (av_packet_ref(&pkt, packet) != 0) return;
  AVPacketList* elt = (AVPacketList*)av_malloc(sizeof(AVPacketList));
  if (elt == nullptr) return;
  elt->pkt = pkt;
  elt->next = nullptr;

  mMutex.Lock();

  if (Last == nullptr) First = elt;
  else Last->next = elt;
  Last = elt;
  Count++;
  Size += elt->pkt.size;

  mMutex.UnLock();
}

bool VideoEngine::AudioPacketQueue::Dequeue(AVPacket& pkt)
{
  bool result = false;

  mMutex.Lock();
  AVPacketList* elt = First;
  if (elt != nullptr)
  {
    First = elt->next;
    if (First == nullptr) Last = nullptr;
    Count--;
    Size -= elt->pkt.size;
    pkt = elt->pkt;
    av_free(elt);
    result = true;
  }
  mMutex.UnLock();

  return result;
}

VideoEngine::VideoEngine()
  : mVideoChanged(false)
  , mVideoIsPlaying(false)
{
  StartEngine();
}

void VideoEngine::Run()
{
  // Drop the file that just failed, so that the main loop below does not retry it forever. A play
  // order received while we were failing wins and is left untouched: it is played on the next turn.
  auto discardFaultyVideo = [this](const Path& faulty)
  {
    bool discarded = false;
    mGuardian.Lock();
    if (!mVideoChanged && (mFileName == faulty)) { mFileName = Path::Empty; discarded = true; }
    mGuardian.UnLock();
    // Taken outside mGuardian: mStaticSyncer is never locked while holding the path guardian
    if (discarded) ReleaseSoundToken();
  };

  while(IsRunning())
  {
    Path currentVideo;
    // An error - whatever its kind - only ends the current playback, never the thread: it is logged,
    // the faulty file is dropped and the engine goes back to waiting for the next order.
    try
    {
      mGuardian.Lock();
      currentVideo = mFileName;
      mVideoChanged = false;
      mGuardian.UnLock();

      // Wait video
      if (currentVideo.IsEmpty())
      {
        // Nothing left to play: hand the texture over to the main loop, which is the only thread
        // allowed to destroy it.
        Sleep();
        mSignal.WaitSignal();
        continue;
      }

      // Enter play loop
      if (InitializeDecoder())
      {
        // Play until next message
        { LOG(LogDebug) << "[Video Engine] is playing " << currentVideo.ToString(); }
        // Frames are about to flow again: the texture is in use and must not be reclaimed
        WakeUp();
        mVideoIsPlaying = true;
        bool decoded = DecodeFrames(HasSoundToken());
        mVideoIsPlaying = false;
        FinalizeDecoder();
        // A decoding failure drops the file as well: the stream is broken, so replaying it - which
        // is what a plain loop turn would do, since the file itself opens perfectly fine - would
        // only spin on the very same error.
        if (!decoded)
        {
          { LOG(LogError) << "[Video Engine] decoding error on " << currentVideo.ToString() << ". Waiting for next order."; }
          discardFaultyVideo(currentVideo);
        }
      }
      else
      {
        { LOG(LogError) << "[Video Engine] got an error playing " << currentVideo.ToString() << ". Waiting for next order."; }
        // Finalize first: it releases every FFmpeg context a partially successful
        // InitializeDecoder() had already allocated.
        FinalizeDecoder();
        discardFaultyVideo(currentVideo);
      }
    }
    catch(const std::exception& e)
    {
      { LOG(LogError) << "[Video Engine] exception caught while playing " << currentVideo.ToString() << ": " << e.what() << ". Waiting for next order."; }
      mVideoIsPlaying = false;
      FinalizeDecoder();
      discardFaultyVideo(currentVideo);
    }
  }
}

void VideoEngine::PlayVideo(const Path& videopath, bool decodeAudio)
{
  #ifdef OPTION_RECALBOX_NO_AUDIO
  decodeAudio = false;
  #endif
  { LOG(LogDebug) << "[Video Engine] Request to play " << videopath.ToString(); }

  // The last engine asked to decode audio takes the sound, any previous holder goes silent.
  // Kept outside the mGuardian section so mGuardian is never held while taking mStaticSyncer.
  if (decodeAudio) AcquireSoundToken();
  else ReleaseSoundToken();

  mGuardian.Lock();
  mFileName = videopath;
  mVideoChanged = true;
  mGuardian.UnLock();

  mSignal.Fire();
}

void VideoEngine::StopVideo()
{
  { LOG(LogDebug) << "[Video Engine] Request to stop playing " << mFileName.ToString(); }

  // This engine no longer claims the sound: let the next requester take it
  ReleaseSoundToken();

  mGuardian.Lock();
  mFileName = Path::Empty;
  mVideoChanged = true;
  mGuardian.UnLock();

  mSignal.Fire();
  mUnfreezeSignal.Fire();

  // The texture is deliberately NOT released here. StopVideo() runs on whichever thread asked for
  // the stop, and destroying a texture outside the GL context owner would issue glDeleteTextures
  // without a current context and corrupt the renderer state cache on the way. Releasing it is
  // left to the main loop, through ReduceSleepingEngineMemoryFootprint(): the decoder thread puts
  // the engine to sleep as soon as it notices the stop, which makes the texture reclaimable.
}

bool VideoEngine::InitializeDecoder()
{
  // Initialize FFMpeg engine
  static bool FFMpegInitialized = false;
  if (!FFMpegInitialized)
  {
    avdevice_register_all();
    avformat_network_init();

    FFMpegInitialized = true;
    { LOG(LogInfo) << "[Video Engine] FFMpeg global context initialized."; }
  }
  // Audio buffers are reset by OpenAudioDevice(), which does it with the device closed and under
  // mAudioSyncer. Wiping them here would trample a callback still running on the previous stream.

  // Open the file
  if (avformat_open_input(&mContext.AudioVideoContext, mFileName.ToChars(), nullptr, nullptr) != 0)
    RETURN_ERROR("Error opening video " << mFileName.ToString(), false);

  // Lookup stream
  if (avformat_find_stream_info(mContext.AudioVideoContext, nullptr) != 0)
    RETURN_ERROR("Error finding streams in " << mFileName.ToString(), false);

  // Lookup audio and vdeo stream indexes
  mContext.AudioStreamIndex = mContext.VideoStreamIndex = -1;
  for (int s = 0; s < (int)mContext.AudioVideoContext->nb_streams; ++s)
  {
    //av_dump_format(mContext.AudioVideoContext, s, mFileName.c_str(), 0);
    switch(mContext.AudioVideoContext->streams[s]->codecpar->codec_type)
    {
      case AVMEDIA_TYPE_AUDIO: if (mContext.AudioStreamIndex < 0) mContext.AudioStreamIndex = s; break;
      case AVMEDIA_TYPE_VIDEO: if (mContext.VideoStreamIndex < 0) mContext.VideoStreamIndex = s; break;
      case AVMEDIA_TYPE_UNKNOWN:
      case AVMEDIA_TYPE_DATA:
      case AVMEDIA_TYPE_ATTACHMENT:
      case AVMEDIA_TYPE_SUBTITLE:
      case AVMEDIA_TYPE_NB:
      default: break;
    }
  }
  if (mContext.VideoStreamIndex < 0)
    RETURN_ERROR("Error finding video stream in " << mFileName.ToString(), false);

  // Initialize audio codec
  if (mContext.AudioStreamIndex >= 0)
  {
    mContext.AudioCodec = (AVCodec*)avcodec_find_decoder(mContext.AudioVideoContext->streams[mContext.AudioStreamIndex]->codecpar->codec_id);
    if (mContext.AudioCodec == nullptr) RETURN_ERROR("Error finding audio codec " << FourCCToString(mContext.AudioVideoContext->streams[mContext.AudioStreamIndex]->codecpar->codec_tag), false);
    mContext.AudioCodecContext = avcodec_alloc_context3(mContext.AudioCodec);
    if (mContext.AudioCodecContext == nullptr) RETURN_ERROR("Error allocating audio codec context", false);
    if (avcodec_parameters_to_context(mContext.AudioCodecContext, mContext.AudioVideoContext->streams[mContext.AudioStreamIndex]->codecpar) != 0) RETURN_ERROR("Error setting parameters to audio codec context", false);
    if (avcodec_open2(mContext.AudioCodecContext, mContext.AudioCodec, nullptr) != 0) RETURN_ERROR("Error opening audio codec", false);
    mContext.ResamplerContext = swr_alloc();
    if (mContext.ResamplerContext == nullptr) RETURN_ERROR("Error allocating audio resampler context", false);
    // "in_chlayout"/"out_chlayout" and NOT "in_channel_layout"/"out_channel_layout": the latter are
    // the legacy uint64 mask options. av_opt_set_chlayout() returns 0 on them - success - while
    // leaving the modern layout at zero channel, and swr_init() then happily returns 0 too. The
    // resampler falls back to stereo, so swr_convert() reads in[1]: harmless on a stereo file,
    // a NULL dereference on a mono one.
    if (av_opt_set_chlayout(mContext.ResamplerContext, "in_chlayout", &mContext.AudioCodecContext->ch_layout, 0) != 0)
      RETURN_ERROR("Error setting audio resampler input channel layout", false);
    if (av_opt_set_chlayout(mContext.ResamplerContext, "out_chlayout", &mContext.AudioCodecContext->ch_layout, 0) != 0)
      RETURN_ERROR("Error setting audio resampler output channel layout", false);
    av_opt_set_int(mContext.ResamplerContext, "in_sample_rate", mContext.AudioCodecContext->sample_rate, 0);
    av_opt_set_int(mContext.ResamplerContext, "out_sample_rate", mContext.AudioCodecContext->sample_rate, 0);
    av_opt_set_sample_fmt(mContext.ResamplerContext, "in_sample_fmt", mContext.AudioCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(mContext.ResamplerContext, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
    if (swr_init(mContext.ResamplerContext) != 0) RETURN_ERROR("Error initializing audio resampler context", false);

    // swr_init() accepts a zero-channel setup, so the channel count is checked here rather than
    // discovered as a crash in swr_convert()
    AVChannelLayout resamplerLayout;
    if (av_opt_get_chlayout(mContext.ResamplerContext, "in_chlayout", 0, &resamplerLayout) != 0)
      RETURN_ERROR("Error reading back audio resampler channel layout", false);
    int resamplerChannels = resamplerLayout.nb_channels;
    av_channel_layout_uninit(&resamplerLayout);
    if (resamplerChannels != mContext.AudioCodecContext->ch_layout.nb_channels)
      RETURN_ERROR("Audio resampler configured for " << resamplerChannels << " channel(s) instead of "
                   << mContext.AudioCodecContext->ch_layout.nb_channels, false);
  }

  // Initialize video codec
  mContext.VideoCodec = (AVCodec*)avcodec_find_decoder(mContext.AudioVideoContext->streams[mContext.VideoStreamIndex]->codecpar->codec_id);
  if (mContext.VideoCodec == nullptr) RETURN_ERROR("Error finding video codec " << mContext.AudioVideoContext->streams[mContext.VideoStreamIndex]->codecpar->codec_id, false);
  mContext.VideoCodecContext = avcodec_alloc_context3(mContext.VideoCodec);
  if (mContext.VideoCodecContext == nullptr) RETURN_ERROR("Error allocating video codec context", false);
  if (avcodec_parameters_to_context(mContext.VideoCodecContext, mContext.AudioVideoContext->streams[mContext.VideoStreamIndex]->codecpar) != 0) RETURN_ERROR("Error setting parameters to video codec context", false);
  if (avcodec_open2(mContext.VideoCodecContext, mContext.VideoCodec, nullptr) != 0) RETURN_ERROR("Error opening video codec", false);

  // Dimensions must be known here: they drive the RGB buffer allocations below
  if (mContext.VideoCodecContext->width <= 0 || mContext.VideoCodecContext->height <= 0)
    RETURN_ERROR("Invalid video dimensions (" << mContext.VideoCodecContext->width << 'x' << mContext.VideoCodecContext->height << ") in " << mFileName.ToString(), false);

  // The color space context is built lazily from the first decoded frame: some broken files have no
  // pixel format available until the decoder actually parses a frame header, and feeding swscale
  // with AV_PIX_FMT_NONE kills the whole process through an assertion
  if (mContext.VideoCodecContext->pix_fmt == AV_PIX_FMT_NONE)
    { LOG(LogWarning) << "[Video Engine] Unspecified pixel format in " << mFileName.ToString() << ". Waiting for the first decoded frame."; }

  // Time frame & duration
  AVStream* videoStream = mContext.AudioVideoContext->streams[mContext.VideoStreamIndex];
  mContext.FrameTime = (long long)(((double)videoStream->avg_frame_rate.den * 1000000000.0) / (double)videoStream->avg_frame_rate.num);
  mContext.TotalTime = (int)(((double)videoStream->avg_frame_rate.den * (double)videoStream->nb_frames * 1000.0) / (double)videoStream->avg_frame_rate.num);

  // Initialize video frame
  mContext.Frame = av_frame_alloc();
  mContext.FrameRGB[0] = av_frame_alloc();
  mContext.FrameRGB[1] = av_frame_alloc();
  if ((mContext.Frame == nullptr) || (mContext.FrameRGB[0] == nullptr) || (mContext.FrameRGB[1] == nullptr)) RETURN_ERROR("Error allocating video frames", false);

  mContext.Width = mContext.AudioVideoContext->streams[mContext.VideoStreamIndex]->codecpar->width;
  mContext.Height = mContext.AudioVideoContext->streams[mContext.VideoStreamIndex]->codecpar->height;
  int argbSize = av_image_get_buffer_size(PIXEL_FORMAT, mContext.Width, mContext.Height, 8);
  if (argbSize < 1) RETURN_ERROR("Error getting video frame size", false);
  mContext.FrameBuffer = (unsigned char*)av_malloc(argbSize);
  if (mContext.FrameBuffer == nullptr) RETURN_ERROR("Error allocating frame buffer", false);

  if (av_image_fill_arrays(&mContext.FrameRGB[0]->data[0], &mContext.FrameRGB[0]->linesize[0], mContext.FrameBuffer, PIXEL_FORMAT, mContext.Width, mContext.Height, 1) < 0)
    RETURN_ERROR("Error setting frame buffer", false);
  if (av_image_fill_arrays(&mContext.FrameRGB[1]->data[0], &mContext.FrameRGB[1]->linesize[0], mContext.FrameBuffer, PIXEL_FORMAT, mContext.Width, mContext.Height, 1) < 0)
    RETURN_ERROR("Error setting frame buffer", false);

  // Initialize audio callback. The device is only opened for the engine holding the sound token:
  // opening it unconditionally used to start a callback on every engine playing a video with an
  // audio stream, all of them racing on the shared audio buffers.
  if (mContext.AudioCodecContext != nullptr)
    (void)OpenAudioDevice(*mContext.AudioCodecContext);

  return true;
}

bool VideoEngine::UpdateColorSpaceContext(const AVFrame& frame)
{
  AVPixelFormat sourceFormat = (AVPixelFormat)frame.format;

  // Nothing changed since the previous frame? Keep the current context (valid or not) as it is
  if ((sourceFormat == mContext.SourceFormat) && (frame.width == mContext.SourceWidth) && (frame.height == mContext.SourceHeight))
    return mContext.ColorsSpaceContext != nullptr;

  // Record new frame properties, valid or not, so that broken frames are checked & logged only once
  mContext.SourceWidth = frame.width;
  mContext.SourceHeight = frame.height;
  mContext.SourceFormat = sourceFormat;

  // Validate everything upfront: swscale calls av_assert0() on unknown pixel formats or null
  // dimensions, and av_assert0 is always compiled in - it aborts the whole frontend
  bool valid = (frame.width > 0) && (frame.height > 0) &&
               (sourceFormat != AV_PIX_FMT_NONE) &&
               (av_pix_fmt_desc_get(sourceFormat) != nullptr) &&
               (sws_isSupportedInput(sourceFormat) > 0);
  if (!valid)
  {
    if (mContext.ColorsSpaceContext != nullptr)
    {
      sws_freeContext(mContext.ColorsSpaceContext);
      mContext.ColorsSpaceContext = nullptr;
    }
    { LOG(LogError) << "[Video Engine] Unsupported frame format (" << frame.width << 'x' << frame.height << ' '
                    << (av_get_pix_fmt_name(sourceFormat) != nullptr ? av_get_pix_fmt_name(sourceFormat) : "unknown pixel format")
                    << ") in " << mFileName.ToString() << ". Frames are skipped."; }
    return false;
  }

  // sws_getCachedContext recycles or frees & rebuilds the previous context on its own
  mContext.ColorsSpaceContext = sws_getCachedContext(mContext.ColorsSpaceContext,
                                                     frame.width,
                                                     frame.height,
                                                     sourceFormat,
                                                     mContext.Width,
                                                     mContext.Height,
                                                     PIXEL_FORMAT,
                                                     SWS_BILINEAR,
                                                     nullptr,
                                                     nullptr,
                                                     nullptr);
  if (mContext.ColorsSpaceContext == nullptr)
    RETURN_ERROR("Error initializing video color space context for " << mFileName.ToString(), false);

  return true;
}

int VideoEngine::DecodeAudioFrame(AVCodecContext& audioContext, unsigned char* buffer, int /*size*/)
{
  static AVPacket packet;
  static AVFrame* frame = av_frame_alloc();

  int dataSize = 0;
  if (!mContext.AudioQueue.Dequeue(packet)) return -1;

  int ret = avcodec_send_packet(&audioContext, &packet);
  av_packet_unref(&packet);
  if (ret < 0) return -1;

  while (ret == 0)
  {
    ret = avcodec_receive_frame(&audioContext, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR(EOF)) return -1;

    // if error, skip frame
    if (ret < 0)  break;

    int nbFrame = frame->nb_samples;
    int outSize = av_samples_get_buffer_size(nullptr, audioContext.channels, nbFrame, AV_SAMPLE_FMT_FLT,
                                             1);
    ret = swr_convert(mContext.ResamplerContext, &mConvertedDataPointer, nbFrame,
                      (const uint8_t **) &frame->data[0], nbFrame);

    if (ret < 0) continue;

    memcpy(buffer, mConvertedData, outSize);
    dataSize = outSize;

    // No data yet, get more frames
    if (dataSize <= 0) continue;

    break;
  }

  // We have data, return it and come back for more later
  return dataSize;
}

void VideoEngine::DecodeAudioFrameOnDemand(uint8_t * stream, int len)
{
  // This runs on the SDL audio thread while the decoder thread may be building or tearing down
  // mContext. Without this lock, Dispose() frees AudioCodecContext then ResamplerContext under our
  // feet and swr_convert() faults on a null context.
  Mutex::AutoLock locker(mAudioSyncer);

  // Context gone (stopped, or between two clips): feed silence rather than decode nothing
  if (mContext.AudioCodecContext == nullptr || mContext.ResamplerContext == nullptr)
  {
    memset(stream, 0, len);
    return;
  }

  while (len > 0)
  {
    if (mConsumedAudio >= mAvailableAudio)
    {
      // already sent all data; get more
      int audioSize = DecodeAudioFrame(*mContext.AudioCodecContext, mAudioBuffer, sizeof(mAudioBuffer));
      if (audioSize < 0)
      {
        // error = silence
        mAvailableAudio = SDL_AUDIO_BUFFER_SIZE;
        memset(mAudioBuffer, 0, sizeof(mAudioBuffer));
      }
      else mAvailableAudio = audioSize;
      mConsumedAudio = 0;
    }
    int Remaining = (int)(mAvailableAudio - mConsumedAudio);
    if (Remaining > len) Remaining = len;
    memcpy(stream, (unsigned char *)mAudioBuffer + mConsumedAudio, Remaining);
    len -= Remaining;
    stream += Remaining;
    mConsumedAudio += Remaining;
  }
}

Texture VideoEngine::GetDisplayableFrame()
{
  int frame = ((int)mContext.FrameInUse ^ 1) & 1;
  mContext.FrammeRGBLocker[frame].Lock();
  if (mContext.FrameRGB[frame] != nullptr)
  {
    AquireTexture();
    const unsigned char* rgba = mContext.FrameRGB[frame]->data[0];
    if (rgba != nullptr)
    {
      if (!mTexture.Valid()) mTexture = TextureManager::Instance().Create(Path::Empty, mContext.Width, mContext.Height, TextureHolder::Properties::Free | TextureHolder::Properties::Mutable | TextureHolder::Properties::NoCache);
      (void)mTexture.UploadRGBA(mContext.Width, mContext.Height, rgba);
    }
    ReleaseTexture();
  }
  mContext.FrammeRGBLocker[frame].UnLock();

  return mTexture;
}

bool VideoEngine::DecodeFrames(bool decodeAudio)
{
  //int VideoFrameCount = 0;
  //int AudioFrameCount = 0;
  AVPacket packet;
  HighResolutionTimer timer;
  timer.Initialize(0);
  while(IsRunning() && !mVideoChanged)
  {
    // Freeze when no heartbeat has been received
    if (mHeartbeat.GetMilliSeconds() >= sMaxTimeBetweenHeartbeat)
    {
      Sleep();
      mUnfreezeSignal.WaitSignal();
      WakeUp();
      // A new order came in while frozen: this is a normal end of playback, not a failure
      if (mVideoChanged) return true;
    }

    int error = av_read_frame(mContext.AudioVideoContext, &packet);

    // Loop ?
    if (error == AVERROR_EOF)
    {
      AVStream* stream = mContext.AudioVideoContext->streams[mContext.VideoStreamIndex];
      avio_seek(mContext.AudioVideoContext->pb, 0, SEEK_SET);
      avformat_seek_file(mContext.AudioVideoContext, mContext.VideoStreamIndex, 0, 0, stream->duration, 0);
      continue;
    }

    if (error >= 0)
    {
      if (packet.stream_index == mContext.VideoStreamIndex)
      {
        // EAGAIN is not a failure: it only means the codec must be drained before it accepts more
        // input, which the receive loop below does. That packet is simply dropped.
        int sent = avcodec_send_packet(mContext.VideoCodecContext, &packet);
        if ((sent != 0) && (sent != AVERROR(EAGAIN)))
        {
          // Unref before leaving: av_read_frame() handed us a reference nobody else will release
          av_packet_unref(&packet);
          RETURN_ERROR("Error sending video packet to codec for " << mFileName.ToString(), false);
        }
        while (avcodec_receive_frame(mContext.VideoCodecContext, mContext.Frame) == 0)
        {
          // Timing
          long long TimeFromPreviousFrame = timer.GetNanoSeconds();
          long long StillToWait = mContext.FrameTime - TimeFromPreviousFrame;
          timer.Initialize(NanoSleep(StillToWait));

          // Make sure the color space context matches the decoded frame, or skip the frame
          if (!UpdateColorSpaceContext(*mContext.Frame)) continue;

          // Get RGBA video frame
          mContext.FrammeRGBLocker[mContext.FrameInUse & 1U].Lock();
          sws_scale(mContext.ColorsSpaceContext,
                    mContext.Frame->data,
                    mContext.Frame->linesize,
                    0,
                    mContext.Frame->height,
                    mContext.FrameRGB[mContext.FrameInUse & 1U]->data,
                    mContext.FrameRGB[mContext.FrameInUse & 1U]->linesize);
          mContext.FrammeRGBLocker[mContext.FrameInUse & 1U].UnLock();

          // Swap frame
          mContext.FrameInUse ^= 1U;

          //++VideoFrameCount;
        }
      }
      else if (decodeAudio && packet.stream_index == mContext.AudioStreamIndex)
      {
        mContext.AudioQueue.Enqueue(&packet);
        //++AudioFrameCount;
      }
      av_packet_unref(&packet);
    }
  }
  return true;
}

void VideoEngine::FinalizeDecoder()
{
  // Silence the device before anything else, and do it outside mAudioSyncer: once this returns no
  // further callback can observe the context we are about to destroy.
  CloseAudioDevice();

  if (mContext.IsInitialied())
  {
    // A callback of another engine may still be running on the shared buffers
    Mutex::AutoLock locker(mAudioSyncer);
    mContext.Dispose();
  }
}

#pragma GCC diagnostic pop
