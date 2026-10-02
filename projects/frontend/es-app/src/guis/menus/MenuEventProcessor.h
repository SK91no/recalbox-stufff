//
// Created by bkg2k on 07/02/25.
//
#pragma once

#include "MenuPersistantData.h"
#include "RecalboxConf.h"
#include "CrtConf.h"

class MenuEventProcessor
{
  public:
    explicit MenuEventProcessor(MenuPersistantData& persistantData)
      : mPersistantData(persistantData)
      , mConf(RecalboxConf::Instance())
      , mCrtConf(CrtConf::Instance())
    {
    }

  private:
    //! Persistant data
    MenuPersistantData& mPersistantData;
    //! Reclabox configuration reference
    RecalboxConf& mConf;
    //! Crt configuration reference
    CrtConf& mCrtConf;
};
