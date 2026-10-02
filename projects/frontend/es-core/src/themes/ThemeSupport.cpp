//
// Created by bkg2k on 16/12/23.
//

#include "ThemeSupport.h"
#include "PropertyNames.h"

ThemePropertyType ThemeSupport::sArray[(int)ThemePropertyName::Count__];

void ThemeSupport::InitializeStatics()
{
  static bool sInitialized = false;
  if (sInitialized) return;
  sInitialized = true;

  // Initialise property name => type
  for(int i = (int)ThemePropertyName::Count__; --i >= 0;)
    switch((ThemePropertyName)i)
    {
      case ThemePropertyName::Alignment:
      case ThemePropertyName::Animations: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::BackgroundColor:
      case ThemePropertyName::Color:
      case ThemePropertyName::ColorTop:
      case ThemePropertyName::ColorBottom:
      case ThemePropertyName::ColorLeft:
      case ThemePropertyName::ColorRight:
      case ThemePropertyName::ColorTopLeft:
      case ThemePropertyName::ColorTopRight:
      case ThemePropertyName::ColorBottomLeft:
      case ThemePropertyName::ColorBottomRight: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::DefaultTransition: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::Delay: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::Disabled: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::Display: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::FadePath:
      case ThemePropertyName::FilledPath:
      case ThemePropertyName::FontPath: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::FontSize: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::FontStyle: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::FontColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::ForceUppercase: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::Height:
      case ThemePropertyName::HorizontalMargin: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::IconA:
      case ThemePropertyName::IconAdvanced:
      case ThemePropertyName::IconArcade:
      case ThemePropertyName::IconB: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::IconColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::IconBios:
      case ThemePropertyName::IconCardReader:
      case ThemePropertyName::IconControllers:
      case ThemePropertyName::IconDownload:
      case ThemePropertyName::IconFastShutdown:
      case ThemePropertyName::IconGames:
      case ThemePropertyName::IconHkA:
      case ThemePropertyName::IconHkB:
      case ThemePropertyName::IconHkL:
      case ThemePropertyName::IconHkR:
      case ThemePropertyName::IconHkX:
      case ThemePropertyName::IconHkY:
      case ThemePropertyName::IconHkLeftRight:
      case ThemePropertyName::IconHotkey:
      case ThemePropertyName::IconJ1LeftRight:
      case ThemePropertyName::IconJ1UpDown:
      case ThemePropertyName::IconJ1UpDownLeftRight:
      case ThemePropertyName::IconJ2LeftRight:
      case ThemePropertyName::IconJ2UpDown:
      case ThemePropertyName::IconJ2UpDownLeftRight:
      case ThemePropertyName::IconKodi:
      case ThemePropertyName::IconL:
      case ThemePropertyName::IconL2:
      case ThemePropertyName::IconL2R2:
      case ThemePropertyName::IconL3:
      case ThemePropertyName::IconL3R3:
      case ThemePropertyName::IconLeftRight:
      case ThemePropertyName::IconLicense:
      case ThemePropertyName::IconLists:
      case ThemePropertyName::IconLR:
      case ThemePropertyName::IconNetwork:
      case ThemePropertyName::IconQuit:
      case ThemePropertyName::IconR:
      case ThemePropertyName::IconR2:
      case ThemePropertyName::IconR3:
      case ThemePropertyName::IconRecalboxRGBDual:
      case ThemePropertyName::IconRestart:
      case ThemePropertyName::IconScraper:
      case ThemePropertyName::IconSelect:
      case ThemePropertyName::IconShutdown:
      case ThemePropertyName::IconSound:
      case ThemePropertyName::IconStart:
      case ThemePropertyName::IconSystem:
      case ThemePropertyName::IconTate:
      case ThemePropertyName::IconThemes:
      case ThemePropertyName::IconUI:
      case ThemePropertyName::IconUpDown:
      case ThemePropertyName::IconUpDownLeftRight:
      case ThemePropertyName::IconUpdates:
      case ThemePropertyName::IconX:
      case ThemePropertyName::IconY: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::KeepRatio: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::LineSpacing: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::Link:
      case ThemePropertyName::LogoAlignment: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::LogoRotation: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::LogoRotationOrigin: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::LogoScale: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::LogoSize: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::TextOnly: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::Loops:
      case ThemePropertyName::MaxLogoCount: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::MaxSize: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::Multiline: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::Origin: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::Path:
      case ThemePropertyName::PathOff:
      case ThemePropertyName::PathOn: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::Pos: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::PrimaryColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::Reflection: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::Rotation: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::RotationOrigin: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::ScrollSound: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::SecondaryColor:
      case ThemePropertyName::SelectedColor:
      case ThemePropertyName::SelectorColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::SelectorHeight: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::SelectorImagePath: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::SelectorImageTile: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::SelectorOffsetX: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::SelectorOffsetY: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::SeparatorColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::Size: sArray[i] = ThemePropertyType::NormalizedPair; break;
      case ThemePropertyName::Text: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::TextColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::TextOffsetX: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::Tile: sArray[i] = ThemePropertyType::Boolean; break;
      case ThemePropertyName::Type: sArray[i] = ThemePropertyType::String; break;
      case ThemePropertyName::UnfilledPath: sArray[i] = ThemePropertyType::Path; break;
      case ThemePropertyName::VerticalMargin: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::ZIndex: sArray[i] = ThemePropertyType::Float; break;
      case ThemePropertyName::KeyColor:
      case ThemePropertyName::KeySelectedColor:
      case ThemePropertyName::KeyTextColor:
      case ThemePropertyName::KeyDisabledColor:
      case ThemePropertyName::KeyModifierColor:
      case ThemePropertyName::KeyModifierLockedColor:
      case ThemePropertyName::KeyTitleColor:
      case ThemePropertyName::KeyEditTextColor: sArray[i] = ThemePropertyType::Color; break;
      case ThemePropertyName::Count__:
      default: abort();
    }
}

HashSet<String>& ThemeSupport::SupportedViews()
{
  static HashSet<String> sSupportedViews;
  if (sSupportedViews.empty())
    for(const String& value : String("system,basic,detailed,menu,gameclip").Split(','))
      sSupportedViews.insert(value);
  return sSupportedViews;
}

HashSet<String>& ThemeSupport::SupportedFeatures()
{
  static HashSet<String> sSupportedFeatures;
  if (sSupportedFeatures.empty())
    for(const String& value : String("carousel,z-index").Split(','))
      sSupportedFeatures.insert(value);
  return sSupportedFeatures;
}

HashMap<ThemeElementType, ThemePropertyNameBits>& ThemeSupport::ElementMap()
{
  static HashMap<ThemeElementType, ThemePropertyNameBits> sElementMap =
    {
      {
        ThemeElementType::Image,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::MaxSize,
          ThemePropertyName::KeepRatio,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Path,
          ThemePropertyName::Tile,
          ThemePropertyName::Color,
          ThemePropertyName::ColorTop,
          ThemePropertyName::ColorBottom,
          ThemePropertyName::ColorLeft,
          ThemePropertyName::ColorRight,
          ThemePropertyName::ColorTopLeft,
          ThemePropertyName::ColorTopRight,
          ThemePropertyName::ColorBottomLeft,
          ThemePropertyName::ColorBottomRight,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
          ThemePropertyName::Reflection,
        },
      },
      {
        ThemeElementType::Box,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Color,
          ThemePropertyName::ColorTop,
          ThemePropertyName::ColorBottom,
          ThemePropertyName::ColorLeft,
          ThemePropertyName::ColorRight,
          ThemePropertyName::ColorTopLeft,
          ThemePropertyName::ColorTopRight,
          ThemePropertyName::ColorBottomLeft,
          ThemePropertyName::ColorBottomRight,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::Video,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::MaxSize,
          ThemePropertyName::KeepRatio,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Path,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
          ThemePropertyName::Animations,
          ThemePropertyName::Loops,
          ThemePropertyName::Delay,
          ThemePropertyName::Reflection,
          ThemePropertyName::Link,
        },
      },
      {
        ThemeElementType::Text,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Text,
          ThemePropertyName::Path,
          ThemePropertyName::BackgroundColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::FontStyle,
          ThemePropertyName::Color,
          ThemePropertyName::Alignment,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::LineSpacing,
          ThemePropertyName::Multiline,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::Markdown,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Text,
          ThemePropertyName::Path,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Color,
          ThemePropertyName::Alignment,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::LineSpacing,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::ScrollText,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::Text,
          ThemePropertyName::Path,
          ThemePropertyName::BackgroundColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::FontStyle,
          ThemePropertyName::Color,
          ThemePropertyName::Alignment,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::TextList,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::SelectorHeight,
          ThemePropertyName::SelectorOffsetY,
          ThemePropertyName::SelectorColor,
          ThemePropertyName::SelectorImagePath,
          ThemePropertyName::SelectorImageTile,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::PrimaryColor,
          ThemePropertyName::SecondaryColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::ScrollSound,
          ThemePropertyName::Alignment,
          ThemePropertyName::HorizontalMargin,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::LineSpacing,
          ThemePropertyName::ZIndex,
        },
      },
      {
        ThemeElementType::LogoList,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::SelectorHeight,
          ThemePropertyName::SelectorColor,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::PrimaryColor,
          ThemePropertyName::SecondaryColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Alignment,
          ThemePropertyName::HorizontalMargin,
          ThemePropertyName::VerticalMargin,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::LineSpacing,
          ThemePropertyName::ZIndex,
        },
      },
      {
        ThemeElementType::CarouselList,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::ZIndex,
        },
      },
      {
        ThemeElementType::GridList,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::ZIndex,
        },
      },
      {
        ThemeElementType::Container,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::NinePatch,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Path,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::DateTime,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Color,
          ThemePropertyName::BackgroundColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Alignment,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
          ThemePropertyName::Display,
        },
      },
      {
        ThemeElementType::Rating,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::Origin,
          ThemePropertyName::Rotation,
          ThemePropertyName::RotationOrigin,
          ThemePropertyName::FilledPath,
          ThemePropertyName::UnfilledPath,
          ThemePropertyName::ZIndex,
          ThemePropertyName::Disabled,
        },
      },
      {
        ThemeElementType::Sound,
        {
          ThemePropertyName::Path,
        },
      },
      {
        ThemeElementType::HelpSystem,
        {
          ThemePropertyName::Pos,
          ThemePropertyName::Size,
          ThemePropertyName::TextColor,
          ThemePropertyName::IconColor,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::IconUpDown,
          ThemePropertyName::IconLeftRight,
          ThemePropertyName::IconUpDownLeftRight,
          ThemePropertyName::IconA,
          ThemePropertyName::IconB,
          ThemePropertyName::IconX,
          ThemePropertyName::IconY,
          ThemePropertyName::IconL,
          ThemePropertyName::IconL2,
          ThemePropertyName::IconL2R2,
          ThemePropertyName::IconL3,
          ThemePropertyName::IconL3R3,
          ThemePropertyName::IconLR,
          ThemePropertyName::IconR,
          ThemePropertyName::IconR2,
          ThemePropertyName::IconR3,
          ThemePropertyName::IconStart,
          ThemePropertyName::IconSelect,
          ThemePropertyName::IconHotkey,
          ThemePropertyName::IconHkA,
          ThemePropertyName::IconHkB,
          ThemePropertyName::IconHkL,
          ThemePropertyName::IconHkR,
          ThemePropertyName::IconHkX,
          ThemePropertyName::IconHkY,
          ThemePropertyName::IconHkLeftRight,
          ThemePropertyName::IconJ1UpDownLeftRight,
          ThemePropertyName::IconJ1UpDown,
          ThemePropertyName::IconJ1LeftRight,
          ThemePropertyName::IconJ2UpDownLeftRight,
          ThemePropertyName::IconJ2UpDown,
          ThemePropertyName::IconJ2LeftRight,
        },
      },
      {
        ThemeElementType::Carousel,
        {
          ThemePropertyName::Type,
          ThemePropertyName::Size,
          ThemePropertyName::Pos,
          ThemePropertyName::Origin,
          ThemePropertyName::Color,
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::FontColor,
          ThemePropertyName::LogoScale,
          ThemePropertyName::LogoRotation,
          ThemePropertyName::LogoRotationOrigin,
          ThemePropertyName::LogoSize,
          ThemePropertyName::TextOnly,
          ThemePropertyName::ForceUppercase,
          ThemePropertyName::LogoAlignment,
          ThemePropertyName::MaxLogoCount,
          ThemePropertyName::SelectorColor,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::PrimaryColor,
          ThemePropertyName::SecondaryColor,
          ThemePropertyName::SelectorHeight,
          ThemePropertyName::SelectorOffsetX,
          ThemePropertyName::SelectorOffsetY,
          ThemePropertyName::TextOffsetX,
          ThemePropertyName::LineSpacing,
          ThemePropertyName::HorizontalMargin,
          ThemePropertyName::DefaultTransition,
          ThemePropertyName::ZIndex,
        },
      },
      {
        ThemeElementType::MenuBackground,
        {
          ThemePropertyName::Color,
          ThemePropertyName::Path,
          ThemePropertyName::FadePath,
        },
      },
      {
        ThemeElementType::MenuIcons,
        {
          ThemePropertyName::IconKodi,
          ThemePropertyName::IconSystem,
          ThemePropertyName::IconUpdates,
          ThemePropertyName::IconControllers,
          ThemePropertyName::IconGames,
          ThemePropertyName::IconUI,
          ThemePropertyName::IconSound,
          ThemePropertyName::IconNetwork,
          ThemePropertyName::IconScraper,
          ThemePropertyName::IconAdvanced,
          ThemePropertyName::IconQuit,
          ThemePropertyName::IconRestart,
          ThemePropertyName::IconShutdown,
          ThemePropertyName::IconFastShutdown,
          ThemePropertyName::IconLicense,
          ThemePropertyName::IconLists,
          ThemePropertyName::IconBios,
          ThemePropertyName::IconRecalboxRGBDual,
          ThemePropertyName::IconTate,
          ThemePropertyName::IconArcade,
          ThemePropertyName::IconDownload,
          ThemePropertyName::IconThemes,
          ThemePropertyName::IconCardReader,
        },
      },
      {
        ThemeElementType::MenuSwitch,
        {
          ThemePropertyName::PathOn,
          ThemePropertyName::PathOff,
        },
      },
      {
        ThemeElementType::MenuSlider,
        {
          ThemePropertyName::Path,
        },
      },
      {
        ThemeElementType::MenuButton,
        {
          ThemePropertyName::Path,
          ThemePropertyName::FilledPath,
        },
      },
      {
        ThemeElementType::MenuText,
        {
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Color,
          ThemePropertyName::SeparatorColor,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::SelectorColor,
        },
      },
      {
        ThemeElementType::MenuTextSmall,
        {
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Color,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::SelectorColor,
        },
      },
      {
        ThemeElementType::MenuSection,
        {
          ThemePropertyName::FontPath,
          ThemePropertyName::FontSize,
          ThemePropertyName::Color,
          ThemePropertyName::SelectedColor,
          ThemePropertyName::Alignment,
        },
      },
      {
        ThemeElementType::MenuSize,
        {
          ThemePropertyName::Height,
        },
      },
      {
        ThemeElementType::Keyboard,
        {
          // Colors of the virtual keyboards. Size is always decided by the VK,
          // so no size/pos here. fontPath sets the font face (size stays VK-driven).
          ThemePropertyName::KeyColor,
          ThemePropertyName::KeySelectedColor,
          ThemePropertyName::KeyTextColor,
          ThemePropertyName::KeyDisabledColor,
          ThemePropertyName::KeyModifierColor,
          ThemePropertyName::KeyModifierLockedColor,
          ThemePropertyName::KeyTitleColor,
          ThemePropertyName::KeyEditTextColor,
          ThemePropertyName::FontPath,
        },
      }
    };
  return sElementMap;
}

HashMap<String, ThemeElementType>& ThemeSupport::ElementType()
{
  static HashMap<String, ThemeElementType> sTypes
  {
    { "image", ThemeElementType::Image },
    { "box", ThemeElementType::Box },
    { "video", ThemeElementType::Video },
    { "text", ThemeElementType::Text },
    { "scrolltext", ThemeElementType::ScrollText },
    { "markdown", ThemeElementType::Markdown },
    { "textlist", ThemeElementType::TextList },
    { "container", ThemeElementType::Container },
    { "ninepatch", ThemeElementType::NinePatch },
    { "datetime", ThemeElementType::DateTime },
    { "rating", ThemeElementType::Rating },
    { "sound", ThemeElementType::Sound },
    { "helpsystem", ThemeElementType::HelpSystem },
    { "carousel", ThemeElementType::Carousel },
    { "menuBackground", ThemeElementType::MenuBackground },
    { "menuIcons", ThemeElementType::MenuIcons },
    { "menuSwitch", ThemeElementType::MenuSwitch },
    { "menuSlider", ThemeElementType::MenuSlider },
    { "menuButton", ThemeElementType::MenuButton },
    { "menuText", ThemeElementType::MenuText },
    { "menuTextSmall", ThemeElementType::MenuTextSmall },
    { "menuSection", ThemeElementType::MenuSection },
    { "menuSize", ThemeElementType::MenuSize },
    { "keyboard", ThemeElementType::Keyboard },
  };

  return sTypes;
}

HashMap<String, ThemePropertyName>& ThemeSupport::PropertyName()
{
  static HashMap<String, ThemePropertyName> sNames
  {
    { "alignment", ThemePropertyName::Alignment },
    { "animations", ThemePropertyName::Animations },
    { "backgroundColor", ThemePropertyName::BackgroundColor },
    { "color", ThemePropertyName::Color },
    { "colorTop", ThemePropertyName::ColorTop },
    { "colorBottom", ThemePropertyName::ColorBottom },
    { "colorLeft", ThemePropertyName::ColorLeft },
    { "colorRight", ThemePropertyName::ColorRight },
    { "colorTopLeft", ThemePropertyName::ColorTopLeft },
    { "colorTopRight", ThemePropertyName::ColorTopRight },
    { "colorBottomLeft", ThemePropertyName::ColorBottomLeft },
    { "colorBottomRight", ThemePropertyName::ColorBottomRight },
    { "defaultTransition", ThemePropertyName::DefaultTransition },
    { "delay", ThemePropertyName::Delay },
    { "disabled", ThemePropertyName::Disabled },
    { "display", ThemePropertyName::Display },
    { "fadePath", ThemePropertyName::FadePath },
    { "filledPath", ThemePropertyName::FilledPath },
    { "fontPath", ThemePropertyName::FontPath },
    { "fontSize", ThemePropertyName::FontSize },
    { "fontStyle", ThemePropertyName::FontStyle },
    { "fontColor", ThemePropertyName::FontColor },
    { "forceUppercase", ThemePropertyName::ForceUppercase },
    { "height", ThemePropertyName::Height },
    { "horizontalMargin", ThemePropertyName::HorizontalMargin },
    { "iconA", ThemePropertyName::IconA },
    { "iconAdvanced", ThemePropertyName::IconAdvanced },
    { "iconArcade", ThemePropertyName::IconArcade },
    { "iconB", ThemePropertyName::IconB },
    { "iconBios", ThemePropertyName::IconBios },
    { "iconCardReader", ThemePropertyName::IconCardReader },
    { "iconColor", ThemePropertyName::IconColor },
    { "iconControllers", ThemePropertyName::IconControllers },
    { "iconDownload", ThemePropertyName::IconDownload },
    { "iconFastShutdown", ThemePropertyName::IconFastShutdown },
    { "iconGames", ThemePropertyName::IconGames },
    { "iconHkA", ThemePropertyName::IconHkA },
    { "iconHkB", ThemePropertyName::IconHkB },
    { "iconHkL", ThemePropertyName::IconHkL },
    { "iconHkR", ThemePropertyName::IconHkR },
    { "iconHkX", ThemePropertyName::IconHkX },
    { "iconHkY", ThemePropertyName::IconHkY },
    { "iconHkLeftRight", ThemePropertyName::IconHkLeftRight },
    { "iconHotkey", ThemePropertyName::IconHotkey },
    { "iconJ1LeftRight", ThemePropertyName::IconJ1LeftRight },
    { "iconJ1UpDown", ThemePropertyName::IconJ1UpDown },
    { "iconJ1UpDownLeftRight", ThemePropertyName::IconJ1UpDownLeftRight },
    { "iconJ2LeftRight", ThemePropertyName::IconJ2LeftRight },
    { "iconJ2UpDown", ThemePropertyName::IconJ2UpDown },
    { "iconJ2UpDownLeftRight", ThemePropertyName::IconJ2UpDownLeftRight },
    { "iconKodi", ThemePropertyName::IconKodi },
    { "iconL", ThemePropertyName::IconL },
    { "iconL2", ThemePropertyName::IconL2 },
    { "iconL2R2", ThemePropertyName::IconL2R2 },
    { "iconL3", ThemePropertyName::IconL3 },
    { "iconL3R3", ThemePropertyName::IconL3R3 },
    { "iconLeftRight", ThemePropertyName::IconLeftRight },
    { "iconLicense", ThemePropertyName::IconLicense },
    { "iconList", ThemePropertyName::IconLists },
    { "iconLR", ThemePropertyName::IconLR },
    { "iconNetwork", ThemePropertyName::IconNetwork },
    { "iconQuit", ThemePropertyName::IconQuit },
    { "iconR", ThemePropertyName::IconR },
    { "iconR2", ThemePropertyName::IconR2 },
    { "iconR3", ThemePropertyName::IconR3 },
    { "iconRecalboxRGBDual", ThemePropertyName::IconRecalboxRGBDual },
    { "iconRestart", ThemePropertyName::IconRestart },
    { "iconScraper", ThemePropertyName::IconScraper },
    { "iconSelect", ThemePropertyName::IconSelect },
    { "iconShutdown", ThemePropertyName::IconShutdown },
    { "iconSound", ThemePropertyName::IconSound },
    { "iconStart", ThemePropertyName::IconStart },
    { "iconSystem", ThemePropertyName::IconSystem },
    { "iconTate", ThemePropertyName::IconTate },
    { "iconThemes", ThemePropertyName::IconThemes },
    { "iconUI", ThemePropertyName::IconUI },
    { "iconUpDown", ThemePropertyName::IconUpDown },
    { "iconUpDownLeftRight", ThemePropertyName::IconUpDownLeftRight },
    { "iconUpdates", ThemePropertyName::IconUpdates },
    { "iconX", ThemePropertyName::IconX },
    { "iconY", ThemePropertyName::IconY },
    { "keepratio", ThemePropertyName::KeepRatio },
    { "lineSpacing", ThemePropertyName::LineSpacing },
    { "link", ThemePropertyName::Link },
    { "logoAlignment", ThemePropertyName::LogoAlignment },
    { "logoRotation", ThemePropertyName::LogoRotation },
    { "logoRotationOrigin", ThemePropertyName::LogoRotationOrigin },
    { "logoScale", ThemePropertyName::LogoScale },
    { "logoSize", ThemePropertyName::LogoSize },
    { "textOnly", ThemePropertyName::TextOnly },
    { "loops", ThemePropertyName::Loops },
    { "maxLogoCount", ThemePropertyName::MaxLogoCount },
    { "maxSize", ThemePropertyName::MaxSize },
    { "multiline", ThemePropertyName::Multiline },
    { "origin", ThemePropertyName::Origin },
    { "path", ThemePropertyName::Path },
    { "pathOff", ThemePropertyName::PathOff },
    { "pathOn", ThemePropertyName::PathOn },
    { "pos", ThemePropertyName::Pos },
    { "primaryColor", ThemePropertyName::PrimaryColor },
    { "reflection", ThemePropertyName::Reflection },
    { "rotation", ThemePropertyName::Rotation },
    { "rotationOrigin", ThemePropertyName::RotationOrigin },
    { "scrollSound", ThemePropertyName::ScrollSound },
    { "secondaryColor", ThemePropertyName::SecondaryColor },
    { "selectedColor", ThemePropertyName::SelectedColor },
    { "selectorColor", ThemePropertyName::SelectorColor },
    { "selectorHeight", ThemePropertyName::SelectorHeight },
    { "selectorImagePath", ThemePropertyName::SelectorImagePath },
    { "selectorImageTile", ThemePropertyName::SelectorImageTile },
    { "selectorOffsetX", ThemePropertyName::SelectorOffsetX },
    { "selectorOffsetY", ThemePropertyName::SelectorOffsetY },
    { "separatorColor", ThemePropertyName::SeparatorColor },
    { "size", ThemePropertyName::Size },
    { "text", ThemePropertyName::Text },
    { "textColor", ThemePropertyName::TextColor },
    { "textOffsetX", ThemePropertyName::TextOffsetX },
    { "tile", ThemePropertyName::Tile },
    { "type", ThemePropertyName::Type },
    { "unfilledPath", ThemePropertyName::UnfilledPath },
    { "zIndex", ThemePropertyName::ZIndex },
    { "keyColor", ThemePropertyName::KeyColor },
    { "keySelectedColor", ThemePropertyName::KeySelectedColor },
    { "keyTextColor", ThemePropertyName::KeyTextColor },
    { "keyDisabledColor", ThemePropertyName::KeyDisabledColor },
    { "keyModifierColor", ThemePropertyName::KeyModifierColor },
    { "keyModifierLockedColor", ThemePropertyName::KeyModifierLockedColor },
    { "keyTitleColor", ThemePropertyName::KeyTitleColor },
    { "keyEditTextColor", ThemePropertyName::KeyEditTextColor },
  };

  return sNames;
}

String ThemeSupport::ReversePropertyName(ThemePropertyName name)
{
  static HashMap<ThemePropertyName, String> sNames
  {
    { ThemePropertyName::Alignment, "alignment" },
    { ThemePropertyName::Animations, "animations" },
    { ThemePropertyName::BackgroundColor, "backgroundColor" },
    { ThemePropertyName::Color, "color" },
    { ThemePropertyName::ColorTop, "colorTop" },
    { ThemePropertyName::ColorBottom, "colorBottom" },
    { ThemePropertyName::ColorLeft, "colorLeft" },
    { ThemePropertyName::ColorRight, "colorRight" },
    { ThemePropertyName::ColorTopLeft, "colorTopLeft" },
    { ThemePropertyName::ColorTopRight, "colorTopRight" },
    { ThemePropertyName::ColorBottomLeft, "colorBottomLeft" },
    { ThemePropertyName::ColorBottomRight, "colorBottomRight" },
    { ThemePropertyName::DefaultTransition, "defaultTransition" },
    { ThemePropertyName::Delay, "delay" },
    { ThemePropertyName::Disabled, "disabled" },
    { ThemePropertyName::Display, "display" },
    { ThemePropertyName::FadePath, "fadePath" },
    { ThemePropertyName::FilledPath, "filledPath" },
    { ThemePropertyName::FontPath, "fontPath" },
    { ThemePropertyName::FontSize, "fontSize" },
    { ThemePropertyName::FontStyle, "fontStyle" },
    { ThemePropertyName::FontColor, "fontColor" },
    { ThemePropertyName::ForceUppercase, "forceUppercase" },
    { ThemePropertyName::Height, "height" },
    { ThemePropertyName::HorizontalMargin, "horizontalMargin" },
    { ThemePropertyName::IconA, "iconA" },
    { ThemePropertyName::IconAdvanced, "iconAdvanced" },
    { ThemePropertyName::IconArcade, "iconArcade" },
    { ThemePropertyName::IconB, "iconB" },
    { ThemePropertyName::IconBios, "iconBios" },
    { ThemePropertyName::IconCardReader, "iconCardReader" },
    { ThemePropertyName::IconColor, "iconColor" },
    { ThemePropertyName::IconControllers, "iconControllers" },
    { ThemePropertyName::IconDownload, "iconDownload" },
    { ThemePropertyName::IconFastShutdown, "iconFastShutdown" },
    { ThemePropertyName::IconGames, "iconGames" },
    { ThemePropertyName::IconHkA, "iconHkA" },
    { ThemePropertyName::IconHkB, "iconHkB" },
    { ThemePropertyName::IconHkL, "iconHkL" },
    { ThemePropertyName::IconHkR, "iconHkR" },
    { ThemePropertyName::IconHkX, "iconHkX" },
    { ThemePropertyName::IconHkY, "iconHkY" },
    { ThemePropertyName::IconHkLeftRight, "iconHkLeftRight" },
    { ThemePropertyName::IconHotkey, "iconHotkey" },
    { ThemePropertyName::IconJ1LeftRight, "iconJ1LeftRight"},
    { ThemePropertyName::IconJ1UpDown, "iconJ1UpDown"},
    { ThemePropertyName::IconJ1UpDownLeftRight, "iconJ1UpDownLeftRight"},
    { ThemePropertyName::IconJ2LeftRight, "iconJ2LeftRight"},
    { ThemePropertyName::IconJ2UpDown, "iconJ2UpDown"},
    { ThemePropertyName::IconJ2UpDownLeftRight, "iconJ2UpDownLeftRight"},
    { ThemePropertyName::IconKodi, "iconKodi" },
    { ThemePropertyName::IconL, "iconL" },
    { ThemePropertyName::IconL2, "iconL2" },
    { ThemePropertyName::IconL2R2, "iconL2R2" },
    { ThemePropertyName::IconL3, "iconL3" },
    { ThemePropertyName::IconL3R3, "iconL3R3" },
    { ThemePropertyName::IconLeftRight, "iconLeftRight" },
    { ThemePropertyName::IconLicense, "iconLicense" },
    { ThemePropertyName::IconLists, "iconList" },
    { ThemePropertyName::IconLR, "iconLR" },
    { ThemePropertyName::IconNetwork, "iconNetwork" },
    { ThemePropertyName::IconQuit, "iconQuit" },
    { ThemePropertyName::IconR, "iconR" },
    { ThemePropertyName::IconR2, "iconR2" },
    { ThemePropertyName::IconR3, "iconR3" },
    { ThemePropertyName::IconRecalboxRGBDual, "iconRecalboxRGBDual" },
    { ThemePropertyName::IconRestart, "iconRestart" },
    { ThemePropertyName::IconScraper, "iconScraper" },
    { ThemePropertyName::IconSelect, "iconSelect" },
    { ThemePropertyName::IconShutdown, "iconShutdown" },
    { ThemePropertyName::IconSound, "iconSound" },
    { ThemePropertyName::IconStart, "iconStart" },
    { ThemePropertyName::IconSystem, "iconSystem" },
    { ThemePropertyName::IconTate, "iconTate" },
    { ThemePropertyName::IconThemes, "iconThemes" },
    { ThemePropertyName::IconUI, "iconUI" },
    { ThemePropertyName::IconUpDown, "iconUpDown" },
    { ThemePropertyName::IconUpDownLeftRight, "iconUpDownLeftRight" },
    { ThemePropertyName::IconUpdates, "iconUpdates" },
    { ThemePropertyName::IconX, "iconX" },
    { ThemePropertyName::IconY, "iconY" },
    { ThemePropertyName::KeepRatio, "keepratio" },
    { ThemePropertyName::LineSpacing, "lineSpacing" },
    { ThemePropertyName::Link, "link" },
    { ThemePropertyName::LogoAlignment, "logoAlignment" },
    { ThemePropertyName::LogoRotation, "logoRotation" },
    { ThemePropertyName::LogoRotationOrigin, "logoRotationOrigin" },
    { ThemePropertyName::LogoScale, "logoScale" },
    { ThemePropertyName::LogoSize, "logoSize" },
    { ThemePropertyName::TextOnly, "textOnly" },
    { ThemePropertyName::Loops, "loops" },
    { ThemePropertyName::MaxLogoCount, "maxLogoCount" },
    { ThemePropertyName::MaxSize, "maxSize" },
    { ThemePropertyName::Origin, "origin" },
    { ThemePropertyName::Path, "path" },
    { ThemePropertyName::PathOff, "pathOff" },
    { ThemePropertyName::PathOn, "pathOn" },
    { ThemePropertyName::Pos, "pos" },
    { ThemePropertyName::PrimaryColor, "primaryColor" },
    { ThemePropertyName::Rotation, "rotation" },
    { ThemePropertyName::RotationOrigin, "rotationOrigin" },
    { ThemePropertyName::ScrollSound, "scrollSound" },
    { ThemePropertyName::SecondaryColor, "secondaryColor" },
    { ThemePropertyName::SelectedColor, "selectedColor" },
    { ThemePropertyName::SelectorColor, "selectorColor" },
    { ThemePropertyName::SelectorHeight, "selectorHeight" },
    { ThemePropertyName::SelectorImagePath, "selectorImagePath" },
    { ThemePropertyName::SelectorImageTile, "selectorImageTile" },
    { ThemePropertyName::SelectorOffsetX, "selectorOffsetX" },
    { ThemePropertyName::SelectorOffsetY, "selectorOffsetY" },
    { ThemePropertyName::SeparatorColor, "separatorColor" },
    { ThemePropertyName::Size, "size" },
    { ThemePropertyName::Text, "text" },
    { ThemePropertyName::TextColor, "textColor" },
    { ThemePropertyName::TextOffsetX, "textOffsetX" },
    { ThemePropertyName::Tile, "tile" },
    { ThemePropertyName::Type, "type" },
    { ThemePropertyName::UnfilledPath, "unfilledPath" },
    { ThemePropertyName::ZIndex, "zIndex" },
    { ThemePropertyName::KeyColor, "keyColor" },
    { ThemePropertyName::KeySelectedColor, "keySelectedColor" },
    { ThemePropertyName::KeyTextColor, "keyTextColor" },
    { ThemePropertyName::KeyDisabledColor, "keyDisabledColor" },
    { ThemePropertyName::KeyModifierColor, "keyModifierColor" },
    { ThemePropertyName::KeyModifierLockedColor, "keyModifierLockedColor" },
    { ThemePropertyName::KeyTitleColor, "keyTitleColor" },
    { ThemePropertyName::KeyEditTextColor, "keyEditTextColor" },
  };

  String* sname = sNames.try_get(name);
  return sname != nullptr ? *sname : "Unknown";
}
