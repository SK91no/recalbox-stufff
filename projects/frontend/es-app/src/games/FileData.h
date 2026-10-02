#pragma once

#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>
#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include "MetadataDescriptor.h"
#include "ItemType.h"
#include "utils/cplusplus/Bitflags.h"
#include <utils/storage/Set.h>
#include <algorithm>

// Usefull macros
#define LockedStatement(fileData, executor) do{ (fileData).Lock(); executor; (fileData).Unlock(); }while(false)
#define LockedExpression(fileData, expression) ({ (fileData).Lock(); auto expressionResult = (expression); (fileData).Unlock(); expressionResult; })

// Forward declarations
class SystemData;
class FolderData;
class RootFolderData;

// A tree node that holds information for a file.
class FileData
{
  public:
    typedef HashMap<String, FileData*> StringMap;
#ifndef DEBUG_FILE_TREE
    typedef Array<FileData*> List;
#else
    class List : public std::vector<FileData*>
    {
      public:
        void Add(FileData* f) { push_back(f); }
        [[nodiscard]] int Count() const { return (int)size(); }
        void Clear() { clear(); }
        [[nodiscard]] bool Empty() const { return empty(); }
        bool Remove(FileData* f) { int c = Count(); erase(std::remove(begin(), end(), f), end()); return c != Count(); }
        void Insert(FileData* f, int p) { insert(begin() + p, f); }
        void Reserve(int i) { reserve(i); }
        void Swap(int i, int j) { std::swap(operator[](i), operator[](j)); }
        FileData*& operator()(int index) { return (*this)[index]; }
        void CopyFrom(const List& other) { insert(end(), other.begin(), other.end()); }
        bool Contains(const FileData* f) { return std::find(begin(), end(), f) != end(); }
    };
#endif
    typedef Array<const FileData*> ConstList;

    enum class TopLevelFilter
    {
      None              = 0x00000000,
      Favorites         = 0x00000001,
      Hidden            = 0x00000002,
      Yoko              = 0x00000004,
      LatestVersion     = 0x00000008,
      Adult             = 0x00000010,
      Preinstalled      = 0x00000020,
      NotAGame          = 0x00000040,
      Board             = 0x00000080,
      OneAndTwoPlayers  = 0x00000100,
      Tate              = 0x00000200,

    };

    //! Game filters
    enum class Filter
    {
      None             = 0,  //!< Include nothing
      Normal           = 1,  //!< Include normal files (not hidden, not favorite)
      Favorite         = 2,  //!< Include favorites
      Hidden           = 4,  //!< Include hidden
      Adult            = 8,
      NotLatest        = 16,
      NoGame           = 32,
      PreInstalled     = 64,
      Board            = 128,
      Yoko             = 256,
      OneAndTwoPlayers = 512,
      Tate             = 1024,
      All              = 2047, //!< Include all
    };

    //! Search attribute enumeration
    enum class SearchAttributes
    {
      ByName        = 1, //!< Search by name, excluding extension
      ByNameWithExt = 2, //!< Search by name, including extension
      ByHash        = 4, //!< Search by hash
      All           = 7, //!< All attributes
    };

  protected:
    //! Top ancestor (link to system)
    RootFolderData& mTopAncestor;
    //! Parent folder
    FolderData* mParent;
    //! Item type - Const ensure mType cannot be modified after being set by the constructor, so that it's alays safe to use c-style cast for FolderData sub-class.
    const ItemType mType;

    /*!
     * Constructor for subclasses only
     * @param type
     * @param path Item path
     * @param system Parent system
     */
    FileData(ItemType type, const Path& path, RootFolderData& ancestor);

  private:
    //! Metadata
    MetadataDescriptor mMetadata;

  public:
    //! Type for mass locking
    typedef HashSet<FolderData*> LockedOnce;

    /*!
     * Constructor
     * @param path Item path on filesystem
     * @param system system to attach to
     */
    FileData(const Path& path, RootFolderData& ancestor);

    //! Destructor
    virtual ~FileData() = default;

    /*
     * Getters
     */

    [[nodiscard]] virtual String Name() const { return mMetadata.Name(); }
    [[nodiscard]] String Hash() const { return mMetadata.RomCrc32AsString(); }
    [[nodiscard]] ItemType Type() const { return mType; }
    [[nodiscard]] Path RomPath() const { return mMetadata.Rom(); }
    [[nodiscard]] FolderData* Parent() const { return mParent; }
    [[nodiscard]] RootFolderData& TopAncestor() const { return mTopAncestor; }
    [[nodiscard]] SystemData& System() const;
    //! Check if we have a CRC32. Keep it unprotected to avoid over-locking object as it's a non critical data
    [[nodiscard]] bool HasCrc32() const { return mMetadata.RomCrc32() != 0; }

    /*
     * Booleans
     */

    [[nodiscard]] bool IsEmpty() const { return mType == ItemType::Empty; }
    [[nodiscard]] bool IsHeader() const { return mType == ItemType::Header; }
    [[nodiscard]] bool IsGame() const { return mType == ItemType::Game; }
    [[nodiscard]] bool IsFolder() const { return mType == ItemType::Folder || mType == ItemType::Root; }
    [[nodiscard]] bool IsFolderOnly() const { return mType == ItemType::Folder; }
    [[nodiscard]] bool IsRoot() const { return mType == ItemType::Root; }
    [[nodiscard]] bool IsTopMostRoot() const { return mType == ItemType::Root && mParent == nullptr; }

    /*
     * Setters
     */

    /*!
     * @brief Set this' parent. Can only be set by a folder and never modified
     * @param parent Parent
     */
    void SetParent(FolderData* parent) { mParent = parent; }

    /*!
     * Get Thumbnail path if there is one, or Image path.
     * @return file path (may be empty)
     */
    [[nodiscard]] Path AnyMedia() const { return mMetadata.HasImage() ? mMetadata.Image() : mMetadata.Box(); }

    /*!
     * Return true if at least one image is available (thumbnail or regular image)
     * @return Boolean result
     */
    [[nodiscard]] bool HasAnyMedia() const { return (mMetadata.HasBox() || mMetadata.HasImage()); }

    /*!
     * const Metadata accessor for Read operations
     * @return const Metadata object
     */
    [[nodiscard]] const MetadataDescriptor& Metadata() const { return mMetadata; }

    /*!
     * Metadata accessor for Write operations only
     * @return Writable Metadata object
     */
    MetadataDescriptor& Metadata() { return mMetadata; }

    /*!
     * @brief Get Pad2Keyboard configuration file path
     * @return Pad2Keyboard configuration file path
     */
    [[nodiscard]] Path P2KPath() const { Path p(RomPath()); return p.ChangeExtension(p.Extension() + ".p2k.cfg"); }

    /*!
     * @brief Get recalbox.conf configuration file path
     * @return recalbox.conf configuration file path
     */
    [[nodiscard]] Path RecalboxConfPath() const { Path p(RomPath()); return p.ChangeExtension(p.Extension() + ".recalbox.conf"); }

      /*!
       * @brief Check if Pad2Keyboard configuration file exists
       * @return Trie if the Pad2Keyboard configuration file exists
       */
    [[nodiscard]] bool HasP2K() const;

    /*!
     * @brief Calculate rom CRC32 and store it in metadata
     * @return This
     */
    FileData& CalculateHash();

    /*!
     * @brief Get region string
     * @return
     */
    //String Regions();

    /*!
     * @brief Check if file data can be displayable
     * @return displayable state
     */
    [[nodiscard]] bool IsDisplayable(TopLevelFilter topfilter) const;

    /*!
     * @brief Check if rom path equals the rom path of the given filedata
     * @param other Other filedata to compare rom path
     * @return True if rom path are equal, false otherwise
     */
    bool AreRomEqual(const FileData& other) { return mMetadata.AreRomEqual(other.mMetadata); }

    /*!
     * @brief Build a top level filter to use in visible file get/count methods
     * @return Top level filter
     */
    static TopLevelFilter BuildTopLevelFilter();
};

DEFINE_BITFLAG_ENUM(FileData::TopLevelFilter, int)
DEFINE_BITFLAG_ENUM(FileData::Filter, int)
DEFINE_BITFLAG_ENUM(FileData::SearchAttributes, int)
