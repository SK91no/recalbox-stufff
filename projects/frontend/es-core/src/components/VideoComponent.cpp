#include <components/VideoComponent.h>
#include <audio/AudioScheduler.h>
#include <VideoEngine.h>
#include <themes/ThemeData.h>
#include <utils/locale/LocaleHelper.h>
#include <Renderer.h>

VideoComponent::VideoComponent(WindowManager& window, IVideoComponentAction* actionInterface, AllowedEffects allowedEffectsd)
  : ThemableComponent(window)
  , mInterface(actionInterface)
  , mState(State::InitializeVideo)
  , mEffect(Effect::BreakingNews)
  , mAllowedEffects(allowedEffectsd)
  , mTargetSize(0)
  , mColorShift(0xFFFFFFFF)
  , mFadeOpacity(0)
  , mVideoDelay(DEFAULT_VIDEODELAY)
  , mVideoEffect(DEFAULT_VIDEOEFFET)
  , mVideoLoop(DEFAULT_VIDEOLOOP)
  , mTopAlpha(0)
  , mBottomAlpha(0)
  , mDecodeAudio(DEFAULT_VIDEODECODEAUDIO)
  , mKeepRatio(false)
{
}

void VideoComponent::resize()
{
  Texture texture = mVideoEngine.GetDisplayableFrame();
  if (!texture.Valid()) return;
  const Vector2f textureSize((float)texture.Width(), (float)texture.Height());

  if (mKeepRatio)
  {
    mSize = textureSize;

    Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

    if (resizeScale.x() < resizeScale.y())
    {
      mSize[0] *= resizeScale.x();
      mSize[1] *= resizeScale.x();
    }
    else
    {
      mSize[0] *= resizeScale.y();
      mSize[1] *= resizeScale.y();
    }

    // for SVG rasterization, always calculate width from rounded height (see comment above)
    mSize[1] = Math::round(mSize[1]);
    mSize[0] = (mSize[1] / textureSize.y()) * textureSize.x();
  }
  else
  {
    // if both components are set, we just stretch
    // if no components are set, we don't resize at all
    mSize = mTargetSize.isZero() ? textureSize : mTargetSize;

    // if only one component is set, we resize in a way that maintains aspect ratio
    // for SVG rasterization, we always calculate width from rounded height (see comment above)
    if ((mTargetSize.x() == 0) && (mTargetSize.y() != 0))
    {
      mSize[1] = Math::round(mTargetSize.y());
      mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
    }
    else if ((mTargetSize.x() != 0) && (mTargetSize.y() == 0))
    {
      mSize[1] = Math::round((mTargetSize.x() / textureSize.x()) * textureSize.y());
      mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
    }
  }
}

void VideoComponent::OnPathChanged()
{
  AudioScheduler::ResumeMusicIfNecessary();
  ResetAnimations();
}

void VideoComponent::setVideo(const Path& path, int delay, int loops, bool decodeAudio)
{
  if (path != mVideoPath || mVideoDelay != delay || mVideoLoop != loops || mDecodeAudio != decodeAudio)
  {
    AudioScheduler::ResumeMusicIfNecessary();
    mVideoEngine.StopVideo();
    mVideoPath = path;
    mVideoDelay = delay;
    mVideoLoop = loops;
    mDecodeAudio = decodeAudio;
    OnPathChanged();
  }
}

void VideoComponent::setVideo(const Path& path, bool decodeAudio)
{
  if (path != mVideoPath || mDecodeAudio != decodeAudio)
  {
    AudioScheduler::ResumeMusicIfNecessary();
    mVideoEngine.StopVideo();
    mVideoPath = path;
    mDecodeAudio = decodeAudio;
    OnPathChanged();
  }
}

void VideoComponent::setResize(float width, float height)
{
  if (width != mTargetSize.x() || height != mTargetSize.y())
  {
    mTargetSize.Set(width, height);
    resize();
  }
}

void VideoComponent::setKeepRatio(bool keepRatio)
{
  if (mKeepRatio != keepRatio)
  {
    mKeepRatio = keepRatio;
    resize();
  }
}

void VideoComponent::setColorShift(unsigned int color)
{
  mColorShift = color;
  // Grab the opacity from the color shift because we may need to apply it if
  // fading textures in
  mOpacity = color & 0xffU;
}

void VideoComponent::setOpacity(unsigned char opacity)
{
  if (opacity != mOpacity)
  {
    mOpacity = opacity;
    mColorShift = ((mColorShift >> 8U) << 8U) | mOpacity;
  }
}

void VideoComponent::ResetAnimations()
{
  { LOG(LogDebug) << "[VideoComponent] Animations reseted!"; }

  mTimer.Initialize(0);
  mState = State::InitializeVideo;
  //{ LOG(LogDebug) << "[VideoComponent] Timer reseted: State::Uninitialized " + DateTime().ToPreciseTimeStamp(); }

  // Stop the video
  mVideoEngine.StopVideo();
}

double VideoComponent::ProcessEffect(int elapsedms, bool in)
{
  double linear = (double) (in ? elapsedms : (mVideoEffect - elapsedms)) / mVideoEffect;

  switch(mEffect)
  {
    case Effect::Bump:
    {
      double sinus = sin((Pi / 2.0) * linear) + sin(Pi * linear) / 2.0;
      return sinus;
    }
    case Effect::Fade:
    case Effect::BreakingNews:
    {
      linear = Math::clampf(linear, 0.0, 1.0);
      return linear;
    }
    case Effect::None:
    case Effect::_LastItem: break;
  }

  return 1.0;
}

bool VideoComponent::ProcessDisplay(double& effect)
{
  bool video = false;
  effect = 0.0;

  if (mState == State::InitializeVideo)
    if (int elapsed = mTimer.GetMilliSeconds(); elapsed >= mVideoDelay && !mVideoPath.IsEmpty())
    {
      mEffect = (Effect)(((int)mEffect + 1) % (int)Effect::_LastItem);
      if ((mAllowedEffects & AllowedEffects::All) != 0)
        while((mAllowedEffects & (1 << (int)mEffect)) == 0)
          mEffect = (Effect)(((int)mEffect + 1) % (int)Effect::_LastItem);
      else
        mEffect = Effect::None;
      // Start video if it's not started yet
      mVideoEngine.PlayVideo(mVideoPath, mDecodeAudio);
      mState = State::WaitForVideoToStart;
      mTimer.Initialize(0);
      //{  LOG(LogDebug) << "[VideoComponent] Timer reseted: State::InitializeVideo " << DateTime().ToPreciseTimeStamp() << " elapsed: "  << elapsed; }
    }

  if (mState == State::WaitForVideoToStart)
    if (mVideoEngine.IsPlaying())
    {
      resize();
      mState = State::StartVideo;
      mTimer.Initialize(0);
      //{ LOG(LogDebug) << "[VideoComponent] Timer reseted: State::BumpVideo " + DateTime().ToPreciseTimeStamp() << " elapsed: " << elapsed; }
      if (mDecodeAudio) AudioScheduler::PauseMusicIfNecessary();
      if (mInterface != nullptr) mInterface->VideoComponentRequireAction(this, IVideoComponentAction::Action::FadeOut);
    }

  if (mState == State::StartVideo)
  {
    video = true;
    int elapsed = mTimer.GetMilliSeconds();
    effect = ProcessEffect(elapsed, true);
    if (elapsed >= mVideoEffect || mEffect == Effect::None)
    {
      mState = State::DisplayVideo;
      mTimer.Initialize(0);
      //{ LOG(LogDebug) << "[VideoComponent] Timer reseted: State::DisplayVideo " + DateTime().ToPreciseTimeStamp() << " elapsed: " << elapsed; }
    }
  }

  if (mState == State::DisplayVideo)
  {
    // Video only
    video = true;
    effect = 1.0;
    int elapsed = mTimer.GetMilliSeconds();
    if ((mVideoLoop > 0) && (elapsed >= mVideoEngine.GetVideoDurationMs() * mVideoLoop - mVideoEffect))
    {
      mState = State::StopVideo;
      mTimer.Initialize(0);
      //{ LOG(LogDebug) << "[VideoComponent] Timer reseted: State::FinalizeVideo " + DateTime().ToPreciseTimeStamp() << " elapsed: " << elapsed; }
      if (mInterface != nullptr) mInterface->VideoComponentRequireAction(this, IVideoComponentAction::Action::FadeIn);
    }
  }

  if (mState == State::StopVideo)
  {
    video = true;
    int elapsed = mTimer.GetMilliSeconds();
    effect = ProcessEffect(elapsed, false);
    if (elapsed >= mVideoEffect || mEffect == Effect::None)
    {
      mState = State::InitializeVideo;
      mVideoEngine.StopVideo();
      AudioScheduler::ResumeMusicIfNecessary();
      mTimer.Initialize(0);
      //{ LOG(LogDebug) << "[VideoComponent] Timer reseted: State::DisplayImage " + DateTime().ToPreciseTimeStamp() << " elapsed: " << elapsed; }
      video = false;
    }
  }

  return video;
}

void VideoComponent::Update(int elapsed)
{
  mVideoEngine.Heartbeat();
  Component::Update(elapsed);
}

void VideoComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  double effect = 0.0;
  bool display = ProcessDisplay(effect);

  if (!display) return;

  Texture videoFrame = mVideoEngine.GetDisplayableFrame();
  if (!videoFrame.Valid()) return;

  // Geometry: the "bump" family scales the frame around its center, the others keep full size.
  // (The video ratio is already baked into mSize by resize(), so we stretch the texture onto the quad.)
  float x = 0.0f;
  float y = 0.0f;
  float w = Math::round(mSize.x());
  float h = Math::round(mSize.y());
  if (mEffect == Effect::Bump || mEffect == Effect::BreakingNews)
  {
    w = (float)(mSize.x() * effect);
    h = (float)(mSize.y() * effect);
    x = Math::round(mSize.x() / 2.0f - w / 2.0f);
    y = Math::round(mSize.y() / 2.0f - h / 2.0f);
  }

  // Color: the "fade" effect modulates the alpha channel, the others stay fully opaque.
  unsigned int alpha = (mEffect == Effect::Fade) ? (unsigned int)(effect * 255.0) : 255u;
  Colors::ColorRGBA color = (mColorShift & 0xFFFFFF00) | (alpha & 0xFFu);

  // Rotation: "BreakingNews" spins the frame around its center. The rotation is carried by the
  // component transform / current matrix, exactly like ImageComponent, so no direct GL is needed.
  setRotationOrigin(0.5f, 0.5f);
  setRotation(mEffect == Effect::BreakingNews ? (float)(Pi * 4.0) * (float)effect : 0.0f);
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Single high level draw: frame + optional projected reflection (mTopAlpha/mBottomAlpha).
  // keepratio stays false: resize() already baked the video ratio into mSize when mKeepRatio is
  // set, and letting the renderer fit the frame again would letterbox the stretched mode.
  // flipY is mandatory: the renderer samples textures bottom-up (FreeImage stores them that way)
  // while swscale hands us top-down RGBA frames.
  videoFrame.Render((int)x, (int)y, (int)w, (int)h,
                    false /*keepratio*/, false /*tiled*/, false /*flipX*/, true /*flipY*/,
                    color, color, color, color,
                    mTopAlpha, mBottomAlpha);
}

void VideoComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Size))
  {
    Vector2f scale = getParent() != nullptr ? getParent()->getSize() : Vector2f(
      Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
    if (element.HasProperty(ThemePropertyName::Size))
    {
      setKeepRatio(false);
      setResize(element.AsVector(ThemePropertyName::Size) * scale);
    }
    else if (element.HasProperty(ThemePropertyName::MaxSize))
    {
      setKeepRatio(true);
      setSize(element.AsVector(ThemePropertyName::MaxSize) * scale);
      setResize(element.AsVector(ThemePropertyName::MaxSize) * scale);
    }
    if (element.HasProperty(ThemePropertyName::KeepRatio))
      setKeepRatio(element.AsBool(ThemePropertyName::KeepRatio));
  }

  if (hasFlag(properties, ThemePropertyCategory::Path))
    setVideo(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty, mDecodeAudio);

  if (hasFlag(properties, ThemePropertyCategory::Color))
    setColorShift(element.HasProperty(ThemePropertyName::Color) ? (unsigned int)element.AsInt(ThemePropertyName::Color) : 0xFFFFFFFF);

  if (hasFlag(properties, ThemePropertyCategory::Effects))
  {
    SetReflection(element.HasProperty(ThemePropertyName::Reflection) ? element.AsVector(ThemePropertyName::Reflection) : Vector2f());
    if (element.HasProperty(ThemePropertyName::Animations))
    {
      mAllowedEffects = AllowedEffects::None;
      for (String& animation: element.AsString(ThemePropertyName::Animations).Split(','))
        if (animation.Trim() == "none") mAllowedEffects = AllowedEffects::None;
        else if (animation.Trim() == "bump") mAllowedEffects |= AllowedEffects::Bump;
        else if (animation.Trim() == "fade") mAllowedEffects |= AllowedEffects::Fade;
        else if (animation.Trim() == "breakingnews") mAllowedEffects |= AllowedEffects::BreakingNews;
    }
    else mAllowedEffects = AllowedEffects::All;
    mVideoLoop = element.HasProperty(ThemePropertyName::Loops) ? element.AsInt(ThemePropertyName::Loops) : DEFAULT_VIDEOLOOP;
    mVideoDelay = element.HasProperty(ThemePropertyName::Delay) ? element.AsInt(ThemePropertyName::Delay) : DEFAULT_VIDEODELAY;
  }
  else
  {
    mAllowedEffects = AllowedEffects::All;
    mVideoLoop = DEFAULT_VIDEOLOOP;
    mVideoDelay = DEFAULT_VIDEODELAY;
  }
  //if (mVideoLoop < 0) mVideoLoop = INT32_MAX;
}

bool VideoComponent::CollectHelpItems(Help& help)
{
  help.Set(Help::Valid(), _("SELECT"));
  return true;
}


