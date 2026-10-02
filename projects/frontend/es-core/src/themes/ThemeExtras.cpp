//
// Created by bkg2k on 11/08/2019.
//

#include <algorithm>
#include "ThemeExtras.h"
#include "utils/storage/Set.h"
#include "utils/Log.h"

void ThemeExtras::AssignExtras(const ThemeData& theme, const String& view, ThemeExtras::List&& extras, bool smart)
{
  // delete old extras (if any)
  for (Extra& extra : mExtras)
    removeChild(&extra.Component());

  if (smart) CreateSmartList(extras);
  else mExtras = std::move(extras);

  // Apply theme
  for (auto& extra : mExtras)
    extra.Component().DoApplyThemeElement(theme, view, extra.Name(), ThemePropertyCategory::All);

  // Sort
  SortExtrasByZIndex();

  // add new components
  for (Extra& extra : mExtras)
    addChild(&extra.Component());
}

void ThemeExtras::SortExtrasByZIndex()
{
  std::ranges::stable_sort(mExtras, [](const Extra& a, const Extra& b) { return b.Component().getZIndex() > a.Component().getZIndex(); });
}

void ThemeExtras::CreateSmartList(ThemeExtras::List& newExtras)
{
  { LOGT(LogWarning) << "[ThemeExtra] Smart merge. Old size = " << mExtras.size() << " - new size = " << newExtras.size(); }

  bool move = false;
  // Create new extra map
  HashMap<String, const Extra*> newExtrasMap;
  for(const Extra& newExtra : newExtras) newExtrasMap[newExtra.Name()] = &newExtra;

  // Check existing extras and delete old extras not in the new list
  for(int i = (int)mExtras.size(); --i >= 0;)
  {
    const Extra& oldExtra = mExtras[i];
    const Extra** foundInNew = newExtrasMap.try_get(oldExtra.Name());
    if (foundInNew == nullptr || (*foundInNew)->Type() != oldExtra.Type()) // Not found or type does not match
    {
      { LOGT(LogWarning) << "[ThemeExtra] Removed " << oldExtra.Name(); }
      //delete oldExtra.mComponent;
      mExtras.erase(mExtras.begin() + i);
      move = true;
    }
  }

  // Create old extra map
  HashMap<String, Extra*> oldExtrasMap;
  for(Extra& oldExtra : mExtras) oldExtrasMap[oldExtra.Name()] = &oldExtra;

  for(Extra& newExtra : newExtras)
  {
    Extra** foundInOld = oldExtrasMap.try_get(newExtra.Name());
    if (foundInOld == nullptr)
    {
      { LOGT(LogWarning) << "[ThemeExtra] Added " << newExtra.Name(); }
      mExtras.push_back(std::move(newExtra));
      move = true;
    }
    else if ((*foundInOld)->Type() != newExtra.Type())
    {
      { LOGT(LogWarning) << "[ThemeExtra] Modified " << newExtra.Name(); }
      // Type does not match, replace type & component
      //delete (*foundInOld)->mComponent;
      **foundInOld = std::move(newExtra);
      move = true;
    }
  }
  if (!move) { LOGT(LogWarning) << "[ThemeExtra] No move"; }
}

ThemableComponent* ThemeExtras::Lookup(const String& name)
{
  for(const Extra& extra : mExtras)
    if (name == extra.Name())
      return &(ThemableComponent&)extra.Component();
  return nullptr;
}


