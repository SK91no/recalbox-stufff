#pragma once

#include <guis/Gui.h>
#include <components/NinePatchComponent.h>
#include <components/ComponentGrid.h>
#include <input/InputDevice.h>
#include "IDeviceConfigured.h"
#include "components/PictureGaleryComponent.h"

class TextScrollComponent;
class MarkdownText;
class TextComponent;

class GuiSelectDevice : public Gui
	                    , public PictureGaleryComponent::IChangeNotifications
{
	public:
		/*!
		 * @brief Constructor
		 * @param window Window manager
		 * @param callbackInterface Callback interface
		 */
		GuiSelectDevice(WindowManager& window, InputDevice& device, IDeviceConfigured* callbackInterface);

		bool ProcessInput(const InputCompactEvent& event) final;
		void Update(int deltaTime) final;
		void Render(const Transform4x4f& transform) final;

	private:
		//! Holding time for unconfigured pads
		static constexpr int sHoldingTime = 2000;
		//! Holding time before bar shows
		static constexpr int sHoldingDeadTime = 500;

		int mHoldTime;
		bool mHolding;

		ComponentGrid mGrid;

		std::shared_ptr<TextComponent> mTitle;
		std::shared_ptr<TextComponent> mDeviceInfo;
		std::shared_ptr<PictureGaleryComponent> mGallery;
		std::shared_ptr<TextScrollComponent> mPad;
		std::shared_ptr<MarkdownText> mDoc;

		unsigned int mColor;
		unsigned int mAlpha = 0;

		//! Selected device
		InputDevice& mDevice;

		//! Callback interface
		IDeviceConfigured* mCallback;

		/*!
		 * @brief Allow to notify that the slot has changed (left/right move)
		 * @param slot New slot number
		 */
		void PictureGalerySlotChanged(int slot) final;
};
