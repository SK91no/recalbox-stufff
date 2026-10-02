//
// Created by bkg2k on 07/12/2019.
//
#pragma once

#include <vector>
#include <utils/String.h>
#include "utils/network/HttpClient.h"
#include <utils/datetime/DateTime.h>
#include <rapidjson/document.h>
#include <scraping/ScrapeResult.h>
#include <games/classifications/Genres.h>
#include <games/classifications/Regions.h>
#include <scraping/scrapers/screenscraper/IConfiguration.h>
#include <scraping/scrapers/screenscraper/IEndPointProvider.h>

class ScreenScraperApis
{
  public:
    //! Game object
    struct Game
    {
      //! Result
      ScrapeResult mResult = ScrapeResult::NotFound;
      //! ScreenScraper Game Name
      String mScreenScraperName;
      //! Game Name
      String mName;
      //! Aliases
      String mAliases;
      //! Game License
      String mLicences;
      //! Developer
      String mDeveloper;
      //! Publisher
      String mPublisher;
      //! Synopsis
      String mSynopsis;
      //! Players
      String mPlayers;
      //! Genre
      String mGenre;
      //! Rom regions
      Regions::RegionPack mRegions;
      //! Rom region
      String mCrc;
      //! Pad2Keyboard
      String mP2k;
      //! Tips & Tricks
      String mTipsAndTricks;
      //! Rotation
      String mRotation;
      //! Editor
      DateTime mReleaseDate;
      //! Rating
      float mRating;
      //! Normalized genre
      GameGenres mGenreId;
      //! Adult state
      bool mAdult;

      //! Media Urls
      struct MediaUrl
      {
        struct Media
        {
          //! source Url
          String mUrl;
          //! file format
          String mFormat;
          //! md5
          String mMd5;
          //! size
          long long mSize;

          [[nodiscard]] bool IsValid() const
          {
            return !mUrl.empty() &&
                   !mFormat.empty() &&
                   !mMd5.empty() &&
                   mSize != 0;
          }
        };

        //! Map media: Media + version field used as the map title
        struct MapMedia : public Media
        {
          //! Map version/title (JSON "version" field)
          String mVersion;
        };

        //! Main image
        Media mImage;
        //! Box image
        Media mBox;
        //! Video
        Media mVideo;
        //! Marquee
        Media mMarquee;
        //! Wheel (clear logo)
        Media mWheel;
        //! Game Manual
        Media mManual;
        //! Game Maps (may contain several entries, one per map)
        std::vector<MapMedia> mMaps;
      }
      MediaSources;
    };

  private:
    //! Credential interface
    IConfiguration& mConfiguration;
    //! Endpoint provider
    IEndPointProvider& mEndPointProvider;

    // Client initialized status
    bool mClientInitialized;

    //! HTTP client
    HttpClient mClient;

    /*!
     * @brief Deserialize a gameinfo request JSON result into a Game object
     * @param json Input json
     * @param game Output Game object
     * @param path Rom path
     * @param md5 file md5
     * @param size file size in byte
     */
    void DeserializeGameInformationInner(const rapidjson::Value& json, Game& game, const Path& path, const String& md5, long long size) const;

    /*!
     * @brief Deserialize a gameinfo request JSON result into a Game object
     * @param json Input json
     * @param game Output Game object
     * @param path Rom path
     * @param md5 file md5
     * @param size file size in byte
     */
    void DeserializeGameInformationOuter(const String& json, Game& game, const Path& path, const String& md5, long long size) const;

    /*!
     * @brief Lookup best matching rom entry in the 'roms' array, according to md5, then filename & size
     * @param json json node
     * @param filename rom filename
     * @param md5 rom md5
     * @param size rom size in byte
     * @return Found node or empty node
     */
    static const rapidjson::Value& LookupBestRomEntry(const rapidjson::Value& json, const String& filename, const String& md5, long long size);

    /*!
     * @brief Select best region regarding configuration & availability
     * @param regions Regions from screenscraper
     * @param path File name to extract region from
     * @param favoriteRegion User favorite regions
     * @return Serialized best region
     */
    static String GetRequiredRegion(const Regions::RegionPack& regions, const Path& path, Regions::GameRegions favoriteRegion);

    /*!
     * @brief Get all regions for a given game
     * @param romRegions Regions extracted from rom information
     * @param path File path
     * @return Packed regions
     */
    static Regions::RegionPack GetRegions(const Regions::RegionPack& romRegions, const Path& path);

    /*!
     * @brief Extract all tips & tricks in a pre-formated markdown text
     * @param array JSON array of tips
     * @param requiredLanguage Required language
     * @return Formatted text
     */
    static String ExtractTips(const rapidjson::Value& array, const String& requiredLanguage);

    /*!
     * @brief Extract the best-matching text from an array of regionalized texts, regarding the given preferred region
     * @param array JSON array of regionalized texts
     * @param requiredRegion Required region
     * @return Best-matching text
     */
    static String ExtractRegionalizedText(const rapidjson::Value& array, const String& requiredRegion);

    /*!
     * @brief Extract the best-matching text from an array of localized texts, regarding the given preferred language
     * @param array JSON array of localized texts
     * @param preferredLanguage Preferred language
     * @return Best-matching text
     */
    static String ExtractLocalizedText(const rapidjson::Value& array, const String& preferredLanguage);

    /*!
     * @brief Extract the best-matching media URL, given the type, parent type and preferred region.
     * @param medias Media name
     * @param type Type
     * @param region Preferred region
     * @param format If a url is found, this string contains the media format (jpg, png, mp4, ...)
     * @param size output media size
     * @param md5 output media md5
     * @return Best-matching url
     */
    static String ExtractMedia(const rapidjson::Value& medias, const char* type, const String& region, String& format, long long& size, String&  md5);

    /*!
     * @brief Extract all map media entries (type == "maps") without region filtering, each with its "version" field
     * @param medias JSON medias array
     * @param output Destination vector (cleared on entry)
     */
    static void ExtractMaps(const rapidjson::Value& medias, std::vector<Game::MediaUrl::MapMedia>& output);

    /*!
     * @brief Extract best matching genre texts, comma-separated.
     * @param array JSON array of genres
     * @param preferredLanguage Preferred language
     * @return Genre strings, comma-separated
     */
    static String ExtractLocalizedGenre(const rapidjson::Value& array, const String& preferredLanguage);

    /*!
     * @brief Extract a normalized game genre
     * @param array JSON array of genres
     * @return GameGenres value
     */
    static GameGenres ExtractNormalizedGenre(const rapidjson::Value& array);

    /*!
     * @brief Extract the adult state
     * @param array JSON array of genres
     * @return True if the game is flagged Adult, false otherwise
     */
    static bool ExtractAdultState(const rapidjson::Value& array);

    /*!
     * @brief Decode inner string encoding, including XML and X encoding like
     * &quot; \u0260 and \r\n
     * @param raw Raw string, decoded in place
     */
    static void DecodeString(String& raw);

    /*!
     * @brief Return a clean game name, removing all non file authorized chars
     * @param source Original game
     * @return Clean name
     */
    static String CleanGameName(const String& source);

    /*!
     * @brief Check if the http client is initialized & initialize if required
     */
    void InitializeClient()
    {
      if (!mClientInitialized)
      {
        if (mEndPointProvider.RequireBasicAuth())
          mClient.SetBasicAuth(mConfiguration.GetLogin(), mConfiguration.GetPassword());
        if (mEndPointProvider.RequireBearer())
          mClient.SetBearer(mConfiguration.GetBearer());
        mClientInitialized = true;
      }
    }

  public:
    /*!
     * @brief Constructor
     * @param credentials
     * @param  endPointProvider Endpoint provider interface
     */
    explicit ScreenScraperApis(IConfiguration* credentials, IEndPointProvider* endPointProvider)
      : mConfiguration(*credentials)
      , mEndPointProvider(*endPointProvider)
      , mClientInitialized(false)
    {
    }

    //! Get user
    ScreenScraperUser GetUserInformation();

    /*!
     * @brief Get Game information
     * @param file Game to work on
     * @param crc32 optional CRC32
     * @param md5 optional MD5
     * @param size Rom size
     * @return Game structure with the result code in the mResult field
     */
    Game GetGameInformation(const FileData& file, const String& crc32, const String& md5, long long size);

    /*!
     * @brief Download a media into a target file
     * @param game Game the media belongs to
     * @param mediaURL Media url
     * @param to Filepath
     * @param size output media size
     * @return ScrapeResult Request status
     */
    ScrapeResult GetMedia(const FileData& game, const String& mediaURL, const Path& to, long long& size);

    static String GetAllNames(const rapidjson::Value& array);
};

