#pragma once

#include <components/base/ThemableComponent.h>

class ScrollableContainer : public ThemableComponent
{
public:
	explicit ScrollableContainer(WindowManager&window);

	Vector2f getScrollPos() const { return mScrollPos; }
	void setScrollPos(const Vector2f& pos) { mScrollPos = pos; }

  void setAutoScroll(bool autoScroll);
	void reset();

	void Update(int deltaTime) override;
	void Render(const Transform4x4f& parentTrans) override;
	int mAutoScrollDelay; // ms to wait before starting to autoscroll
	int mAutoScrollSpeed; // ms to wait before scrolling down by mScrollDir
	int mAutoScrollResetAccumulator;
	int mAutoScrollAccumulator;

    /*
     * Themable implementation
     */

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::None; } // Container must be polymorphic

  private:
	Vector2f getContentSize();

	Vector2f mScrollPos;
	Vector2f mScrollDir;
	bool mAtEnd;

};
