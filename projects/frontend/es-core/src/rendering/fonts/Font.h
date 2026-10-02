//
// Created by bkg2k on 10/12/2020.
//
#pragma once

#include <utils/os/fs/Path.h>
#include <rendering/fonts/IFontFaceProvider.h>
#include <rendering/fonts/GlyphStorage.h>
#include <rendering/textures/Texture.h>
#include <rendering/fonts/Alignment.h>
#include <rendering/fonts/ArabicShaper.h>
#include "FontEffect.h"
#include "rendering/opengl/Colors.h"
#include "rendering/opengl/Rectangle.h"
#include <Renderer.h>

#define FONT_SIZE_EXTRASMALL (0.030f)
#define FONT_SIZE_SMALL (0.035f)
#define FONT_SIZE_MEDIUM (0.045f)
#define FONT_SIZE_LARGE (0.085f)

#define FONT_PATH_LIGHT ":/ubuntu_condensed.ttf"
#define FONT_PATH_REGULAR ":/ubuntu_condensed.ttf"
#define FONT_PATH_CRT ":/f8bitfortressplus.ttf"

class NewFont : private INoCopy
{
  public:
    //! Default font size, in screen height ratio
    static constexpr const float sDefaultFontSize = .035f;

    //! Chunk of text
    struct TextChunk
    {
      int Start;  //!< Start offset
      int Length; //!< Chunk length
      int Width;  //!< Width in pixel
    };
    //! List of Chunks
    struct TextChunkList
    {
      String RefString;        //!< Reference string
      FontEffect Effect;       //!< Font effect
      Array<TextChunk> Chunks; //!< Chunks in the reference string

      [[nodiscard]] int Width() const { int w = 0; for(const TextChunk& c : Chunks) w = Math::max(w, c.Width); return w; }
      [[nodiscard]] int Height(float lineScale) const { return Math::roundi((float)Chunks.Count() * lineScale); }
    };

    /*!
     * @brief Create default font
     * @param faceProvider Font face provider
     * @param heightInPixel Required height in pixel
     * @param arabicLocale True when the system locale requires Arabic shaping
     */
    NewFont(IFontFaceProvider& faceProvider, float heightInPixel, bool arabicLocale);

    /*!
     * @brief Create specific font
     * @param faceProvider Font face provider
     * @param fontFile Font path or resource path
     * @param heightInPixel Required height in pixel
     * @param arabicLocale True when the system locale requires Arabic shaping
     */
    NewFont(IFontFaceProvider& faceProvider, const Path& fontFile, float heightInPixel, bool arabicLocale);

    /*!
     * @brief Get the width in pixels of the given Unicode character using the default effect
     * @param unicode Unicode codepoint to measure
     * @return Width in pixels
     */
    int CharWidth(unsigned int unicode)
    {
      return CharWidth(unicode, mDefaultEffect);
    }

    /*!
     * @brief Get the width in pixels of the given Unicode character
     * @param unicode Unicode codepoint to measure
     * @param effect Font effect
     * @return Width in pixels
     */
    int CharWidth(unsigned int unicode, const FontEffect& effect);

    /*!
     * @brief Get the width in pixels of the given string using the default effect
     * @param string String to measure
     * @return Width in pixels
     */
    int TextWidth(const String& string)
    {
      return TextWidth(string.data(), 0, (int)string.length(), mDefaultEffect);
    }

    /*!
     * @brief Get the width in pixels of the given string
     * @param string String to measure
     * @param effect Font effect
     * @return Width in pixels
     */
    int TextWidth(const String& string, const FontEffect& effect)
    {
      return TextWidth(string.data(), 0, (int)string.length(), effect);
    }

    /*!
     * @brief Get the width in pixels of a pre-shaped Unicode codepoint array
     * @param unicodes Pre-shaped Unicode codepoint array
     * @param start Start index in the array
     * @param length Number of codepoints to measure
     * @param effect Font effect
     * @return Width in pixels
     */
    int TextWidth(const Array<unsigned int>& unicodes, int start, int length, const FontEffect& effect);

    /*!
     * @brief Truncate the string according to the given max width if required, and add ellipsis '…'
     * @param string String to truncate
     * @param maxWidth Maximum allowed width in pixel
     * @return Truncated string or original string if the original string fits into the given width
     */
    String TextEllipsis(const String& string, int maxWidth)
    {
      return TextEllipsis(string, mDefaultEffect, maxWidth);
    }

    /*!
     * @brief Truncate the string according to the given max width if required, and add ellipsis '…'
     * @param string String to truncate
     * @param effect Font effect
     * @param maxWidth Maximum allowed width in pixel
     * @return Truncated string or original string if the original string fits into the given width
     */
    String TextEllipsis(const String& string, const FontEffect& effect, int maxWidth);

    /*!
     * @brief Get the width and height in pixels of the given string using the default effect
     * @param string String to measure
     * @return Width and height in pixels as a Vector2f
     */
    Vector2f TextSize(const String& string)
    {
      return { (float)TextWidth(string.data(), 0, (int)string.length(), mDefaultEffect), (float)mRealMaxHeightInPixel };
    }

    /*!
     * @brief Get the width and height in pixels of the given string
     * @param string String to measure
     * @param effect Font effect
     * @return Width and height in pixels as a Vector2f
     */
    Vector2f TextSize(const String& string, const FontEffect& effect)
    {
      return { (float)TextWidth(string.data(), 0, (int)string.length(), effect), (float)mRealMaxHeightInPixel };
    }

    /*!
     * @brief Get the width and height in pixels of a pre-shaped Unicode codepoint array
     * @param unicodes Pre-shaped Unicode codepoint array
     * @param start Start index in the array
     * @param length Number of codepoints to measure
     * @param effect Font effect
     * @return Width and height in pixels as a Vector2f
     */
    Vector2f TextSize(const Array<unsigned int>& unicodes, int start, int length, const FontEffect& effect)
    {
      return { (float)TextWidth(unicodes, start, length, effect), (float)mRealMaxHeightInPixel };
    }

    /*!
     * @brief Split a string into word-wrapped chunks that fit within the given width
     * @param string Source string
     * @param start Start byte offset in the string
     * @param length Byte length of the substring to process
     * @param offsetInPixel Horizontal offset already consumed on the first line (e.g. by a bullet)
     * @param widthInPixel Maximum line width in pixels
     * @param effect Font effect
     * @return List of chunks with their byte offsets, lengths and pixel widths
     */
    TextChunkList SplitText(const String& string, int start, int length, int offsetInPixel, int widthInPixel, const FontEffect& effect);

    /*!
     * @brief Split a string into word-wrapped chunks that fit within the given width
     * @param string Source string
     * @param offsetInPixel Horizontal offset already consumed on the first line
     * @param widthInPixel Maximum line width in pixels
     * @param effect Font effect
     * @return List of chunks with their byte offsets, lengths and pixel widths
     */
    TextChunkList SplitText(const String& string, int offsetInPixel, int widthInPixel, const FontEffect& effect)
    {
      return SplitText(string, 0, string.Count(), offsetInPixel, widthInPixel, effect);
    }

    /*!
     * @brief Split a string into word-wrapped chunks that fit within the given width using the default effect
     * @param string Source string
     * @param widthInPixel Maximum line width in pixels
     * @param effect Font effect
     * @return List of chunks with their byte offsets, lengths and pixel widths
     */
    TextChunkList SplitText(const String& string, int widthInPixel, const FontEffect& effect)
    {
      return SplitText(string, 0, string.Count(), 0, widthInPixel, effect);
    }

    /*!
     * @brief Split a string into word-wrapped chunks that fit within the given width using the default effect
     * @param string Source string
     * @param widthInPixel Maximum line width in pixels
     * @return List of chunks with their byte offsets, lengths and pixel widths
     */
    TextChunkList SplitText(const String& string, int widthInPixel)
    {
      return SplitText(string, 0, string.Count(), 0, widthInPixel, mDefaultEffect);
    }

    /*!
     * @brief Draw a single character with Y as the font baseline, using the default effect
     * @param character Unicode character to draw
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param color Text color
     * @return Character advance width in pixels
     */
    int DrawCharBaseline(UnicodeChar character, int x, int y, Colors::ColorARGB color)
    {
      return DrawCharBaseline(character, x, y, color, mDefaultEffect);
    }

    /*!
     * @brief Draw a single character with Y as the font baseline
     * @param character Unicode character to draw
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param color Text color
     * @param effects Font effect
     * @return Character advance width in pixels
     */
    int DrawCharBaseline(UnicodeChar character, int x, int y, Colors::ColorARGB color, const FontEffect& effects);

    /*!
     * @brief Draw a single character using the default effect
     * @param character Unicode character to draw
     * @param x X coordinate
     * @param y Y coordinate
     * @param origin Origin within the character box — Center draws the character centered on (X,Y)
     */
    void DrawChar(UnicodeChar character, int x, int y, Colors::ColorARGB, ::Alignment origin);

    /*!
     * @brief Draw a single character
     * @param character Unicode character to draw
     * @param x X coordinate
     * @param y Y coordinate
     * @param origin Origin within the character box — Center draws the character centered on (X,Y)
     * @param effects Font effect
     */
    void DrawChar(UnicodeChar character, int x, int y, Colors::ColorARGB, ::Alignment origin, const FontEffect& effects);

    /*!
     * @brief Draw a string at coordinate X, Y. The Y line is the baseline of the font, not the top!
     * @param string String to draw
     * @param start start byte in the string
     * @param length length byte of the string
     * @param x X coordinate to draw text to
     * @param y Y coordinate to draw text to
     * @param color Character color
     * @param alignment Horizontal alignment applied on X coordinate
     * @param effect Font effect
     */
    void DrawText(const String& string, int start, int length, int x, int y, Colors::ColorARGB color, ::Alignment alignment, const FontEffect& effect);

    /*!
     * @brief Draw a string at coordinate X, Y. The Y line is the baseline of the font, not the top!
     * @param string String to draw
     * @param x X coordinate to draw text to
     * @param y Y coordinate to draw text to
     * @param color Character color
     * @param alignment Horizontal alignment applied on X coordinate
     * @param effect Font effect
     */
    void DrawText(const String& string, int x, int y, Colors::ColorARGB color, ::Alignment alignment, const FontEffect& effect)
    {
      DrawText(string, 0, (int)string.length(), x, y, color, alignment, effect);
    }

    /*!
     * @brief Draw a string at coordinate X, Y. The Y line is the baseline of the font, not the top!
     * @param string String to draw
     * @param x X coordinate to draw text to
     * @param y Y coordinate to draw text to
     * @param color Character color
     * @param alignment Horizontal alignment applied on X coordinate
     */
    void DrawText(const String& string, int x, int y, Colors::ColorARGB color, ::Alignment alignment)
    {
      DrawText(string, x, y, color, alignment, mDefaultEffect);
    }

    /*!
     * @brief Draw a multiline string at coordinate X, Y. The Y line is the baseline of the font, not the top!
     * @param strings Strings to draw
     * @param x X coordinate to draw text to
     * @param y Y coordinate to draw text to
     * @param color Character color
     * @param alignment Horizontal alignment applied on X coordinate
     * @param interline Interline in ratio of font height
     */
    void DrawText(const TextChunkList& strings, int x, int y, Colors::ColorARGB color, ::HorizontalAlignment alignment, float interline = 0);

    /*!
     * @brief Draw a single line text in the given rectangle
     * @param string String to draw
     * @param start start byte in the string
     * @param length length byte of the string
     * @param into Rectangle to draw string in (perform clipping)
     * @param xOffset X Offset applied to the text inside the rectangle
     * @param yOffset Y Offset applied to the text inside the rectangle
     * @param color Text color
     * @param alignment Text alignment inside the rectangle
     * @param effect Font effect
     */
    void DrawText(const String& string, int start, int length, const Rectangle& into, float xOffset, float yOffset, Colors::ColorARGB color, ::Alignment alignment, const FontEffect& effect);

    /*!
     * @brief Draw a single line text in the given rectangle
     * @param string String to draw
     * @param into Rectangle to draw string in (perform clipping)
     * @param xOffset X Offset applied to the text inside the rectangle
     * @param yOffset Y Offset applied to the text inside the rectangle
     * @param color Text color
     * @param alignment Text alignment inside the rectangle
     * @param effect Font effect
     */
    void DrawText(const String& string, const Rectangle& into, float xOffset, float yOffset, Colors::ColorARGB color, ::Alignment alignment, const FontEffect& effect)
    {
      DrawText(string, 0, string.Count(), into, xOffset, yOffset, color, alignment, effect);
    }

    /*!
     * @brief Draw a single line text in the given rectangle
     * @param string String to draw
     * @param into Rectangle to draw string in (perform clipping)
     * @param color Text color
     * @param alignment Text alignment inside the rectangle
     * @param effect Font effect
     */
    void DrawText(const String& string, const Rectangle& into, Colors::ColorARGB color, ::Alignment alignment, const FontEffect& effect)
    {
      DrawText(string, 0, string.Count(), into, 0, 0, color, alignment, effect);
    }

    /*!
     * @brief Draw a single line text in the given rectangle
     * @param string String to draw
     * @param into Rectangle to draw string in (perform clipping)
     * @param color Text color
     * @param alignment Text alignment inside the rectangle
     */
    void DrawText(const String& string, const Rectangle& into, Colors::ColorARGB color, ::Alignment alignment)
    {
      DrawText(string, 0, string.Count(), into, 0, 0, color, alignment, mDefaultEffect);
    }

    /*!
     * @brief Draw a multi-line pre-split text in the given rectangle with clipping
     * @param strings Pre-split chunk list produced by SplitText
     * @param into Rectangle to draw into (clipping is applied)
     * @param xoffset X offset applied to the text inside the rectangle
     * @param yoffset Y offset applied to the text inside the rectangle
     * @param color Text color
     * @param alignment Text alignment inside the rectangle
     * @param interline Interline spacing in ratio of font height
     */
    void DrawText(const TextChunkList& strings, const Rectangle& into, float xOffset, float yOffset, Colors::ColorARGB color, ::Alignment alignment, float interline = 0);

    //! Get requested height
    [[nodiscard]] int RequestedHeight() const { return (int)mRequestedHeightInPixel; }

    //! Get maximum height (height of the higher character)
    [[nodiscard]] int Height() const { return mRealMaxHeightInPixel; }

    //! Get maximum offset from the top to the baseline (from the higher character)
    [[nodiscard]] int TopToBaseline() const { return mTopToBaseline; }

    //! Get maximum offset from the baseline to the bottom (from the higher character)
    [[nodiscard]] int BottomToBaseline() const { return mBottomToBaseline; }

    //! Clear all caches
    void ClearCaches();

    #ifdef DEBUG
    void RenderFontCache();
    #endif

    //! Font path
    [[nodiscard]] const Path& GetPath() const { return mPath; }

  private:
    //! Face provider
    IFontFaceProvider& mFaceProvider;
    //! Font glyph store
    GlyphStorage mGlyphStore;

    //! Default font effect
    const FontEffect mDefaultEffect;

    //! Cached font face for non default fonts
    FT_Face mFace;

    //! Font resource
    Path mPath;
    //! Font height in pixel
    float mRequestedHeightInPixel;
    //! Real max font height
    int mRealMaxHeightInPixel;
    //! Maximum offset from the top to the baseline in pixel
    int mTopToBaseline;
    //! Maximum offset from the baseline to the bottom in pixel
    int mBottomToBaseline;

    //! Current texture in use
    Texture mCurrentTexture;
    //! Current texture width
    int mTextureWidth;
    //! Current texture height
    int mTextureHeight;
    //! Current free position X in the current texture
    int mCursorX;
    //! Current free position Y in the current texture
    int mCursorY;
    //! Row height
    int mRowHeight;
    //! Incremented every time the glyph store is cleared, invalidating all Glyph pointers
    int mGlyphGeneration;

    //! This is the default font and may look up characters across multiple fallback fonts
    bool mDefault;
    //! True when the system locale requires Arabic shaping (ar/fa/ur). Immutable after construction.
    bool mArabicLocale;

    //! Reusable buffer for BiDi / Arabic shaping output (avoids per-call heap allocation)
    Array<unsigned int> mBiDiBuffer;

    /*!
     * @brief Get the most appropriate font face for the given character
     * @param unicode Unicode character
     * @return Pre-configured font face
     */
    FT_Face FaceForChar(UnicodeChar);

    /*!
     * @brief Get the most appropriate font face for the given character, falling back to default fonts
     * @param unicode Unicode character
     * @return Pre-configured font face
     */
    FT_Face FaceForCharFallback(UnicodeChar) const;

    /*!
     * @brief Make room in the current texture for a new glyph, creating a new texture if needed
     * @param glyph Glyph slot to get metrics from
     * @param w Required width in pixels
     * @param h Required height in pixels
     * @return True if the glyph fits at the current cursor position, false if it cannot fit at all
     */
    bool MakeRoomInTexture(FT_GlyphSlot glyph, int w, int h);

    /*!
     * @brief Allocate a new glyph atlas texture sized for the current font
     * @param face Font face used to determine texture dimensions
     * @param minWidth Minimum width in pixels the texture must accept
     * @param minHeight Minimum height in pixels the texture must accept
     */
    void MakeNewTexture(FT_Face face, int minWidth, int minHeight);

    /*!
     * @brief Copy the given glyph bitmap into the current atlas texture
     * @param own Rendered bitmap glyph to copy
     * @return True if the bitmap has been uploaded, false if the upload failed
     */
    bool RenderGlyphInTexture(FT_BitmapGlyph glyph);

    /*!
     * @brief Load and cache the glyph for the given Unicode character
     * @param unicode Unicode character to prepare
     * @param embold True to produce a bold variant — bold and regular glyphs are cached separately
     * @return Pointer to the cached glyph, or nullptr if the GL context is unavailable
     */
    Glyph* Prepare(UnicodeChar unicode, bool embold);

    /*!
     * @brief Draw a substring at (X, Y) where Y is the font baseline
     * @param string Source string
     * @param start Start byte offset in the string
     * @param length Byte length of the substring to draw
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param effect Font effect
     */
    void DrawTextRaw(const String& string, int start, int length, int x, int y, Colors::ColorARGB, const FontEffect& effect);

    /*!
     * @brief Draw a pre-shaped Unicode codepoint array at (X, Y) where Y is the font baseline
     * @param unicodes Pre-shaped codepoint array produced by ArabicShaper::Process
     * @param x X coordinate
     * @param y Y coordinate (baseline)
     * @param effect Font effect
     */
    void DrawTextRaw(const Array<unsigned int>& unicodes, int x, int y, Colors::ColorARGB, const FontEffect& effect);

    /*!
     * @brief Get the width in pixels of a substring
     * @param string Source string
     * @param start Start byte offset in the string
     * @param length Byte length of the substring to measure
     * @param effect Font effect
     * @return Width in pixels
     */
    int TextWidth(const String& string, int start, int length, const FontEffect& effect);

    /*!
     * @brief Create Glyph vertexes from glyph structure
     * @param glyph Glyph structure
     * @param x X target coordinate
     * @param y Y target coordinate
     * @param effect Font effect
     * @param vertex Rectangle vertex array
     */
    static void MapGlyphTexture(const Glyph& glyph, float x, float y, const FontEffect::Final& effect, Vertex::Rectangle& vertex);
};
