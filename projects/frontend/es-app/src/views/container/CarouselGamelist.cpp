//
// Created by bkg2k on 23/12/25.
//

#include "CarouselGamelist.h"

CarouselGamelist::CarouselGamelist(WindowManager& window, SystemData& system, IGamelistEvents& eventReceiver,
                                   PictogramCaches& pictogramCaches)
  : AbstractGamelist(window, system, this, { { .mMoveDelay=500, .mNext=5000}, { .mMoveDelay=100, .mNext=INT32_MAX} }, true)
  , Themable(*this)
  , mEvents(eventReceiver)
  , mCaches(pictogramCaches)
  , mLastParent(nullptr)
  , mColors { 0 }
  , mDecorations(RecalboxConf::Instance().GetSystemGamelistDecoration(system))
{
  // Default "theming"
  const MenuThemeData& menu = ThemeManager::Instance().Menu();
  mFont = menu.Text().font;
  SetColor(0, menu.Text().color);
  SetColor(1, menu.Text().color);
}

FileData::List CarouselGamelist::GetHeaderGames(HeaderData* header)
{
  FileData::List result;
  bool found = false;
  for(const auto& entry : AbstractGamelist::GetRawEntries())
  {
    if (found && entry.mGame->IsGame())
      result.Add(entry.mGame);
    if (entry.mGame->IsHeader())
    {
      if (found) break; // found another header, just break;
      if (entry.mGame == header) found = true; // found the requested header
    }
  }
  return result;
}

bool CarouselGamelist::ProcessListInput(const InputCompactEvent& event)
{
  (void)event;
  return false;
}

void CarouselGamelist::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  Themable::OnApplyThemeElement(element, properties);
}

void CarouselGamelist::BaseListIndexChanged(int newIndex, FileData* game)
{
  mEvents.OnGameSelected(*this, game, newIndex);
}
