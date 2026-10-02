//
// Created by bkg2k on 14/12/23.
//
#pragma once

#include <themes/PropertyCategories.h>
#include <utils/String.h>
#include "themes/ElementTypes.h"

// Forward declaration
class Component;
class ThemeData;
class ThemeElement;

class Themable
{
  public:
    //! Default desctrustor
    virtual ~Themable() = default;

    /*!
     * @brief Constructor
     * @param component Reference constructor
     */
    explicit Themable(Component* component)
      : mComponent(*component)
    {
    }

    /*!
     * @brief Called to apply a theme element on the current component
     * @param theme Theme
     * @param viewName View name
     * @param elementName Element name
     * @param properties Properties to update
     */
    void DoApplyThemeElement(const ThemeData& theme, const String& viewName, const String& elementName, ThemePropertyCategory properties);

    /*!
     * @brief Called to apply a theme element on the current component using all available properties
     * @param theme Theme
     * @param viewName View name
     * @param elementName Element name
     * @param properties Properties to update
     */
    void DoApplyThemeElementPolymorphic(const ThemeData& theme, const String& viewName, const String& elementName, ThemePropertyCategory properties);

    /*!
     * @brief Force implementations to return a theme element type
     * @return Element type
     */
    [[nodiscard]] virtual ThemeElementType GetThemeElementType() const = 0;

    /*!
     * @brief Implementations may return a second compatible elemen type
     * @return Element type
     */
    [[nodiscard]] virtual ThemeElementType GetThemeElementType2() const { return ThemeElementType::None; };

  protected:
    /*!
     * @brief Called when a theme element is applyed on the current component.
     * @param element Element instance
     * @param properties Properties to update
     */
    virtual void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) { (void)element; (void)properties; }

  private:
    /*!
     * @brief Called when a theme element is applyed on the current component.
     * Actually, this implementation only takes properties related to the Component
     * and is called from DoApplyThemeElement.
     * There is no need for overridden implementation to call the component implementation.
     * Also no need for overridden implementation to process pos/size/origin and all basic component properties
     * @param element Element instance
     * @param properties Properties to update
     */
    void OnApplyThemeElementBase(const ThemeElement& element, ThemePropertyCategory properties);

    //! Component reference
    Component& mComponent;
};
