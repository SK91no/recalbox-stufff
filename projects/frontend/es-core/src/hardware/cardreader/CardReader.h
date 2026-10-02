//
// Created by digitallumberjack on 26/10/25.
//

#pragma once

#include <utils/cplusplus/StaticLifeCycleControler.h>

#include "CardReaderListener.h"
#include "rendering/opengl/Colors.h"
#include "utils/os/system/Thread.h"
#include "utils/sync/SyncMessageSender.h"


class CardReader : public StaticLifeCycleControler<CardReader>, public Thread
                 , public ISyncMessageReceiver<void>
{
public:
  explicit CardReader(CardReaderListener * listener);
  class Association
  {
  public:
    Association(const String& game, const String& system_uuid) :
      game(game),
      systemUUID(system_uuid)
    {
    }

    String game;
    String systemUUID;
    static const Association Empty;
  };
  static const String VideoPath;

  void Run() final;
  [[nodiscard]] bool IsAvailable();
  [[nodiscard]] bool IsCardPlugged();
  [[nodiscard]] String GetCardId();
  [[nodiscard]] String GetCardUUIDString();

  [[nodiscard]] bool ShouldBootOnGame();

  [[nodiscard]] String CurrentCardRomPath();
  [[nodiscard]] String CurrentCardSystemUUID();

  void DeleteGameForCard();
  void SetGameForCard(const String& string, const String& guid);
  String GetCardReaderVersion();
  String GetModuleVersion();
  bool ConsoleMode();
  void ReceiveSyncMessage() final;
  Colors::ColorRGBA ConsoleModeBackgroundColor();
  Path ConsoleModeVideo();
  Path ConsoleModeSound();

private:
  Mutex mMutex;
  CardReaderListener * mListener;
  bool updating;
  bool mLastAvailable;
  bool mLastCardPlugged;
  SyncMessageSender<void> mSender;
  String systemUUID;
  String romPath;

  Association CurrentAssociation();

};
