#include "views/SystemView.h"
#include "views/ViewController.h"
#include "animations/LambdaAnimation.h"
#include <guis/GuiMsgBox.h>
#include <RecalboxConf.h>
#include <guis/GuiNetPlay.h>
#include <systems/SystemManager.h>
#include <usernotifications/NotificationManager.h>
#include "audio/AudioScheduler.h"
#include <guis/GuiSearch.h>
#include <emulators/run/GameRunner.h>
#include "MenuFilter.h"
#include "network/Networks.h"
#include <guis/menus/MenuProvider.h>

// buffer values for scrolling velocity (left, stopped, right)
const int logoBuffersLeft[] = { -1, -2, -5, -7, -9, -11 };
const int logoBuffersRight[] = { 1, 2, 5, 7, 9, 11 };

SystemView::SystemView(WindowManager& window, SystemManager& systemManager, const IGlobalVariableResolver& resolver)
  : IList<SystemViewData, SystemData*>(window, LIST_SCROLL_STYLE_SLOW, LoopType::Always)
  , mResolver(resolver)
  , mSystemManager(systemManager)
  , mCarousel()
  , mSystemInfo(window, "SYSTEM INFO", &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false), 0x33333300, ::Alignment::Center)
  , mProgressInterface(nullptr)
  , mSender(*this)
  , mSystemFromWitchToExtractData(nullptr)
  , mCurrentSystem(nullptr)
  , mCamOffset(0)
  , mExtrasCamOffset(0)
  , mExtrasFadeOpacity(0.0f)
  , mViewNeedsReload(true)
  , mShowing(false)
  , mLaunchKodi(false)
{
  setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
  Start("system-info");
}

void SystemView::AlignComponent(CarouselType carouselType, ::Alignment alignment, Component* component)
{
  if (carouselType == CarouselType::Vertical || carouselType == CarouselType::VerticalWheel)
  {
    ::HorizontalAlignment hz = AlignmentExtractHorizontal(alignment);
    if      (hz == ::HorizontalAlignment::Left)  component->setOrigin(0, 0.5);
    else if (hz == ::HorizontalAlignment::Right) component->setOrigin(1.0, 0.5);
    else                                         component->setOrigin(0.5, 0.5);
  }
  else
  {
    ::VerticalAlignment vt = AlignmentExtractVertical(alignment);
    if      (vt == ::VerticalAlignment::Top)    component->setOrigin(0.5, 0);
    else if (vt == ::VerticalAlignment::Bottom) component->setOrigin(0.5, 1);
    else                                        component->setOrigin(0.5, 0.5);
  }
}

void SystemView::AssignSystemLogo(const SystemData& system, const ThemeData& theme, Entry& entry)
{
  // Reset previous logo/text
  entry.data.logo.reset();
  entry.data.logotext.reset();

  if (mCarousel.textOnly)
  {
    // Text-list mode: rendered directly with the font in renderTextList (no TextComponent)
    entry.data.logo = nullptr;
    entry.data.logotext = nullptr;
    entry.data.textWidth = -1; // invalidate cached text width
  }
  else
  {
    // make logo
    const ThemeElement* logoElement = theme.Element("system", "logo", ThemeElementType::Image, ThemeElementType::None);
    bool hasLogo = logoElement != nullptr && logoElement->HasProperty(ThemePropertyName::Path);
    Path logoPath = hasLogo ? logoElement->AsPath(ThemePropertyName::Path) : Path::Empty;
    // Theme logo
    bool isDefault = hasLogo && ( logoPath.FilenameWithoutExtension().LowerCase() == "default" || logoPath.ToString().ToLowerCase().Contains("/default/") );
    bool fileExists = hasLogo && logoPath.Exists();
    Path internalLogoPath;
    bool internalLogoExists = ThemeManager::GetDefaultSystemLogo(system, internalLogoPath);

    if (fileExists && !(isDefault && internalLogoExists))
    {
      ImageComponent* logo = new ImageComponent(mWindow, ImgProps::KeepRatio);
      logo->setResize(mCarousel.logoSize * mCarousel.logoScale);
      logo->DoApplyThemeElement(theme, "system", "logo", ThemePropertyCategory::Path | ThemePropertyCategory::Color);
      logo->StartLoading();
      entry.data.logo = std::shared_ptr<ImageComponent>(logo);
      if (isDefault)
      {
        TextComponent* text = new TextComponent(mWindow, system.Name(), &FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false),
                                                mCarousel.fontcolor ^ 0x80808000, ::Alignment::Center);
        text->setSize(mCarousel.logoSize * mCarousel.logoScale);
        entry.data.logotext = std::shared_ptr<TextComponent>(text);
        text->setAlignment(mCarousel.logoAlignment);
      }
    }
    else if (internalLogoExists)
    {
      ImageComponent* logo = new ImageComponent(mWindow, ImgProps::KeepRatio);
      logo->setResize(mCarousel.logoSize * mCarousel.logoScale);
      logo->setImage(internalLogoPath, false);
      logo->DoApplyThemeElement(theme, "system", "logo", ThemePropertyCategory::Color);
      logo->StartLoading();
      entry.data.logo = std::shared_ptr<ImageComponent>(logo);
    }
    else
    {
      String systemName = system.Name();
      systemName.Remove(SystemManager::sGenrePrefix);
      GameGenres genre = Genres::LookupFromName(systemName);
      if (genre == GameGenres::None)
      {
        // no logo in theme; use text
        entry.data.logo = nullptr;
        TextComponent* text = new TextComponent(mWindow, system.FullName(), mCarousel.fontlarge, 0x000000FF,
                                                ::Alignment::Center);
        text->setSize(mCarousel.logoSize * mCarousel.logoScale);
        entry.data.logotext = std::shared_ptr<TextComponent>(text);
        text->setAlignment(mCarousel.logoAlignment);
      }
      else
      {
        ImageComponent* logo = new ImageComponent(mWindow, ImgProps::KeepRatio);
        logo->setResize(mCarousel.logoSize * mCarousel.logoScale);
        logo->setColor(0xFF);
        logo->setImage(Genres::GetResourcePath(genre), false);
        logo->StartLoading();
        entry.data.logo = std::shared_ptr<ImageComponent>(logo);

        TextComponent* text = new TextComponent(mWindow, system.FullName(), mCarousel.fontmedium,
                                                mCarousel.fontcolor, ::Alignment::Center);
        text->setSize(mCarousel.logoSize * mCarousel.logoScale);
        entry.data.logotext = std::shared_ptr<TextComponent>(text);
        text->setAlignment(mCarousel.logoAlignment);
      }
    }
  }

  // Text-list mode positions its entries every frame in renderTextList()
  if (!mCarousel.textOnly)
  {
    if (entry.data.logo)
    {
      AlignComponent(mCarousel.type, mCarousel.logoAlignment, entry.data.logo.get());
      Vector2f denormalized = mCarousel.logoSize * entry.data.logo->getOrigin();
      entry.data.logo->setPosition(denormalized.x(), denormalized.y(), 0.0);
    }

    if (entry.data.logotext)
    {
      AlignComponent(mCarousel.type, mCarousel.logoAlignment, entry.data.logotext.get());
      Vector2f denormalized = mCarousel.logoSize * entry.data.logotext->getOrigin();
      entry.data.logotext->setPosition(denormalized.x(), denormalized.y(), 0.0);
    }
  }
}

void SystemView::addSystem(SystemData * it)
{
  const ThemeData& theme = (it)->Theme();

  if (mViewNeedsReload)
    RefreshViewElements(theme);

  Entry e;
  e.name = (it)->Name();
  e.object = it;

  AssignSystemLogo(*it, it->Theme(), e);

  e.data.backgroundExtras = std::make_shared<ThemeExtras>(mWindow);
  e.data.backgroundExtras->AssignExtras((it)->Theme(), "system", (it)->Theme().GetExtras("system", mWindow, nullptr), false);
  e.data.backgroundExtras->setSize(mSize);

  this->add(e);
}

SystemData* SystemView::Prev()
{
  SystemData* prev = mSystemManager.PreviousVisible(mCurrentSystem);
  while(!prev->HasVisibleGame())
    prev = mSystemManager.PreviousVisible(prev);

  return prev;
}

void SystemView::Sort()
{
  // Make a reference map: system => entry
  HashMap<const SystemData*, Entry*> map;
  for(Entry& entry : mEntries) map[entry.object] = &entry;

  // Sort
  std::vector<Entry> newEntries;
  for(const SystemData* system : mSystemManager.VisibleSystemList())
    if (Entry** entry = map.try_get(system); entry != nullptr)
      newEntries.push_back(**entry);
    else
    { LOG(LogError) << "[SystemView] Sort cannot lookup visible system '" << system->FullName() << "' in system entries!"; }

  // Set new sorted vector
  mEntries = newEntries;
  goToSystem(mCurrentSystem, false);
}

void SystemView::populate()
{
  mEntries.clear();

  // Count valid systems
  int count = 0;
  SystemManager::List systems;
  for (const auto& it : mSystemManager.VisibleSystemList())
    if (it->HasVisibleGame())
    {
      systems.Add(it);
      ++count;
    }

  // Initialize progress
  if (mProgressInterface != nullptr)
  {
    mProgressInterface->SetMaximum(count);
    mProgressInterface->SetProgress(0);
  }

  // Initialize systems
  count = 0;
  for (const auto& it : systems)
  {
    addSystem(it);
    if (mProgressInterface != nullptr)
      mProgressInterface->SetProgress(++count);
  }
  Sort();
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
  // Systems lazy initialization
  if (mEntries.empty())
    populate();

  if (!setCursor(system)) // When deleting last favorite from favorite view, favorite system is no longer available
    setCursor(mSystemManager.FirstNonEmptySystem());

  if(!animate)
    finishAnimation(0);
  onCursorChanged(CursorState::Stopped);
}

bool SystemView::ProcessInput(const InputCompactEvent& event)
{
    switch (mCarousel.type)
    {
      case CarouselType::Vertical:
      case CarouselType::VerticalWheel:
      {
        if (event.UpPressed()) { listInput(-1); return true; }
        if (event.DownPressed()) { listInput(1); return true; }
        if (event.J1UpPressed()) { listInput(-1); return true; }
        if (event.J1DownPressed()) { listInput(1); return true; }
        //if (event.J2UpPressed()) { listInput(-1, 1); return true; }
        //if (event.J2DownPressed()) { listInput(1, 1); return true; }
        break;
      }
      case CarouselType::Horizontal:
      default:
      {
        if (event.LeftPressed()) { listInput(-1); return true; }
        if (event.RightPressed()) { listInput(1); return true; }
        if (event.J1LeftPressed()) { listInput(-1); return true; }
        if (event.J1RightPressed()) { listInput(1); return true; }
        //if (event.J2LeftPressed()) { listInput(-1, 1); return true; }
        //if (event.J2RightPressed()) { listInput(1, 1); return true; }
        break;
      }
    }
    if (event.L1Pressed()) { listInput(-5); return true; }
    if (event.R1Pressed()) { listInput(5); return true; }
    if (event.ValidReleased())
    {
      stopScrolling();
      ViewController::Instance().goToGameList(getSelected());
      return true;
    }
    if (event.HotkeyWestReleased())
    {
      switch(RecalboxConf::Instance().GetScreenSaverType())
      {
        case RecalboxConf::Screensaver::Demo:
        {
          // Enter sleep mode & let main loop run demos
          mWindow.DoSleep();
          return true;
        }
        case RecalboxConf::Screensaver::Gameclip:
        {
          mWindow.DoSleep();
          ViewController::Instance().goToGameClipView();
          return true;
        }
        case RecalboxConf::Screensaver::Bouncing:
        {
          mWindow.DoSleep();
          ViewController::Instance().goToBouncingView();
          return true;
        }
        case RecalboxConf::Screensaver::Black:
        case RecalboxConf::Screensaver::Dim:
        case RecalboxConf::Screensaver::Suspend:
        default: break;
      }
    }

    if (event.NorthReleased())
    {
      bool kodiExists = RecalboxSystem::kodiExists();
      bool kodiEnabled = RecalboxConf::Instance().GetKodiEnabled();
      bool kodiX = RecalboxConf::Instance().GetKodiXButton();
      bool netplay = RecalboxConf::Instance().GetNetplayEnabled();

      if (kodiExists && kodiEnabled && kodiX && !mLaunchKodi && !mWindow.HasGui())
      {
        if (netplay) MenuProvider::ShowMenu(MenuContainerType::ChooseKodiLobby, InheritableContext());
        else
        {
          mLaunchKodi = true;
          if (!GameRunner::Instance().RunKodi()) { LOG(LogWarning) << "[SystemView] Kodi terminated with non-zero result!"; }
          mLaunchKodi = false;
        }
      }
      else if (netplay && !mWindow.HasGui())
      {
        if (Networks::Instance().HasIP(Networks::IPVersion::Both, Networks::Interfaces::Both))
          mWindow.pushGui(new GuiNetPlay(mWindow, mSystemManager));
        else
          mWindow.displayMessage(_("There is no network available.\nPlease connect your recalbox and try again."));
      }
    }

    if (event.SelectReleased() && MenuFilter::ShouldDisplayMenu(MenuFilter::Menu::Exit))
    {
      MenuProvider::ShowMenu(MenuContainerType::Quit, InheritableContext());
      return true;
    }

    if (event.StartReleased() && MenuFilter::ShouldDisplayMenu(MenuFilter::Menu::Main))
    {
      MenuProvider::ShowMenu(MenuContainerType::Main, InheritableContext());
      return true;
    }

    if (event.WestReleased() && MenuFilter::ShouldDisplayMenu(MenuFilter::Menu::Search))
    {
      mWindow.pushGui(new GuiSearch(mWindow, mSystemManager, nullptr));
      return true;
    }

  if (event.HotkeyR1Released() && AudioScheduler::Instance().IsPlaying())
  {
    AudioScheduler::Instance().StopAll();
    AudioScheduler::Instance().PlayNextMusic();
  }

  if (event.AnyDirectionReleased() || event.L1Released() || event.R1Released())
    listInput(0);

  return InputExtra(event);
}

void SystemView::Update(int deltaTime)
{
  UpdateExtra(deltaTime);
  listUpdate(deltaTime);

  if (mCarousel.textOnly && mBarTimer > 0) mBarTimer -= deltaTime;

  if (mCarousel.textOnly && !mEntries.empty() && mCarousel.fontlarge != nullptr)
  {
    int textWidth = entryTextWidth(mCursor, entryDisplayName(mCursor));
    int availWidth = (int)textListAreaWidth();
    if (textWidth > availWidth)
    {
      int scrollSpan = textWidth - availWidth;
      switch (mMarqueeStep)
      {
        case MarqueeSteps::LeftPause:
          mMarqueeOffset = 0;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::ScrollToRight; }
          break;
        case MarqueeSteps::ScrollToRight:
          mMarqueeOffset = (mMarqueeTime * sMarqueeSpeed1) / 1000;
          if (mMarqueeOffset >= scrollSpan) { mMarqueeTime = 0; mMarqueeOffset = scrollSpan; mMarqueeStep = MarqueeSteps::RightPause; }
          break;
        case MarqueeSteps::RightPause:
          mMarqueeOffset = scrollSpan;
          if (mMarqueeTime > sMarqueePause) { mMarqueeTime = 0; mMarqueeStep = MarqueeSteps::RollOver; }
          break;
        case MarqueeSteps::RollOver:
          mMarqueeOffset = scrollSpan + (mMarqueeTime * sMarqueeSpeed2) / 1000;
          if (mMarqueeOffset >= textWidth + (int)(mCarousel.size.x() / 4)) { mMarqueeTime = 0; mMarqueeOffset = 0; mMarqueeStep = MarqueeSteps::LeftPause; }
          break;
      }
      mMarqueeTime += deltaTime;
    }
    else
      mMarqueeOffset = 0;
  }

  Component::Update(deltaTime);
}

void SystemView::onCursorChanged(const CursorState& state)
{
  (void)state;

  // Reset marquee when selection changes
  mMarqueeOffset = 0;
  mMarqueeTime   = 0;
  mMarqueeStep   = MarqueeSteps::LeftPause;

  // Trigger scrollbar fade-out on navigation
  if (mCarousel.textOnly)
  {
    mShowBar  = false;
    mBarTimer = sBarFadeTime + sBarFixedTime;
  }

  if(mCurrentSystem != getSelected())
  {
    mCurrentSystem = getSelected();
    ThemeManager::ChangeThemeContext("system", getSelectedEntry().backgroundExtras->Extras(), getSelected(), nullptr);
    AudioScheduler::Instance().SetTheme(getSelected()->Theme()).PlayNextMusic();
  }
  // update help style
  UpdateHelpBar();

  // update externs
  NotificationManager::Instance().Notify(*getSelected(), Notification::SystemBrowsing);

  float startPos = mCamOffset;

  float posMax = (float)mEntries.size();
  float target = (float)mCursor;

  // what's the shortest way to get to our target?
  // it's one of these...

  float endPos = target; // directly
  float dist = std::abs(endPos - startPos);

  if (std::abs(target + posMax - startPos) < dist) endPos = target + posMax; // loop around the end (0 -> max)
  if (std::abs(target - posMax - startPos) < dist) endPos = target - posMax; // loop around the start (max - 1 -> -1)

  // Set next system from witch extract game information
  SetNextSystem(getSelected());
  cancelAnimation(1);
  cancelAnimation(2);
  mSystemInfo.setOpacity(0);

  // no need to animate transition, we're not going anywhere (probably mEntries.size() == 1)
  if(endPos == mCamOffset && endPos == mExtrasCamOffset)
    return;

  Animation* anim = nullptr;
  bool move_carousel = RecalboxConf::Instance().GetThemeCarousel();
  RecalboxConf::Transition transitionTheme = ThemeManager::Instance().Main().getThemeTransition();
  if (transitionTheme == RecalboxConf::Transition::Fade)
  {
    float startExtrasFade = mExtrasFadeOpacity;
    anim = new LambdaAnimation([startExtrasFade, startPos, endPos, posMax, this, move_carousel](float t)
    {
      t -= 1;
      float f = lerp<float>(startPos, endPos, t*t*t + 1);
      if(f < 0) f += posMax;
      if(f >= posMax) f -= posMax;
      mCamOffset = move_carousel ? f : endPos;

      t += 1;
      if(t < 0.3f) mExtrasFadeOpacity = lerp<float>(0.0f, 1.0f, t / 0.3f + startExtrasFade);
      else if(t < 0.7f) mExtrasFadeOpacity = 1.0f;
      else mExtrasFadeOpacity = lerp<float>(1.0f, 0.0f, (t - 0.7f) / 0.3f);

      if(t > 0.5f) mExtrasCamOffset = endPos;
    }, 500);
  }
  else if (transitionTheme == RecalboxConf::Transition::Slide)
  { // slide
    anim = new LambdaAnimation([startPos, endPos, posMax, this, move_carousel](float t)
    {
      t -= 1;
      float f = lerp<float>(startPos, endPos, t*t*t + 1);
      if(f < 0) f += posMax;
      if(f >= posMax) f -= posMax;

      mCamOffset = move_carousel ? f : endPos;
      mExtrasCamOffset = f;
    }, 500);
  }
  else
  {
  // instant
    anim = new LambdaAnimation([this, startPos, endPos, posMax, move_carousel](float t)
    {
      t -= 1;
      float f = lerp<float>(startPos, endPos, t*t*t + 1);
      if (f < 0) f += posMax;
      if (f >= posMax) f -= posMax;
      mCamOffset = move_carousel ? f : endPos;
      mExtrasCamOffset = endPos;
    }, move_carousel ? 500 : 1);
  }

  setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::Render(const Transform4x4f& parentTrans)
{
  if(size() == 0) return;  // nothing to render

  // draw the list elements (titles, backgrounds, logos)
  Transform4x4f trans = getTransform() * parentTrans;

  auto systemInfoZIndex = mSystemInfo.getZIndex();
  auto minMax = std::minmax(mCarousel.zIndex, systemInfoZIndex);

  renderExtras(trans, INT16_MIN, minMax.first);

  if (mCarousel.zIndex > mSystemInfo.getZIndex()) renderInfoBar(trans);
  else renderCarousel(trans);

  renderExtras(trans, minMax.first, minMax.second);

  if (mCarousel.zIndex > mSystemInfo.getZIndex()) renderCarousel(trans);
  else renderInfoBar(trans);

  renderExtras(trans, minMax.second, INT16_MAX);
  renderFade(trans);
}

bool SystemView::CollectHelpItems(Help& help)
{
  help.Set(mCarousel.type == CarouselType::Vertical ? HelpType::UpDown : HelpType::LeftRight, _("CHOOSE"))
      .Set(Help::Valid(), _("SELECT"))
      .Set(HelpType::LR, _("MOVE 5 BY 5"));

  if (MenuFilter::ShouldEnableFeature(MenuFilter::Kodi) && RecalboxConf::Instance().GetKodiXButton() && MenuFilter::ShouldEnableFeature(MenuFilter::Kodi))
    help.Set(HelpType::X, RecalboxConf::Instance().GetNetplayEnabled() ? _("KODI/NETPLAY") : _("START KODI"));
  else if (RecalboxConf::Instance().GetNetplayEnabled() && MenuFilter::ShouldEnableFeature(MenuFilter::Netplay))
    help.Set(HelpType::X, _("NETPLAY"));

  if(MenuFilter::ShouldDisplayMenu(MenuFilter::Exit))
    help.Set(HelpType::Select, _("QUIT"));

  if(MenuFilter::ShouldDisplayMenu(MenuFilter::Main))
    help.Set(HelpType::Start, _("MENU"));

  if(MenuFilter::ShouldDisplayMenu(MenuFilter::Search))
    help.Set(HelpType::Y, _("SEARCH"));

  switch(RecalboxConf::Instance().GetScreenSaverType())
  {
    case RecalboxConf::Screensaver::Demo: help.Set(HelpType::HkY, _("demo")); break;
    case RecalboxConf::Screensaver::Gameclip: help.Set(HelpType::HkY, _("gameclip")); break;
    case RecalboxConf::Screensaver::Black:
    case RecalboxConf::Screensaver::Dim:
    case RecalboxConf::Screensaver::Suspend:
    default: break;
  }
  if (AudioScheduler::Instance().IsPlaying())
    help.Set(HelpType::HkR, _("SKIP MUSIC"));

  return true;
}

void SystemView::SwitchToTheme(const ThemeData& uselessTheme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)refreshOnly;
  (void)uselessTheme;

  // Refrehs view elements
  RefreshViewElements(ThemeManager::Instance().Main());

  // Refresh systems
  for(auto& entry : mEntries)
  {
    //DateTime start;
    const ThemeData& theme = ThemeManager::Instance().System(entry.object);
    SystemViewData& data = entry.data;

    // Reassign logo properties
    AssignSystemLogo(*entry.object, theme, entry);

    // Refresh extras
    data.backgroundExtras = std::make_shared<ThemeExtras>(mWindow);
    data.backgroundExtras->AssignExtras(theme, "system", theme.GetExtras("system", mWindow, nullptr), false);
    data.backgroundExtras->setSize(mSize);

    interface->ThemeSwitchTick();
  }

  SetNextSystem(mCurrentSystem); // Force refresh of curren system info
}

//  Get the ThemeElements that make up the SystemView.
void SystemView::RefreshViewElements(const ThemeData& theme)
{
  { LOG(LogDebug) << "[SystemView] Get View Elements"; }
  getDefaultElements();

  const ThemeElement* carouselElem = theme.Element("system", "systemcarousel", ThemeElementType::Carousel, ThemeElementType::None);
  getCarouselFromTheme(carouselElem);

  const ThemeElement* sysInfoElem = theme.Element("system", "systemInfo", ThemeElementType::Text, ThemeElementType::None);
  if (sysInfoElem != nullptr)
  {
    mSystemInfo.DoApplyThemeElement(theme, "system", "systemInfo", ThemePropertyCategory::All);
    if (!sysInfoElem->HasProperty(ThemePropertyName::Alignment))
      mSystemInfo.setAlignment(::Alignment::Center);
  }

  mViewNeedsReload = false;
}

//  Render system carousel
void SystemView::renderCarousel(const Transform4x4f& trans)
{
  if (mCarousel.textOnly) { renderTextList(trans); return; }

  // background box behind logos
  Transform4x4f carouselTrans = trans;
  carouselTrans.translate(Vector3f(mCarousel.pos.x(), mCarousel.pos.y(), 0.0));
  carouselTrans.translate(Vector3f(mCarousel.origin.x() * mCarousel.size.x() * -1, mCarousel.origin.y() * mCarousel.size.y() * -1, 0.0f));

  Vector2f clipPos(carouselTrans.translation().x(), carouselTrans.translation().y());
  Renderer::Instance().Clip(carouselTrans, mCarousel.size);

  Renderer::SetMatrix(carouselTrans);
  Renderer::DrawRectangle(0.0, 0.0, mCarousel.size.x(), mCarousel.size.y(), mCarousel.color);

  // draw logos
  Vector2f logoSpacing(0.0, 0.0); // NB: logoSpacing will include the size of the logo itself as well!
  float xOff = 0;
  float yOff = 0;

  switch (mCarousel.type)
  {
    case CarouselType::VerticalWheel:
    {
      yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2 - (mCamOffset * logoSpacing[1]);
      switch (AlignmentExtractHorizontal(mCarousel.logoAlignment))
      {
        case HorizontalAlignment::Left: xOff = mCarousel.logoSize.x() / 10; break;
        case HorizontalAlignment::Center: xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2;break;
        case HorizontalAlignment::Right: xOff = (float) (mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1));break;
      }
      break;
    }
    case CarouselType::Vertical:
    {
      logoSpacing[1] = ((mCarousel.size.y() - (mCarousel.logoSize.y() * (float) mCarousel.maxLogoCount)) /
                        (float) (mCarousel.maxLogoCount)) + mCarousel.logoSize.y();
      yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2 - (mCamOffset * logoSpacing[1]);
      switch (AlignmentExtractHorizontal(mCarousel.logoAlignment))
      {
        case HorizontalAlignment::Left: xOff = mCarousel.logoSize.x() / 10; break;
        case HorizontalAlignment::Center: xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2; break;
        case HorizontalAlignment::Right: xOff = (float) (mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1)); break;
      }
      break;
    }
    case CarouselType::Horizontal:
    {
      logoSpacing[0] = ((mCarousel.size.x() - (mCarousel.logoSize.x() * (float) mCarousel.maxLogoCount)) /
                        (float) (mCarousel.maxLogoCount)) + mCarousel.logoSize.x();
      xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2 - (mCamOffset * logoSpacing[0]);
      switch (AlignmentExtractVertical(mCarousel.logoAlignment))
      {
        case VerticalAlignment::Top: yOff = mCarousel.logoSize.y() / 10; break;
        case VerticalAlignment::Center: yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2; break;
        case VerticalAlignment::Bottom: yOff = (float) (mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1)); break;
      }
      break;
    }
    default: break;
  }

  int center = (int)(mCamOffset);
  int logoCount = Math::min(mCarousel.maxLogoCount, (int)mEntries.size());

  // Adding texture loading buffers depending on scrolling speed and status
  int bufferIndex = Math::absi(getScrollingVelocity());

  int bufferLeft = logoBuffersLeft[bufferIndex];
  int bufferRight = logoBuffersRight[bufferIndex];
  if (logoCount == 1)
  {
    bufferLeft = 0;
    bufferRight = 0;
  }

  for (int i = center - logoCount / 2 + bufferLeft; i <= center + logoCount / 2 + bufferRight; i++)
  {
    int index = i;
    while (index < 0)
      index += (int)mEntries.size();
    while (index >= (int)mEntries.size())
      index -= (int)mEntries.size();

    Transform4x4f logoTrans = carouselTrans;
    logoTrans.translate(Vector3f((float)i * logoSpacing[0] + xOff, (float)i * logoSpacing[1] + yOff, 0));

    float distance = (float)i - mCamOffset;

    float scale = 1.0f + ((mCarousel.logoScale - 1.0f) * (1.0f - fabs(distance)));
    scale = Math::min(mCarousel.logoScale, Math::max(1.0f, scale));
    scale /= mCarousel.logoScale;

    int opacity = Math::roundi(0x80 + ((0xFF - 0x80) * (1 - fabs(distance))));
    opacity = Math::max((int) 0x80, opacity);

    if (mEntries[index].data.logo)
    {
      const std::shared_ptr<Component>& comp = mEntries[index].data.logo;
      if (mCarousel.type == CarouselType::VerticalWheel)
      {
        comp->setRotationDegrees(mCarousel.logoRotation * distance);
        comp->setRotationOrigin(mCarousel.logoRotationOrigin);
      }
      comp->setScale(scale);
      comp->setOpacity(opacity);
      comp->Render(logoTrans);
    }

    if (mEntries[index].data.logotext)
    {
      const std::shared_ptr<Component> &comp2 = mEntries[index].data.logotext;
      if (mCarousel.type == CarouselType::VerticalWheel)
      {
        comp2->setRotationDegrees(mCarousel.logoRotation * distance);
        comp2->setRotationOrigin(mCarousel.logoRotationOrigin);
      }
      comp2->setScale(scale);
      comp2->setOpacity(opacity);
      comp2->Render(logoTrans);
    }
  }
  Renderer::Instance().Unclip();
}

int SystemView::wrapEntryIndex(int index) const
{
  int count = (int)mEntries.size();
  if (count <= 0) return 0;
  index %= count;
  return index < 0 ? index + count : index;
}

String SystemView::entryDisplayName(int index) const
{
  const String& name = mEntries[index].object->FullName();
  return mCarousel.forceUppercase ? name.ToUpperCaseUTF8() : name;
}

int SystemView::entryTextWidth(int index, const String& name)
{
  int& cached = mEntries[index].data.textWidth;
  if (cached < 0)
    cached = mCarousel.fontlarge->TextWidth(name);
  return cached;
}

float SystemView::textListAreaWidth() const
{
  return Math::max(0.0f, mCarousel.size.x() - mCarousel.horizontalMargin * 2.0f);
}

//  Render the systems as a text list, ROM-list style (selector bar + selected/primary colors)
void SystemView::renderTextList(const Transform4x4f& trans)
{
  Transform4x4f listTrans = trans;
  listTrans.translate(Vector3f(mCarousel.pos.x(), mCarousel.pos.y(), 0.0f));
  listTrans.translate(Vector3f(mCarousel.origin.x() * mCarousel.size.x() * -1, mCarousel.origin.y() * mCarousel.size.y() * -1, 0.0f));

  Renderer::SetMatrix(listTrans);
  Renderer::DrawRectangle(0.0f, 0.0f, mCarousel.size.x(), mCarousel.size.y(), mCarousel.color);

  float rowHeight = mCarousel.fontlarge->Height() * mCarousel.lineSpacing;
  if (rowHeight <= 0.0f) return;

  float screenCount = mCarousel.size.y() / rowHeight;

  // Use mCursor (integer) like TextListComponent — no smooth camera inertia in text-list mode
  float cursor = (float)mCursor;
  float startOffset = cursor - screenCount / 2.0f;
  startOffset = Math::max(0.0f, startOffset);
  if ((float)mEntries.size() > screenCount)
    startOffset = Math::min(startOffset, (float)mEntries.size() - screenCount);
  else
    startOffset = 0.0f;

  float selectorHeight = mCarousel.selectorHeight > 0.0f ? mCarousel.selectorHeight : rowHeight;
  float selectorX = mCarousel.selectorOffsetX;
  float selectorY = (cursor - startOffset) * rowHeight + mCarousel.selectorOffsetY;

  int firstRow = (int)startOffset;
  int visibleRows = (int)screenCount + 2;

  ::Alignment textAlign = AlignmentMergeVerticalAndHorizontal(VerticalAlignment::Center, AlignmentExtractHorizontal(mCarousel.logoAlignment));
  float leftMargin = mCarousel.horizontalMargin;
  float textAreaWidth = textListAreaWidth();

  Renderer::Instance().Clip(listTrans, mCarousel.size);

  // 1. Alternating row backgrounds (skip the selected row, covered by the selector bar)
  for (int i = firstRow; i <= firstRow + visibleRows; i++)
  {
    int index = wrapEntryIndex(i);
    if (index == mCursor) continue;
    float y = ((float)i - startOffset) * rowHeight;
    Renderer::DrawRectangle(0.0f, y, mCarousel.size.x(), rowHeight, (index & 1) != 0 ? sRowTintOdd : sRowTintEven);
  }

  // 2. Selector bar
  Renderer::DrawRectangle(selectorX, selectorY, mCarousel.size.x(), selectorHeight, mCarousel.selectorColor);

  // 3. Text entries (direct font rendering)
  FontEffect noEffect;
  for (int i = firstRow; i <= firstRow + visibleRows; i++)
  {
    int index = wrapEntryIndex(i);
    const String name = entryDisplayName(index);
    int textWidth = entryTextWidth(index, name);

    float y = ((float)i - startOffset) * rowHeight;
    bool selected = index == mCursor;
    unsigned int color = selected ? mCarousel.selectedColor
                       : ((index & 1) != 0 ? mCarousel.secondaryColor : mCarousel.primaryColor);

    // textOffsetX is a pure horizontal translation of the text (works at any alignment);
    // the marquee offset is added on top for the selected row.
    float xOffset = mCarousel.textOffsetX + (selected ? -(float)mMarqueeOffset : 0.0f);
    Rectangle textArea(leftMargin, y, textAreaWidth, rowHeight);
    mCarousel.fontlarge->DrawText(name, textArea, xOffset, 0.0f, color, textAlign, noEffect);

    // Marquee wrap-around copy
    if (selected && mMarqueeOffset > 0)
    {
      float wrapX = xOffset + (float)textWidth + (float)textWidth / 4.0f;
      mCarousel.fontlarge->DrawText(name, textArea, wrapX, 0.0f, color, textAlign, noEffect);
    }
  }

  Renderer::Instance().Unclip();

  // 4. Scrollbar (drawn outside clip, fades out like TextListComponent)
  if (mShowBar || mBarTimer >= 0)
  {
    int totalEntries = (int)mEntries.size();
    int screenCountInt = (int)screenCount;
    int maxOffset = totalEntries - screenCountInt;
    if (maxOffset > 0)
    {
      int verticalSize = (int)mCarousel.size.y() - 4;
      int barHeight = (int)(((float)verticalSize * screenCount) / (float)totalEntries);
      int screenWidth = Renderer::Instance().DisplayWidthAsInt();
      int barWidth = screenWidth > 1024 ? 6 : screenWidth > 480 ? 4 : 2;
      int yOffset = (int)((startOffset * (float)(verticalSize - barHeight)) / (float)maxOffset);
      int alpha = (mShowBar || mBarTimer > sBarFadeTime) ? 0x80 : (0x80 * mBarTimer) / sBarFadeTime;
      unsigned int barColor = (mCarousel.primaryColor & 0xFFFFFF00) | (unsigned int)alpha;
      Renderer::SetMatrix(listTrans);
      Renderer::DrawRectangle((int)mCarousel.size.x() - 2 - barWidth, 2 + yOffset, barWidth, barHeight, barColor);
    }
  }
}

void SystemView::renderInfoBar(const Transform4x4f& trans)
{
  Renderer::SetMatrix(trans);
  mSystemInfo.Render(trans);
}

bool SystemView::InputExtra(const InputCompactEvent& event)
{
  int extrasCenter = (int)mExtrasCamOffset;
  // Adding texture loading buffers depending on scrolling speed and status
  //int bufferIndex = Math::absi(getScrollingVelocity());
  for (int i = extrasCenter /*+ logoBuffersLeft[bufferIndex]*/; i <= extrasCenter /*+ logoBuffersRight[bufferIndex]*/; i++)
  {
    int index = i;
    while (index < 0) index += (int)mEntries.size();
    while (index >= (int)mEntries.size()) index -= (int)mEntries.size();
    //Only render selected system when not showing
    if (mShowing || index == mCursor)
    {
      SystemViewData& data = mEntries[index].data;
      for(ThemeExtras::Extra& extra : data.backgroundExtras->Extras())
        if (extra.Component().ProcessInput(event))
          return true;
    }
  }
  return false;
}

void SystemView::UpdateExtra(int deltaTime)
{
  int extrasCenter = (int)mExtrasCamOffset;
  // Adding texture loading buffers depending on scrolling speed and status
  int bufferIndex = Math::absi(getScrollingVelocity());
  for (int i = extrasCenter + logoBuffersLeft[bufferIndex]; i <= extrasCenter + logoBuffersRight[bufferIndex]; i++)
  {
    int index = i;
    while (index < 0) index += (int)mEntries.size();
    while (index >= (int)mEntries.size()) index -= (int)mEntries.size();
    //Only render selected system when not showing
    if (mShowing || index == mCursor)
    {
      SystemViewData& data = mEntries[index].data;
      for(ThemeExtras::Extra& extra : data.backgroundExtras->Extras())
        extra.Component().Update(deltaTime);
    }
  }
}

// Draw background extras
void SystemView::renderExtras(const Transform4x4f& trans, float lower, float upper)
{
  int extrasCenter = (int)mExtrasCamOffset;

  Renderer::Instance().ClipAbsolute(Rectangle(0.f, 0.f, mSize.x(), mSize.y()));

  // Adding texture loading buffers depending on scrolling speed and status
  int bufferIndex = Math::absi(getScrollingVelocity());

  for (int i = extrasCenter + logoBuffersLeft[bufferIndex]; i <= extrasCenter + logoBuffersRight[bufferIndex]; i++)
  {
    int index = i;
    while (index < 0) index += (int)mEntries.size();
    while (index >= (int)mEntries.size()) index -= (int)mEntries.size();

    //Only render selected system when not showing
    if (mShowing || index == mCursor)
    {
      Transform4x4f extrasTrans = trans;
      if (mCarousel.type == CarouselType::Horizontal)
        extrasTrans.translate(Vector3f(((float)i - mExtrasCamOffset) * mSize.x(), 0, 0));
      else
        extrasTrans.translate(Vector3f(0, ((float)i - mExtrasCamOffset) * mSize.y(), 0));

      Renderer::Instance().Clip(extrasTrans, mSize);
      SystemViewData& data = mEntries[index].data;
      for(ThemeExtras::Extra& extra : data.backgroundExtras->Extras())
        if (extra.Component().getZIndex() >= lower && extra.Component().getZIndex() < upper)
          extra.Component().Render(extrasTrans);
      Renderer::Instance().Unclip();
    }
  }
  Renderer::Instance().Unclip();
}

void SystemView::renderFade(const Transform4x4f& trans)
{
  // fade extras if necessary
  if (mExtrasFadeOpacity != 0.0f)
  {
      Renderer::SetMatrix(trans);
      Renderer::DrawRectangle(0.0f, 0.0f, mSize.x(), mSize.y(), 0x00000000 | (unsigned char)(mExtrasFadeOpacity * 255));
  }
}

// Populate the system carousel with the legacy values
void  SystemView::getDefaultElements()
{
  // Carousel
  mCarousel.type = CarouselType::Horizontal;
  mCarousel.logoAlignment = ::Alignment::Center;
  mCarousel.size.x() = mSize.x();
  mCarousel.size.y() = 0.2325f * mSize.y();
  mCarousel.pos.x() = 0.0f;
  mCarousel.pos.y() = 0.5f * (mSize.y() - mCarousel.size.y());
  mCarousel.origin.x() = 0.0f;
  mCarousel.origin.y() = 0.0f;
  mCarousel.color = 0xFFFFFFD8;
  mCarousel.logoScale = 1.2f;
  mCarousel.logoRotation = 7.5;
  mCarousel.logoRotationOrigin.x() = -5;
  mCarousel.logoRotationOrigin.y() = 0.5;
  mCarousel.logoSize.x() = 0.25f * (Math::max(mSize.y(), mSize.x()));
  mCarousel.logoSize.y() = 0.155f * (Math::min(mSize.y(), mSize.x()));
  mCarousel.textOnly = false;
  mCarousel.forceUppercase = false;
  mCarousel.selectorColor = 0xFFFFFF33;
  mCarousel.selectedColor = 0xFFFFFFFF;
  mCarousel.primaryColor = 0xE6E6E6FF;
  mCarousel.secondaryColor = 0xC6C6C6FF;
  mCarousel.selectorHeight = 0.0f;
  mCarousel.selectorOffsetX = 0.0f;
  mCarousel.selectorOffsetY = 0.0f;
  mCarousel.textOffsetX = 0.0f;
  mCarousel.lineSpacing = 1.5f;
  mCarousel.horizontalMargin = 0.0f;
  mCarousel.maxLogoCount = 3;
  mCarousel.zIndex = 40;
  mCarousel.fontlarge = nullptr;
  mCarousel.fontmedium = nullptr;

  // System Info Bar
  mSystemInfo.setSize(mSize.x(), mSystemInfo.getFont()->Height()*2.2f);
  mSystemInfo.setOrigin(0, 0);
  mSystemInfo.setPosition(0, (mCarousel.pos.y() + mCarousel.size.y() - 0.2f));
  mSystemInfo.setBackgroundColor(0xDDDDDDD8);
  mSystemInfo.setFont(&FontManager::Instance().FromDefault(Renderer::Instance().Is480pOrLower() ? FONT_SIZE_MEDIUM : FONT_SIZE_SMALL, false));
  mSystemInfo.setAlignment(::Alignment::Center);
  mSystemInfo.setColor(0x000000FF);
  mSystemInfo.setZIndex(50);
  mSystemInfo.setDefaultZIndex(50);
}

void SystemView::getCarouselFromTheme(const ThemeElement* elem)
{
  if (elem != nullptr)
  {
    if (elem->HasProperty(ThemePropertyName::Type))
    {
      String type = elem->AsString(ThemePropertyName::Type);
      if (type == "vertical") mCarousel.type = CarouselType::Vertical;
      else if (type == "vertical_wheel") mCarousel.type = CarouselType::VerticalWheel;
      else mCarousel.type = CarouselType::Horizontal;
    }
    if (elem->HasProperty(ThemePropertyName::Size)) mCarousel.size = elem->AsVector(ThemePropertyName::Size) * mSize;
    if (elem->HasProperty(ThemePropertyName::Pos)) mCarousel.pos = elem->AsVector(ThemePropertyName::Pos) * mSize;
    if (elem->HasProperty(ThemePropertyName::Origin)) mCarousel.origin = elem->AsVector(ThemePropertyName::Origin);
    if (elem->HasProperty(ThemePropertyName::Color)) mCarousel.color = (unsigned int) elem->AsInt(ThemePropertyName::Color);
    if (elem->HasProperty(ThemePropertyName::LogoScale)) mCarousel.logoScale = elem->AsFloat(ThemePropertyName::LogoScale);
    if (elem->HasProperty(ThemePropertyName::LogoSize)) mCarousel.logoSize = elem->AsVector(ThemePropertyName::LogoSize) * mSize;
    if (elem->HasProperty(ThemePropertyName::TextOnly)) mCarousel.textOnly = elem->AsBool(ThemePropertyName::TextOnly);
    if (elem->HasProperty(ThemePropertyName::ForceUppercase)) mCarousel.forceUppercase = elem->AsBool(ThemePropertyName::ForceUppercase);
    if (elem->HasProperty(ThemePropertyName::SelectorColor)) mCarousel.selectorColor = (unsigned int) elem->AsInt(ThemePropertyName::SelectorColor);
    if (elem->HasProperty(ThemePropertyName::SelectedColor)) mCarousel.selectedColor = (unsigned int) elem->AsInt(ThemePropertyName::SelectedColor);
    if (elem->HasProperty(ThemePropertyName::PrimaryColor)) mCarousel.primaryColor = (unsigned int) elem->AsInt(ThemePropertyName::PrimaryColor);
    if (elem->HasProperty(ThemePropertyName::SecondaryColor)) mCarousel.secondaryColor = (unsigned int) elem->AsInt(ThemePropertyName::SecondaryColor);
    if (elem->HasProperty(ThemePropertyName::SelectorHeight)) mCarousel.selectorHeight = elem->AsFloat(ThemePropertyName::SelectorHeight) * mSize.y();
    if (elem->HasProperty(ThemePropertyName::SelectorOffsetX)) mCarousel.selectorOffsetX = elem->AsFloat(ThemePropertyName::SelectorOffsetX) * mSize.x();
    if (elem->HasProperty(ThemePropertyName::SelectorOffsetY)) mCarousel.selectorOffsetY = elem->AsFloat(ThemePropertyName::SelectorOffsetY) * mSize.y();
    if (elem->HasProperty(ThemePropertyName::TextOffsetX)) mCarousel.textOffsetX = elem->AsFloat(ThemePropertyName::TextOffsetX) * mSize.x();
    if (elem->HasProperty(ThemePropertyName::LineSpacing)) mCarousel.lineSpacing = elem->AsFloat(ThemePropertyName::LineSpacing);
    if (elem->HasProperty(ThemePropertyName::HorizontalMargin)) mCarousel.horizontalMargin = elem->AsFloat(ThemePropertyName::HorizontalMargin) * mSize.x();
    if (elem->HasProperty(ThemePropertyName::MaxLogoCount)) mCarousel.maxLogoCount = Math::roundi(elem->AsFloat(ThemePropertyName::MaxLogoCount));
    if (elem->HasProperty(ThemePropertyName::ZIndex)) mCarousel.zIndex = elem->AsFloat(ThemePropertyName::ZIndex);
    if (elem->HasProperty(ThemePropertyName::LogoRotation)) mCarousel.logoRotation = elem->AsFloat(ThemePropertyName::LogoRotation);
    if (elem->HasProperty(ThemePropertyName::LogoRotationOrigin)) mCarousel.logoRotationOrigin = elem->AsVector(ThemePropertyName::LogoRotationOrigin);
    if (elem->HasProperty(ThemePropertyName::LogoAlignment)) mCarousel.logoAlignment = elem->AsAlignment(ThemePropertyName::LogoAlignment);
    mCarousel.fontlarge = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false));
    mCarousel.fontmedium = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false));
    if (elem->HasProperty(ThemePropertyName::FontColor)) mCarousel.fontcolor = (unsigned int) elem->AsInt(ThemePropertyName::FontColor);
    else if (elem->HasProperty(ThemePropertyName::Link)) mCarousel.fontcolor = (unsigned int) elem->AsInt(ThemePropertyName::Link);
    else mCarousel.fontcolor = 0xE6E6E6FF;
  }
  if (mCarousel.fontlarge == nullptr) mCarousel.fontlarge = &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false);
  if (mCarousel.fontmedium == nullptr) mCarousel.fontmedium = &FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false);
}

void SystemView::RemoveCurrentSystem()
{
  removeSystem(mCurrentSystem);
}

void SystemView::removeSystem(SystemData * system)
{
  SystemData* previousSystem = Prev();
  for(auto it = mEntries.begin(); it != mEntries.end(); ++it)
    if (it->object == system)
    {
      mEntries.erase(it);
      break;
    }
  goToSystem(system == mCurrentSystem ? previousSystem : mCurrentSystem, true);
}

void SystemView::manageSystemsList()
{
  std::vector<Entry> backupedEntries = mEntries;

  for (const auto& system : mSystemManager.AllSystems())
  {
    if(system->Descriptor().IsPort())
      continue;

    bool hasGame = system->HasVisibleGame();
    bool systemIsAlreadyVisible = LookupSystemByName(system->Name()) != nullptr;

    if(!systemIsAlreadyVisible && hasGame) addSystem(system);
    else if (systemIsAlreadyVisible && !hasGame) removeSystem(system);
  }

  if (mEntries.empty())
  {
    mEntries = backupedEntries;
    mWindow.displayMessage(_("Last operation removed all systems!\n\nThey have been restored to allow normal operations, regardless of the current filters."));
  }
}

SystemData* SystemView::LookupSystemByName(const String& name)
{
  for (auto& mEntrie : mEntries)
    if (mEntrie.object->Name() == name)
      return mEntrie.object;
  return nullptr;
}

void SystemView::SetNextSystem(const SystemData* system)
{
  Mutex::AutoLock locker(mSystemLocker);
  mSystemFromWitchToExtractData = system;
  mSystemSignal.Fire();
}

void SystemView::Run()
{
  while(IsRunning())
  {
    mSystemSignal.WaitSignal();
    if (!IsRunning()) return; // Destructor called :)
    int favorites = 0;
    int hidden = 0;
    int count = 0;
    for(bool working = true; working; )
    {
      mSystemLocker.Lock();
      const SystemData* system = mSystemFromWitchToExtractData;
      mSystemFromWitchToExtractData = nullptr;
      mSystemLocker.UnLock();
      if (system != nullptr)
      {
        count = system->GameCount(favorites, hidden);
        if (system->IsFavorite()) favorites = 0; // Favorite system display only favorites
      }
      else working = false;
    }
    if (count != 0)
      mSender.Send({ count, favorites, hidden });
  }
}

void SystemView::ReceiveSyncMessage(const SystemGameCount& data)
{
  //! Still on system list?
  if (ViewController::Instance().isViewing(ViewType::SystemList))
  {
    mSystemLocker.Lock();
    bool workToDoFirst = mSystemFromWitchToExtractData != nullptr;
    mSystemLocker.UnLock();
    if (workToDoFirst) return;

    // animate mSystemInfo's opacity (fade out, wait, fade back in)
    cancelAnimation(1);
    cancelAnimation(2);

    const float infoStartOpacity = (float)mSystemInfo.getOpacity() / 255.f;

    Animation* infoFadeOut = new LambdaAnimation([infoStartOpacity, this] (float t)
                                                 {
                                                   mSystemInfo.setOpacity((unsigned char)(lerp<float>(infoStartOpacity, 0.f, t) * 255));
                                                 }, (int)(infoStartOpacity * 150));

    // also change the text after we've fully faded out
    setAnimation(infoFadeOut, 0, [this, data]
    {
      String text(_N("%i GAME AVAILABLE", "%i GAMES AVAILABLE", data.VisibleGames));
      text.Replace("%i", String(data.VisibleGames));
      if (data.Favorites != 0)
        text.Append(", ").Append(_N("%i FAVORITE", "%i FAVORITES", data.Favorites))
            .Replace("%i", String(data.Favorites));
      mSystemInfo.setText(text);
    }, false, 1);

    Animation* infoFadeIn = new LambdaAnimation([this](float t)
                                                {
                                                  mSystemInfo.setOpacity((unsigned char)(lerp<float>(0.f, 1.f, t) * 255));
                                                }, 300);

    // wait ms to fade in
    setAnimation(infoFadeIn, 800, nullptr, false, 2);
  }
}


