//
// Created by bkg2k on 11/08/2019.
//
#pragma once

#include <components/base/Component.h>
#include "components/base/ThemableComponent.h"
#include <algorithm>

class ThemeExtras : public Component
{
  public:
    //! Little structure that associate name and type to an extra component
    class Extra
    {
      public:
        //! Constructor
        Extra(const String& name, ThemeElementType type, ThemableComponent* component)
          : mName(name)
          , mType(type)
          , mComponent(component)
        {}

        //! Copy constructor is not allowed - component must have only one owner!
        Extra(const Extra&) = delete;

        //! Move constructor
        Extra(Extra&& source) noexcept
        {
          mName = source.mName;
          mType = source.mType;
          mComponent = source.mComponent;
          source.mComponent = nullptr;
        }

        //! Copy assignment is not allowed - component must have only one owner!
        Extra& operator = (const Extra& source) = delete;

        //! Move assigneent
        Extra& operator = (Extra&& source) noexcept
        {
          if (&source != this)
          {
            mName = source.mName;
            mType = source.mType;
            mComponent = source.mComponent;
            source.mComponent = nullptr;
          }
          return *this;
        }

        [[nodiscard]] const String& Name() const { return mName; }
        [[nodiscard]] ThemeElementType Type() const { return mType; }
        ThemableComponent& Component() { return *mComponent; }
        [[nodiscard]] const ThemableComponent& Component() const { return *mComponent; }

      private:
        //! Element name
        String mName;
        //! Element type
        ThemeElementType mType;
        //! Attached component
        ThemableComponent* mComponent;
    };

    typedef std::vector<Extra> List;

    //! Constructor
    explicit ThemeExtras(WindowManager& window)
      : Component(window)
    {
    }

    /*!
     * @brief Assign new extra array (transfer elements)
     * @param extras Extra
     * @param smart True to try keeping as much as possible of extra component when names/types are matching
     */
    void AssignExtras(const ThemeData& theme, const String& view, List&& extras, bool smart);

    /*!
     * @brief Access array element
     * @return
     */
    List& Extras() {return mExtras;}

    /*!
     * @brief Lookup component by name
     * @param name Component name
     * @return Component reference or nullptr if not found
     */
    ThemableComponent* Lookup(const String& name);

  private:
    //! Extra list
    List mExtras;

    /*!
     * @brief Sort all elements regarding their Z-Order.
     * Keep same elements in their original order
     */
    void SortExtrasByZIndex();

    /*!
     * @brief Create a merged list between the current list and the given list, trying to keep component as mush as possible
     * @param newExtras New extra list
     * @return
     */
    void CreateSmartList(List& newExtras);
};

