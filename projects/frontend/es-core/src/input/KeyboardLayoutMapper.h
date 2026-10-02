//
// Created by Pit64 on 11/12/2025.
//
#pragma once

#include <SDL2/SDL.h>
#include <utils/String.h>
#include <utils/storage/HashMap.h>

/*!
 * @brief Keyboard layout mapper for handling different keyboard layouts (AZERTY, QWERTZ, etc.)
 *
 * This class provides generic keyboard layout mapping capabilities to support
 * international keyboards. It maps physical key scancodes to logical keycodes
 * and remaps text input characters based on the configured layout.
 */
class KeyboardLayoutMapper
{
public:
  /*!
   * @brief Initialize the mapper with a specific keyboard layout
   * @param layout Layout identifier (e.g., "fr", "de", "uk", "us")
   */
  explicit KeyboardLayoutMapper(const String& layout);

  /*!
   * @brief Map a scancode to the appropriate keycode for the current layout
   * @param scancode Physical key scancode
   * @return Logical keycode for the configured layout
   */
  SDL_Keycode MapScancode(SDL_Scancode scancode) const;

  /*!
   * @brief Remap text input characters for the current layout
   * @param text Input text (assumed to be US QWERTY)
   * @param altGrPressed Whether the AltGr (Right Alt) key is pressed
   * @return Remapped text for the actual layout
   */
  String RemapText(const char* text, bool altGrPressed) const;

  /*!
   * @brief Get the current keyboard layout identifier
   * @return Layout identifier string
   */
  const String& GetLayout() const { return mLayout; }

  /*!
   * @brief Check if using native SDL mapping (X11/Wayland)
   * Detects the video driver lazily at call time to ensure video is initialized
   * @return True if using native SDL, false if custom remapping is needed
   */
  bool IsUsingNativeSDL() const;

private:
  //! Current keyboard layout
  String mLayout;

  //! Scancode to keycode mapping for the current layout
  HashMap<SDL_Scancode, SDL_Keycode> mScancodeMap;

  //! Character remapping for text input
  HashMap<unsigned int, const char*> mCharMap;

  //! Character remapping for text input with AltGr pressed
  HashMap<unsigned int, const char*> mAltGrCharMap;

  /*!
   * @brief Initialize the layout-specific mappings
   */
  void InitializeMappings();

  /*!
   * @brief Add French AZERTY layout mappings
   */
  void AddFrenchLayout();

  /*!
   * @brief Add German QWERTZ layout mappings
   */
  void AddGermanLayout();

  /*!
   * @brief Add Spanish layout mappings
   */
  void AddSpanishLayout();

  /*!
   * @brief Add Italian layout mappings
   */
  void AddItalianLayout();

  /*!
   * @brief Add Danish layout mappings
   * */
  void AddDanishLayout();

  /*!
   * @brief Add UK QWERTY layout mappings
   */
  void AddUKLayout();
};