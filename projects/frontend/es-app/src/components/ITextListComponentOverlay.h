//
// Created by bkg2k on 08/02/2020.
//
#pragma once

#include <rendering/opengl/Rectangle.h>

template<typename T> class ITextListComponentOverlay
{
  public:
    //! Default destructor
    virtual ~ITextListComponentOverlay() = default;

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param parentTrans Parent transform matrice
     * @param leftWidth Left overlay width
     * @param rightWidth Right overlay width
     * @param labelWidth Cell's label width in pixel
     * @param area item rectangle
     * @param data Linked data
     * @param color draw color
     */
    virtual void OverlayApply(const Transform4x4f& parentTrans, int leftWidth, int rightWidth, int labelWidth, const Rectangle& area, const T& data, int index, unsigned int& color) = 0;

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param data Linked data
     * @param labelWidth Cell's label width in pixel
     * @return left offset
     */
    virtual float OverlayGetLeftOffset(const T& data, int labelWidth) = 0;

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param data Linked data
     * @param labelWidth Cell's label width in pixel
     * @return right offset
     */
    virtual float OverlayGetRightOffset(const T& data, int labelWidth) = 0;
};
