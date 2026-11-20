#include "platform.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include <d3d9.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <cmath>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

namespace
{
	HWND g_window = nullptr;
	HINSTANCE g_instance = nullptr;
	HBRUSH g_backgroundBrush = nullptr;
	D3DCOLOR g_clearColor = 0;

	IDirect3D9* g_d3d = nullptr;
	IDirect3DDevice9* g_device = nullptr;
	D3DPRESENT_PARAMETERS g_present{};
	bool g_deviceLost = false;

	UINT g_pendingWidth = 0;
	UINT g_pendingHeight = 0;
	float g_dpiScale = 1.0f;
	int g_dragHeight = 0;
	int g_dragRightInset = 0;
	bool g_quit = false;

	// Windows 11 DWM attributes (defined here so older SDKs still compile).
	constexpr DWORD kDwmUseDarkMode = 20;
	constexpr DWORD kDwmCornerPreference = 33;
	constexpr DWORD kDwmBorderColor = 34;
	constexpr int kDwmCornerRound = 2;

	void ResetDevice() noexcept
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		const HRESULT result = g_device->Reset(&g_present);
		if (result == D3DERR_INVALIDCALL)
			IM_ASSERT(0);
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	bool CreateDevice() noexcept
	{
		g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
		if (!g_d3d)
			return false;

		g_present = {};
		g_present.Windowed = TRUE;
		g_present.SwapEffect = D3DSWAPEFFECT_DISCARD;
		g_present.BackBufferFormat = D3DFMT_UNKNOWN;
		g_present.EnableAutoDepthStencil = FALSE; // 2D UI only, no depth buffer needed
		g_present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

		// Fall back to software vertex processing on GPUs/VMs without hardware T&L.
		constexpr DWORD kVertexProcessing[] = { D3DCREATE_HARDWARE_VERTEXPROCESSING, D3DCREATE_SOFTWARE_VERTEXPROCESSING };
		for (const DWORD flags : kVertexProcessing)
		{
			if (SUCCEEDED(g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_window, flags, &g_present, &g_device)))
				return true;
		}
		return false;
	}

	void DestroyDevice() noexcept
	{
		if (g_device) { g_device->Release(); g_device = nullptr; }
		if (g_d3d) { g_d3d->Release(); g_d3d = nullptr; }
	}

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam))
			return true;

		switch (message)
		{
		case WM_NCHITTEST:
		{
			// Let Windows drag the window by our custom title bar.
			POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(window, &point);
			RECT client{};
			GetClientRect(window, &client);
			if (point.y >= 0 && point.y < g_dragHeight && point.x < client.right - g_dragRightInset)
				return HTCAPTION;
			return HTCLIENT;
		}

		case WM_NCLBUTTONDBLCLK:
			if (wParam == HTCAPTION)
				return 0; // fixed-size window: ignore double-click-to-maximize
			break;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				g_pendingWidth = LOWORD(lParam);
				g_pendingHeight = HIWORD(lParam);
			}
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // disable the ALT application menu
				return 0;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(window, message, wParam, lParam);
	}
}

bool platform::Create(const wchar_t* title, int width, int height, std::uint32_t backgroundRgb) noexcept
{
	ImGui_ImplWin32_EnableDpiAwareness();

	g_instance = GetModuleHandleW(nullptr);
	const BYTE red = (backgroundRgb >> 16) & 0xFF, green = (backgroundRgb >> 8) & 0xFF, blue = backgroundRgb & 0xFF;
	g_clearColor = D3DCOLOR_XRGB(red, green, blue);
	g_backgroundBrush = CreateSolidBrush(RGB(red, green, blue)); // no white flash before the first frame

	WNDCLASSEXW windowClass{};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = g_instance;
	windowClass.hIcon = LoadIconW(g_instance, MAKEINTRESOURCEW(1));
	windowClass.hIconSm = windowClass.hIcon;
	windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	windowClass.hbrBackground = g_backgroundBrush;
	windowClass.lpszClassName = kWindowClass;
	if (!RegisterClassExW(&windowClass))
		return false;

	// Open centred on the monitor the cursor is on, scaled for its DPI.
	POINT cursor{};
	GetCursorPos(&cursor);
	const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
	g_dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);

	MONITORINFO monitorInfo{ sizeof(monitorInfo) };
	GetMonitorInfoW(monitor, &monitorInfo);
	const RECT& work = monitorInfo.rcWork;

	const int scaledWidth = static_cast<int>(std::lround(width * g_dpiScale));
	const int scaledHeight = static_cast<int>(std::lround(height * g_dpiScale));
	const int x = work.left + (work.right - work.left - scaledWidth) / 2;
	const int y = work.top + (work.bottom - work.top - scaledHeight) / 2;

	g_window = CreateWindowExW(0, kWindowClass, title, WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
		x, y, scaledWidth, scaledHeight, nullptr, nullptr, g_instance, nullptr);
	if (!g_window)
		return false;

	// Windows 11: rounded corners + a subtle border that matches the theme. Ignored on Windows 10.
	const BOOL darkMode = TRUE;
	const COLORREF border = RGB(0x2A, 0x2E, 0x35);
	DwmSetWindowAttribute(g_window, kDwmUseDarkMode, &darkMode, sizeof(darkMode));
	DwmSetWindowAttribute(g_window, kDwmCornerPreference, &kDwmCornerRound, sizeof(kDwmCornerRound));
	DwmSetWindowAttribute(g_window, kDwmBorderColor, &border, sizeof(border));

	if (!CreateDevice())
	{
		DestroyDevice();
		DestroyWindow(g_window);
		g_window = nullptr;
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	ImGui_ImplWin32_Init(g_window);
	ImGui_ImplDX9_Init(g_device);
	return true; // shown later by Show(), once the height has been fitted
}

void platform::Show() noexcept
{
	ShowWindow(g_window, SW_SHOWDEFAULT);
	SetForegroundWindow(g_window);
}

void platform::FitHeight(int height) noexcept
{
	RECT rect{};
	GetWindowRect(g_window, &rect);
	const int currentHeight = rect.bottom - rect.top;
	if (height <= 0 || height == currentHeight)
		return;

	// Keep the window centred where it was.
	SetWindowPos(g_window, nullptr, rect.left, rect.top + (currentHeight - height) / 2,
		rect.right - rect.left, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void platform::Destroy() noexcept
{
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	DestroyDevice();

	if (g_window && IsWindow(g_window))
		DestroyWindow(g_window);
	g_window = nullptr;

	UnregisterClassW(kWindowClass, g_instance);

	if (g_backgroundBrush)
	{
		DeleteObject(g_backgroundBrush);
		g_backgroundBrush = nullptr;
	}
}

bool platform::PumpMessages() noexcept
{
	MSG message;
	while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
	{
		if (message.message == WM_QUIT)
			g_quit = true;
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return !g_quit;
}

bool platform::WaitForEvents(DWORD timeoutMs) noexcept
{
	return MsgWaitForMultipleObjectsEx(0, nullptr, timeoutMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0;
}

bool platform::BeginFrame() noexcept
{
	if (IsIconic(g_window))
		return false;

	if (g_deviceLost)
	{
		const HRESULT state = g_device->TestCooperativeLevel();
		if (state == D3DERR_DEVICELOST)
			return false;
		if (state == D3DERR_DEVICENOTRESET)
			ResetDevice();
		g_deviceLost = false;
	}

	if (g_pendingWidth && g_pendingHeight)
	{
		if (g_pendingWidth != g_present.BackBufferWidth || g_pendingHeight != g_present.BackBufferHeight)
		{
			g_present.BackBufferWidth = g_pendingWidth;
			g_present.BackBufferHeight = g_pendingHeight;
			ResetDevice();
		}
		g_pendingWidth = g_pendingHeight = 0;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	return true;
}

void platform::EndFrame() noexcept
{
	ImGui::EndFrame();
	ImGui::Render();

	g_device->SetRenderState(D3DRS_ZENABLE, FALSE);
	g_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	g_device->Clear(0, nullptr, D3DCLEAR_TARGET, g_clearColor, 1.0f, 0);

	if (SUCCEEDED(g_device->BeginScene()))
	{
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		g_device->EndScene();
	}

	if (g_device->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST)
		g_deviceLost = true;
}

HWND platform::Handle() noexcept { return g_window; }
float platform::DpiScale() noexcept { return g_dpiScale; }
bool platform::IsFocused() noexcept { return GetForegroundWindow() == g_window; }

void platform::SetDragArea(int height, int rightInset) noexcept
{
	g_dragHeight = height;
	g_dragRightInset = rightInset;
}

void platform::Minimize() noexcept { ShowWindow(g_window, SW_MINIMIZE); }
void platform::Close() noexcept { PostMessageW(g_window, WM_CLOSE, 0, 0); }
