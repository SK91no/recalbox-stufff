//
// Created by bkg2k on 22/01/24.
//
#pragma once

// Forward declaration
class VideoComponent;

class IVideoComponentAction
{
  public:
    //! Required actions
    enum class Action
    {
      FadeIn,  //!< Linked component must fade in
      FadeOut, //!< Linked component must fade out
    };

    //! Default destructor
    virtual ~IVideoComponentAction() = default;

    /*!
     * @brief Called when a video component require action
     * @param source Source video component
     * @param action Required action
     */
    virtual void VideoComponentRequireAction(const VideoComponent* source, Action action) = 0;
};