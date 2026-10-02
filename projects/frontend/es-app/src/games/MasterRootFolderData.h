//
// Created by bkg2k on 11/05/25.
//
#pragma once

class SystemData;

#include "RootFolderData.h"

class MasterRootFolderData : public RootFolderData
{
  public:
    /*!
     * @brief Constructor
     * @param system Parent system
     */
    explicit MasterRootFolderData(SystemData& system)
      : RootFolderData(*this, RootFolderData::Ownership::None, RootFolderData::Types::None, Path(), system)
    {}

    /*!
     * @brief Destructor - delete subroots
     */
    ~MasterRootFolderData() override
    {
      // Destroy any subroot first
      for(RootFolderData* root : SubRoots())
      {
        RemoveChild(root);
        delete root;
      }
    }

    //! Has sub root?
    [[nodiscard]] bool HasSubRoots() const
    {
      for(FileData* child : mChildren)
        if (child->IsRoot())
          return true;
      return false;
    }

    /*!
     * @brief Add Sub-root
     * @param subroot Sub-root to add
     */
    void AddSubRoot(RootFolderData* subroot) { AddChild(subroot, true); }

    //! Get sub-root folders
    [[nodiscard]] std::vector<RootFolderData*> SubRoots() const
    {
      std::vector<RootFolderData*> roots;
      for(FileData* child : mChildren)
      {
        assert(child->IsRoot() && "Non subroot item injected in master root !");
        roots.push_back((RootFolderData*) child);
      }
      return roots;
    }
};