//
// Created by bkg2k on 16/12/23.
//
#pragma once

#include <utils/String.h>
#include <utils/storage/HashMap.h>
#include <utils/storage/Set.h>
#include "ElementTypes.h"
#include "PropertyNames.h"
#include "PropertyTypes.h"
#include "utils/storage/EnumBitArray.h"

class ThemeSupport
{
  public:
    //! Static initializations
    static void InitializeStatics();

    //! Authorized elements/sub-elements in the xml
    static HashMap<ThemeElementType, ThemePropertyNameBits>& ElementMap();
    //! Supported features set
    static HashSet<String>& SupportedFeatures();
    //! Supported view set
    static HashSet<String>& SupportedViews();
    //! Element string to type enum
    static HashMap<String, ThemeElementType>& ElementType();
    //! Element string to name enum
    static HashMap<String, ThemePropertyName>& PropertyName();
    //! Element string name name enum
    static String ReversePropertyName(ThemePropertyName name);
    //! Property type from property name
    static ThemePropertyType PropertyTypeFromName(ThemePropertyName name) { return sArray[(int)name]; }

  private:
    static ThemePropertyType sArray[(int)ThemePropertyName::Count__];
};
