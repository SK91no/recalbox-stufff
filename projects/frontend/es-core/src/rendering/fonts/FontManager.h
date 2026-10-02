//
// Created by bkg2k on 10/12/2020.
//
#pragma once

#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <utils/storage/HashMap.h>
#include <rendering/fonts/Font.h>
#include <rendering/fonts/IFontFaceProvider.h>
#include "FontTemplate.h"
#include <themes/ThemeElement.h>
#include <themes/PropertyCategories.h>

class FontManager : public StaticLifeCycleControler<FontManager>
                  , public IFontFaceProvider
{
  public:
    //! Constructor
    FontManager();

    //! Destructor
    virtual ~FontManager();

    //! Get the default font at the default size
    NewFont& Default() { return FromFile(Path(), NewFont::sDefaultFontSize, false); }

    //! Get the default font at the specified size
    NewFont& FromDefault(float size, bool sizeInPixels) { return FromFile(Path(), size, sizeInPixels); }

    //! Get font from the specified file or embedded resource at the given size
    NewFont& FromFile(const Path& fineFile, float size, bool sizeInPixels);

    //! Get font from the same file as an existing font instance, at the given size
    NewFont& FromFont(const NewFont& from, float size, bool sizeInPixels);

    //! Get default font
    //NewFont& FromDefault(const NewFont& sizeTemplate);

    //! Get font from specified file or resource
    //NewFont& FromFile(const Path& fineFile, const NewFont& sizeTemplate);

    //! Get font from the specified template, scaled by the given factor
    NewFont& FromTemplate(const Path& fontFile, FontTemplate font, float factor);

    //! Get font from the specified template at its nominal size
    NewFont& FromTemplate(const Path& fontFile, FontTemplate font) { return FromTemplate(fontFile, font, 1.f); }

    //! Get font from the specified template using the default font file at its nominal size
    NewFont& FromTemplate(FontTemplate font) { return FromTemplate(Path::Empty, font, 1.f); }

    //! Get font from the specified template proportionally resized to match the given base size
    NewFont& FromTemplateResized(const Path& fontFile, FontTemplate font, float factor);

    //! True when the system locale is an Arabic-script locale (ar/fa/ur)
    [[nodiscard]] bool IsArabicLocale() const { return mArabicLocale; }

    //! Clear all fonts' internal glyph caches
    void ClearFontCaches();

    /*!
     * @brief Build new font from theme properties & optional original font
     * @param elem Theme element
     * @param properties Theme property category
     * @param orig Original font
     * @return New font
     */
    NewFont& FromTheme(const ThemeElement& elem, ThemePropertyCategory properties, NewFont* orig);

  private:
    //! Internal font w/ icons
    static constexpr const char* sInternalFont = ":/Recalbox_icons.ttf";
    //! Default fixed font
    static constexpr const char* sFixedFont = ":/UbuntuMonoR.ttf";

    //! Fonts : Identifier/Font instance
    HashMap<String, NewFont*> mFontCollection;
    //! Fonts faces Identifier/Font face
    HashMap<String, FT_Face> mFaceCollection;
    //! Font data Path/Data
    HashMap<String, String> mFaceData;

    //! Free type library handler
    FT_Library mLibrary;
    //! True when the system locale is an Arabic-script locale (ar, fa, ur).
    //! Immutable after construction; a locale change requires a frontend reboot.
    bool mArabicLocale;

    /*!
     * @brief Build a unique cache key for the given font file and pixel size
     * @param fontFile Font file path (empty for the default font)
     * @param heightInPixel Font height in pixels
     * @return Unique string identifier
     */
    static String FormatIdentifier(const Path& fontFile, float heightInPixel);

    /*
     * IFontFaceProvider implementation
     */

    /*!
     * @brief Get the default font face at the given pixel size
     * @param heightInPixel Font height in pixels
     * @return Default font face — never nullptr
     */
    FT_Face GetDefaultFontFace(float heightInPixel) override { return GetFontFace(Path(), heightInPixel); }

    /*!
     * @brief Get a font face for the given file and pixel size, loading it if necessary
     * @param path Font file path (empty for the default font)
     * @param heightInPixel Font height in pixels
     * @return Font face, or nullptr if the file could not be loaded
     */
    FT_Face GetFontFace(const Path& path, float heightInPixel) override;
};
