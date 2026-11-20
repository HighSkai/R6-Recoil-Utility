#include "config.h"
#include "platform.h"
#include "recoil.h"
#include "ui.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	// One instance only: two copies running would pull twice as hard.
	const HANDLE instanceLock = CreateMutexW(nullptr, FALSE, L"Local\\R6RecoilUtility");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if (const HWND existing = FindWindowW(platform::kWindowClass, nullptr))
		{
			ShowWindow(existing, SW_RESTORE);
			SetForegroundWindow(existing);
		}
		if (instanceLock)
			CloseHandle(instanceLock);
		return EXIT_SUCCESS;
	}

	config::Load();

	if (!platform::Create(L"R6 Recoil Utility", ui::kWidth, ui::kHeight, ui::kBackground))
	{
		MessageBoxW(nullptr, L"Could not create the window / Direct3D 9 device.", L"R6 Recoil Utility", MB_ICONERROR);
		platform::Destroy();
		return EXIT_FAILURE;
	}

	ui::Init(platform::DpiScale());

	// Lay out one frame while hidden, fit the window to it, then show it.
	if (platform::BeginFrame())
	{
		ui::Render();
		platform::EndFrame();
	}
	platform::FitHeight(ui::RequiredHeight());

	recoil::Start(platform::Handle());

	// Event-driven loop: ~60 fps right after input or while animating, then idle.
	// The recoil thread posts a message on every state change, so the status
	// still updates instantly while the window sits behind the game.
	ULONGLONG lastEvent = GetTickCount64();
	bool shown = false;
	while (platform::PumpMessages())
	{
		bool animating = false;
		if (platform::BeginFrame())
		{
			animating = ui::Render();
			platform::EndFrame();
		}

		if (!shown)
		{
			platform::Show();
			shown = true;
		}

		DWORD timeout = 1000;
		if (animating || GetTickCount64() - lastEvent < 500)
			timeout = 16;
		else if (platform::IsFocused())
			timeout = 250;

		if (platform::WaitForEvents(timeout))
			lastEvent = GetTickCount64();
	}

	recoil::Stop();
	config::Save();
	platform::Destroy();

	if (instanceLock)
		CloseHandle(instanceLock);
	return EXIT_SUCCESS;
}
