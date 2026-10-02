//
// Created by bkg2k on 15/12/23.
//
#pragma once

#include "views/ViewTypes.h"

// Forward declaration
class WindowManager;

class IViewChanged
{
  public:
    //! Constructor
    IViewChanged(WindowManager& windowMAnager);

    //! Default destructor
    virtual ~IViewChanged();

    /*!
     * @brief Notify implementation that the current view juste changed, or a window has been show or hide
     * @param currentView Current view
     * @param hasWindowOver Has window over the current view?
     */
    virtual void ViewChanged(ViewType currentView, bool hasWindowOver) = 0;

  private:
    WindowManager& mWindowManager;
};
