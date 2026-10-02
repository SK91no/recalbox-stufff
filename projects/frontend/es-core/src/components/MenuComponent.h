#pragma once

#include <guis/GuiInfoPopupBase.h>
#include <components/NinePatchComponent.h>
#include <components/ComponentList.h>
#include <components/TextComponent.h>
#include <components/ComponentGrid.h>
#include <themes/MenuThemeData.h>
#include <guis/GuiMsgBoxScroll.h>
#include <components/DateTimeComponent.h>
#include <guis/GuiInfoPopup.h>
#include <themes/ThemeManager.h>
#include "TextScrollComponent.h"
#include <rendering/fonts/FontManager.h>

class ButtonComponent;
class ImageComponent;

class MenuComponent : public Component
{
  public:
    MenuComponent(WindowManager&window, const String& title, NewFont* titleFont);
    MenuComponent(WindowManager&window, const String& title)
      : MenuComponent(window, title, &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false))
    {
    }

    void onSizeChanged() override;

    void addRow(const ComponentListRow& row, bool setCursorHere = false, bool updateGeometry = true) { mList->addRow(row, setCursorHere, updateGeometry); if (updateGeometry) updateSize(); }

    void addRowWithHelp(ComponentListRow& row, const String& label, const String& help = "", bool setCursorHere = false, bool updateGeometry = true)
    {
      if (!help.empty()) {
        row.makeHelpInputHandler(buildHelpGui(label, help));
      }
      addRow(row, setCursorHere, updateGeometry);
    }

    void addWithLabel(const std::shared_ptr<Component>& comp, const String& label, const String& help = "", bool setCursorHere = false, bool invert_when_selected = true, const std::function<void()>& acceptCallback = nullptr)
    {
      ComponentListRow row;
      const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
      row.addElement(std::make_shared<TextComponent>(mWindow, label.ToUpperCaseUTF8(), menuTheme.Text().font, menuTheme.Text().color), true);
      row.addElement(comp, false, invert_when_selected);
      if (acceptCallback) {
        row.makeAcceptInputHandler(acceptCallback);
      }
      if (!help.empty()) {
        row.makeHelpInputHandler(buildHelpGui(label, help));
      }
      addRow(row, setCursorHere, true);
    }

    void addWithLabel(const std::shared_ptr<Component>& comp, const Path& iconPath, const String& label, const String& help)
    {
      ComponentListRow row;
      const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

      if (!iconPath.IsEmpty())
      {
        // icon
        auto icon = std::make_shared<ImageComponent>(mWindow);
        icon->setImage(iconPath, false);
        icon->setColorShift(menuTheme.Text().color);
        icon->setResize(0, menuTheme.Text().font->Height() * 1.25f);
        row.addElement(icon, false, true);

        // spacer between icon and text
        auto spacer = std::make_shared<Component>(mWindow);
        spacer->setSize(10.f + Math::round(menuTheme.Text().font->Height() * 1.25f) - Math::round(icon->getSize().x()), 0);
        row.addElement(spacer, false, true);
      }

      row.addElement(std::make_shared<TextComponent>(mWindow, label.ToUpperCaseUTF8(), menuTheme.Text().font, menuTheme.Text().color), true);
      row.addElement(comp, false, true);

      //if (acceptCallback) row.makeAcceptInputHandler(acceptCallback);
      if (!help.empty()) row.makeHelpInputHandler(buildHelpGui(label, help));
      addRow(row, false, true);
    }

    void addButton(const String& label, const String& helpText, const std::function<void()>& callback);

    void setFooter(const String& label);

    void setTitle(const String& title, NewFont* font = &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false));

    void setCursorToList() { mGrid.setCursorTo(mList); }
    void setCursorToList(int index) { mGrid.setCursorTo(mList); mList->setCursorIndex(index % mList->Count()); }
    int getButtonsSize() { return (int)mButtons.size(); }
    void setCursorToButtons() { assert(mButtonGrid); mGrid.setCursorTo(mButtonGrid); }
    void setCursorToButton(int index) {
        setCursorToButtons();
        mGrid.moveCursor(Vector2i(index, 0));
    }
    void clear() { mList->clear(); }

    bool CollectHelpItems(Help& help) override;

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    void refresh()
    {
      updateGrid();
      updateSize();
    }

    void SetDefaultButton(int index);

    /*!
     * @brief Update menu regading the given theme
     * @param theme New theme to apply
     */
    void UpdateMenuTheme(const MenuThemeData& theme);

    static std::shared_ptr<ComponentGrid> MakeButtonGrid(WindowManager&window, const std::vector< std::shared_ptr<ButtonComponent> >& buttons);
    static std::shared_ptr<ComponentGrid> MakeMultiDimButtonGrid(WindowManager&window, const std::vector< std::vector< std::shared_ptr<ButtonComponent> > >& buttons, float outerWidth, float outerHeight);
    static std::shared_ptr<ImageComponent> MakeArrow(WindowManager&window);

  protected:
    [[nodiscard]] ComponentList* getList() const { return mList.get(); }

  private:

    void updateSize();
    void updateGrid();
    [[nodiscard]] float getButtonGridHeight() const;
    [[nodiscard]] float getFooterHeight() const;

    ComponentGrid mGrid;

    std::shared_ptr<TextScrollComponent> mTitle;
    std::shared_ptr<DateTimeComponent> mDateTime;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtonGrid;
    std::shared_ptr<TextComponent> mFooter;
    std::vector< std::shared_ptr<ButtonComponent> > mButtons;

    int mTimeAccumulator;

    std::function<void()> buildHelpGui(const String& label, const String& help)
    {
      return [this, label, help] () {
        int dur = RecalboxConf::Instance().GetPopupHelp();
        if (dur != 0)
          mWindow.InfoPopupAdd(new GuiInfoPopup(mWindow, label + "\n" + help, dur, PopupType::Help));
        return true;
      };
    }

    static float MenuWidth()
    {
      return Math::min(Renderer::Instance().DisplayHeightAsFloat() * 1.2f,
                       Renderer::Instance().DisplayWidthAsFloat() * 0.90f);
    }
};
