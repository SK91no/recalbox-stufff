//
// Created by bkg2k on 22/08/24.
//
#pragma once

#include <utils/storage/HashMap.h>
#include <games/classifications/Regions.h>
#include <themes/MenuThemeData.h>
#include <memory>
#include <rendering/textures/Texture.h>

class MenuThemeDataCache
{
  public:
    MenuThemeDataCache(const MenuThemeData& menutheme)
      : mTheme(menutheme)
    {}

    /*!
     * @brief Get texture for the menu icon
     * @param icon icon type to get menu icon texture from
     * @return menu icon texture
     */
    Texture GetIcon(MenuThemeData::MenuIcons::Type icon, int width, int height);

    /*!
     * @brief Get texture for the menu element
     * @param element element type to get menu element texture from
     * @return menu element texture
     */
    Texture GetElement(MenuThemeData::IconElement::Type element, int width, int height);

    /*!
     * @brief Get texture for the menu element
     * @param element element type to get menu element texture from
     * @return menu element texture
     */
    Texture GetFromPath(const Path& path, int width, int height);

    //! Clean all texture to free memory
    void Clear()
    {
      mIconToTextures.clear();
      mElementToTextures.clear();
      mPathToTextures.clear();
    }

  private:
    //! icon to texture
    HashMap<MenuThemeData::MenuIcons::Type, Texture> mIconToTextures;
    //! element to texture
    HashMap<MenuThemeData::IconElement::Type, Texture> mElementToTextures;
    //! element to texture
    HashMap<Path, Texture> mPathToTextures;
    //! Hashmap locker
    Mutex mLocker;
    //! Menu theme reference
    const MenuThemeData& mTheme;
};
