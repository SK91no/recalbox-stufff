#include "SwitchComponent.h"
#include "Renderer.h"
#include "WindowManager.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>
#include <rendering/fonts/FontManager.h>

SwitchComponent::SwitchComponent(WindowManager&window)
  : Component(window)
  , mImage(window)
  , mInterface(nullptr)
  , mId(0)
  , mState(false)
  , mInitialState(false)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	mImage.setImage(menuTheme.Elements().FromType(mState ? MenuThemeData::IconElement::Type::On : MenuThemeData::IconElement::Type::Off), false);
	mImage.setResize(0, (float)FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false).Height() * (Renderer::Instance().Is240p() ? 1.4f : 1.0f));
	mImage.setColorShift(menuTheme.Text().color);
	mOriginColor = menuTheme.Text().color;
	mSize = mImage.getSize();
}

void SwitchComponent::onSizeChanged()
{
	mImage.setSize(mSize);
}

void SwitchComponent::setColor(unsigned int color) {
	mImage.setColorShift(color);
}

bool SwitchComponent::ProcessInput(const InputCompactEvent& event)
{
	if(event.ValidReleased())
	{
		setState(!mState);
		return true;
	}

	return false;
}

void SwitchComponent::Render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

  mImage.Render(trans);

	renderChildren(trans);
}

void SwitchComponent::setImageState()
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
  mImage.setImage(menuTheme.Elements().FromType(mState ? MenuThemeData::IconElement::Type::On : MenuThemeData::IconElement::Type::Off), false);
}

void SwitchComponent::setState(bool state)
{
  if (state != mState)
  {
    mState = state;
    onStateChanged();
    setImageState();
  }
}

void SwitchComponent::onStateChanged()
{
	if (mChangedCallback)
	  mChangedCallback(mState);
	if (mInterface != nullptr)
	  mInterface->SwitchComponentChanged(mId, mState);
}

bool SwitchComponent::CollectHelpItems(Help& help)
{
	help.Set(Help::Valid(), _("CHANGE"));
	return true;
}

String SwitchComponent::getValue() const
{
  return mState ? "true" : "false";
}

bool SwitchComponent::changed() const
{
	return mInitialState != mState;
}

