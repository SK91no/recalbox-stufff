#include "components/DateTimeComponent.h"
#include "Renderer.h"
#include "WindowManager.h"
#include "utils/Log.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>
#include <rendering/fonts/FontManager.h>

DateTimeComponent::DateTimeComponent(WindowManager&window, Display dispMode)
  : ThemableComponent(window)
  , mDisplayMode(dispMode)
  , mRelativeUpdateAccumulator(0)
  , mColor(0x777777FF)
  , mOriginColor(0)
  , mBackgroundColor(0)
  , mFont(&FontManager::Instance().FromFile(Path(FONT_PATH_LIGHT), FONT_SIZE_SMALL, false))
  , mHorizontalAlignment(::HorizontalAlignment::Left)
  , mUppercase(false)
  , mAutoCalcExtentX(false)
  , mAutoCalcExtentY(false)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	setFont(menuTheme.SmallText().font);
	setColor(menuTheme.Text().color);
	setOriginColor(mColor);
	mFlag = true;
	updateTextCache();
}

void DateTimeComponent::setDisplayMode(Display mode)
{
	mDisplayMode = mode;
	updateTextCache();
}

void DateTimeComponent::Update(int deltaTime)
{
	if(mDisplayMode == Display::RelativeToNow || mDisplayMode == Display::RealTime)
	{
		mRelativeUpdateAccumulator += deltaTime;
		if(mRelativeUpdateAccumulator > 1000)
		{
			mRelativeUpdateAccumulator = 0;
			updateTextCache();
		}
	}

  Component::Update(deltaTime);
}

void DateTimeComponent::calculateExtent(const String& text)
{
  if (mAutoCalcExtentX)
    mSize = mFont->TextSize(mUppercase ? text.ToUpperCaseUTF8() : text);
  else if(mAutoCalcExtentY)
    mSize.y() = (float)mFont->SplitText(mUppercase ? text.ToUpperCaseUTF8() : text, (int)mSize.x()).Chunks.Count() * (float)mFont->Height();
}

void DateTimeComponent::Render(const Transform4x4f& parentTrans)
{
  if(mThemeDisabled) return;

	Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Display mode = getCurrentDisplayMode();
  const String dispString = mUppercase ? getDisplayString(mode).UpperCase() : getDisplayString(mode);
  Rectangle area(0.f, 0.f, mSize.x(), mSize.y());
  if (mBackgroundColor != 0) Renderer::DrawRectangle(area, mBackgroundColor);
  mFont->DrawText(dispString, area, (mColor & 0xFFFFFF00) | getOpacity(), AlignmentMergeVerticalAndHorizontal(::VerticalAlignment::Center, mHorizontalAlignment));
}

void DateTimeComponent::setValue(const String& val)
{
  DateTime dt(false);
  if (DateTime::FromCompactISO6801(val, dt))
  {
    mTime = dt;
    updateTextCache();
  }
}

void DateTimeComponent::setValue(const DateTime& dt)
{
  mTime = dt;
  updateTextCache();
}

String DateTimeComponent::getValue() const
{
	return mTime.ToCompactISO8601();
}

DateTimeComponent::Display DateTimeComponent::getCurrentDisplayMode() const
{
	return mDisplayMode;
}

String DateTimeComponent::getDisplayString(Display mode) const
{
	char strbuf[256];

	switch(mode)
	{
	  case Display::Date: return mTime.ToStringFormat("%YYYY/%MM/%dd");
    case Display::DateTime: return mTime.ToStringFormat("%YYYY/%MM/%dd %HH:%mm:%ss");
    case Display::Time: return mTime.ToStringFormat("%HH:%mm:%ss");
    case Display::Year: return mTime.ToStringFormat("%YYYY");
    case Display::RealTime: return DateTime().ToStringFormat("%HH:%mm:%ss");
    case Display::RelativeToNow:
		{
      TimeSpan diff = DateTime() - mTime;
      if (diff.IsNegative() || diff.TotalDays() > 3560)
        return _("never");

			if (diff.TotalSeconds() < 2) return _("just now");
			if (diff.TotalSeconds() < 60) { snprintf(strbuf, sizeof(strbuf), _N("%i sec ago", "%i secs ago", diff.TotalSeconds()).c_str(), (int) diff.TotalSeconds()); return strbuf; }
			if (diff.TotalMinutes() < 60) { snprintf(strbuf, sizeof(strbuf), _N("%i min ago", "%i mins ago", diff.TotalMinutes()).c_str(), (int) diff.TotalMinutes()); return strbuf; }
			if (diff.TotalHours()   < 24) { snprintf(strbuf, sizeof(strbuf), _N("%i hour ago", "%i hours ago", diff.TotalHours()).c_str(), (int) diff.TotalHours()); return strbuf; }
			snprintf(strbuf, sizeof(strbuf), _N("%i day ago", "%i days ago", diff.TotalDays()).c_str(), (int) diff.TotalDays());
			return strbuf;
		}
	  default: break;
	}

	return "???";
}

void DateTimeComponent::updateTextCache()
{
	mFlag = !mFlag;

  Display mode = getCurrentDisplayMode();
  const String dispString = mUppercase ? getDisplayString(mode).UpperCase() : getDisplayString(mode);
  calculateExtent(dispString);
}

void DateTimeComponent::setColor(unsigned int color)
{
	mColor = color;
}

void DateTimeComponent::setFont(NewFont* font)
{
	mFont = font;
	updateTextCache();
}

void DateTimeComponent::setHorizontalAlignment(::HorizontalAlignment align)
{
	mHorizontalAlignment = align;
	updateTextCache();
}

void DateTimeComponent::onSizeChanged()
{
  mAutoCalcExtentX = (getSize().x() == 0);
  mAutoCalcExtentY = (getSize().y() == 0);
	updateTextCache();
}

void DateTimeComponent::setUppercase(bool uppercase)
{
	mUppercase = uppercase;
	updateTextCache();
}

void DateTimeComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
	if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    setColor(element.HasProperty(ThemePropertyName::Color) ? (unsigned int) element.AsInt(ThemePropertyName::Color) : 0);
    mBackgroundColor = element.HasProperty(ThemePropertyName::BackgroundColor) ? (unsigned int) element.AsInt(ThemePropertyName::BackgroundColor) : 0;
  }

	if (hasFlag(properties, ThemePropertyCategory::Display))
  {
    if (element.HasProperty(ThemePropertyName::Display))
    {
      String str = element.AsString(ThemePropertyName::Display);
      if (str == "date") setDisplayMode(Display::Date);
      else if (str == "dateTime") setDisplayMode(Display::DateTime);
      else if (str == "year") setDisplayMode(Display::Year);
      else if (str == "realTime") setDisplayMode(Display::RealTime);
      else if (str == "time") setDisplayMode(Display::Time);
      else if (str == "RelativeToNow") setDisplayMode(Display::RelativeToNow);
      else { LOG(LogError) << "[DateTimeComponent] Unknown date time display mode string: " << str; }
    }
    //else setDisplayMode(Display::Date);
  }

	if (hasFlag(properties, ThemePropertyCategory::Text))
  {
    if (element.HasProperty(ThemePropertyName::Alignment)) setHorizontalAlignment(AlignmentExtractHorizontal(element.AsAlignment(ThemePropertyName::Alignment)));
    else setHorizontalAlignment(HorizontalAlignment::Left);
    setUppercase(element.HasProperty(ThemePropertyName::ForceUppercase) ? element.AsBool(ThemePropertyName::ForceUppercase) : mUppercase);
  }

  if (hasFlag(properties, ThemePropertyCategory::Font))
    if (element.HasProperty(ThemePropertyName::FontPath))
	    setFont(&FontManager::Instance().FromTheme(element, properties, mFont));
}
