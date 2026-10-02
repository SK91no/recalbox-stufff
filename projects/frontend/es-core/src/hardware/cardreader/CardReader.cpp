//
// Created by digitallumberjack on 26/10/25.
//

#include "CardReader.h"

#include "RecalboxConf.h"
#include "emulators/run/GameRunner.h"
#include "utils/Files.h"
#include "utils/Log.h"

#define FILE_DIR "/sys/kernel/recalbox-card-reader/"
//#define FILE_DIR "/recalbox/share/system/recalbox-card-reader/"

#define FILE_AVAILABLE FILE_DIR"available"
#define FILE_PLUGGED FILE_DIR"plugged"
#define FILE_ID FILE_DIR"uuid"
#define ASSOCIATION FILE_DIR"association"
#define RESET_CARD FILE_DIR"reset_card"
#define FILE_CRVERSION FILE_DIR"firmware_version"
#define FILE_MODVERSION FILE_DIR"module_version"

const String CardReader::VideoPath = "/recalbox/system/resources/cardreader/";

const CardReader::Association CardReader::Association::Empty = Association("","");

CardReader::CardReader(CardReaderListener* listener) :
    StaticLifeCycleControler<CardReader>("CardReader"),
    mListener(listener), updating(false), mLastAvailable(IsAvailable()), mLastCardPlugged(IsCardPlugged()), mSender(*this)
{

  if (Path(FILE_DIR).IsDirectory())
  {
    const Association asso = CurrentAssociation();
    systemUUID = asso.systemUUID;
    romPath = asso.game;
    {
      const RecalboxConf* conf = &RecalboxConf::Instance();
      LOG(LogInfo) << "[CardReader] CardReader available("<< mLastAvailable << "), firmware version(" << GetCardReaderVersion() << "), plugged("
      << (IsCardPlugged() ? "true" : "false") << "), card uuid(" << GetCardUUIDString()
      << "), card association(" << systemUUID << "||" << romPath
      << "), will boot on game(" << (ShouldBootOnGame() ? "true" : "false")
      << "), ConsoleMode(" << (conf->GetCRConsoleMode() ? "true" : "false")
      << "), ConsoleModeExit(" << conf->GetCRConsoleModeExit()
      << "), ConsoleModeVideo(" << conf->GetCRConsoleModeVideo()
      << "), ConsoleModeSound(" << conf->GetCRConsoleModeSound()
      << "), ";
    }
    Start("cardReader");
  }
  else LOG(LogInfo) << "[CardReader] CardReader not installed, not starting thread";
}

void CardReader::Run()
{
  while (IsRunning())
  {
    Sleep(1000);
    if (!IsAvailable())
    {
      if (mLastAvailable)
      {
        { LOG(LogInfo) << "[CardReader] CardReader available no more"; }
        mLastAvailable = false;
        mLastCardPlugged = false;
        if (GameRunner::IsGameRunning())
        {
          { LOG(LogDebug) << "[CardReader] Game is running, calling CardUnplugged"; }
          mListener->CardUnplugged();
        }
        else
        {
          { LOG(LogDebug) << "[CardReader] Game is not running, calling async"; }
          mSender.Send();
        }
      }
    }
    else
    {
      if (!mLastAvailable)
      {
        { LOG(LogInfo) << "[CardReader] CardReader available again"; }
        mLastAvailable = true;
      }
      bool cardPlugged = IsCardPlugged();
      if (mLastCardPlugged && !cardPlugged)
      {
        { LOG(LogInfo) << "[CardReader] A card was unplugged"; }
        mLastCardPlugged = false;
        if (GameRunner::IsGameRunning())
        {
          { LOG(LogDebug) << "[CardReader] Game is running, calling CardUnplugged"; }
          mListener->CardUnplugged();
        }
        else
        {
          { LOG(LogDebug) << "[CardReader] Game is not running, calling async"; }
          mSender.Send();
        }
      }
      else if (!mLastCardPlugged && cardPlugged)
      {
        systemUUID = CurrentCardSystemUUID();
        romPath = CurrentCardRomPath();
        mLastCardPlugged = true;
        { LOG(LogInfo) << "[CardReader] A card was plugged : system(" << systemUUID << "), game(" << romPath << ")" ; }
        if (GameRunner::IsGameRunning())
        {
          { LOG(LogDebug) << "[CardReader] Game is running, calling CardPlugged"; }
          mListener->CardPlugged(romPath, systemUUID);
        }
        else
        {
          { LOG(LogDebug) << "[CardReader] Game is not running, calling async"; }
          mSender.Send();
        }
      }
    }
  }
}

void CardReader::ReceiveSyncMessage()
{
  { LOG(LogInfo) << "[CardReader] Async here, I got : available(" << mLastAvailable << "), plugged(" << mLastCardPlugged << "), romPath(" << romPath << "), system(" << systemUUID << ")" ; }
  if (mLastAvailable && mLastCardPlugged)
    mListener->CardPlugged(romPath, systemUUID);
  else
    mListener->CardUnplugged();
}

bool CardReader::IsAvailable()
{
  mMutex.Lock();
  String available = Files::LoadFile(Path(FILE_AVAILABLE));
  updating = available == "updating";
  mMutex.UnLock();
  return updating || available == "true";
}

bool CardReader::IsCardPlugged()
{
  bool res = IsAvailable();
  mMutex.Lock();
  res = res && Path(FILE_PLUGGED).Exists() && Files::LoadFile(Path(FILE_PLUGGED)) == "true";
  mMutex.UnLock();
  return res;
}

String CardReader::GetCardId()
{
  mMutex.Lock();
  String res = Files::LoadFile(Path(FILE_ID));
  mMutex.UnLock();
  return res;
}

String CardReader::GetCardUUIDString()
{
  mMutex.Lock();
  String uuid = Files::LoadFile(Path(FILE_ID));
  mMutex.UnLock();
  if (uuid.empty() || uuid == "0")
    uuid = "";
  return uuid;
}

CardReader::Association CardReader::CurrentAssociation()
{
  if (!IsCardPlugged() || GetCardUUIDString().empty())
    return Association::Empty;
  mMutex.Lock();
  String gameAndSystem = Files::LoadFile(Path(ASSOCIATION));
  mMutex.UnLock();
  String::List pathAndSystemUUID = gameAndSystem.Split("||");
  if (gameAndSystem.StartsWith("none") || pathAndSystemUUID.size() != 2)
    return Association::Empty;
  return Association(pathAndSystemUUID[1], pathAndSystemUUID[0]);
}

String CardReader::CurrentCardRomPath()
{
  return CurrentAssociation().game;
}

String CardReader::CurrentCardSystemUUID()
{
  return CurrentAssociation().systemUUID;
}

void CardReader::DeleteGameForCard()
{
  if (!IsCardPlugged() || GetCardUUIDString().empty())
    return;
  mMutex.Lock();
  Files::WriteToFile(Path(RESET_CARD), "reset", 0);
  mMutex.UnLock();
}

void CardReader::SetGameForCard(const String& gamePath, const String& systemUUID)
{
  if (!IsCardPlugged() || GetCardUUIDString().empty())
    return;
  String boot(systemUUID);
  boot.Append("||").Append(gamePath);
  { LOG(LogDebug) << "[CardReader] Associate " << boot; }
  mMutex.Lock();
  Files::WriteToFile(Path(ASSOCIATION), boot, 0);
  mMutex.UnLock();
}

bool CardReader::ShouldBootOnGame()
{
  Association association = CurrentAssociation();
  return !association.game.empty() && !association.systemUUID.empty();
}

String CardReader::GetCardReaderVersion()
{
  mMutex.Lock();
  String version = Files::LoadFile(Path(FILE_CRVERSION));
  mMutex.UnLock();
  return version;
}

String CardReader::GetModuleVersion()
{
  mMutex.Lock();
  String version = Files::LoadFile(Path(FILE_MODVERSION));
  mMutex.UnLock();
  return version;
}

bool CardReader::ConsoleMode()
{
  return RecalboxConf::Instance().GetCRConsoleMode() && IsAvailable();
}

Colors::ColorRGBA CardReader::ConsoleModeBackgroundColor()
{
  return 0x210F65ff;
}

Path CardReader::ConsoleModeVideo()
{
  Path path = Path(String(CardReader::VideoPath).Append(RecalboxConf::Instance().GetCRConsoleModeVideo()).Append(".mp4"));
  if (!path.Exists())
    path = Path(String(CardReader::VideoPath).Append(RecalboxConf::Instance().GetCRConsoleModeVideoDefault()).Append(".mp4"));
  return path;
}

Path CardReader::ConsoleModeSound()
{
  if (RecalboxConf::Instance().GetCRConsoleModeSound() == "bios")
  {
    { LOG(LogDebug) << "[CardReaderConsoleView] Starting Bios sound"; }
    Path path = Path(String(CardReader::VideoPath).Append(RecalboxConf::Instance().GetCRConsoleModeVideo()).Append(".wav"));
    if (!path.Exists())
      path = Path(String(CardReader::VideoPath).Append(RecalboxConf::Instance().GetCRConsoleModeVideo()).Append(".mp3"));
    if (!path.Exists())
      path = Path(String(CardReader::VideoPath).Append(RecalboxConf::Instance().GetCRConsoleModeVideoDefault()).Append(".mp3"));
    if(!path.Exists())
      return Path::Empty;
    return path;
  }
  return Path::Empty;
}