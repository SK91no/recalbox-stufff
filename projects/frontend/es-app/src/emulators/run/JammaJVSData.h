//
// Created by gugue_u on 01/04/2022.
//

#pragma once


#include "hardware/crt/CrtAdapterType.h"
#include "hardware/Board.h"
#include "CrtConf.h"
#include "emulators/EmulatorData.h"

class JammaJVSData
{
  private:
    bool systemIsDreamcastArcade(const String& systemName) const
    {
      return systemName == "atomiswave" || systemName == "naomigd" || systemName == "naomi";
    }

    bool systemIsArcade(const FileData& game, const EmulatorData& emulator) const
    {
      return systemIsDreamcastArcade(game.System().Name())
              || (emulator.Emulator() == "libretro" && (emulator.Core() == "fbneo" || emulator.Core().StartsWith("mame")));
    }

  public:
    JammaJVSData() {};
    bool ShouldConfigureArcadePanelConfiggen() const {
      return Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma || Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS;
    }

    bool ShouldSwitchTo6ButtonLayout(const FileData& game, const EmulatorData& emulator) const {
      // Model 3 (supermodel) is native arcade — its JVS panel must not be reshuffled.
      return  ShouldConfigureArcadePanelConfiggen()
              && (CrtConf::Instance().GetSystemCRTJammaPanelButtons() == "6" || Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS)
              && ! systemIsArcade(game, emulator)
              && emulator.Emulator() != "supermodel";
    }

    bool ShouldConfigureNeoGeoLayout(const FileData& game, const EmulatorData& emulator) const {
      String buttonOnPanel = CrtConf::Instance().GetSystemCRTJammaPanelButtons();
      return  ShouldConfigureArcadePanelConfiggen()
              && (buttonOnPanel == "6" || buttonOnPanel == "5")
              && systemIsArcade(game, emulator);
    }

    String JammaControlType(const FileData& game, const EmulatorData& emulator, int player) const {
      if(ShouldSwitchTo6ButtonLayout(game, emulator))
        return "6btns";
      else if(ShouldConfigureNeoGeoLayout(game, emulator))
      {
        if(player == 1)
          return CrtConf::Instance().GetSystemCRTJammaNeogeoLayoutP1();
        if(player == 2)
          return CrtConf::Instance().GetSystemCRTJammaNeogeoLayoutP2();
      }
      return "standard";
    }
};
