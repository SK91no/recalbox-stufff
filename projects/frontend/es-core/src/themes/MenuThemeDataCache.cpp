//
// Created by bkg2k on 22/08/24.
//

#include "MenuThemeDataCache.h"
#include <themes/ThemeManager.h>
#include <rendering/textures/TextureManager.h>

Texture MenuThemeDataCache::GetIcon(MenuThemeData::MenuIcons::Type icon, int width, int height)
{
  Mutex::AutoLock lock(mLocker);
  Texture* resource = mIconToTextures.try_get(icon);
  if (resource == nullptr)
  {
    // Load flag
    Texture texture = TextureManager::Instance().Create(ThemeManager::Instance().Menu().Icons().FromType(icon), width, height, TextureHolder::Properties::ImmediateLoad);
    TextureManager::Instance().RequestTextureLoading(texture, true);
    mIconToTextures.insert(icon, texture);
    resource = mIconToTextures.try_get(icon);
  }
  return *resource;
}

Texture MenuThemeDataCache::GetElement(MenuThemeData::IconElement::Type element, int width, int height)
{
  Mutex::AutoLock lock(mLocker);
  Texture* resource = mElementToTextures.try_get(element);
  if (resource == nullptr)
  {
    // Load flag
    Texture texture = TextureManager::Instance().Create(ThemeManager::Instance().Menu().Elements().FromType(element), width, height, TextureHolder::Properties::ImmediateLoad);
    TextureManager::Instance().RequestTextureLoading(texture, true);
    mElementToTextures.insert(element, texture);
    resource = mElementToTextures.try_get(element);
  }
  return *resource;
}

Texture MenuThemeDataCache::GetFromPath(const Path& path, int width, int height)
{
  Mutex::AutoLock lock(mLocker);
  Texture* resource = mPathToTextures.try_get(path);
  if (resource == nullptr)
  {
    // Load flag
    Texture texture = TextureManager::Instance().Create(path, width, height, TextureHolder::Properties::ImmediateLoad);
    TextureManager::Instance().RequestTextureLoading(texture, true);
    mPathToTextures.insert(path, texture);
    resource = mPathToTextures.try_get(path);
  }
  return *resource;
}
