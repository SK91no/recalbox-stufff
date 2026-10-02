//
// Created by bkg2k on 01/11/2019.
//
#pragma once

#include <utils/Log.h>
#include <utils/String.h>

class EmulatorDescriptor
{
  public:
    //! Compatibility information
    enum class Compatibility : char
    {
      Unknown, //!< Unknown
      High   , //!< Perfect or near perfect
      Good   , //!< Some games don't work at all or have issues
      Average, //!< Expect to run half of the games
      Low    , //!< only a few games run well and w/o issues
      __Count
    };

    //! Speed information
    enum class Speed : char
    {
      Unknown, //!< Unknown
      High   , //!< Perfect or near perfect
      Good   , //!< Some games will not run at full speed or may have slowdowns
      Average, //!< Expect around half of the games running at a decent framerate
      Low    , //!< Only a few games are playable
      __Count
    };

    //! Core structure
    class CoreData
    {
      public:
        //! Constructor
        CoreData(const String& name,
                 int priority,
                 const String& extensions,
                 bool netplay,
                 const String& compatibility,
                 const String& speed,
                 bool softpatching,
                 bool crtAvailable,
                 const String& flatBaseFile,
                 const String& ignoreDrivers,
                 const String& splitDrivers,
                 int limit,
                 const String& video_backend)
          : mFlatBaseName(flatBaseFile)
          , mIgnoreDrivers(ignoreDrivers)
          , mSplitDrivers(splitDrivers)
          , mName(name)
          , mAllExtensions(extensions)
          , mVideoBackend(video_backend)
          , mPriority(priority)
          , mLimit(limit)
          , mCompatibility(ConvertCompatibility(compatibility))
          , mSpeed(ConvertSpeed(speed))
          , mNetplay(netplay)
          , mSoftpatching(softpatching)
          , mCRTAvailable(crtAvailable)
        {
          mAllExtensions.Remove('!');
          for(const String& ext : extensions.Split(' ', true))
            if (ext.Contains('!')) mArchiveExtensions.Append(' ').Append(ext);
            else mFileExtensions.Append(' ').Append(ext);
          mArchiveExtensions.Remove('!').Trim(' ');
          mFileExtensions.Trim(' ');
        }

        /*
         * Accessors
         */

        [[nodiscard]] const String& FlatBaseName() const { return mFlatBaseName; }   //!< Flat file base name
        [[nodiscard]] const String& IgnoreDrivers() const { return mIgnoreDrivers; } //!< Flat file base name
        [[nodiscard]] const String& SplitDrivers() const { return mSplitDrivers; }   //!< Flat file base name
        [[nodiscard]] const String& Name() const { return mName; }                   //!< Core name (file name)
        [[nodiscard]] String Extensions() const { return String(mAllExtensions); }   //!< Supported extensions
        [[nodiscard]] String FileExtensions() const { return String(mFileExtensions); }   //!< Supported extensions
        [[nodiscard]] String ArchiveExtensions() const { return String(mArchiveExtensions); }   //!< Supported extensions
        [[nodiscard]] const String& VideoBackend() const { return mVideoBackend; }   //!< Video backend to use for that core
        [[nodiscard]] int Priority() const { return mPriority; }                     //!< Core priority
        [[nodiscard]] int Limit() const { return mLimit; }                           //!< Manufacturer/driver limit
        [[nodiscard]] Compatibility CompatibilityRate() const { return mCompatibility; } //!< Compatibility rate
        [[nodiscard]] Speed AverageSpeed() const { return mSpeed; }                  //!< Average speed
        [[nodiscard]] String CompatibilityRateAsString() const { return ConvertCompatibility(mCompatibility); } //!< Compatibility rate
        [[nodiscard]] String AverageSpeedAsString() const { return ConvertSpeed(mSpeed); }                      //!< Average speed
        [[nodiscard]] bool Netplay() const { return mNetplay; }                      //!< Netplay compatible?
        [[nodiscard]] bool Softpatching() const { return mSoftpatching; }            //!< Softpathing compatible?
        [[nodiscard]] bool CRTAvailable() const { return mCRTAvailable; }            //!< Available on CRT?

      private:
        // Arcade properties
        String mFlatBaseName;         //!< Flat file base name
        String mIgnoreDrivers;        //!< Flat file base name
        String mSplitDrivers;         //!< Flat file base name
        // Core properties
        String mName;                 //!< Core name (file name)
        String mAllExtensions;        //!< Supported extensions
        String mFileExtensions;       //!< Supported extensions, non-archive only
        String mArchiveExtensions;    //!< Supported extensions, archive only
        String mVideoBackend;         //!< Video backend to use for that core
        int mPriority;                //!< Core priority
        int mLimit;                   //!< Manufacturer/driver limit
        Compatibility mCompatibility; //!< Compatibility rate
        Speed mSpeed;                 //!< Average speed
        bool mNetplay;                //!< Netplay compatible?
        bool mSoftpatching;           //!< Softpatching compatible?
        bool mCRTAvailable;           //!< Available on CRT?
    };

    //! Default constructor
    EmulatorDescriptor()
      : EmulatorDescriptor("")
    {
    }

    /*!
     * @brief Constructor
     * @param emulator Emulator name
     */
    explicit EmulatorDescriptor(const String& emulator)
      : mEmulator(emulator)
    {
    }

    //! Get emulator name
    [[nodiscard]] const String& Name() const { return mEmulator; }

    //! Get core count
    [[nodiscard]] int CoreCount() const { return (int)mCores.size(); }

    //! Get core
    [[nodiscard]] const CoreData& Core(int index) const { return (unsigned int)index < (unsigned int)mCores.size() ? mCores[index] : mCores[0]; }

    //! Has at least one core?
    [[nodiscard]] bool HasAny() const { return !mCores.empty(); }

    [[nodiscard]] bool CRTAvailable() const
    {
      for(const CoreData& core : mCores)
        if (core.CRTAvailable())
          return true;
      return false;
    }

    /*!
     * @brief Check if the emulator has a core matching the given name
     * @param name Core name
     * @return True of the emulator has this core, false otherwise
     */
    [[nodiscard]] bool HasCore(const String& name) const
    {
      for(const CoreData& core : mCores)
        if (name == core.Name())
          return true;
      return false;
    }

    /*!
     * @brief Check if the emulator has a core matching the given name
     * @param name Core name
     * @return True of the emulator has this core, fals eotherwise
     */
    [[nodiscard]] const CoreData& Core(const String& name) const
    {
      for(const CoreData& core : mCores)
        if (name == core.Name())
          return core;
      return sEmptyCore;
    }

    /*!
     * @brief Add core
     * @param name Name (file name part)
     * @param priority Priority (lowest value = highest priority)
     * @param extensions File extensions
     * @param netplay Netplay support?
     * @param compatibility Compatibility evaluation
     * @param speed Speed evaluation
     * @param video backend of the core
     */
    void AddCore(const String& name,
                 int priority,
                 const String& extensions,
                 bool netplay,
                 const String& compatibility,
                 const String& speed,
                 bool softpatching,
                 bool crtAvailable,
                 const String& flatBaseFile,
                 const String& ignoreDrivers,
                 const String& splitDrivers,
                 int limit,
                 const String& video_backend)
    {
      mCores.push_back(CoreData(name, priority, extensions, netplay, compatibility, speed, softpatching, crtAvailable, flatBaseFile, ignoreDrivers, splitDrivers, limit, video_backend));
    }

    //! Empty/null core
    static const CoreData& NullCore() { return sEmptyCore; }

  private:
    //! Empty/null core
    static CoreData sEmptyCore;

    //! Emulator name
    String mEmulator;
    //! Core specifications
    std::vector<CoreData> mCores;

    /*!
     * @brief Convert compatibility string to compatibility enum
     * @param compatibility compatibility string
     * @return compatibility enum
     */
    static Compatibility ConvertCompatibility(const String& compatibility)
    {
      Compatibility result = Compatibility::Unknown;
      if      (compatibility == "unknown"  ) result = Compatibility::Unknown;
      else if (compatibility == "high"     ) result = Compatibility::High;
      else if (compatibility == "good"     ) result = Compatibility::Good;
      else if (compatibility == "average"  ) result = Compatibility::Average;
      else if (compatibility == "low"      ) result = Compatibility::Low;
      else { LOG(LogError) << "[SystemDescriptor] Unknown compatibility type " << compatibility << " !"; }
      return result;
    }

    /*!
     * @brief Convert speed string to speed enum
     * @param speed speed string
     * @return speed enum
     */
    static Speed ConvertSpeed(const String& speed)
    {
      Speed result = Speed::Unknown;
      if      (speed == "unknown"  ) result = Speed::Unknown;
      else if (speed == "high"     ) result = Speed::High;
      else if (speed == "good"     ) result = Speed::Good;
      else if (speed == "average"  ) result = Speed::Average;
      else if (speed == "low"      ) result = Speed::Low;
      else { LOG(LogError) << "[SystemDescriptor] Unknown Speed type " << speed << " !"; }
      return result;
    }

    static String ConvertCompatibility(Compatibility compatibility)
    {
      switch(compatibility)
      {
        case Compatibility::High: { static String s("high"); return s; }
        case Compatibility::Good: { static String s("good"); return s; }
        case Compatibility::Average: { static String s("average"); return s; }
        case Compatibility::Low: { static String s("low"); return s; }
        case Compatibility::__Count:
        case Compatibility::Unknown:
        default: break;
      }
      static String s("unknown");
      return s;
    }

    static String ConvertSpeed(Speed speed)
    {
      switch(speed)
      {
        case Speed::High: { static String s("high"); return s; }
        case Speed::Good: { static String s("good"); return s; }
        case Speed::Average: { static String s("average"); return s; }
        case Speed::Low: { static String s("low"); return s; }
        case Speed::Unknown:
        case Speed::__Count:
        default: break;
      }
      static String s("unknown");
      return s;
    }
};

