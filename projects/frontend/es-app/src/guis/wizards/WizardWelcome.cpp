//
// Created by bkg2k on 11/10/23.
//

#include "WizardWelcome.h"
#include "components/PictureComponent.h"
#include "themes/ThemeManager.h"
#include "guis/menus/MenuProvider.h"
#include "hardware/cardreader/CardReader.h"


WizardBase::Move WizardWelcome::OnKeyReceived(int page, const InputCompactEvent& event)
{
  (void)page;
  (void)event;
  return Move::None;
}

bool WizardWelcome::OnComponentRequired(int page, int componentIndex, Rectangle& where, std::shared_ptr<Component>& component)
{
  switch((Pages)page)
  {
    case Pages::Intro:
    {
      if (componentIndex == 0)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

        component = ShouldShowLite() ? std::make_shared<PictureComponent>(mWindow, Path(":/wizard/logo_recaltower_text_reduced.svg"), ImgProps::KeepRatio) : std::make_shared<PictureComponent>(mWindow, Path(":/recalbox.svg"), ImgProps::KeepRatio);
        component->setColor(menuTheme.Text().color);
        where = Rectangle(0, 0, 3, 15);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text = _("The Recalbox team thanks you for your trust!\n\n"
                        "Let's take advantage of this first launch to discover together how to use:\n- your Recalbox");

        if (ShouldShowLite())
          text.Append(' ').Append(_("and its Recaltower"));
        if (!ShouldShowJAMMA2() && !ShouldShowJVS())
          text.Append('\n').Append(_("- your controller"));
        if (ShouldShowDUAL2())
          text.Append('\n').Append(_("- your Recalbox RGB DUAL 2"));
        if (ShouldShowJAMMA2())
          text.Append('\n').Append(_("- your Recalbox RGB JAMMA 2"));
        if (ShouldShowJVS())
          text.Append("\n- your Recalbox JVS");
        if (ShouldShowCardReader())
          text.Append('\n').Append(_("- your Recalbox Card Reader"));

        if (!ShouldShowJAMMA2() && !ShouldShowJVS() && InputManager::Instance().ConfiguredControllersCount() == 0)
          text.Append('\n').Append('\n').Append(_("Please connect your controller via USB and press X to continue."));
        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(3, 1, 7, 9);
        return true;
      }
      break;
    }
    case Pages::Controller:
    {
      SetTitle(_("Controller"));
      if (componentIndex == 0)
      {
        component = std::make_shared<PictureComponent>(mWindow, Path(":/wizard/controller.svg"), ImgProps::KeepRatio);
        where = Rectangle(0, 6, 10, 4);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text = _(
            "You can navigate through the menus using the directional pad, **select a system**, or start a game with the **B button**. The **A button** allows you to exit a menu or game list.\n\n"
            "Access the **main menu** using the **START button**.\n"
            "To exit a game, press the SELECT and START buttons simultaneously.");
        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(0, 0, 10, 6);
        return true;
      }
      break;
    }
    case Pages::JAMMA2:
      {
        SetTitle(_("RGB JAMMA 2"));
        if (componentIndex == 0)
        {
          component = std::make_shared<PictureComponent>(mWindow, Path(":/wizard/arcade_panel.svg"), ImgProps::KeepRatio);
          where = Rectangle(0, 6, 10, 4);
          return true;
        }
        if (componentIndex == 1)
        {
          const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
          String text = _("You can navigate through the menus using the joystick, **select a system**, or start a game with the **button 1**. The **button 2** allows you to exit a menu or game list."
                          "\n\nAccess the **main menu** using the **START button**.\nTo **exit a game**, **press and hold START** for 4 seconds and release.");
          std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
          component = textComponent;
          where = Rectangle(0, 0, 10, 6);
          RecalboxConf::Instance().SetFirstInstallJamma2(false).Save();
          return true;
        }
        break;
      }
    case Pages::JVS:
      {
        SetTitle(_("Recalbox JVS"));
        if (componentIndex == 0)
        {
          component = std::make_shared<PictureComponent>(mWindow, Path(":/wizard/arcade_panel.svg"), ImgProps::KeepRatio);
          where = Rectangle(0, 6, 10, 4);
          return true;
        }
        if (componentIndex == 1)
        {
          const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
          String text = _("You can navigate through the menus using the joystick, **select a system**, or start a game with the **button 1**. The **button 2** allows you to exit a menu or game list."
                          "\n\nAccess the **main menu** using the **START button**.\nTo **exit a game**, **press and hold START** to access firmware menu.");
          std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
          component = textComponent;
          where = Rectangle(0, 0, 10, 6);
          RecalboxConf::Instance().SetFirstInstallJvs(false).Save();
          return true;
        }
        break;
      }
    case Pages::DUAL2:
    {
      SetTitle(_("RGB DUAL 2"));
      if (componentIndex == 0)
      {
        component = std::make_shared<PictureComponent>(mWindow, Path(":/crt/logorgbdual2.png"), ImgProps::KeepRatio);
        where = Rectangle(0, 0, 3, 10);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text = _("Your Recalbox RGB DUAL 2 has been detected and installed automatically! You can now enjoy **all your games** on Recalbox with **perfect pixels and frequency**!");
        if (Board::Instance().CrtBoard().IsCrtAdapterAttached())
          text.Append(_("\n\nConsider this: you can automatically switch back to **HDMI mode** by connecting an HDMI cable and restarting Recalbox!"));
        else
          text.Append(_("\n\nAn HDMI cable has been detected and the “HDMI priority” mode is enabled in the options. To switch back to CRT mode, disconnect the HDMI cable and restart, or disable the HDMI priority option in the Recalbox RGB DUAL 2 (START) menu options."));


        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(3, 0, 6, 10);

        RecalboxConf::Instance().SetFirstInstallDual2(false).Save();
        return true;
      }
      break;
    }
    case Pages::CardReader:
    {
      SetTitle(_("Recalbox Card Reader"));
      if (componentIndex == 0)
      {
        component = std::make_shared<PictureComponent>(mWindow, Path(":/wizard/cardreader.png"), ImgProps::KeepRatio);
        where = Rectangle(0, 6, 10, 4);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text = _("Your Recalbox Card Reader has been detected and **installed automatically!**\n\nYou can start using it right away by **inserting a card** into the reader and going to the game of your choice to **associate the game and card** using the **menu** available with the **START** button.");
        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(0, 0, 10, 6);
        RecalboxConf::Instance().SetFirstInstallCardReader(false).Save();
        return true;
      }
      break;
    }
    case Pages::AddGames:
    {
      SetTitle(_("Add games"));
      if (componentIndex == 0)
      {
        component = std::make_shared<PictureComponent>(mWindow, Path(":/win_network_share.png"), ImgProps::KeepRatio);
        where = Rectangle(0, 5, 10, 5);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text = _("Recalbox shares its ROM, BIOS, and save files folders on the local network. Adding your ROMs couldn't be easier: on your computer, search for your “recalbox” in the network shares.\n"
                        "Go to the ROMs folder, then copy your game to the folder corresponding to its system. For example, to add a “NES” game, copy it to the ‘roms/nes’ folder.");
        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(0, 0, 10, 5);
        return true;
      }
      break;
    }
    case Pages::Update:
    {
      SetTitle(_("Update"));
      if (componentIndex == 0)
      {
        component = std::make_shared<PictureComponent>(mWindow, Path(":/recalbox_qrcode.svg"), ImgProps::KeepRatio);
        where = Rectangle(0, 0, 2, 10);
        return true;
      }
      if (componentIndex == 1)
      {
        const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
        String text;
        if (ShouldShowLite())
        {
          text.Append(_("You are currently running a preview version of **Recalbox 10**. An update to the public version will be offered automatically when it becomes available for download.").Append("\n\n"));

         text.Append(_("Find useful information and tutorials at recalbox.com, or on the recalbox wiki by scanning the QR code."));
        }
        else
        {
          text.Append(_("Remember to connect your Recalbox to the internet to enjoy all its features!\n"));
          text.Append(_("Updates will be offered to you automatically.\n\n"
                          "Find useful information and tutorials at recalbox.com, or on the recalbox wiki by scanning the QR code."));
        }

        std::shared_ptr<MarkdownText> textComponent = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopLeft);
        component = textComponent;
        where = Rectangle(3, 0, 7, 10);
        RecalboxConf::Instance().SetFirstTimeUse(false).Save();
        return true;
      }
      break;
    }
    case Pages::Count: break;
  }

  return false;
}

bool WizardWelcome::OnButtonRequired(int page, int buttonIndex, String& buttonText)
{
  if ((Pages)page == Pages::Update && buttonIndex == 0)
  {
    buttonText = _("NETWORK SETTINGS");
    return true;
  }
  return false;
}

WizardBase::Move WizardWelcome::OnButtonClick(int page, int buttonIndex)
{
  if ((Pages)page == Pages::Update && buttonIndex == 0)
    MenuProvider::ShowMenu(MenuContainerType::Network, InheritableContext());
    //mWindow.pushGui(new MenuNetwork(mWindow));
  return Move::None;
}

WizardBase::Move WizardWelcome::OnNextClick(int nextPage)
{
  if ( ((Pages)nextPage == Pages::DUAL2 && !ShouldShowDUAL2())
    || ((Pages)nextPage == Pages::CardReader && !ShouldShowCardReader())
    || ((Pages)nextPage == Pages::JAMMA2 && !ShouldShowJAMMA2())
    || ((Pages)nextPage == Pages::JVS && !ShouldShowJVS())
    || ((Pages)nextPage == Pages::Controller && (ShouldShowJAMMA2() || ShouldShowJVS()))
    || ((Pages)nextPage >= Pages::AddGames && !ShouldShowFirstInstall()))
    return Move::Forward;
  return Move::None;
}

int WizardWelcome::FirstPage()
{
  if (ShouldShowFirstInstall()) return static_cast<int>(Pages::Intro);
  if (ShouldShowDUAL2()) return static_cast<int>(Pages::DUAL2);
  if (ShouldShowJAMMA2()) return static_cast<int>(Pages::JAMMA2);
  if (ShouldShowJVS()) return static_cast<int>(Pages::JVS);
  if (ShouldShowCardReader()) return static_cast<int>(Pages::CardReader);
  return 0;
}

bool WizardWelcome::ShouldShowLite()
{
  return RecalboxSystem::IsLiteVersion();
}

bool WizardWelcome::ShouldShowFirstInstall()
{
  return RecalboxConf::Instance().GetFirstTimeUse();
}

bool WizardWelcome::ShouldShowJAMMA2()
{
  return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma && RecalboxConf::Instance().GetFirstInstallJamma2();
}

bool WizardWelcome::ShouldShowJVS()
{
  return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS && RecalboxConf::Instance().GetFirstInstallJvs();
}

bool WizardWelcome::ShouldShowDUAL2()
{
  return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBDual2 && RecalboxConf::Instance().GetFirstInstallDual2();
}

bool WizardWelcome::ShouldShowCardReader()
{
  return CardReader::Instance().IsAvailable() && RecalboxConf::Instance().GetFirstInstallCardReader();
}

bool WizardWelcome::ShouldShowWizard()
{
  return  InputManager::Instance().ConfiguredControllersCount() > 0 && (ShouldShowFirstInstall() || ShouldShowDUAL2() || ShouldShowJAMMA2() || ShouldShowJVS() || ShouldShowCardReader());
}

