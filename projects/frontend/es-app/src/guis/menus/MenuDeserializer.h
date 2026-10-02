//
// Created by bkg2k on 16/04/25.
//
#pragma once

#include "MenuDefinition.h"
#include <utils/os/fs/Path.h>
#include <utils/Xml.h>
#include <utils/storage/HashMap.h>
#include <utils/storage/Array.h>
#include "MenuItemType.h"

class MenuDeserializer
{
  public:
    [[nodiscard]] const HashMap<MenuContainerType, MenuDefinition>&  MenuDefinitions() const { return mMenuDefinitions; }

    /*!
     * @brief Deserialize menu.xml
     */
    void DeserializeMenus(const Path& menuPath);

    /*!
     * @brief Deserialize menu.xml
     * @return True if the operation is successfull, false otherwise
     */
    bool DeserializeXml(const Path& xmlPath);

    /*!
     * @brief Deserialize single menu
     * @param menu Menu xml node
     * @param items Item list in which to add new deserialized submenu - null for top level menus menu
     */
    void DeserializeMenu(const String& parentId, const XmlNode& menu, std::vector<ItemDefinition>* items);

    /*!
     * @brief Deserialize menu reference
     * @param parentId parent menu identifier
     * @param items Item list in which to add new deserialized reference
     * @param menuRef menu reference node
     */
    void DeserializeMenuRef(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& menuRef);

    /*!
     * @brief Deserialize single item
     * @param parentId parent menu identifier
     * @param items Item list in which to add new deserialized item
     * @param item Item xml node
     */
    static void DeserializeItem(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& item);

    /*!
     * @brief Deserialize single setting
     * @param parentId parent menu identifier
     * @param items Item list in which to add new deserialized item
     * @param item Item xml node
     */
    static void DeserializeSetting(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& item);

    /*!
     * @brief Deserialize header
     * @param parentId parent menu identifier
     * @param items Item list in which to add new deserialized header
     * @param header Header xml node
     */
    static void DeserializeHeader(const String& parentId, std::vector<ItemDefinition>& items, const XmlNode& header);

  private:
    //! Menus definititons id:definition
    HashMap<MenuContainerType, MenuDefinition> mMenuDefinitions;

    /*!
     * @brief Check all submenu reference and verify reference ID are existing menus
     */
    void CheckMenuReferences();

    /*!
     * @brief Check an item against allowed properties. Fail if at least one attribute is unknown
     * @param parentId Parent menu id
     * @param allowedProperties List of allowed properties
     * @param node XML node to check
     */
    static void CheckProperties(const String& parentId, const Array<const char*>& allowedProperties, const XmlNode& node);
};
