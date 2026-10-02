//
// Created by bkg2k on 30/03/2020.
//

#include <utils/Files.h>
#include <utils/Log.h>
#include <utils/json/JSONBuilder.h>
#include <systems/SystemDeserializer.h>
#include <utils/datetime/DateTime.h>
#include "RequestHandler.h"
#include "Mime.h"
#include "RequestHandlerTools.h"
#include <utils/network/Url.h>
#include <usernotifications/NotificationManager.h>

using namespace Pistache;

void RequestHandler::FileServer(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "FileServer");

  Path path = mWWWRoot / (request.resource() != "/" ? request.resource() : mDefaultFile);
  String ext = path.Extension();

  bool knownMime = Mime::ExtToMIME.contains(ext);
  const Pistache::Http::Mime::MediaType& mimeType = knownMime ? Mime::ExtToMIME[ext] : Mime::BinaryFile;
  if (!knownMime)
    LOG(LogWarning) << "Unknown MIME Type for file extension: " << ext;
  RequestHandlerTools::SendResource(path, response, mimeType);
}

void RequestHandler::Versions(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "Versions");

  // Get libretro cores
  HashMap<String, String> cores;
  String::List coreLines = Files::LoadFile(Path("/recalbox/share/system/configs/retroarch.corenames")).Split('\n');
  for(const String& coreLine : coreLines)
  {
    String::List coreItems = coreLine.Split(';');
    if (coreItems.size() == 3) cores[coreItems[0]] = coreItems[2];
  }
  String retroarchVersion;
  (void)RequestHandlerTools::OutputOf("retroarch --version 2>&1").Extract("Version: ", " ", retroarchVersion, true);

  // Get Recalbox & Linux version
  String recalboxVersion = Files::LoadFile(Path("/recalbox/recalbox.version"));
  String linuxVersion = Files::LoadFile(Path("/proc/version"));
  int p = linuxVersion.Find('('); if (p >= 0) linuxVersion = linuxVersion.SubString(0, p).Trim();

  // Sort cores alphabetically
  String::List sortedKeys;
  for(const auto& core : cores)
    sortedKeys.push_back(core.first);
  std::sort(sortedKeys.begin(), sortedKeys.end());

  // Build final version object
  JSONBuilder json;
  json.Open()
      .Field("webapi", "2.0")
      .Field("recalbox", recalboxVersion)
      .Field("linux", linuxVersion)
      .OpenObject("libretro")
        .Field("retroarch", retroarchVersion)
        .OpenObject("cores");
          for(const String& key : sortedKeys)
            json.Field(key.c_str(), cores[key]);
        json.CloseObject()
      .CloseObject()
      .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, json, Mime::Json);
}

void RequestHandler::Architecture(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "Architecture");

  const String& arch = RequestHandlerTools::GetArchitecture();

  JSONBuilder json;
  json.Open()
      .Field("arch", arch)
      .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, json, Mime::Json);
}

void RequestHandler::SystemInfo(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "CpuInfo");

  JSONBuilder sysInfo;
  sysInfo.Open()
         .Field("timestamp", DateTime().ToISO8601())
         .Field("platform", mSysInfos.BuildPlatformObject())
         .Field("cpus", mSysInfos.BuildCpuObject(true))
         .Field("memory", mSysInfos.BuildMemoryObject(true))
         .Field("temperature", mSysInfos.BuildTemperatureObject(true))
         .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, sysInfo, Mime::Json);
}

void RequestHandler::StorageInfo(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "StorageInfo");

  String::List lines = RequestHandlerTools::OutputLinesOf("df -T");
  JSONBuilder result;
  result.Open()
        .OpenObject("storages");
  int id = 0;
  for(int i = (int)lines.size(); --i>0; )
  {
    String::List parts = lines[i].Split(' ', true);
    if (parts.size() == 7)
    {
      RequestHandlerTools::DeviceInfo info(parts[6], parts[0], parts[1], parts[2], parts[3]);
      RequestHandlerTools::GetDevicePropertiesOf(info);
      // System overlay
      if (info.Mount == "/") result.Field(String(id++).data(), RequestHandlerTools::BuildPartitionObject(info, "system"));
      // Boot partition
      else if (info.Mount == "/boot") result.Field(String(id++).data(), RequestHandlerTools::BuildPartitionObject(info, "boot"));
      // Any nfs/smb network mount in share
      else if (info.Mount.StartsWith("/recalbox/share") &&
               String("nfs|smb").Contains(info.FileSystemType)) result.Field(String(id++).data(), RequestHandlerTools::BuildPartitionObject(info, "network"));
      // Any device mount in share
      else if (info.Mount == "/recalbox/share" ||
               info.Mount.StartsWith("/recalbox/share/externals/")) result.Field(String(id++).data(), RequestHandlerTools::BuildPartitionObject(info, "share"));
      // Anything else
      else result.Field(String(id++).data(), RequestHandlerTools::BuildPartitionObject(info, "unknown"));
    }
    else LOG(LogError) << "df -T unknown result : " << lines[i];
  }
  result.CloseObject()
        .Close();
  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}

void RequestHandler::BiosDownload(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "BiosDownload");

  Path path = RequestHandlerTools::GetCompressedBiosFolder();
  RequestHandlerTools::SendResource(path, response, Mime::Zip);
}

void RequestHandler::BiosUpload(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "BiosUpload");

  String fileName;
  int start = 0;
  int size = 0;
  if (RequestHandlerTools::GetUploadedFile(request, fileName, start, size))
  {
    const char* data = request.body().data() + start;

    // Get bios
    String biosMd5 = md5(data, size);

    // Load and scan
    if (mBiosManager.SystemCount() == 0)
      mBiosManager.LoadFromFile();
    mBiosManager.Scan(nullptr, true);

    // Lookup
    const Bios* bios = nullptr;
    BiosManager::LookupResult result = mBiosManager.Lookup(Url::URLDecode(request.splatAt(0).name()), biosMd5, bios);

    switch (result)
    {
      case BiosManager::AlreadyExists:
      case BiosManager::Found:
      {
        bool ok = true;
        if (result == BiosManager::Found) ok = Files::SaveFile(bios->Filepath(), data, size);
        if (ok)
          RequestHandlerTools::Send(response, Http::Code::Ok, RequestHandlerTools::SerializeBiosToJSON(*bios), Mime::Json);
        else
          RequestHandlerTools::Send(response, Http::Code::Insufficient_Storage);
        break;
      }
      case BiosManager::NotFound:
      default: RequestHandlerTools::Error404(response);
        break;
    }
    // Note: each switch branch already sends its response.
  }
  else
    RequestHandlerTools::Send(response, Http::Code::Bad_Request, "No valid uploaded file", Mime::PlainText);
}

void RequestHandler::BiosGetAll(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "BiosGetAll");

  // Load and scan
  if (mBiosManager.SystemCount() == 0)
    mBiosManager.LoadFromFile();
  mBiosManager.Scan(nullptr, true);

  JSONBuilder output;
  output.Open();

  // Loop through systems
  for(int sysIndex = 0; sysIndex < mBiosManager.SystemCount(); sysIndex++)
  {
    BiosList list = mBiosManager.SystemBios(sysIndex);
    output.Field(list.Name().c_str(), RequestHandlerTools::SerializeBiosListToJSON(list));
  }
  output.Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, output, Mime::Json);
}

void RequestHandler::BiosGetSystem(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "BiosGetSystem");

  // Load and scan
  if (mBiosManager.SystemCount() == 0)
    mBiosManager.LoadFromFile();
  mBiosManager.Scan(nullptr, true);

  const String systemName = Url::URLDecode(request.splatAt(0).name());
  const BiosList list = mBiosManager.SystemBios(systemName);
  // Empty system does not have a name => not found
  if (list.Name() == systemName)
    RequestHandlerTools::Send(response, Http::Code::Ok, RequestHandlerTools::SerializeBiosListToJSON(list), Mime::Json);
  else
    RequestHandlerTools::Error404(response);
}

void RequestHandler::SystemsGetAll(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsGetAll");

  RequestHandlerTools::Send(response, Http::Code::Ok, RequestHandlerTools::SerializeSystemListToJSON(mSystemManager.AllSystems()), Mime::Json);
}

void RequestHandler::SystemsGetActives(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsGetActives");

  RequestHandlerTools::Send(response, Http::Code::Ok, RequestHandlerTools::SerializeSystemListToJSON(mSystemManager.VisibleSystemList()), Mime::Json);
}

void RequestHandler::SystemsResourceGetConsole(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsResourceGetConsole");

  Path first;
  Path second;
  RequestHandlerTools::GetSystemResourcePath(first, second, Url::URLDecode(request.splatAt(0).name()), Url::URLDecode(request.splatAt(1).name()), "console.svg");
  RequestHandlerTools::SendResource(first, second, response, Mime::ImageSvg);
}

void RequestHandler::SystemsResourceGetController(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsResourceGetController");

  Path first;
  Path second;
  RequestHandlerTools::GetSystemResourcePath(first, second, Url::URLDecode(request.splatAt(0).name()), Url::URLDecode(request.splatAt(1).name()), "controller.svg");
  RequestHandlerTools::SendResource(first, second, response, Mime::ImageSvg);
}

void RequestHandler::SystemsResourceGetControls(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsResourceGetControls");

  Path first;
  Path second;
  RequestHandlerTools::GetSystemResourcePath(first, second, Url::URLDecode(request.splatAt(0).name()), Url::URLDecode(request.splatAt(1).name()), "controls.svg");
  RequestHandlerTools::SendResource(first, second, response, Mime::ImageSvg);
}

void RequestHandler::SystemsResourceGetGame(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsResourceGetGame");

  Path first;
  Path second;
  RequestHandlerTools::GetSystemResourcePath(first, second, Url::URLDecode(request.splatAt(0).name()), Url::URLDecode(request.splatAt(1).name()), "game.svg");
  RequestHandlerTools::SendResource(first, second, response, Mime::ImageSvg);
}

void RequestHandler::SystemsResourceGetLogo(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemsResourceGetLogo");

  Path first;
  RequestHandlerTools::GetLogoResourcePath(first, Url::URLDecode(request.splatAt(0).name()));
  RequestHandlerTools::SendResource(first, response, Mime::ImageSvg);
}

void RequestHandler::ConfigurationGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ConfigurationGet");

  String ns = Url::URLDecode(request.splatAt(0).name());
  const HashMap<String, Validator>& keys = RequestHandlerTools::SelectConfigurationKeySet(ns, mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::GetKeyValues(ns, keys, response);
}

void RequestHandler::ConfigurationOptions(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ConfigurationOptions");

  String ns = Url::URLDecode(request.splatAt(0).name());
  const HashMap<String, Validator>& keys = RequestHandlerTools::SelectConfigurationKeySet(ns, mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::GetKeyValueOptions(keys, response);
}

void RequestHandler::ConfigurationSet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ConfigurationSet");

  String ns = Url::URLDecode(request.splatAt(0).name());
  const HashMap<String, Validator>& keys = RequestHandlerTools::SelectConfigurationKeySet(ns, mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::SetKeyValues(ns, keys, request.body(), response);
}

void RequestHandler::ConfigurationDelete(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ConfigurationDelete");

  String ns = Url::URLDecode(request.splatAt(0).name());
  const HashMap<String, Validator>& keys = RequestHandlerTools::SelectConfigurationKeySet(ns, mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::DeleteKeyValues(ns, keys, request.body(), response);
}

void RequestHandler::SystemConfigurationGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemConfigurationGet");

  // Check system
  String subSystem = Url::URLDecode(request.splatAt(0).name());
  if (!RequestHandlerTools::IsValidSystem(subSystem, mSystemManager))
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  // Check data
  const HashMap<String, Validator> keys = RequestHandlerTools::SelectConfigurationKeySet("specific", mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::GetKeyValues(subSystem, keys, response);
}

void RequestHandler::SystemConfigurationSet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemConfigurationSet");

  // Check system
  String subSystem = Url::URLDecode(request.splatAt(0).name());
  if (!RequestHandlerTools::IsValidSystem(subSystem, mSystemManager))
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  // Check data
  const HashMap<String, Validator> keys = RequestHandlerTools::SelectConfigurationKeySet("specific", mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::SetKeyValues(subSystem, keys, request.body(), response, true);
}

void RequestHandler::SystemConfigurationOptions(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemConfigurationOptions");

  // Check system
  String subSystem = Url::URLDecode(request.splatAt(0).name());
  if (!RequestHandlerTools::IsValidSystem(subSystem, mSystemManager))
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  // Check data
  const HashMap<String, Validator> keys = RequestHandlerTools::SelectConfigurationKeySet("specific", mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::GetKeyValueOptions(keys, response);
}

void RequestHandler::SystemConfigurationDelete(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "SystemConfigurationDelete");

  // Check system
  String subSystem = Url::URLDecode(request.splatAt(0).name());
  if (!RequestHandlerTools::IsValidSystem(subSystem, mSystemManager))
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  // Check data
  const HashMap<String, Validator> keys = RequestHandlerTools::SelectConfigurationKeySet("specific", mSystemManager);
  if (keys.empty())
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  RequestHandlerTools::DeleteKeyValues(subSystem, keys, request.body(), response);
}

void RequestHandler::MediaOptions(const Rest::Request& request, Http::ResponseWriter response)
{
    RequestHandlerTools::LogRoute(request, "MediaOptions");
    RequestHandlerTools::Send(response, Http::Code::Ok);
}

void RequestHandler::MediaGetList(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MediaGetList");
  RequestHandlerTools::GetJSONMediaList(response);
}

void RequestHandler::MediaDelete(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MediaDelete");

  String mediaName = Url::URLDecode(request.splatAt(0).name());
  Path mediaPath("/recalbox/share/screenshots");
  Path media = mediaPath / mediaName;

  // Guard against path traversal: only files under the screenshots folder may be deleted.
  if (!RequestHandlerTools::IsPathConfinedTo(media, "/recalbox/share/screenshots"))
  {
    RequestHandlerTools::Error404(response);
    return;
  }

  if (!media.Exists())
  {
    RequestHandlerTools::Error404(response);
    return;
  }
  (void)media.Delete();
  RequestHandlerTools::GetJSONMediaList(response);
}

void RequestHandler::MediaTakeScreenshot(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MediaTakeScreenshot");

  String date = DateTime().ToStringFormat("%YYYY-%MM-%ddT%HH-%mm-%ss-%fffZ");

  RequestHandlerTools::OutputOf("raspi2png -p /recalbox/share/screenshots/screenshot-" + date + ".png");
  RequestHandlerTools::GetJSONMediaList(response);
}

void RequestHandler::MediaGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MediaGet");

  // Get path
  Path path(Decode64(Url::URLDecode(request.splatAt(0).name())));
  // { LOG(LogDebug) << "[RequestHandler] path = " << Url::URLDecode(request.splatAt(0).name()); }
  RequestHandlerTools::SendMedia(path, response);
}

void RequestHandler::MetadataGetInfo(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetInfo");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameMetadataInformation(mSystemManager, request.splatAt(0).name(), fullPath, response);
}

void RequestHandler::MetadataGetImage(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetImage");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameResource(mSystemManager, request.splatAt(0).name(), fullPath, RequestHandlerTools::Media::Image, response);
}

void RequestHandler::MetadataGetBox(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetBox");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameResource(mSystemManager, request.splatAt(0).name(), fullPath, RequestHandlerTools::Media::Box, response);
}

void RequestHandler::MetadataGetVideo(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetVideo");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameResource(mSystemManager, request.splatAt(0).name(), fullPath, RequestHandlerTools::Media::Video, response);
}

void RequestHandler::MetadataGetMap(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetMap");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameResource(mSystemManager, request.splatAt(0).name(), fullPath, RequestHandlerTools::Media::Map, response);
}

void RequestHandler::MetadataGetManual(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MetadataGetManual");
  String fullPath(Url::URLDecode(request.splatAt(1).name()));
  if (!fullPath.StartsWith('/')) fullPath.Insert(0, '/');
  RequestHandlerTools::SendGameResource(mSystemManager, request.splatAt(0).name(), fullPath, RequestHandlerTools::Media::Manual, response);
}

void RequestHandler::StatusGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "StatusGet");
  RequestHandlerTools::Send(response, Http::Code::Ok, NotificationManager::Instance().LastJSONEvent(), Mime::Json);
}

void RequestHandler::MediaGetScreenshot(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "MediaGetScreenshot");

  String fileName = Url::URLDecode(request.splatAt(0).name());
  Path path = Path("/recalbox/share/screenshots/" + fileName);
  RequestHandlerTools::SendMedia(path, response);
}

void RequestHandler::RomsGetTotal(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "RomsGetTotal");

  const SystemData* allGamesSystem = mSystemManager.VirtualSystemByType(VirtualSystemType::AllGames);
  const FileData::TopLevelFilter filter = FileData::BuildTopLevelFilter();

  int totalGames = 0;
  if (allGamesSystem != nullptr)
  {
    const FileData::List allGames = allGamesSystem->getAllGames();
    for (const FileData* game : allGames)
      if (game->IsDisplayable(filter))
        totalGames++;
  }

  JSONBuilder result;
  result.Open()
      .Field("total", totalGames)
      .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}

void RequestHandler::RomsGetList(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "RomsGetList");

  String systemName = Url::URLDecode(request.splatAt(0).name());
  SystemData* system = mSystemManager.SystemByName(systemName);
  if (system == nullptr)
  {
    RequestHandlerTools::Error404(response);
    return;
  }
  FileData::List allGames = system->getAllGames();

  class Serializer : public ISerializeToJson<const FileData>
  {
    public:
      // Serialize a single game
      JSONBuilder Serialize(const FileData* game) override
      {
        if (game->IsGame())
          return JSONBuilder()
            .Open()
                .Field("path", game->Metadata().Rom().ToString())
                .Field("name", game->Metadata().Name())
                .Field("publisher", game->Metadata().Publisher())
                .Field("developer", game->Metadata().Developer())
                .Field("genre", game->Metadata().Genre())
                .Field("players", game->Metadata().PlayerMax())
                .Field("rating", game->Metadata().Rating())
                .Field("favorite", game->Metadata().Favorite())
            .Close();
        return JSONBuilder();
      }
  } serializer;

  JSONBuilder roms;
  roms.Open()
      .Field<FileData>("roms", allGames, serializer)
      .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, roms, Mime::Json);
}

void RequestHandler::RomsDelete(const Rest::Request& request, Http::ResponseWriter response)
{
  (void)response;
  RequestHandlerTools::LogRoute(request, "RomsDelete");
}

void RequestHandler::ThemeSystemViewGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeSystemViewGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "systemview", response);
}

void RequestHandler::ThemeMenuSetGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeMenuSetGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "menuset", response);
}

void RequestHandler::ThemeIconSetGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeIconSetGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "iconset", response);
}

void RequestHandler::ThemeGamelistViewGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeGamelistViewGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "gamelistview", response);
}

void RequestHandler::ThemeColorSetGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeColorSetGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "colorset", response);
}

void RequestHandler::ThemeGameclipViewGet(const Rest::Request& request, Http::ResponseWriter response)
{
  RequestHandlerTools::LogRoute(request, "ThemeGameclipViewGet");

  RequestHandlerTools::GetThemeKeyValue(Url::URLDecode(request.splatAt(0).name()), "gameclipview", response);
}

static const char Base64Values[] =
  {
    00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
    00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
    00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 62, 00, 00, 00, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 00, 00, 00, 00, 00, 00,
    00, 00, 01, 02, 03, 04, 05, 06, 07,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 00, 00, 00, 00, 00,
    00, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
  };

String RequestHandler::Decode64(const String& base64)
{
  String result;

  // Align on 3 char
  int len = (int)base64.size() & -4;
  if (len <= 0) return result;   // need at least one full 4-char group; avoids base64[-1] OOB read

  int off7 = 0;

  // Padding?
  bool padded = (base64[off7 + len - 1] == '=');
  if (padded) len -= 4;

  // 3 char loop
  for (int L = len >> 2; --L >= 0; off7 += 4)
  {
    int V = (Base64Values[(unsigned int)(unsigned char)base64[off7]] << 18) +
            (Base64Values[(unsigned int)(unsigned char)base64[off7 + 1]] << 12) +
            (Base64Values[(unsigned int)(unsigned char)base64[off7 + 2]] << 6) +
            (Base64Values[(unsigned int)(unsigned char)base64[off7 + 3]]);
    result.Append((char)(V >> 16))
          .Append((char)(V >> 8))
          .Append((char)V);
  }
  // remaining
  if (padded)
  {
    int V = (Base64Values[(unsigned int)(unsigned char)base64[off7]] << 10) +
            (Base64Values[(unsigned int)(unsigned char)base64[off7 + 1]] << 4) +
            (Base64Values[(unsigned int)(unsigned char)base64[off7 + 2]] >> 2);
    result.Append((char)(V >> 8));
    if (base64[off7 + 2] != '=')
      result.Append((char)V);
  }

  return result;
}

