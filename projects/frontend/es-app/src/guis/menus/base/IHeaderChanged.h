//
// Created by bkg2k on 22/08/24.
//
#pragma once

class IHeaderChanged
{
  public:
    //! default destructor
    virtual ~IHeaderChanged() = default;

    //! Header changed notification
    virtual void HeaderFoldStateChanged() = 0;
};