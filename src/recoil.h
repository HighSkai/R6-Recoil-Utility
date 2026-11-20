#pragma once
#include <Windows.h>
#include <atomic>

// Recoil compensation engine. Runs on its own thread so the UI never blocks
// while the mouse is held, and the UI never slows the pull down.
namespace recoil
{
	constexpr int kMinStrength = 1;
	constexpr int kMaxStrength = 100;
	constexpr double kPixelsPerSecondPerStrength = 10.0; // strength 10 = 100 px/s

	// Shared between the UI thread and the worker. Plain atomics, no locks.
	inline std::atomic<bool> enabled{ false };
	inline std::atomic<int>  strength{ 15 };
	inline std::atomic<int>  angle{ 90 };           // degrees: 0 = right, 90 = straight down
	inline std::atomic<int>  toggleKey{ 'G' };      // virtual-key code, 0 = no hotkey
	inline std::atomic<bool> suppressHotkey{ false }; // set by the UI while typing / rebinding
	inline std::atomic<bool> pulling{ false };      // true while actively moving the mouse

	// notifyWindow receives a WM_APP message whenever enabled/pulling changes,
	// so the UI can redraw without polling.
	void Start(HWND notifyWindow) noexcept;
	void Stop() noexcept;
}
