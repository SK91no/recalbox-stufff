#include <themes/MenuThemeData.h>
#include <rendering/fonts/FontManager.h>

void MenuThemeData::Load(ThemeData& mainTheme)
{
  Reset();

	if (const ThemeElement* elem = mainTheme.Element("menu", "menuSize", ThemeElementType::MenuSize, ThemeElementType::None); elem != nullptr)
		if (elem->HasProperty(ThemePropertyName::Height))
			mSize.height = elem->AsFloat(ThemePropertyName::Height);

	if (const ThemeElement* elem = mainTheme.Element("menu", "menubg", ThemeElementType::MenuBackground, ThemeElementType::None); elem != nullptr)
	{
		if (elem->HasProperty(ThemePropertyName::Path))
			mBackground.path = Path(elem->AsString(ThemePropertyName::Path));
		if (elem->HasProperty(ThemePropertyName::FadePath))
			mBackground.fadePath = Path(elem->AsString(ThemePropertyName::FadePath));
		if (elem->HasProperty(ThemePropertyName::Color))
			mBackground.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menutitle", ThemeElementType::MenuText, ThemeElementType::None); elem != nullptr)
	{
		if(elem->HasProperty(ThemePropertyName::FontPath) || elem->HasProperty(ThemePropertyName::FontSize))
			mTitle.font = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false));
		if(elem->HasProperty(ThemePropertyName::Color))
			mTitle.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menufooter", ThemeElementType::MenuText, ThemeElementType::None); elem != nullptr)
	{
    if(elem->HasProperty(ThemePropertyName::FontPath) || elem->HasProperty(ThemePropertyName::FontSize))
			mFooter.font = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));
		if(elem->HasProperty(ThemePropertyName::Color))
			mFooter.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menutext", ThemeElementType::MenuText, ThemeElementType::None); elem != nullptr)
	{
    if(elem->HasProperty(ThemePropertyName::FontPath) || elem->HasProperty(ThemePropertyName::FontSize))
			mText.font = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false));

		if(elem->HasProperty(ThemePropertyName::Color))          mText.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
		if(elem->HasProperty(ThemePropertyName::SeparatorColor)) mText.separatorColor = (unsigned int)elem->AsInt(ThemePropertyName::SeparatorColor);
		if(elem->HasProperty(ThemePropertyName::SelectedColor))  mText.selectedColor = (unsigned int)elem->AsInt(ThemePropertyName::SelectedColor);
		if(elem->HasProperty(ThemePropertyName::SelectorColor))  mText.selectorColor = (unsigned int)elem->AsInt(ThemePropertyName::SelectorColor);
	}

  mSection = mText;
  if (const ThemeElement* elem = mainTheme.Element("menu", "menusection", ThemeElementType::MenuSection, ThemeElementType::None); elem != nullptr)
  {
    if(elem->HasProperty(ThemePropertyName::FontPath) || elem->HasProperty(ThemePropertyName::FontSize))
      mSection.font = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::Font, mText.font);
    if(elem->HasProperty(ThemePropertyName::Color))          mSection.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
    if(elem->HasProperty(ThemePropertyName::SelectedColor))  mSection.selectedColor = (unsigned int)elem->AsInt(ThemePropertyName::SelectedColor);
    if(elem->HasProperty(ThemePropertyName::Alignment))
    {
      String align = elem->AsString(ThemePropertyName::Alignment);
      if (align == "left") mSection.alignment = Alignment::CenterLeft;
      else if (align == "right") mSection.alignment = Alignment::CenterRight;
      else mSection.alignment = Alignment::Center;
    }
  }

  mTextSmall = mText;
	if (const ThemeElement* elem = mainTheme.Element("menu", "menutextsmall", ThemeElementType::MenuTextSmall, ThemeElementType::None); elem != nullptr)
	{
    if(elem->HasProperty(ThemePropertyName::FontPath) || elem->HasProperty(ThemePropertyName::FontSize))
      mTextSmall.font = &FontManager::Instance().FromTheme(*elem, ThemePropertyCategory::All, &FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false));

		if(elem->HasProperty(ThemePropertyName::Color))         mTextSmall.color = (unsigned int)elem->AsInt(ThemePropertyName::Color);
		if(elem->HasProperty(ThemePropertyName::SelectedColor)) mText.selectedColor = (unsigned int)elem->AsInt(ThemePropertyName::SelectedColor);
		if(elem->HasProperty(ThemePropertyName::SelectorColor)) mText.selectedColor = (unsigned int)elem->AsInt(ThemePropertyName::SelectorColor);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menubutton", ThemeElementType::MenuButton, ThemeElementType::None); elem != nullptr)
	{
		if(elem->HasProperty(ThemePropertyName::Path))       mIconElement.mButton = elem->AsString(ThemePropertyName::Path);
		if(elem->HasProperty(ThemePropertyName::FilledPath)) mIconElement.mButtonFilled = elem->AsString(ThemePropertyName::FilledPath);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menuswitch", ThemeElementType::MenuSwitch, ThemeElementType::None); elem != nullptr)
	{
		if(elem->HasProperty(ThemePropertyName::PathOn))  mIconElement.mOn = elem->AsString(ThemePropertyName::PathOn);
		if(elem->HasProperty(ThemePropertyName::PathOff)) mIconElement.mOff = elem->AsString(ThemePropertyName::PathOff);
	}

	if (const ThemeElement* elem = mainTheme.Element("menu", "menuslider", ThemeElementType::MenuSlider, ThemeElementType::None); elem != nullptr)
		if (elem->HasProperty(ThemePropertyName::Path)) mIconElement.mKnob = elem->AsString(ThemePropertyName::Path);

	if (const ThemeElement* elem = mainTheme.Element("menu", "menuicons", ThemeElementType::MenuIcons, ThemeElementType::None); elem != nullptr)
    {
		if (elem->HasProperty(ThemePropertyName::IconKodi))            mIconSet.mKodi = elem->AsString(ThemePropertyName::IconKodi);
		if (elem->HasProperty(ThemePropertyName::IconSystem))          mIconSet.mSystem = elem->AsString(ThemePropertyName::IconSystem);
		if (elem->HasProperty(ThemePropertyName::IconUpdates))         mIconSet.mUpdates = elem->AsString(ThemePropertyName::IconUpdates);
		if (elem->HasProperty(ThemePropertyName::IconGames))           mIconSet.mGames = elem->AsString(ThemePropertyName::IconGames);
		if (elem->HasProperty(ThemePropertyName::IconControllers))     mIconSet.mControllers = elem->AsString(ThemePropertyName::IconControllers);
		if (elem->HasProperty(ThemePropertyName::IconUI))              mIconSet.mUI = elem->AsString(ThemePropertyName::IconUI);
		if (elem->HasProperty(ThemePropertyName::IconSound))           mIconSet.mSound = elem->AsString(ThemePropertyName::IconSound);
		if (elem->HasProperty(ThemePropertyName::IconNetwork))         mIconSet.mNetwork = elem->AsString(ThemePropertyName::IconNetwork);
		if (elem->HasProperty(ThemePropertyName::IconScraper))         mIconSet.mScraper = elem->AsString(ThemePropertyName::IconScraper);
		if (elem->HasProperty(ThemePropertyName::IconAdvanced))        mIconSet.mAdvanced = elem->AsString(ThemePropertyName::IconAdvanced);
		if (elem->HasProperty(ThemePropertyName::IconQuit))            mIconSet.mQuit = elem->AsString(ThemePropertyName::IconQuit);
		if (elem->HasProperty(ThemePropertyName::IconRestart))         mIconSet.mRestart = elem->AsString(ThemePropertyName::IconRestart);
		if (elem->HasProperty(ThemePropertyName::IconShutdown))        mIconSet.mShutdown = elem->AsString(ThemePropertyName::IconShutdown);
		if (elem->HasProperty(ThemePropertyName::IconFastShutdown))    mIconSet.mFastShutdown = elem->AsString(ThemePropertyName::IconFastShutdown);
		if (elem->HasProperty(ThemePropertyName::IconLicense))         mIconSet.mLicense = elem->AsString(ThemePropertyName::IconLicense);
		if (elem->HasProperty(ThemePropertyName::IconLists))           mIconSet.mLists = elem->AsString(ThemePropertyName::IconLists);
		if (elem->HasProperty(ThemePropertyName::IconRecalboxRGBDual)) mIconSet.mRecalboxRGBDual = elem->AsString(ThemePropertyName::IconRecalboxRGBDual);
		if (elem->HasProperty(ThemePropertyName::IconTate))            mIconSet.mTate = elem->AsString(ThemePropertyName::IconTate);
		if (elem->HasProperty(ThemePropertyName::IconArcade))          mIconSet.mArcade = elem->AsString(ThemePropertyName::IconArcade);
		if (elem->HasProperty(ThemePropertyName::IconDownload))        mIconSet.mDownload = elem->AsString(ThemePropertyName::IconDownload);
		if (elem->HasProperty(ThemePropertyName::IconBios))            mIconSet.mBios = elem->AsString(ThemePropertyName::IconBios);
		if (elem->HasProperty(ThemePropertyName::IconThemes))          mIconSet.mThemes = elem->AsString(ThemePropertyName::IconThemes);
		if (elem->HasProperty(ThemePropertyName::IconCardReader))      mIconSet.mCardReader = elem->AsString(ThemePropertyName::IconCardReader);
	}
}

void MenuThemeData::Reset()
{
  mBackground  = { .alignment=Alignment::Center, .color=0xFFFFFFFF, .selectedColor=0xFFFFFFFF, .selectorColor=0xFFFFFFFF, .separatorColor=0xFFFFFFFF, .path=Path(":/frame.png"), .fadePath=Path(":/scroll_gradient.png"), .font=nullptr                     };
  mTitle       = { .alignment=Alignment::Center, .color=0x555555FF, .selectedColor=0x555555FF, .selectorColor=0x555555FF, .separatorColor=0xFFFFFFFF, .path=Path::Empty        , .fadePath=Path::Empty                  , .font=&FontManager::Instance().FromDefault(FONT_SIZE_LARGE, false)  };
  mFooter      = { .alignment=Alignment::Center, .color=0xC6C6C6FF, .selectedColor=0xC6C6C6FF, .selectorColor=0xC6C6C6FF, .separatorColor=0xFFFFFFFF, .path=Path::Empty        , .fadePath=Path::Empty                  , .font=&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false)  };
  mText        = { .alignment=Alignment::Center, .color=0x777777FF, .selectedColor=0xFFFFFFFF, .selectorColor=0x878787FF, .separatorColor=0xC6C7C6FF, .path=Path::Empty        , .fadePath=Path::Empty                  , .font=&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false) };
  mSection     = { .alignment=Alignment::Center, .color=0x7F3300FF, .selectedColor=0xFFFFFFFF, .selectorColor=0x878787FF, .separatorColor=0xC6C7C6FF, .path=Path::Empty        , .fadePath=Path::Empty                  , .font=&FontManager::Instance().FromDefault(FONT_SIZE_MEDIUM, false) };
  mTextSmall   = { .alignment=Alignment::Center, .color=0x777777FF, .selectedColor=0xFFFFFFFF, .selectorColor=0x878787FF, .separatorColor=0xC6C7C6FF, .path=Path::Empty        , .fadePath=Path::Empty                  , .font=&FontManager::Instance().FromDefault(FONT_SIZE_SMALL, false)  };
  mIconSet.Reset();
  mIconElement.Reset();
  mSize        = { .height =  0.85f };
}

void MenuThemeData::IconElement::Reset()
{
  bool is240p = Renderer::Instance().Is240p();
  mButton = Path(is240p ? ":/button_small.png" : ":/button.png");
  mButtonFilled = Path(is240p ? ":/button_small_filled.png" : ":/button_filled.png");
  mOn = Path(":/on.svg");
  mOff = Path(":/off.svg");
  mOptionArrow = Path(":/option_arrow.svg");
  mArrow = Path(":/arrow.svg");
  mKnob = Path(":/slider_knob.svg");
  mCheckBox = Path(":/checkbox_unchecked.svg");
  mCheckBoxFilled = Path(":/checkbox_checked.svg");
}

MenuThemeData::MenuIcons::Type MenuThemeData::MenuIcons::IconFromString(const String& iconName)
{
  if (iconName == "Kodi"        ) return Type::Kodi;
  if (iconName == "System"      ) return Type::System;
  if (iconName == "Updates"     ) return Type::Updates;
  if (iconName == "InGame"      ) return Type::InGame;
  if (iconName == "Controllers" ) return Type::Controllers;
  if (iconName == "UI"          ) return Type::Ui;
  if (iconName == "Sound"       ) return Type::Sound;
  if (iconName == "Network"     ) return Type::Network;
  if (iconName == "Scraper"     ) return Type::Scraper;
  if (iconName == "Advanced"    ) return Type::Advanced;
  if (iconName == "Quit"        ) return Type::Quit;
  if (iconName == "Restart"     ) return Type::Restart;
  if (iconName == "Shutdown"    ) return Type::Shutdown;
  if (iconName == "FastShutdown") return Type::Fastshutdown;
  if (iconName == "License"     ) return Type::License;
  if (iconName == "Lists"       ) return Type::Lists;
  if (iconName == "RRGBD"       ) return Type::RecalboxRGBDual;
  if (iconName == "Tate"        ) return Type::Tate;
  if (iconName == "Arcade"      ) return Type::Arcade;
  if (iconName == "Download"    ) return Type::Download;
  if (iconName == "Bios"        ) return Type::Bios;
  if (iconName == "Themes"      ) return Type::Themes;
  if (iconName == "CardReader"  ) return Type::CardReader;
  if (iconName == "JVS"         ) return Type::Jvs;
  return Type::Unknown;
}
