#include <utils/storage/HashMap.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include "components/HelpComponent.h"
#include "Renderer.h"
#include "views/ViewController.h"
#include <rendering/fonts/FontManager.h>
#include <rendering/textures/TextureManager.h>

#define ICON_TEXT_SPACING (Renderer::Instance().Is480pOrLower() ? 2.0f : Math::max(Renderer::Instance().DisplayWidthAsFloat() * 0.004f, 2.0f)) // space between [icon] and [text] (px)
#define ENTRY_SPACING Math::max(Renderer::Instance().DisplayWidthAsFloat() * 0.008f, 2.0f) // space between [text] and next [icon] (px)

namespace
{
  //! Icon display width for the given line height, ratio preserved.
  //! Height() returns 0 for a texture that is not loaded yet: the loader silently refuses new
  //! requests while its queue is saturated, which is exactly what happens on game exit, right after
  //! every texture has been freed. Dividing by it blindly raises SIGFPE, so fall back to no width -
  //! the icon simply takes no room until it is loaded, as SVG icons already do.
  int IconDisplayWidth(const Texture& icon, int height)
  {
    if (icon.IsSVG()) return icon.Width();
    int iconHeight = icon.Height();
    return iconHeight > 0 ? (icon.Width() * height) / iconHeight : 0;
  }
}

const HashMap<HelpType, Path>& HelpComponent::IconPathMap()
{
  static const HashMap<HelpType, Path> sIconPathMap =
  {
    { HelpType::UpDown,            Path(":/help/dpad_updown.svg") },
    { HelpType::LeftRight,         Path(":/help/dpad_leftright.svg") },
    { HelpType::AllDirections,     Path(":/help/dpad_all.svg") },
    { HelpType::Start,             Path(":/help/button_start.svg") },
    { HelpType::Select,            Path(":/help/button_select.svg") },
    { HelpType::A,                 Path(":/help/button_a.svg") },
    { HelpType::B,                 Path(":/help/button_b.svg") },
    { HelpType::X,                 Path(":/help/button_x.svg") },
    { HelpType::Y,                 Path(":/help/button_y.svg") },
    { HelpType::L,                 Path(":/help/button_l.svg") },
    { HelpType::R,                 Path(":/help/button_r.svg") },
    { HelpType::LR,                Path(":/help/button_lr.svg") },
    { HelpType::L2,                Path(":/help/button_l2.svg") },
    { HelpType::R2,                Path(":/help/button_r2.svg") },
    { HelpType::L2R2,              Path(":/help/button_l2r2.svg") },
    { HelpType::L3,                Path(":/help/button_l3.svg") },
    { HelpType::R3,                Path(":/help/button_r3.svg") },
    { HelpType::L3R3,              Path(":/help/button_l3r3.svg") },
    { HelpType::Joy1AllDirections, Path(":/help/joystick_all_L.svg") },
    { HelpType::Joy1UpDown,        Path(":/help/joystick_updown_L.svg") },
    { HelpType::Joy1LeftRight,     Path(":/help/joystick_leftright_L.svg") },
    { HelpType::Joy2AllDirections, Path(":/help/joystick_all_R.svg") },
    { HelpType::Joy2UpDown,        Path(":/help/joystick_updown_R.svg") },
    { HelpType::Joy2LeftRight,     Path(":/help/joystick_leftright_R.svg") },
    { HelpType::HotkeyCombo,       Path(":/help/button_hotkey.svg") },
    { HelpType::HkUpDown,          Path(":/help/dpad_updown.svg") },
    { HelpType::HkLeftRight,       Path(":/help/dpad_leftright.svg") },
    { HelpType::HkA,               Path(":/help/button_a.svg") },
    { HelpType::HkB,               Path(":/help/button_b.svg") },
    { HelpType::HkX,               Path(":/help/button_x.svg") },
    { HelpType::HkY,               Path(":/help/button_y.svg") },
    { HelpType::HkL,               Path(":/help/button_l.svg") },
    { HelpType::HkR,               Path(":/help/button_r.svg") },
  };
  return sIconPathMap;
}

HelpComponent::HelpComponent(WindowManager&window)
  : ThemableComponent(window)
  , IViewChanged(window)
  //, mGrid(mWindow, Vector2i(Help::TypeCount() * 5, 1))
  , mScrolling(Scrolling::Initialize)
  , mScrollingTimeAccumulator(0)
  , mScrollingLength(0)
  , mScrollingOffset(0)
  , mTextWidth { 0 }
  , mFont(&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false))
  , mPositionFromTheme(0.012f, 0.9515f, 0.f)
  , mSizeFromTheme(1, 1)
  , mTotalWidth(0)
  , mHeight(0)
  , mIconColor(0x777777FF)
  , mTextColor(0x777777FF)
  , mShow(RecalboxConf::Instance().GetShowHelp())
  , mFontRequestedHeight(mFont->RequestedHeight())
{
}

void HelpComponent::Refresh(const Help& newHelpItems, bool force)
{
  // No change?
  if (mHelp.Equals(newHelpItems) && !force && !newHelpItems.IsForcedPosition())
    return;

  if (mHelp.IsForcedPosition())
  {
    mFont = &FontManager::Instance().FromFont(*mFont, mFontRequestedHeight, false);
    PurgeCache();
  }

  // Empty Help?
  //mGrid.ClearEntries();
  mHelp = newHelpItems;
	if (!RecalboxConf::Instance().GetShowHelp() || mHelp.Empty()) return;

  const int height = mFont->Height();
  mHeight = height;
  mTotalWidth = 0;

  for (int i = 0; i < Help::TypeCount(); ++i)
    if (mHelp.IsSet(Help::TypeFromIndex(i)))
    {
      // If the type is equal or higher than HotkCombo, the hotkey icon is required
      int iconWidth = 0;
      if (int hk = (int)HelpType::HotkeyCombo; i >= hk)
      {
        if (!mIcons[hk].Valid())
        {
          // Load Hotkey
          Path path = mImagesPath[hk].Exists() ? mImagesPath[hk] : IconPathMap().get_or_return_default(Help::TypeFromIndex(hk));
          mIcons[hk] = TextureManager::Instance().Create(path, 0, height, TextureHolder::Properties::ImmediateLoad | TextureHolder::Properties::NoCache);
          TextureManager::Instance().RequestTextureLoading(mIcons[hk], true);
        }
        iconWidth += IconDisplayWidth(mIcons[hk], height);
      }
      if (!mIcons[i].Valid())
      {
        // Load button
        Path path = mImagesPath[i].Exists() ? mImagesPath[i] : IconPathMap().get_or_return_default(Help::TypeFromIndex(i));
        mIcons[i] = TextureManager::Instance().Create(path, 0, height, TextureHolder::Properties::ImmediateLoad | TextureHolder::Properties::NoCache);
        TextureManager::Instance().RequestTextureLoading(mIcons[i], true);
      }
      iconWidth += IconDisplayWidth(mIcons[i], height);

      // Compute width
      mTotalWidth += iconWidth;
      mTotalWidth += (int)ICON_TEXT_SPACING;
      mTotalWidth += (mTextWidth[i] = mFont->TextWidth(mHelp.Text(Help::TypeFromIndex(i)).ToUpperCaseUTF8()));
      mTotalWidth += (int)ENTRY_SPACING;
    }

  // Reset scrolling
  mScrollingLength = mTotalWidth - (int)(mSizeFromTheme.x() * Renderer::Instance().DisplayWidthAsFloat());
  mScrollingOffset = 0;
  mScrollingTimeAccumulator = 0;
  mScrolling = Scrolling::Initialize;

  if (mHelp.IsForcedPosition())
  {
    setPosition(Vector3f(mHelp.ForcedX() * Renderer::Instance().DisplayWidthAsFloat(), mHelp.ForcedY() * Renderer::Instance().DisplayHeightAsFloat(), 0.0f).round());
    setSize(mHelp.ForcedWidth() * Renderer::Instance().DisplayWidthAsFloat(), (float)height);
  }
  else
  {
    setPosition(Vector3f(mPositionFromTheme.x() * Renderer::Instance().DisplayWidthAsFloat(), mPositionFromTheme.y() * Renderer::Instance().DisplayHeightAsFloat(), 0.0f).round());
    setSize(mSizeFromTheme.x() * Renderer::Instance().DisplayWidthAsFloat(), (float)height);
  }
}

void HelpComponent::Render(const Transform4x4f& parentTrans)
{
  if (!mShow) return;

	if (mTotalWidth > 0)
  {
    Transform4x4f trans = parentTrans * getTransform();

    Renderer::Instance().Clip(trans, mSize);

    trans.translate({ (float)-mScrollingOffset, 0, 0 });
    Renderer::SetMatrix(trans);

    const int height = mFont->Height();
    int offset = 0;
    for (int i = 0; i < Help::TypeCount(); ++i)
      if (mHelp.IsSet(Help::TypeFromIndex(i)))
      {
        // If the type is equal or higher than HotkCombo, the hotkey icon is required
        if (int hk = (int)HelpType::HotkeyCombo; i >= hk && mIcons[hk].Valid())
        {
          Texture texture = mIcons[(int)HelpType::HotkeyCombo];
          int width = IconDisplayWidth(texture, height);
          texture.Render(offset, 0, width, mHeight, true, false, mIconColor);
          offset += width;
        }
        if (mIcons[i].Valid())
        {
          Texture texture = mIcons[i];
          int width = IconDisplayWidth(texture, height);
          texture.Render(offset, 0, width, height, true, false, mIconColor);
          offset += width;
        }
        offset += (int)ICON_TEXT_SPACING;
        int y = 0; //(((height - mFont->Height()) / 2));
        mFont->DrawText(mHelp.Text(Help::TypeFromIndex(i)).ToUpperCaseUTF8(),Rectangle(offset, y, mTextWidth[i], height), mTextColor, ::Alignment::TopLeft);
        offset += mTextWidth[i] + (int)ENTRY_SPACING;
      }

    Renderer::Instance().Unclip();
  }
}

void HelpComponent::Update(int deltaTime)
{
  if (mTotalWidth > 0)
    if (mScrollingLength > 10)
      switch(mScrolling)
      {
        case Scrolling::Initialize:
        {
          mScrollingOffset = 0;
          mScrollingTimeAccumulator = 0;
          mScrolling = Scrolling::PauseLeft;
          break;
        }
        case Scrolling::PauseLeft:
        {
          mScrollingOffset = 0;
          if (mScrollingTimeAccumulator += deltaTime; mScrollingTimeAccumulator > sPauseTime)
          {
            mScrollingTimeAccumulator = 0;
            mScrolling = Scrolling::ScrollToRight;
          }
          break;
        }
        case Scrolling::ScrollToRight:
        {
          if (++mScrollingOffset >= mScrollingLength)
          {
            mScrollingTimeAccumulator = 0;
            mScrolling = Scrolling::PauseRight;
          }
          break;
        }
        case Scrolling::PauseRight:
        {
          mScrollingOffset = mScrollingLength - 1;
          if (mScrollingTimeAccumulator += deltaTime; mScrollingTimeAccumulator > sPauseTime)
          {
            mScrollingTimeAccumulator = 0;
            mScrolling = Scrolling::ScrollToLeft;
          }
          break;
        }
        case Scrolling::ScrollToLeft:
        {
          if (--mScrollingOffset <= 0)
          {
            mScrollingTimeAccumulator = 0;
            mScrolling = Scrolling::PauseLeft;
          }
          break;
        }
      }
}

void HelpComponent::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)theme; // Always use main theme
  (void)refreshOnly; // Always rebuild, this is fast
  ViewChanged(ViewController::Instance().CurrentView(), mWindow.HasGui());
  PurgeCache();
  Refresh(mHelp, true);
  (void)interface;
}

void HelpComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  (void)properties;

  if(element.HasProperty(ThemePropertyName::Pos)) mPositionFromTheme = Vector3f(element.AsVector(ThemePropertyName::Pos));
  else                                            mPositionFromTheme = Vector3f(0.012f, 0.9515f, 0.f);
  if(element.HasProperty(ThemePropertyName::Size)) mSizeFromTheme = Vector2f(element.AsVector(ThemePropertyName::Size));
  else                                             mSizeFromTheme = Vector2f(1.f - mPositionFromTheme.x(), 1);

  // Reset size to its calculated size cause the caller already read & set the size property
  setSize(mSizeFromTheme.x() * Renderer::Instance().DisplayWidthAsFloat(), (float)mHeight);

  mTextColor = (element.HasProperty(ThemePropertyName::TextColor)) ? (unsigned int)element.AsInt(ThemePropertyName::TextColor) : 0x777777FF;
  mIconColor = (element.HasProperty(ThemePropertyName::IconColor)) ? (unsigned int)element.AsInt(ThemePropertyName::IconColor) : 0x777777FF;
  mFont = &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false);
  if(element.HasProperty(ThemePropertyName::FontSize) || element.HasProperty(ThemePropertyName::FontPath))
    mFont = &FontManager::Instance().FromTheme(element, ThemePropertyCategory::All, mFont);

  for(int i=Help::TypeCount(); --i>=0; )
    mImagesPath[i] = Path(IconPathMap().get_or_return_default(Help::TypeFromIndex(i)));
  if(element.HasProperty(ThemePropertyName::IconUpDown))            mImagesPath[(int)HelpType::UpDown]            = Path(element.AsString(ThemePropertyName::IconUpDown));
  if(element.HasProperty(ThemePropertyName::IconLeftRight))         mImagesPath[(int)HelpType::LeftRight]         = Path(element.AsString(ThemePropertyName::IconLeftRight));
  if(element.HasProperty(ThemePropertyName::IconUpDownLeftRight))   mImagesPath[(int)HelpType::AllDirections]     = Path(element.AsString(ThemePropertyName::IconUpDownLeftRight));
  if(element.HasProperty(ThemePropertyName::IconA))                 mImagesPath[(int)HelpType::A]                 = Path(element.AsString(ThemePropertyName::IconA));
  if(element.HasProperty(ThemePropertyName::IconB))                 mImagesPath[(int)HelpType::B]                 = Path(element.AsString(ThemePropertyName::IconB));
  if(element.HasProperty(ThemePropertyName::IconX))                 mImagesPath[(int)HelpType::X]                 = Path(element.AsString(ThemePropertyName::IconX));
  if(element.HasProperty(ThemePropertyName::IconY))                 mImagesPath[(int)HelpType::Y]                 = Path(element.AsString(ThemePropertyName::IconY));
  if(element.HasProperty(ThemePropertyName::IconL))                 mImagesPath[(int)HelpType::L]                 = Path(element.AsString(ThemePropertyName::IconL));
  if(element.HasProperty(ThemePropertyName::IconR))                 mImagesPath[(int)HelpType::R]                 = Path(element.AsString(ThemePropertyName::IconR));
  if(element.HasProperty(ThemePropertyName::IconLR))                mImagesPath[(int)HelpType::LR]                = Path(element.AsString(ThemePropertyName::IconLR));
  if(element.HasProperty(ThemePropertyName::IconL2))                mImagesPath[(int)HelpType::L2]                = Path(element.AsString(ThemePropertyName::IconL2));
  if(element.HasProperty(ThemePropertyName::IconR2))                mImagesPath[(int)HelpType::R2]                = Path(element.AsString(ThemePropertyName::IconR2));
  if(element.HasProperty(ThemePropertyName::IconL2R2))              mImagesPath[(int)HelpType::L2R2]              = Path(element.AsString(ThemePropertyName::IconL2R2));
  if(element.HasProperty(ThemePropertyName::IconL3))                mImagesPath[(int)HelpType::L3]                = Path(element.AsString(ThemePropertyName::IconL3));
  if(element.HasProperty(ThemePropertyName::IconR3))                mImagesPath[(int)HelpType::R3]                = Path(element.AsString(ThemePropertyName::IconR3));
  if(element.HasProperty(ThemePropertyName::IconL3R3))              mImagesPath[(int)HelpType::L3R3]              = Path(element.AsString(ThemePropertyName::IconL3R3));
  if(element.HasProperty(ThemePropertyName::IconStart))             mImagesPath[(int)HelpType::Start]             = Path(element.AsString(ThemePropertyName::IconStart));
  if(element.HasProperty(ThemePropertyName::IconSelect))            mImagesPath[(int)HelpType::Select]            = Path(element.AsString(ThemePropertyName::IconSelect));
  if(element.HasProperty(ThemePropertyName::IconHotkey))            mImagesPath[(int)HelpType::HotkeyCombo]       = Path(element.AsString(ThemePropertyName::IconHotkey));
  if(element.HasProperty(ThemePropertyName::IconJ1UpDownLeftRight)) mImagesPath[(int)HelpType::Joy1AllDirections] = Path(element.AsString(ThemePropertyName::IconJ1UpDownLeftRight));
  if(element.HasProperty(ThemePropertyName::IconJ1UpDown))          mImagesPath[(int)HelpType::Joy1UpDown]        = Path(element.AsString(ThemePropertyName::IconJ1UpDown));
  if(element.HasProperty(ThemePropertyName::IconJ1LeftRight))       mImagesPath[(int)HelpType::Joy1LeftRight]     = Path(element.AsString(ThemePropertyName::IconJ1LeftRight));
  if(element.HasProperty(ThemePropertyName::IconJ2UpDownLeftRight)) mImagesPath[(int)HelpType::Joy2AllDirections] = Path(element.AsString(ThemePropertyName::IconJ2UpDownLeftRight));
  if(element.HasProperty(ThemePropertyName::IconJ2UpDown))          mImagesPath[(int)HelpType::Joy2UpDown]        = Path(element.AsString(ThemePropertyName::IconJ2UpDown));
  if(element.HasProperty(ThemePropertyName::IconJ2LeftRight))       mImagesPath[(int)HelpType::Joy2LeftRight]     = Path(element.AsString(ThemePropertyName::IconJ2LeftRight));
  if(element.HasProperty(ThemePropertyName::IconHkA))               mImagesPath[(int)HelpType::HkA]               = Path(element.AsString(ThemePropertyName::IconHkA));
  if(element.HasProperty(ThemePropertyName::IconHkB))               mImagesPath[(int)HelpType::HkB]               = Path(element.AsString(ThemePropertyName::IconHkB));
  if(element.HasProperty(ThemePropertyName::IconHkL))               mImagesPath[(int)HelpType::HkL]               = Path(element.AsString(ThemePropertyName::IconHkL));
  if(element.HasProperty(ThemePropertyName::IconHkR))               mImagesPath[(int)HelpType::HkR]               = Path(element.AsString(ThemePropertyName::IconHkR));
  if(element.HasProperty(ThemePropertyName::IconHkX))               mImagesPath[(int)HelpType::HkX]               = Path(element.AsString(ThemePropertyName::IconHkX));
  if(element.HasProperty(ThemePropertyName::IconHkY))               mImagesPath[(int)HelpType::HkY]               = Path(element.AsString(ThemePropertyName::IconHkY));
  if(element.HasProperty(ThemePropertyName::IconHkLeftRight))       mImagesPath[(int)HelpType::HkLeftRight]       = Path(element.AsString(ThemePropertyName::IconHkLeftRight));

  if (mHelp.IsForcedPosition())
  {
    setPosition(Vector3f(mHelp.ForcedX() * Renderer::Instance().DisplayWidthAsFloat(), mHelp.ForcedY() * Renderer::Instance().DisplayHeightAsFloat(), 0.0f).round());
    setSize(mHelp.ForcedWidth() * Renderer::Instance().DisplayWidthAsFloat(), (float)mHeight);
    mFontRequestedHeight = mFont->RequestedHeight();
  }
}

void HelpComponent::ViewChanged(ViewType currentView, bool hasWindowOver)
{
  String viewName = "system";
  const ThemeData* theme = &ThemeManager::Instance().Main();

  if (hasWindowOver)
  {
    // We are on a menu or a window. Select menu view or if it doesn't exist, just keep default
    if (ThemeManager::Instance().Main().HasMenuView())
      viewName = "menu";
  }
  else
  {
    switch(currentView)
    {
      case ViewType::GameList:
      {
        ISimpleGameListView& view = ((ISimpleGameListView&)ViewController::Instance().CurrentUi());
        viewName = view.getName();
        theme = &ThemeManager::Instance().System(&view.System());
        break;
      }
      case ViewType::GameClip:
      case ViewType::SystemList:
      case ViewType::CrtCalibration:
      case ViewType::CardReaderConsoleView:
      case ViewType::None:
      case ViewType::SplashScreen:
      default: break; // Keep system view
    }
  }
  DoApplyThemeElement(*theme, viewName, "help", ThemePropertyCategory::All);

  PurgeCache();
  Refresh(mHelp, true);
}

void HelpComponent::PurgeCache()
{
  for(Texture& icon : mIcons)
    icon.Clear();
}

