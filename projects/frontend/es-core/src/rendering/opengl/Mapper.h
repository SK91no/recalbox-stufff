//
// Created by bkg2k on 15/09/2020.
//
#pragma once

#include "Vertex.h"
#include "Rectangle.h"
#include "rendering/RenderingTypes.h"

class Mapper
{
  public:
    /*!
     * @brief Create vertexes array that maps a rectangle texture into a target display area
     * The rectangle is split into two triangle:
     * - TopLeft - BottomLeft - TopRight
     * - TopRight - BottomLeft - BotomRight
     * @param target Target display area
     * @param texture Texture rectangle
     * @param textureWidth Texture total width
     * @param textureHeight Texture total height
     * @param vertexes Vertexes that receive mapping data (must contains at lease 6 vertexes
     */
    static void MapTexture(const Rectangle& target, const Rectangle& texture, float textureWidth, float textureHeight, Vertex* vertexes);

    /*!
     * @brief Create vertexes array that maps a whole texture into a target display area
     * The rectangle is split into two triangle:
     * - TopLeft - BottomLeft - TopRight
     * - TopRight - BottomLeft - BotomRight
     * @param target Target display area
     * @param vertexes Vertexes that receive mapping data (must contains at lease 6 vertexes
     */
    static void MapTexture(const Rectangle& target, Vertex* vertexes);

    /*!
     * @brief Create color array for a mapped rectangle
     * @param topleftColor Top Left Color
     * @param toprightColor Top Right Color
     * @param bottomrightColor Bottom Right Color
     * @param bottomleftColor Bottom Left Color
     * @param glColors GL Colors (must contains at least 6 * 4 bytes)
     */
    static void MapColors(unsigned int topleftColor, unsigned int toprightColor,
                          unsigned int bottomrightColor, unsigned int bottomleftColor, ColorByte* glColors);
};
