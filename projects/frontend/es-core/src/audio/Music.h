#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include <SDL2/SDL_mixer.h>

class ThemeData;

class Music
{
  private:
    //! Music path
    Path            mPath;
    //! SDL Music
    Mix_Music*      mMusic;
    //! Cached duration
    float   mCachedDuration;

    /*!
     * @brief Load music into SDL structures
     */
    void Initialize();
    /*!
     * @brief Free SDL structures
     */
    void Finalize();

    /*!
     * @brief Direct constructor
     * @param path Music filepath
     */
    explicit Music(const Path & path);

    /*!
     * @brief Music player callback when amusic ends
     */
    static void MusicEndCallback();

  public:
    /*!
     * @brief Destructor
     */
    ~Music();

    /*!
     * @brief Play current music
     * @param repeat True to play the current music in loop
     * @return True if the music started successfuly
     */
  	bool Play(bool repeat);

		//! Pause current music
    static void Pause();

		//! Resume current music
    static void Resume();

    /*!
  	 * @brief Stop current music if it's playing
  	 */
  	static void Stop();

  	/*!
  	 * @brief Return music name
  	 * @return Music name
  	 */
  	[[nodiscard]] String Name() const { return mPath.FilenameWithoutExtension(); }

    /*!
     * @brief Return music path
     * @return Music path
     */
    [[nodiscard]] const Path& FilePath() const { return mPath; }

		/*!
		 * @brief Return music duration
		 * @return Music duration in seconds
		 */
		[[nodiscard]] float Duration() const { return mCachedDuration; }

		/*!
		 * @brief Return music position
		 * @return Music position in seconds
		 */
		[[nodiscard]] float Position() const { return (float)Mix_GetMusicPosition(mMusic); }

		/*!
		 * @brief Set music position
		 * @param position Position in seconds
		 */
		void SetPosition(float position) const { Mix_SetMusicPosition((double)position); }

    /*!
  	 * @brief Build a new music file
     * @param path Music filepath
  	 * @return New music instance or null if the file does not exist
  	 */
  	static Music* LoadFromPath(const Path & path);
};

#endif
