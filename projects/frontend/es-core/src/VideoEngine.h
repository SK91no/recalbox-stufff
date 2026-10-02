//
// Created by bkg2k on 14/05/19.
//
#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <utils/datetime/HighResolutionTimer.h>
#include <utils/os/system/Thread.h>
#include <utils/os/system/Mutex.h>
#include <utils/os/system/Signal.h>

#include "rendering/textures/Texture.h"

extern "C"
{
#include <libavfilter/avfilter.h>        // libavfilter-dev
#include <libavformat/avformat.h>        // libavformat-dev
#include <libavcodec/avcodec.h>          // libavformat-dev
#include <libavdevice/avdevice.h>        // libavdevice-dev
#include <libavutil/imgutils.h>          // libavutil-dev
#include <libavutil/pixdesc.h>           // libavutil-dev
#include <libswresample/swresample.h>    // libavformat-dev
#include <libswscale/swscale.h>          // libavfilter-dev
}

class VideoEngine : private Thread
{
  public:
    static constexpr int sMaxTimeBetweenHeartbeat =
    #ifdef DEBUG
      120;
    #else
      30;
    #endif

    /*!
     * @brief Default constructor
     */
    VideoEngine();

    /*!
     * @brief Destructor
     */
    ~VideoEngine() override
    {
      // Order matters. The decoder thread is the only writer of the sleeping list, so it has to be
      // joined before we leave that list, otherwise it could register a soon-to-be-dangling pointer
      // and ReduceSleepingEngineMemoryFootprint() would walk it. ~Thread() joins too, but only
      // after this body has run. The texture is then released by the mTexture member destructor, on
      // this very thread - the context owner, since an engine dies with its VideoComponent.
      StopVideo();
      Stop();
      WakeUp();
      // Last resort: the decoder thread normally closes the device in FinalizeDecoder(), but it
      // exits without finalizing when it was idle. Leaving the device open on a dying engine hands
      // SDL a callback pointing at freed memory.
      CloseAudioDevice();
      ReleaseSoundToken();
    }

    /*!
     * @brief Start playing a video file
     * If a video is already playing, a call to stop is performed playing the new video
     * @param videopath Path to the video file ot play
     */
    void PlayVideo(const Path& videopath, bool decodeAudio = false);

    /*!
     * @brief Stop the currently playing video file.
     * Does nothing if no file is actually playing
     */
    void StopVideo();

    /*!
     * @brief Reset heartbeat timer
     * If the heartbeat timer is not reset within 30ms, the video player stops and wait for a new heatbeet
     */
    void Heartbeat()
    {
      if (mHeartbeat.GetMilliSeconds() >= sMaxTimeBetweenHeartbeat)
        mUnfreezeSignal.Fire();
      mHeartbeat.Initialize(0);
    }

    //! This engine is put to sleep: its texture becomes reclaimable by the context owner thread.
    //! Called by the decoder thread only - and by the destructor, once that thread is joined.
    void Sleep() { Mutex::AutoLock locker(mStaticSyncer); mSleepingEngines.AddUnique(this); }

    //! Wake up this engine. Same single-writer rule as Sleep().
    void WakeUp() { Mutex::AutoLock locker(mStaticSyncer); mSleepingEngines.Remove(this); }

    /*!
     * @brief Release the textures of every sleeping engine.
     * MUST be called from the thread owning the GL context (the main loop): this is where video
     * textures are actually destroyed, since no other thread is allowed to do it.
     */
    static void ReduceSleepingEngineMemoryFootprint()
    {
      Mutex::AutoLock locker(mStaticSyncer);
      for (int i = mSleepingEngines.Count(); --i >= 0;)
        mSleepingEngines[i]->InvalidateTexture();
    }

    //! Check if the video is playing
    [[nodiscard]] bool IsPlaying() const { return mVideoIsPlaying; }

    /*!
     * @brief Return true if the engine is in idle state
     * @return True if in idle state, false otherwise
     */
    [[nodiscard]] int GetVideoDurationMs() const { return mVideoIsPlaying ? mContext.TotalTime : 0; }

    /*!
     * @brief Get current image of the current playing video
     * This must be called fr
     * @return TextureData containing last image
     */
    Texture GetDisplayableFrame();

  private:
    /*!
     * @brief Small packet queue
     */
    class AudioPacketQueue
    {
      private:
        Mutex mMutex;

        //! First paquet
        AVPacketList* First;
        //! Last paquet
        AVPacketList* Last;
        //! Packet count
        int Count;
        //! Packet size
        int Size;

      public:
        /*!
         * @brief Constructor
         */
        AudioPacketQueue()
          : First(nullptr)
          , Last(nullptr)
          , Count(0)
          , Size(0)
        {
        }

        void Reset()
        {
          AVPacket packet;
          while(Dequeue(packet)) {
              av_packet_unref(&packet);
          }
          First = Last = nullptr;
          Count = Size = 0;
        }

        /*!
         * @brief Enqueue a new packet
         * @param packet packet to enqueue
         */
        void Enqueue(const AVPacket* packet);

        /*!
         * @brief Dequeue a packet or return nullptr of the queue is empty.
         * This call is non blocking
         * @param packet packet to dequeue
         * @return True if the packet is valid
         */
        bool Dequeue(AVPacket& packet);
    };

    /*!
     * @brief Internal context, all embedded in a POD object
     */
    class PlayerContext
    {
      public:
        //! Global audio/video context
        AVFormatContext* AudioVideoContext;

        //! Index of the first audio stream available in the file
        int AudioStreamIndex;
        //! Index of the first video stream available in the file
        int VideoStreamIndex;

        //! Audio codec
        AVCodec* AudioCodec;
        //! Video codec
        AVCodec* VideoCodec;
        //! Audio codec context
        AVCodecContext* AudioCodecContext;
        //! Video codec context
        AVCodecContext* VideoCodecContext;

        //! Rescaler context
        SwrContext* ResamplerContext;
        //! Color Space context
        SwsContext* ColorsSpaceContext;

        //! Audio packets
        AudioPacketQueue AudioQueue;

        //! Frame buffer
        unsigned char* FrameBuffer;

        //! Source video Frame in native pixel format
        AVFrame* Frame;
        //! Video Frame transcode to RGB
        AVFrame* FrameRGB[2];
        //! RGB frame access locker
        Mutex FrammeRGBLocker[2];
        //! Buffer in use by the decoder (0/1)
        unsigned int FrameInUse;

        //! Video width in pixel
        int Width;
        //!Video height in pixel
        int Height;

        //! Source frame width the color space context has been built for
        int SourceWidth;
        //! Source frame height the color space context has been built for
        int SourceHeight;
        //! Source frame pixel format the color space context has been built for
        AVPixelFormat SourceFormat;

        //! Frame time in Nano-seconds
        long long FrameTime;
        //! Video duration in milliseconds
        int TotalTime;

        PlayerContext()
          : AudioVideoContext(nullptr),
            AudioStreamIndex(-1),
            VideoStreamIndex(-1),
            AudioCodec(nullptr),
            VideoCodec(nullptr),
            AudioCodecContext(nullptr),
            VideoCodecContext(nullptr),
            ResamplerContext(nullptr),
            ColorsSpaceContext(nullptr),
            FrameBuffer(nullptr),
            Frame(nullptr),
            FrameRGB(),
            FrameInUse(0),
            Width(0),
            Height(0),
            SourceWidth(0),
            SourceHeight(0),
            SourceFormat(AV_PIX_FMT_NONE),
            FrameTime(0),
            TotalTime(0)
        {
        }

        void Dispose()
        {
          // Protect both access to RGB data
          FrammeRGBLocker[0].Lock();
          FrammeRGBLocker[1].Lock();

          if (AudioVideoContext != nullptr ) avformat_close_input(&AudioVideoContext);
          if (AudioCodecContext != nullptr ) avcodec_free_context(&AudioCodecContext);
          if (VideoCodecContext != nullptr ) avcodec_free_context(&VideoCodecContext);
          if (ResamplerContext != nullptr  ) swr_free(&ResamplerContext);
          if (ColorsSpaceContext != nullptr) sws_freeContext(ColorsSpaceContext);
          if (Frame != nullptr             ) av_frame_free(&Frame);
          if (FrameRGB[0] != nullptr       ) av_frame_free(&FrameRGB[0]);
          if (FrameRGB[1] != nullptr       ) av_frame_free(&FrameRGB[1]);
          if (FrameBuffer != nullptr       ) av_free(FrameBuffer);

          AudioVideoContext = nullptr;
          AudioCodec = VideoCodec = nullptr;
          AudioCodecContext = VideoCodecContext = nullptr;
          ResamplerContext = nullptr;
          ColorsSpaceContext = nullptr;
          AudioStreamIndex = VideoStreamIndex = -1;
          FrameBuffer = nullptr;
          Frame = FrameRGB[0] = FrameRGB[1] = nullptr;
          Width = Height = 0;
          SourceWidth = SourceHeight = 0;
          SourceFormat = AV_PIX_FMT_NONE;
          FrameInUse = 0;
          FrameTime = 0;

          AudioQueue.Reset();

          // Unlock RGB data
          FrammeRGBLocker[0].UnLock();
          FrammeRGBLocker[1].UnLock();
        }

        [[nodiscard]] bool IsInitialied() const { return AudioVideoContext != nullptr; }
    };

    static constexpr int SDL_AUDIO_BUFFER_SIZE = 4096;

    //! Signal used to unlock the thread and actually run the video decoding
    Signal mSignal;
    //! Signal used to instruct other thread a video has been stopped
    Signal mUnfreezeSignal;

    //! Path protector
    Mutex mGuardian;
    //! Video filename
    Path mFileName;
    //! Video changed flag
    volatile bool mVideoChanged;
    //! Video is playing
    volatile bool mVideoIsPlaying;

    //! Global context
    PlayerContext mContext;
    //! Textures to render
    Texture mTexture;
    //! Texture protector
    Mutex mTextureSyncer;

    //! Last heartbeat timer
    HighResolutionTimer mHeartbeat;
    //! Static items protector
    static Mutex mStaticSyncer;
    //! Sleeping engine
    static Array<VideoEngine*> mSleepingEngines;
    //! Sound token (which engine can play the sound). Last engine asked to decode audio wins.
    static VideoEngine* mSoundToken;
    //! Engine currently owning the SDL audio device, nullptr when the device is closed
    static VideoEngine* mAudioDeviceOwner;

    /*!
     * @brief Serializes the SDL audio callback against the decoder thread.
     * The callback reads mContext and the shared audio buffers below while the decoder thread
     * builds and destroys them. NEVER hold this while calling SDL_PauseAudio/SDL_CloseAudio:
     * those wait for the running callback, which would be waiting for this very mutex.
     */
    static Mutex mAudioSyncer;

    //! Audio buffer - shared: the sound token guarantees a single audio decoder at a time
    static unsigned char mConvertedData[(192000 * 3) / 2];
    //! Audio buffer pointer
    static unsigned char* mConvertedDataPointer;

    //! PCM waiting to be handed to SDL - shared, same reason as mConvertedData
    static unsigned char mAudioBuffer[(192000 * 3) / 2];
    //! Bytes available in mAudioBuffer
    static unsigned int mAvailableAudio;
    //! Bytes of mAudioBuffer already handed to SDL
    static unsigned int mConsumedAudio;

    /*!
     * Thread main loop
     */
    void Run() override;

    /*!
     * @brief Unlock the thread on exit
     */
    void Break() override
    {
      // Both signals: the decoder blocks on mSignal when idle, but on mUnfreezeSignal when frozen,
      // and Stop() joins the thread - firing only one of them would hang shutdown in that state.
      mSignal.Fire();
      mUnfreezeSignal.Fire();
    }

    static void AudioCallback(void *userdata, unsigned char* stream, int len)
    {
      ((VideoEngine*)userdata)->DecodeAudioFrameOnDemand(stream, len);
    }

    bool InitializeDecoder();

    //! Give this engine the exclusive right to play sound. The last requester always wins.
    void AcquireSoundToken() { Mutex::AutoLock locker(mStaticSyncer); mSoundToken = this; }

    //! Drop the sound token, but only if this engine still holds it
    void ReleaseSoundToken() { Mutex::AutoLock locker(mStaticSyncer); if (mSoundToken == this) mSoundToken = nullptr; }

    //! Check whether this engine is the one allowed to play sound
    [[nodiscard]] bool HasSoundToken() { Mutex::AutoLock locker(mStaticSyncer); return mSoundToken == this; }

    /*!
     * @brief Open the SDL audio device for this engine, taking it over from any previous owner.
     * SDL_OpenAudio drives the one legacy device, so the previous owner is closed down first.
     * @param audioContext Audio codec context giving the channel count and sample rate
     * @return True if the device is open and running for this engine
     */
    bool OpenAudioDevice(const AVCodecContext& audioContext);

    //! Close the SDL audio device, but only if this engine owns it
    void CloseAudioDevice();

    void DecodeAudioFrameOnDemand(unsigned char * stream, int len);

    int DecodeAudioFrame(AVCodecContext& audioContext, unsigned char* buffer, int size);

    /*!
     * @brief Create or update the color space context so that it matches the given decoded frame.
     * Frame properties are validated first: swscale raises fatal assertions - aborting the whole
     * process - when it is fed an unknown pixel format or null dimensions
     * @param frame Last decoded frame
     * @return True if the color space context is ready to convert the given frame, false if the frame must be skipped
     */
    bool UpdateColorSpaceContext(const AVFrame& frame);

    /*!
     * @brief Decode the current video until a new order is received or the thread is stopped
     * @param decodeAudio True to also decode the audio stream
     * @return True on normal exit (new order, stop), false if the stream is broken and must be
     * dropped instead of being replayed
     */
    bool DecodeFrames(bool decodeAudio = false);

    void FinalizeDecoder();

    /*!
     * @brief Start the video engine. After calling this method, the player is ready to play video
     */
    void StartEngine() { static int engine = 0; Thread::Start(String("VideoEngine").Append(++engine)); }

    //! Empty audio buffers to avoid start noise
    static void ClearAudioBuffers();

    /*!
     * @brief Sleep the requested nanoseconds
     * @param nanoseconds Nanoseconds to sleep
     * @return Nanosecond remaining if the sleep exited before the given time
     */
    static int NanoSleep(long long nanoseconds);

    //! Convert fourCC code to char* string
    static char* FourCCToString(unsigned int fourcc);

    //! Lock texture for thread safety
    void AquireTexture() { mTextureSyncer.Lock(); }

    //! Release texture
    void ReleaseTexture() { mTextureSyncer.UnLock(); }

    //! Release the texture. Destroys a GPU object: context owner thread only.
    void InvalidateTexture() { Mutex::AutoLock locker(mTextureSyncer); mTexture.Clear(); }
};

#pragma GCC diagnostic pop
