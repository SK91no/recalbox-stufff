//
// Created by bkg2k on 03/04/2020.
// Last modification by Maksthorr on 28/04/2023
//

#include <pistache/include/pistache/router.h>
#include <utils/Log.h>
#include <emulators/EmulatorList.h>
#include <systems/SystemDeserializer.h>
#include <rapidjson/document.h>
#include <utils/Files.h>
#include "RequestHandlerTools.h"
#include "Mime.h"
#include <LibretroRatio.h>
#include <utils/Zip.h>
#include <patreon/PatronInfo.h>
#include <hardware/Board.h>
#include "RecalboxConf.h"
#include "web/WebManagerSync.h"
#include <utils/locale/LocaleHelper.h>
#include "themes/ThemeSpec.h"
#include "hardware/cardreader/CardReader.h"

using namespace Pistache;

void RequestHandlerTools::GetJSONMediaList(Pistache::Http::ResponseWriter& response)
{
  Path mediaPath("/recalbox/share/screenshots");
  Path::PathList list = mediaPath.GetDirectoryContent();

  static String imagesExtensions(".jpg|.jpeg|.png|.gif");
  static String videosExtensions(".mkv|.avi|.mp4|.webm");

  JSONBuilder result;
  result.Open()
        .Field("mediaPath", mediaPath.ToString())
        .OpenObject("mediaList");
  for(const Path& path : list)
  {
    bool ok = false;
    result.OpenObject(path.MakeRelative(mediaPath, ok).ToChars());
    String ext = path.Extension().LowerCase();
    if (imagesExtensions.find(ext) != String::npos) result.Field("type", "image");
    else if (videosExtensions.find(ext) != String::npos) result.Field("type", "video");
    else result.Field("type", "unknown");
    result.CloseObject();
  }
  result.CloseObject()
        .Close();
  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}

JSONBuilder RequestHandlerTools::BuildPartitionObject(const DeviceInfo& info, const String& recalboxtype)
{
  JSONBuilder result;
  result.Open()
        .Field("recalbox", recalboxtype)
        .Field("devicebus", info.Bus)
        .Field("devicetype", info.Type)
        .Field("devicemodel", info.Model)
        .Field("mount", info.Mount)
        .Field("filesystem", info.FileSystem)
        .Field("filesystemtype", info.FileSystemType)
        .Field("size", info.Size << 10)
        .Field("used", info.Used << 10)
        .Field("label", info.Label)
        .Close();
  return result;
}

void RequestHandlerTools::GetDevicePropertiesOf(DeviceInfo& info)
{
  // Get info from udevadm
  String::List strings = RequestHandlerTools::OutputLinesOf(String("/sbin/udevadm info --query=property --name=").Append(info.FileSystem));
  for(const String& string : strings)
  {
    int pos = string.Find('=');
    if (pos < 0) continue;
    if (string.StartsWith(LEGACY_STRING("ID_BUS=")))
      info.Bus = string.SubString(pos + 1);
    else if (string.StartsWith(LEGACY_STRING("ID_VENDOR=")))
      info.Model.Insert(0, string.SubString(pos + 1) + ' ');
    else if (string.StartsWith(LEGACY_STRING("ID_MODEL=")))
      info.Model.Append(string.SubString(pos + 1));
    else if (string.StartsWith(LEGACY_STRING("ID_TYPE=")))
      info.Type = string.SubString(pos + 1);
    else if (string.StartsWith(LEGACY_STRING("ID_NAME=")))
      info.Model = string.SubString(pos + 1);
    else if (string.StartsWith(LEGACY_STRING("ID_FS_TYPE=")))
      info.FileSystemType = string.SubString(pos + 1);
    else if (string.StartsWith(LEGACY_STRING("ID_FS_LABEL=")))
      info.Label = string.SubString(pos + 1);
  }
  // Try to get missing info
  if (info.Mount.StartsWith("/dev/mmcblk"))
  {
    info.Bus = "sdio";
    info.Type = "sd";
  }
  info.Model.Replace('_', ' ');
}

void RequestHandlerTools::GetSystemResourcePath(Path& regionPath, Path& basePath, const String& system, const String& region, const char* resourceFileName)
{
  String regionString(sSystemResourceRegionPath);
  regionString.Replace("%SYSTEM%", system)
              .Replace("%REGION%", region)
              .Replace("%FILE%", resourceFileName);
  regionPath = regionString;

  String baseString(sSystemResourceBasePath);
  baseString.Replace("%SYSTEM%", system)
            .Replace("%FILE%", resourceFileName);
  basePath = baseString;
}

void RequestHandlerTools::GetLogoResourcePath(Path& path, const String& system)
{
  RecalboxConf& conf = RecalboxConf::Instance();

  // Full localized logo
  String logoString(sLogoPath);
  logoString.Replace("%SYSTEM%", String(system).Append('-').Append(conf.GetSystemLanguage()));
  if (Path(logoString).Exists()) { path = logoString; return;; }

  // Language only localized logo
  logoString.Assign(sLogoPath);
  logoString.Replace("%SYSTEM%", String(system).Append('-').Append(conf.GetSystemLanguage().SubString(0, 2)));
  if (Path(logoString).Exists()) { path = logoString; return; }

  // Regionalized logo
  logoString.Assign(sLogoPath);
  logoString.Replace("%SYSTEM%", String(system).Append('-').Append(conf.GetThemeRegion()));
  if (Path(logoString).Exists()) { path = logoString; return; }

  // Default logo
  logoString.Assign(sLogoPath);
  path = logoString.Replace("%SYSTEM%", system);
}

JSONBuilder RequestHandlerTools::SerializeBiosToJSON(const Bios& bios)
{
  return JSONBuilder().Open()
                      .Field("mandatory", bios.IsMandatory())
                      .Field("hashMatchingMandatory", bios.IsHashMatchingMandatory())
                      .Field("displayFileName", bios.Filename(false))
                      .Field("notes", bios.Notes())
                      .Field("currentMd5", bios.CurrentMD5())
                      .Field("md5List", bios.MD5List())
                      .Field("cores", bios.Cores().Split(','))
                      .Field("lightStatus", bios.LightStatusAsString())
                      .Field("realStatus", bios.BiosStatusAsString())
                      .Close();
}

JSONBuilder RequestHandlerTools::SerializeBiosListToJSON(const BiosList& biosList)
{
  JSONBuilder systemJson;
  systemJson.Open()
            .Field("fullName", biosList.FullName())
            .OpenObject("scanResult")
            .Field("total", biosList.BiosCount())
            .Field("ok", biosList.TotalBiosOk())
            .Field("ko", biosList.TotalBiosKo())
            .Field("unsafe", biosList.TotalBiosUnsafe())
            .Field("notFound", biosList.TotalFileNotFound())
            .Field("hashOk", biosList.TotalHashMatching())
            .Field("hashKo", biosList.TotalHashNotMatching())
            .CloseObject()
            .OpenObject("biosList");
  for(int biosIndex = biosList.BiosCount(); --biosIndex >= 0; )
  {
    const Bios& bios = biosList.BiosAt(biosIndex);
    systemJson.Field(bios.Filepath().ToChars(), SerializeBiosToJSON(bios));
  }
  systemJson.CloseObject()
            .Close();

  return systemJson;
}

String RequestHandlerTools::OutputOf(const String& command)
{
  String output;
  FILE* pipe = popen(command.c_str(), "r");
  if (pipe != nullptr)
  {
    char buffer[1024];
    while (feof(pipe) == 0)
      if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        output.Append(buffer);
    pclose(pipe);
  }
  return output;
}

String::List RequestHandlerTools::OutputLinesOf(const String& command)
{
  return OutputOf(command).Split('\n');
}

void RequestHandlerTools::Error404(Http::ResponseWriter& response)
{
  LOG(LogError) << "Error 404";
  SetHeaders(response);
  Send(response, Http::Code::Not_Found, "404 - Not found", Mime::PlainText);
}

JSONBuilder RequestHandlerTools::SerializeEmulatorsAndCoreToJson(const EmulatorList& emulatorlist)
{
  JSONBuilder emulators;
  emulators.Open();
  for(int i = emulatorlist.Count(); --i >= 0; )
  {
    const EmulatorDescriptor& emulator = emulatorlist.EmulatorAt(i);
    JSONBuilder CoreJson;
    CoreJson.Open();
    for(int c = emulator.CoreCount(); --c >= 0; )
      CoreJson.Field(String((int)emulator.Core(c).Priority()).c_str(), emulator.Core(c).Name());
    CoreJson.Close();
    emulators.Field(emulator.Name().c_str(), CoreJson);
  }
  emulators.Close();
  return emulators;
}

namespace
{
  //! CORS origin to echo back for the current request (local-network origins only). Per-thread because
  //! Pistache handles each request on a single worker thread: set in LogRoute, read in SetHeaders.
  thread_local String sAllowedCorsOrigin;

  //! Extract the host from an Origin value ("scheme://host[:port]"), handling [IPv6] literals.
  String OriginHost(const String& origin)
  {
    int p = origin.Find("://");
    String hostport = (p >= 0) ? origin.SubString(p + 3) : origin;
    if (hostport.StartsWith('['))
    {
      int e = hostport.Find(']');
      return (e > 1) ? hostport.SubString(1, e - 1) : hostport;
    }
    int c = hostport.Find(':');
    return (c >= 0) ? hostport.SubString(0, c) : hostport;
  }

  //! True if the host is on a local/private network (RFC1918, loopback, link-local, mDNS, recalbox hostname).
  bool IsLocalOriginHost(const String& rawHost)
  {
    if (rawHost.empty()) return false;
    String host(rawHost); host.LowerCase();
    if (host == "localhost" || host == "127.0.0.1" || host == "::1") return true;
    if (host.EndsWith(".local")) return true;
    String hostname = RecalboxConf::Instance().GetHostname(); hostname.LowerCase();
    if (!hostname.empty() && host == hostname) return true;
    // IPv6 unique-local (fc00::/7) and link-local (fe80::/10)
    if (host.StartsWith("fc") || host.StartsWith("fd")) return true;
    if (host.StartsWith("fe8") || host.StartsWith("fe9") || host.StartsWith("fea") || host.StartsWith("feb")) return true;
    // IPv4 dotted quad
    String::List parts = host.Split('.');
    if (parts.size() == 4)
    {
      int o[4];
      for (int i = 0; i < 4; ++i)
        if (!parts[i].TryAsInt(o[i]) || o[i] < 0 || o[i] > 255) return false;
      if (o[0] == 127) return true;                              // loopback
      if (o[0] == 10) return true;                               // 10.0.0.0/8
      if (o[0] == 172 && o[1] >= 16 && o[1] <= 31) return true;  // 172.16.0.0/12
      if (o[0] == 192 && o[1] == 168) return true;               // 192.168.0.0/16
      if (o[0] == 169 && o[1] == 254) return true;               // 169.254.0.0/16 link-local
    }
    return false;
  }
}

void RequestHandlerTools::LogRoute(const Pistache::Rest::Request& request, const char* methodName)
{
  // Decide the CORS origin to echo back: only local-network origins are accepted (see SetHeaders).
  sAllowedCorsOrigin = String();
  auto origin = request.headers().tryGetRaw("Origin");
  if (origin.has_value())
  {
    String value(origin->value().c_str());
    if (IsLocalOriginHost(OriginHost(value))) sAllowedCorsOrigin = value;
  }

  LOG(LogDebug) << methodName << ": " << MethodToString(request.method()) << ' ' << request.resource();
}

const char* RequestHandlerTools::MethodToString(Pistache::Http::Method method)
{
  switch(method)
  {
    case Http::Method::Options: return "OPTIONS";
    case Http::Method::Get: return "GET";
    case Http::Method::Post: return "POST";
    case Http::Method::Head: return "HEAD";
    case Http::Method::Put: return "PUT";
    case Http::Method::Patch: return "PATCH";
    case Http::Method::Delete: return "DELETE";
    case Http::Method::Trace: return "TRACE";
    case Http::Method::Connect: return "CONNECT";

    // Avoid enum Warning
    case Http::Method::Acl:
    case Http::Method::BaselineControl:
    case Http::Method::Bind:
    case Http::Method::Checkin:
    case Http::Method::Checkout:
    case Http::Method::Copy:
    case Http::Method::Label:
    case Http::Method::Link:
    case Http::Method::Lock:
    case Http::Method::Merge:
    case Http::Method::Mkactivity:
    case Http::Method::Mkcalendar:
    case Http::Method::Mkcol:
    case Http::Method::Mkredirectref:
    case Http::Method::Mkworkspace:
    case Http::Method::Move:
    case Http::Method::Orderpatch:
    case Http::Method::Pri:
    case Http::Method::Propfind:
    case Http::Method::Proppatch:
    case Http::Method::Rebind:
    case Http::Method::Report:
    case Http::Method::Search:
    case Http::Method::Unbind:
    case Http::Method::Uncheckout:
    case Http::Method::Unlink:
    case Http::Method::Unlock:
    case Http::Method::Update:
    case Http::Method::Updateredirectref:
    case Http::Method::VersionControl:
    default: break;
  }
  return "UNKNOWN";
}

/*
 * Send
 */


void RequestHandlerTools::SetHeaders(Http::ResponseWriter& response)
{
  response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>("GET, POST, OPTIONS, DELETE");
  response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>("*");
  // Restrict CORS to local-network origins: echo the request Origin only when it is local (computed in
  // LogRoute). Non-local cross-origin browser requests get no Allow-Origin and are blocked. Same-origin
  // requests (the bundled web manager) need no Allow-Origin and are unaffected.
  if (!sAllowedCorsOrigin.empty())
  {
    response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>(sAllowedCorsOrigin.c_str());
    response.headers().addRaw(Pistache::Http::Header::Raw("Vary", "Origin"));
  }
}

void
RequestHandlerTools::Send(Pistache::Http::ResponseWriter& response, Pistache::Http::Code code, const String& body,
                          const Pistache::Http::Mime::MediaType& mime)
{
  SetHeaders(response);
  response.send(code, body, mime);
}

Pistache::Async::Promise<ssize_t> RequestHandlerTools::Send(Pistache::Http::ResponseWriter& response, Pistache::Http::Code code)
{
  SetHeaders(response);
  return response.send(code, nullptr, 0, Pistache::Http::Mime::MediaType());
}

void RequestHandlerTools::SendResource(const Path& resourcepath, Http::ResponseWriter& response, const Pistache::Http::Mime::MediaType& mimeType)
{
  if (resourcepath.Exists())
  {
    SetHeaders(response);
    Http::serveFile(response, resourcepath.ToString(), mimeType);
  }
  else
    Error404(response);
}

void RequestHandlerTools::SendResource(const Path& preferedpath, const Path& fallbackpath, Http::ResponseWriter& response, const Http::Mime::MediaType& mimetype)
{
  if (preferedpath.Exists())
  {
    SetHeaders(response);
    Http::serveFile(response, preferedpath.ToString(), mimetype);
  }
  else if (fallbackpath.Exists())
  {
    SetHeaders(response);
    Http::serveFile(response, fallbackpath.ToString(), mimetype);
  }
  else
    Error404(response);
}

const HashMap<String, Validator>& RequestHandlerTools::SelectConfigurationKeySet(const String& _namespace, SystemManager& manager)
{
  enum class Namespace
  {
    Unknown,
    System,
    EmulationStation,
    Frontend,
    Scraper,
    Kodi,
    Hyperion,
    Audio,
    Wifi,
    Wifi2,
    Wifi3,
    Controllers,
    Updates,
    Global,   //<! All emulators settings
    Specific, //<! Specific emulators settings
    Patron,
    Music,
    Hat,
    Autorun,
    Tate,
    Lircd,
    Pinball,
    CardReader,
  };

  static HashMap<String, Namespace> sConverter
  ({
    { "system"          , Namespace::System },
    { "emulationstation", Namespace::EmulationStation },
    { "frontend"        , Namespace::Frontend },
    { "scraper"         , Namespace::Scraper },
    { "kodi"            , Namespace::Kodi },
    { "hyperion"        , Namespace::Hyperion },
    { "audio"           , Namespace::Audio },
    { "wifi"            , Namespace::Wifi },
    { "wifi2"           , Namespace::Wifi2 },
    { "wifi3"           , Namespace::Wifi3 },
    { "controllers"     , Namespace::Controllers },
    { "updates"         , Namespace::Updates },
    { "global"          , Namespace::Global },
    { "specific"        , Namespace::Specific },
    { "patron"          , Namespace::Patron },
    { "music"           , Namespace::Music },
    { "hat"             , Namespace::Hat },
    { "autorun"         , Namespace::Autorun },
    { "tate"            , Namespace::Tate },
    { "lircd"           , Namespace::Lircd },
    { "pinball"         , Namespace::Pinball },
    { "cardreader"      , Namespace::CardReader },
  });

  Namespace* pns = sConverter.try_get(_namespace);
  Namespace ns = (pns != nullptr) ? *pns : Namespace::Unknown;

  switch(ns)
  {
    case Namespace::System:
    {
      static HashMap<String, Validator> sList
      ({
        { "power.switch"                          , Validator(false, { "ATX_RASPI_R2_6", "MAUSBERRY", "REMOTEPIBOARD_2003", "REMOTEPIBOARD_2005", "WITTYPI", "PIN56ONOFF", "PIN56PUSH", "PIN356ONOFFRESET", "PIN356PUSHRESET" }) },
        { "fbcp.enabled"                          , Validator(true) },
        { "splash.length"                         , Validator(-1, 300) },
        { "splash.select"                         , Validator(false, { "all", "recalbox", "custom" }) },
        { "manager.enabled"                       , Validator(true) },
        { "es.videomode"                          , Validator(GetAvailableFrontEndResolutions(), false) },
        { "emulators.specialkeys"                 , Validator(false, { "default", "nomenu", "none" }) },
        { "hostname"                              , Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") },
        { "samba.enabled"                         , Validator(true) },
        { "virtual-gamepads.enabled"              , Validator(true) },
        { "ssh.enabled"                           , Validator(true) },
        { "language"                              , Validator(GetAvailableLanguages(), false) },
        { "kblayout"                              , Validator(GetAvailableKeyboardLayout(), false) },
        { "timezone"                              , Validator(GetAvailableTimeZone(), false) },
        { "secondminitft.enabled"                 , Validator(true) },
        { "secondminitft.type"                    , Validator(false, { "overlay", "default" }) },
        { "secondminitft.resolution"              , Validator(false, { "240p", "320p" }) },
        { "secondminitft.imagestretchenabled"     , Validator(true) },
        { "secondminitft.imageenlargeenabled"     , Validator(true) },
        { "secondminitft.imagealphaenabled"       , Validator(true) },
        { "secondminitft.imageignoreaspectenabled", Validator(true) },
        { "secondminitft.disablevideoines"        , Validator(true) },
        { "secondminitft.backlightcontrol"        , Validator(0, 100) },
        { "secondminitft.usemarquee"              , Validator(true) },
        { "secondminitft.sleepenabled"            , Validator(true) },
        { "externalscreen.prefered"               , Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") },
        { "externalscreen.forceresolution"        , Validator("0123456789x") },
        { "externalscreen.forcefrequency"         , Validator("0123456789.") },
        { "es.force43"                            , Validator(true) },
        { "splash.enabled"                        , Validator(true) },
       });

      return sList;
    }
    case Namespace::EmulationStation:
    {
      static HashMap<String, Validator> sList
      ({
        { "menu"                        , Validator(false, { "default", "bartop", "none" }) },
        { "selectedsystem"              , Validator(GetSupportedSystemList(manager), false) },
        { "bootongamelist"              , Validator(true) },
        { "hidesystemview"              , Validator(true) },
        { "gamelistonly"                , Validator(true) },
        { "forcebasicgamelistview"      , Validator(true) },
        { "filteradultgames"            , Validator(true) },
        { "clock"                       , Validator(true) },
        { "favoritesonly"               , Validator(true) },
        { "screensaver.time"            , Validator(0, 30) },
        { "screensaver.type"            , Validator(GetAvailableScreensavers(), false) },
        { "showgameclipclippingitem"    , Validator(true) },
        { "showgamecliphelpitems"       , Validator(true) },
        { "theme.folder"                , Validator(GetAvailableThemes(), false) },
        { "collection.allgames"         , Validator(true) },
        { "collection.multiplayer"      , Validator(true) },
        { "collection.lastplayed"       , Validator(true) },
        { "collection.tate"             , Validator(true) },
        { "collection.dial"             , Validator(true) },
        { "collection.trackball"        , Validator(true) },
        { "virtualarcade"               , Validator(true) },
        { "virtualarcade.hideoriginals" , Validator(true) },
        { "collection.ports.hide"       , Validator(true) },
        { "collection.lightgun.hide"    , Validator(true) },
        { "quicksystemselect"           , Validator(true) },
        { "showhelp"                    , Validator(true) },
        { "popoup.help"                 , Validator(0, 10) },
        { "popoup.music"                , Validator(0, 10) },
        { "popoup.netplay"              , Validator(0, 10) },
        { "systemsorting"               , Validator(false, { "default", "name", "releasedate", "1type2name", "1type2releasedate", "1manufacturer2name", "1manufacturer2releasedate", "1type2manufacturer3name", "1type2manufacturer3releasedate" }) },
        { "theme.carousel"              , Validator(true) },
        { "theme.transition"            , Validator(false, { "slide", "instant", "fade" }) },
        { "theme.region"                , Validator(false, { "eu", "jp", "us" }) },
        { "brightness"                  , Validator(0, 8) },
        { "showhidden"                  , Validator(true) },
        { "showonlylatestversion"       , Validator(true) },
        { "hidenogames"                 , Validator(true) },
        { "arcade.view.hidenonworking"  , Validator(true) },
        { "arcade.view.enhanced"        , Validator(true) },
        { "arcade.view.hideclones"      , Validator(true) },
        { "arcade.view.hidebios"        , Validator(true) },
        { "arcade.usedatabasenames"     , Validator(true) },
        { "virtualarcade.manufacturers" , Validator(GetArcadeManufacturerList(), true) },
        { "pad0"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad1"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad2"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad3"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad4"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad5"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad6"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad7"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "pad8"                        , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopwrstuvwxyz0123456789: ") },
        { "debuglogs"                   , Validator(true) },
        { "pads.osd"                    , Validator(true) },
        { "pads.osd.type"               , Validator(false, { "dreamcast", "megadrive", "nintendo64", "playstation", "snes", "xbox" }) },
        { "tateonly"                    , Validator(true) },
        { "battery.hidden"              , Validator(true) },
        { "favoritesfirst"              , Validator(true) },
        { "onegameonerom"               , Validator(true) },
        { "rompreferredregion"          , Validator(GetRegionEntries(false), false) },
        { "romsecondpreferredregion"    , Validator(GetRegionEntries(true), false) },
        { "rompreferredfallback"        , Validator(GetOneGameOneRomPreferedFallback(), false) },
        { "displaybyfilename"           , Validator(true) },
        { "game.transition"             , Validator(false, { "zoom", "fade", "instant" }) },
        { "hideboardgames"              , Validator(true) },
        { "showonly3plusplayers"        , Validator(true) },
        { "yokoonly"                    , Validator(true) },
       });

      return sList;
    }
    case Namespace::Frontend:
    {
      static HashMap<String, Validator> sList
      ({
        { "enableeditfavorites", Validator(true) },
        { "listfastmovenabled" , Validator(true) },
      });

      return sList;
    }
    case Namespace::Scraper:
    {
      static HashMap<String, Validator> sList
      ({
        { "extractregionfromfilename"   , Validator(true) },
        { "getnamefrom"                 , Validator(false, { "0", "1", "2" }) },
        { "source"                      , Validator(GetAvailableScrapers(), false) },
        { "auto"                        , Validator(true) },
        { "screenscraper.region"        , Validator(GetRegionEntries(false), false) },
        { "screenscraper.language"      , Validator(GetScraperLanguages(), false) },
        { "screenscraper.media"         , Validator(false, { "InGameScreenShot", "TitleScreenshot", "ClearLogo", "Marquee", "Box2D", "Box3D", "MixV1", "MixV2" }) },
        { "screenscraper.thumbnail"     , Validator(false, { "InGameScreenShot", "TitleScreenshot", "ClearLogo", "Marquee", "Box2D", "Box3D", "MixV1", "MixV2" }) },
        { "screenscraper.video"         , Validator(false, { "none", "OriginalVideo", "NormalizedVideo" }) },
        { "screenscraper.manual"        , Validator(true) },
        { "screenscraper.maps"          , Validator(true) },
        { "screenscraper.p2k"           , Validator(true) },
        { "screenscraper.user"          , Validator() },
        { "screenscraper.password"      , Validator() },
        { "screenscraper.regionPriority", Validator(false, { "DetectedRegion", "favoriteRegion" }) },
      });

      return sList;
    }
    case Namespace::Kodi:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled"         , Validator(true) },
        { "atstartup"       , Validator(true) },
        { "xbutton"         , Validator(true) },
        { "videomode"       , Validator(GetAvailableFrontEndResolutions(), false) },
        { "network.waitmode", Validator(false, { "required", "wish", "nonce" }) },
        { "network.waittime", Validator(0, INT32_MAX) },
        { "network.waithost", Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") },
      });

      return sList;
    }
    case Namespace::Hyperion:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled", Validator(true) },
      });

      return sList;
    }
    case Namespace::Wifi:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled"    , Validator(true) },
        { "region"     , Validator(false, { "US", "CA", "JP", "DE", "NL", "IT", "PT", "LU", "NO", "FI", "DK", "CH", "CZ", "ES", "GB", "KR", "CN", "FR", "HK", "SG", "TW", "BR", "IL", "SA", "LB", "AE", "ZA", "AR", "AU", "AT", "BO", "CL", "GR", "IS", "IN", "IE", "KW", "LI", "LT", "MX", "MA", "NZ", "PL", "PR", "SK", "SI", "TH", "UY", "PA", "RU", "KW", "LI", "LT", "MX", "MA", "NZ", "PL", "PR", "SK", "SI", "TH", "UY", "PA", "RU", "EG", "TT", "TR", "CR", "EC", "HN", "KE", "UA", "VN", "BG", "CY", "EE", "MU", "RO", "CS", "ID", "PE", "VE", "JM", "BH", "OM", "JO", "BM", "CO", "DO", "GT", "PH", "LK", "SV", "TN", "PK", "QA", "DZ" }) },
        { "ssid"       , Validator() },
        { "key"        , Validator() },
        { "ip"         , Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") },
        { "gateway"    , Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") },
        { "netmask"    , Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") },
        { "nameservers", Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") },
        { "priority"   , Validator(0, 20) },
        { "connect"    , Validator(true) },
      });

      return sList;
    }
    case Namespace::Wifi2:
    {
      static HashMap<String, Validator> sList
      ({
        { "ssid"       , Validator() },
        { "key"        , Validator() },
      });

      return sList;
    }
    case Namespace::Wifi3:
    {
      static HashMap<String, Validator> sList
      ({
        { "ssid"       , Validator() },
        { "key"        , Validator() },
      });

      return sList;
    }
    case Namespace::Audio:
    {
      static HashMap<String, Validator> sList
      ({
        { "device"      , Validator(GetAvailableSoundDevices(), false) },
        { "volume"      , Validator(0, 100) },
        { "music.volume", Validator(0, 100) },
        { "bgmusic"     , Validator(true) },
        { "mode"        , Validator(false, { "musicxorvideosound", "musicandvideosound", "musiconly", "videosoundonly", "none"}) },
      });

      return sList;
    }
    case Namespace::Controllers:
    {
      static HashMap<String, Validator> sList
      ({
        { "bluetooth.enabled"       , Validator(true) },
        { "bluetooth.ertm"          , Validator(true) },
        { "ps3.enabled"             , Validator(true) },
        { "ps3.driver"              , Validator(false, { "bluez", "official", "shanwan" }) },
        { "gpio.enabled"            , Validator(true) },
        { "gpio.args"               , Validator() },
        { "steam.enabled"           , Validator(true) },
        { "db9.enabled"             , Validator(true) },
        { "db9.args"                , Validator() },
        { "gamecon.enabled"         , Validator(true) },
        { "gamecon.args"            , Validator() },
        { "xarcade.enabled"         , Validator(true) },
        { "joycond.enabled"         , Validator(true) },
        { "swapvalidateandcancel"   , Validator(true) },
        { "bluetooth.autopaironboot", Validator(true) },
      });

      return sList;
    }
    case Namespace::Updates:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled", Validator(true) },
        { "type"   , Validator(GetUpdateTypeEntries(), false) },
      });

      return sList;
    }
    case Namespace::Global:
    {
      static HashMap<String, Validator> sList
      ({
        { "videomode"                 , Validator(GetAvailableGlobalResolutions(), false) },
        { "shaderset"                 , Validator({ "none", "crtcurved", "scanlines", "retro", "custom" }, false) },
        { "shaderset.file"            , Validator(GetAvailableShaders(), false) },
        { "integerscale"              , Validator(true) },
        { "shaders"                   , Validator(true) },
        { "ratio"                     , Validator(GetSupportedRatioList(), false) },
        { "smooth"                    , Validator(true) },
        { "rewind"                    , Validator(true) },
        { "autosave"                  , Validator(true) },
        { "retroachievements"         , Validator(true) },
        { "retroachievements.hardcore", Validator(true) },
        { "retroachievements.username", Validator() },
        { "retroachievements.password", Validator() },
        { "retroachievements.sounds"  , Validator(true) },
        { "inputdriver"               , Validator(false, { "auto", "udev", "sdl2"}) },
        { "demo.systemlist"           , Validator(GetSupportedSystemList(manager), true) },
        { "demo.duration"             , Validator(30, 600) },
        { "demo.infoscreenduration"   , Validator(5, 30) },
        { "translate"                 , Validator(true) },
        { "translate.from"            , Validator(false, {"auto", "EN", "ES", "FR", "IT", "DE", "JP", "NL", "CS", "DA", "SV", "HR", "KO", "ZH_CN", "ZH_TW", "CA", "BG", "BN", "EU", "AZ", "AR", "SQ", "AF", "EO", "ET", "TL", "FI", "GL", "KA", "EL", "GU", "HT", "IW", "HI", "HU", "IS", "ID", "GA", "KN", "LA", "LV", "LT", "MK", "MS", "MT", "NO", "FA", "PL", "PT", "RO", "RU", "SR", "SK", "SL", "SW", "TA", "TE", "TH", "TR", "UK", "UR", "VI", "CY", "YI"}) },
        { "translate.to"              , Validator(false, {"auto", "EN", "ES", "FR", "IT", "DE", "JP", "NL", "CS", "DA", "SV", "HR", "KO", "ZH_CN", "ZH_TW", "CA", "BG", "BN", "EU", "AZ", "AR", "SQ", "AF", "EO", "ET", "TL", "FI", "GL", "KA", "EL", "GU", "HT", "IW", "HI", "HU", "IS", "ID", "GA", "KN", "LA", "LV", "LT", "MK", "MS", "MT", "NO", "FA", "PL", "PT", "RO", "RU", "SR", "SK", "SL", "SW", "TA", "TE", "TH", "TR", "UK", "UR", "VI", "CY", "YI"}) },
        { "translate.apikey"          , Validator() },
        { "translate.url"             , Validator() },
        { "netplay"                   , Validator(true) },
        { "netplay.active"            , Validator(true) },
        { "netplay.nickname"          , Validator("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'_-") },
        { "netplay.port"              , Validator(1, 65535) },
        { "netplay.relay"             , Validator(false, { "none", "nyc", "madrid", "montreal", "saopaulo" }) },
        { "netplay.lobby"             , Validator() },
        { "recalboxoverlays"          , Validator(true) },
        { "softpatching"              , Validator(false, { "auto", "select", "disable", "launchLast" }) },
        { "showfps"                   , Validator(true) },
        { "hidepreinstalledgames"     , Validator(true) },
        { "show.savestate.before.run" , Validator(true) },
        { "quitpresstwice"            , Validator(true) },
        { "reducelatency"             , Validator(true) },
        { "runahead"                  , Validator(true) },
        { "hdmode"                    , Validator(true) },
        { "widescreenmode"            , Validator(true) },
        { "vulkandriver"              , Validator(true) },
        { "rumble"                    , Validator(true) },
        { "autoblitter"               , Validator(false, { "none", "recalbox", "viku" }) },
        { "crthandheldformat"         , Validator(false, { "fullscreen", "original" }) }, //<! RRGBD2
      });

      return sList;
    }
    case Namespace::Specific:
    {
      static HashMap<String, Validator> sList
      ({
        { "videomode"                 , Validator(GetAvailableFrontEndResolutions(), false) },
        { "emulator"                  , Validator("abcdefghijklmnopqrstuvwxyz0123456789_") },
        { "core"                      , Validator("abcdefghijklmnopqrstuvwxyz0123456789_") },
        { "shaders"                   , Validator(true) },
        { "shaderset"                 , Validator({ "none", "crtcurved", "scanlines", "retro", "custom" }, false) },
        { "ratio"                     , Validator(GetSupportedRatioList(), false) },
        { "smooth"                    , Validator(true) },
        { "rewind"                    , Validator(true) },
        { "autosave"                  , Validator(true) },
        { "integerscale"              , Validator(true) },
        { "configfile"                , Validator() }, //<! Empty or "dummy" to cancel any config from configgen
        { "demo.include"              , Validator(true) }, //<! Include system to the demo screensaver
        { "demo.duration"             , Validator(30, 600) }, //<! Duration of the demo for the current system
        { "ignore"                    , Validator(true) }, //<! Ignore to display the specific system
        { "filteradultnames"          , Validator(true) },
        { "regionfilter"              , Validator(GetAvailableRegionFilter(), false) },
        { "flatfolders"               , Validator(true) },
        { "sort"                      , Validator(GetAvailableSingleSystemSorts(), false) },
        { "supergameboy"              , Validator(false, { "gb", "sgb", "ask" }) }, //<! For GB only
        { "realgamecubepads"          , Validator(true) }, //<! For GameCube only, play w/ gc pads w/ adapters
        { "realwiimotes"              , Validator(true) }, //<! For Wii only, use real wiimotes in Dolphin
        { "emulatedwiimotes"          , Validator(true) }, //<! For Wii only, emulate wiimotes w/ std pads
        { "sensorbar.position"        , Validator(true) }, //<! For Wii only, specify Dolphin Bar position around screen. 1: top, 0: bottom
      });

      return sList;
    }
    case Namespace::Patron:
    {
      static HashMap<String, Validator> sList
      ({
        { "privatekey", Validator("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") },
      });

      return sList;
    }
    case Namespace::Music:
    {
      static HashMap<String, Validator> sList
      ({
        { "remoteplaylist.enable", Validator(true) },
      });

      return sList;
    }
    case Namespace::Hat:
    {
      static HashMap<String, Validator> sList
      ({
        { "wpaf.enabled", Validator(true) },
        { "wpaf.board"  , Validator(false, { "wspoehatb", "argonforty", "piboy", "rpipoeplus", "fanshim" }) },
      });

      return sList;
    }
    case Namespace::Autorun:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled" , Validator(true) },
        { "uuid"    , Validator() },
        { "gamepath", Validator() },
      });

      return sList;
    }
    case Namespace::Tate:
    {
      static HashMap<String, Validator> sList
      ({
        { "gamerotation", Validator(false, { "0", "1", "2", "3" }) }, //<! 0: normal, 1: left, 2: upsidedown, 3: right
      });

      return sList;
    }
    case Namespace::Lircd:
    {
      static HashMap<String, Validator> sList
      ({
        { "enabled", Validator(true) },
      });

      return sList;
    }
    case Namespace::Pinball:
    {
      static HashMap<String, Validator> sList
      ({
        { "screen1"           , Validator(GetAvailableScreens(), false) }, //<! table
        { "screen1.resolution", Validator() }, //<! table
        { "screen2"           , Validator(GetAvailableScreens(), false) }, //<! backglass
        { "screen2.resolution", Validator() }, //<! backglass
        { "cabinet"           , Validator(true) },
      });

      return sList;
    }
    case Namespace::CardReader:
    {
      static HashMap<String, Validator> sList
      ({
        { "consolemode"     , Validator(true) },
        { "consolemodeexit" , Validator(GetCRConsoleModeExit(), false) },
        // { "consolemodevideo", Validator(GetCRConsoleModeVideo(), false) },
        { "consolemodesound", Validator(GetCRConsoleModeSound(), false) },
      });

      return sList;
    }
    case Namespace::Unknown:
    default: break;
  }

  static HashMap<String, Validator> sList;
  return sList;
}

void RequestHandlerTools::GetKeyValues(const String& domain, const HashMap<String, Validator>& keys, Http::ResponseWriter& response, const HashMap<String, String>* overlay)
{
  // Read directly from the live configuration singleton (no more autonomous IniFile).
  RecalboxConf& configuration = RecalboxConf::Instance();
  JSONBuilder result;
  result.Open();
  for(const auto& key : keys)
  {
    String k(domain); k.Append('.'); k.Append(key.first);
    // An overlay value (just-validated, not yet applied on the main thread) takes precedence,
    // so the response reflects what the caller just set despite the asynchronous apply.
    const String* over = (overlay != nullptr) ? overlay->try_get(key.first) : nullptr;
    JSONBuilder value;
    value.Open()
         .Field("exist", over != nullptr ? true : configuration.Exists(k));
    switch (key.second.Type())
    {
      case Validator::Types::StringFree:
      case Validator::Types::StringConstrained:
      case Validator::Types::StringPicker: value.Field("value", over != nullptr ? *over : configuration.AsString(k)); break; // FIX 21/04/2023
      case Validator::Types::StringMultiPicker: value.Field("value", over != nullptr ? over->Split(',') : configuration.AsStringList(k)); break;
      case Validator::Types::IntRange: { int iv = 0; if (over != nullptr) (void)over->TryAsInt(iv); else iv = configuration.AsInt(k, 0); value.Field("value", iv); break; }
      case Validator::Types::Bool: value.Field("value", over != nullptr ? over->AsBool() : configuration.AsBool(k, false)); break;
      default: LOG(LogError) << "Unknown type " << (int) key.second.Type() << " for " << key.first;
    }
    value.Close();
    result.Field(key.first.c_str(), value);
  }
  result.Close();
  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}

void RequestHandlerTools::GetKeyValueOptions(const HashMap<String, Validator>& keys, Http::ResponseWriter& response)
{
  JSONBuilder result;
  result.Open();
  for(const auto& key : keys)
  {
    JSONBuilder properties;
    properties.Open()
              .Field("type", key.second.TypeAsString());
    switch (key.second.Type())
    {
      case Validator::Types::StringFree: break;
      case Validator::Types::StringConstrained: properties.Field("allowedChars", key.second.StringConstraint()); break;
      case Validator::Types::StringPicker:
      case Validator::Types::StringMultiPicker: properties.Field("allowedStringList", key.second.StringList()); break;
      case Validator::Types::IntRange: properties.Field("lowerValue", key.second.Lower()).Field("higherValue", key.second.Higher()); break;
      case Validator::Types::Bool: break;
      default: LOG(LogError) << "Unknown type " << (int) key.second.Type() << " for " << key.first;
    }
    if (key.second.HasDisplay())
      properties.Field("displayableStringList", key.second.DisplayList());
    properties.Close();
    result.Field(key.first.c_str(), properties);
  }
  result.Close();
  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}

void RequestHandlerTools::SetKeyValues(const String& domain, const HashMap<String, Validator>& keys, const String& jsonString, Http::ResponseWriter& response, bool systemDomain)
{
  rapidjson::Document json;
  json.Parse(jsonString.c_str());
  if (json.HasParseError())
  {
    RequestHandlerTools::Send(response, Http::Code::Bad_Request, "JSON Parsing error", Mime::PlainText);
    return;
  }

  // First pass: validate everything. Collect normalized values so the change is atomic
  // (we only enqueue once all keys are valid) and so we can echo them back.
  HashMap<String, String> accepted;
  if (json.GetType() == rapidjson::Type::kObjectType)
    for(const auto& key : json.GetObject())
    {
      String shortKey(key.name.GetString());
      String value;
      switch(key.value.GetType())
      {
        case rapidjson::kFalseType: value = "0"; break;
        case rapidjson::kTrueType: value = "1"; break;
        case rapidjson::kStringType: value = key.value.GetString(); break;
        case rapidjson::kNumberType: value = String(key.value.GetInt()); break;

        case rapidjson::kArrayType:
        {
            auto myArray = key.value.GetArray();
            value = "";
            for (unsigned int i = 0; i < myArray.Size(); i++)
            {
                const auto& val = myArray[i];
                if (val.IsString())
                {
                    value +=  String(val.GetString()) + ',';
                }
            }
            if (!myArray.Empty())
            {
                value.pop_back();
            }

            break;
        }

        case rapidjson::kNullType:
        case rapidjson::kObjectType:
        {
          RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Key '" + shortKey + "' has an invalid value!", Mime::PlainText);
          return;
        }
        default: break;
      }
      Validator* validator = keys.try_get(shortKey);
      if (validator == nullptr)
      {
        RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Unknown Key: " + shortKey, Mime::PlainText);
        return;
      }
      if (!validator->Validate(value))
      {
        RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Key '" + shortKey + "' has an invalid value!", Mime::PlainText);
        return;
      }
      accepted[shortKey] = value;
    }

  // Second pass: hand the changes to the main thread through the bridge. They are applied
  // there exclusively through the typed RecalboxConf setters (so watchers fire) and persisted.
  for(const auto& change : accepted)
  {
    if (systemDomain)
      WebManagerSync::Instance().PushSystemConfigChange(domain, change.first, change.second);
    else
    {
      String longKey(domain); longKey.Append('.'); longKey.Append(change.first);
      WebManagerSync::Instance().PushConfigChange(longKey, change.second);
    }
  }

  // Echo the accepted values (the apply is asynchronous, so reading the singleton now would be stale)
  GetKeyValues(domain, keys, response, &accepted);
}

void RequestHandlerTools::DeleteKeyValues(const String& domain, const HashMap<String, Validator>& keys, const String& jsonString, Http::ResponseWriter& response)
{
  rapidjson::Document json;
  json.Parse(jsonString.c_str());
  if (json.HasParseError())
  {
    RequestHandlerTools::Send(response, Http::Code::Bad_Request, "JSON Parsing error", Mime::PlainText);
    return;
  }

  // First pass: make sure every requested key is known (atomic deletion)
  String::List toDelete;
  if (json.GetType() == rapidjson::Type::kArrayType)
    for(const auto& key : json.GetArray())
    {
      String shortKey(key.GetString());
      if (!keys.contains(shortKey))
      {
        RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Unknown Key: " + shortKey, Mime::PlainText);
        return;
      }
      toDelete.push_back(shortKey);
    }

  // Second pass: hand the deletions to the main thread through the bridge
  for(const String& shortKey : toDelete)
  {
    String longKey(domain); longKey.Append('.'); longKey.Append(shortKey);
    WebManagerSync::Instance().PushConfigDelete(longKey);
  }

  GetKeyValues(domain, keys, response);
}

HashMap<String, String> RequestHandlerTools::GetSupportedSystemList(const SystemManager& manager)
{
  static HashMap<String, String> result;

  if (result.empty())
  {
    for(const SystemData* system : manager.AllSystems())
      if (system->HasVisibleGame())
      {
        // Don't add system with 0 game in it
        if (system->Name().Contains(SystemManager::sArcadeManufacturerPrefix))
        {
          // Add 'Arcade' prefix to identify virtual arcade systems and use its real name
          String manufacturerKey = system->Name()
            .SubString(String(SystemManager::sArcadeManufacturerPrefix).Count())
            .Replace('-', '\\');

          String displayName = system->FullName();
          for (const auto& pair : ArcadeVirtualSystems::GetVirtualArcadeSystemListExtended())
          {
            if (pair.first == manufacturerKey)
            {
              displayName = pair.second;
              break;
            }
          }

          String identifier = _("Arcade").ToLowerCase().ToUpperCase(0, 1).Append(" - ").Append(displayName);
          result.insert_unique({ system->Name(), identifier });
        }
        else if (system->Name().Contains(SystemManager::sGenrePrefix))
        {
          // Add 'Genre' prefix to identify virtual genre systems
          String identifier = _("Genre").ToLowerCase().ToUpperCase(0, 1).Append(" - ").Append(system->FullName().ToUpperCase(0, 1));
          result.insert_unique({ system->Name(), identifier });
        }
        else if (!system->IsPorts())
          // Add real system without distinct ports
          result.insert_unique({ system->Name(), system->FullName() });
      }
  }

  return result;
}

HashMap<String, String> RequestHandlerTools::GetArcadeManufacturerList()
{
  static HashMap<String, String> result;

  if (result.empty())
  {
    for(const String& rawIdentifier : ArcadeVirtualSystems::GetVirtualArcadeSystemList())
    {
      String identifier(SystemManager::sArcadeManufacturerPrefix);
      identifier.Append(rawIdentifier).Replace('\\', '-');
      result.insert_unique({ identifier, String(rawIdentifier).Replace('\\', " - ") });
    }
  }

  return result;
}

const String::List& RequestHandlerTools::GetSupportedRatioList()
{
  static String::List result;

  if (result.empty())
    for(const auto& ratio : LibretroRatio::GetRatio())
      result.push_back(ratio.second);

  return result;
}

const String& RequestHandlerTools::GetArchitecture()
{
  static String sArchitecture(Files::LoadFile(Path("/recalbox/recalbox.arch")));
  return sArchitecture;
}

HashMap<String, String> RequestHandlerTools::GetAvailableGlobalResolutions()
{
  static HashMap<String, String> sResolutions;

  if (sResolutions.empty())
  {
    ResolutionAdapter resolutionAdapter;
    sResolutions.insert_unique("default", _("NATIVE"));
    for(const ResolutionAdapter::Resolution& resolution : resolutionAdapter.Resolutions(true))
    {
      String reso = resolution.ToRawString();
      sResolutions.insert_unique({ reso, resolution.ToString() });
    }
  }

  return sResolutions;
}

HashMap<String, String> RequestHandlerTools::GetAvailableFrontEndResolutions()
{
  static HashMap<String, String> sResolutions;

  if (sResolutions.empty())
  {
    ResolutionAdapter resolutionAdapter;
    sResolutions.insert_unique("", _("USE GLOBAL"));
    sResolutions.insert_unique("default", _("NATIVE"));
    for(const ResolutionAdapter::Resolution& resolution : resolutionAdapter.Resolutions(true))
    {
      String reso = resolution.ToRawString();
      sResolutions.insert_unique({ reso, resolution.ToString() });
    }
  }

  return sResolutions;
}

HashMap<String, String> RequestHandlerTools::GetAvailableSoundDevices()
{
  static HashMap<String, String> result;

  if (result.empty())
  {
    IAudioController::DeviceList playbackList = AudioController::Instance().GetPlaybackList();

    for(const auto& playback : playbackList)
    {
      result.insert_unique({ playback.InternalName, playback.DisplayableName });
    }
  }

  return result;
}

const String::List& RequestHandlerTools::GetAvailableTimeZone()
{
  static String::List output;

  if (output.empty())
  {
    String zones = Files::LoadFile(Path("/usr/share/zoneinfo/zone.tab"));
    String::List list = zones.Split('\n');
    for (const String& line : list)
      if (!line.empty() && line[0] != '#')
      {
        int start = line.Find('\t');
        if (start < 0) continue;
        start = line.Find('\t', start + 1);
        if (start < 0) continue;
        int stop = line.Find('\t', ++start);
        String tz = line.SubString(start, stop - start);
        output.push_back(tz);
      }
  }

  return output;
}

String::List RequestHandlerTools::GetRegionEntries(const bool allowNone)
{
  String::List sRegions;

  for (const Regions::GameRegions region : Regions::AvailableRegions())
  {
    if (Regions::GameRegions::Unknown == region)
    {
      if (allowNone) sRegions.push_back("none");
    }
    else
      sRegions.push_back(Regions::GameRegionsFromEnum(region));
  }

  return sRegions;
}

HashMap<String, String> RequestHandlerTools::GetScraperLanguages()
{
  static HashMap<String, String> sLanguages({
    { "cz", "Čeština"    },
    { "da", "Dansk"      },
    { "de", "Deutsch"    },
    { "es", "Español"    },
    { "en", "English"    },
    { "fr", "Français"   },
    { "it", "Italiano"   },
    { "ja", "日本語"      },
    { "ko", "한국어"       },
    { "nl", "Nederlands" },
    { "no", "Norsk"      },
    { "hu", "Magyar"     },
    { "pl", "Polski"     },
    { "pt", "Português"  },
    { "ru", "Русский"    },
    { "sk", "Slovenčina" },
    { "fi", "Suomi"      },
    { "sv", "Svenska"    },
    { "tr", "Türkçe"     },
    { "zh", "简体中文"     },
  });

  return sLanguages;
}

HashMap<String, String> RequestHandlerTools::GetAvailableLanguages()
{
  static HashMap<String, String> sLanguages
  ({
     { "eu_ES", "EUSKARA"        },
     { "zh_TW", "正體中文"         },
     { "zh_CN", "简体中文"         },
     { "de_DE", "DEUTSCH"        },
     { "en_US", "ENGLISH"        },
     { "en_GB", "ENGLISH (UK)"   },
     { "es_ES", "ESPAÑOL"        },
     { "fr_FR", "FRANÇAIS"       },
     { "it_IT", "ITALIANO"       },
     { "pt_BR", "PORTUGUES"      },
     { "sv_SE", "SVENSKA"        },
     { "tr_TR", "TÜRKÇE"         },
     { "ca_ES", "CATALÀ"         },
     { "ar_YE", "اللغة العربية"  },
     { "nl_NL", "NEDERLANDS"     },
     { "el_GR", "ελληνικά"       },
     { "ko_KR", "한국어"          },
     { "nn_NO", "NORSK"          },
     { "nb_NO", "BOKMAL"         },
     { "pl_PL", "POLSKI"         },
     { "ja_JP", "日本語"          },
     { "ru_RU", "Русский"        },
     { "hu_HU", "MAGYAR"         },
     { "cs_CZ", "čeština"        },
     { "lv_LV", "latviešu"       },
     { "lb_LU", "Lëtzebuergesch" },
  });

  return sLanguages;
}

HashMap<String, String> RequestHandlerTools::GetAvailableSingleSystemSorts()
{
  static HashMap<String, String> sSorts
  ({
    { "0",  "filenameascending"     },
    { "1",  "filenamedescending"    },
    { "16", "ratingascending"       },
    { "17", "ratingdescending"      },
    { "4",  "timesplayedascending"  },
    { "5",  "timesplayeddescending" },
    { "6",  "lastplayedascending"   },
    { "7",  "lastplayeddescending"  },
    { "8",  "playerascending"       },
    { "9",  "playerdescending"      },
    { "10", "developerascending"    },
    { "11", "developerdescending"   },
    { "12", "publisherascending"    },
    { "13", "publisherdescending"   },
    { "14", "genreascending"        },
    { "15", "genredescending"       },
    { "18", "releasedateascending"  },
    { "19", "releasedatedescending" },
  });

  return sSorts;
}

HashMap<String, String> RequestHandlerTools::GetAvailableRegionFilter()
{
  static HashMap<String, String> sRegions
  ({
    { "0",   "Unknown"                                },
    { "1",   "Djibouti"                               },
    { "2",   "Eritrea"                                },
    { "3",   "Ethiopia"                               },
    { "4",   "Afghanistan"                            },
    { "5",   "Namibia"                                },
    { "6",   "South Africa"                           },
    { "7",   "Cameroon"                               },
    { "8",   "Ghana"                                  },
    { "9",   "Armenia"                                },
    { "10",  "Argentina"                              },
    { "11",  "World"                                  },
    { "12",  "United Arab Emirates"                   },
    { "13",  "Bahrain"                                },
    { "14",  "Algeria"                                },
    { "15",  "Egypt"                                  },
    { "16",  "Israel"                                 },
    { "17",  "Iraq"                                   },
    { "18",  "Jordan"                                 },
    { "19",  "Comoros"                                },
    { "20",  "Kuwait"                                 },
    { "21",  "Lebanon"                                },
    { "22",  "Libya"                                  },
    { "23",  "Morocco"                                },
    { "24",  "Mauritania"                             },
    { "25",  "Oman"                                   },
    { "26",  "Palestinian Authority"                  },
    { "27",  "Qatar"                                  },
    { "28",  "Saudi Arabia"                           },
    { "29",  "Sudan"                                  },
    { "30",  "Somalia"                                },
    { "31",  "South Sudan"                            },
    { "32",  "Syria"                                  },
    { "33",  "Chad"                                   },
    { "34",  "Tunisia"                                },
    { "35",  "Yemen"                                  },
    { "36",  "Chile"                                  },
    { "37",  "American Samoa"                         },
    { "38",  "India"                                  },
    { "39",  "Tanzania"                               },
    { "40",  "Spain"                                  },
    { "41",  "Azerbaijan"                             },
    { "42",  "Bosnia and Herzegovina"                 },
    { "43",  "Russia"                                 },
    { "44",  "Belgium"                                },
    { "45",  "Belarus"                                },
    { "46",  "Zambia"                                 },
    { "47",  "Bulgaria"                               },
    { "48",  "Nigeria"                                },
    { "49",  "Bermuda"                                },
    { "50",  "Mali"                                   },
    { "51",  "Brunei"                                 },
    { "52",  "Bangladesh"                             },
    { "53",  "Bolivia"                                },
    { "54",  "China"                                  },
    { "55",  "Brazil"                                 },
    { "56",  "France"                                 },
    { "57",  "Bahamas"                                },
    { "58",  "Canada"                                 },
    { "59",  "Andorra"                                },
    { "60",  "Italy"                                  },
    { "61",  "Uganda"                                 },
    { "62",  "Colombia"                               },
    { "63",  "Czechia"                                },
    { "64",  "Cuba"                                   },
    { "65",  "Cyprus"                                 },
    { "66",  "United Kingdom"                         },
    { "67",  "Denmark"                                },
    { "68",  "Greenland"                              },
    { "69",  "Kenya"                                  },
    { "70",  "Germany"                                },
    { "71",  "Austria"                                },
    { "72",  "Switzerland"                            },
    { "73",  "Liechtenstein"                          },
    { "74",  "Luxembourg"                             },
    { "75",  "Niger"                                  },
    { "76",  "Maldives"                               },
    { "77",  "Senegal"                                },
    { "78",  "Bhutan"                                 },
    { "79",  "Estonia"                                },
    { "80",  "Togo"                                   },
    { "81",  "Greece"                                 },
    { "82",  "Caribbean"                              },
    { "83",  "Europe"                                 },
    { "84",  "Antigua and Barbuda"                    },
    { "85",  "Anguilla"                               },
    { "86",  "Australia"                              },
    { "87",  "Barbados"                               },
    { "88",  "Burundi"                                },
    { "89",  "Botswana"                               },
    { "90",  "Belize"                                 },
    { "91",  "Cocos (Keeling) Islands"                },
    { "92",  "Cook Islands"                           },
    { "93",  "Christmas Island"                       },
    { "94",  "Dominica"                               },
    { "95",  "Finland"                                },
    { "96",  "Fiji"                                   },
    { "97",  "Falkland Islands"                       },
    { "98",  "Micronesia"                             },
    { "99",  "Grenada"                                },
    { "100", "Guernsey"                               },
    { "101", "Gibraltar"                              },
    { "102", "Gambia"                                 },
    { "103", "Guam"                                   },
    { "104", "Guyana"                                 },
    { "105", "Hong Kong SAR"                          },
    { "106", "Indonesia"                              },
    { "107", "Ireland"                                },
    { "108", "Isle of Man"                            },
    { "109", "British Indian Ocean Territory"         },
    { "110", "Jersey"                                 },
    { "111", "Jamaica"                                },
    { "112", "Kiribati"                               },
    { "113", "Saint Kitts and Nevis"                  },
    { "114", "Cayman Islands"                         },
    { "115", "Saint Lucia"                            },
    { "116", "Liberia"                                },
    { "117", "Lesotho"                                },
    { "118", "Madagascar"                             },
    { "119", "Marshall Islands"                       },
    { "120", "Macao SAR"                              },
    { "121", "Northern Mariana Islands"               },
    { "122", "Montserrat"                             },
    { "123", "Malta"                                  },
    { "124", "Mauritius"                              },
    { "125", "Malawi"                                 },
    { "126", "Malaysia"                               },
    { "127", "Norfolk Island"                         },
    { "128", "Netherlands"                            },
    { "129", "Nauru"                                  },
    { "130", "Niue"                                   },
    { "131", "New Zealand"                            },
    { "132", "Papua New Guinea"                       },
    { "133", "Philippines"                            },
    { "134", "Pakistan"                               },
    { "135", "Pitcairn Islands"                       },
    { "136", "Puerto Rico"                            },
    { "137", "Palau"                                  },
    { "138", "Rwanda"                                 },
    { "139", "Solomon Islands"                        },
    { "140", "Seychelles"                             },
    { "141", "Sweden"                                 },
    { "142", "Singapore"                              },
    { "143", "St Helena, Ascension, Tristan da Cuhna" },
    { "144", "Slovenia"                               },
    { "145", "Sierra Leone"                           },
    { "146", "Sint Marteen"                           },
    { "147", "Swaziland"                              },
    { "148", "Turks and Caicos Islands"               },
    { "149", "Tokelau"                                },
    { "150", "Tonga"                                  },
    { "151", "Trinidad and Tobago"                    },
    { "152", "Tuvalu"                                 },
    { "153", "U.S. Outlying Islands"                  },
    { "154", "Saint Vincent and the Grenadines"       },
    { "155", "British Virgin Islands"                 },
    { "156", "U.S. Virgin Islands"                    },
    { "157", "Vanuatu"                                },
    { "158", "Samoa"                                  },
    { "159", "Zimbabwe"                               },
    { "160", "Latin America"                          },
    { "161", "Costa Rica"                             },
    { "162", "Dominican Republic"                     },
    { "163", "Ecuador"                                },
    { "164", "Equatorial Guinea"                      },
    { "165", "Guatemala"                              },
    { "166", "Honduras"                               },
    { "167", "Mexico"                                 },
    { "168", "Nicaragua"                              },
    { "169", "Panama"                                 },
    { "170", "Peru"                                   },
    { "171", "Paraguay"                               },
    { "172", "El Salvador"                            },
    { "173", "Uruguay"                                },
    { "174", "Venezuela"                              },
    { "175", "Iran"                                   },
    { "176", "Guinea"                                 },
    { "177", "Faroe Islands"                          },
    { "178", "Burkina Faso"                           },
    { "179", "Benin"                                  },
    { "180", "Saint Barthélemy"                       },
    { "181", "Congo (DRC)"                            },
    { "182", "Central African Republic"               },
    { "183", "Congo"                                  },
    { "184", "Côte d'Ivoire"                          },
    { "185", "Gabon"                                  },
    { "186", "French Guiana"                          },
    { "187", "Guadeloupe"                             },
    { "188", "Haiti"                                  },
    { "189", "Monaco"                                 },
    { "190", "Saint Martin"                           },
    { "191", "Martinique"                             },
    { "192", "New Caledonia"                          },
    { "193", "French Polynesia"                       },
    { "194", "Saint Pierre and Miquelon"              },
    { "195", "Réunion"                                },
    { "196", "Wallis and Futuna"                      },
    { "197", "Mayotte"                                },
    { "198", "Croatia"                                },
    { "199", "Hungary"                                },
    { "200", "Iceland"                                },
    { "201", "San Marino"                             },
    { "202", "Vatican City"                           },
    { "203", "Japan"                                  },
    { "204", "Georgia"                                },
    { "205", "Cabo Verde"                             },
    { "206", "Kazakhstan"                             },
    { "207", "Cambodia"                               },
    { "208", "North Korea"                            },
    { "209", "Korea"                                  },
    { "210", "Kyrgyzstan"                             },
    { "211", "Laos"                                   },
    { "212", "Angola"                                 },
    { "213", "Lithuania"                              },
    { "214", "Latvia"                                 },
    { "215", "Mozambique"                             },
    { "216", "Macedonia, FYRO"                        },
    { "217", "Mongolia"                               },
    { "218", "Myanmar"                                },
    { "219", "Norway"                                 },
    { "220", "Svalbard and Jan Mayen"                 },
    { "221", "Nepal"                                  },
    { "222", "Aruba"                                  },
    { "223", "Bonaire, Sint Eustatius and Saba"       },
    { "224", "Curaçao"                                },
    { "225", "Suriname"                               },
    { "226", "Poland"                                 },
    { "227", "Portugal"                               },
    { "228", "Guinea-Bissau"                          },
    { "229", "São Tomé and Principe"                  },
    { "230", "Timor-Leste"                            },
    { "231", "Romania"                                },
    { "232", "Moldova"                                },
    { "233", "Ukraine"                                },
    { "234", "Sri Lanka"                              },
    { "235", "Slovakia"                               },
    { "236", "Albania"                                },
    { "237", "Kosovo"                                 },
    { "238", "Montenegro"                             },
    { "239", "Serbia"                                 },
    { "240", "Åland Islands"                          },
    { "241", "Tajikistan"                             },
    { "242", "Thailand"                               },
    { "243", "Turkmenistan"                           },
    { "244", "Turkey"                                 },
    { "245", "Uzbekistan"                             },
    { "246", "Vietnam"                                },
    { "247", "Taiwan"                                 },
    { "248", "USA"                                    },
    { "249", "Asia"                                   },
  });

  return sRegions;
}

HashMap<String, String> RequestHandlerTools::GetAvailableScreensavers()
{
  static HashMap<String, String> sScreensavers({
    { "dim"     , "DIM" },
    { "black"   , "BLACK" },
    { "demo"    , "DEMO" },
    { "gameclip", "GAMECLIP" },
    { "bouncing", "BOUNCING" },
  });

  if (Board::Instance().HasSuspendResume())
    sScreensavers.insert_unique({"suspend", "SUSPEND"});

  return sScreensavers;
}

HashMap<String, String> RequestHandlerTools::GetAvailableScreens()
{
  // Until there is a way to get screen names
  static HashMap<String, String> sScreens({
    { "None", "None" },
    { "eDP", "eDP" },
    { "HDMI-A-0", "HDMI-A-0" },
  });

  return sScreens;
}

HashMap<String, String> RequestHandlerTools::GetUpdateTypeEntries()
{
  HashMap<String, String> sUpdateTypes({
    { "stable", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::Stable) },
    { "patron", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::Patron) },
    { "patron-alpha", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::PatronAlpha) },
  });

  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::Alpha)
    sUpdateTypes.insert_unique({ "alpha", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::Alpha) });

  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::RRGBD2)
    sUpdateTypes.insert_unique({ "rrgbd2", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::RRGBD2) });

  if (RecalboxConf::Instance().GetUpdateType() == RecalboxConf::UpdateType::JVS)
    sUpdateTypes.insert_unique({ "jvs", RecalboxConf::UpdateTypeFromEnum(RecalboxConf::UpdateType::JVS) });

  return sUpdateTypes;
}

HashMap<String, String> RequestHandlerTools::GetAvailableScrapers()
{
  static HashMap<String, String> sPatronInfos({
    { "ScreenScraper", "ScreenScraper" }
  });

  if (PatronInfo::Instance().IsPatron() && PatronInfo::Instance().BossLevel() >= 2)
    sPatronInfos.insert_unique({ "Recalbox", "Recalbox" });

  return sPatronInfos;
}

String::List RequestHandlerTools::GetOneGameOneRomPreferedFallback()
{
  static String::List sFallbacks
  ({
    "World > USA > Europe > Japan",
    "World > USA > Japan > Europe",
    "World > Europe > USA > Japan",
    "World > Europe > Japan > USA",
    "World > Japan > USA > Europe",
    "World > Japan > Europe > USA",
    "USA > World > Europe > Japan",
    "USA > World > Japan > Europe",
    "USA > Europe > World > Japan",
    "USA > Europe > Japan > World",
    "USA > Japan > World > Europe",
    "USA > Japan > Europe > World",
    "Europe > World > USA > Japan",
    "Europe > World > Japan > USA",
    "Europe > USA > World > Japan",
    "Europe > USA > Japan > World",
    "Europe > Japan > World > USA",
    "Europe > Japan > USA > World",
    "Japan > World > USA > Europe",
    "Japan > World > Europe > USA",
    "Japan > USA > World > Europe",
    "Japan > USA > Europe > World",
    "Japan > Europe > World > USA",
    "Japan > Europe > USA > World",
    "Asia > World > USA > Europe > Japan",
    "Asia > World > USA > Japan > Europe",
    "Asia > World > Japan > USA > Europe",
    "Asia > World > Japan > Europe > USA",
    "Latin America > World > USA > Europe > Japan",
    "Latin America > World > Europe > USA > Japan",
    "Latin America > USA > World > Europe > Japan",
    "Latin America > Europe > World > USA > Japan",
  });

  return sFallbacks;
}

HashMap<String, String> RequestHandlerTools::GetCRConsoleModeExit()
{
  static HashMap<String, String> sCRConsoleModeExit
  ({
    { "HK + START" , "hkstart"   },
    { "START 5 SEC", "start5sec" }
  });

  return sCRConsoleModeExit;
}

// HashMap<String, String> RequestHandlerTools::GetCRConsoleModeVideo()
// {
//   static HashMap<String, String> sCRConsoleModeVideo;
//
//   for (const auto& file : Path(CardReader::VideoPath).GetDirectoryContent(false))
//     if (file.Extension() == ".mp4")
//       sCRConsoleModeVideo.insert_unique(file.FilenameWithoutExtension(), file.FilenameWithoutExtension());
//   if (sCRConsoleModeVideo.empty())
//     sCRConsoleModeVideo.insert_unique("NONE", "none");
//
//   return sCRConsoleModeVideo;
// }

HashMap<String, String> RequestHandlerTools::GetCRConsoleModeSound()
{
  static HashMap<String, String> sCRConsoleModeSound
  ({
  { "Bios"          , "bios"  },
  { "Recalbox Music", "music" }
  });

  return sCRConsoleModeSound;
}

HashMap<String, String> RequestHandlerTools::GetAvailableKeyboardLayout()
{
  static HashMap<String, String> kbLayouts;

  if (kbLayouts.empty())
  {
    static Path layouts("/usr/share/keymaps/i386");
    static String ext(".map.gz");
    for (const char* sub : { "azerty", "bepo", "qwerty", "qwertz" })
      for(const Path& filePath : (layouts / sub).GetDirectoryContent())
      {
        String fileName = filePath.Filename();
        if (fileName.EndsWith(ext))
        {
          fileName = fileName.SubString(0, fileName.Count() - ext.Count());
          kbLayouts.insert_unique(String(fileName), String(sub) + ":" + fileName);
        }
      }
  }

  return kbLayouts;
}

void RequestHandlerTools::GetAvailableShadersIn(const Path& rootPath, String::List& results)
{
  Path::PathList pathList = rootPath.GetDirectoryContent();
  for(const Path& path : pathList)
    if (path.IsDirectory())
      GetAvailableShadersIn(path, results);
    else if (path.IsFile())
      if (path.Extension() == ".glslp")
        results.push_back(path.ToString());
}

String::List RequestHandlerTools::GetAvailableShaders()
{
  String::List result;
  static Path shaderPath("/recalbox/share/shaders");

  GetAvailableShadersIn(shaderPath, result);
  int commonPartLength = (int)shaderPath.ToString().length() + 1;
  for(String& path : result)
    path.erase(0, commonPartLength);

  return result;
}HashMap<String, String> RequestHandlerTools::GetAvailableThemes()
{
  ThemeManager::ThemeList themeSets = ThemeManager::AvailableThemes();

  // Sort names
  std::vector<ThemeSpec> list;
  for (const auto& theme : themeSets) list.push_back({ theme.first, theme.second });
  std::sort(list.begin(), list.end(), [](const ThemeSpec& a, const ThemeSpec& b) { return a.Name.ToLowerCase() < b.Name.ToLowerCase(); });

  // Get current state from global resolver for compatibility check
  const auto& resolver = ThemeManager::Instance().GlobalResolver();
  const bool tate = resolver.IsTate();
  const ThemeData::Compatibility currentCompatibility = (resolver.HasJamma() ? ThemeData::Compatibility::Jamma : ThemeData::Compatibility::None) |
                                                        (resolver.HasCrt() ? ThemeData::Compatibility::Crt : ThemeData::Compatibility::None) |
                                                        (resolver.HasHDMI() ? ThemeData::Compatibility::Hdmi : ThemeData::Compatibility::None);
  const ThemeData::Resolutions currentResolution = resolver.IsQVGA() ? ThemeData::Resolutions::QVGA :
                                                   resolver.IsVGA() ? ThemeData::Resolutions::VGA :
                                                   resolver.IsHD() ? ThemeData::Resolutions::HD :
                                                   ThemeData::Resolutions::FHD;

  HashMap<String, String> result;
  for (const ThemeSpec& theme : list)
  {
    ThemeData::Compatibility compatibility;
    ThemeData::Resolutions resolutions;
    String displayableName;
    String author;
    int version, recalboxVersion;

    if (ThemeData::FetchCompatibility(theme.FolderPath / ThemeManager::sRootThemeFile,
                                   compatibility, resolutions, displayableName,
                                   author, version, recalboxVersion))
    {
      const bool compatible = ((currentCompatibility & compatibility) != ThemeData::Compatibility::None) &&
                              ((currentResolution & resolutions) != ThemeData::Resolutions::None) &&
                              (!tate || hasFlag(compatibility, ThemeData::Compatibility::Tate)) &&
                              (recalboxVersion >= 9);

      displayableName.Insert(0, compatible ? "\u2713 " : "\u26a0 ");

      result.insert_unique(theme.FolderPath.Filename(), std::move(displayableName));
    }
  }

  return result;
}

void RequestHandlerTools::GetEmbeddedBios(const Path& base, HashMap<String, bool>& results)
{
  // Read all embedded path. Bas must be /recalbox/share/ as bios is already in path of every line
  String::List pathlist = Files::LoadFile(Path("/recalbox/share_init/embedded.list")).Split('\n');
  results.clear();
  for(const String& path : pathlist)
    results.insert_unique(String((base / path).ToString()), true);
}

void RequestHandlerTools::GetAvailableBios(const Path& rootPath, Path::PathList& results, const HashMap<String, bool>& embedded)
{
  Path::PathList pathList = rootPath.GetDirectoryContent();
  for(const Path& path : pathList)
    if (path.IsDirectory())
      GetAvailableBios(path, results, embedded);
    else if (path.IsFile())
      if (!embedded.contains(path.ToString()))
        results.push_back(path);
}

Path RequestHandlerTools::GetCompressedBiosFolder()
{
  Path baseFolder("/recalbox/share/bios");

  // Get embedded list
  HashMap<String, bool> embedded;
  GetEmbeddedBios(baseFolder.Directory(), embedded);

  // Get diff list embedded/user bios
  Path::PathList biosPath;
  GetAvailableBios(baseFolder, biosPath, embedded);

  Path destination("/recalbox/share/system/bios_backup.zip");
  {
    Zip zip(destination, true);
    for(const Path& path : biosPath)
      zip.Add(path, baseFolder);
  }

  return destination;
}

bool RequestHandlerTools::IsValidSystem(const String& system, const SystemManager& manager)
{
  HashMap<String, String> systemList = RequestHandlerTools::GetSupportedSystemList(manager);
  return systemList.contains(system);
}

bool
RequestHandlerTools::GetUploadedFile(const Rest::Request& request, String& filename, int& startOffset, int& size)
{
  #define GUF_DOUBLE_EOF "\r\n\r\n"
  #define GUF_BOUNDARY "boundary="
  #define GUF_FILENAME "filename=\""

  // Get barrier
  const auto* contentType = request.headers().tryGet<Http::Header::ContentType>().get();
  if (contentType == nullptr) return false;
  String boundary = contentType->mime().raw();
  size_t start = boundary.find(GUF_BOUNDARY);
  if (start == String::npos) return false;
  boundary.erase(0, start + (sizeof(GUF_BOUNDARY) - 1));
  boundary.Insert(0, "--", 2);

  LOG(LogError) << "BOUNDARY: " << boundary;

  // Lookup filename
  start = request.body().find(boundary, 0);
  if (start != String::npos)
    for(;;)
    {
      // Find start/stop
      size_t end = request.body().find(boundary, start + boundary.size());
      if (end == String::npos) break;

      // Get EOL
      size_t headerStart = request.body().find('\n', start + boundary.size());
      if (headerStart != String::npos && headerStart < end)
      {
        // Get double EOL
        size_t headerStop = request.body().find(GUF_DOUBLE_EOF, ++headerStart);
        if (headerStop != String::npos && headerStop < end)
        {
          // locate filename
          size_t fileNameStart = request.body().find(GUF_FILENAME, headerStart);
          if (headerStart != String::npos && headerStart < headerStop)
          {
            fileNameStart += (sizeof(GUF_FILENAME) - 1);
            const size_t fileNameStop = request.body().find('"', fileNameStart);
            if (fileNameStop != String::npos && fileNameStop < headerStop)
            {
              // Extract filename
              filename = request.body().substr(fileNameStart, fileNameStop - fileNameStart);
              // Store indexes
              startOffset = (int)(headerStop + sizeof(GUF_DOUBLE_EOF) - 1);
              size = (int)(end - startOffset) - 2 /* size of last EOF */;
              return true;
            }
          }
        }
      }
      // Restart from next boundary
      start = end;
    }

  #undef GUF_FILENAME
  #undef GUF_BOUNDARY
  #undef GUF_DOUBLE_EOF

  return false;
}

bool RequestHandlerTools::ExtractArray(const Rest::Request& request, String::List& result)
{
  rapidjson::Document json;
  json.Parse(request.body().c_str());
  if (!json.HasParseError())
    if (json.GetType() == rapidjson::Type::kArrayType)
    {
      for (const auto& key : json.GetArray())
        result.emplace_back(key.GetString());
      return true;
    }

  return false;
}

FileData* RequestHandlerTools::GetGame(SystemManager& manager, const String& systemName, const String& romFullPath)
{
  // Extract system
  SystemData* system = manager.SystemByName(systemName);
  if (system == nullptr) return nullptr;

  // Lookup game
  return system->MasterRoot().LookupGameByFilePath(romFullPath);
}

void RequestHandlerTools::SendGameResource(SystemManager& manager, const String& systemName, const String& gameFullPath, Media media,
                                           Pistache::Http::ResponseWriter& response)
{
  FileData* game = RequestHandlerTools::GetGame(manager, systemName, gameFullPath);
  if (game != nullptr)
  {
    Path mediaPath;
    switch(media)
    {
      case Media::Image: mediaPath = game->Metadata().Image(); break;
      case Media::Box: mediaPath = game->Metadata().Box(); break;
      case Media::Video: mediaPath = game->Metadata().Video(); break;
      case Media::Map:
      case Media::Manual:
      default: break;
    }
    SendMedia(mediaPath, response);
  }
  else RequestHandlerTools::Error404(response);
}

void RequestHandlerTools::SendGameMetadataInformation(SystemManager& manager,
                                                     const String& systemName, const String& gameFullPath,
                                                     Http::ResponseWriter& response)
{
  FileData* game = RequestHandlerTools::GetGame(manager, systemName, gameFullPath);
  if (game != nullptr)
  {
    JSONBuilder result;
    MetadataDescriptor& meta = game->Metadata();
    result.Open()
            .Field("name", meta.Name())
            .Field("synopsys", meta.Description())
            .Field("publisher", meta.Publisher())
            .Field("developer", meta.Developer())
            .Field("emulator", meta.Emulator())
            .Field("core", meta.Core())
            .Field("ratio", meta.Ratio())
            .Field("releaseDate", (long long int)meta.ReleaseDateEpoc())
            .Field("lastPlayed", meta.LastPlayed().ToEpochTime())
            .Field("regions", Regions::Serialize4Regions(meta.Region()))
            .Field("playCount", meta.PlayCount())
            .Field("crc32", meta.RomCrc32AsString())
            .Field("adult", meta.Adult())
            .Field("favorite", meta.Favorite())
            .Field("hidden", meta.Hidden())
            .Field("rating", meta.Rating())
            .OpenObject("players").Field("min", meta.PlayerMin()).Field("max", meta.PlayerMax()).CloseObject()
            .OpenObject("genres").Field("free", meta.Genre().Replace(',', ", ", 2).Replace(" ", 2, ' ')).Field("normalized", (int)meta.GenreId()).CloseObject()
            .OpenObject("availableMedia")
              .Field("hasImage", meta.HasImage())
              .Field("hasBox", meta.HasBox())
              .Field("hasVideo", meta.HasVideo())
            .CloseObject()
          .Close();
    RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
  }
  else RequestHandlerTools::Error404(response);
}

JSONBuilder RequestHandlerTools::SerializeSystemListToJSON(const SystemManager::List& array)
{
  class Serializer : public ISerializeToJson<const SystemData>
  {
    public:
      // Serialize a single game
      JSONBuilder Serialize(const SystemData* system) override
      {
        const SystemDescriptor& desc = system->Descriptor();

        // Rom path
        String::List romPath;
        for(const RootFolderData* root : system->MasterRoot().SubRoots())
          romPath.push_back(root->RomPath().ToString());

        // Emulator/cores
        JSONBuilder::JSONList cores;
        const EmulatorList& emulatorList = desc.EmulatorTree();
        for(int i = 0; i < emulatorList.Count(); ++i)
        {
          const EmulatorDescriptor& descriptor = emulatorList.EmulatorAt(i);
          for(int c = 0; c < descriptor.CoreCount(); ++c)
          {
            const EmulatorDescriptor::CoreData& core = descriptor.Core(c);
            JSONBuilder emulators;
            emulators.Open()
                       .Field("emulator", descriptor.Name())
                       .Field("core", core.Name())
                       .Field("extensions", core.Extensions())
                       .Field("availableOnCRT", core.CRTAvailable())
                       .Field("hasNetplay", core.Netplay())
                       .Field("priority", core.Priority())
                       .Field("speed", (int)core.AverageSpeed())
                       .Field("compatibility", (int)core.CompatibilityRate())
                     .Close();
            cores.push_back(emulators);
          }
        }

        // System
        RecalboxConf& configuration = RecalboxConf::Instance();
        JSONBuilder result;
        result.Open()
                .Field("name", desc.Name())
                .Field("fullName", desc.FullName())
                .Field("uuid", desc.GUID())
                .Field("themeFolder", desc.ThemeFolder())
                .Field("themeRegion", configuration.AsString("emulationstation.theme.region"))
                .Field("manufacturer", desc.Manufacturer())
                .Field("releaseDate", desc.ReleaseDate().ToStringFormat("%YYYY-%MM"))
                .Field("romPath", romPath)
                .Field("extensions", desc.Extension())
                .Field("type", (int)desc.Type())
                .Field("ignoredFiles", desc.IgnoredFiles())
                .OpenObject("inputs")
                  .Field("pads", (int)desc.PadRequirement())
                  .Field("keyboard", (int)desc.KeyboardRequirement())
                  .Field("mouse", (int)desc.MouseRequirement())
                .CloseObject()
                .OpenObject("properties")
                  .Field("hasLightgunSupport", desc.LightGun())
                  .Field("isReadOnly", desc.IsReadOnly())
                  .Field("isPort", desc.IsPort())
                  .Field("hasNetplay", desc.HasNetPlayCores())
                .CloseObject()
                .Field("emulators", cores)
              .Close();

        return result;
      }
  } serializer;

  HashMap<String, String> speeds;
  for(int i = 0; i < (int)EmulatorDescriptor::Speed::__Count; ++i)
    speeds[String(i)] = ConvertEmulatorSpeed((EmulatorDescriptor::Speed)i);
  HashMap<String, String> compatibility;
  for(int i = 0; i < (int)EmulatorDescriptor::Compatibility::__Count; ++i)
    compatibility[String(i)] = ConvertEmulatorCompatibility((EmulatorDescriptor::Compatibility)i);
  HashMap<String, String> requirements;
  for(int i = 0; i < (int)SystemDescriptor::DeviceRequirement::__Count; ++i)
    requirements[String(i)] = ConvertDeviceRequirement((SystemDescriptor::DeviceRequirement)i);
  HashMap<String, String> types;
  for(int i = 0; i < (int)SystemDescriptor::SystemType::__Count; ++i)
    types[String(i)] = ConvertSystemType((SystemDescriptor::SystemType)i);

  JSONBuilder systems;
  systems.Open()
           .OpenObject("enumerations")
             .Field("systemTypes", types)
             .Field("deviceRequirement", requirements)
             .Field("emulatorSpeed", speeds)
             .Field("emulatorCompatibility", compatibility)
           .CloseObject()
           .Field<SystemData>("systems", array, serializer)
         .Close();

  return systems;
}

bool RequestHandlerTools::IsPathConfinedTo(const Path& path, const String& root)
{
  const String& p = path.ToString();
  // Reject any parent-directory traversal segment in the requested path.
  if (p.Contains("/../") || p.EndsWith("/..") || p.StartsWith("../") || p == "..") return false;
  // Confine under root, with a path boundary so ".../share-evil" does not match ".../share".
  String r(root);
  while (r.EndsWith("/")) r = r.SubString(0, (int)r.size() - 1);
  return p == r || p.StartsWith(r + "/");
}

void RequestHandlerTools::SendMedia(const Path& mediaPath, Http::ResponseWriter& response)
{
  // Guard against path traversal: the path may originate from a client (MediaGet decodes a base64 path).
  // Confine to the media data roots; combined with the extension whitelist below this blocks arbitrary reads.
  if (!IsPathConfinedTo(mediaPath, "/recalbox/share") && !IsPathConfinedTo(mediaPath, "/recalbox/share_init"))
  {
    RequestHandlerTools::Error404(response);
    return;
  }
  // Check extension
  String ext = mediaPath.Extension().LowerCase();
  if (mediaPath.Exists())
  {
    if (mediaPath.IsFile())
    {
      // Text
      if (ext == ".txt")      RequestHandlerTools::SendResource(mediaPath, response, Mime::PlainText);
      else if (ext == ".pdf") RequestHandlerTools::SendResource(mediaPath, response, Mime::FilePdf);
      // Image
      else if (ext == ".gif") RequestHandlerTools::SendResource(mediaPath, response, Mime::ImageGif);
      else if (ext == ".jpg") RequestHandlerTools::SendResource(mediaPath, response, Mime::ImageJpg);
      else if (ext == ".png") RequestHandlerTools::SendResource(mediaPath, response, Mime::ImagePng);
      else if (ext == ".svg") RequestHandlerTools::SendResource(mediaPath, response, Mime::ImageSvg);
      // Video
      else if (ext == ".mkv") RequestHandlerTools::SendResource(mediaPath, response, Mime::VideoMkv);
      else if (ext == ".mp4") RequestHandlerTools::SendResource(mediaPath, response, Mime::VideoMp4);
      else if (ext == ".avi") RequestHandlerTools::SendResource(mediaPath, response, Mime::VideoAvi);
      else if (ext == ".webm") RequestHandlerTools::SendResource(mediaPath, response, Mime::VideoWebm);
      // Unknown
      else RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Invalid media extension!", Mime::PlainText);
    }
    else RequestHandlerTools::Send(response, Http::Code::Bad_Request, "Target is a directory!", Mime::PlainText);
  }
  else RequestHandlerTools::Error404(response);
}

void RequestHandlerTools::GetThemeKeyValue(const String& name, const char* key, Http::ResponseWriter& response)
{
  String themeKey(sThemeKeyValue);
  themeKey.Replace("%NAME%", name)
          .Replace("%KEY%", key);
  String option = themeKey;

  RecalboxConf& configuration = RecalboxConf::Instance();

  JSONBuilder result;
  result.Open()
        .Field("exist", configuration.Exists(option))
        .Field(key, configuration.AsString(option))
        .Close();

  RequestHandlerTools::Send(response, Http::Code::Ok, result, Mime::Json);
}
