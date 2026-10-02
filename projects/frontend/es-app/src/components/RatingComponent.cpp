#include <utils/locale/LocaleHelper.h>
#include <components/RatingComponent.h>
#include <themes/ThemeData.h>
#include "Renderer.h"
#include "WindowManager.h"
#include <rendering/textures/TextureManager.h>

String RatingComponent::sFilledTexture(":/star_filled.svg");
String RatingComponent::sUnfilledTexture(":/star_unfilled.svg");

RatingComponent::RatingComponent(WindowManager&window, unsigned int color, float value, const String& discriminent)
  : ThemableComponent(window)
  , mDiscriminent(discriminent)
  , mValue(value)
  , mColor(color)
  , mOriginColor(color)
  , mId(0)
  , mInterface(nullptr)
{
	mFilledTexture = TextureManager::Instance().Create(Path(discriminent + '|' + sFilledTexture));
	mUnfilledTexture = TextureManager::Instance().Create(Path(discriminent + '|' + sUnfilledTexture));
	mSize.Set(64 * sRatingStarCount, 64);
}

RatingComponent::RatingComponent(WindowManager&window, float value, const String& discriminent)
  : RatingComponent(window, 0xFFFFFFFF, value, discriminent)
{
}

void RatingComponent::setValue(float value)
{
  mValue = Math::clamp(value, 0.f, 1.f);
}

void RatingComponent::setValue(const String& value)
{
  mValue = 0.0f;
	if(!value.empty())
  {
		if (value.TryAsFloat(mValue))
  		mValue = Math::clamp(mValue, 0.f, 1.f);
	}
}

void RatingComponent::onSizeChanged()
{
	if(mSize.y() == 0)

		mSize[1] = mSize.x() / sRatingStarCount;
	else if(mSize.x() == 0)
		mSize[0] = mSize.y() * sRatingStarCount;

	if(mSize.y() > 0)
	{
		int heightPx = Math::roundi(mSize.y());
		if (mFilledTexture.Valid())
			mFilledTexture.SetTargetSize(heightPx, heightPx, true);
		if(mUnfilledTexture.Valid())
			mUnfilledTexture.SetTargetSize(heightPx, heightPx, true);
	}
}

void RatingComponent::Render(const Transform4x4f& parentTrans)
{
  if(mThemeDisabled) return;

	Transform4x4f trans = (parentTrans * getTransform()).round();
	Renderer::SetMatrix(trans);

  if (mUnfilledTexture.MakeReady())
    mUnfilledTexture.Render(0, 0, Math::roundi(mSize.y() * sRatingStarCount), Math::roundi(mSize.y()), true, (mColor & 0xFFFFFF00) | mOpacity);
  if (mFilledTexture.MakeReady())
    mFilledTexture.Render(0, 0, Math::roundi(mSize.y() * mValue * sRatingStarCount), Math::roundi(mSize.y()), true, (mColor & 0xFFFFFF00) | mOpacity);
}

bool RatingComponent::ProcessInput(const InputCompactEvent& event)
{
	if (event.ValidReleased() || event.AnyRightReleased())
	{
		if (mValue += .5f / sRatingStarCount; mValue > 1.f) mValue = 0.f;
		if (mInterface != nullptr)
		  mInterface->RatingChanged(mId, mValue);
	}
	else if (event.AnyLeftReleased())
  {
    if (mValue -= .5f / sRatingStarCount; mValue < 0.f) mValue = 1.f;
    if (mInterface != nullptr)
      mInterface->RatingChanged(mId, mValue);
  }

	return Component::ProcessInput(event);
}

void RatingComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
	if (hasFlag(properties, ThemePropertyCategory::Path))
  {
    mFilledTexture = TextureManager::Instance().Create(Path(mDiscriminent + '|' + (element.HasProperty(ThemePropertyName::FilledPath) ? element.AsString(ThemePropertyName::FilledPath) : sFilledTexture)));
    mUnfilledTexture = TextureManager::Instance().Create(Path(mDiscriminent + '|' + (element.HasProperty(ThemePropertyName::UnfilledPath) ? element.AsString(ThemePropertyName::UnfilledPath) : sUnfilledTexture)));
    onSizeChanged();
  }
}

bool RatingComponent::CollectHelpItems(Help& help)
{
	help.Set(HelpType::LeftRight, _("-/+ stars"));
	return true;
}

