//
// Created by bkg2k on 14/01/2020.
//
#pragma once

#include "TextureHolder.h"
#include "rendering/opengl/Vertex.h"

class TextureManager;

class Texture
{
  private:
    //! TextureHolder reference - Cannot be null.
    TextureHolder* mTexture;

    /*!
     * @brief Original constructor - Only accessible from TextureManager
     * @param textureHolder Texture holder reference
     */
    explicit Texture(TextureHolder* textureHolder)
      : mTexture(textureHolder)
    {
      if (mTexture != nullptr)
        mTexture->IncReference();
    }

    friend TextureManager;

  public:
    /*!
     * @brief Default constructor
     */
    Texture()
      : mTexture(nullptr)
    {
    }

    /*!
     * @brief Destructor - Delete original Texture holder when reference count reached 0
     */
    ~Texture()
    {
      if (mTexture != nullptr)
        if (mTexture->DecReference() == 0)
          delete mTexture;
    }

    /*!
     * @brief Copy constructor - Increase reference count
     * @param source source object
     */
    Texture(const Texture& source)
      : mTexture(source.mTexture)
    {
      if (mTexture != nullptr)
        mTexture->IncReference();
    }

    /*!
     * @brief Move constructor - Reference count does not move
     * @param source source object
     */
    Texture(Texture&& source) noexcept
      : mTexture(source.mTexture)
    {
      source.mTexture = nullptr;
    }

    /*!
     * @brief Copy operator - Increase reference if not self-copied
     * @param source source object
     * @return this
     */
    Texture& operator = (const Texture& source)
    {
      if (&source != this)
      {
        // Remove old reference
        if (mTexture != nullptr)
          if (mTexture->DecReference() == 0) delete mTexture;

        // Assign new reference
        mTexture = source.mTexture;
        if (mTexture != nullptr) mTexture->IncReference();
      }
      return *this;
    }

    /*!
     * @brief Move operator - Reference count does not move
     * @param source source object
     * @return this
     */
    Texture& operator = (Texture&& source) noexcept
    {
      if (&source != this)
      {
        // Remove old reference
        if (mTexture != nullptr)
          if (mTexture->DecReference() == 0) delete mTexture;

        // Assign new reference
        mTexture = source.mTexture;
        source.mTexture = nullptr;
      }

      return *this;
    }

    /*!
     * @brief Check if underlying object exists
     * @return True if the underlying object is not null
     */
    [[nodiscard]] bool Valid() const
    {
      return (mTexture != nullptr);
    }

    /*
     * Protected Rooting
     */

    /*!
     * @brief Blit texture
     * @param vertex Vertex list
     * @param color Color list
     * @param vertexCount Vectex count
     * @param tiled Draw tiled texture
     */
    void Render(const Vertex vertex[], const ColorByte color[], int vertexCount, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Blit texture
     * @param vertex Vertex list
     * @param color Color list
     * @param vertexCount Vectex count
     * @param tiled Draw tiled texture
     */
    void Render(const Vertex vertex[], unsigned int color, int vertexCount, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Upload an alpha bitmap into the texture
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, one byte per pixel
     */
    [[nodiscard]] bool UploadAlpha(int width, int height, const void* data) const;

    /*!
     * @brief Upload an RGBA bitmap into the texture
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, 4 byte per pixel
     */
    [[nodiscard]] bool UploadRGBA(int width, int height, const void* data) const;

    /*!
     * @brief Upload an alpha bitmap into the texture
     * @param x Destination x coordinate (in pixel)
     * @param y Destination y coordinate (in pixel)
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, one byte per pixel
     */
    [[nodiscard]] bool UploadAlphaPart(int x, int y, int width, int height, const void* data) const;

    /*!
     * @brief Upload an RGBA bitmap into the texture
     * @param x Destination x coordinate (in pixel)
     * @param y Destination y coordinate (in pixel)
     * @param width Bitmap width
     * @param height Bitmap Height
     * @param data Pointer to Bitmap data, 4 byte per pixel
     */
    [[nodiscard]] bool UploadRGBAPart(int x, int y, int width, int height, const void* data) const;

    /*!
     * @brief Get resource size
     * @return Size
     */
    [[nodiscard]] Vector2i Size() const;

    /*!
     * @brief Get resource width
     * @return Width in pixel
     */
    [[nodiscard]] int Width() const;

    /*!
     * @brief Get resource height
     * @return height in pixel
     */
    [[nodiscard]] int Height() const;

    /*!
     * @brief Set target size (SVG only, FreeTexture only)
     * @param width Required width
     * @param height Required height
     * @param force Bypass lowest size control and force a new rasterization for SVG images
     * @note If the texture is already loaded, it is unloaded
     */
    void SetTargetSize(int width, int height, bool force = false);

    /*!
     * @brief Check if the texture is ready to be drawn
     * @return True if the texture is loaded or loading
     */
    [[nodiscard]] bool IsReady() const
    {
      if (mTexture != nullptr) return mTexture->Status() == TextureHolder::Status::Loaded;
      return false;
    }

    /*!
     * @brief Check if the texture is ready to be drawn. If not, request the load
     * @return True if the texture is loaded
     */
    bool MakeReady();

    /*!
     * @brief Clear the texture and free all
     */
    void Clear()
    {
      if (mTexture != nullptr)
      {
        mTexture->Clear();
        if (mTexture->DecReference() == 0)
          delete mTexture;
        mTexture = nullptr;
      }
    }

    //! Identify SVG source
    [[nodiscard]] bool IsSVG() const
    {
      if (mTexture != nullptr) return mTexture->IsSVG();
      return false;
    }

    //! Get raw GL identifier
    [[nodiscard]] int Identifier() const
    {
      if (mTexture != nullptr) return (int)mTexture->mGLTextureID;
      return 0;
    }

    /*
     * High level rendering
     */

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates & texture width/height
     * @param x X coordinate
     * @param y Y coordinate
     */
    void Render(int x, int y, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates & width/height
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     */
    void Render(int x, int y, int w, int h, bool tiled, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param alpha Blending alpha
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, texture width/height and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param alpha Blending alpha
     */
    void Render(int x, int y, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height and a given blending alpha
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param alpha Blending alpha
     */
    void Render(int x, int y, int w, int h, bool tiled, unsigned char alpha, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, keeping ratio yes/no and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio, false to stretch
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool flipX, bool flipY, bool keepratio, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, texture width/height and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Blending color
     */
    void Render(int x, int y, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height and a given blending color
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param color Blending color
     */
    void Render(int x, int y, int w, int h, bool tiled, Colors::ColorARGB color, Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, ratio & and blending colors
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height and blending colors
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     */
    void Render(int x, int y, int w, int h, bool tiled,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height, ratio,  blending colors & lower reflection
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param keepratio True to keep ratio
     * @param flipOnX True to flip texture around vertical axis
     * @param flipOnY True to flip texture around horizontal axis
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     * @param topAlphaReflection Top alpha reflection
     * @param bottomAlphaReflection Bottom alpha reflection
     */
    void Render(int x, int y, int w, int h, bool keepratio, bool tiled,
                bool flipOnX, bool flipOnY,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                float topAlphaReflection,
                float bottomAlphaReflection,
                Renderer::TextureType type = Renderer::TextureType::RGBA);

    /*!
     * @brief Draw the current texture using full coordinates, width/height,  blending colors & lower reflection
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     * @param flipOnX True to flip texture around vertical axis
     * @param flipOnY True to flip texture around horizontal axis
     * @param topleftcolor Top left color blending
     * @param toprightcolor  Top right color blending
     * @param bottomrightcolor Bottom Right color blending
     * @param bottomleftcolor Bottom Left color blending
     * @param topAlphaReflection Top alpha reflection
     * @param bottomAlphaReflection Bottom alpha reflection
     */
    void Render(int x, int y, int w, int h, bool tiled,
                bool flipOnX, bool flipOnY,
                Colors::ColorARGB topleftcolor,
                Colors::ColorARGB toprightcolor,
                Colors::ColorARGB bottomrightcolor,
                Colors::ColorARGB bottomleftcolor,
                float topAlphaReflection,
                float bottomAlphaReflection,
                Renderer::TextureType type = Renderer::TextureType::RGBA);
} __attribute__((packed));
