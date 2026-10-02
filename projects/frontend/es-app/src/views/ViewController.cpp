#include <RecalboxConf.h>
#include "systems/SystemData.h"
#include "views/ViewController.h"

#include "views/gamelist/ArcadeGameListView.h"
#include "guis/GuiDetectDevice.h"
#include "animations/LaunchAnimation.h"
#include "animations/MoveCameraAnimation.h"
#include "animations/LambdaAnimation.h"

#include "RotationManager.h"
#include "guis/GuiSaveStates.h"
#include "guis/menus/FastMenuList.h"
#include "utils/Zip.h"
#include "utils/7z/SevenZipInspector.h"
#include "guis/menus/MenuProvider.h"

#include <MainRunner.h>
#include <bios/BiosManager.h>
#include <guis/GuiMsgBox.h>
#include <rendering/textures/TextureManager.h>

#include <games/GameFilesUtils.h>

ViewController::ViewController(WindowManager& window, SystemManager& systemManager, const IGlobalVariableResolver& resolver)
	: StaticLifeCycleControler<ViewController>("ViewController")
	, Gui(window)
  , mGameToLaunch(nullptr)
  , mLaunchCameraTarget()
  , mCheckFlags(LaunchCheckFlags::None)
  , mResolver(resolver)
	, mSystemManager(systemManager)
	, mCurrentView(&mSplashView)
  , mCurrentSystem(nullptr)
	, mSystemListView(window, systemManager, resolver)
  , mSplashView(window)
  , mCRConsoleView(window)
	, mGameClipView(window, systemManager)
  , mBouncingView(window)
  , mCrtView(window)
  , mCurrentViewType(ViewType::None)
  , mPreviousViewType(ViewType::None)
	, mCamera(Transform4x4f::Identity())
	, mFadeOpacity(0)
	, mLockInput(false)
  , mLastGameLaunched(0LL)
  , mFrequencyLastChoiceMulti60(0)
  , mFrequencyLastChoice(0)
  , mResolutionLastChoice(0)
  , mSignalLastChoice(0)
  , mSuperGameboyLastChoice(0)
  , mSoftPatchingPreviouslyUsed(false)
  , mSender(*this)
  , mNextItem(nullptr)
{

  // Set interfaces
  systemManager.SetProgressInterface(&mSplashView);
  systemManager.SetLoadingPhaseInterface(&mSplashView);
  systemManager.SetChangeNotifierInterface(this);
  // Splash
  ChangeView(ViewType::SplashScreen, nullptr);
  mSplashView.setPosition(0,0);

	// System View
  mSystemListView.setPosition(0, Renderer::Instance().DisplayHeightAsFloat());

  //! Centralized thread that process slow information for gamelists
  Thread::Start("GamelistSlow");

  // Let the texture memory guard tear down gamelist views as a last resort under memory pressure
  TextureManager::Instance().RegisterGamelistCacheControl(this);
}

ViewController::~ViewController()
{
  // Stop notifications from the texture memory guard before we tear our gamelist views down
  TextureManager::Instance().UnregisterGamelistCacheControl(this);

  // Stop gamelist thread
  Thread::Stop();

  for(const auto& view : mGameListViews)
    delete view.second;
}

void ViewController::goToStart()
{
  if (CardReader::Instance().ConsoleMode() && mCurrentViewType == ViewType::SplashScreen)
  {
    goToGameConsoleView();
    return;
  }

  CheckFilters();

  String systemName = RecalboxConf::Instance().GetStartupSelectedSystem();
  int index = systemName.empty() ? -1 : mSystemManager.getVisibleSystemIndex(systemName);
  SystemData* selectedSystem = index < 0 ? nullptr : mSystemManager.VisibleSystemList()[index];

  if ((selectedSystem == nullptr) || !selectedSystem->HasVisibleGame())
    selectedSystem = mSystemManager.FirstNonEmptySystem();

  if (selectedSystem == nullptr)
  {
    mWindow.pushGui(new GuiMsgBox(mWindow, _("Your filters preferences hide all your games !\nThe filters will be reset and recalbox will be reloaded."), _("OK"), [] { ResetFilters();}));
    return;
  }
  if (RecalboxConf::Instance().GetStartupHideSystemView())
    goToGameList(selectedSystem);
  else
  {
    if (RecalboxConf::Instance().GetStartupStartOnGamelist())
      goToGameList(selectedSystem);
    else
    {
      mSystemListView.SetProgressInterface(&mSplashView);
      mSystemListView.populate();
      mSystemListView.SetProgressInterface(nullptr);
      goToSystemView(selectedSystem);
    }
  }
}

bool ViewController::CheckFilters()
{

  if(mSystemManager.FirstNonEmptySystem() == nullptr)
  {
    ResetFilters();
    mWindow.pushGui(new GuiMsgBox(mWindow, _("Your filters preferences hide all your games !\nAll filters have been reset."), _("OK"), []{}));
    return false;
  }
  return true;
}

void ViewController::ResetFilters()
{
  RecalboxConf& conf = RecalboxConf::Instance();

  conf.SetShowOnlyLatestVersion(false);
  conf.SetFavoritesOnly(false);
  conf.SetGlobalHidePreinstalled(false);
  conf.SetHideNoGames(false);
  conf.SetFilterAdultGames(false);

  conf.SetCollectionPorts(true);
}

void ViewController::goToQuitScreen()
{
  mSplashView.Quit();
  ChangeView(ViewType::SplashScreen, nullptr);
  mCamera.translation().Set(0,0,0);
}

void ViewController::goToSystemView(SystemData* system)
{
  CheckFilters();

  if (!system->HasVisibleGame()) system = mSystemManager.FirstNonEmptySystem();

  float position = ReorderGamelistViewBeforeMoving(system, Move::None);
  mSystemListView.setPosition(position, mSystemListView.getPosition().y());
  mCamera.translation().x() = -position;

  ChangeView(ViewType::SystemList, system);

	playViewTransition();

  NotificationManager::Instance().Notify(*system, Notification::SystemBrowsing);
}

void ViewController::goToGameConsoleView()
{
  if (mCurrentViewType == ViewType::CardReaderConsoleView) return; // #TODO: avoid ths method being called in loop
  ChangeView(ViewType::CardReaderConsoleView, nullptr);
}


void ViewController::goToGameClipView()
{
  if (mCurrentViewType == ViewType::GameClip) return; // #TODO: avoid ths method being called in loop

  ChangeView(ViewType::GameClip, nullptr);
  NotificationManager::Instance().Notify(Notification::StartGameClip);
  mGameClipView.Reset();
}

void ViewController::goToBouncingView()
{
  if (mCurrentViewType == ViewType::Bouncing) return; // #TODO: avoid the method being called in loop

  ChangeView(ViewType::Bouncing, nullptr);
  mBouncingView.Reset();
}

void ViewController::goToCrtView(CrtCalibrationView::CalibrationType screenType)
{
  ChangeView(ViewType::CrtCalibration, nullptr);
  mCrtView.Initialize(screenType);
}

void ViewController::selectGamelistAndCursor(FileData *file)
{
  SystemData& system = file->System();
  ReorderGamelistViewBeforeMoving(&system, Move::None);
  goToGameList(&system);
  ISimpleGameListView* view = GetOrCreateGamelistView(&system);
  view->Gamelist().RebuildFolderStack(file);
  if (view->Gamelist().LookupGameIndex(file) == -1)
  {
    view->populateList(*file->Parent(), file);
  }
  else
  {
    view->Gamelist().SetSelectedGame(file, true);
  }
}

SystemData* ViewController::goToNextGameList()
{
	assert(mCurrentViewType == ViewType::GameList);
	SystemData* system = mCurrentSystem;
	assert(system);

  CheckFilters();
  SystemData* next = mSystemManager.NextVisible(system);
	while(!next->HasVisibleGame())
    next = mSystemManager.NextVisible(next);

  AudioScheduler::Instance().SetTheme(next->Theme()).PlayNextMusic();
  ReorderGamelistViewBeforeMoving(next, Move::Right);
  goToGameList(next);

  return next;
}

void ViewController::goToPrevGameList()
{
	assert(mCurrentViewType == ViewType::GameList);
	SystemData* system = mCurrentSystem;
	assert(system);

  CheckFilters();
  SystemData* prev = mSystemManager.PreviousVisible(system);
	while(!prev->HasVisibleGame()) {
		prev = mSystemManager.PreviousVisible(prev);
	}

  AudioScheduler::Instance().SetTheme(prev->Theme()).PlayNextMusic();
  ReorderGamelistViewBeforeMoving(prev, Move::Left);
  goToGameList(prev);
}

float ViewController::ReorderGamelistViewBeforeMoving(SystemData* target, Move move)
{
  // If we move left to the last system, let introduce an offet
  int currentIndex = mSystemManager.SystemVisibleIndex(mCurrentSystem);
  int targetIndex = mSystemManager.SystemVisibleIndex(target);
  int offset = 0;
  if (move == Move::Left && currentIndex < targetIndex) offset = 1;

  // Move every single gamelist to its position regading visible system list
  ISimpleGameListView* gamelistView = GetOrCreateGamelistView(target);
  float screenWidth = Renderer::Instance().DisplayWidthAsFloat();
  float screenHeight = Renderer::Instance().DisplayHeightAsFloat();
  for(auto& kv : mGameListViews)
  {
    int position = mSystemManager.SystemVisibleIndex(kv.first) + offset;
    kv.second->setPosition((float)position * screenWidth, screenHeight * 2.f);
    if (target == mCurrentSystem)
      mCamera.translation().x() = (float)position * screenWidth;
  }

  switch(move)
  {
    case Left: { if (offset != 0) gamelistView->setPosition(0, gamelistView->getPosition().y()); break; }
    case Right: { if (targetIndex < currentIndex) gamelistView->setPosition((float)mSystemManager.VisibleSystemList().Count() * screenWidth, gamelistView->getPosition().y()); break; }
    case None:
    default: break;
  }

  return gamelistView->getPosition().x();
}

void ViewController::goToGameList(SystemData* system)
{
	RotationType rotation = RotationType::None;
	if(system->Rotatable() && RotationManager::ShouldRotateTateEnter(rotation))
	{
		mWindow.Rotate(rotation);
	}
	if (mCurrentViewType != ViewType::GameList)
	{
    // move system list
    float position = ReorderGamelistViewBeforeMoving(system, Move::None);
    mSystemListView.setPosition(position, mSystemListView.getPosition().y());
    mCamera.translation().x() = -position;
	}

	if (mInvalidGameList[system])
	{
		mInvalidGameList[system] = false;
    if (!ForceGamelistViewRecreation(system))
    {
      // if listview has been reload due to last game has been deleted,
      // we have to stop the previous goToGameList process because current
      // system will no longer exists in the available list
      return;
    }
	}

  ChangeView(ViewType::GameList, system);
	playViewTransition();

  ISimpleGameListView* gamelistView = GetOrCreateGamelistView(system);
  NotificationManager::Instance().Notify(*gamelistView->Gamelist().GetSelectedGame(), Notification::GamelistBrowsing);
  // for reload cursor video path if present
  gamelistView->DoUpdateGameInformation(false);
}

void ViewController::playViewTransition()
{
	Vector3f target = mCurrentView->getPosition();

	// no need to animate, we're not going anywhere (probably goToNextGamelist() or goToPrevGamelist() when there's only 1 system)
	if(target == -mCamera.translation() && !isAnimationPlaying(0))
		return;

  RecalboxConf::Transition transitionTheme = ThemeManager::Instance().Main().getThemeTransition();
	if(transitionTheme == RecalboxConf::Transition::Fade)
	{
		// fade
		// stop whatever's currently playing, leaving mFadeOpacity wherever it is
		cancelAnimation(0);

		auto fadeFunc = [this](float t) {
			mFadeOpacity = lerp<float>(0.f, 1.f, t);
		};

		const static int FADE_DURATION = 240; // fade in/out time
		const static int FADE_WAIT = 320; // time to wait between in/out
		setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), 0, [this, fadeFunc, target] {
			this->mCamera.translation() = -target;
      UpdateHelpBar();
			setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), FADE_WAIT, nullptr, true);
		});

		// fast-forward animation if we're partway faded
		if(target == -mCamera.translation())
		{
			// not changing screens, so cancel the first half entirely
			advanceAnimation(0, FADE_DURATION);
			advanceAnimation(0, FADE_WAIT);
			advanceAnimation(0, FADE_DURATION - (int)(mFadeOpacity * FADE_DURATION));
		}else{
			advanceAnimation(0, (int)(mFadeOpacity * FADE_DURATION));
		}
	} else if (transitionTheme == RecalboxConf::Transition::Slide){
		// slide or simple slide
		setAnimation(new MoveCameraAnimation(mCamera, target));
    UpdateHelpBar(); // update help prompts immediately
	} else {
		// instant
		setAnimation(new LambdaAnimation(
				[this, target](float t)
		{
      (void)t;
		  this->mCamera.translation() = -target;
		}, 1));
    UpdateHelpBar();
	}
}

void ViewController::Launch(FileData* game, const GameLinkedData& data, const Vector3f& cameraTarget, bool forceGoToGame)
{
  // Avoid launch repeat
  DateTime now;
  if ((now - mLastGameLaunched).TotalMilliseconds() < 2000) return;
  mLastGameLaunched = now;

  if (!game->IsGame())
  {
    { LOG(LogError) << "[ViewController] Tried to launch something that isn't a game"; }
    return;
  }

  if (forceGoToGame)
    selectGamelistAndCursor(game);

  mGameLinkedData = data;
  mGameToLaunch = game;
  mLaunchCameraTarget = cameraTarget;
  mCheckFlags = LaunchCheckFlags::None;
  LaunchCheck();
}

void ViewController::FastMenuLineSelected(int menuIndex, int itemIndex)
{
  if (itemIndex == -1)
    return;
  switch((FastMenuType)menuIndex)
  {
    case FastMenuType::Frequencies:
    {
      static CrtData::CrtVideoStandard videoStandards[] =
      {
        CrtData::CrtVideoStandard::AUTO,
        CrtData::CrtVideoStandard::NTSC,
        CrtData::CrtVideoStandard::PAL,
      };
      mGameLinkedData.ConfigurableCrt().ConfigureRegion(CrtData::CrtRegion::AUTO);
      if ((unsigned int)itemIndex < sizeof(videoStandards))
        mGameLinkedData.ConfigurableCrt().ConfigureVideoStandard(videoStandards[itemIndex]);
      else { LOG(LogError) << "[ViewController] Unprocessed fast menu item!"; return; }
      LaunchCheck();
      mFrequencyLastChoice = itemIndex;
      break;
    }
    case FastMenuType::FrequenciesMulti60:
    {
      static CrtData::CrtRegion regions[] =
      {
        CrtData::CrtRegion::AUTO,
        CrtData::CrtRegion::US,
        CrtData::CrtRegion::JP,
        CrtData::CrtRegion::EU,
      };
      static CrtData::CrtVideoStandard videoStandards[sizeof(regions)] =
      {
        CrtData::CrtVideoStandard::AUTO,
        CrtData::CrtVideoStandard::NTSC,
        CrtData::CrtVideoStandard::NTSC,
        CrtData::CrtVideoStandard::PAL,
      };
      if ((unsigned int)itemIndex < sizeof(regions))
      {
        mGameLinkedData.ConfigurableCrt().ConfigureRegion(regions[itemIndex]);
        mGameLinkedData.ConfigurableCrt().ConfigureVideoStandard(videoStandards[itemIndex]);
      }
      else { LOG(LogError) << "[ViewController] Unprocessed fast menu item!"; return; }
      LaunchCheck();
      mFrequencyLastChoiceMulti60 = itemIndex;
      break;
    }
    case FastMenuType::CrtResolution:
    {
      CrtData::CrtMode res = CrtData::CrtMode::Auto;
      ICrtInterface& board = Board::Instance().CrtBoard();
      EmulatorData emulator = EmulatorManager::GetGameEmulator(*mGameToLaunch);
      if(board.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31)
      {
        res = itemIndex == 0 ? CrtData::CrtMode::DoubleFreq : CrtData::CrtMode::Force480;
      }
      if(board.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzMulti1531)
      {
        if (mGameLinkedData.Crt().GameIsHD(*mGameToLaunch, emulator))
        {
          static CrtData::CrtMode KHzMulti1531HDmodes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force480, CrtData::CrtMode::Force480i
          };
          res = KHzMulti1531HDmodes[itemIndex];
        }
        else
        {
          static CrtData::CrtMode KHzMulti1531modes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force480
          };
          res = KHzMulti1531modes[itemIndex];
        }
      }
      if(board.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzTriFreq)
      {
        if (mGameLinkedData.Crt().GameIsHD(*mGameToLaunch, emulator))
        {
          static CrtData::CrtMode KHzTriFreqHdmodes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force384p, CrtData::CrtMode::Force480, CrtData::CrtMode::Force480i
          };
          res = KHzTriFreqHdmodes[itemIndex];
        }
        else
        {
          static CrtData::CrtMode KHzTriFreqmodes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force480
          };
          res = KHzTriFreqmodes[itemIndex];
        }
      }

      if(board.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzMulti1525)
      {
          static CrtData::CrtMode KHz15modes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force384p, CrtData::CrtMode::Force480
          };
          res = KHz15modes[itemIndex];
      }
      if(board.GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15)
      {
        static CrtData::CrtMode KHz15modes[] =
          {
            CrtData::CrtMode::Auto, CrtData::CrtMode::Force240, CrtData::CrtMode::Force480
          };
        res = KHz15modes[itemIndex];
      }
      mGameLinkedData.ConfigurableCrt().ConfigureForceResolution(res);
      LaunchCheck();
      mResolutionLastChoice = itemIndex;
      break;
    }
    case FastMenuType::CrtSignal:
    {
      CrtData::CrtSignal res = CrtData::CrtSignal::RGB;
      res = itemIndex == 0 ? CrtData::CrtSignal::RGB : CrtData::CrtSignal::Composite;
      mGameLinkedData.ConfigurableCrt().ConfigureForceSignal(res);
      LaunchCheck();
      mSignalLastChoice = itemIndex;
      break;
    }
    case FastMenuType::SuperGameboy:
    {
      mGameLinkedData.ConfigurableSuperGameBoy().Enable(itemIndex != 0);
      LaunchCheck();
      mSuperGameboyLastChoice = itemIndex;
    }
  }
}

void ViewController::SaveStateSlotSelected(int slot)
{
  mGameLinkedData.ConfigurableSaveState().SetSlotNumber(slot);
  LaunchCheck();
}

void ViewController::SoftPathingDisabled()
{
  mGameLinkedData.ConfigurablePatch().SetDisabledSoftPatching(true);
  mSoftPatchingPreviouslyUsed = false;
  LaunchCheck();
}

void ViewController::SoftPatchingSelected(const Path& path)
{
  mGameLinkedData.ConfigurablePatch().SetPatchPath(path);
  mSoftPatchingPreviouslyUsed = true;
  LaunchCheck();
}

bool ViewController::CheckBios(const EmulatorData& emulator)
{
  // Check bios
  if ((mCheckFlags & LaunchCheckFlags::Bios) == 0)
  {
    mCheckFlags |= LaunchCheckFlags::Bios;
    const BiosList& biosList = BiosManager::Instance().SystemBios(mGameToLaunch->System().Name());
    if (biosList.HasBiosKoFor(emulator.Emulator(), emulator.Core()))
    {
      // Build emulator name
      String emulatorString = emulator.Emulator();
      if (emulator.Emulator() != emulator.Core()) emulatorString.Append('/').Append(emulator.Core());
      // Build text
      String text = _("At least one mandatory BIOS is missing for %emulator%!\nYour game '%game%' will very likely not run at all until required BIOS are put in the expected folder.\n\nDo you want to launch the game anyway?")
        .Replace("%emulator%", emulatorString)
        .Replace("%game%", mGameToLaunch->Name());
      // Show the dialog box
      Gui* gui = new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr);
      mWindow.pushGui(gui);
      return true;
    }
  }
  return false;
}

bool ViewController::CheckExtensions(const EmulatorData& emulator)
{
  // Extension check
  if ((mCheckFlags & LaunchCheckFlags::Extension) == 0)
  {
    mCheckFlags |= LaunchCheckFlags::Extension;
    String ext = mGameToLaunch->Metadata().RomFileOnly().Extension().LowerCase();
    if (emulator.CoreInfo().Extensions().StartsWith("files:"))
    {
      String extensions(emulator.CoreInfo().Extensions()); extensions.Remove("files:");
      String executable = mGameToLaunch->Metadata().RomFileOnly().Filename();
      if (!extensions.Contains(executable))
      {
        String text = (_F(_("The emulator selected to launch {0} does not support file '{1}'. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name() / executable).ToString();
        mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
        return true;
      }
    }
    else if (ext == ".zip")
    {
      ext.Insert(0, ' ').Append(' ');
      String extensions(' '); extensions.Append(emulator.CoreInfo().Extensions()).Append(' ');
      // zip support?
      if (!extensions.Contains(ext))
      {
        String text = (_F(_("The emulator selected to launch {0} does not support zipped files/roms. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name()).ToString();
        mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
        return true;
      }
      // Check extension in the zip itself
      if (!mGameToLaunch->System().IsArcade())
      {
        extensions = emulator.CoreInfo().FileExtensions();
        if (!extensions.empty()) extensions.Insert(0, ' ').Append(' ');
        Zip zip(mGameToLaunch->RomPath());
        bool compatible = extensions.empty(); // No extension to check == compatible
        for (int i = zip.Count(); --i >= 0;)
          if (extensions.Contains(String(' ').Append(zip.FileName(i).Extension().LowerCase()).Append(' ')))
            compatible = true;
        if (!compatible)
        {
          String text = (_F(_("This zipped game does not contain any file supported by the selected emulator. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name()).ToString();
          mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this]{ LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
          return true;
        }
      }
    }
    else if (ext == ".7z")
    {
      ext.Insert(0, ' ').Append(' ');
      String extensions(' '); extensions.Append(emulator.CoreInfo().Extensions()).Append(' ');
      // zip support?
      if (!extensions.Contains(ext))
      {
        String text = (_F(_("The emulator selected to launch {0} does not support 7zipped files/roms. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name()).ToString();
        mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
        return true;
      }
      // Check extension in the zip itself
      if (!mGameToLaunch->System().IsArcade())
      {
        extensions = emulator.CoreInfo().FileExtensions();
        if (!extensions.empty()) extensions.Insert(0, ' ').Append(' ');
        SevenZipInspector zip(mGameToLaunch->RomPath());
        bool compatible = extensions.empty(); // No extension to check == compatible
        for(int i = (int)zip.AllFiles().size(); --i >= 0;)
          if (String extFile = String(' ').Append(zip.AllFiles()[i].Extension().LowerCase()).Append(' ');
            extensions.Contains(extFile))
            compatible = true;
        if (!compatible)
        {
          String text = (_F(_("This 7zipped game does not contain any file supported by the selected emulator. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name()).ToString();
          mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
          return true;
        }
      }
    }
    else
    {
      ext.Insert(0, ' ').Append(' ');
      String extensions(' '); extensions.Append(emulator.CoreInfo().Extensions()).Append(' ');
      if (!extensions.Contains(ext))
      {
        String text = (_F(_("The emulator selected to launch {0} does not support file extension '{1}'. Would you like to run the game anyway, even though there's a high chance it will not work?")) / mGameToLaunch->Name() / ext).ToString();
        mWindow.pushGui((new GuiMsgBox(mWindow, text, _("YES"), [this] { LaunchCheck(); }, _("NO"), nullptr))->SetDefaultButton(1));
        return true;
      }
    }
  }

  return false;
}

bool ViewController::CheckRefreshRate()
{
  if ((mCheckFlags & LaunchCheckFlags::Frequency) == 0)
    if (mCheckFlags |= LaunchCheckFlags::Frequency; mGameLinkedData.Crt().IsRegionOrStandardConfigured())
      if (mGameLinkedData.Crt().MustChoosePALorNTSC(mGameToLaunch->System()))
      {
        if (mGameToLaunch->System().Name() == "megadrive")
          mWindow.pushGui(new FastMenuList(mWindow, this, _("Game refresh rate"), mGameToLaunch->Name(),
                                           (int) FastMenuType::FrequenciesMulti60,
                                           {{_("AUTO")},
                                            {_("60Hz (US)")},
                                            {_("60Hz (JP)")},
                                            {_("50Hz (EU)")}}, mFrequencyLastChoiceMulti60));
        else
          mWindow.pushGui(new FastMenuList(mWindow, this, _("Game refresh rate"), mGameToLaunch->Name(),
                                           (int) FastMenuType::Frequencies,
                                           {{_("AUTO")},
                                            {"60Hz"},
                                            {"50Hz"}}, mFrequencyLastChoice));
        return true;
      }
  return false;
}

bool ViewController::CheckCRTSignalType()
{
  // CRT Signal Type choice
  if ((mCheckFlags & LaunchCheckFlags::CrtSignal) == 0)
  {
    if (mCheckFlags |= LaunchCheckFlags::CrtSignal; mGameLinkedData.Crt().IsForceSignalSelectionConfigured())
    {
      if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15 &&
          Board::Instance().CrtBoard().HasCompositeSupport() && mGameLinkedData.Crt().MustChooseCRTSignal())
      {
        mWindow.pushGui(new FastMenuList(mWindow, this, _("Signal"), mGameToLaunch->Name(),
                                         (int) FastMenuType::CrtSignal,
                                         {{"RGB (SCART)"},{"Composite (SCART/RCA/JACK)"}}, mSignalLastChoice));
        return true;
      }
    }
  }
  return false;
}

bool ViewController::CheckCRTResolution(const EmulatorData& emulator)
{
  // CRT Resolution choice
  if ((mCheckFlags & LaunchCheckFlags::CrtResolution) == 0)
  {
    if (mCheckFlags |= LaunchCheckFlags::CrtResolution; mGameLinkedData.Crt().IsForceResolutionSelectionConfigured())
    {
      //const bool is31kHz = Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31;
      if (mGameLinkedData.Crt().MustChooseResolution(mGameToLaunch, emulator))
      {
        if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31)
        {
          mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                              (int) FastMenuType::CrtResolution,
                                              {{"240p@120"},{"480p"}}, mResolutionLastChoice));
        }
        if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzMulti1531)
        {
          if (mGameLinkedData.Crt().GameIsHD(*mGameToLaunch, emulator))
            mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                            (int) FastMenuType::CrtResolution,
                                            {{"AUTO"}, {"240p"}, {"480p"}, {"480i"}}, mResolutionLastChoice));
          else
            mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                            (int) FastMenuType::CrtResolution,
                                            {{"AUTO"}, {"240p"}, {"480p"}}, mResolutionLastChoice));
        }
        if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzTriFreq)
        {
          if (mGameLinkedData.Crt().GameIsHD(*mGameToLaunch, emulator))
            mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                              (int) FastMenuType::CrtResolution,
                                              {{"AUTO"}, {"240p"}, {"384p"}, {"480p"}, {"480i"}}, mResolutionLastChoice));
          else
            mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                              (int) FastMenuType::CrtResolution,
                                              {{"AUTO"}, {"240p"}, {"480p"}}, mResolutionLastChoice));
        }
        if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHzMulti1525)
        {
          if (mGameLinkedData.Crt().GameIsHD(*mGameToLaunch, emulator))
            mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                                (int) FastMenuType::CrtResolution,
                                                {{"AUTO"}, {"240p"}, {"384p"}, {"480i"}}, mResolutionLastChoice));
        }
        if(Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz15)
        {
          mWindow.pushGui(new FastMenuList(mWindow, this, _("Game resolution"), mGameToLaunch->Name(),
                                              (int) FastMenuType::CrtResolution,
                                              {{"AUTO"}, {"240p"}, {"480i"}}, mResolutionLastChoice));
        }
        return true;
      }
    }
  }
  return false;
}

bool ViewController::CheckSoftPatching(const EmulatorData& emulator)
{
  if ((mCheckFlags & LaunchCheckFlags::SoftPatching) == 0)
  {
    mCheckFlags |= LaunchCheckFlags::SoftPatching;
    if (bool coreIsSoftpatching = mGameToLaunch->System().Descriptor().IsSoftpatching(emulator.Emulator(), emulator.Core()); coreIsSoftpatching)
      switch(RecalboxConf::Instance().GetGlobalSoftpatching())
      {
        case RecalboxConf::SoftPatching::Auto:
        {
          if (!GameFilesUtils::HasAutoPatch(mGameToLaunch))
          {
            Path priorityPath = GameFilesUtils::GetSubDirPriorityPatch(mGameToLaunch);
            if (!priorityPath.IsEmpty() && priorityPath.Exists())
              mGameLinkedData.ConfigurablePatch().SetPatchPath(priorityPath);
          }
          break;
        }
        case RecalboxConf::SoftPatching::LaunchLast:
        {
          Path lastPatchPath = mGameToLaunch->Metadata().LastPatch();

          if (lastPatchPath.Exists())
          {
            mGameLinkedData.ConfigurablePatch().SetPatchPath(lastPatchPath);
            break;
          }
          if (lastPatchPath.Filename() == "original")
          {
            mGameLinkedData.ConfigurablePatch().SetDisabledSoftPatching(true);
            break;
          }
          // if no patch are configured yet go to next SoftPatching::Select case
          [[fallthrough]];
        }
        case RecalboxConf::SoftPatching::Select:
        {
          std::vector<Path> patches = GameFilesUtils::GetSoftPatches(mGameToLaunch);
          if (!mGameLinkedData.ConfigurablePatch().IsConfigured() && !patches.empty())
          {
            //mWindow.pushGui(new MenuSoftpatchingLauncher(mWindow, *mGameToLaunch, std::move(patches), this));
            MenuProvider::ShowMenu(MenuContainerType::SelectSoftpatch, InheritableContext(&mGameToLaunch->System(), mGameToLaunch, this));
            return true;
          }
          break;
        }
        case RecalboxConf::SoftPatching::Disable:
        default:
        {
          mGameLinkedData.ConfigurablePatch().SetDisabledSoftPatching(true);
          break;
        }
      }
  }

  // No soft patching
  return false;
}

bool ViewController::CheckSuperGameboy()
{
  // SuperGameBoy choice
  if ((mCheckFlags & LaunchCheckFlags::SuperGameboy) == 0)
    if(mCheckFlags |= LaunchCheckFlags::SuperGameboy; mGameLinkedData.SuperGameBoy().ShouldAskForSuperGameBoy(mGameToLaunch->System()))
    {
      mWindow.pushGui(new FastMenuList(mWindow, this, _("GameBoy Mode"), mGameToLaunch->Name(), (int)FastMenuType::SuperGameboy,
                                       { { "GameBoy", _("Start the game standard Game Boy mode") }, { "SuperGameBoy", _("Start the game in Super Game Boy mode") } }, mSuperGameboyLastChoice));
      return true;
    }
  return false;
}

bool ViewController::CheckSaveStates(const EmulatorData& emulator)
{
  // Save state slot
  if ((mCheckFlags & LaunchCheckFlags::SaveState) == 0)
    if (mCheckFlags |= LaunchCheckFlags::SaveState; emulator.IsLibretro() && RecalboxConf::Instance().GetGlobalShowSaveStateBeforeRun())
      if (!GameFilesUtils::GetGameSaveStateFiles(*mGameToLaunch).empty())
      {
        mWindow.pushGui(new GuiSaveStates(mWindow, mSystemManager, *mGameToLaunch, this, false));
        return true;
      }
  return false;
}

void ViewController::LaunchCheck()
{
  EmulatorData emulator = EmulatorManager::GetGameEmulator(*mGameToLaunch);
  if (!emulator.IsValid())
  {
    { LOG(LogError) << "[ViewController] Empty emulator/core when running " << mGameToLaunch->RomPath().ToString() << '!'; }
    return;
  }

  // Check BIOS
  if (CheckBios(emulator)) return;
  // Check extension
  if (CheckExtensions(emulator)) return;


  // Signal type choice ?
  if (CheckCRTSignalType()) return;
  // Refresh rate choice ?
  if (CheckRefreshRate()) return;
  // CRT Resolution choice ?
  if (CheckCRTResolution(emulator)) return;
  // Softpatching choice ?
  if (CheckSoftPatching(emulator)) return;
  // GB/SGB Choice ?
  if (CheckSuperGameboy()) return;
  // Save state choice ?
  if (CheckSaveStates(emulator)) return;


  LaunchAnimated(emulator);
}

void ViewController::LaunchActually(const EmulatorData& emulator)
{
  mFlagCaches.Clear();

  DateTime start;
  ResolutionAdapter adapter;
  GameRunner::Instance().RunGame(adapter, *mGameToLaunch, emulator, mGameLinkedData);
  TimeSpan elapsed = DateTime() - start;

  if (elapsed.TotalMilliseconds() <= 3000) // 3s
  {
    // Build text
    String text = _("It seems that your game didn't start at all!\n\nIt's most likely due to either:\n- bad rom\n- missing/bad mandatory bios files\n- missing/bad optional BIOS files (but required for this very game)");
    // Show the dialog box
    Gui* gui = new GuiMsgBox(mWindow, text, _("OK"), ::Alignment::CenterLeft);
    mWindow.pushGui(gui);
  }

  // Callback gamelist
  if (mCurrentViewType == ViewType::GameList)
    ((ISimpleGameListView*)mCurrentView)->ReturnedFromGame(mGameToLaunch);

  // Re-sort last played system if it exists
  mSystemManager.UpdateSystemsOnGameChange(mGameToLaunch, MetadataType::LastPlayed, false);
}

void ViewController::LaunchAnimated(const EmulatorData& emulator)
{
  Vector3f center = mLaunchCameraTarget.isZero() ? Vector3f(Renderer::Instance().DisplayWidthAsFloat() / 2.0f, Renderer::Instance().DisplayHeightAsFloat() / 2.0f, 0) : mLaunchCameraTarget;

	Transform4x4f origCamera = mCamera;
	origCamera.translation() = -mCurrentView->getPosition();

	center += mCurrentView->getPosition();
	stopAnimation(1); // make sure the fade in isn't still playing
	mLockInput = true;

  RecalboxConf::Transition transitionTheme = RecalboxConf::Instance().GetGameTransition();

	auto launchFactory = [this, origCamera, &emulator] (const std::function<void(std::function<void()>)>& backAnimation)
	{
		return [this, origCamera, backAnimation, emulator]
		{
		  LaunchActually(emulator);

      mCamera = origCamera;
			backAnimation([this] { mLockInput = false; });
      if (mCurrentViewType == ViewType::GameList)
        ((ISimpleGameListView*)mCurrentView)->DoUpdateGameInformation(true);
		};
	};

	if(transitionTheme == RecalboxConf::Transition::Fade)
	{
		// fade out, launch game, fade back in
		auto fadeFunc = [this](float t) {
			mFadeOpacity = lerp<float>(0.0f, 1.0f, t);
		};
		setAnimation(new LambdaAnimation(fadeFunc, 800), 0, launchFactory([this, fadeFunc](const std::function<void()>& finishedCallback) {
			setAnimation(new LambdaAnimation(fadeFunc, 800), 0, finishedCallback, true);
		}));
	} else if (transitionTheme == RecalboxConf::Transition::Slide){

		// move camera to zoom in on center + fade out, launch game, come back in
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 1500), 0, launchFactory([this, center](const std::function<void()>& finishedCallback) {
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 1000), 0, finishedCallback, true);
		}));
	} else {
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, launchFactory([this, center](const std::function<void()>& finishedCallback) {
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, finishedCallback, true);
		}));
	}
}

ISimpleGameListView* ViewController::GetOrCreateGamelistView(SystemData* system, bool reorder)
{
	//if we already made one, return that one
	auto exists = mGameListViews.find(system);
	if(exists != mGameListViews.end())
		return exists->second;

	//if we didn't, make it, remember it, and return it
	ISimpleGameListView* view =
    (system->Descriptor().IsArcade() && RecalboxConf::Instance().GetArcadeViewEnhanced() && !(system->Name() == "daphne")) ?
    new ArcadeGameListView(mWindow, mSystemManager, *system, mResolver, mFlagCaches) :
    new DetailedGameListView(mWindow, mSystemManager, *system, mResolver, mFlagCaches);
  view->DoInitialize();

	mGameListViews[system] = view;
	mInvalidGameList[system] = false;

  if (reorder) ReorderGamelistViewBeforeMoving(system, Move::None);

	return view;
}

bool ViewController::ForceGamelistViewRecreation(SystemData* system)
{
  for (auto& it : mGameListViews)
    if (it.first == system)
    {
      ISimpleGameListView* view = it.second;
      bool isCurrent = (mCurrentView == view);
      FileData* cursor = view->Gamelist().HasGame() ? view->Gamelist().GetSelectedGame() : nullptr;
      mGameListViews.erase(system);

      if (system->HasVisibleGame())
      {
        ISimpleGameListView* newView = GetOrCreateGamelistView(system, false);
        newView->setPosition(view->getPosition());
        if (cursor != nullptr) newView->Gamelist().SetSelectedGame(cursor, true);
        if (isCurrent) mCurrentView = newView;
        mGameListViews[system] = newView;
        return true;
      }
    }
  return false;
}

bool ViewController::ProcessInput(const InputCompactEvent& event)
{
	if (mLockInput) return true;

	/* if we receive a button pressure for a non-configured joystick, suggest the joystick configuration */
	if (event.AskForConfiguration())
	{
		mWindow.pushGui(new GuiDetectDevice(mWindow, false, nullptr));
		return true;
	}

  return mCurrentView->ProcessInput(event);
}

void ViewController::Update(int deltaTime)
{
  mCurrentView->Update(deltaTime);
	updateSelf(deltaTime);
}

void ViewController::Render(const Transform4x4f& parentTrans)
{
  switch(mCurrentViewType)
  {
    case ViewType::GameClip:
    case ViewType::Bouncing:
    case ViewType::CardReaderConsoleView:
    case ViewType::CrtCalibration: mCurrentView->Render(parentTrans); break;
      // All the following view have been drawn before as they are part of the "global" visible area
    case ViewType::SystemList:
    case ViewType::GameList:
    case ViewType::SplashScreen:
    case ViewType::None:
    default:
    {
      Transform4x4f trans = mCamera * parentTrans;
      Transform4x4f transInverse(Transform4x4f::Identity());
      transInverse.invert(trans);

      // camera position, position + size
      Vector3f viewStart = transInverse.translation();
      Vector3f viewEnd = transInverse * Vector3f(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat(), 0);

      int vpl = (int)viewStart.x();
      int vpu = (int)viewStart.y();
      int vpr = (int)viewEnd.x() - 1;
      int vpb = (int)viewEnd.y() - 1;

      // Draw splash
      for(;;)
      {
        // clipping - only y
        const Vector3f& position = mSplashView.getPosition();
        const Vector2f& size = mSplashView.getSize();

        int gu = (int)position.y();
        int gb = (int)position.y() + (int)size.y() - 1;

        if (gb < vpu) break;
        if (gu > vpb) break;

        mSplashView.Render(trans);
        break;
      }

      // Draw systemview
      for(;;)
      {
        // clipping - only y
        const Vector3f& position = mSystemListView.getPosition();
        const Vector2f& size = mSystemListView.getSize();

        int gu = (int)position.y();
        int gb = (int)position.y() + (int)size.y() - 1;

        if (gb < vpu) break;
        if (gu > vpb) break;

        mSystemListView.Render(trans);
        break;
      }

      // Draw gamelists
      for (auto& mGameListView : mGameListViews)
      {
        // clipping
        const Vector3f& position = mGameListView.second->getPosition();
        const Vector2f& size = mGameListView.second->getSize();

        int gl = (int)position.x();
        int gu = (int)position.y();
        int gr = (int)position.x() + (int)size.x() - 1;
        int gb = (int)position.y() + (int)size.y() - 1;

        if (gr < vpl) continue;
        if (gl > vpr) continue;
        if (gb < vpu) continue;
        if (gu > vpb) continue;

        mGameListView.second->Render(trans);
      }

      break;
    }
  }

	if(mWindow.peekGui() == nullptr) // TODO:: dafuk?!
		mWindow.renderHelpPromptsEarly();

	// fade out
	if(mFadeOpacity != 0.0)
	{
		Renderer::SetMatrix(parentTrans);
		Renderer::DrawRectangle(0, 0, Renderer::Instance().DisplayWidthAsInt(), Renderer::Instance().DisplayHeightAsInt(), 0x00000000 | (unsigned char)(mFadeOpacity * 255));
	}
}

void ViewController::InvalidateGamelist(const SystemData* system)
{
	for (auto& mGameListView : mGameListViews)
		if (system == (mGameListView.first))
		{
			mInvalidGameList[mGameListView.first] = true;
			break;
		}
}

void ViewController::InvalidateAllGamelistsExcept(const SystemData* systemExclude)
{
	for (auto& mGameListView : mGameListViews)
		if (systemExclude != (mGameListView.first))
			mInvalidGameList[mGameListView.first] = true;
}

void ViewController::DestroyGameListViews()
{
  // Free all gamelist views but the current one (still used by ReturnedFromGame on game exit).
  std::vector<SystemData*> toRemove;
  for (auto& kv : mGameListViews)
    if (kv.second != mCurrentView)
    {
      delete kv.second;
      toRemove.push_back(kv.first);
    }
  for (SystemData* system : toRemove)
  {
    mGameListViews.erase(system);
    mInvalidGameList.erase(system);
  }
}

bool ViewController::DestroyOldestGameListView()
{
  // Find the least-recently-used gamelist view, NEVER the current one (still displayed on screen and
  // reused by ReturnedFromGame on game exit). Timestamps share the texture tick base (LastUse()).
  SystemData* oldestSystem = nullptr;
  ISimpleGameListView* oldestView = nullptr;
  unsigned long long oldest = 0;
  for (auto& kv : mGameListViews)
  {
    ISimpleGameListView* view = kv.second;
    if (view == mCurrentView) continue; // Protect the current gamelist
    if (oldestView == nullptr || view->LastUse() < oldest)
    {
      oldest = view->LastUse();
      oldestView = view;
      oldestSystem = kv.first;
    }
  }

  if (oldestView == nullptr) return false; // Nothing evictable (only the current view is alive)

  { LOG(LogTrace) << "[ViewController] Memory pressure: freeing oldest gamelist view for system " << oldestSystem->Name(); }
  delete oldestView;
  mGameListViews.erase(oldestSystem);
  mInvalidGameList.erase(oldestSystem);
  return true;
}

bool ViewController::CollectHelpItems(Help& help)
{
	return mCurrentView != nullptr && mCurrentView->CollectHelpItems(help);
}

void ViewController::ChangeView(ViewType newViewMode, SystemData* targetSystem)
{
  // Save previous mode & deinit
  mPreviousViewType = mCurrentViewType;
  mCurrentView->onHide();
  mWindow.SetCanSleep(true);
  switch(mCurrentViewType)
  {
    case ViewType::None:
    case ViewType::SplashScreen:
    case ViewType::SystemList:
    case ViewType::GameList:
    case ViewType::CardReaderConsoleView:
    case ViewType::CrtCalibration: break;
    case ViewType::Bouncing:
    {
      WakeUp();
      break;
    }
    case ViewType::GameClip:
    {
      WakeUp();
      if (RecalboxConf::Instance().GetAudioMode() == AudioMode::MusicsXorVideosSound && newViewMode != ViewType::SplashScreen)
        AudioScheduler::Instance().SetTheme(mCurrentSystem->Theme()).PlayNextMusic();
      break;
    }
  }

  // Process new mode
  mCurrentViewType = newViewMode;

  switch(mCurrentViewType)
  {
    case ViewType::None: return; // lol
    case ViewType::SplashScreen:
    {
      mCurrentView = &mSplashView;
      break;
    }
    case ViewType::CardReaderConsoleView:
    {
      { LOG(LogDebug) << "[CardReaderConsoleView] Launching console view"; }
      mWindow.SetCanSleep(false);
      mCurrentView = &mCRConsoleView;
      break;
    }
    case ViewType::SystemList:
    {
      mCurrentView = &mSystemListView;
      mSystemListView.goToSystem(targetSystem, false);
      mCurrentSystem = targetSystem;
      break;
    }
    case ViewType::GameList:
    {
      mCurrentView = GetOrCreateGamelistView(targetSystem);
      mCurrentSystem = targetSystem;
      break;
    }
    case ViewType::GameClip:
    {
      if (RecalboxConf::Instance().GetAudioMode() == AudioMode::MusicsXorVideosSound)
        AudioScheduler::Instance().StopAll();
      mCurrentView = &mGameClipView;
      break;
    }
    case ViewType::CrtCalibration:
    {
      mCurrentView = &mCrtView;
      break;
    }
    case ViewType::Bouncing:
    {
      mCurrentView = &mBouncingView;
      break;
    }
  }
  mCurrentView->onShow();
  mWindow.NotifyViewChanges();
  UpdateHelpBar();
}

void ViewController::BackToPreviousView()
{
  if (mPreviousViewType == ViewType::None) return; // Previous mode not initialized
  ChangeView(mPreviousViewType, mCurrentSystem);
  mPreviousViewType = ViewType::None; // Reset previous mode so that you cannot go back until next forward move
}

void ViewController::SelectSystem(SystemData* system)
{
  if (mCurrentViewType == ViewType::SystemList) mSystemListView.goToSystem(system, false);
  else if (mCurrentViewType == ViewType::GameList) goToGameList(system);
}

void ViewController::ShowSystem(SystemData* system)
{
  mSystemListView.addSystem(system);
  mSystemListView.Sort();
  InvalidateAllGamelistsExcept(nullptr);
  GetOrCreateGamelistView(system);
}

void ViewController::HideSystem(SystemData* system)
{
  // Remove system in system view
  mSystemListView.removeSystem(system);

  // Are we on the gamelist that just need to be removed?
  if (isViewing(ViewType::GameList))
    if (&((ISimpleGameListView*)mCurrentView)->System() == system)
    {
      SystemData* nextSystem = goToNextGameList();
      mSystemListView.goToSystem(nextSystem, false);
    }
}

void ViewController::UpdateSystem(SystemData* system)
{
  //InvalidateGamelist(system);
  // Force refresh if we're on this system's view
  ISimpleGameListView** view = mGameListViews.try_get(system);
  if (view != nullptr)
    (*view)->ListRefreshRequired();
}

void ViewController::SystemShownWithNoGames(SystemData* system)
{
  String text = (_F(_("System \"{0}\" has no visible game yet!\n\nIt will show up automatically as soon as it has visible games.")) / system->FullName()).ToString();
  mWindow.InfoPopupAddRegular(text, 10, PopupType::Recalbox, false);
}

void ViewController::ToggleFavorite(FileData* game, bool forceStatus, bool forcedStatus)
{
  // Toggle favorite
  game->Metadata().SetFavorite(forceStatus ? forcedStatus : !game->Metadata().Favorite());

  // Fire dynamic system refresh
  mSystemManager.UpdateSystemsOnGameChange(game, MetadataType::Favorite, false);
  // Force refresh of current system cause we probably fired this event from there
  //if (mCurrentViewType == ViewType::GameList)
  //  ((ISimpleGameListView*)mCurrentView)->ListRefreshRequired();

  // Refresh game in its regular view
  ISimpleGameListView** view = mGameListViews.try_get(&game->System());
  if (view != nullptr) (*view)->RefreshItem(game);

  // Info popup
  String message = game->Metadata().Favorite() ? _("Added to favorites") : _("Removed from favorites");
  mWindow.InfoPopupAddRegular(message.Append(":\n").Append(game->Name()), RecalboxConf::Instance().GetPopupHelp(), PopupType::None, false);
}

void ViewController::Run()
{
  while(IsRunning())
  {
    mSignal.WaitSignal();
    if (!IsRunning()) return; // Destructor called :)
    for(bool working = true; working; )
    {
      mLocker.Lock();
      const FileData* item = mNextItem;
      mNextItem = nullptr;
      mLocker.UnLock();
      if (item == nullptr) working = false;
      else if (item->IsFolder())
      {
        FolderData* folder = (FolderData*)item;
        int count = folder->CountAll(true, folder->System().Excludes());

        class Filter : public IFilter
        {
          private:
            HashSet<Path> mDups;
            FolderImagesPath mPaths;
            int mCount;

          public:
            Filter() : mCount(0) {}

            bool ApplyFilter(const FileData& file) override
            {
              if (mCount < sFoldersMaxGameImageCount)
                if (file.HasAnyMedia())
                  if (Path p(file.AnyMedia()); !mDups.contains(p))
                  {
                    mDups.insert(p);
                    mPaths[mCount++] = p;
                  }
              return false;
            }

            FolderImagesPath& GetPaths() { return mPaths; }
        } filter;
        folder->CountFilteredItemsRecursively(&filter, false, true);

        // Store results
        SlowDataInformation* data = mMessageFactory.Obtain();
        data->Set(item, count, false, &filter.GetPaths());
        mSender.Send(data);
      }
      else if (item->IsGame())
      {
        if (item->HasP2K()) // Send message only if p2k is available
        {
          SlowDataInformation* data = mMessageFactory.Obtain();
          data->Set(item, 0, true, nullptr);
          mSender.Send(data);
        }
      }
    }
  }
}

void ViewController::ReceiveSyncMessage(SlowDataInformation* const& data)
{
  if (mCurrentViewType == ViewType::GameList)
    ((ISimpleGameListView*)mCurrentView)->UpdateSlowData(*data);
  mMessageFactory.Recycle(data);
}

void ViewController::FetchSlowDataFor(FileData* data)
{
  Mutex::AutoLock locker(mLocker);
  mNextItem = data;
  mSignal.Fire();
}

void ViewController::RequestSlowOperation(ISlowSystemOperation* interface, ISlowSystemOperation::List systems, bool autoSelectMonoSystem)
{
  String text = systems.Count() > 1 ?
                _("INITIALIZING SYSTEMS...") :
                (_F(_("INITIALIZING SYSTEM {0}")) / systems.First()->FullName()).ToString();
  auto* gui = new GuiWaitLongExecution<DelayedSystemOperationData, bool>(mWindow, *this);
  gui->Execute({ std::move(systems), interface, autoSelectMonoSystem }, text);
  mWindow.pushGui(gui);
}

bool ViewController::Execute(GuiWaitLongExecution<DelayedSystemOperationData, bool>& from,
                             const DelayedSystemOperationData& parameter)
{
  (void)from;
  if (parameter.mSlowIMethodInterface != nullptr)
    parameter.mSlowIMethodInterface->SlowPopulateExecute(parameter.mSystemList);
  return true;
}

void ViewController::Completed(const DelayedSystemOperationData& parameter, const bool& result)
{
  (void)result;
  if (parameter.mSlowIMethodInterface != nullptr)
    parameter.mSlowIMethodInterface->SlowPopulateCompleted(parameter.mSystemList, parameter.autoSelectMonoSystem);
}

void ViewController::ForceGamelistRefresh(SystemData& data)
{
  if (mGameListViews.contains(&data))
    mGameListViews[&data]->ListRefreshRequired();
}

void ViewController::ConfigurationArcadeViewEnhancedChanged(const bool& value)
{
  for(auto& gamelist : mGameListViews)
    if (gamelist.first->IsArcade())
    {
      ISimpleGameListView* previousGamelist = gamelist.second;
      if ((value && previousGamelist->Type() == GamelistViewType::Detailed) ||
          (!value && previousGamelist->Type() == GamelistViewType::Arcade))
        ForceGamelistViewRecreation(gamelist.first);
    }
  if (mCurrentViewType == ViewType::GameList)
    goToGameList(mCurrentSystem);
}

void ViewController::FirstGuiOpened()
{
  if (mCurrentView != nullptr)
    mCurrentView->onHide();
}

void ViewController::LastGuiClosed()
{
  if (mCurrentView != nullptr)
    mCurrentView->onShow();
}

void ViewController::GameExited()
{
  if (mCurrentViewType == ViewType::CardReaderConsoleView)
    mCRConsoleView.Reset();
}

bool ViewController::CanPlayMusic()
{
  return mCurrentViewType != ViewType::CardReaderConsoleView || RecalboxConf::Instance().GetCRConsoleModeSound() == "music";
}
