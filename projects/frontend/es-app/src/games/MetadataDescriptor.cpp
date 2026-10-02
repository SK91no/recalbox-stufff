#include "MetadataDescriptor.h"
#include "MetadataFieldDescriptor.h"
#include "utils/locale/LocaleHelper.h"

// TODO: Use const char* instead
const String MetadataDescriptor::GameNodeIdentifier("game");
const String MetadataDescriptor::FolderNodeIdentifier("folder");

MetadataStringHolder<16> MetadataDescriptor::sNameHolder(1 << 20, 128 << 10);
MetadataStringHolder<16> MetadataDescriptor::sDescriptionHolder(1 << 20, 128 << 10);
MetadataStringHolder<12> MetadataDescriptor::sDeveloperHolder(64 << 10, 32 << 10);
MetadataStringHolder<12> MetadataDescriptor::sPublisherHolder(64 << 10, 32 << 10);
MetadataStringHolder<12> MetadataDescriptor::sGenreHolder(64 << 10, 32 << 10);
MetadataStringHolder<8>  MetadataDescriptor::sEmulatorHolder(2 << 10, 1 << 10);
MetadataStringHolder<8>  MetadataDescriptor::sCoreHolder(2 << 10, 1 << 10);
MetadataStringHolder<8>  MetadataDescriptor::sRatioHolder(1 << 10, 1 << 10);
MetadataStringHolder<12> MetadataDescriptor::sPathHolder(64 << 10, 32 << 10);
MetadataStringHolder<16> MetadataDescriptor::sFileHolder(128 << 10, 32 << 10);
MetadataStringHolder<16> MetadataDescriptor::sMapHolder(8 << 10, 4 << 10);

#ifdef _METADATA_STATS_
int MetadataDescriptor::LivingClasses = 0;
int MetadataDescriptor::LivingFolders = 0;
int MetadataDescriptor::LivingGames = 0;
#endif

const MetadataFieldDescriptor* MetadataDescriptor::GetMetadataFieldDescriptors(ItemType type, int& count)
{
  switch(type)
  {
    case ItemType::Game:
    {
      static const MetadataFieldDescriptor sGameMetadataDescriptors[] =
      {
        MetadataFieldDescriptor("path"          , ""     , MetadataType::Path              ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultRom               , &MetadataDescriptor::RomAsString               , nullptr, &MetadataDescriptor::SetRomPathAsString           , nullptr, false, true , false),
        MetadataFieldDescriptor("name"          , ""     , MetadataType::Alias             ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultName              , &MetadataDescriptor::NameAsString              , nullptr, &MetadataDescriptor::SetName                      , nullptr, false, true , false),
        MetadataFieldDescriptor("aliases"       , ""     , MetadataType::License           ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultAliases           , &MetadataDescriptor::AliasesAsString           , nullptr, &MetadataDescriptor::SetAliases                   , nullptr, false, true , false),
        MetadataFieldDescriptor("licences"      , ""     , MetadataType::Name              ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultLicences          , &MetadataDescriptor::LicencesAsString          , nullptr, &MetadataDescriptor::SetLicences                  , nullptr, false, true , false),
        MetadataFieldDescriptor("rating"        , "0.0"  , MetadataType::Rating            ,MetadataFieldDescriptor::DataType::Rating , MetadataFieldDescriptor::EditableType::Rating , &MetadataDescriptor::IsDefaultRating            , &MetadataDescriptor::RatingAsString            , nullptr, &MetadataDescriptor::SetRatingAsString            , nullptr, false, true , false),
        MetadataFieldDescriptor("favorite"      , "false", MetadataType::Favorite          ,MetadataFieldDescriptor::DataType::Bool   , MetadataFieldDescriptor::EditableType::Switch , &MetadataDescriptor::IsDefaultFavorite          , &MetadataDescriptor::FavoriteAsString          , nullptr, &MetadataDescriptor::SetFavoriteAsString          , nullptr, false, true , true ),
        MetadataFieldDescriptor("hidden"        , "false", MetadataType::Hidden            ,MetadataFieldDescriptor::DataType::Bool   , MetadataFieldDescriptor::EditableType::Switch , &MetadataDescriptor::IsDefaultHidden            , &MetadataDescriptor::HiddenAsString            , nullptr, &MetadataDescriptor::SetHiddenAsString            , nullptr, false, true , true ),
        MetadataFieldDescriptor("emulator"      , ""     , MetadataType::Emulator          ,MetadataFieldDescriptor::DataType::List   , MetadataFieldDescriptor::EditableType::List   , &MetadataDescriptor::IsDefaultEmulator          , &MetadataDescriptor::EmulatorAsString          , nullptr, &MetadataDescriptor::SetEmulator                  , nullptr, false, true , true ),
        MetadataFieldDescriptor("core"          , ""     , MetadataType::Core              ,MetadataFieldDescriptor::DataType::List   , MetadataFieldDescriptor::EditableType::List   , &MetadataDescriptor::IsDefaultCore              , &MetadataDescriptor::CoreAsString              , nullptr, &MetadataDescriptor::SetCore                      , nullptr, false, true , true ),
        MetadataFieldDescriptor("ratio"         , ""     , MetadataType::Ratio             ,MetadataFieldDescriptor::DataType::List   , MetadataFieldDescriptor::EditableType::List   , &MetadataDescriptor::IsDefaultRatio             , &MetadataDescriptor::RatioAsString             , nullptr, &MetadataDescriptor::SetRatio                     , nullptr, false, true , true ),
        MetadataFieldDescriptor("desc"          , ""     , MetadataType::Synopsis          ,MetadataFieldDescriptor::DataType::Text   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultDescription       , &MetadataDescriptor::DescriptionAsString       , nullptr, &MetadataDescriptor::SetDescription               , nullptr, false, false, false),
        MetadataFieldDescriptor("image"         , ""     , MetadataType::Image             ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultImage             , &MetadataDescriptor::ImageAsString             , nullptr, &MetadataDescriptor::SetImagePathAsString         , nullptr, false, false, false),
        MetadataFieldDescriptor("box"           , ""     , MetadataType::Box               ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultBox               , &MetadataDescriptor::BoxAsString               , nullptr, &MetadataDescriptor::SetBoxPathAsString           , nullptr, false, false, false),
        MetadataFieldDescriptor("manual"        , ""     , MetadataType::Manual            ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultManual            , &MetadataDescriptor::ManualAsString            , nullptr, &MetadataDescriptor::SetManualPathAsString        , nullptr, false, false, false),
        MetadataFieldDescriptor("maps"          , ""     , MetadataType::Map               ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultMap               , &MetadataDescriptor::MapAsString               , &MetadataDescriptor::SerializeMapsAsXml, nullptr                                           , &MetadataDescriptor::SetMapsAsXml, false, false, false),
        MetadataFieldDescriptor("logo"          , ""     , MetadataType::Logo              ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultLogo              , &MetadataDescriptor::LogoAsString              , nullptr, &MetadataDescriptor::SetLogoPathAsString          , nullptr, false, false, false),
        MetadataFieldDescriptor("video"         , ""     , MetadataType::Video             ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultVideo             , &MetadataDescriptor::VideoAsString             , nullptr, &MetadataDescriptor::SetVideoPathAsString         , nullptr, false, false, false),
        MetadataFieldDescriptor("tips"          , ""     , MetadataType::TipsAndTricks     ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultTipsAndTricks     , &MetadataDescriptor::TipsAndTricksAsString     , nullptr, &MetadataDescriptor::SetTipsAndTricksPathAsString , nullptr, false, false, false),
        MetadataFieldDescriptor("releasedate"   , ""     , MetadataType::ReleaseDate       ,MetadataFieldDescriptor::DataType::Date   , MetadataFieldDescriptor::EditableType::Date   , &MetadataDescriptor::IsDefaultReleaseDateEpoc   , &MetadataDescriptor::ReleaseDateAsString       , nullptr, &MetadataDescriptor::SetReleaseDateAsString       , nullptr, false, false, false),
        MetadataFieldDescriptor("developer"     , ""     , MetadataType::Developer         ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultDeveloper         , &MetadataDescriptor::DeveloperAsString         , nullptr, &MetadataDescriptor::SetDeveloper                 , nullptr, false, false, false),
        MetadataFieldDescriptor("publisher"     , ""     , MetadataType::Publisher         ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultPublisher         , &MetadataDescriptor::PublisherAsString         , nullptr, &MetadataDescriptor::SetPublisher                 , nullptr, false, false, false),
        MetadataFieldDescriptor("genre"         , ""     , MetadataType::Genre             ,MetadataFieldDescriptor::DataType::Int    , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultGenre             , &MetadataDescriptor::GenreAsString             , nullptr, &MetadataDescriptor::SetGenre                     , nullptr, false, false, false),
        MetadataFieldDescriptor("genreid"       , ""     , MetadataType::GenreId           ,MetadataFieldDescriptor::DataType::Int    , MetadataFieldDescriptor::EditableType::List   , &MetadataDescriptor::IsDefaultGenreId           , &MetadataDescriptor::GenreIdAsString           , nullptr, &MetadataDescriptor::SetGenreIdAsString           , nullptr, false, false, false),
        MetadataFieldDescriptor("adult"         , ""     , MetadataType::Adult             ,MetadataFieldDescriptor::DataType::Bool   , MetadataFieldDescriptor::EditableType::Switch , &MetadataDescriptor::IsDefaultAdult             , &MetadataDescriptor::AdultAsString             , nullptr, &MetadataDescriptor::SetAdultAsString             , nullptr, false, false, false),
        MetadataFieldDescriptor("players"       , "1"    , MetadataType::Players           ,MetadataFieldDescriptor::DataType::Range  , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultPlayerRange       , &MetadataDescriptor::PlayersAsString           , nullptr, &MetadataDescriptor::SetPlayersAsString           , nullptr, false, false, false),
        MetadataFieldDescriptor("region"        , ""     , MetadataType::Region            ,MetadataFieldDescriptor::DataType::String , MetadataFieldDescriptor::EditableType::Text   , &MetadataDescriptor::IsDefaultRegion            , &MetadataDescriptor::RegionAsString            , nullptr, &MetadataDescriptor::SetRegionAsString            , nullptr, false, false, false),
        MetadataFieldDescriptor("playcount"     , "0"    , MetadataType::PlayCount         ,MetadataFieldDescriptor::DataType::Int    , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultPlayCount         , &MetadataDescriptor::PlayCountAsString         , nullptr, &MetadataDescriptor::SetPlayCountAsString         , nullptr, true , false, true ),
        MetadataFieldDescriptor("lastplayed"    , "0"    , MetadataType::LastPlayed        ,MetadataFieldDescriptor::DataType::Date   , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultLastPlayedEpoc    , &MetadataDescriptor::LastPlayedAsString        , nullptr, &MetadataDescriptor::SetLastPlayedAsString        , nullptr, true , false, true ),
        MetadataFieldDescriptor("hash"          , "0"    , MetadataType::Crc32             ,MetadataFieldDescriptor::DataType::Crc32  , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultRomCrc32          , &MetadataDescriptor::RomCrc32AsString          , nullptr, &MetadataDescriptor::SetRomCrc32AsString          , nullptr, true , false, false),
        MetadataFieldDescriptor("lastPatch"     , ""     , MetadataType::LastPatch         ,MetadataFieldDescriptor::DataType::Path   , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultLastPath          , &MetadataDescriptor::LastPatchAsString         , nullptr, &MetadataDescriptor::SetLastPatchAsString         , nullptr, true , false, false),
        MetadataFieldDescriptor("rotation"      , "None" , MetadataType::Rotation          ,MetadataFieldDescriptor::DataType::Int    , MetadataFieldDescriptor::EditableType::Switch , &MetadataDescriptor::IsDefaultRotation          , &MetadataDescriptor::RotationAsString          , nullptr, &MetadataDescriptor::SetRotationAsString          , nullptr, false, false, false),
        MetadataFieldDescriptor("timeplayed"    , "0"    , MetadataType::TimePlayed        ,MetadataFieldDescriptor::DataType::Int    , MetadataFieldDescriptor::EditableType::None   , &MetadataDescriptor::IsDefaultTotalPlayTime     , &MetadataDescriptor::TotalPlayTimeAsString     , nullptr, &MetadataDescriptor::SetTotalPlayTimeAsString     , nullptr, false, false, true ),
      };

      count = sizeof(sGameMetadataDescriptors) / sizeof(MetadataFieldDescriptor);
      return &sGameMetadataDescriptors[0];
    }
    case ItemType::Folder:
    {
      static const MetadataFieldDescriptor sFolderMetadataDescriptors[] =
      {
        MetadataFieldDescriptor("path"       , ""        , MetadataType::Path        ,MetadataFieldDescriptor::DataType::Path  , MetadataFieldDescriptor::EditableType::None  , &MetadataDescriptor::IsDefaultRom            , &MetadataDescriptor::RomAsString         , nullptr, &MetadataDescriptor::SetRomPathAsString      , nullptr, false, true , false),
        MetadataFieldDescriptor("name"       , ""        , MetadataType::Name        ,MetadataFieldDescriptor::DataType::String, MetadataFieldDescriptor::EditableType::Text  , &MetadataDescriptor::IsDefaultName           , &MetadataDescriptor::NameAsString        , nullptr, &MetadataDescriptor::SetName                 , nullptr, false, true , false),
        MetadataFieldDescriptor("hidden"     , "false"   , MetadataType::Hidden      ,MetadataFieldDescriptor::DataType::Bool  , MetadataFieldDescriptor::EditableType::Switch, &MetadataDescriptor::IsDefaultHidden         , &MetadataDescriptor::HiddenAsString      , nullptr, &MetadataDescriptor::SetHiddenAsString       , nullptr, false, true , true ),
        MetadataFieldDescriptor("desc"       , ""        , MetadataType::Synopsis    ,MetadataFieldDescriptor::DataType::Text  , MetadataFieldDescriptor::EditableType::Text  , &MetadataDescriptor::IsDefaultDescription    , &MetadataDescriptor::DescriptionAsString , nullptr, &MetadataDescriptor::SetDescription          , nullptr, false, false, false),
        MetadataFieldDescriptor("image"      , ""        , MetadataType::Image       ,MetadataFieldDescriptor::DataType::Path  , MetadataFieldDescriptor::EditableType::Text  , &MetadataDescriptor::IsDefaultImage          , &MetadataDescriptor::ImageAsString       , nullptr, &MetadataDescriptor::SetImagePathAsString    , nullptr, false, false, false),
      };

      count = sizeof(sFolderMetadataDescriptors) / sizeof(MetadataFieldDescriptor);
      return &sFolderMetadataDescriptors[0];
    }
    case ItemType::Root:
    case ItemType::Empty:
    case ItemType::Header:
    default: break;
  }
  count = 0;
  return nullptr;
}

const MetadataDescriptor& MetadataDescriptor::Default()
{
  static bool initialized = false;
  static MetadataDescriptor defaultData(Path::Empty, "default", ItemType::Game);

  if (!initialized)
  {
    defaultData.DisableDirtiness();
    int count = 0;
    const MetadataFieldDescriptor* fields = GetMetadataFieldDescriptors(ItemType::Game, count);

    for (; --count >= 0;)
    {
      // Get field descriptor
      const MetadataFieldDescriptor& field = fields[count];

      // Set default value
      String value = field.DefaultValue();
      if (field.SetValueMethod() != nullptr) (defaultData.*field.SetValueMethod())(value);
      //else (defaultData.*field.SetXmlValueMethod())();
    }
    initialized = true;
    defaultData.EnableDirtiness();
  }

  return defaultData;
}

String MetadataDescriptor::IntToRange(unsigned short range)
{
  int max = (int)(range >> 8);
  int min = (int)(range & 0xFF);

  // min = max, only one number
  if (min == max) return String(max);

  String value = String(max);

  // min or more range
  if (min == 0xFF)
  {
    value += '+';
  }
  else
  {
    // Full range
    value = String(min).Append('-').Append(value);
  }
  return value;
}

bool MetadataDescriptor::RangeToInt(const String& range, [[in, out]] unsigned short& to)
{
  // max+ (min+)
  int p = range.Find('+');
  if (p >= 0)
  {
    if (!StringToInt(range, p, 0, '+')) return false;
    to = (p << 8) + 0xFF;
    return true;
  }

  // max-max
  p = range.Find('-');
  if (p < 0)
  {
    if (!StringToInt(range, p)) return false;
    to = (p << 8) + p;
    return true;
  }

  // min-max
  int min = 0; if (!StringToInt(range, min, 0, '-')) return false;
  int max = 0; if (!StringToInt(range, max, p + 1, 0  )) return false;
  if (min > max) { min = min ^ max; max = max ^ min; min = min ^ max; }
  to = (max << 8) + min;
  return true;
}

bool MetadataDescriptor::IntToHex(int from, String& to)
{
  static const char* hexa = "0123456789ABCDEF";
  char result[9];
  result[sizeof(result) - 1] = 0;

  for (int i = sizeof(result) - 1, p = 0; -- i >= 0;)
    result[p++] = hexa[(from >> (i << 2)) & 0xF];

  to = result;
  return true;
}

bool MetadataDescriptor::HexToInt(const String& from, int& to)
{
  if (from.empty()) return false;
  const char* src = from.c_str();

  int result = 0;
  for (;; src++)
  {
    int v = (unsigned char)src[0];
    if ((unsigned int)(v - 0x30) <= 9) { result <<= 4; result += v - 0x30; }
    else
    {
      v &= 0xDF;
      if ((unsigned int)(v - 0x41) <= 5) { result <<= 4; result += v - 0x37; }
      else break;
    }
  }
  if (src[0] != 0) return false;

  to = result;
  return true;
}

bool MetadataDescriptor::StringToInt(const String& from, int& to, int offset, char stop)
{
  const char* src = from.c_str() + offset;

  bool sign = (src[0] == '-');
  if (sign) src++;

  int result = 0;
  while ((unsigned int)(src[0] - 0x30) <= 9) { result *= 10; result += src[0] - 0x30; src++; }
  if (src[0] != stop) return false;

  to = sign ? -result : result;
  return true;
}

bool MetadataDescriptor::StringToInt(const String& from, int& to)
{
  const char* src = from.c_str();

  bool sign = (src[0] == '-');
  if (sign) src++;

  int result = 0;
  while ((unsigned int)(src[0] - 0x30) <= 9) { result *= 10; result += src[0] - 0x30; src++; }
  if (src[0] != 0) return false;

  to = sign ? -result : result;
  return true;
}

bool MetadataDescriptor::StringToFloat(const String& from, float& to)
{
  const char* src = from.c_str();

  bool sign = (src[0] == '-');
  if (sign) src++;

  int intPart = 0;
  int fractPart = 0;
  int pow10 = 1;

  // Integer part
  while ((unsigned int)(src[0] - 0x30) <= 9) { intPart *= 10; intPart += src[0] - 0x30; src++; }
  if (src[0] == '.')
  {
    src++;
    while ((unsigned int)(src[0] - 0x30) <= 9) { fractPart *= 10; fractPart += src[0] - 0x30; src++; pow10 *= 10; }
  }
  if (src[0] != 0) return false;

  float result = (float)intPart + ((float)fractPart / (float)pow10);

  to = sign ? -result : result;
  return true;
}

void MetadataDescriptor::BuildUserDataMap(const String& data, HashMap<String, String>& map)
{
  for(int start = 0; start < data.Count(); )
  {
    // Look for key separator
    int end = data.Find(',', start);
    if (end < 0) end = data.Count();

    // Look for key/value separator
    int equal = data.Find('=', start);
    if (equal <= start  || equal >= end) { LOG(LogError) << "[MetadataDescriptor] malformated user data " << data; }
    else
    {
      String key = data.SubString(start, equal - start);
      String value = data.SubString(equal + 1, end - (equal + 1));
      map[key] = value;
    }

    // Next step
    start = end + 1;
  }
}

std::vector<MetadataDescriptor::MapEntry> MetadataDescriptor::Maps() const
{
  std::vector<MapEntry> result;
  String combined = sMapHolder.GetString(mMapFile);
  if (combined.empty()) return result;
  Path dir = sPathHolder.GetPath(mMapPath);
  String::List parts = combined.Split('|');
  // Single untitled filename, as stored by SetMapPath()
  if (parts.size() == 1) { result.push_back({ String::Empty, dir / parts[0] }); return result; }
  for (int i = 0; i + 1 < (int)parts.size(); i += 2)
    result.push_back({ parts[i], dir / parts[i + 1] });
  return result;
}

void MetadataDescriptor::SetMaps(const std::vector<MapEntry>& maps)
{
  if (maps.empty()) return;
  String combined;
  for (size_t i = 0; i < maps.size(); ++i)
  {
    if (i != 0) combined.Append('|');
    // The pipe is the field separator: it cannot survive inside a title
    combined.Append(String(maps[i].mTitle).Replace('|', ' ')).Append('|').Append(maps[i].mPath.Filename());
  }
  StringHolderTypes::Index16 newPath = sPathHolder.AddString16(maps[0].mPath.Directory().ToString());
  StringHolderTypes::Index32 newFile = sMapHolder.AddString32(combined);
  if (newPath != mMapPath || newFile != mMapFile)
  {
    mMapPath = newPath;
    mMapFile = newFile;
    SetDirty();
  }
}

void MetadataDescriptor::SerializeMapsAsXml(XmlNode& parent, const Path& relativeTo) const
{
  std::vector<MapEntry> maps = Maps();
  if (maps.empty()) return;
  bool dummy = false;
  XmlNode mapsNode = parent.append_child("maps");
  for (const MapEntry& m : maps)
  {
    XmlNode mapNode = mapsNode.append_child("map");
    Xml::AddAttribute(mapNode, "title", m.mTitle);
    Xml::AddAttribute(mapNode, "path" , m.mPath.MakeRelative(relativeTo, dummy).ToString());
  }
}

void MetadataDescriptor::SetMapsAsXml(const XmlNode& parent, const Path& relativeTo)
{
  std::vector<MapEntry> maps;

  // Multi-map format: <maps><map title="..." path="..."/>...</maps>
  if (XmlNode mapsNode = parent.child("maps"); mapsNode != nullptr)
    for (XmlNode child : mapsNode.children("map"))
    {
      String title = Xml::AttributeAsString(child, "title", String::Empty);
      String path  = Xml::AttributeAsString(child, "path" , String::Empty);
      if (path.empty()) continue;
      maps.push_back({ title, Path(path).ToAbsolute(relativeTo) });
    }

  // Legacy single-map format: <map>path</map>
  if (maps.empty())
    if (String path = Xml::AsString(parent, "map", String::Empty); !path.empty())
      maps.push_back({ String::Empty, Path(path).ToAbsolute(relativeTo) });

  SetMaps(maps);
}

bool MetadataDescriptor::Deserialize(const XmlNode from, const String& userData, const Path& relativeTo)
{
  #ifdef _METADATA_STATS_
    if (_Type == ItemType::Game) LivingGames--;
    if (_Type == ItemType::Folder) LivingFolders--;
  #endif

  String name = from.name();
  if (name == GameNodeIdentifier) mType = ItemType::Game;
  else if (name == FolderNodeIdentifier) mType = ItemType::Folder;
  else return false; // Unidentified node

  mTimeStamp = (unsigned int)Xml::AttributeAsInt(from, "timestamp", 0);

  // Build userData map
  HashMap<String, String> userDataMap;
  bool hasUserData = !userData.empty();
  if (hasUserData) BuildUserDataMap(userData, userDataMap);

  #ifdef _METADATA_STATS_
    if (_Type == ItemType::Game) LivingGames++;
    if (_Type == ItemType::Folder) LivingFolders++;
  #endif

  int count = 0;
  const MetadataFieldDescriptor* fields = GetMetadataFieldDescriptors(mType, count);
  if (fields == nullptr) return false;

  for (; --count >= 0; )
  {
    // Get field descriptor
    const MetadataFieldDescriptor& field = fields[count];

    // XML setter takes priority over the string setter: the field reads its own subtree from the game node
    if (SetXmlValueMethodType xmlSetter = field.SetXmlValueMethod(); xmlSetter != nullptr)
    {
      DisableDirtiness();
      (this->*xmlSetter)(from, relativeTo);
      EnableDirtiness();
      continue;
    }

    // Get field data as string
    const String& defaultStringValue = field.DefaultValue();
    String value = Xml::AsString(from, field.Key(), defaultStringValue);
    // override with userdata if any
    if (hasUserData)
      if (String* pvalue = userDataMap.try_get(field.Key()); pvalue != nullptr)
        value = *pvalue;
    // Ignore default values
    if (value == defaultStringValue) continue;

    // Absolute path
    if (field.Type() == MetadataFieldDescriptor::DataType::Path)
      value = Path(value).ToAbsolute(relativeTo).ToString();

    // Set value - Disable dirtiness to not override any previous dirty state
    DisableDirtiness();
    (this->*field.SetValueMethod())(value);
    EnableDirtiness();
  }

  // Control name
  if (mName <= 0) SetName(sFileHolder.GetString(mRomFile));

  return true;
}

void MetadataDescriptor::Serialize(XmlNode parentNode, [[out]] String& userData, const Path& filePath, const Path& relativeTo) const
{
  (void)filePath;
  int count = 0;
  bool dummy = false;

  const MetadataFieldDescriptor* fields = GetMetadataFieldDescriptors(mType, count);
  if (fields == nullptr) return;

  // Add empty node game/folder
  XmlNode node = parentNode.append_child(mType == ItemType::Game ? GameNodeIdentifier.c_str() : FolderNodeIdentifier.c_str());
  Xml::AddAttribute(node, "source", "Recalbox");
  Xml::AddAttribute(node, "timestamp", mTimeStamp);

  // Metadata
  String value;
  for (; --count >= 0; )
  {
    // Get field descriptor
    const MetadataFieldDescriptor& field = fields[count];

    // Default value?
    if ((this->*field.IsDefaultValueMethod())()) continue;

    // XML serializer takes priority over the string getter: append the field's subtree directly under the game/folder node
    if (SerializeXmlMethodType xmlSerializer = field.SerializeXmlMethod(); xmlSerializer != nullptr)
    {
      (this->*xmlSerializer)(node, relativeTo);
      continue;
    }

    // Get value
    value = (this->*field.GetValueMethod())();

    // Relative path
    if (field.Type() == MetadataFieldDescriptor::DataType::Path)
      value = Path(value).MakeRelative(relativeTo, dummy).ToString();

    // Store
    if (field.IsUserdata())
    {
      if (!userData.empty()) userData.Append(',');
      userData.Append(field.Key()).Append('=').Append(value);
    }
    else Xml::AddAsString(node, field.Key(), value);
  }
}

void MetadataDescriptor::Merge(const MetadataDescriptor& sourceMetadata)
{
  int count = 0;
  const MetadataFieldDescriptor* fields = GetMetadataFieldDescriptors(mType, count);
  if (fields == nullptr) return;

  for (; --count >= 0; )
  {
    // Get field descriptor
    const MetadataFieldDescriptor& field = fields[count];

    // XML-only fields have no string setter and cannot go through the generic path
    if (field.SetValueMethod() == nullptr) continue;

    // Get/Set if current field is the default value
    if (!(this->*field.IsDefaultValueMethod())())
    {
      (this->*field.SetValueMethod())((sourceMetadata.*field.GetValueMethod())());
      // A field has been copied. Set the dirty flag
      mDirty = true;
    }
  }
}

void MetadataDescriptor::CleanupHolders()
{
  LOG(LogDebug) << "[MetadataDescriptor] Name storage: "        << sNameHolder.StorageSize()          << " - object count: " << sNameHolder.ObjectCount()       ;
  LOG(LogDebug) << "[MetadataDescriptor] Description storage: " << sDescriptionHolder.StorageSize()   << " - object count: " << sDescriptionHolder.ObjectCount();
  LOG(LogDebug) << "[MetadataDescriptor] Publisher storage: "   << sPublisherHolder.StorageSize()     << " - object count: " << sPublisherHolder.ObjectCount()  ;
  LOG(LogDebug) << "[MetadataDescriptor] Developer storage: "   << sDeveloperHolder.StorageSize()     << " - object count: " << sDeveloperHolder.ObjectCount()  ;
  LOG(LogDebug) << "[MetadataDescriptor] Genre storage: "       << sGenreHolder.StorageSize()         << " - object count: " << sGenreHolder.ObjectCount()      ;
  LOG(LogDebug) << "[MetadataDescriptor] Ratio storage: "       << sRatioHolder.StorageSize()         << " - object count: " << sRatioHolder.ObjectCount()      ;
  LOG(LogDebug) << "[MetadataDescriptor] Core storage: "        << sCoreHolder.StorageSize()          << " - object count: " << sCoreHolder.ObjectCount()       ;
  LOG(LogDebug) << "[MetadataDescriptor] Emulator storage: "    << sEmulatorHolder.StorageSize()      << " - object count: " << sEmulatorHolder.ObjectCount()   ;
  LOG(LogDebug) << "[MetadataDescriptor] Path storage: "        << sPathHolder.StorageSize()          << " - object count: " << sPathHolder.ObjectCount()       ;
  LOG(LogDebug) << "[MetadataDescriptor] File storage: "        << sFileHolder.StorageSize()          << " - object count: " << sFileHolder.ObjectCount()       ;
  LOG(LogDebug) << "[MetadataDescriptor] Total storage: "       << (sNameHolder.StorageSize() +
                                                                    sDescriptionHolder.StorageSize() +
                                                                    sPublisherHolder.StorageSize() +
                                                                    sDeveloperHolder.StorageSize() +
                                                                    sGenreHolder.StorageSize() +
                                                                    sRatioHolder.StorageSize() +
                                                                    sCoreHolder.StorageSize() +
                                                                    sEmulatorHolder.StorageSize() +
                                                                    sPathHolder.StorageSize() +
                                                                    sFileHolder.StorageSize());
  LOG(LogDebug) << "[MetadataDescriptor] Total objects: "       << (sNameHolder.ObjectCount() +
                                                                    sDescriptionHolder.ObjectCount() +
                                                                    sPublisherHolder.ObjectCount() +
                                                                    sDeveloperHolder.ObjectCount() +
                                                                    sGenreHolder.ObjectCount() +
                                                                    sRatioHolder.ObjectCount() +
                                                                    sCoreHolder.ObjectCount() +
                                                                    sEmulatorHolder.ObjectCount() +
                                                                    sPathHolder.ObjectCount() +
                                                                    sFileHolder.ObjectCount());
}

