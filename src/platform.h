#pragma once
#include <Windows.h>
#include <cstdint>

// Borderless Win32 window + Direct3D 9 device + Dear ImGui backends.
namespace platform
{
	constexpr wchar_t kWindowClass[] = L"R6RecoilUtilityWindow";

	// Size is in logical (96 DPI) pixels and is scaled to the monitor's DPI.
	bool Create(const wchar_t* title, int width, int height, std::uint32_t backgroundRgb) noexcept;
	void Destroy() noexcept;

	// The window starts hidden so it can be sized to its content before it appears.
	void Show() noexcept;
	void FitHeight(int height) noexcept; // physical pixels, keeps the window centred

	// Returns false once the window has been closed.
	bool PumpMessages() noexcept;

	// Sleeps until input/a message arrives or the timeout passes. Returns true if woken by a message.
	bool WaitForEvents(DWORD timeoutMs) noexcept;

	// Returns false when nothing should be drawn (minimized or device lost).
	bool BeginFrame() noexcept;
	void EndFrame() noexcept;

	HWND Handle() noexcept;
	float DpiScale() noexcept;
	bool IsFocused() noexcept;

	// Area (in physical pixels) at the top of the window that drags it, minus a
	// strip on the right that is left clickable for the window buttons.
	void SetDragArea(int height, int rightInset) noexcept;

	void Minimize() noexcept;
	void Close() noexcept;
}
