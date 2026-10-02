//
// Created by bkg2k on 02/03/24.
//
#pragma once

#include <games/FileData.h>
//#include <games/MetadataDescriptor.h>
#include <systems/SystemData.h>
#include <games/FileSorts.h>

/*!
 * @brief This class is used as a placeholder in gamelists
 * Name is brought into local space to avoid conflicts w/ optimized names in regular items
 */
class HeaderData : public FileData
{
  public:
    //!  Constructor
    HeaderData(FileSorts::Sorts sort, const String& name, SystemData& system)
      : FileData(ItemType::Header, Path::Empty, system.MasterRoot())
      , mName(name)
      , mSort(sort)
      , mInt(0)
      , mFloat(0.f)
      , mFolded(false)
    {
    }

    //! Constructor
    HeaderData(FileSorts::Sorts sort, const String& name, SystemData& system, int idata, float fdata)
      : FileData(ItemType::Header, Path::Empty, system.MasterRoot())
      , mName(name)
      , mSort(sort)
      , mInt(idata)
      , mFloat(fdata)
      , mFolded(false)
    {
    }

    //! Get local name undecorated
    [[nodiscard]] String Name() const final { return mName; }

    //! Get decorated local name
    [[nodiscard]] String Name(bool leftIcon) const
    {
      if (leftIcon)
      {
        String result(mFolded ? "▶ " : "▼ ");
        result.Append(mName);
        return result;
      }
      String result(mName);
      result.Append(mFolded ? " ◀" : " ▼");
      return result;
    }

    //! Original sort
    FileSorts::Sorts Sort() const { return mSort; }

    //! Fold status
    [[nodiscard]] bool IsFolded() const { return mFolded; }
    //! Set fold/unfold
    void SetFolded(bool folded) { mFolded = folded; }
    //! Toggle folded state
    void ToggleFolded() { mFolded = !mFolded; }

    //! Integer data
    [[nodiscard]] int Int() const { return mInt; }
    //! Float data
    [[nodiscard]] float Float() const { return mFloat; }

  private:
    //! Direct name as string
    String mName;
    //! Sort that generated this header
    FileSorts::Sorts mSort;
    //! Integer data
    int mInt;
    //! float data
    float mFloat;
    //! Fold status
    bool mFolded;
    //! Metadata
    using FileData::Metadata;
};
