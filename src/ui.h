#pragma once
#include <cstdint>

namespace ui
{
	// Initial window size in logical (96 DPI) pixels. The height is then fitted
	// to the content after the first frame (see RequiredHeight).
	constexpr int kWidth = 420;
	constexpr int kHeight = 540;
	constexpr std::uint32_t kBackground = 0x111317;

	// Loads fonts and applies the theme for the given DPI scale.
	void Init(float dpiScale) noexcept;

	// Draws one frame. Returns true while something on screen is animating
	// (so the caller keeps rendering at full rate).
	bool Render() noexcept;

	// Window height (physical pixels) the last frame's layout needs.
	int RequiredHeight() noexcept;
}
