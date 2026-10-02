//
// Created by xizor on 05/10/2019.
//

#include <components/MenuComponent.h>
#include <guis/GuiTextEditPopupKeyboard.h>
#include <views/ViewController.h>
#include "GuiSearch.h"
#include "views/MenuFilter.h"
#include "guis/menus/MenuProvider.h"


#define TITLE_HEIGHT (mTitle->getFont()->Height() + Renderer::Instance().DisplayHeightAsFloat()*0.0437f )

GuiSearch::GuiSearch(WindowManager& window, SystemManager& systemManager, SystemData* system, SearchForcedOptions* forcedOptions)
  : Gui(window)
  , mSystemManager(systemManager)
  , mGrid(window, Vector2i(3, 3))
  , mMenuTheme(ThemeManager::Instance().Menu())
  , mList(nullptr)
  , mSystemData(system)
  , mForcedOptions(false)
  , mJustOpen(true)
{
  if (forcedOptions != nullptr)
  {
    mForcedOptions = true;
    mForcedSearch = forcedOptions->mSearchText;
    mForcedContext = forcedOptions->mContext;
    mJustOpen = false;
  }

  addChild(&mGrid);

  initGridsNStuff();

  updateSize();
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2,
              (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

  mSystemManager.DeleteFastSearchCache();
  PopulateGrid(mForcedOptions ? mForcedSearch : String::Empty);
}

void GuiSearch::initGridsNStuff()
{
  //init Title

  if (mForcedOptions)
  {
    switch(mForcedContext)
    {
      case FolderData::FastSearchContext::Licences:
      case FolderData::FastSearchContext::Path:
      case FolderData::FastSearchContext::Name:
      case FolderData::FastSearchContext::Description:
      case FolderData::FastSearchContext::Developer:
      case FolderData::FastSearchContext::Publisher:
      case FolderData::FastSearchContext::All:
      default: break;
    }
  }

  mTitle = std::make_shared<TextComponent>(mWindow, _("SEARCH") + " : ", mMenuTheme.Text().font,
                                           mMenuTheme.Text().color, ::Alignment::CenterRight);

  mGrid.setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1));

  //init search textfield
  mSearch = std::make_shared<TextEditComponent>(mWindow);
  if (!mForcedOptions)
  {
    mGrid.setEntry(mSearch, Vector2i(0, 1), false, true, Vector2i(3, 1));
  }

  //init search option selector
  mSearchChoices = std::make_shared<OptionListComponent<FolderData::FastSearchContext> >(mWindow, _("SEARCH BY"),
                                                                                         false);
  FolderData::FastSearchContext currentSearch = FolderData::FastSearchContext::Name;

  mSearchChoices->add(_("NAME"), FolderData::FastSearchContext::Name,               currentSearch == FolderData::FastSearchContext::Name);
  mSearchChoices->add(_("DESCRIPTION"), FolderData::FastSearchContext::Description, currentSearch == FolderData::FastSearchContext::Description);
  mSearchChoices->add(_("DEVELOPER"), FolderData::FastSearchContext::Developer,     currentSearch == FolderData::FastSearchContext::Developer);
  mSearchChoices->add(_("PUBLISHER"), FolderData::FastSearchContext::Publisher,     currentSearch == FolderData::FastSearchContext::Publisher);
  mSearchChoices->add(_("FILENAME"), FolderData::FastSearchContext::Path,           currentSearch == FolderData::FastSearchContext::Path);
  mSearchChoices->add(_("LICENCE"), FolderData::FastSearchContext::Licences,        currentSearch == FolderData::FastSearchContext::Licences);
  mSearchChoices->add(_("ALL"), FolderData::FastSearchContext::All,                 currentSearch == FolderData::FastSearchContext::All);

  mSearchChoices->setChangedCallback([this]
                                     {
                                       mGrid.setColWidthPerc(1, mSearchChoices->getSize().x() / mSize.x());
                                       PopulateGrid(mSearch->getValue());
                                     });

  if (!mForcedOptions)
  {
    mGrid.setEntry(mSearchChoices, Vector2i(1, 0), false, true, Vector2i(1, 1));
  }

  //init big center grid with List and Meta
  mGridMeta = std::make_shared<ComponentGrid>(mWindow, Vector2i(4, 3));
  mGridMeta->setEntry(std::make_shared<Component>(mWindow), Vector2i(1, 0), false, false, Vector2i(1, 3));
  mGridMeta->setEntry(std::make_shared<Component>(mWindow), Vector2i(2, 0), false, false, Vector2i(2, 1));
  mText = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.Text().font, mMenuTheme.Text().color, ::Alignment::Center);
  mList = std::shared_ptr<TextGamelist>(new TextGamelist(mWindow, nullptr, *this, mCache));
  mGridMeta->setEntry(mList, Vector2i(0, 0), true, true, Vector2i(1, 3));

  mGrid.setEntry(mGridMeta, Vector2i(0, 2), true, true, Vector2i(3, 1));
  mGridMeta->setEntry(mText, Vector2i(0, 0), false, true, Vector2i(4, 3));


  //init grid for Meta with logo, image etc
  mGridLogoAndMD = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 6));

  // 3x3 grid to center logo
  mGridLogo = std::make_shared<ComponentGrid>(mWindow, Vector2i(3, 3));

  mGridLogoAndMD->setEntry(mGridLogo, Vector2i(0, 0), false, true, Vector2i(2, 1));

  mMDDeveloper = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.SmallText().font,
                                          mMenuTheme.SmallText().color, ::Alignment::CenterLeft);
  mMDDeveloperLabel = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.SmallText().font,
                                                      mMenuTheme.SmallText().color, ::Alignment::CenterLeft);
  mMDPublisher = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.SmallText().font,
                                                 mMenuTheme.SmallText().color, ::Alignment::CenterLeft);
  mMDPublisherLabel = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.SmallText().font,
                                                      mMenuTheme.SmallText().color, ::Alignment::CenterLeft);

  mGridLogoAndMD->setEntry(mMDDeveloperLabel, Vector2i(0, 2), false, false, Vector2i(1, 1));
  mGridLogoAndMD->setEntry(mMDDeveloper, Vector2i(1, 2), false, true, Vector2i(1, 1));
  mGridLogoAndMD->setEntry(mMDPublisherLabel, Vector2i(0, 4), false, false, Vector2i(1, 1));
  mGridLogoAndMD->setEntry(mMDPublisher, Vector2i(1, 4), false, true, Vector2i(1, 1));
  mGridLogoAndMD->setEntry(std::make_shared<Component>(mWindow), Vector2i(0, 1), false, false, Vector2i(2, 1));
  mGridLogoAndMD->setEntry(std::make_shared<Component>(mWindow), Vector2i(0, 3), false, false, Vector2i(2, 1));
  mGridLogoAndMD->setEntry(std::make_shared<Component>(mWindow), Vector2i(0, 5), false, false, Vector2i(2, 1));

  mGridMeta->setEntry(mGridLogoAndMD, Vector2i(2, 1), false, true, Vector2i(1, 1));

  mResultSystemLogo = std::make_shared<PictureComponent>(mWindow);
  mResultSystemLogo->setKeepRatio(true);
  mGridLogo->setEntry(mResultSystemLogo, Vector2i(0, 0), false, true, Vector2i(2, 2));

  // selected result thumbnail + video
  mResultThumbnail = std::make_shared<PictureComponent>(mWindow, ImgProps::KeepRatio | ImgProps::NoCache);
  mGridMeta->setEntry(mResultThumbnail, Vector2i(3, 1), false, true, Vector2i(1, 1));
  mResultVideo = std::make_shared<VideoComponent>(mWindow, nullptr);
  mGridMeta->setEntry(mResultVideo, Vector2i(3, 1), false, true, Vector2i(1, 1));

  // selected result desc + container
  mDescContainer = std::make_shared<ScrollableContainer>(mWindow);
  mResultDesc = std::make_shared<TextComponent>(mWindow, "", mMenuTheme.Text().font, mMenuTheme.Text().color);
  mResultDesc->setMultiline(true);
  mDescContainer->addChild(mResultDesc.get());
  mDescContainer->setAutoScroll(true);
  mGridMeta->setEntry(mDescContainer, Vector2i(2, 2), false, true, Vector2i(2, 1));

  //col with mList
  mGridMeta->setColWidthPerc(0, 0.39f);
  //spacer col
  mGridMeta->setColWidthPerc(1, 0.01f);
  //cols with metadata
  mGridMeta->setColWidthPerc(2, 0.30f);
  mGridMeta->setColWidthPerc(3, 0.30f);
  //spacer row
  mGridMeta->setRowHeightPerc(0, 0.0825f);
}

GuiSearch::~GuiSearch()
{
  if (mList)
  {
    mList->clear();
  }
}

bool GuiSearch::ProcessInput(const class InputCompactEvent & event)
{
  if (event.CancelReleased())
  {
    clear();
    Close();
    return true;
  }
  if (event.R1Released() && !mForcedOptions)
  {
    mWindow.CreateVirtualKeyboard(_("SEARCH"), mSearch->getValue(), this);
    return true;
  }
  if (event.SelectReleased())
  {
    clear();
    GoToGame();
    Close();
    mWindow.CloseAll();
    return true;
  }
  if (!mSearchResults.Empty())
  {
    FileData* cursor = mSearchResults[mList->getCursor()];

    // NETPLAY
    if ((event.NorthReleased()) && (RecalboxConf::Instance().GetNetplayEnabled())
        && cursor->System().Descriptor().HasNetPlayCores())
    {
      clear();
      MenuProvider::ShowMenu(MenuContainerType::NetplayHost, InheritableContext(&cursor->System(), cursor, (IMetadataChanged*)nullptr, 0));
      //mWindow.pushGui(new MenuNetPlayHostPasswords(mWindow, *cursor));
      return true;
    }
    if (event.WestReleased() && MenuFilter::ShouldEnableFeature(MenuFilter::Favorites) && RecalboxConf::Instance().GetEnableEditFavorites())
    {
      if (cursor->IsGame())
      {
        ViewController::Instance().ToggleFavorite(cursor);
        populateGridMeta(mList->getCursor());
      }
      return true;
    }
  }

  if ((event.AnyLeftReleased() || event.AnyRightReleased()) && !mForcedOptions) return mSearchChoices->ProcessInput(event);

  return mList->ProcessInput(event);
}

void GuiSearch::updateSize()
{
  const float height = Renderer::Instance().DisplayHeightAsFloat() * 0.85f;
  const float width = Renderer::Instance().DisplayWidthAsFloat() * 0.95f;
  setSize(width, height);
}

void GuiSearch::onSizeChanged()
{
  //mSearch->setSize(mSize.x() - 12, mSearch->getSize().y());

  // update grid row/col sizes
  mGrid.setColWidthPerc(0, 0.5f);
  mGrid.setRowHeightPerc(0, TITLE_HEIGHT / mSize.y());
  mGrid.setColWidthPerc(1, mSearchChoices->getSize().x() / mSize.x());
  float v = mSearch->getTextHeight() + mSearch->getVerticalMargins();
  mGrid.setRowHeightPerc(1, v / mSize.y());

  mGrid.setSize(mSize);
}

bool GuiSearch::CollectHelpItems(Help& help)
{
  help.Clear();
  if (AmIOnTopOfScreen())
  {
    help.Set(HelpType::UpDown, _("SELECT"))
        .Set(Help::Cancel(), _("BACK"));

    if (!mForcedOptions)
      help.Set(HelpType::LeftRight, _("SEARCH IN..."))
          .Set(HelpType::R, _("KEYBOARD"));
    
    if (!mList->isEmpty())
    {
      help.Set(Help::Valid(), _("LAUNCH"))
          .Set(HelpType::Select, _("GO TO GAME"))
          .Set(HelpType::X, _("NETPLAY"));
      if (RecalboxConf::Instance().GetEnableEditFavorites())
        help.Set(HelpType::Y, mSearchResults[mList->getCursor()]->Metadata().Favorite() ? _("Remove from favorite") : _("Favorite"));
    }
    return true;
  }
  return false;
}

void GuiSearch::Update(int deltaTime)
{
  Component::Update(deltaTime);

  if (mJustOpen)
  {
    mWindow.CreateVirtualKeyboard(_("SEARCH"), mSearch->getValue(), this);
    mJustOpen = false;
  }
}

void GuiSearch::Render(const Transform4x4f& parentTrans)
{
  Gui::Render(parentTrans);

  Transform4x4f trans = parentTrans * getTransform();

  renderChildren(trans);

  Renderer::SetMatrix(trans);
  Renderer::DrawRectangle(0.f, 0.f, mSize.x(), mSize.y(), 0x00000011);

  // cheap trick to force those damn arrows in OptionList
  if (!mForcedOptions)
  {
    mSearchChoices->onSizeChanged();
  }
}

void GuiSearch::PopulateGrid(const String& search)
{
  if (mList)
    mList->clear();
  mSearchResults.Clear();

  if (search.length()>2)
  {

    if (mForcedOptions)
    {
      FileData::List results = mSystemManager.SearchTextInGames(mForcedContext, search, 100, nullptr);
      for (auto& game : results) {
        if (FolderData::FastSearchContext::Name == mForcedContext && game->Metadata().MainAliasOrName() == search)
          mSearchResults.Add(game);
        else  if (FolderData::FastSearchContext::Licences == mForcedContext)
        {
          String gameLicences = game->Metadata().Licences();
          if (gameLicences == search)
          {
            mSearchResults.Add(game);
            continue;
          }
          for (auto& licence: game->Metadata().Licences().Split(" | "))
          {
            if (licence == search)
            {
              mSearchResults.Add(game);
              break;
            }
          }
        }
      }
    }
    else mSearchResults = mSystemManager.SearchTextInGames(mSearchChoices->getSelected(), search, 100, mSystemData);

    if (!mSearchResults.Empty())
    {
      mText->setValue("");
      bool displayByFileName = RecalboxConf::Instance().GetDisplayByFileName();
      mList->Interface().ClearGames();
      for (auto *game : mSearchResults)
      {
        String gameName(displayByFileName ? game->Metadata().RomFileOnly().FilenameWithoutExtension() : game->Name());
        mList->Interface().AddGames(game, gameName, GameType::Normal, 0);
      }

      clear();
      mGrid.getCellAt(0, 2)->canFocus = true;
      populateGridMeta(0);
    }
    else
    {
      mText->setValue(_("NO RESULTS"));
      clear();
      if (mList) mList->clear();
    }
  }
  else
  {
    mText->setValue(_("TYPE AT LEAST 3 CHARACTERS"));
    clear();
    if (mList) mList->clear();
    mGrid.getCellAt(0, 2)->canFocus = false;
  }
}

void GuiSearch::clear()
{
  mResultThumbnail->setImage(Path::Empty, false);
  mResultVideo->setVideo(Path::Empty, 0, 0, false);
  mResultSystemLogo->setImage(Path::Empty, false);
  mResultDesc->setText("");
  mMDPublisherLabel->setText("");
  mMDPublisher->setText("");
  mMDDeveloperLabel->setText("");
  mMDDeveloper->setText("");
}

//called when changing cursor in mList to populate MD
void GuiSearch::populateGridMeta(int i)
{
  FileData& game = *mSearchResults[i];
  //screenshot & video
  mResultThumbnail->setImage(game.Metadata().Image());
  mResultThumbnail->setKeepRatio(true);
  // todo asap fix playing video if video was playing already in gamelistView
  //mResultVideo->setVideo(game.Metadata().Video(), 2000, 1);
  mResultVideo->setResize(mGridMeta->getColWidth(2) * 0.9f, mGridMeta->getRowHeight(1)*0.9f);
  mResultVideo->setKeepRatio(true);

  //system logo retieved from theme
  Path path;
  ThemeManager::GetDefaultSystemLogo(game.System(), path);
  mResultSystemLogo->setImage(path);
  mGridLogoAndMD->setRowHeightPerc(0, 0.5f);
  ResizeGridLogo();

  //Metadata
  mResultDesc->setText(game.Metadata().Description());
  mDescContainer->setSize(mGridMeta->getColWidth(2) + mGridMeta->getColWidth(3), mGridMeta->getRowHeight(2)*0.9f);
  mResultDesc->setSize(mDescContainer->getSize().x(), 0); // make desc text wrap at edge of container
  mMDPublisherLabel->setText(_("Publisher") + " : ");
  mMDDeveloperLabel->setText(_("Developer") + " : ");
  mMDPublisher->setText(game.Metadata().Publisher());
  mMDDeveloper->setText(game.Metadata().Developer());
  float height = (mMDDeveloperLabel->getFont()->Height() + 2) / mGridLogoAndMD->getSize().y();
  mGridLogoAndMD->setRowHeightPerc(2, height);
  mGridLogoAndMD->setRowHeightPerc(4, height);
  int width = (int)Math::max(mMDDeveloperLabel->getSize().x(), mMDPublisherLabel->getSize().x());
  mGridLogoAndMD->setColWidthPerc(0, (float)width / mGridLogoAndMD->getSize().x());
  mGridLogoAndMD->getCellAt(0, 2)->resize = true;
  mGridLogoAndMD->getCellAt(0, 4)->resize = true;

  UpdateHelpBar();
}

void GuiSearch::ResizeGridLogo()
{
  //trying to center the logo in a 3x3 grid by adjusting col and row size
  int height = (int)mResultSystemLogo->getSize().y();
  int width = (int)mResultSystemLogo->getSize().x();

  float spacer2 = (mGridLogo->getSize().x() - (float)width) / 2 / mGridLogo->getSize().x();

  mGridLogo->setColWidthPerc(0, spacer2);
  mGridLogo->setColWidthPerc(1, (float)width / mGridLogo->getSize().x());
  mGridLogo->setColWidthPerc(2, spacer2);

  float spacer = (mGridLogo->getSize().y() - (float)height) / 2 / mGridLogo->getSize().y();

  mGridLogo->setRowHeightPerc(0, spacer);
  mGridLogo->setRowHeightPerc(1, (float)height / mGridLogo->getSize().y());
  mGridLogo->setRowHeightPerc(2, spacer);
}

void GuiSearch::launch(int index)
{
  if (mList->size() != 0)
  {
    mResultVideo->setVideo(Path::Empty, 0, 0, false);

    int index = mList->getCursor();
    Vector3f target(Renderer::Instance().DisplayWidthAsFloat() / 2.0f, Renderer::Instance().DisplayHeightAsFloat() / 2.0f, 0);
    ViewController::Instance().Launch(mSearchResults[index], GameLinkedData(), target, ViewController::Instance().IsInVirtualSystem());
  }
}

void GuiSearch::GoToGame()
{
  if (mList->size() != 0)
  {
    mResultVideo->setVideo(Path::Empty, 0, 0, false);

    int index = mList->getCursor();
    ViewController::Instance().selectGamelistAndCursor(mSearchResults[index]);
  }
}

void GuiSearch::VirtualKeyboardTextChange(IVirtualKeyboardBase& /*vk*/, const String& text)
{
  mSearch->setValue(text);
  PopulateGrid(text);
}

void GuiSearch::VirtualKeyboardValidated(IVirtualKeyboardBase& /*vk*/, const String& text)
{
  // If no text has been selected, close also the search gui
  if (text.empty())
    Close();
}

