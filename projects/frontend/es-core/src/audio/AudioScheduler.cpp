#include "AudioScheduler.h"
#include "music/RemotePlaylist.h"
#include "patreon/PatronInfo.h"
#include "themes/ThemeManager.h"

#include <SDL.h>
#include <views/SystemView.h>
#include <utils/locale/LocaleHelper.h>
#include <guis/GuiInfoPopup.h>
#include <themes/ThemeData.h>
#include <audio/AudioController.h>

AudioScheduler::AudioScheduler(WindowManager& window)
  : StaticLifeCycleControler<AudioScheduler>("AudioScheduler")
  , mCurrentAudioMode(RecalboxConf::Instance().GetAudioMode())
  , mJukeboxInterface(nullptr)
  , mWindow(window)
  , mMusicPlaying(nullptr)
  , mCurrentMusicSource(MusicSource::None)
  , mSender(*this)
  , mRandomGenerator(mRandomDevice())
  , mTheme(nullptr)
  , mPopupAllowed(true)
{
  Initialize();
}

AudioScheduler::~AudioScheduler()
{
  // Finalize SDL
  Finalize();
}

void AudioScheduler::ClearCaches()
{
  // Delete all sounds
  for(auto& sound : mSoundMap)
    delete sound.second;
  mSoundMap.clear();

  // Free the current music (the only one kept alive)
  delete mMusicPlaying;
  mMusicPlaying = nullptr;
}

void AudioScheduler::Initialize()
{
  ClearCaches();
  mCurrentMusicSource = MusicSource::None;

  // Open the audio device and pause
  #ifndef OPTION_RECALBOX_NO_AUDIO
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
  #endif
  {
    { LOG(LogError) << "[AudioScheduler] MUSIC Error - Unable to open SDLMixer audio: " << SDL_GetError(); }
    return;
  }

  { LOG(LogInfo) << "[AudioScheduler] SDL AUDIO Initialized"; }
}

void AudioScheduler::Finalize()
{
  // Completely tear down SDL audio. else SDL hogs audio resources and emulators might fail to start...
  { LOG(LogInfo) << "[AudioScheduler] Shutting down SDL AUDIO"; }
  #ifndef OPTION_RECALBOX_NO_AUDIO
  Mix_HookMusicFinished(nullptr);
  Mix_HaltMusic();
  Mix_CloseAudio();
  #endif

  // Free musics/sounds
  ClearCaches();
}

void AudioScheduler::Reactivate()
{
  Initialize();
  PlayNextMusic();
}

AudioScheduler::AudioHandle AudioScheduler::LoadSound(const Path& path)
{
  // Get handle
  AudioHandle handle = path.ToString().Hash64() | 1;

  // Already exists
  if (mSoundMap.contains(handle))
    return handle;

  // Try to load
  Sound* sound = Sound::BuildFromPath(path);
  if (sound == nullptr) return 0; // Not found
  // Add and return the handle
  mSoundMap[handle] = sound;
  return handle;
}

bool AudioScheduler::PlaySound(AudioScheduler::AudioHandle handle)
{
  auto it = mSoundMap.find(handle);
  if (it != mSoundMap.end())
  {
    StopAll();
    it->second->Play();
    return true;
  }
  return false;
}

bool AudioScheduler::PlayMusicFile(const Path& path, bool loop)
{
  // Load on demand - no cache: load the new track first so a failed load doesn't stop the current one
  Music* music = Music::LoadFromPath(path);
  if (music == nullptr) return false;

  // Stop & free the previous music, then take ownership of the new one
  StopAll();
  mMusicPlaying = music;
  mCurrentMusicTitle = music->Name();

  music->Play(loop);

  if (mJukeboxInterface != nullptr)
    mJukeboxInterface->MusicStart(mCurrentMusicTitle);
  return true;
}

bool AudioScheduler::PlayRemoteTrack(const Path& path)
{
  if (!PlayMusicFile(path, false)) return false;
  mCurrentMusicSource = MusicSource::RemoteTrack;
  AudioController::Instance().SetMusicVolume(RecalboxConf::Instance().GetAudioMusicVolume());
  return true;
}

void AudioScheduler::PauseMusic()
{
  Music::Pause();
}

void AudioScheduler::ResumeMusic()
{
  Music::Resume();
}

void AudioScheduler::StopAll()
{
  Music::Stop();
  Sound::Stop();
  delete mMusicPlaying;
  mMusicPlaying = nullptr;
}

void AudioScheduler::PlayNextMusic()
{
  #ifdef OPTION_RECALBOX_NO_AUDIO
  return;
  #endif

  // Global playing capability
  if (!CanPlayMusic()) return;

  // Patron music takes absolute priority — never interrupt a playing remote track
  if (mCurrentMusicSource == MusicSource::RemoteTrack && IsPlaying())
    return;

  // Theme ?
  if (mTheme!= nullptr && (!PatronInfo::Instance().IsPatron() || !IsPlaying()))
  {
    const ThemeElement* elem = mTheme->Element("system", "directory", ThemeElementType::Sound, ThemeElementType::None);
    mThemeMusicFolder = ((elem == nullptr) || !elem->HasProperty(ThemePropertyName::Path)) ? Path::Empty : Path(elem->AsString(ThemePropertyName::Path));
    elem = mTheme->Element("system", "bgsound", ThemeElementType::Sound, ThemeElementType::None);
    mThemeMusic = ((elem == nullptr) || !elem->HasProperty(ThemePropertyName::Path)) ? Path::Empty : Path(elem->AsString(ThemePropertyName::Path));
  }

  Path previousPath = mMusicPlaying != nullptr ? mMusicPlaying->FilePath() : Path::Empty;
  Path musicToPlay;
  const char* log = "No music found.";
  MusicSource source = MusicSource::None;

  // Then check user folder
  Path musicPath = FetchRandomMusic(Path(sMusicFolder), previousPath);
  if (!musicPath.IsEmpty())
  {
    musicToPlay = musicPath;
    log = "user directory";
    source = MusicSource::User;
  }

  // Remote music?
  RemotePlaylist::TrackInfo* remoteTrack = nullptr;
  if (source == MusicSource::None)
  {
    if (RemotePlaylist::Available()) remoteTrack = RemotePlaylist::Instance().GetNextTrack();
    if (remoteTrack != nullptr)
    {
      musicToPlay = remoteTrack->LocalPath();
      log = "remote playlist";
      source = MusicSource::RemoteTrack;
    }
  }

  // check Theme music first
  if (source == MusicSource::None && !mThemeMusic.IsEmpty())
  {
    musicToPlay = mThemeMusic;
    log = "theme music (Background)";
    source = MusicSource::ThemeSystem;
  }

  // Finally check theme folder
  if (source == MusicSource::None && !mThemeMusicFolder.IsEmpty())
  {
    musicPath = FetchRandomMusic(mThemeMusicFolder, previousPath);
    if (!musicPath.IsEmpty())
    {
      musicToPlay = musicPath;
      log = "theme music (Theme folder)";
      source = MusicSource::Theme;
    }
  }

  if (source == MusicSource::None)
  {
    { LOG(LogError) << "[AudioScheduler] No music source!"; }
    return;
  }
  { LOG(LogInfo) << "[AudioScheduler] Will play music from:" << log; }

  // Do not relaunch currently playing song
  if (mMusicPlaying != nullptr && mMusicPlaying->FilePath() == musicToPlay)
    return;

  // Do not interrupt musics except for a theme system music
  if (mMusicPlaying != nullptr)
    if ((source != MusicSource::ThemeSystem) && (source == mCurrentMusicSource))
      return;

  // Play! (loads on demand, frees the previous track)
  if (!PlayMusicFile(musicToPlay, false))
    return;
  mCurrentMusicSource = source;

  // Confirm remote track consumption so the download loop advances its lookahead
  if (source == MusicSource::RemoteTrack)
    RemotePlaylist::Instance().TrackAccepted();

  // set music volume
  AudioController::Instance().SetMusicVolume(RecalboxConf::Instance().GetAudioMusicVolume());

  // Popup?
  int popupDuration = RecalboxConf::Instance().GetPopupMusic();
  if (popupDuration != 0 && mPopupAllowed)
  {
    if (popupDuration < 10) popupDuration = 10;
    // Create music popup
    if (source == MusicSource::RemoteTrack)
    {
      String text(_("Now playing"));
      text.Append(":\n").Append(remoteTrack->Name()).Append('\n')
          .Append('(').Append(remoteTrack->MixTape()).Append(_(" by ")).Append(remoteTrack->Artist()).Append(')');
      mWindow.InfoPopupAdd(new GuiInfoPopup(mWindow, text, popupDuration, PopupType::Music));
    }
    else
      mWindow.InfoPopupAdd(new GuiInfoPopup(mWindow, _("Now playing").Append(":\n") + mCurrentMusicTitle, popupDuration, PopupType::Music));
  }
}

std::vector<Path> AudioScheduler::ListMusicInFolder(const Path& path)
{
  std::vector<Path> musics;

  if (!path.IsDirectory()) return musics;

  Path::PathList list = path.RecurseDirectoryContent();
  for(Path& musicPath : list)
  {
    // Skip if not a file
    if (!musicPath.IsFile()) continue;

    // Skip if no match
    String ext = musicPath.Extension().LowerCase().Append('|');
    static String supportedExtensions = "|.wav|.mp3|.ogg|.flac|.midi|.mid|.mod|.s3m|.xm|.it|.669|.apun|.dsm|.far|.amf|.gdm|.imf|.med|.mtm|.okt|.stm|.stx|.ult|.uni|.opus|";
    if (!supportedExtensions.Contains(ext)) continue;

    // File matches, store it
    musics.push_back(musicPath);
  }

  return musics;
}

Path AudioScheduler::FetchRandomMusic(const Path& from, const Path& previousPath)
{
  // Get musics from folders
  std::vector<Path> musics = ListMusicInFolder(from);
  if (musics.empty())
    return Path::Empty;

  // Adjust randomizer distribution
  if (mSystemRandomizer.b() != (int)musics.size() - 1)
    mSystemRandomizer = std::uniform_int_distribution<int>(0, (int)musics.size() - 1);

  // Try to find a new music
  for(int i = sMaxTries; --i >= 0; )
  {
    int randomIndex = mSystemRandomizer(mRandomGenerator);
    if (!previousPath.IsEmpty() && (musics[randomIndex] == previousPath.ToString()))
      continue;
    return Path(musics[randomIndex]);
  }

  return Path::Empty;
}

void AudioScheduler::ReceiveSyncMessage()
{
  if (mCurrentMusicSource == MusicSource::RemoteTrack)
    RemotePlaylist::Instance().TrackConsumed();

  // The track ended on its own: free it (we own the only instance)
  delete mMusicPlaying;
  mMusicPlaying = nullptr;

  if (mJukeboxInterface != nullptr)
    mJukeboxInterface->MusicComplete();
  else if (CanPlayMusic())
    PlayNextMusic();
}

void AudioScheduler::PauseMusicIfNecessary()
{
  if (RecalboxConf::Instance().GetAudioMode() == AudioMode::MusicsXorVideosSound)
    PauseMusic();
}

void AudioScheduler::ResumeMusicIfNecessary()
{
  if (RecalboxConf::Instance().GetAudioMode() == AudioMode::MusicsXorVideosSound)
    ResumeMusic();
}

bool AudioScheduler::IsPlaying()
{
  return mMusicPlaying != nullptr;
}

float AudioScheduler::PlayingMusicTotalTime() const
{
  if (mMusicPlaying != nullptr) return mMusicPlaying->Duration();
  return 0;
}

float AudioScheduler::PlayingMusicPosition() const
{
  if (mMusicPlaying != nullptr) return mMusicPlaying->Position();
  return 0;
}

void AudioScheduler::SeekMusic(float position)
{
  if (mMusicPlaying != nullptr)
  {
    position = Math::clamp(position, 0, mMusicPlaying->Duration());
    mMusicPlaying->SetPosition(position);
  }
}

const String AudioScheduler::PlayingMusicName() const
{
  if (mMusicPlaying != nullptr) return mMusicPlaying->Name();
  return String::Empty;
}

const Path& AudioScheduler::PlayingMusicPath() const
{
  if (mMusicPlaying != nullptr) return mMusicPlaying->FilePath();
  return Path::Empty;
}

void AudioScheduler::ConfigurationAudioModeChanged(const AudioMode& audioMode)
{
  mCurrentAudioMode = audioMode;
  if (!IsPlaying() && CanPlayMusic()) PlayNextMusic();
  else if (IsPlaying() && !CanPlayMusic()) StopAll();
}

bool AudioScheduler::CanPlayMusic() const
{
  switch (mCurrentAudioMode)
  {
    case AudioMode::MusicsOnly:
    case AudioMode::MusicsAndVideosSound:
    case AudioMode::MusicsXorVideosSound: return true;
    case AudioMode::VideosSoundOnly:
    case AudioMode::None: break;
  }
  return false;
}

bool AudioScheduler::CanDecodeVideoSound() const
{
  switch (mCurrentAudioMode)
  {
    case AudioMode::VideosSoundOnly:
    case AudioMode::MusicsAndVideosSound:
    case AudioMode::MusicsXorVideosSound: return true;
    case AudioMode::MusicsOnly:
    case AudioMode::None: break;
  }
  return false;
}
