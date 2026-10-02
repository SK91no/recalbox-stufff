#pragma once

#include <vector>
#include <utils/os/fs/Path.h>
#include <utils/Xml.h>
#include <utils/datetime/DateTime.h>
#include <games/classifications/Regions.h>
#include "ItemType.h"
#include "games/classifications/Genres.h"
#include "MetadataStringHolder.h"
#include "hardware/RotationType.h"
#include "SupportSides.h"
#include "SupportTypes.h"
#include "utils/String.h"


//#define _METADATA_STATS_

// Forward declaration (MetadataFieldDescriptor must include MetadataDescriptor)
class MetadataFieldDescriptor;

class MetadataDescriptor
{
  public:
    //! Single map entry (title + relative path). Used to round-trip the composite map field.
    struct MapEntry
    {
      String mTitle;
      Path   mPath;
    };

  private:
    #ifdef _METADATA_STATS_
    static int LivingClasses;
    static int LivingFolders;
    static int LivingGames;
    #endif

    //! Game node <game></game>
    static const String GameNodeIdentifier;
    //! Folder node <folder></folder>
    static const String FolderNodeIdentifier;

    //! Game name string holder
    static MetadataStringHolder<16> sNameHolder;
    //! Description string holder
    static MetadataStringHolder<16> sDescriptionHolder;
    //! Developer string holder
    static MetadataStringHolder<12> sDeveloperHolder;
    //! Publisher string holder
    static MetadataStringHolder<12> sPublisherHolder;
    //! Publisher string holder
    static MetadataStringHolder<12> sGenreHolder;
    //! Publisher string holder
    static MetadataStringHolder<8> sEmulatorHolder;
    //! Publisher string holder
    static MetadataStringHolder<8> sCoreHolder;
    //! Publisher string holder
    static MetadataStringHolder<8> sRatioHolder;
    //! Path string holder
    static MetadataStringHolder<12> sPathHolder;
    //! File string holder
    static MetadataStringHolder<16> sFileHolder;
    //! Map string holder (stores either a single map filename or a composite "title|file|...|title|file")
    static MetadataStringHolder<16> sMapHolder;

    // Please keep field ordered by type size to reduce alignment padding
    // 32 bits
    unsigned int                  mTimeStamp;      //!< Scraping timestamp
    Regions::RegionPack           mRegion;         //!< Rom/Game Region
    unsigned int                  mReleaseDate;    //!< Release data (epoch)
    unsigned int                  mLastPlayed;     //!< Last time played (epoch)
    unsigned int                  mTotalPlayTime;  //!< Game total play time
    int                           mRomCrc32;       //!< Rom Crc32
    // 24 bits
    StringHolderTypes::Index32    mRomFile:24;     //!< Rom file
    StringHolderTypes::Index32    mName:24;        //!< Name as simple string
    StringHolderTypes::Index32    mAliases:24;     //!< Aliasess
    StringHolderTypes::Index32    mLicences:24;    //!< Licences,
    StringHolderTypes::Index32    mDescription:24; //!< Description, multiline text
    StringHolderTypes::Index32    mImageFile:24;   //!< Image file
    StringHolderTypes::Index32    mBoxFile:24;     //!< Box file
    StringHolderTypes::Index32    mLogoFile:24;    //!< Logo file
    StringHolderTypes::Index32    mVideoFile:24;   //!< Video file
    StringHolderTypes::Index32    mTipsFile:24;    //!< Tips&Trics file
    StringHolderTypes::Index32    mGenre:24;       //!< Genres, comma separated
    StringHolderTypes::Index32    mDeveloper:24;   //!< Developer name index
    StringHolderTypes::Index32    mPublisher:24;   //!< Publisher name index
    StringHolderTypes::Index32    mManualFile:24;  //!< Manual file index
    StringHolderTypes::Index32    mMapFile:24;     //!< Map file index
    // 16 bits
    StringHolderTypes::Index16    mRomPath;        //!< Rom path
    StringHolderTypes::Index16    mImagePath;      //!< Image path
    StringHolderTypes::Index16    mBoxPath;        //!< 2D/3D box path
    StringHolderTypes::Index16    mVideoPath;      //!< Video path
    StringHolderTypes::Index16    mTipsPath;       //!< Tips&Tricks path
    StringHolderTypes::Index16    mManualPath;     //!< Manual path
    StringHolderTypes::Index16    mMapPath;        //!< Map path
    StringHolderTypes::Index16    mLogoPath;       //!< Logo path
    StringHolderTypes::Index16    mEmulator;       //!< Specific emulator
    StringHolderTypes::Index16    mCore;           //!< Specific core
    StringHolderTypes::Index16    mLastPatchPath;  //!< Last patch path
    StringHolderTypes::Index16    mLastPatchFile;  //!< Last patch file
    unsigned short                mPlayers;        //!< Players range: LSB:from - MSB:to (allow sorting by max players)
    unsigned short                mPlayCount;      //!< Play counter
    GameGenres                    mGenreId;        //!< Normalized Genre
    // 8 bits
    StringHolderTypes::Index8     mRatio;          //!< Specific screen ratio
    char                          mRating;         //!< Rating from 0 to 100 (100%)
    // bit groups
    int                           mSupportIndex:5; //!< Support number (disk, tape, ...)  - Not persisted
    ItemType                      mType:3;         //!< Metadata type
    int                           mSupportTotal:5; //!< Total Supports  - Not persisted
    bool                          mFavorite:1;     //!< Favorite game
    bool                          mHidden:1;       //!< Hidden game
    bool                          mAdult:1;        //!< Adult state
    SupportSides                  mSupportSide:2;  //!< Support faces - Not persisted
    RotationType                  mRotation:2;     //!< Rotation flag
    SupportTypes                  mSupportType:4;  //!< Support type - Not persisted
    bool                          mLatestVerion:1; //!< Latest version of this game?
    bool                          mPreinstalled:1; //!< Preinstalled game?
    bool                          mNoGame:1;       //!< Nor a game (application, tools, ...)
    bool                          mHasFavoriteChild:1;
    bool                          mPathValidated:1;//!< File path has been validated during loading

    bool                          mDirty:1;        //!< Dirty flag (modified data flag)
    bool                          mDirtyEnabled:1; //!< Allow disabling dirtiness temporarily

    //! Default value storage for fast default detection
    static const MetadataDescriptor& Default();

    /*!
     * Return the first static internal field descriptor reference
     * @param type ObjectType from which to retrieve
     * @param count Number of actual field descriptor available
     * @return first static internal field descriptor reference
     */
    static const MetadataFieldDescriptor* GetMetadataFieldDescriptors(ItemType type, int& count);

    /*!
     * Convert integer range to string: low-high
     * @param range Integer range: Highest into MSB, Lowest into LSB
     * @return Converted string
     */
    static String IntToRange(unsigned short range);
    /*!
     * Convert a range X-Y to an int: Highest into MSB, Lowest into LSB (allow sorting by highest value)
     * @param range Range string
     * @param to destination int
     * @return True if the operation is successful. False otherwise.
     */
    static bool RangeToInt(const String& range, unsigned short& to);
    /*!
     * Convert int32 to Hexadecimal string
     * @param from Int32 value to convert to string
     * @param to Hexadecimal result string
     * @return True if the operation is successful. False otherwise.
     */
    static bool IntToHex(int from, String& to);
    /*!
     * Convert Hexa string into int32
     * @param from Hexadecimal string
     * @param to Target int32
     * @return True if the operation is successful. False otherwise.
     */
    static bool HexToInt(const String& from, int& to);
    /*!
     * Fast string to int conversion
     * @param from source string
     * @param to destination int
     * @param offset offset in source string
     * @param stop Stop char
     * @return True if the operation is successful. False otherwise.
     */
    static bool StringToInt(const String& from, int& to, int offset, char stop);
    /*!
     * Fast string to int conversion
     * @param from source string
     * @param to destination int
     * @return True if the operation is successful. False otherwise.
     */
    static bool StringToInt(const String& from, int& to);
    /*!
     * Fast string to float conversion
     * @param from source string
     * @param to destination float
     * @return True if the operation is successful. False otherwise.
     */
    static bool StringToFloat(const String& from, float& to);

    /*!
     * @brief Split key=value from the userData string and fill in the given map
     * @param data userData to split
     * @param map Map to fill
     */
    static void BuildUserDataMap(const String& data, HashMap<String, String>& map);

  public:
    /*!
     * Destructor
     */
    ~MetadataDescriptor() = default;

    /*!
     * @brief Force default metadata initialization
     */
    static void InitializeDefaultMetadata() { Default(); }

    /*!
     * @brief Cleanup temporary data
     */
    static void CleanupHolders();

    /*!
     * Default constructor
     */
    explicit MetadataDescriptor(const Path& path, const String& defaultName, ItemType type)
      : mTimeStamp(0)
      , mReleaseDate(0)
      , mLastPlayed(0)
      , mTotalPlayTime(0)
      , mRomCrc32(0)
      , mRomFile(0)
      , mName(0)
      , mAliases(0)
      , mLicences(0)
      , mDescription(0)
      , mImageFile(0)
      , mBoxFile(0)
      , mLogoFile(0)
      , mVideoFile(0)
      , mTipsFile(0)
      , mGenre(0)
      , mDeveloper(0)
      , mPublisher(0)
      , mManualFile(0)
      , mMapFile(0)
      , mRomPath(0)
      , mImagePath(0)
      , mBoxPath(0)
      , mVideoPath(0)
      , mTipsPath(0)
      , mManualPath(0)
      , mMapPath(0)
      , mLogoPath(0)
      , mEmulator(0)
      , mCore()
      , mLastPatchPath(0)
      , mLastPatchFile(0)
      , mPlayers((1 << 8) + 1)
      , mPlayCount(0)
      , mGenreId(GameGenres::None)
      , mRatio(0)
      , mRating(0)
      , mSupportIndex(0)
      , mType(type)
      , mSupportTotal(0)
      , mFavorite(false)
      , mHidden(false)
      , mAdult(false)
      , mSupportSide(SupportSides::None)
      , mRotation(RotationType::None)
      , mSupportType(SupportTypes::Unknown)
      , mLatestVerion(false)
      , mPreinstalled(false)
      , mNoGame(false)
      , mHasFavoriteChild(false)
      , mPathValidated(false)
      , mDirty(false)
      , mDirtyEnabled(false)
    {
      SetRomPath(path);
      SetName(defaultName);
      EnableDirtiness();
      #ifdef _METADATA_STATS_
      LivingClasses++;
      if (_Type == ItemType::Game) LivingGames++;
      if (_Type == ItemType::Folder) LivingFolders++;
      #endif
    }

    /*!
     * Move constructor
     * @param source  Source to move data from
     */
    MetadataDescriptor(const MetadataDescriptor& source) noexcept
    #ifdef _METADATA_STATS_
      : mName(source.mName),
        mAlias(source.mAlias),
        mSSAlias(source.mSSAlias),
        mDescription(source.mDescription),
        mImageFile(source.mImageFile),
        mThumbnailFile(source.mThumbnailFile),
        mVideoFile(source.mVideoFile),
        mTipsFile(source.mTipsFile),
        mRating(source.mRating),
        mReleaseDate(source.mReleaseDate),
        mLastPlayed(source.mLastPlayed),
        mGenre(source.mGenre),
        mDeveloper(source.mDeveloper),
        mPublisher(source.mPublisher),
        mRegion(source.mRegion),
        mPlayers(source.mPlayers),
        mRomCrc32(source.mRomCrc32),
        mImagePath(source.mImagePath),
        mThumbnailPath(source.mThumbnailPath),
        mVideoPath(source.mVideoPath),
        mTipsPath(source.mTipsPath),
        mEmulator(source.mEmulator),
        mCore(source.mCore),
        mPlaycount(source.mPlaycount),
        mGenreId(source.mGenreId),
        mRatio(source.mRatio),
        mTimeStamp(source.mTimeStamp),
        mFavorite(source.mFavorite),
        mHidden(source.mHidden),
        mAdult(source.mAdult),
        mLatestVerion(source.mLatestVerion),
        mNoGame(source.mNoGame),
        mDirty(source.mDirty),
        mType(source.mType),
        mRotation(source.mRotation),
        mTimePlayed(source.mTimePlayed),
        mHasFavoriteChild(false)

    {
      LivingClasses++;
      if (_Type == ItemType::Game) LivingGames++;
      if (_Type == ItemType::Folder) LivingFolders++;
    }
    #else
      = default;
    #endif

    /*!
     * Assignment operator - Required by STL objects since a copy operator is defined
     * @param source Source to copy data from
     */
    MetadataDescriptor& operator = (const MetadataDescriptor& source)
    {
      if (&source == this) return *this;

      #ifdef _METADATA_STATS_
      LivingClasses--;
      if (_Type == ItemType::Game) LivingGames--;
      if (_Type == ItemType::Folder) LivingFolders--;
      #endif

      mRomFile       = source.mRomFile      ;
      mRomPath       = source.mRomPath      ;
      mName          = source.mName         ;
      mAliases       = source.mAliases      ;
      mLicences      = source.mLicences     ;
      mDescription   = source.mDescription  ;
      mImagePath     = source.mImagePath    ;
      mImageFile     = source.mImageFile    ;
      mBoxPath       = source.mBoxPath      ;
      mBoxFile       = source.mBoxFile      ;
      mVideoPath     = source.mVideoPath    ;
      mVideoFile     = source.mVideoFile    ;
      mTipsPath      = source.mTipsPath     ;
      mTipsFile      = source.mTipsFile     ;
      mGenre         = source.mGenre        ;
      mEmulator      = source.mEmulator     ;
      mCore          = source.mCore         ;
      mRatio         = source.mRatio        ;
      mDeveloper     = source.mDeveloper    ;
      mPublisher     = source.mPublisher    ;
      mRegion        = source.mRegion       ;
      mRating        = source.mRating       ;
      mGenreId       = source.mGenreId      ;
      mPlayers       = source.mPlayers      ;
      mReleaseDate   = source.mReleaseDate  ;
      mPlayCount     = source.mPlayCount    ;
      mLastPlayed    = source.mLastPlayed   ;
      mRomCrc32      = source.mRomCrc32     ;
      mFavorite      = source.mFavorite     ;
      mHidden        = source.mHidden       ;
      mAdult         = source.mAdult        ;
      mDirty         = source.mDirty        ;
      mLatestVerion  = source.mLatestVerion ;
      mPreinstalled  = source.mPreinstalled ;
      mNoGame        = source.mNoGame       ;
      mType          = source.mType         ;
      mRotation      = source.mRotation     ;
      mTotalPlayTime = source.mTotalPlayTime;
      mSupportIndex  = source.mSupportIndex ;
      mSupportTotal  = source.mSupportTotal ;
      mSupportSide   = source.mSupportSide  ;
      mPathValidated = source.mPathValidated;
      mManualFile    = source.mManualFile;
      mMapFile       = source.mMapFile;
      mLogoFile      = source.mLogoFile;
      mManualPath    = source.mManualPath;
      mMapPath       = source.mMapPath;
      mLogoPath      = source.mLogoPath;

      #ifdef _METADATA_STATS_
      if (_Type == ItemType::Game) LivingGames++;
      if (_Type == ItemType::Folder) LivingFolders++;
      #endif

      return *this;
    }

    /*!
     * Move operator
     * @param source Source to move data from
     */
    MetadataDescriptor& operator = (MetadataDescriptor&& source) noexcept
    {
      #ifdef _METADATA_STATS_
      LivingClasses--;
      if (_Type == ItemType::Game) LivingGames--;
      if (_Type == ItemType::Folder) LivingFolders--;
      #endif

      mRomFile       = source.mRomFile      ;
      mRomPath       = source.mRomPath      ;
      mName          = source.mName         ;
      mAliases       = source.mAliases      ;
      mLicences      = source.mLicences     ;
      mDescription   = source.mDescription  ;
      mImagePath     = source.mImagePath    ;
      mImageFile     = source.mImageFile    ;
      mBoxPath       = source.mBoxPath      ;
      mBoxFile       = source.mBoxFile      ;
      mVideoPath     = source.mVideoPath    ;
      mVideoFile     = source.mVideoFile    ;
      mTipsPath      = source.mTipsPath     ;
      mTipsFile      = source.mTipsFile     ;
      mEmulator      = source.mEmulator     ;
      mCore          = source.mCore         ;
      mLastPatchPath = source.mLastPatchPath;
      mLastPatchFile = source.mLastPatchFile;
      mRatio         = source.mRatio        ;
      mGenre         = source.mGenre        ;
      mDeveloper     = source.mDeveloper    ;
      mPublisher     = source.mPublisher    ;
      mRating        = source.mRating       ;
      mGenreId       = source.mGenreId      ;
      mPlayers       = source.mPlayers      ;
      mReleaseDate   = source.mReleaseDate  ;
      mPlayCount     = source.mPlayCount    ;
      mLastPlayed    = source.mLastPlayed   ;
      mRomCrc32      = source.mRomCrc32     ;
      mFavorite      = source.mFavorite     ;
      mHidden        = source.mHidden       ;
      mAdult         = source.mAdult        ;
      mDirty         = source.mDirty        ;
      mLatestVerion  = source.mLatestVerion ;
      mPreinstalled  = source.mPreinstalled ;
      mNoGame        = source.mNoGame       ;
      mType          = source.mType         ;
      mRotation      = source.mRotation     ;
      mTotalPlayTime = source.mTotalPlayTime;
      mSupportIndex  = source.mSupportIndex ;
      mSupportTotal  = source.mSupportTotal ;
      mSupportSide   = source.mSupportSide  ;
      mPathValidated = source.mPathValidated;
      mManualFile    = source.mManualFile;
      mMapFile       = source.mMapFile;
      mLogoFile      = source.mLogoFile;
      mManualPath    = source.mManualPath;
      mMapPath       = source.mMapPath;
      mLogoPath      = source.mLogoPath;

      #ifdef _METADATA_STATS_
      if (_Type == ItemType::Game) LivingGames++;
      if (_Type == ItemType::Folder) LivingFolders++;
      #endif

      return *this;
    }

    /*!
     * Deserialize data from a given Xml node
     * @param from XML Node to deserialize from
     * @param userData User data string of form: key1=value1,kay2=value2,...
     * @param relativeTo Root path
     * @return True of the node has been successfully deserialized
     */
    bool Deserialize(XmlNode from, const String& userData, const Path& relativeTo);

    /*!
     * Serialize internal data to XML node
     * @param relativeTo Root path
     * @param userData serialized user data string of form: key1=value1,kay2=value2,...
     * @return Serialized XML node
     */
    void Serialize(XmlNode parentTree, [[out]] String& userData, const Path& filePath, const Path& relativeTo) const;

    /*!
     * Merge value from the source metadata object into the current object
     * current fields ate replaced only if they have their default value.
     * @param source Metadata object from which to merge data
     */
    void Merge(const MetadataDescriptor& source);

    /*
     * Last scraping Timestamp
     */

    [[nodiscard]] unsigned int TimeStamp() const { return mTimeStamp; }
    void SetTimeStamp() { mTimeStamp = (unsigned int)DateTime().ToEpochTime(); }

    /*
     * Accessors
     */

    [[nodiscard]] ItemType Type() const { return mType; }

    [[nodiscard]] Path         Rom()           const { return sPathHolder.GetPath(mRomPath) / sFileHolder.GetString(mRomFile); }
    [[nodiscard]] Path         RomFolderOnly() const { return sPathHolder.GetPath(mRomPath); }
    [[nodiscard]] Path         RomFileOnly()   const { return sFileHolder.GetPath(mRomFile);        }
    [[nodiscard]] String       Name()          const { return sNameHolder.GetString(mName);                 }
    [[nodiscard]] String       Aliases()       const { return sNameHolder.GetString(mAliases);              }
    [[nodiscard]] String       Licences()      const { return sNameHolder.GetString(mLicences);             }
    [[nodiscard]] String       Description()   const { return sDescriptionHolder.GetString(mDescription);   }
    [[nodiscard]] Path         Image()         const { return sPathHolder.GetPath(mImagePath) / sFileHolder.GetString(mImageFile); }
    [[nodiscard]] Path         Box()           const { return sPathHolder.GetPath(mBoxPath) / sFileHolder.GetString(mBoxFile); }
    [[nodiscard]] Path         Manual()        const { return sPathHolder.GetPath(mManualPath) / sFileHolder.GetString(mManualFile); }
    [[nodiscard]] Path         Map()           const { std::vector<MapEntry> maps = Maps(); return maps.empty() ? Path::Empty : maps[0].mPath; }
    [[nodiscard]] std::vector<MapEntry> Maps() const;
    [[nodiscard]] Path         Logo()          const { return sPathHolder.GetPath(mLogoPath) / sFileHolder.GetString(mLogoFile); }
    [[nodiscard]] Path         Video()         const { return sPathHolder.GetPath(mVideoPath) / sFileHolder.GetString(mVideoFile); }
    [[nodiscard]] Path         TipsAndTricks() const { return sPathHolder.GetPath(mTipsPath) / sFileHolder.GetString(mTipsFile); }
    [[nodiscard]] String       Developer()     const { return sDeveloperHolder.GetString(mDeveloper);       }
    [[nodiscard]] String       Publisher()     const { return sPublisherHolder.GetString(mPublisher);       }
    [[nodiscard]] String       Genre()         const { return sGenreHolder.GetString(mGenre);               }
    [[nodiscard]] String       Emulator()      const { return sEmulatorHolder.GetString(mEmulator);         }
    [[nodiscard]] String       Core()          const { return sCoreHolder.GetString(mCore);                 }
    [[nodiscard]] String       Ratio()         const { return sRatioHolder.GetString(mRatio);               }
    [[nodiscard]] Path         LastPatch()     const {return sPathHolder.GetPath(mLastPatchPath) / sFileHolder.GetString(mLastPatchFile); }

    [[nodiscard]] float              Rating()          const { return (float)mRating / 100.f;                           }
    [[nodiscard]] unsigned short     PlayerRange()     const { return mPlayers;                          }
    [[nodiscard]] int                PlayerMax()       const { return (int)(mPlayers >> 8);              }
    [[nodiscard]] int                PlayerMin()       const { return (int)(mPlayers & 0xFF);            }
    [[nodiscard]] unsigned int       ReleaseDateEpoc() const { return mReleaseDate;                      }
    [[nodiscard]] DateTime           ReleaseDate()     const { return DateTime((long long)mReleaseDate); }
    [[nodiscard]] int                PlayCount()       const { return mPlayCount;                        }
    [[nodiscard]] unsigned int       LastPlayedEpoc()  const { return mLastPlayed;                       }
    [[nodiscard]] DateTime           LastPlayed()      const { return DateTime((long long)mLastPlayed);  }
    [[nodiscard]] Regions::RegionPack Region()         const { return mRegion;                           }
    [[nodiscard]] int                RomCrc32()        const { return mRomCrc32;                         }
    [[nodiscard]] bool               Favorite()        const { return mFavorite;                         }
    [[nodiscard]] bool               AnyFavorite()     const { return mFavorite || mHasFavoriteChild;    }
    [[nodiscard]] bool               Hidden()          const { return mHidden;                           }
    [[nodiscard]] bool               Adult()           const { return mAdult;                            }
    [[nodiscard]] bool               LatestVersion()   const { return mLatestVerion;                     }
    [[nodiscard]] bool               Preinstalled()    const { return mPreinstalled;                     }
    [[nodiscard]] bool               NoGame()          const { return mNoGame;                           }
    [[nodiscard]] GameGenres         GenreId()         const { return mGenreId;                          }
    [[nodiscard]] RotationType       Rotation()        const { return mRotation;                         }
    [[nodiscard]] int                TotalPlayTime()   const { return (int)mTotalPlayTime;               }
    [[nodiscard]] int                SupportIndex()    const { return mSupportIndex;                     }
    [[nodiscard]] int                SupportTotal()    const { return mSupportTotal;                     }
    [[nodiscard]] SupportSides       SupportSide()     const { return mSupportSide;                      }
    [[nodiscard]] bool               HasFavoriteChild()const { return mHasFavoriteChild;                 }

    [[nodiscard]] SupportTypes       SupportType()     const { return mSupportType;                      }

    /*
     * Validators
     */

    [[nodiscard]] bool HasImage()     const { return (mImagePath     | mImageFile    ) > 0; }
    [[nodiscard]] bool HasBox()       const { return (mBoxPath       | mBoxFile      ) > 0; }
    [[nodiscard]] bool HasVideo()     const { return (mVideoPath     | mVideoFile    ) > 0; }
    [[nodiscard]] bool HasLogo()      const { return (mLogoPath      | mLogoFile     ) > 0; }
    [[nodiscard]] bool HasMap()       const { return (mMapPath       | mMapFile      ) > 0; }
    [[nodiscard]] bool HasTipsAndTricks() const { return (mTipsPath      | mTipsFile     ) > 0; }
    [[nodiscard]] bool HasManual()        const { return (mManualPath    | mManualFile   ) > 0; }
    [[nodiscard]] bool HasLicences()      const { return mLicences > 0; }

    /*
     * String accessors
     */

    [[nodiscard]] String RomAsString()           const { return (sPathHolder.GetPath(mRomPath) / sFileHolder.GetString(mRomFile)).ToString(); }
    [[nodiscard]] String NameAsString()          const { return sNameHolder.GetString(mName);                 }
    [[nodiscard]] String AliasesAsString()       const { return sNameHolder.GetString(mAliases);              }
    [[nodiscard]] String LicencesAsString()      const { return sNameHolder.GetString(mLicences);             }
    [[nodiscard]] String EmulatorAsString()      const { return sEmulatorHolder.GetString(mEmulator);         }
    [[nodiscard]] String CoreAsString()          const { return sCoreHolder.GetString(mCore);                 }
    [[nodiscard]] String RatioAsString()         const { return sRatioHolder.GetString(mRatio);               }
    [[nodiscard]] String DescriptionAsString()   const { return sDescriptionHolder.GetString(mDescription);   }
    [[nodiscard]] String ImageAsString()         const { return (sPathHolder.GetPath(mImagePath) / sFileHolder.GetString(mImageFile)).ToString(); }
    [[nodiscard]] String BoxAsString()           const { return (sPathHolder.GetPath(mBoxPath) / sFileHolder.GetString(mBoxFile)).ToString(); }
    [[nodiscard]] String ManualAsString()        const { return (sPathHolder.GetPath(mManualPath) / sFileHolder.GetString(mManualFile)).ToString(); }
    [[nodiscard]] String MapAsString()           const { return Map().ToString(); }
    [[nodiscard]] String LogoAsString()          const { return (sPathHolder.GetPath(mLogoPath) / sFileHolder.GetString(mLogoFile)).ToString(); }
    [[nodiscard]] String VideoAsString()         const { return (sPathHolder.GetPath(mVideoPath) / sFileHolder.GetString(mVideoFile)).ToString(); }
    [[nodiscard]] String TipsAndTricksAsString() const { return (sPathHolder.GetPath(mTipsPath) / sFileHolder.GetString(mTipsFile)).ToString(); }
    [[nodiscard]] String DeveloperAsString()     const { return sDeveloperHolder.GetString(mDeveloper);       }
    [[nodiscard]] String PublisherAsString()     const { return sPublisherHolder.GetString(mPublisher);       }
    [[nodiscard]] String GenreAsString()         const { return sGenreHolder.GetString(mGenre);               }
    [[nodiscard]] String RegionAsString()        const { return Regions::Serialize4Regions(mRegion);          }

    [[nodiscard]] String RatingAsString()        const { return String((float)mRating / 100.f, 2);                        }
    [[nodiscard]] String PlayersAsString()       const { return IntToRange(mPlayers);                                 }
    [[nodiscard]] String ReleaseDateAsString()   const { return mReleaseDate != 0 ? DateTime((long long)mReleaseDate).ToCompactISO8601() : ""; }
    [[nodiscard]] String PlayCountAsString()     const { return String(mPlayCount);                           }
    [[nodiscard]] String LastPlayedAsString()    const { return mLastPlayed != 0 ? DateTime((long long)mLastPlayed).ToCompactISO8601() : ""; }
    [[nodiscard]] String FavoriteAsString()      const { return mFavorite ? "true" : "false";                         }
    [[nodiscard]] String RomCrc32AsString()      const { String r; IntToHex(mRomCrc32, r); return r;             }
    [[nodiscard]] String HiddenAsString()        const { return mHidden ? "true" : "false";                           }
    [[nodiscard]] String AdultAsString()         const { return mAdult ? "true" : "false";                            }
    [[nodiscard]] String GenreIdAsString()       const { return String((int)mGenreId);                           }
    [[nodiscard]] String LastPatchAsString()     const { return (sPathHolder.GetPath(mLastPatchPath) / sFileHolder.GetString(mLastPatchFile)).ToString(); }
    [[nodiscard]] String RotationAsString()      const { return RotationUtils::StringValue(mRotation); }
    [[nodiscard]] String TotalPlayTimeAsString() const { return String(mTotalPlayTime);                        }
    [[nodiscard]] String SupportTypeAsString()   const { return SupportTypeToString(mSupportType); }
    [[nodiscard]] String SupportSideAsString()   const { return SupportSideToString(mSupportSide); }
    [[nodiscard]] String SupportIndexAsString()  const { return String(mSupportIndex);                     }
    [[nodiscard]] String SupportTotalAsString()  const { return String(mSupportTotal);                     }

    [[nodiscard]] String MainAliasOrName() const {
      String aliases = Aliases();
      if (!aliases.empty()) {
        int pos = aliases.Find('|');
        String alias = aliases.SubString(0, pos);
        return alias.SubString(0, (int)alias.size()-4);
      }
      return Name();
    }

    /*
     * Setters
     */

    #define CheckValueAndSet(var, value) do{ if (auto v = value; (var) != v) { (var) = v; SetDirty(); } }while(false)

    #define CheckPathAndSet(varp, varf, path) \
    do{ \
      StringHolderTypes::Index16 p = (varp); \
      StringHolderTypes::Index32 f = (varf); \
      (varp) = sPathHolder.AddString16((path).Directory().ToString()); \
      (varf) = sFileHolder.AddString32((path).Filename()); \
      if ((varp) != p || mRomFile != f) SetDirty();                   \
    }while(false)

    void SetRomPath(const Path& rom)                    { CheckPathAndSet(mRomPath, mRomFile, rom); }
    void SetImagePath(const Path& image)                { CheckPathAndSet(mImagePath, mImageFile, image); }
    void SetBoxPath(const Path& box)                    { CheckPathAndSet(mBoxPath, mBoxFile, box); }
    void SetManualPath(const Path& manual)              { CheckPathAndSet(mManualPath, mManualFile, manual); }
    void SetMapPath(const Path& map)                    { StringHolderTypes::Index16 p = mMapPath; StringHolderTypes::Index32 f = mMapFile; mMapPath = sPathHolder.AddString16(map.Directory().ToString()); mMapFile = sMapHolder.AddString32(map.Filename()); if (mMapPath != p || mMapFile != f) SetDirty(); }
    void SetMaps(const std::vector<MapEntry>& maps);
    void SetLogoPath(const Path& logo)                  { CheckPathAndSet(mLogoPath, mLogoFile, logo); }
    void SetVideoPath(const Path& video)                { CheckPathAndSet(mVideoPath, mVideoFile, video); }
    void SetTipsAndTricksPath(const Path& tips)         { CheckPathAndSet(mTipsPath, mTipsFile, tips); }
    void SetLastPatch(const Path& patch)                { CheckPathAndSet(mLastPatchPath, mLastPatchFile, patch); }
    void SetEmulator(const String& emulator)            { CheckValueAndSet(mEmulator, sEmulatorHolder.AddString16(emulator)); }
    void SetCore(const String& core)                    { CheckValueAndSet(mCore, sCoreHolder.AddString16(core)); }
    void SetRatio(const String& ratio)                  { CheckValueAndSet(mRatio, ratio=="auto" ? sRatioHolder.AddString8(String::Empty) : mRatio = sRatioHolder.AddString8(ratio)); }
    void SetGenre(const String& genre)                  { CheckValueAndSet(mGenre, sGenreHolder.AddString32(genre)); }
    void SetName(const String& name)                    { CheckValueAndSet(mName, sNameHolder.AddString32(name)); }
    void SetAliases(const String& aliases)              { CheckValueAndSet(mAliases, sNameHolder.AddString32(aliases)); if (int pos = aliases.Find('|'); pos >= 0) CheckValueAndSet(mAliases, sNameHolder.AddString32(aliases.SubString(0, pos))); }
    void SetLicences(const String& licences)            { CheckValueAndSet(mLicences, sNameHolder.AddString32(licences)); }
    void SetDescription(const String& description)      { CheckValueAndSet(mDescription, sDescriptionHolder.AddString32(description)); }
    void SetReleaseDate(const DateTime& releaseDate)    { CheckValueAndSet(mReleaseDate, (unsigned int)releaseDate.ToEpochTime()); }
    void SetDeveloper(const String& developer)          { CheckValueAndSet(mDeveloper, sDeveloperHolder.AddString32(developer)); }
    void SetPublisher(const String& publisher)          { CheckValueAndSet(mPublisher, sPublisherHolder.AddString32(publisher)); }
    void SetRating(float rating)                        { if (rating < 0.f) rating = 0.f; else if (rating > 1.f) rating = 1.f; CheckValueAndSet(mRating, (char)(rating * 100.f + 0.5f)); }
    void SetPlayers(int min, int max)                   { CheckValueAndSet(mPlayers, (max << 8) + min); }
    void SetPlayersRaw(unsigned short maxmin)           { CheckValueAndSet(mPlayers, maxmin); }
    void SetRegion(const Regions::RegionPack& regions)  { CheckValueAndSet(mRegion, ({ Regions::RegionPack r = regions; r.Sort(); })); }
    void SetRomCrc32(int romcrc32)                      { CheckValueAndSet(mRomCrc32, romcrc32); }
    void SetFavorite(bool favorite)                     { CheckValueAndSet(mFavorite, favorite); }
    void SetHidden(bool hidden)                         { CheckValueAndSet(mHidden, hidden); }
    void SetAdult(bool adult)                           { CheckValueAndSet(mAdult, adult); }
    void SetGenreId(GameGenres genre)                   { CheckValueAndSet(mGenreId, genre); }
    void SetRotation(RotationType rotation)             { CheckValueAndSet(mRotation, rotation); }
    void SetTotalPlayTime(unsigned short timePlayed)    { CheckValueAndSet(mTotalPlayTime, timePlayed); }
    void SetPlayCount(int playCount)                    { CheckValueAndSet(mPlayCount, (short)playCount); }
    void SetLastPlayedDate(const DateTime& lastPlayed)  { CheckValueAndSet(mLastPlayed, (unsigned int)lastPlayed.ToEpochTime()); }
    // Volatiles flags - no dirtiness
    void SetPreinstalled(bool preinstalled)             { mPreinstalled = preinstalled;                                               }
    void SetLatestVersion(bool latestVersion)           { mLatestVerion = latestVersion;                                              }
    void SetNoGame(bool noGame)                         { mNoGame       = noGame;                                                     }
    void SetSupportIndex(int index)                     { mSupportIndex = index;                                                      }
    void SetSupportTotal(int total)                     { mSupportTotal = total;                                                      }
    void SetSupportSide(SupportSides side)              { mSupportSide  = side;                                                       }
    void SetHasFavoriteChild(bool hasFavoriteChild)     { mHasFavoriteChild = hasFavoriteChild;                                       }

    void SetSupportType(SupportTypes type)              { mSupportType  = type;                                                       }

    //! Disable dirtiness until EnableDirtiness is called
    void DisableDirtiness() { mDirtyEnabled = false; }
    //! Enable dirtiness (enabled by default)
    void EnableDirtiness() { mDirtyEnabled = true; }

    // Special setter to force dirty
    void SetDirty() { if (mDirtyEnabled) mDirty = true; }
    void UnsetDirty() { mDirty = false; }

    // Validation
    void Invalidate() { mPathValidated = false; }
    void Validate() { mPathValidated = true; }
    [[nodiscard]] bool IsValidated() const { return mPathValidated; }

    /*
     * Volatile setters - do not set the Dirty flag for auto-saving
     */

    void SetVolatileDescription(const String& description) { DisableDirtiness(); SetDescription(description); EnableDirtiness(); }
    void SetVolatileImagePath(const Path& image) { DisableDirtiness(); SetImagePath(image); EnableDirtiness(); }

    /*
     * String setters
     */

    void SetRomPathAsString(const String& image)           { SetRomPath(Path(image));           }
    void SetLastPatchAsString(const String& patch)         { SetLastPatch(Path(patch)); }
    void SetImagePathAsString(const String& image)         { SetImagePath(Path(image));         }
    void SetBoxPathAsString(const String& box)             { SetBoxPath(Path(box)); }
    void SetManualPathAsString(const String& manual)       { SetManualPath(Path(manual)); }
    void SetMapPathAsString(const String& map)             { SetMapPath(Path(map)); }
    void SetMapsAsXml(const XmlNode& parent, const Path& relativeTo);
    void SerializeMapsAsXml(XmlNode& parent, const Path& relativeTo) const;
    void SetLogoPathAsString(const String& logo)           { SetLogoPath(Path(logo)); }
    void SetVideoPathAsString(const String& video)         { SetVideoPath(Path(video));         }
    void SetTipsAndTricksPathAsString(const String& tips)  { SetTipsAndTricksPath(Path(tips));         }
    void SetReleaseDateAsString(const String& releasedate) { static DateTime zero(0LL); DateTime st; SetReleaseDate(DateTime::FromCompactISO6801(releasedate, st) ? st : zero); }
    void SetLastPlayedAsString(const String& lastplayed)   { static DateTime zero(0LL); DateTime st; SetLastPlayedDate(DateTime::FromCompactISO6801(lastplayed, st) ? st : zero); }
    void SetRatingAsString(const String& rating)           { float f = 0.0f; if (StringToFloat(rating, f)) SetRating(f);              }
    void SetPlayersAsString(const String& players)         { unsigned short maxmin = 0; if (RangeToInt(players, maxmin)) SetPlayersRaw(maxmin); else SetPlayers(1, 1); }
    void SetFavoriteAsString(const String& favorite)       { SetFavorite(favorite == "true");                                         }
    void SetHiddenAsString(const String& hidden)           { SetHidden(hidden == "true");                                             }
    void SetAdultAsString(const String& adult)             { SetAdult(adult == "true");                                               }
    void SetRomCrc32AsString(const String& romcrc32)       { int c = 0; if (HexToInt(romcrc32, c)) SetRomCrc32(c);                    }
    void SetPlayCountAsString(const String& playcount)     { int p = 0; if (StringToInt(playcount, p)) SetPlayCount(p);               }
    void SetGenreIdAsString(const String& genre)           { int g = 0; if (StringToInt(genre, g)) SetGenreId((GameGenres)g);         }
    void SetRegionAsString(const String& region)           { SetRegion(Regions::Deserialize4Regions(region).Sort());                  }
    void SetRotationAsString(const String& rotation)       { SetRotation(RotationUtils::FromString(rotation));                        }
    void SetTotalPlayTimeAsString(const String& timePlayed){ int u = 0; if (StringToInt(timePlayed, u)) SetTotalPlayTime(u);          }
    /*
     * Defaults
     */

    [[nodiscard]] bool IsDefaultRom()             const { return Default().mRomFile == mRomFile && Default().mRomPath == mRomPath; }
    [[nodiscard]] bool IsDefaultName()            const { return Default().mName == mName;               }
    [[nodiscard]] bool IsDefaultAliases()         const { return Default().mAliases == mAliases;         }
    [[nodiscard]] bool IsDefaultLicences()        const { return Default().mLicences == mLicences;       }
    [[nodiscard]] bool IsDefaultEmulator()        const { return Default().mEmulator == mEmulator;       }
    [[nodiscard]] bool IsDefaultCore()            const { return Default().mCore == mCore;               }
    [[nodiscard]] bool IsDefaultRatio()           const { return Default().mRatio == mRatio;             }
    [[nodiscard]] bool IsDefaultDescription()     const { return Default().mDescription == mDescription; }
    [[nodiscard]] bool IsDefaultImage()           const { return Default().mImageFile == mImageFile && Default().mImagePath == mImagePath; }
    [[nodiscard]] bool IsDefaultBox()             const { return Default().mBoxFile == mBoxFile && Default().mBoxPath == mBoxPath; }
    [[nodiscard]] bool IsDefaultManual()          const { return Default().mManualFile == mManualFile && Default().mManualPath == mManualPath; }
    [[nodiscard]] bool IsDefaultMap()             const { return Default().mMapFile == mMapFile && Default().mMapPath == mMapPath; }
    [[nodiscard]] bool IsDefaultLogo()            const { return Default().mLogoFile == mLogoFile && Default().mLogoPath == mLogoPath; }
    [[nodiscard]] bool IsDefaultVideo()           const { return Default().mVideoFile == mVideoFile && Default().mVideoPath == mVideoPath; }
    [[nodiscard]] bool IsDefaultTipsAndTricks()   const { return Default().mTipsFile == mTipsFile && Default().mTipsPath == mTipsPath; }
    [[nodiscard]] bool IsDefaultDeveloper()       const { return Default().mDeveloper == mDeveloper;     }
    [[nodiscard]] bool IsDefaultPublisher()       const { return Default().mPublisher == mPublisher;     }
    [[nodiscard]] bool IsDefaultGenre()           const { return Default().mGenre == mGenre;             }
    [[nodiscard]] bool IsDefaultRegion()          const { return Default().mRegion.Pack == mRegion.Pack; }
    [[nodiscard]] bool IsDefaultRating()          const { return Default().mRating == mRating;           }
    [[nodiscard]] bool IsDefaultPlayerRange()     const { return Default().mPlayers == mPlayers;         }
    [[nodiscard]] bool IsDefaultReleaseDateEpoc() const { return Default().mReleaseDate == mReleaseDate; }
    [[nodiscard]] bool IsDefaultPlayCount()       const { return Default().mPlayCount == mPlayCount;     }
    [[nodiscard]] bool IsDefaultLastPlayedEpoc()  const { return Default().mLastPlayed == mLastPlayed;   }
    [[nodiscard]] bool IsDefaultRomCrc32()        const { return Default().mRomCrc32 == mRomCrc32;       }
    [[nodiscard]] bool IsDefaultFavorite()        const { return Default().mFavorite == mFavorite;       }
    [[nodiscard]] bool IsDefaultHidden()          const { return Default().mHidden == mHidden;           }
    [[nodiscard]] bool IsDefaultAdult()           const { return Default().mAdult == mAdult;             }
    [[nodiscard]] bool IsDefaultGenreId()         const { return Default().mGenreId == mGenreId;         }
    [[nodiscard]] bool IsDefaultLastPath()        const { return Default().mLastPatchPath == mLastPatchPath && Default().mLastPatchFile == mLastPatchFile; }
    [[nodiscard]] bool IsDefaultRotation()        const { return Default().mRotation == mRotation; }
    [[nodiscard]] bool IsDefaultTotalPlayTime()   const { return Default().mTotalPlayTime == mTotalPlayTime; }
    [[nodiscard]] bool IsDefaultTate()            const { return Default().mRotation == mRotation; }

    /*
     * Convenient Accessors
     */

    [[nodiscard]] bool IsDirty()  const { return mDirty; }
    [[nodiscard]] bool HasSupportInformation() const { return mSupportSide != SupportSides::None || mSupportIndex != 0; }
    [[nodiscard]] bool IsFirstOrUniqueSupport() const { return mSupportSide == SupportSides::A || mSupportIndex == 1; }

    /*
     * Special modifiers
     */

    void IncPlayCount() { mPlayCount++; SetDirty(); }
    void SetLastPlayedNow() { mLastPlayed = (unsigned int)DateTime().ToEpochTime(); SetDirty(); }


    /*
     * Metadata FieldManagement Methods
     */

    /*!
     * Return the first static internal field descriptor reference
     * @param count Number of actual field descriptor available
     * @return first static internal field descriptor reference
     */
    const MetadataFieldDescriptor* GetMetadataFieldDescriptors(int& count) { return GetMetadataFieldDescriptors(mType, count); }

    /*
     * Index accessors
     */

    [[nodiscard]] StringHolderTypes::Index32 FileIndex() const { return mRomFile; }
    [[nodiscard]] StringHolderTypes::Index32 NameIndex() const { return mName; }
    [[nodiscard]] StringHolderTypes::Index32 AliasesIndex() const { return mAliases; }
    [[nodiscard]] StringHolderTypes::Index32 LicencesIndex() const { return mLicences; }
    [[nodiscard]] StringHolderTypes::Index32 DescriptionIndex() const { return mDescription; }
    [[nodiscard]] StringHolderTypes::Index32 DeveloperIndex() const { return mDeveloper; }
    [[nodiscard]] StringHolderTypes::Index32 PublisherIndex() const { return mPublisher; }

    static int FileIndexCount() { return sFileHolder.ObjectCount(); }
    static int NameIndexCount() { return sNameHolder.ObjectCount(); }
    static int DescriptionIndexCount() { return sDescriptionHolder.ObjectCount(); }
    static int DeveloperIndexCount() { return sDeveloperHolder.ObjectCount(); }
    static int PublisherIndexCount() { return sPublisherHolder.ObjectCount(); }

    /*
     * Search
     */

    [[nodiscard]] bool IsMatchingFileIndex(StringHolderTypes::Index32 index) const { return mRomFile == index; }
    [[nodiscard]] bool IsMatchingNameIndex(StringHolderTypes::Index32 index) const { return mName == index; }
    [[nodiscard]] bool IsMatchingDescriptionIndex(StringHolderTypes::Index32 index) const { return mDescription == index; }
    [[nodiscard]] bool IsMatchingDeveloperIndex(StringHolderTypes::Index32 index) const { return mDeveloper == index; }
    [[nodiscard]] bool IsMatchingPublisherIndex(StringHolderTypes::Index32 index) const { return mPublisher == index; }

    /*!
     * @brief Search text in game names
     * @param originaltext Text to search for
     * @param output Result container
     */
    static void SearchInNames(const String& originaltext, StringHolderTypes::FoundTextList& output, int context) { sNameHolder.FindText(originaltext, output, context); }

    /*!
     * @brief Search text in descriptions
     * @param originaltext Text to search for
     * @param output Result container
     */
    static void SearchInDescription(const String& originaltext, StringHolderTypes::FoundTextList& output, int context) { sDescriptionHolder.FindText(originaltext, output, context); }

    /*!
     * @brief Search text in developer names
     * @param originaltext Text to search for
     * @param output Result container
     */
    static void SearchInDeveloper(const String& originaltext, StringHolderTypes::FoundTextList& output, int context) { sDeveloperHolder.FindText(originaltext, output, context); }

    /*!
     * @brief Search text in publisher names
     * @param originaltext Text to search for
     * @param output Result container
     */
    static void SearchInPublisher(const String& originaltext, StringHolderTypes::FoundTextList& output, int context) { sPublisherHolder.FindText(originaltext, output, context); }

    /*!
     * @brief Search text in file names
     * @param originaltext Text to search for
     * @param output Result container
     */
    static void SearchInPath(const String& originaltext, StringHolderTypes::FoundTextList& output, int context) { sFileHolder.FindText(originaltext, output, context); }

    /*
     * Part comparers
     */

    [[nodiscard]] bool AreRomEqual(const MetadataDescriptor& other) const { return mRomFile == other.mRomFile && mRomPath == other.mRomPath; }
} __attribute((packed));

