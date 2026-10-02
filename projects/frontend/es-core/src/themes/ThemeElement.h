//
// Created by bkg2k on 09/11/2019.
//
#pragma once

#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include <utils/math/Vector2f.h>
#include "ElementTypes.h"
#include "PropertyNames.h"
#include "utils/storage/HashMap.h"
#include "utils/storage/Array.h"
#include "rendering/fonts/Alignment.h"
#include "ThemeContexts.h"
#include "ThemePropertyBag.h"
#include "IThemeContextualVariableResolver.h"
#include "ThemeVariablesHolder.h"
#include "games/ItemType.h"

class Renderer;

class ThemeElement
{
  public:
    //! Property entry: a property name and the index of its (deduplicated, pooled) value
    struct PropEntry
    {
      ThemePropertyName mName; //!< Property name
      int mBagIndex;           //!< Index into the shared PropertyBag pool
    };
    //! Properties - few per element, so a flat array + double-ended linear lookup beats a hashmap
    Array<PropEntry> mProperties;
    //! Property map
    ThemePropertyNameBits mAvailableProperties;
    //! Property map
    ThemePropertyNameBits mLocalizedProperties;
    //! Element name
    String mName;
    //! Variable resolver
    ThemeVariablesHolder& mVariables;
    //! Element type
    ThemeElementType mType;
    //! Is extra element
    bool mExtra;
    //! Merged property contexts
    ThemeContexts mMergedContext;
    //! Limited view
    ThemeItemType mLimitedVisibility;

  public:
    /*!
     * @brief Constructor
     * @param name Element name
     * @param type Element type
     * @param extra Element is an extra
     */
    ThemeElement(const String& name, ThemeElementType type, bool extra, ThemeItemType limitedVisibility, ThemeVariablesHolder& variables)
      : mProperties(0, 1) // small granularity (2-item steps): few properties per element, avoid the default 16-item blocks
      , mName(name)
      , mVariables(variables)
      , mType(type)
      , mExtra(extra)
      , mMergedContext(ThemeContexts::None)
      , mLimitedVisibility(limitedVisibility)
    {
    }

    [[nodiscard]] String AsString(ThemePropertyName name) const;
    [[nodiscard]] Path AsPath(ThemePropertyName name) const;
    [[nodiscard]] int AsInt(ThemePropertyName name) const;
    [[nodiscard]] float AsFloat(ThemePropertyName name) const;
    [[nodiscard]] bool AsBool(ThemePropertyName name) const;
    [[nodiscard]] Vector2f AsVector(ThemePropertyName name) const;
    [[nodiscard]] Alignment AsAlignment(ThemePropertyName name) const;

    [[nodiscard]] bool HasProperty(ThemePropertyName property) const { return mAvailableProperties.IsSet(property); }

    [[nodiscard]] bool HasProperties() const { return !mProperties.Empty(); }

    [[nodiscard]] const String& Name() const { return mName; }

    [[nodiscard]] ThemeElementType Type() const { return mType; }

    [[nodiscard]] bool Extra() const { return mExtra; }

    [[nodiscard]] ThemeItemType LimitedVisibility() const { return mLimitedVisibility; }

    [[nodiscard]] bool HasUnresolvedVariables() const { return !isEmpty(mMergedContext); }

    [[nodiscard]] bool HasLimitedVisiblity() const { return !isEmpty(mLimitedVisibility); }

    void AddVectorProperty(ThemePropertyName name, float x, float y);
    void AddVectorProperty(ThemePropertyName name, const Vector2f& v);
    void AddStringProperty(ThemePropertyName name, const String& s);
    void AddVariableProperty(ThemePropertyName name, const String& s, ThemeContexts context);
    void AddIntProperty(ThemePropertyName name, int v);
    void AddFloatProperty(ThemePropertyName name, float v);
    void AddBoolProperty(ThemePropertyName name, bool v);

    ThemeElement& MergeExtra(bool extra) { mExtra |= extra; return *this; }
    ThemeElement& MergeVisibility(ThemeItemType limitedVisibility) { mLimitedVisibility |= limitedVisibility; return *this; }

    [[nodiscard]] bool IsAlreadyLocalized(ThemePropertyName property) const { return mLocalizedProperties.IsSet(property); }
    void SetLocalized(ThemePropertyName property) { mLocalizedProperties.Set(property); }

  private:
    //! Find the pooled value index for a property, or -1 if absent (double-ended linear scan)
    [[nodiscard]] int FindBag(ThemePropertyName name) const;
    //! Set or replace the pooled value index for a property
    void SetBag(ThemePropertyName name, int bagIndex);
};
