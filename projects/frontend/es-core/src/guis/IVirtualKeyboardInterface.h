#pragma once

#include <utils/String.h>
#include <guis/IVirtualKeyboardBase.h>

class IVirtualKeyboardInterface
{
  public:
    virtual ~IVirtualKeyboardInterface() = default;

    /*!
     * @brief Called when the VK opens
     */
    virtual void VirtualKeyboardShow(IVirtualKeyboardBase& vk) { (void)vk; }

    /*!
     * @brief Called when the edited text change.
     * Current text is available from the Text() method.
     */
    virtual void VirtualKeyboardTextChange(IVirtualKeyboardBase& vk, const String& text) = 0;

    /*!
     * @brief Called when the edited text is validated (Enter or Start)
     * Current text is available from the Text() method.
     */
    virtual void VirtualKeyboardValidated(IVirtualKeyboardBase& vk, const String& text) = 0;


    /*!
     * @brief Called when the VK closes
     */
    virtual void VirtualKeyboardHide(IVirtualKeyboardBase& vk) { (void)vk; }
};
