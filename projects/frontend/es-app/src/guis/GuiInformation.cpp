//
// Created by bkg2k on 08/04/2026.
//

#include "GuiInformation.h"
#include <utils/locale/LocaleHelper.h>
#include <themes/MenuThemeData.h>
#include <components/TextComponent.h>
#include <hardware/Board.h>
#include <hardware/Case.h>
#include <hardware/crt/ICrtInterface.h>
#include <utils/Files.h>
#include <Renderer.h>
#include <SDL.h>

#include "utils/Sizes.h"
#include <hardware/devices/storage/StorageDevices.h>
#include <hardware/Overclocking.h>
#include <hardware/cardreader/CardReader.h>
#include <audio/AudioController.h>
#include <input/InputManager.h>
#include <input/InputDevice.h>
#include <network/Networks.h>
#include <RecalboxConf.h>
#include <sys/statvfs.h>

GuiInformation::GuiInformation(WindowManager& window)
  : Gui(window)
  , mBackground(window, Path(":/frame.png"))
  , mGrid(window, Vector2i(3, 3))
{
  addChild(&mBackground);
  addChild(&mGrid);

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  mBackground.setImagePath(menuTheme.Background().path);
  mBackground.setCenterColor(menuTheme.Background().color);
  mBackground.setEdgeColor(menuTheme.Background().color);

  // Title
  mTitle = std::make_shared<TextComponent>(mWindow, _("SYSTEM INFORMATION"), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true, Vector2i(1, 1));

  // Body
  mText = std::make_shared<MarkdownText>(mWindow, BuildSystemInfo(), *menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::TopLeft);
  mText->SetUpDownScrolling(true);
  mText->SetAutoscroll(false);
  mGrid.setEntry(mText, Vector2i(1, 1), true, true, Vector2i(1, 1));

  // Size & position
  const float width  = Renderer::Instance().DisplayWidthAsFloat()  * 0.90f;
  const float height = Renderer::Instance().DisplayHeightAsFloat() * 0.85f;

  mText->setSize(width * 0.96f, 0);

  setSize(width, height);
  setPosition((Renderer::Instance().DisplayWidthAsFloat()  - width)  / 2.f,
              (Renderer::Instance().DisplayHeightAsFloat() - height) / 2.f);
}

String GuiInformation::BuildSystemInfo()
{
  String info;

  info.Append("**").Append(_("BOARD")).Append("**\n");
  info.Append("- ").Append(_("Model")).Append(": ").Append(Board::GetBoardName()).Append(String::LF);
  info.Append("- ").Append(_("Data bus")).Append(": ").Append(String(Board::GetDataBusSize())).Append("-bit\n");

  info.Append("\n**").Append(_("MEMORY")).Append("**\n");
  {
    int totalMB = Board::Instance().TotalMemory();
    if (totalMB > 0)
      info.Append("- ").Append(_("Total")).Append(": ").Append(String(totalMB)).Append(" MB\n");

    // Parse /proc/meminfo for MemFree / MemAvailable
    for (const String& line : Files::LoadAllFileLines(Path("/proc/meminfo")))
    {
      String key, val;
      if (!line.Extract(':', key, val, true)) continue;
      key.Trim(); val.Trim();
      if (key == "MemFree" || key == "MemAvailable" || key == "SwapTotal")
      {
        // val is e.g. "123456 kB" — parse as int64 to avoid overflow on > 2 GB values
        String::List parts = val.Split(' ');
        long long kb = parts.empty() ? 0LL : parts[0].AsInt64();
        info.Append("- ").Append(key).Append(": ").Append(Sizes(kb * 1024LL).ToHumanSize()).Append(String::LF);
      }
    }
  }

  ICrtInterface& crt = Board::Instance().CrtBoard();
  if (crt.IsCrtAdapterAttached())
  {
    info.Append("\n**").Append(_("CRT ADAPTER")).Append("**\n");
    info.Append("- ").Append(_("Name")).Append(": ").Append(crt.Name()).Append(String::LF);
    if (crt.Has31KhzSupport())
      info.Append("- 31kHz: ").Append(_("supported")).Append(String::LF);
    if (crt.Has120HzSupport())
      info.Append("- 120Hz: ").Append(_("supported")).Append(String::LF);
    if (crt.HasCompositeSupport())
      info.Append("- Composite: ").Append(_("supported")).Append(String::LF);
    if (crt.HasWideScreenSupport())
      info.Append("- Widescreen: ").Append(_("supported")).Append(String::LF);
  }

  if (Case::HasCase())
  {
    info.Append("\n**").Append(_("CASE")).Append("**\n");
    info.Append("- ").Append(Case::CurrentCase().DisplayName()).Append(String::LF);
  }

  info.Append("\n**").Append(_("CPU")).Append("**\n");
  {
    String model;
    int cores = 0;
    String hardware;
    for (const String& line : Files::LoadAllFileLines(Path("/proc/cpuinfo")))
    {
      String key, val;
      if (!line.Extract(':', key, val, true)) continue;
      key.Trim(); val.Trim();
      if ((key == "model name" || key == "Model name") && model.empty())
        model = val;
      else if (key == "processor")
        cores++;
      else if (key == "Hardware" && hardware.empty())
        hardware = val;
    }
    if (!model.empty())
      info.Append("- ").Append(_("Model")).Append(": ").Append(model).Append(String::LF);
    else if (!hardware.empty())
      info.Append("- ").Append(_("Hardware")).Append(": ").Append(hardware).Append(String::LF);
    if (cores > 0)
      info.Append("- ").Append(_("Cores")).Append(": ").Append(String(cores)).Append(String::LF);

    // Overclock (Raspberry Pi only — only populated when OC configs exist)
    Overclocking& oc = Board::Instance().GetOverclocking();
    if (oc.HasOverclocks())
    {
      const Overclocking::Overclock& current = oc.GetCurrentOverclock();
      const Overclocking::Overclock& none    = Overclocking::NoOverclock();
      if (current == none)
        info.Append("- ").Append(_("Overclock")).Append(": ").Append(_("None")).Append(String::LF);
      else
        info.Append("- ").Append(_("Overclock")).Append(": ").Append(current.Description())
            .Append(" (").Append(String(current.Frequency())).Append(" MHz)")
            .Append(current.Hazardous() ? " ⚠" : "").Append(String::LF);
    }
  }

  info.Append("\n**").Append(_("DISPLAY")).Append("**\n");
  {
    if (crt.IsCrtAdapterAttached())
    {
      // CRT output: show virtual (signal) resolution — the meaningful one for the user
      info.Append("- ").Append(_("Output")).Append(": CRT — ").Append(String(crt.Name())).Append(String::LF);
      info.Append("- ").Append(_("Resolution")).Append(": ")
          .Append(String(Renderer::Instance().DisplayWidthAsInt())).Append("×")
          .Append(String(Renderer::Instance().DisplayHeightAsInt())).Append(String::LF);
      info.Append("- ").Append(_("Frequency")).Append(": ")
          .Append(String(ICrtInterface::HorizontalFrequencyToHumanReadable(crt.GetHorizontalFrequency())))
          .Append(String::LF);
    }
    else
    {
      info.Append("- ").Append(_("Output")).Append(": HDMI").Append(String::LF);
      info.Append("- ").Append(_("Resolution")).Append(": ")
          .Append(String(Renderer::Instance().DisplayWidthAsInt())).Append("×")
          .Append(String(Renderer::Instance().DisplayHeightAsInt())).Append(String::LF);
    }
  }

  info.Append("\n**").Append(_("VIDEO")).Append("**\n");
  {
    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver != nullptr)
      info.Append("- ").Append(_("Driver")).Append(": ").Append(String(driver)).Append(String::LF);

    String vendor   = Renderer::GetGLVendor();
    String renderer = Renderer::GetGLRenderer();
    String version  = Renderer::GetGLVersion();
    if (!vendor.empty())
      info.Append("- ").Append(_("GL Vendor")).Append(": ").Append(vendor).Append(String::LF);
    if (!renderer.empty())
      info.Append("- ").Append(_("GL Renderer")).Append(": ").Append(renderer).Append(String::LF);
    if (!version.empty())
      info.Append("- ").Append(_("GL Version")).Append(": ").Append(version).Append(String::LF);
    info.Append("- Vulkan: ").Append(Board::Instance().HasVulkanSupport() ? _("supported") : _("unsupported")).Append(String::LF);
  }

  info.Append("\n**").Append(_("AUDIO")).Append("**\n");
  {
    // Active output and volumes
    String active = AudioController::Instance().GetActivePlaybackName();
    if (!active.empty())
      info.Append("- ").Append(_("Active output")).Append(": ").Append(active).Append(String::LF);
    info.Append("- ").Append(_("System volume")).Append(": ")
        .Append(String(AudioController::Instance().GetVolume())).Append("%").Append(String::LF);
    info.Append("- ").Append(_("Music volume")).Append(": ")
        .Append(String(AudioController::Instance().GetMusicVolume())).Append("%").Append(String::LF);

    // Available outputs
    IAudioController::DeviceList devices = AudioController::Instance().GetPlaybackList();
    if (!devices.empty())
    {
      info.Append("- ").Append(_("Available outputs")).Append(":").Append(String::LF);
      for (const DeviceDescriptor& d : devices)
        info.Append("  - ").Append(d.DisplayableName).Append(String::LF);
    }
  }

  if (Networks::IsInstantiated())
  {
    info.Append("\n**").Append(_("NETWORK")).Append("**\n");
    Networks& net = Networks::Instance();

    // Ethernet
    {
      String ip4 = net.IP(Networks::IPVersion::IPv4, Networks::Interfaces::Ethernet);
      String ip6 = net.IP(Networks::IPVersion::IPv6, Networks::Interfaces::Ethernet);
      if (!ip4.empty() || !ip6.empty())
      {
        info.Append("- **").Append(_("Ethernet")).Append("**").Append(String::LF);
        if (!ip4.empty()) info.Append("  - IPv4: ").Append(ip4).Append(String::LF);
        if (!ip6.empty()) info.Append("  - IPv6: ").Append(ip6).Append(String::LF);
      }
    }

    // WiFi
    {
      bool wifiEnabled = RecalboxConf::Instance().GetWifiEnabled();
      String ssid      = RecalboxConf::Instance().GetWifiSSID();
      bool hasIface    = net.HasWIFIInterface();

      if (hasIface || wifiEnabled)
      {
        info.Append("- **").Append(_("WiFi")).Append("**: ")
            .Append(wifiEnabled ? _("enabled") : _("disabled")).Append(String::LF);
        if (!ssid.empty())
          info.Append("  - SSID: ").Append(ssid).Append(String::LF);
        if (wifiEnabled)
        {
          String ip4 = net.IP(Networks::IPVersion::IPv4, Networks::Interfaces::Wifi);
          String ip6 = net.IP(Networks::IPVersion::IPv6, Networks::Interfaces::Wifi);
          if (!ip4.empty()) info.Append("  - IPv4: ").Append(ip4).Append(String::LF);
          if (!ip6.empty()) info.Append("  - IPv6: ").Append(ip6).Append(String::LF);
          if (ip4.empty() && ip6.empty())
            info.Append("  - ").Append(_("not connected")).Append(String::LF);
        }
      }
    }
  }

  if (InputManager::IsInstantiated())
  {
    InputManager& input = InputManager::Instance();
    int count = input.DeviceCount();
    if (count > 0)
    {
      info.Append("\n**").Append(_("CONTROLLERS")).Append("**\n");
      for (int i = 0; i < count; ++i)
      {
        InputDevice& dev = input.GetDeviceConfigurationFromIndex(i);
        if (!dev.IsPad()) continue;
        info.Append("- ").Append(dev.Name());
        if (dev.HasBatteryLevel())
          info.Append(" — ").Append(_("battery")).Append(": ").Append(String(dev.BatteryLevel())).Append("%");
        info.Append(String::LF);
      }
    }
  }

  if (CardReader::IsInstantiated() && CardReader::Instance().IsAvailable())
  {
    info.Append("\n**").Append(_("CARD READER")).Append("**\n");
    String version = CardReader::Instance().GetCardReaderVersion();
    if (!version.empty())
      info.Append("- ").Append(_("Version")).Append(": ").Append(version).Append(String::LF);
    if (CardReader::Instance().IsCardPlugged())
    {
      info.Append("- ").Append(_("Card")).Append(": ").Append(_("inserted")).Append(String::LF);
      String cardId = CardReader::Instance().GetCardId();
      if (!cardId.empty())
        info.Append("- ").Append(_("Card ID")).Append(": ").Append(cardId).Append(String::LF);
      String uuid = CardReader::Instance().GetCardUUIDString();
      if (!uuid.empty())
        info.Append("- UUID: ").Append(uuid).Append(String::LF);
    }
    else
      info.Append("- ").Append(_("Card")).Append(": ").Append(_("none")).Append(String::LF);
  }

  info.Append("\n**").Append(_("STORAGE")).Append("**\n");
  {
    struct PartEntry
    {
      String    device;   //!< bare device name, e.g. sda1
      String    parent;   //!< e.g. sda
      String    mountPt;
      String    fsType;
      long long total = 0;
      long long free  = 0;
    };
    std::vector<PartEntry> entries;

    for (const String& line : Files::LoadAllFileLines(Path("/proc/mounts")))
    {
      String::List cols = line.Split(' ', true);
      if (cols.size() < 4) continue;

      const String& dev = cols[0];
      const String& mp  = cols[1];
      const String& fs  = cols[2];

      // Only real block devices — skip virtual filesystems
      if (!dev.StartsWith("/dev/")) continue;
      if (dev.StartsWith("/dev/loop") || dev.StartsWith("/dev/ram")) continue;
      if (fs == "devtmpfs" || fs == "squashfs" || fs == "overlay") continue;

      PartEntry e;
      e.device  = Path(dev).Filename(); // bare name, e.g. sda1 (no /dev/ prefix)
      e.mountPt = mp;
      e.fsType  = fs;
      e.parent  = StorageDevices::StripPartitionSuffix(e.device);

      struct statvfs sv {};
      if (statvfs(mp.data(), &sv) == 0)
      {
        e.total = (long long)sv.f_blocks * (long long)sv.f_bsize;
        e.free  = (long long)sv.f_bfree  * (long long)sv.f_bsize;
      }
      entries.push_back(e);
    }

    String lastParent;
    for (const PartEntry& e : entries)
    {
      // New physical disk header
      if (e.parent != lastParent)
      {
        lastParent = e.parent;
        StorageDevices::PhysicalType pt = StorageDevices::DetectPhysicalType(e.parent);
        info.Append("- **").Append(e.parent)
            .Append(" [").Append(StorageDevices::PhysicalTypeToString(pt)).Append("]**")
            .Append(String::LF);
      }

      // Partition line: "  - /dev/sda1 [ext4] /mnt: 42 GB / 100 GB (42%)"
      int pct = e.total > 0 ? (int)((double)(e.total - e.free) * 100.0 / (double)e.total) : 0;
      info.Append("  - ").Append(e.device)
          .Append(" [").Append(e.fsType).Append("] ")
          .Append(e.mountPt).Append(": ");
      if (e.total > 0)
        info.Append(Sizes(e.total - e.free).ToHumanSize())
            .Append(" / ").Append(Sizes(e.total).ToHumanSize())
            .Append(" (").Append(String(pct)).Append("% used)");
      else
        info.Append(_("unknown size"));
      info.Append(String::LF);
    }

    if (entries.empty())
      info.Append("- ").Append(_("No mounted device found")).Append(String::LF);
  }

  return info;
}

void GuiInformation::onSizeChanged()
{
  mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

  const float titlePercent = (float)mTitle->getFont()->Height() * 1.6f / mSize.y();

  mGrid.setColWidthPerc(0, 0.02f, false);
  mGrid.setColWidthPerc(1, 0.96f, false);
  mGrid.setColWidthPerc(2, 0.02f, false);
  mGrid.setRowHeightPerc(0, titlePercent, false);
  mGrid.setRowHeightPerc(1, 0,     false);
  mGrid.setRowHeightPerc(2, 0.02f, false);
  mGrid.setSize(mSize);
}

bool GuiInformation::CollectHelpItems(Help& help)
{
  help.Set(Help::Cancel(), _("BACK"))
      .Set(Help::Valid(),  _("SAVE"));
  return true;
}

bool GuiInformation::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased())
  {
    Close();
    return true;
  }
  if (event.ValidReleased())
  {
    Files::SaveFile(Path(sOutputFile), mText->Text());
    mWindow.displayMessage((_F("System information saved into {0}") / sNetworkFile).ToString());
    return true;
  }
  return Gui::ProcessInput(event);
}
