//
// Created by bkg2k on 19/12/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <hardware/crt/CrtAdapterType.h>
#include <hardware/BoardType.h>
#include <string>

class ICrtInterface
{
  public:
    enum class HorizontalFrequency
    {
      Auto, //!< auto
      KHz15, //!< 15 Khz
      KHzMulti1525, //!< MultiSync
      KHz31, //!< 31 Khz
      KHzMulti1531, //!< MultiSync
      KHzTriFreq, //!< MultiSync
    };

    static std::string HorizontalFrequencyToString(HorizontalFrequency freq){
      switch (freq)
      {
        case HorizontalFrequency::Auto: return "Auto";
        case HorizontalFrequency::KHzMulti1525: return "kHzMulti1525";
        case HorizontalFrequency::KHzMulti1531: return "kHzMulti1531";
        case HorizontalFrequency::KHzTriFreq: return "kHzTriFreq";
        case HorizontalFrequency::KHz31: return "kHz31";
        case HorizontalFrequency::KHz15:
        default: return "kHz15";
      }
    }

    static std::string HorizontalFrequencyToHumanReadable(HorizontalFrequency freq){
      switch (freq)
      {
        case HorizontalFrequency::Auto: return "Auto";
        case HorizontalFrequency::KHz15: return "15kHz";
        case HorizontalFrequency::KHzMulti1525: return "Multi 15/25kHz";
        case HorizontalFrequency::KHzMulti1531: return "Multi 15/31kHz";
        case HorizontalFrequency::KHzTriFreq: return "TriFreq 15/25/31kHz";
        case HorizontalFrequency::KHz31: return "31kHz";
        default: return "Auto";
      }
    }
    /*!
     * @brief Default constructor
     * @param automaticallyDetected True if the board has been automatically detected
     */
    explicit ICrtInterface(bool automaticallyDetected, BoardType boardType) : mAutomaticallyDetected(automaticallyDetected), mBoardType(boardType) {}

    //! Default destructor
    virtual ~ICrtInterface() = default;

    /*!
     * @brief Check if a CRT adpater is attached
     * @return True if a CRT adapter is attached, false otherwise
     */
    virtual bool IsCrtAdapterAttached() const = 0;

    /*!
     * @brief Get CRT adapter
     * @return CRT adater
     */
    virtual CrtAdapterType GetCrtAdapter() const = 0;

    /*!
     * @brief Check if the adapter supports 31khz
     * @return True if the adapter supports 31khz
     */
    virtual bool Has31KhzSupport() const = 0;

    /*!
     * @brief Check if the adapter supports 120hz modes
     * @return True if the adapter supports 120hz modes
     */
    virtual bool Has120HzSupport() const { return false; }

    /*!
     * @brief Check if the adapter supports multisync 15/31khz
     * @return True if the adapter supports multisync 15/31khz
     */
    virtual bool HasMultiSyncSupport() const { return false; }

    /*!
     * @brief Check if the adapter supports composite
     * @return True if the adapter supports composite
     */
    virtual bool HasCompositeSupport() const { return false; }

    /*!
     * @brief Check if the adapter supports widescreen
     * @return True if the adapter supports widescreen
     */
    virtual bool HasWideScreenSupport() const { return false; }

    /*!
     * @brief Check if the session can be widescreen or no
     * @return True if the session can be widescreen
     */
    virtual bool ShouldWideScreen() const { return false; }

    /*!
     * @brief Check if the adapter force composite mode
     * @return True if the adapter force composite mode, otherwize RGB mode
     */
    [[nodiscard]] virtual bool MustForceComposite() const { return false; };

    /*!
     * @brief Get horizontal frequency
     * @return Horitontal frequency
     */
    virtual HorizontalFrequency GetHorizontalFrequency() const = 0;

    /*!
     * @brief Check if the adapter has forced 50hz support
     * @return True if the adapter support forcing 50hz mode, false otherwise
     */
    virtual bool HasForced50hzSupport() const { return false; }

    /*!
     * @brief Check if the adapter has forced 31khz support
     * @return True if the adapter support forcing 31khz mode, false otherwise
     */
    virtual bool HasForced31khzSupport() const { return false; }

    /*!
     * @brief Check if the adapter has forced composite support
     * @return True if the adapter support forcing composite mode, false otherwise
     */
    virtual bool HasForcedCompositeSupport() const { return false; }

    /*!
     * @brief Check if the adapter force 50hz mode
     * @return True if the adapter force 50hz mode, otherwise automatic mode
     */
    virtual bool MustForce50Hz() const = 0;

    /*!
     * @brief Hard 31kHz lock — adapter cannot output anything else, user cannot override.
     * @return True iff the only legal output is 31kHz (e.g. JVS Naomi-based cab).
     */
    virtual bool MustForce31kHz() const { return false;};

    /*!
     * @brief Soft 31kHz preference — adapter defaults to 31kHz but user can override via menu.
     * @return True iff a DIP-style switch suggests 31kHz (e.g. RGBDual2 31kHz dipswitch).
     */
    virtual bool MustForce31kHzByDefault() const { return false; }

    /*!
     * @brief Check if the adapter is locked to 15kHz by hardware (no 31kHz available)
     * @return True if a hardware switch forces 15kHz only (e.g. JVS DIP "15kHz only")
     */
    virtual bool MustForce15kHz() const { return false; }

    /*!
     * @brief Returns the name of the adapter
     * @return the name of the adapter
     */
    virtual std::string& Name() const = 0;

    /*!
     * @brief Returns the short name of the adapter
     * @return the short name of the adapter
     */
    virtual std::string& ShortName() const = 0;
    /*!
     * @brief Check if this board has been automatically detected
     * @return True of the board has been automatically detected, false otherwise
     */
    bool HasBeenAutomaticallyDetected() const { return mAutomaticallyDetected; }

    /*!
     * @brief Check if combo adapter/board supports interlaced
     */
    [[nodiscard]] virtual bool HasInterlacedSupport() const { return mBoardType != BoardType::Pi5; }

    /*!
     * @brief Return forced resolution
     */
    [[nodiscard]] virtual bool HasForcedResolution(int &w, int &h) const { (void)w; (void) h; return false; }

  private:
    //! Automatically detected?
    bool mAutomaticallyDetected;
    BoardType mBoardType;
};
