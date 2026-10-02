//
// Created by bkg2k on 30/08/24.
//
#pragma once

#include "rendering/opengl/Vertex.h"
#include "rendering/opengl/Colors.h"
#include <rendering/textures/TextureManager.h>

class RatingImage
{
  public:
    /*!
     * @brief Constructor
     * @param image Image resource
     */
    explicit RatingImage(int height)
      : mFilledStar(TextureManager::Instance().Create(Path(String(height) + '|' + ":/star_filled.svg"), TextureHolder::Properties::ImmediateLoad))
      , mUnfilledStar(TextureManager::Instance().Create(Path(String(height) + '|' + ":/star_unfilled.svg"), TextureHolder::Properties::ImmediateLoad))
      , mLastX(-1)
      , mLastY(-1)
      , mLastValue(-1)
      , mStarHeight(0)
      , mStarLineWidth(0)
    {
      ResetSize(height);
    }

    //! Width
    [[nodiscard]] int Width() const { return mStarLineWidth; }
    //! Height
    [[nodiscard]] int Height() const { return mStarHeight; }

    //! Reset size
    void ResetSize(int height)
    {
      if (height != mStarHeight)
      {
        mStarHeight = height;
        mStarLineWidth = height * sStarCount;
        mFilledStar.SetTargetSize(height, height);
        mUnfilledStar.SetTargetSize(height, height);
        TextureManager::Instance().RequestTextureLoading(mFilledStar, true);
        TextureManager::Instance().RequestTextureLoading(mUnfilledStar, true);
        PrepareDrawingTo(0, 0, 0);
      }
    }

    /*!
     * @brief Draw the nine patch image
     * @param targetArea Target rectangle
     * @param value current rating value between 0.f and 1.f
     * @param color Color
     */
    void Render(float x, float y, float value, Colors::ColorRGBA color);

  private:
    //! Start line count
    static constexpr int sStarCount = 5;
    //! Filled star
    Texture mFilledStar;
    //! Unfiles star
    Texture mUnfilledStar;
    //! Vertexes
    Vertex mVertices[Vertex::sVertexPerRectangle * 2];
    //! Last x coordinate
    float mLastX;
    //! Last Y coordinate
    float mLastY;
    //! Last value
    float mLastValue;
    //! Star height
    int mStarHeight;
    //! Stars width
    int mStarLineWidth;

    /*!
     * @brief Prepare drawing to the target area
     * @param targetArea Target area
     * @param value current rating value between 0.f and 1.f
     */
    void PrepareDrawingTo(float x, float y, float value);
};

