//
// Created by bkg2k on 03/08/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "rendering/textures/TextureHolder.h"
#include "Shader.h"

class Canvas : protected TextureHolder
{
  public:
    /*!
     * @brief Constructor
     * @param width Texture width
     * @param height Texture height
     */
    Canvas(int width, int height, int colorSpace);

    /*!
     * @brief Destructor
     */
    ~Canvas() override;

    /*!
     * @brief Activate this canvas - All subsequent drawing will be done in this texture
     */
    void Activate() const;

    /*!
     * @brief De-activate this canvas - All subsequent drawing will be done on screen
     */
    void Deactivate();

    /*!
     * @brief Render this texture fullscreen
     */
    void RenderFullScreen();

    /*!
     * @brief Render this texture grayed and fullscreen
     */
    void RenderGrayscaleFullScreen();

    /*!
     * @brief Render this texture grayed, blurred and fullscreen
     */
    void RenderBlurredFullScreen();

  private:
    //! Optional second buffer
    Canvas* mNextCanvas;

    //! Frame buffer
    GLuint mFrameBuffer;

    //! Color space
    int mColorSpace;

    /*!
     * @brief Create internal buffers
     * @return True on success, false on failure
     */
    bool CreateFrameBuffer();

    //! Return static gray shader, on demand
    static Shader& GrayShader();

    //! Return static blur shader, on demand
    static Shader& BlurShader();

    //! Allocate second buffer if not already done
    void AllocateSecondBuffer();

    //! Free second buffer if it has been previously allocated
    void FreeSecondBuffer();
};



