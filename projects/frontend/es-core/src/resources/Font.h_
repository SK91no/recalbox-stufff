#pragma once

#include <memory>
#include <map>
#include <Renderer.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include "themes/PropertyCategories.h"

#include FT_FREETYPE_H

class ThemeElement;
class ResourceManager;

#define MIN_DISPLAY_SIZE (Math::min(Renderer::Instance().DisplayHeightAsFloat(), Renderer::Instance().DisplayWidthAsFloat()))
#define FONT_SIZE_EXTRASMALL ((unsigned int) (0.030f * MIN_DISPLAY_SIZE))
#define FONT_SIZE_SMALL ((unsigned int) (0.035f  * MIN_DISPLAY_SIZE))
#define FONT_SIZE_MEDIUM ((unsigned int) (0.045f  * MIN_DISPLAY_SIZE))
#define FONT_SIZE_LARGE ((unsigned int) (0.085f  * MIN_DISPLAY_SIZE))


#define FONT_PATH_LIGHT ":/ubuntu_condensed.ttf"
#define FONT_PATH_REGULAR ":/ubuntu_condensed.ttf"
#define FONT_PATH_CRT ":/f8bitfortressplus.ttf"

typedef unsigned int UnicodeChar;

enum class TextAlignment : unsigned char
{
	Left,
	Center, // centers both horizontally and vertically
	Right,
	Top,
	Bottom
};

//A TrueType Font renderer that uses FreeType and OpenGL.
//The library is automatically initialized when it's needed.
class Font : public IReloadable
{
  private:
    static FT_Library sLibrary;
    static std::map< std::pair<Path, int>, std::weak_ptr<Font> > sFontMap;

    Font(int size, const Path& path);

    struct FontTexture
    {
      GLuint textureId;
      Vector2i textureSize;

      Vector2i writePos;
      int rowHeight;

      FontTexture();
      ~FontTexture();
      bool findEmpty(const Vector2i& size, Vector2i& cursor_out);

      // you must call initTexture() after creating a FontTexture to get a textureId
      void initTexture(); // initializes the OpenGL texture according to this FontTexture's settings, updating textureId
      void deinitTexture(); // deinitializes the OpenGL texture if any exists, is automatically called in the destructor
    };

    struct FontFace
    {
      const String data;
      FT_Face face;

      FontFace(String&& d, int size);
      virtual ~FontFace();
    };

    void rebuildTextures();
    void unloadTextures();

    std::vector<FontTexture> mTextures;

    void getTextureForNewGlyph(const Vector2i& glyphSize, FontTexture*& tex_out, Vector2i& cursor_out);

    std::map< unsigned int, std::unique_ptr<FontFace> > mFaceCache;
    FT_Face getFaceForChar(UnicodeChar id);
    void clearFaceCache();

  public:
    struct Glyph
    {
      FontTexture* texture;

      Vector2f texPos;
      Vector2f texSize; // in texels!

      Vector2f advance;
      Vector2f bearing;
    };

  private:
    //! Cached vertex array size
    static constexpr int sVertexArraySize = Vertex::sVertexPerRectangle * 1024;
    //! Direct drawing vertex cache
    static Vertex mVertexes[sVertexArraySize + Vertex::sVertexPerRectangle];

    //! Graphic char map
    std::map<UnicodeChar, Glyph> mGlyphMap;

    Glyph* getGlyph(UnicodeChar id);

    int mMaxGlyphHeight;

    const int mSize;
    const Path mPath;

    //! Maximum bearing of charaters from 32 to 128
    float mBearingMax;
    //! Maximum size of charaters from 32 to 128
    float mSizeMax;

    float getNewlineStartOffset(const String& text, unsigned int charStart, float xLen, TextAlignment alignment);

  public:
    static Path sRecalboxIconPath; //!< Recalbox icons
    static Path sDroidPath;        //!< japanese, chinese, korean
    static Path sDejaVuPath;
    static Path sUbuntuPath;

    static void initLibrary();

    static std::shared_ptr<Font> get(int size, const Path& path = getDefaultPath());

    virtual ~Font();

    Vector2f sizeText(const String& text, float lineSpacing = 1.5f); // Returns the expected size of a string when rendered.  Extra spacing is applied to the Y axis.
    float TextWidth(const String& text);
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color) { RenderDirect(text, offsetX, offsetY, color, 0.0f, TextAlignment::Left, 1.5f, false); }
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color, bool nospacing) { RenderDirect(text, offsetX, offsetY, color, 0.0f, TextAlignment::Left, 1.5f, nospacing); }
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color, float lineSpacing) { RenderDirect(text, offsetX, offsetY, color, 0.0f, TextAlignment::Left, lineSpacing, false); }
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color, float xLen, bool nospacing) { RenderDirect(text, offsetX, offsetY, color, xLen, TextAlignment::Left, 1.5f, nospacing); }
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color, float xLen, TextAlignment alignment, bool nospacing = false) { RenderDirect(text, offsetX, offsetY, color, xLen, alignment, 1.5f, nospacing); }
    void RenderDirect(const String& text, float offsetX, float offsetY, unsigned int color, float xLen, TextAlignment alignment, float lineSpacing, bool nospacing = false);
    void renderCharacter(unsigned int unicode, float x, float y, float wr, float hr, unsigned int color);

    String wrapText(String text, float xLen); // Inserts newlines into text to make it wrap properly.
    Vector2f sizeWrappedText(const String& text, float xLen, float lineSpacing = 1.5f); // Returns the expected size of a string after wrapping is applied.
    Vector2f getWrappedTextCursorOffset(const String& text, float xLen, size_t cursor, float lineSpacing = 1.5f); // Returns the position of of the cursor after moving "cursor" characters.

    float getHeight(float lineSpacing = 1.5f) const;
    float getLetterHeight();

    void reload(ResourceManager&) final { rebuildTextures(); }
    void unload(ResourceManager&) final { unloadTextures(); }

    int getSize() const;
    inline const Path& getPath() const { return mPath; }

    static Path getDefaultPath() { static Path defaultFont(FONT_PATH_REGULAR); return defaultFont; }

    static std::shared_ptr<Font> getFromTheme(const ThemeElement& elem, ThemePropertyCategory properties, const std::shared_ptr<Font>& orig);

    size_t getMemUsage() const; // returns an approximation of VRAM used by this font's texture (in bytes)
    static size_t getTotalMemUsage(); // returns an approximation of total VRAM used by font textures (in bytes)

    // utf8 stuff
    static size_t getNextCursor(const String& str, size_t cursor);
    static size_t getPrevCursor(const String& str, size_t cursor);
    static size_t moveCursor(const String& str, size_t cursor, int moveAmt); // negative moveAmt = move backwards, positive = move forwards
    static UnicodeChar readUnicodeChar(const String& str, size_t& cursor); // reads unicode character at cursor AND moves cursor to the next valid unicode char

    /*!
     * @brief Shortent text according to the given max width
     * If the original text width is higher than the max width, the test is shortened with a three dot caracter
     * @param text Original text
     * @param mawWidth Maximum width allowed
     * @return Shortened text or original text there is no need to cut it
     */
    String ShortenText(const String& text, float mawWidth);

    /*!
     * @brief Return the max bearing (distance from top to baseline)
     * @return Max bearing value
     */
    float getMaxBearing() const { return mBearingMax; }

    /*!
     * @brief Return the max height of charaters (distance from top to bottom)
     * @return Max height
     */
    float getMaxHeight() const { return mSizeMax; }


    Glyph& Character(unsigned int unicode) { return *getGlyph(unicode); }
};
