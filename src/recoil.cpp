#include "recoil.h"

#include <timeapi.h>
#include <cmath>
#include <numbers>
#include <thread>

namespace
{
	std::thread g_worker;
	std::atomic<bool> g_running{ false };
	HWND g_window = nullptr;

	bool IsDown(int vk) noexcept
	{
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	void MoveMouse(LONG dx, LONG dy) noexcept
	{
		INPUT input{};
		input.type = INPUT_MOUSE;
		input.mi.dx = dx;
		input.mi.dy = dy;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		SendInput(1, &input, sizeof(INPUT));
	}

	void NotifyUi() noexcept
	{
		PostMessageW(g_window, WM_APP, 0, 0);
	}

	// Millisecond sleeps that are actually ~1 ms. Plain Sleep(1) is rounded up to
	// the 15.6 ms system tick, and Windows 11 ignores timeBeginPeriod for windows
	// that are hidden behind the game, so a high-resolution waitable timer is used.
	class Ticker
	{
	public:
		Ticker() noexcept
		{
			timer_ = CreateWaitableTimerExW(nullptr, nullptr,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
			if (!timer_)
			{
				timeBeginPeriod(1);
				usingFallback_ = true;
			}
		}

		~Ticker()
		{
			if (timer_)
				CloseHandle(timer_);
			if (usingFallback_)
				timeEndPeriod(1);
		}

		Ticker(const Ticker&) = delete;
		Ticker& operator=(const Ticker&) = delete;

		void Wait(int milliseconds) const noexcept
		{
			if (timer_)
			{
				LARGE_INTEGER due{};
				due.QuadPart = -static_cast<LONGLONG>(milliseconds) * 10'000; // relative, 100 ns units
				if (SetWaitableTimerEx(timer_, &due, 0, nullptr, nullptr, nullptr, 0))
				{
					WaitForSingleObject(timer_, INFINITE);
					return;
				}
			}
			Sleep(static_cast<DWORD>(milliseconds));
		}

	private:
		HANDLE timer_ = nullptr;
		bool usingFallback_ = false;
	};

	void Run() noexcept
	{
		using recoil::enabled;

		const Ticker ticker;

		LARGE_INTEGER frequency{}, last{}, now{};
		QueryPerformanceFrequency(&frequency);

		double carryX = 0.0, carryY = 0.0; // sub-pixel remainder, keeps the angle exact
		bool wasPulling = false;
		int lastKey = -1;
		bool keyWasDown = false;

		while (g_running.load(std::memory_order_relaxed))
		{
			// Toggle hotkey (edge triggered). When the key is rebound, start from its
			// current state so the key press that set it does not also toggle.
			const int key = recoil::toggleKey.load(std::memory_order_relaxed);
			const bool keyDown = key != 0 && IsDown(key);
			if (key != lastKey)
			{
				lastKey = key;
				keyWasDown = keyDown;
			}

			const HWND foreground = GetForegroundWindow();
			const bool typingInUi = foreground == g_window && recoil::suppressHotkey.load(std::memory_order_relaxed);

			if (keyDown && !keyWasDown && !typingInUi)
			{
				enabled.store(!enabled.load());
				NotifyUi();
			}
			keyWasDown = keyDown;

			// Pull only while firing, and never while the user is clicking around in our own window.
			const bool isEnabled = enabled.load(std::memory_order_relaxed);
			const bool shouldPull = isEnabled && foreground != g_window && IsDown(VK_LBUTTON);

			if (shouldPull)
			{
				QueryPerformanceCounter(&now);
				if (!wasPulling)
				{
					last = now;
					carryX = carryY = 0.0;
				}

				double dt = static_cast<double>(now.QuadPart - last.QuadPart) / static_cast<double>(frequency.QuadPart);
				if (dt > 0.05) dt = 0.05; // never jump if the thread was stalled
				last = now;

				const double speed = recoil::strength.load(std::memory_order_relaxed) * recoil::kPixelsPerSecondPerStrength;
				const double radians = recoil::angle.load(std::memory_order_relaxed) * (std::numbers::pi / 180.0);

				carryX += std::cos(radians) * speed * dt;
				carryY += std::sin(radians) * speed * dt;

				const auto moveX = static_cast<LONG>(carryX); // truncates toward zero, keeps the sign
				const auto moveY = static_cast<LONG>(carryY);
				if (moveX != 0 || moveY != 0)
				{
					MoveMouse(moveX, moveY);
					carryX -= moveX;
					carryY -= moveY;
				}
			}

			if (shouldPull != wasPulling)
			{
				recoil::pulling.store(shouldPull);
				NotifyUi();
				wasPulling = shouldPull;
			}

			// 1 ms resolution while armed, relaxed polling while only the hotkey matters.
			ticker.Wait(isEnabled ? 1 : 10);
		}

		recoil::pulling.store(false);
	}
}

void recoil::Start(HWND notifyWindow) noexcept
{
	if (g_running.exchange(true))
		return;

	g_window = notifyWindow;
	g_worker = std::thread(Run);
}

void recoil::Stop() noexcept
{
	if (!g_running.exchange(false))
		return;

	if (g_worker.joinable())
		g_worker.join();
}
