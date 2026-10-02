//
// Created by bkg2k on 17/12/23.
//
#pragma once

#include <utils/storage/HashMap.h>

enum class ThemeElementType
{
  Polymorphic = -1, //!< Special value !
  None = 0, //!< Special value
  Image,
  Box,
  Video,
  Text,
  ScrollText,
  Markdown,
  TextList,
  LogoList,
  CarouselList,
  GridList,
  Container,
  NinePatch,
  DateTime,
  Rating,
  Sound,
  HelpSystem,
  Carousel,
  MenuBackground,
  MenuIcons,
  MenuSwitch,
  MenuSlider,
  MenuButton,
  MenuText,
  MenuTextSmall,
  MenuSection,
  MenuSize,
  Keyboard,
};

inline const char* ThemeElementTypeToString(ThemeElementType type)
{
  static HashMap<ThemeElementType, const char*> sNames
  ({
    { ThemeElementType::Polymorphic,    "Polymorphic" },
    { ThemeElementType::None,           "None" },
    { ThemeElementType::Image,          "Image" },
    { ThemeElementType::Box,            "Box" },
    { ThemeElementType::Video,          "Video" },
    { ThemeElementType::Text,           "Text" },
    { ThemeElementType::ScrollText,     "ScrollText" },
    { ThemeElementType::Markdown,       "Markdown" },
    { ThemeElementType::TextList,       "TextList" },
    { ThemeElementType::LogoList,       "LogoList" },
    { ThemeElementType::CarouselList,   "CarouselList" },
    { ThemeElementType::GridList,       "GridList" },
    { ThemeElementType::Container,      "Container" },
    { ThemeElementType::NinePatch,      "NinePatch" },
    { ThemeElementType::DateTime,       "DateTime" },
    { ThemeElementType::Rating,         "Rating" },
    { ThemeElementType::Sound,          "Sound" },
    { ThemeElementType::HelpSystem,     "HelpSystem" },
    { ThemeElementType::Carousel,       "Carousel" },
    { ThemeElementType::MenuBackground, "MenuBackground" },
    { ThemeElementType::MenuIcons,      "MenuIcons" },
    { ThemeElementType::MenuSwitch,     "MenuSwitch" },
    { ThemeElementType::MenuSlider,     "MenuSlider" },
    { ThemeElementType::MenuButton,     "MenuButton" },
    { ThemeElementType::MenuText,       "MenuText" },
    { ThemeElementType::MenuTextSmall,  "MenuTextSmall" },
    { ThemeElementType::MenuSection,    "MenuSection" },
    { ThemeElementType::MenuSize,       "MenuSize" },
    { ThemeElementType::Keyboard,       "Keyboard" },
  });

  const char** found = sNames.try_get(type);
  if (found != nullptr)
    return *found;

  return "Unknown";
}
