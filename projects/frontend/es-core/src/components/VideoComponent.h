#pragma once

#include <components/base/ThemableComponent.h>
#include <utils/String.h>
#include <utils/datetime/HighResolutionTimer.h>
#include "IVideoComponentAction.h"
#include "VideoEngine.h"
#include <Renderer.h>

class VideoComponent : public ThemableComponent
{
  public:
    enum class AllowedEffects
    {
      None         = 0,
      Bump         = (1 << 0),
      Fade         = (1 << 1),
      BreakingNews = (1 << 2),
      All          = Bump | Fade | BreakingNews,
    };

  private:
    enum class State
    {
      InitializeVideo,
      WaitForVideoToStart,
      StartVideo,
      DisplayVideo,
      StopVideo,
    };

    enum class Effect
    {
      None = -1,
      Bump = 0,
      Fade,
      BreakingNews,
      _LastItem,
    };

    //! Action interface
    IVideoComponentAction* mInterface;

    //! Video path
    Path mVideoPath;

    //! Video state
    State mState;

    //! Video effect
    Effect mEffect;
    //! Allowed video effects
    AllowedEffects mAllowedEffects;

    Vector2f mTargetSize;

    // Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
    // Used internally whenever the resizing parameters or texture change.
    void resize();

    unsigned int mColorShift;

    unsigned char mFadeOpacity;

    // Video engine
    VideoEngine mVideoEngine;

    //! Time to vidéo
    int mVideoDelay;
    //! Effect timing
    int mVideoEffect;
    //! Video loop (0 for infinite loop)
    int mVideoLoop;

    //!< Reflexion top alpha
    float mTopAlpha;
    //!< Reflexion top alpha
    float mBottomAlpha;

    //! Decode audio ?
    bool mDecodeAudio;
    //! Keep ratio ?
    bool mKeepRatio;

    //! High resolution timer for time computations
    HighResolutionTimer mTimer;

    /*!
     * @brief Decide what to display
     * @param effect [out] Set to the video bump value
     * @return bool if the video frame has to be displayed
     */
    bool ProcessDisplay(double& effect);

    /*!
     * @brief Stop video and reset component context
     */
    void ResetAnimations();

    /*!
     * @brief Process effect and return a double from 0.0 to 1.0
     * @param elapsedms elapsed time in millisecond
     * @param in True if the effect is used in starting video phase. False in stopping video phase
     * @return 0.0 to 1.0 value.
     */
    double ProcessEffect(int elapsedms, bool in);

    //! Pi constant
    static constexpr double Pi = 3.1415926535;

  public:

    //! Constructor
    explicit VideoComponent(WindowManager& window, IVideoComponentAction* actionInterface, AllowedEffects allowedEffects = AllowedEffects::All);

    //! Destructor
    ~VideoComponent() override = default;

    //! Called on path change
    void OnPathChanged();

    /*!
     * @brief Set video
     * @param path Path to video file
     * @param delay Delay before actually playing the video in milliseconds
     * @param loops Number of loops (0 = infinite loops)
     */
    void setVideo(const Path& path, int delay, int loops, bool decodeAudio);

    /*!
     * @brief Set video
     * @param path Path to video file
     */
    void setVideo(const Path& path, bool decodeAudio);

    void setOpacity(unsigned char opacity) override;

    // Resize the image to fit this size. If one axis is zero, scale that axis to maintain aspect ratio.
    // If both are non-zero, potentially break the aspect ratio.  If both are zero, no resizing.
    // Can be set before or after an image is loaded.
    // setMaxSize() and setResize() are mutually exclusive.
    void setResize(float width, float height);
    inline void setResize(const Vector2f& size) { setResize(size.x(), size.y()); }

    //! Set keep ratio
    void setKeepRatio(bool keepRatio);

    // Multiply all pixels in the image by this color when rendering.
    void setColorShift(unsigned int color);
    void setColor(unsigned int color) override { setColorShift(color); }

    void Update(int elapsed) override;
    void Render(const Transform4x4f& parentTrans) override;

    /*
     * Themable implementation
     */

    /*!
     * @brief Called when a theme element is applyed on the current component.
     * @param element Element instance
     * @param properties Properties to update
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Video; }

    bool CollectHelpItems(Help& help) override;

    [[nodiscard]] bool isDiplayed() const { return mState == State::DisplayVideo; }

    [[nodiscard]] int GetVideoDurationMs() const { return mVideoEngine.GetVideoDurationMs(); }

    static constexpr int DEFAULT_VIDEODELAY = 2000;
    static constexpr int DEFAULT_VIDEOEFFET = 500;
    static constexpr int DEFAULT_VIDEOLOOP  = 1;
    static constexpr bool DEFAULT_VIDEODECODEAUDIO  = false;

    /*!
     * @brief Set top and bottom alpha for reflection
     * if both value are zero'ed, no reflection is applied
     * @param top Top alpha
     * @param bottom Bottom alpha
     */
    void SetReflection(float top, float bottom)
    {
      top = Math::clamp(top, 0.f, 1.f);
      bottom = Math::clamp(bottom, 0.f, 1.f);
      mTopAlpha = top;
      mBottomAlpha = bottom;
    }

    /*!
     * @brief Set top and bottom alpha for reflection
     * if both value are zero'ed, no reflection is applied
     * @param v Vector containing top alpha in x and bottom alpha in y
     */
    void SetReflection(const Vector2f& v) { SetReflection(v.x(), v.y()); }
};

DEFINE_BITFLAG_ENUM(VideoComponent::AllowedEffects, int)