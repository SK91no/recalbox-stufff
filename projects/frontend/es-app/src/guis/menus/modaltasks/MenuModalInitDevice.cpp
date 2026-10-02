//
// Created by bkg2k on 25/11/24.
//

#include "MenuModalInitDevice.h"
#include "guis/GuiMsgBox.h"
#include "MainRunner.h"
#include "hardware/devices/storage/StorageDevices.h"
#include "recalbox/RecalboxSystem.h"
#include "utils/Files.h"

MenuModalInitDevice::MenuModalInitDevice(WindowManager& window, const DeviceMount& device, USBInitializationAction action)
  : IMenuModalTask<DeviceInitializationParameters, bool>(window,
                                                         action == USBInitializationAction::OnlyRomFolders ? _("Initializing roms folders on device ").Append(device.DisplayableDeviceName()) : _("Moving share to device ").Append(device.DisplayableDeviceName()),
                                                         DeviceInitializationParameters(action, device))
{
}

bool MenuModalInitDevice::TaskExecute(const DeviceInitializationParameters& parameter)
{
  switch(parameter.Action())
  {
    case USBInitializationAction::None: break;
    case USBInitializationAction::OnlyRomFolders: return SystemManager::CreateRomFoldersIn(parameter.Device());
    case USBInitializationAction::CompleteShare:
    {
      struct
      {
        Path From;
        Path To;
      }
        pathFromTo[]
        {
          { Path("/recalbox/share/bios")       , parameter.Device().MountPoint() / "recalbox/bios"       },
          { Path("/recalbox/share/cheats")     , parameter.Device().MountPoint() / "recalbox/cheats"     },
          { Path("/recalbox/share/kodi")       , parameter.Device().MountPoint() / "recalbox/kodi"       },
          { Path("/recalbox/share/music")      , parameter.Device().MountPoint() / "recalbox/music"      },
          { Path("/recalbox/share/overlays")   , parameter.Device().MountPoint() / "recalbox/overlays"   },
          { Path("/recalbox/share/roms")       , parameter.Device().MountPoint() / "recalbox/roms"       },
          { Path("/recalbox/share/saves")      , parameter.Device().MountPoint() / "recalbox/saves"      },
          { Path("/recalbox/share/screenshots"), parameter.Device().MountPoint() / "recalbox/screenshots"},
          { Path("/recalbox/share/shaders")    , parameter.Device().MountPoint() / "recalbox/shaders"    },
          { Path("/recalbox/share/system")     , parameter.Device().MountPoint() / "recalbox/system"     },
          { Path("/recalbox/share/themes")     , parameter.Device().MountPoint() / "recalbox/themes"     },
          { Path("/recalbox/share/userscripts"), parameter.Device().MountPoint() / "recalbox/userscripts"},
        };

      // Copy share folders
      for(const auto& ft : pathFromTo)
      {
        { LOG(LogInfo) << "[MainRunner] USB Initialization: Copying " << ft.From.Filename(); }
        SetText(_("Copying %s folder...").Replace("%s", ft.From.Filename()));
        if (!Files::CopyFolder(ft.From, ft.To, true)) return false;
      }

      // Install new device
      { LOG(LogInfo) << "[MainRunner] USB Initialization: Setup boot device."; }
      SetText(_("Setting up boot device..."));
      StorageDevices& storages = StorageDevices::Instance().Refresh();
      for(const StorageDevices::Device& storage : storages.GetShareDevices())
        if (storage.DevicePath == parameter.Device().Device().ToString())
        {
          { LOG(LogInfo) << "[MainRunner] USB Initialization: Boot device set to " << storage.UUID; }
          storages.SetShareDevice(storage);
          return true;
        }
    }
  }
  return false;
}

void MenuModalInitDevice::TaskComplete(const bool& result)
{
  (void)RecalboxSystem::execute("sync");
  switch(mIn.Action())
  {
    case USBInitializationAction::None: break;
    case USBInitializationAction::OnlyRomFolders:
    {
      mWindow.pushGui(new GuiMsgBox(mWindow, result ? _("Your USB device has been initialized! You can unplug it and copy your games on it.")
                                                                            : _("Initialization failed! Your USB device is full or contains errors. Please repair or use another device."), _("OK"), nullptr));
      break;
    }
    case USBInitializationAction::CompleteShare:
    {
      if (result)
        mWindow.pushGui(new GuiMsgBox(mWindow, _("Your USB device has been initialized! Ready to reboot on your new share device!"),
                                                  _("OK"), [] { MainRunner::RequestQuit(MainRunner::ExitState::Relaunch); }));
      else
        mWindow.pushGui(new GuiMsgBox(mWindow, _("Initialization failed! Your USB device is full or contains errors. Please repair or use another device."), _("OK"), nullptr));
      break;
    }
  }
}
