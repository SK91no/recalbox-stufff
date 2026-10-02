#include <hardware/Board.h>
#include <hardware/boards/odroidadvancego2/OdroidAdvanceGo2Board.h>
#include <hardware/boards/anbernic/RG351VBoard.h>
#include <hardware/boards/anbernic/RG353XBoard.h>
#include <sys/utsname.h>
#include <hardware/boards/NullBoard.h>
#include <hardware/boards/pc/PcComputers.h>
#include <hardware/boards/pc/SteamDeck.h>
#include <hardware/boards/pis/PiBoard.h>
#include "hardware/crt/CrtAdapterDetector.h"
#include "hardware/crt/CrtNull.h"
#include "utils/Files.h"
#include "hardware/crt/CrtRGBDual.h"
#include <hardware/boards/pi400/Pi400Board.h>
#include <hardware/Case.h>
#include <hardware/boards/pi500/Pi500Board.h>
#include <hardware/JvsCab.h>

Board::Board(IHardwareNotifications& notificationInterface, const Options& options)
  : StaticLifeCycleControler("Board")
  , mOptions(options)
  , mType(BoardType::UndetectedYet)
  , mMemory(0)
  , mSender(notificationInterface)
  , mBoard(*GetBoardInterface(mSender))
  , mCrtBoard(GetCrtBoard())
{
  if(mCrtBoard.GetCrtAdapter() != CrtAdapterType::None)
    { LOG(LogInfo) << "[CRT] Detected CRT Adapter: " << mCrtBoard.Name(); }
  JvsCab::LogDetection();
}

IBoardInterface* Board::GetBoardInterface(HardwareMessageSender& messageSender)
{
  BoardType model = GetBoardType();
  switch(model)
  {
    case BoardType::RG351V: // might need adaptation!
    {
      { LOG(LogInfo) << "[Hardware] Anbernic RG351V."; }
      return new RG351VBoard(messageSender, model);
    }
    case BoardType::OdroidAdvanceGo:
    {
      { LOG(LogInfo) << "[Hardware] Odroid Advance Go 1/2 detected."; }
      return new OdroidAdvanceGo2Board(messageSender, model);
    }
    case BoardType::OdroidAdvanceGoSuper:
    {
      { LOG(LogInfo) << "[Hardware] Odroid Advance Go Super detected."; }
      return new OdroidAdvanceGo2Board(messageSender, model);
    }
    case BoardType::RG351P:
    {
      { LOG(LogInfo) << "[Hardware] Anbernic RG351P/M detected."; }
      return new OdroidAdvanceGo2Board(messageSender, model);
    }
    case BoardType::RG353P:
    case BoardType::RG353V:
    case BoardType::RG353M:
    case BoardType::RG503:
    {
      { LOG(LogInfo) << "[Hardware] Anbernic RG353x."; }
      return new RG353XBoard(messageSender, model);
    }
    case BoardType::SteamDeckLCD:
    case BoardType::SteamDeckOLED:
    {
      { LOG(LogInfo) << "[Hardware] SteamDeck detected."; }
      return new SteamDeck(messageSender, model);
    }
    case BoardType::PCx86:
    case BoardType::PCx64:
    {
      { LOG(LogInfo) << "[Hardware] x86 or x64 PC detected."; }
      return new PcComputers(messageSender);
    }
    case BoardType::UndetectedYet: { LOG(LogInfo) << "[Hardware] Undetected hardware."; break; }
    case BoardType::Unknown: { LOG(LogInfo) << "[Hardware] Unknown hardware."; break; }
    case BoardType::Pi0: { LOG(LogInfo) << "[Hardware] Raspberry Pi Zero detected."; return new PiBoard(messageSender, BoardType::Pi0); }
    case BoardType::Pi02: { LOG(LogInfo) << "[Hardware] Raspberry Pi Zero 2 detected."; return new PiBoard(messageSender,BoardType::Pi02); }
    case BoardType::Pi1: { LOG(LogInfo) << "[Hardware] Raspberry Pi 1 detected."; return new PiBoard(messageSender, BoardType::Pi1); }
    case BoardType::Pi2: { LOG(LogInfo) << "[Hardware] Raspberry Pi 2 detected."; return new PiBoard(messageSender, BoardType::Pi2); }
    case BoardType::Pi3: { LOG(LogInfo) << "[Hardware] Raspberry Pi 3 detected."; return new PiBoard(messageSender, BoardType::Pi3); }
    case BoardType::Pi3plus: { LOG(LogInfo) << "[Hardware] Raspberry Pi 3B+ detected."; return new PiBoard(messageSender, BoardType::Pi3plus); }
    case BoardType::Pi4: { LOG(LogInfo) << "[Hardware] Raspberry Pi 4 detected."; return new PiBoard(messageSender, BoardType::Pi4); }
    case BoardType::Pi5: { LOG(LogInfo) << "[Hardware] Raspberry Pi 5 detected."; return new PiBoard(messageSender, BoardType::Pi5); }
    case BoardType::Pi400:
    {
      { LOG(LogInfo) << "[Hardware] Pi 400 detected."; }
      return new Pi400Board(messageSender);
    }
    case BoardType::Pi500:
    {
      { LOG(LogInfo) << "[Hardware] Pi 500 detected."; }
      return new Pi500Board(messageSender);
    }
    case BoardType::PiUnknown: { LOG(LogInfo) << "[Hardware] Unknown raspberry pi detected."; break; }
    case BoardType::OdroidUnknown: { LOG(LogInfo) << "[Hardware] Unknown odroid detected."; break; }
    case BoardType::AnbernicUnknown: { LOG(LogInfo) << "[Hardware] Unknown an,bernic detected."; break; }
    default: break;
  }

  { LOG(LogInfo) << "[Hardware] Default hardware interface created."; }
  return new NullBoard(messageSender);
}

int Board::GetPiMemory(unsigned int revision)
{
  int  memorySize     = ((int)revision >> 20) & 0x7;

  switch(memorySize)
  {
    case 0: return 256;
    case 1: return 512;
    case 2: return 1024;
    case 3: return 2048;
    case 4: return 4096;
    case 5: return 8192;
    case 6: return 16384;
    default: break;
  }

  return 8192;
}

BoardType Board::GetPiModel(unsigned int revision)
{
  // Split - uuuuuuuuFMMMCCCCPPPPTTTTTTTTRRRR
  bool newGeneration  = ((revision >> 23) & 1) != 0;
  int  memorySize     = ((int)revision >> 20) & 0x7; (void)memorySize;
  int  manufacturer   = ((int)revision >> 16) & 0xF; (void)manufacturer;
  int  processor      = ((int)revision >> 12) & 0xF; (void)processor;
  int  model          = ((int)revision >>  4) & 0xFF;
  int  revisionNumber = ((int)revision >>  0) & 0xF; (void)revisionNumber;

  // Old revision numbering
  if (!newGeneration)
    return BoardType::Pi1;

  // New models
  switch ((RaspberryModel)model)
  {
    case RaspberryModel::Zero:
    case RaspberryModel::ZeroW: return BoardType::Pi0;
    case RaspberryModel::Zero2: return BoardType::Pi02;
    case RaspberryModel::OneA:
    case RaspberryModel::OneAPlus:
    case RaspberryModel::OneB:
    case RaspberryModel::OneBPlus:
    case RaspberryModel::OneCM1: return BoardType::Pi1;
    case RaspberryModel::TwoB: return BoardType::Pi2;
    case RaspberryModel::TreeB:
    case RaspberryModel::TreeCM3: return BoardType::Pi3;
    case RaspberryModel::TreeBPlus:
    case RaspberryModel::TreeCM3Plus:
    case RaspberryModel::TreeAPlus: return BoardType::Pi3plus;
    case RaspberryModel::FourB:
    case RaspberryModel::FourCM4: return BoardType::Pi4;
    case RaspberryModel::FourHundred: return BoardType::Pi400;
    case RaspberryModel::FiveB:
    case RaspberryModel::FiveCM5Lite:
    case RaspberryModel::FiveCM5: return BoardType::Pi5;
    case RaspberryModel::FiveHundred: return BoardType::Pi500;
    case RaspberryModel::Alpha:
    default: break;
  }

  return BoardType::PiUnknown;
}


#define REVISION_STRING "Revision"
#define MODEL_STRING "Model"
#define HARDWARE_STRING "Hardware"

#define SizeLitteral(x) (sizeof(x) - 1)

int Board::TotalMemory()
{
  if (mType == BoardType::UndetectedYet) GetBoardType();
  return mMemory;
}

BoardType Board::GetBoardType()
{
  String hardware = "";
  String revision;
  String model;
  if (mType != BoardType::UndetectedYet) return mType;

  // Try uname (for PC)
  utsname uName {};
  memset(&uName, 0, sizeof(uName));
  uname(&uName);
  String machine(uName.machine);
  { LOG(LogDebug) << "[Hardware] Machine identifier: '" << machine << '\''; }
  if (machine == "i386") return mType = BoardType::PCx86;
  if (machine == "i686") return mType = BoardType::PCx86;
  if (machine == "x86_64")
  {
    hardware = Files::LoadFile(Path("/sys/devices/virtual/dmi/id/board_name")).Trim();
    if (hardware == "Jupiter") { LOG(LogInfo) << "[Hardware] SteamDeck LCD"; return mType = BoardType::SteamDeckLCD; }
    if (hardware == "Galileo") { LOG(LogInfo) << "[Hardware] SteamDeck OLED"; return mType = BoardType::SteamDeckOLED; }
    { LOG(LogInfo) << "[Hardware] PC x86_64"; return mType = BoardType::PCx64; }
  }
  // Then try CPU info
  mType = BoardType::Unknown;

  // Pre-identification data
  String left;
  String right;
  for(const String& line : Files::LoadAllFileLines(Path("/proc/cpuinfo"/* "/tmp/cpuinfo"*/)))
    if (line.Extract(':', left, right, true))
    {
      if (left == HARDWARE_STRING) hardware = right;
      if (left == MODEL_STRING) model = right;
      if (left == REVISION_STRING) revision = right;
    }

  // Finally read devicetree model
  if (hardware.empty())
    hardware = Files::LoadFile(Path("/sys/firmware/devicetree/base/model"));

  { LOG(LogInfo) << "[Hardware] " << hardware; }

  // Identification
  if      (hardware == "Anbernic RG351V") { LOG(LogInfo) << "[Hardware] Anbernic RG351V"; mType = BoardType::RG351V; }
  else if (hardware == "Anbernic RG351P") { LOG(LogInfo) << "[Hardware] Anbernic RG351P"; mType = BoardType::RG351P; }
  else if (hardware.Contains("RG353P"))   { LOG(LogInfo) << "[Hardware] Anbernic RG353P"; mType = BoardType::RG353P; }
  else if (hardware.Contains("RG353V"))   { LOG(LogInfo) << "[Hardware] Anbernic RG353V"; mType = BoardType::RG353V; }
  else if (hardware.Contains("RG353M"))   { LOG(LogInfo) << "[Hardware] Anbernic RG353M"; mType = BoardType::RG353M; }
  else if (hardware.Contains("RG503"))    { LOG(LogInfo) << "[Hardware] Anbernic RG503";  mType = BoardType::RG503; }
  else if (hardware == "Hardkernel ODROID-GO3") { LOG(LogInfo) << "[Hardware] Odroid Advance Go Super revision " << revision; mType = BoardType::OdroidAdvanceGoSuper; }
  else if ((hardware == "Hardkernel ODROID-GO2") || (hardware == "Hardkernel ODROID-GO1") || (hardware == "Hardkernel ODROID-GO"))
  {
    { LOG(LogInfo) << "[Hardware] Odroid Advance Go 1/2 revision " << revision; }
    mType = BoardType::OdroidAdvanceGo;
  }
  else
  {
    if (model.LowerCase().Contains("raspberry"))
      if (int irevision = 0; revision.Insert(0, '$').TryAsInt(irevision))
      {
        mType = GetPiModel(irevision);
        mMemory = GetPiMemory(irevision);
      }
  }

  return mType;
}

ICrtInterface& Board::GetCrtBoard()
{
  if (CanHaveCRTBoard())
    return *CrtAdapterDetector::CreateCrtBoard(GetBoardType(), mOptions);

  // Null board
  return *(new CrtNull(false, GetBoardType()));
}

bool Board::CanHaveCRTBoard()
{
  if (mOptions.EmulateRGBDual() || mOptions.EmulateRGBJamma()) return true;
  #ifdef OPTION_RECALBOX_SIMULATE_RRGBD
  return true;
  #else
  switch(GetBoardType())
  {
    case BoardType::Pi02:
    case BoardType::Pi3:
    case BoardType::Pi3plus:
    case BoardType::Pi4:
    case BoardType::Pi400:
    case BoardType::Pi5:
    case BoardType::Pi500: return true;
    case BoardType::Pi1:
    case BoardType::Pi2:
    case BoardType::UndetectedYet:
    case BoardType::Unknown:
    case BoardType::Pi0:
    case BoardType::PiUnknown:
    case BoardType::RG351V:
    case BoardType::RG351P:
    case BoardType::RG353M:
    case BoardType::RG353V:
    case BoardType::RG353P:
    case BoardType::RG503:
    case BoardType::AnbernicUnknown:
    case BoardType::OdroidAdvanceGo:
    case BoardType::OdroidAdvanceGoSuper:
    case BoardType::OdroidUnknown:
    case BoardType::PCx86:
    case BoardType::PCx64:
    case BoardType::SteamDeckLCD:
    case BoardType::SteamDeckOLED:
    default: break;
  }

  return false;
  #endif
}

String Board::GetBoardName()
{
  String arch = "unknown";
  switch(Board::Instance().GetBoardType())
  {
    case BoardType::UndetectedYet:        arch = "undetected"; break;
    case BoardType::Unknown:              arch = "unknown"; break;
    case BoardType::Pi0:                  arch = "RPi zero"; break;
    case BoardType::Pi02:                 arch = "RPi zero 2"; break;
    case BoardType::Pi1:                  arch = "RPi 1"; break;
    case BoardType::Pi2:                  arch = "RPi 2"; break;
    case BoardType::Pi3:                  arch = "RPi 3"; break;
    case BoardType::Pi3plus:              arch = "RPi 3B+"; break;
    case BoardType::Pi4:                  arch = "RPi 4"; break;
    case BoardType::Pi400:                arch = "RPi 400"; break;
    case BoardType::Pi5:                  arch = "RPi 5"; break;
    case BoardType::Pi500:                arch = "RPi 500"; break;
    case BoardType::PiUnknown:            arch = "unknown RPi"; break;
    case BoardType::OdroidAdvanceGo:      arch = "Odroid GO Advance"; break;
    case BoardType::OdroidAdvanceGoSuper: arch = "Odroid GO Super"; break;
    case BoardType::OdroidUnknown:        arch = "unknown Odroid"; break;
    case BoardType::SteamDeckLCD:
    case BoardType::SteamDeckOLED:        arch = "Steam Deck"; break;
    case BoardType::PCx86:                arch = "PC x86"; break;
    case BoardType::PCx64:                arch = "PC x64"; break;
    case BoardType::RG351P:               arch = "RG351P/M"; break;
    case BoardType::RG353P:               arch = "RG353P"; break;
    case BoardType::RG353V:               arch = "RG353V"; break;
    case BoardType::RG353M:               arch = "RG353M"; break;
    case BoardType::RG503:                arch = "RG503"; break;
    case BoardType::AnbernicUnknown:      arch = "unknown Anbernic"; break;
    case BoardType::RG351V:               arch = "RG351V"; break;
  }
  return arch;
}

int Board::GetDataBusSize()
{
  #if INTPTR_MAX == INT32_MAX
  return 32;
  #elif INTPTR_MAX == INT64_MAX
  return 64;
  #else
    #error "Environment not 32 or 64-bit."
  #endif
}

bool Board::IsHandheldSystem()
{
  switch(Board::Instance().GetBoardType())
  {
    case BoardType::SteamDeckLCD:
    case BoardType::SteamDeckOLED:
    case BoardType::OdroidAdvanceGo:
    case BoardType::OdroidAdvanceGoSuper:
    case BoardType::OdroidUnknown:
    case BoardType::RG351P:
    case BoardType::RG353P:
    case BoardType::RG353V:
    case BoardType::RG353M:
    case BoardType::RG503:
    case BoardType::RG351V:
    case BoardType::AnbernicUnknown: return true;
    case BoardType::UndetectedYet:
    case BoardType::Unknown:
    case BoardType::Pi0:
    case BoardType::Pi02:
    case BoardType::Pi1:
    case BoardType::Pi2:
    case BoardType::Pi3:
    case BoardType::Pi3plus:
    case BoardType::Pi4:
    case BoardType::Pi400:
    case BoardType::Pi5:
    case BoardType::Pi500:
    case BoardType::PiUnknown:
    case BoardType::PCx86:
    case BoardType::PCx64:
    default: break;
  }
  switch(Case::CurrentCase().Model())
  {
    case Case::CaseModel::SteamDeckLCD:
    case Case::CaseModel::SteamDeckOLED:
    case Case::CaseModel::PiBoyDMG:
    case Case::CaseModel::PiBoyXRS:
    case Case::CaseModel::GPiV1:
    case Case::CaseModel::GPiV2:
    case Case::CaseModel::GPiV3:
    case Case::CaseModel::GPi2:
    case Case::CaseModel::GPi2W: return true;
    case Case::CaseModel::Nuxii:
    case Case::CaseModel::Nespi4Case:
    case Case::CaseModel::Nespi4CaseManual:
    case Case::CaseModel::SuperPi4Case:
    case Case::CaseModel::NespiCasePlus:
    case Case::CaseModel::PiStation:
    case Case::CaseModel::SuperPiCase:
    case Case::CaseModel::MegaPiCase:
    case Case::CaseModel::ArgonOne:
    case Case::CaseModel::ArgonOneV3:
    case Case::CaseModel::RaspberryPiTouchDisplay:
    case Case::CaseModel::RecalboxRGBDualOrRGBHat:
    case Case::CaseModel::SixtyFourPiCase:
    case Case::CaseModel::DreamCase:
    case Case::CaseModel::None:
    default: break;
  }
  return false;
}
