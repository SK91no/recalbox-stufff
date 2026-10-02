//
// Created by bkg2k on 31/05/23.
//
#pragma once

#include <views/gamelist/DetailedGameListView.h>
#include <systems/arcade/ArcadeTupple.h>
#include "IArcadeGamelistInterface.h"

class ArcadeGameListView : public DetailedGameListView
                         , public IArcadeGamelistInterface
                         , private RecalboxConf::IArcadeUseDatabaseNamesNotification
                         , private RecalboxConf::IArcadeViewHideBiosNotification
                         , private RecalboxConf::IArcadeViewHideNonWorkingNotification
                         , private RecalboxConf::ISystemEmulatorNotification
{
  public:
    /*!
     * @brief Constructor
     * @param window Windows manager reference
     * @param systemManager System manager reference
     * @param system Target system
     */
    ArcadeGameListView(WindowManager& window, SystemManager& systemManager, SystemData& system, const IGlobalVariableResolver& resolver, PictogramCaches& flagCache);

    [[nodiscard]] GamelistViewType Type() const override { return GamelistViewType::Arcade; };

  private:
    //! Linked arcade/game structure + clone list
    struct ParentTupple : public ArcadeTupple
    {
      ArcadeTuppleList* mCloneList;
      bool mFolded;   //!< True if the parent is folded
      //! Constructor
      ParentTupple(const ArcadeGame* arcade, FileData* game, bool folded) : ArcadeTupple(arcade, game), mCloneList(nullptr), mFolded(folded) {}
      //! Copy constructor
      ParentTupple(const ParentTupple& source)
        : ArcadeTupple(source)
        , mCloneList(source.mCloneList != nullptr ? new ArcadeTuppleList(*source.mCloneList) : nullptr)
        , mFolded(source.mFolded)
        {}
      //! Move constructor
      ParentTupple(ParentTupple&& source) noexcept
        : ArcadeTupple(source)
        , mCloneList(source.mCloneList)
        , mFolded(source.mFolded)
      {
        source.mCloneList = nullptr;
      }
      //! Copy operator
      ParentTupple& operator = (const ParentTupple& source)
      {
        if (&source != this)
        {
          ArcadeTupple::operator=(source);
          mCloneList = source.mCloneList != nullptr ? new ArcadeTuppleList(*source.mCloneList) : nullptr;
          mFolded = source.mFolded;
        }
        return *this;
      }
      //! Move operator
      //! Copy operator
      ParentTupple& operator = (ParentTupple&& source) noexcept
      {
        if (&source != this)
        {
          ArcadeTupple::operator=(source);
          mCloneList = source.mCloneList; source.mCloneList = nullptr;
          mFolded = source.mFolded;
        }
        return *this;
      }
      //! Destructor
      ~ParentTupple() { delete mCloneList; }
      //! Inject clones
      void AddClones(ArcadeTuppleList* list) { mCloneList = list; }
    };

    typedef std::vector<ParentTupple> ParentTuppleList;

    //! List
    ParentTuppleList mGameList;

    //! Last database to use
    const ArcadeDatabase* mDatabase;
    //! Default emulator for the current folder
    String mDefaultEmulator;
    //! Default core for the current folder
    String mDefaultCore;

    /*!
     * @brief Return parent only list
     * @return FileData list
     */
    FileData::List GetHighLevelFileList() override;

    /*!
     * @brief Get Arcade interface
     * @return Arcade interface
     */
    IArcadeGamelistInterface* getArcadeInterface() override { return this; }

    /*!
     * @brief Refresh name & properties of the given item
     */
    void RefreshItem(FileData* game) final;

    /*!
     * @brief Rebuild the gamelist - regenerate internal structure
     * @param folder
     * @param target
     */
    void populateList(const FolderData& folder, FileData* target) final;

    /*!
     * @brief Rebuild the gamelist using internal structures
     * @param target If not null, the selected game is selected in the resulting list
     */
    void BuildList(FileData* target);

    /*!
     * @brief Sort items to render special arcade list
     * @param items Unsorted games
     * @param comparer Sort comparer
     * @param ascending True for an ascending sort, false for a descending sort
     * @return True if Arcade games habe been filled, false if no database matches the folder emulator
     */
    bool BuildAndSortArcadeGames(FileData::List& items, FileSorts::ComparerArcade comparer, bool ascending);

    /*!
     * @brief Add a complete sorted lists of arcade tupple to the gamelist
     * @param categoryLists One or more arcade tupple list to sort and add
     * @param comparer Sort comparer
     * @param ascending True = ascending sort
     */
    void AddSortedCategories(const std::vector<ParentTuppleList*>& categoryLists, FileSorts::ComparerArcade comparer, bool ascending);

    /*!
     * @brief Get display name of the given game
     * @param game Arcade tupple
     * @param original True original game
     * @return Final display name
     */
    static String GetDisplayName(const ArcadeTupple& game, const FileData& original);

    /*!
     * @brief Get description of the given game
     * @param game Game
     * @return Description
     */
    String GetDescription(FileData& game) override;

    /*!
     * @brief Get raw text display name w/o any icon or decoration
     * @param game Game from wich to get the name
     * @return Raw display name
     */
    String GetRawDisplayName(const FileData& game) override;

    /*!
     * @brief Get available regions from the given listt
     * @return Region list (may be empty)
     */
    static Regions::List AvailableRegionsInGames(ParentTuppleList& list);

    /*!
     * @brief Lookup the arcade tupple attached to the given
     * @param item game
     * @return ArcadeTupple (or empty ArcadeTupple if the lookup fails!)
     */
    const ArcadeTupple& Lookup(const FileData& item);

    //! Fold all parents
    void FoldAll();

    //! Unfold all parents
    void UnfoldAll();

    //! Fold the given parent or the current clone's parent
    void Fold(FileData* item);

    //! Unfold the given parent
    void Unfold(FileData* item);

    /*
     * IGamelistEvents override
     */

    /*!
     * @brief The user just performed a fold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the fold action has been performed
     * @param index Item index
     */
    void OnFoldGame(IGamelistContainer& caller, FileData* game, int index) override { (void)caller; (void)index; Fold(game); }

    /*!
     * @brief The user just performed an unfold action on a game
     * @param caller Calling gamelist implementation
     * @param game Game on which the unfold action has been performed
     * @param index Item index
     */
    void OnUnfoldGame(IGamelistContainer& caller, FileData* game, int index) override { (void)caller; (void)index; Unfold(game); }

    /*!
     * @brief The user performed a fold all game action
     * @param caller Calling gamelist implementation
     */
    void OnFoldAllGame(IGamelistContainer& caller) override { (void)caller; FoldAll(); }

    /*!
     * @brief The user performed an unfold all game action
     * @param caller Calling gamelist implementation
     */
    void OnUnfoldAllGame(IGamelistContainer& caller) override { (void)caller; UnfoldAll(); }

    /*
     * IArcadeGamelistInterface implementation
     */

    [[nodiscard]] const SystemData& GetAttachedSystem() const override { return mSystem; };

    /*!
     * @brief Check if the Arcade game list has a valid database for the current emulator/core
     * @return
     */
    [[nodiscard]] bool HasValidDatabase() const override { return mDatabase != nullptr; }

    /*!
     * @brief Get driver list (arcade view only)
     * @return Driver list
     */
    [[nodiscard]] std::vector<ArcadeDatabase::Manufacturer> GetManufacturerList() const override;

    /*!
     * @brief Get the current emulator name for the current folder
     * @return Emulator name
     */
    [[nodiscard]] String GetCurrentEmulatorName() const override { return mDefaultEmulator; }

    /*!
     * @brief Get the current core name for the current folder
     * @return Core name
     */
    [[nodiscard]] String GetCurrentCoreName() const override { return mDefaultCore; }

    /*!
     * @brief Get the number of games attached to the given driver, in the current gamalist
     * @param driverIndex Driver index
     * @return Game count
     */
    [[nodiscard]] int GetGameCountForManufacturer(int driverIndex) const override;

    /*!
     * @brief Check if the current database can be filtered
     * @return
     */
    [[nodiscard]] bool CanBeFiltered() const override { return mDatabase != nullptr && mDatabase->CanBeFiltered(); }

    /*!
     * @brief Check if at least one manufacturer in the Limited manufacturers holder match one of the manufacturer in the given set
     * @param manufacturerSet manufacturer set
     * @param manufacturers manufacturer holders
     * @return True if at least one match has been found, false otherwise
     */
    static bool HasMatchingManufacturer(const HashSet<int>& manufacturerSet, const ArcadeGame::LimitedManufacturerHolder& manufacturers);

    /*
     * ISimpleGameListView overrides
     */

    /*!
     * @brief Called back from view manager when a game exited and the user is back to the gamelist
     * @param game Game ran
     */
    //void ReturnedFromGame(FileData* game) override { (void)game; }

    /*
     * RecalboxConf::IArcadeUseDatabaseNamesNotification
     */

    void ConfigurationArcadeUseDatabaseNamesChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::IArcadeViewHideBiosNotification
     */

    void ConfigurationArcadeViewHideBiosChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::IArcadeViewHideNonWorkingNotification
     */

    void ConfigurationArcadeViewHideNonWorkingChanged(const bool& value) final { (void)value; ListRefreshRequired(); }

    /*
     * RecalboxConf::ISystemEmulatorNotification
     */

    void ConfigurationSystemEmulatorChanged(const SystemData& system, const String& value) final
    {
      (void)value;
      // In Arcade view, emulator change may change the way games are listed, so refresh the list
      if (&system == &mSystem)
        ListRefreshRequired();
    }
};
