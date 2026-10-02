//
// Created by bkg2k on 26/11/24.
//
#pragma once

#include "utils/String.h"
#include "utils/os/fs/Path.h"

class Overclocking
{
  public:
    //! O/C file & description
    struct Overclock
    {
      private:
        Path mFile;
        String mDescription;
        int mFrequency;
        bool mHazardous;

      public:
        //! Constrctor
        Overclock(const Path& path, const String& description, int frequency, bool hazardous)
          : mFile(path)
          , mDescription(description)
          , mFrequency(frequency)
          , mHazardous(hazardous)
        {}

        // Getters
        [[nodiscard]] const Path& FilePath() const { return mFile; }
        [[nodiscard]] const String& Description() const { return mDescription; }
        [[nodiscard]] int Frequency() const { return mFrequency; }
        [[nodiscard]] bool Hazardous() const { return mHazardous; }

        //! Equality operator
        bool operator == (const Overclock& r) const
        {
          return (mFile == r.mFile) &&
                 (mDescription == r.mDescription) &&
                 (mHazardous == r.mHazardous) &&
                 (mFrequency == r.mFrequency);
        }
    };
    //! List of overclock
    typedef std::vector<Overclock> List;

    //! Constructor
    Overclocking()
    {
      Load();
    }

    //! Has valid overclocks?
    [[nodiscard]] bool HasOverclocks() const { return !mOverclocks.empty(); }

    //! Get list
    [[nodiscard]] const List& GetOverclockList() const { return mOverclocks; }

    //! Install the given overclock
    static bool Install(const Overclock& oc);

    //! Default no overclocking
    static const Overclock& NoOverclock() ;

    //! Get selected O/C
    Overclock GetCurrentOverclock() const;

    //! Reset overclocking
    static void Reset();

  private:
    //! O/C base folder
    static constexpr const char* sOverclockBaseFolder = "/recalbox/system/configs/overclocking";
    //! Target O/C file
    static constexpr const char* sOverclockFile = "/boot/recalbox-oc-config.txt";

    //! O/C List
    List mOverclocks;

    //! Load overclocks
    void Load();
};