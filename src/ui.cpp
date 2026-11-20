#include "ui.h"

#include "config.h"
#include "platform.h"
#include "recoil.h"

#include "../imgui/imgui.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>

namespace
{
	// ------------------------------------------------------------------ theme

	constexpr std::uint32_t kBg = ui::kBackground;
	constexpr std::uint32_t kSurface = 0x181B20;
	constexpr std::uint32_t kRaised = 0x22262D;
	constexpr std::uint32_t kHover = 0x2C3139;
	constexpr std::uint32_t kPressed = 0x353B44;
	constexpr std::uint32_t kBorder = 0x2A2E35;
	constexpr std::uint32_t kText = 0xE8EAED;
	constexpr std::uint32_t kTextDim = 0x8C939D;
	constexpr std::uint32_t kAccent = 0xF5A524;
	constexpr std::uint32_t kAccentHover = 0xFFB845;
	constexpr std::uint32_t kAccentText = 0x1A1408;
	constexpr std::uint32_t kGreen = 0x3DD68C;
	constexpr std::uint32_t kRed = 0xE5484D;

	constexpr float kTitleHeight = 44.0f;
	constexpr float kOuterPad = 14.0f;
	constexpr float kCardPad = 14.0f;
	constexpr float kCardGap = 10.0f;
	constexpr float kRounding = 10.0f;

	float g_scale = 1.0f;
	ImFont* g_fontBody = nullptr;
	ImFont* g_fontBold = nullptr;
	ImFont* g_fontLarge = nullptr;

	float S(float value) { return value * g_scale; }

	ImVec4 Rgb(std::uint32_t hex, float alpha = 1.0f)
	{
		return { ((hex >> 16) & 0xFF) / 255.0f, ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f, alpha };
	}

	ImU32 Col(std::uint32_t hex, float alpha = 1.0f)
	{
		return ImGui::ColorConvertFloat4ToU32(Rgb(hex, alpha));
	}

	ImU32 Mix(std::uint32_t from, std::uint32_t to, float t)
	{
		const ImVec4 a = Rgb(from), b = Rgb(to);
		return ImGui::ColorConvertFloat4ToU32({ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1.0f });
	}

	ImVec2 operator+(ImVec2 a, ImVec2 b) { return { a.x + b.x, a.y + b.y }; }
	ImVec2 operator-(ImVec2 a, ImVec2 b) { return { a.x - b.x, a.y - b.y }; }
	ImVec2 operator*(ImVec2 a, float s) { return { a.x * s, a.y * s }; }

	void LoadFonts()
	{
		ImGuiIO& io = ImGui::GetIO();

		char windowsDir[MAX_PATH]{};
		GetWindowsDirectoryA(windowsDir, MAX_PATH);

		const auto load = [&](const char* file, float size) -> ImFont*
		{
			const std::string path = std::string(windowsDir) + "\\Fonts\\" + file;
			if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
				return nullptr;
			return io.Fonts->AddFontFromFileTTF(path.c_str(), S(size));
		};

		g_fontBody = load("segoeui.ttf", 15.0f);
		if (!g_fontBody)
		{
			ImFontConfig fallback;
			fallback.SizePixels = S(13.0f);
			g_fontBody = io.Fonts->AddFontDefault(&fallback);
		}

		g_fontBold = load("seguisb.ttf", 15.0f);
		if (!g_fontBold)
			g_fontBold = g_fontBody;

		g_fontLarge = load("seguisb.ttf", 19.0f);
		if (!g_fontLarge)
			g_fontLarge = g_fontBold;

		io.FontDefault = g_fontBody;
	}

	void ApplyTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style = ImGuiStyle();

		style.WindowPadding = { 0.0f, 0.0f };
		style.FramePadding = { 10.0f, 7.0f };
		style.ItemSpacing = { 8.0f, 8.0f };
		style.ItemInnerSpacing = { 6.0f, 6.0f };
		style.WindowRounding = 0.0f;
		style.ChildRounding = kRounding;
		style.FrameRounding = 6.0f;
		style.PopupRounding = 8.0f;
		style.GrabRounding = 6.0f;
		style.ScrollbarRounding = 6.0f;
		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.ScrollbarSize = 10.0f;
		style.ScaleAllSizes(g_scale);

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = Rgb(kText);
		colors[ImGuiCol_TextDisabled] = Rgb(kTextDim);
		colors[ImGuiCol_WindowBg] = Rgb(kBg);
		colors[ImGuiCol_ChildBg] = Rgb(kSurface);
		colors[ImGuiCol_PopupBg] = Rgb(kRaised);
		colors[ImGuiCol_Border] = Rgb(kBorder);
		colors[ImGuiCol_BorderShadow] = { 0, 0, 0, 0 };
		colors[ImGuiCol_FrameBg] = Rgb(kRaised);
		colors[ImGuiCol_FrameBgHovered] = Rgb(kHover);
		colors[ImGuiCol_FrameBgActive] = Rgb(kHover);
		colors[ImGuiCol_Button] = Rgb(kRaised);
		colors[ImGuiCol_ButtonHovered] = Rgb(kHover);
		colors[ImGuiCol_ButtonActive] = Rgb(kPressed);
		colors[ImGuiCol_Header] = Rgb(kAccent, 0.16f);
		colors[ImGuiCol_HeaderHovered] = Rgb(kHover);
		colors[ImGuiCol_HeaderActive] = Rgb(kAccent, 0.26f);
		colors[ImGuiCol_CheckMark] = Rgb(kAccent);
		colors[ImGuiCol_SliderGrab] = Rgb(kAccent);
		colors[ImGuiCol_SliderGrabActive] = Rgb(kAccentHover);
		colors[ImGuiCol_Separator] = Rgb(kBorder);
		colors[ImGuiCol_ScrollbarBg] = { 0, 0, 0, 0 };
		colors[ImGuiCol_ScrollbarGrab] = Rgb(kHover);
		colors[ImGuiCol_ScrollbarGrabHovered] = Rgb(kPressed);
		colors[ImGuiCol_ScrollbarGrabActive] = Rgb(kPressed);
		colors[ImGuiCol_TextSelectedBg] = Rgb(kAccent, 0.35f);
		colors[ImGuiCol_NavHighlight] = Rgb(kAccent);
		colors[ImGuiCol_ModalWindowDimBg] = { 0, 0, 0, 0.5f };
	}

	// ------------------------------------------------------------------ keys

	std::string KeyName(int vk)
	{
		switch (vk)
		{
		case 0: return "None";
		case VK_LBUTTON: return "Mouse 1";
		case VK_RBUTTON: return "Mouse 2";
		case VK_MBUTTON: return "Mouse 3";
		case VK_XBUTTON1: return "Mouse 4";
		case VK_XBUTTON2: return "Mouse 5";
		}

		const UINT scan = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC_EX);
		LONG keyParam = static_cast<LONG>(scan & 0xFF) << 16;
		if ((scan & 0xFF00) == 0xE000 || (scan & 0xFF00) == 0xE100)
			keyParam |= 1 << 24; // extended key (arrows, Insert, Home, ...)

		wchar_t wide[64]{};
		if (scan != 0 && GetKeyNameTextW(keyParam, wide, 64) > 0)
		{
			char utf8[128]{};
			if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), nullptr, nullptr) > 0)
				return utf8;
		}

		char fallback[16];
		std::snprintf(fallback, sizeof(fallback), "Key 0x%02X", vk);
		return fallback;
	}

	// First key currently held, for rebinding. Left/right mouse are skipped (fire / aim),
	// as are the left/right modifier variants so the generic Shift/Ctrl/Alt get bound.
	int PollPressedKey()
	{
		for (int vk = 1; vk < 0xFF; ++vk)
		{
			if (vk == VK_LBUTTON || vk == VK_RBUTTON || (vk >= VK_LSHIFT && vk <= VK_RMENU))
				continue;
			if (GetAsyncKeyState(vk) & 0x8000)
				return vk;
		}
		return 0;
	}

	// ------------------------------------------------------------------ cards

	struct Card
	{
		ImVec2 min;
		float width = 0.0f;
	} g_card;

	float CardInnerWidth() { return g_card.width - 2.0f * S(kCardPad); }
	float CardInnerRight() { return g_card.min.x + g_card.width - S(kCardPad); }

	// Cards draw their background after their contents so they size to fit.
	void BeginCard(float width)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);
		drawList->ChannelsSetCurrent(1);

		g_card.min = ImGui::GetCursorScreenPos();
		g_card.width = width;
		ImGui::SetCursorScreenPos(g_card.min + ImVec2{ S(kCardPad), S(kCardPad) });
		ImGui::BeginGroup();
	}

	void EndCard()
	{
		ImGui::EndGroup();
		const ImVec2 max{ g_card.min.x + g_card.width, ImGui::GetItemRectMax().y + S(kCardPad) };

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSetCurrent(0);
		drawList->AddRectFilled(g_card.min, max, Col(kSurface), S(kRounding));
		drawList->AddRect(g_card.min, max, Col(kBorder), S(kRounding), 0, S(1.0f));
		drawList->ChannelsMerge();

		ImGui::SetCursorScreenPos({ g_card.min.x, max.y + S(kCardGap) });
	}

	void CardHeader(const char* title, const char* detail = nullptr)
	{
		const float y = ImGui::GetCursorScreenPos().y;
		ImGui::PushFont(g_fontBold);
		ImGui::TextUnformatted(title);
		ImGui::PopFont();

		if (detail)
		{
			const float width = ImGui::CalcTextSize(detail).x;
			ImGui::GetWindowDrawList()->AddText({ CardInnerRight() - width, y }, Col(kTextDim), detail);
		}
		ImGui::Dummy({ 0.0f, S(2.0f) });
	}

	// ------------------------------------------------------------------ widgets

	bool PrimaryButton(const char* label, ImVec2 size)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, Rgb(kAccent));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Rgb(kAccentHover));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Rgb(kAccent));
		ImGui::PushStyleColor(ImGuiCol_Text, Rgb(kAccentText));
		ImGui::PushFont(g_fontBold);
		const bool pressed = ImGui::Button(label, size);
		ImGui::PopFont();
		ImGui::PopStyleColor(4);
		return pressed;
	}

	bool ToggleSwitch(const char* id, bool on, bool& animating)
	{
		const ImVec2 size{ S(46.0f), S(26.0f) };
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImGuiID stateId = ImGui::GetID(id);
		const bool clicked = ImGui::InvisibleButton(id, size);
		const bool hovered = ImGui::IsItemHovered();

		ImGuiStorage* storage = ImGui::GetStateStorage();
		const float target = on ? 1.0f : 0.0f;
		float t = storage->GetFloat(stateId, target);
		const float step = ImGui::GetIO().DeltaTime * 9.0f;
		t = t < target ? std::min(target, t + step) : std::max(target, t - step);
		storage->SetFloat(stateId, t);
		if (t != target)
			animating = true;

		const float radius = size.y * 0.5f;
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(pos, pos + size, Mix(hovered ? kPressed : kHover, kGreen, t), radius);

		const float knobX = pos.x + radius + t * (size.x - 2.0f * radius);
		drawList->AddCircleFilled({ knobX, pos.y + radius }, radius - S(3.0f), Col(on ? 0xFFFFFF : kText), 24);

		if (hovered)
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		return clicked;
	}

	// Flat slider: label + value on one line, track underneath. Mouse wheel nudges by 1.
	bool Slider(const char* label, int& value, int minimum, int maximum, const char* format, float width)
	{
		ImGui::PushID(label);

		const ImVec2 headerPos = ImGui::GetCursorScreenPos();
		ImGui::TextColored(Rgb(kTextDim), "%s", label);

		char valueText[32];
		std::snprintf(valueText, sizeof(valueText), format, value);
		ImGui::PushFont(g_fontBold);
		const float valueWidth = ImGui::CalcTextSize(valueText).x;
		ImGui::GetWindowDrawList()->AddText(g_fontBold, g_fontBold->FontSize,
			{ headerPos.x + width - valueWidth, headerPos.y }, Col(kText), valueText);
		ImGui::PopFont();

		const float height = S(20.0f);
		const float knobRadius = S(7.0f);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##track", { width, height });
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();

		const float left = pos.x + knobRadius;
		const float right = pos.x + width - knobRadius;
		int next = value;

		if (active)
		{
			const float t = std::clamp((ImGui::GetIO().MousePos.x - left) / (right - left), 0.0f, 1.0f);
			next = minimum + static_cast<int>(std::lround(t * static_cast<float>(maximum - minimum)));
		}
		else if (hovered && ImGui::GetIO().MouseWheel != 0.0f)
		{
			next = std::clamp(value + (ImGui::GetIO().MouseWheel > 0.0f ? 1 : -1), minimum, maximum);
		}

		const bool changed = next != value;
		value = next;

		const float t = static_cast<float>(value - minimum) / static_cast<float>(maximum - minimum);
		const float knobX = left + t * (right - left);
		const float centerY = pos.y + height * 0.5f;
		const float trackHalf = S(2.0f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled({ left, centerY - trackHalf }, { right, centerY + trackHalf }, Col(kHover), trackHalf);
		drawList->AddRectFilled({ left, centerY - trackHalf }, { knobX, centerY + trackHalf }, Col(kAccent), trackHalf);
		if (hovered || active)
			drawList->AddCircleFilled({ knobX, centerY }, knobRadius + S(4.0f), Col(kAccent, 0.18f), 24);
		drawList->AddCircleFilled({ knobX, centerY }, knobRadius, Col(kText), 24);
		drawList->AddCircleFilled({ knobX, centerY }, knobRadius - S(3.0f), Col(kAccent), 24);

		if (hovered || active)
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImGui::PopID();
		return changed;
	}

	// Round dial showing the pull direction; the arrow grows with strength.
	// Drag to aim, scroll to nudge, hold Shift to snap to 5 degrees.
	bool AngleDial(int& angle, float strength01, float diameter)
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##dial", { diameter, diameter });
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		const ImGuiIO& io = ImGui::GetIO();

		const float radius = diameter * 0.5f;
		const ImVec2 center = pos + ImVec2{ radius, radius };
		int next = angle;

		if (active)
		{
			const float dx = io.MousePos.x - center.x;
			const float dy = io.MousePos.y - center.y;
			if (dx * dx + dy * dy > S(4.0f) * S(4.0f))
			{
				next = static_cast<int>(std::lround(std::atan2(dy, dx) * 180.0 / std::numbers::pi));
				if (io.KeyShift)
					next = static_cast<int>(std::lround(next / 5.0)) * 5;
				next = ((next % 360) + 360) % 360;
			}
		}
		else if (hovered && io.MouseWheel != 0.0f)
		{
			next = ((angle + (io.MouseWheel > 0.0f ? 1 : -1)) % 360 + 360) % 360;
		}

		const bool changed = next != angle;
		angle = next;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddCircleFilled(center, radius, Col(kRaised), 64);
		drawList->AddCircle(center, radius - S(0.5f), Col(hovered || active ? kPressed : kBorder), 64, S(1.0f));
		drawList->AddCircle(center, radius * 0.58f, Col(kBorder), 48, S(1.0f));

		// Crosshair + 45 degree ticks.
		for (int tick = 0; tick < 8; ++tick)
		{
			const float a = static_cast<float>(tick) * static_cast<float>(std::numbers::pi) / 4.0f;
			const ImVec2 dir{ std::cos(a), std::sin(a) };
			const bool major = tick % 2 == 0;
			drawList->AddLine(center + dir * (radius * (major ? 0.80f : 0.86f)), center + dir * (radius * 0.94f),
				Col(major ? kTextDim : kBorder), S(1.0f));
		}

		const float radians = static_cast<float>(angle) * static_cast<float>(std::numbers::pi) / 180.0f;
		const ImVec2 dir{ std::cos(radians), std::sin(radians) };
		const ImVec2 normal{ -dir.y, dir.x };
		const float length = radius * (0.28f + 0.46f * strength01);
		const ImVec2 tip = center + dir * length;

		drawList->AddLine(center, tip, Col(kAccent), S(3.0f));
		drawList->AddTriangleFilled(tip + dir * S(9.0f), tip + normal * S(6.0f), tip - normal * S(6.0f), Col(kAccent));
		drawList->AddCircleFilled(center, S(4.5f), Col(kAccent), 16);

		if (hovered || active)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			if (!active)
				ImGui::SetTooltip("Drag to set the pull direction\nScroll to nudge, hold Shift to snap");
		}
		return changed;
	}

	bool WindowButton(const char* id, ImVec2 pos, ImVec2 size, bool isClose)
	{
		ImGui::SetCursorScreenPos(pos);
		const bool pressed = ImGui::InvisibleButton(id, size);
		const bool hovered = ImGui::IsItemHovered();
		const bool held = ImGui::IsItemActive();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (hovered)
			drawList->AddRectFilled(pos, pos + size, isClose ? Col(kRed, held ? 0.8f : 1.0f) : Col(held ? kPressed : kHover));

		const ImVec2 center = pos + size * 0.5f;
		const float half = S(5.0f);
		const ImU32 iconColor = Col(hovered ? 0xFFFFFF : kTextDim);
		if (isClose)
		{
			drawList->AddLine(center - ImVec2{ half, half }, center + ImVec2{ half, half }, iconColor, S(1.3f));
			drawList->AddLine(center + ImVec2{ -half, half }, center + ImVec2{ half, -half }, iconColor, S(1.3f));
		}
		else
		{
			drawList->AddLine(center - ImVec2{ half, 0.0f }, center + ImVec2{ half, 0.0f }, iconColor, S(1.3f));
		}
		return pressed;
	}

	// ------------------------------------------------------------------ sections

	bool g_rebinding = false;
	int g_selectedPreset = -1;
	char g_presetName[64] = "";

	void TitleBar(float width)
	{
		const float height = S(kTitleHeight);
		const ImVec2 origin = ImGui::GetWindowPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		drawList->AddLine({ origin.x, origin.y + height - 1.0f }, { origin.x + width, origin.y + height - 1.0f }, Col(kBorder), 1.0f);

		// Logo: amber tile with a crosshair.
		const float logoSize = S(22.0f);
		const ImVec2 logoMin{ origin.x + S(kOuterPad), origin.y + (height - logoSize) * 0.5f };
		const ImVec2 logoCenter = logoMin + ImVec2{ logoSize * 0.5f, logoSize * 0.5f };
		drawList->AddRectFilled(logoMin, logoMin + ImVec2{ logoSize, logoSize }, Col(kAccent), S(6.0f));
		drawList->AddCircle(logoCenter, S(5.5f), Col(kAccentText), 24, S(1.6f));
		drawList->AddLine({ logoCenter.x, logoMin.y + S(3.5f) }, { logoCenter.x, logoMin.y + logoSize - S(3.5f) }, Col(kAccentText), S(1.6f));
		drawList->AddLine({ logoMin.x + S(3.5f), logoCenter.y }, { logoMin.x + logoSize - S(3.5f), logoCenter.y }, Col(kAccentText), S(1.6f));

		ImGui::SetCursorScreenPos({ logoMin.x + logoSize + S(10.0f), origin.y + (height - g_fontBold->FontSize) * 0.5f });
		ImGui::PushFont(g_fontBold);
		ImGui::TextUnformatted("R6 Recoil Utility");
		ImGui::PopFont();

		const ImVec2 buttonSize{ S(46.0f), height - 1.0f };
		if (WindowButton("##minimize", { origin.x + width - 2.0f * buttonSize.x, origin.y }, buttonSize, false))
			platform::Minimize();
		if (WindowButton("##close", { origin.x + width - buttonSize.x, origin.y }, buttonSize, true))
			platform::Close();

		platform::SetDragArea(static_cast<int>(height), static_cast<int>(2.0f * buttonSize.x));
	}

	void StatusCard(float width, bool& animating)
	{
		BeginCard(width);

		const bool enabled = recoil::enabled.load();
		const bool pulling = recoil::pulling.load();
		const int key = recoil::toggleKey.load();
		const std::string keyName = KeyName(key);

		// State line.
		const ImVec2 top = ImGui::GetCursorScreenPos();
		const std::uint32_t stateColor = pulling ? kAccent : enabled ? kGreen : kTextDim;
		const float dotRadius = S(4.5f);
		ImGui::GetWindowDrawList()->AddCircleFilled(
			{ top.x + dotRadius, top.y + g_fontLarge->FontSize * 0.55f }, dotRadius, Col(stateColor), 16);

		ImGui::SetCursorScreenPos({ top.x + dotRadius * 2.0f + S(9.0f), top.y });
		ImGui::PushFont(g_fontLarge);
		ImGui::TextUnformatted(pulling ? "Pulling" : enabled ? "Active" : "Off");
		ImGui::PopFont();

		ImGui::SetCursorScreenPos({ top.x, ImGui::GetCursorScreenPos().y - S(4.0f) });
		if (enabled)
			ImGui::TextColored(Rgb(kTextDim), "Hold left mouse in game to pull");
		else if (key != 0)
			ImGui::TextColored(Rgb(kTextDim), "Press %s or flip the switch to start", keyName.c_str());
		else
			ImGui::TextColored(Rgb(kTextDim), "Flip the switch to start");
		const float blockBottom = ImGui::GetItemRectMax().y;

		// Switch, vertically centred on the two lines.
		const ImVec2 afterText = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos({ CardInnerRight() - S(46.0f), top.y + (blockBottom - top.y - S(26.0f)) * 0.5f });
		if (ToggleSwitch("##enabled", enabled, animating))
			recoil::enabled = !enabled;
		ImGui::SetCursorScreenPos(afterText);

		// Divider.
		ImGui::Dummy({ 0.0f, S(2.0f) });
		const float dividerY = ImGui::GetCursorScreenPos().y;
		ImGui::GetWindowDrawList()->AddLine({ g_card.min.x + 1.0f, dividerY }, { g_card.min.x + g_card.width - 1.0f, dividerY }, Col(kBorder), 1.0f);
		ImGui::Dummy({ 0.0f, S(2.0f) });

		// Hotkey row.
		const float rowY = ImGui::GetCursorScreenPos().y;
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Toggle key");

		const float buttonWidth = S(130.0f);
		ImGui::SetCursorScreenPos({ CardInnerRight() - buttonWidth, rowY });
		if (g_rebinding)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, Rgb(kAccent, 0.20f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Rgb(kAccent, 0.26f));
			ImGui::PushStyleColor(ImGuiCol_Text, Rgb(kAccent));
			ImGui::Button("Press a key...", { buttonWidth, 0.0f });
			ImGui::PopStyleColor(3);
		}
		else
		{
			ImGui::PushFont(g_fontBold);
			if (ImGui::Button((keyName + "##hotkey").c_str(), { buttonWidth, 0.0f }))
				g_rebinding = true;
			ImGui::PopFont();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(g_rebinding ? "Esc cancels, Backspace clears" : "Click to change the toggle key");

		if (g_rebinding)
		{
			if (!platform::IsFocused())
			{
				g_rebinding = false;
			}
			else if (const int pressed = PollPressedKey())
			{
				if (pressed != VK_ESCAPE)
				{
					recoil::toggleKey = pressed == VK_BACK ? 0 : pressed;
					config::Save();
				}
				g_rebinding = false;
			}
		}

		EndCard();
	}

	void PatternCard(float width)
	{
		BeginCard(width);
		CardHeader("Pattern", "Scroll over a control to fine-tune");

		int strength = recoil::strength.load();
		int angle = recoil::angle.load();
		bool released = false;

		const float dialSize = S(108.0f);
		const float strength01 = static_cast<float>(strength - recoil::kMinStrength) /
			static_cast<float>(recoil::kMaxStrength - recoil::kMinStrength);

		const ImVec2 rowStart = ImGui::GetCursorScreenPos();
		if (AngleDial(angle, strength01, dialSize))
			recoil::angle = angle;
		released |= ImGui::IsItemDeactivated();

		// Sliders, vertically centred next to the dial.
		const float gap = S(18.0f);
		const float sliderWidth = CardInnerWidth() - dialSize - gap;
		const float slidersHeight = 2.0f * (ImGui::GetTextLineHeight() + S(20.0f)) + 3.0f * ImGui::GetStyle().ItemSpacing.y;
		ImGui::SetCursorScreenPos({ rowStart.x + dialSize + gap, rowStart.y + (dialSize - slidersHeight) * 0.5f });
		ImGui::BeginGroup();

		if (Slider("Strength", strength, recoil::kMinStrength, recoil::kMaxStrength, "%d", sliderWidth))
			recoil::strength = strength;
		released |= ImGui::IsItemDeactivated();

		ImGui::Dummy({ 0.0f, 0.0f });
		if (Slider("Angle", angle, 0, 359, "%d\xC2\xB0", sliderWidth))
			recoil::angle = angle;
		released |= ImGui::IsItemDeactivated();

		ImGui::EndGroup();
		ImGui::SetCursorScreenPos({ rowStart.x, std::max(rowStart.y + dialSize, ImGui::GetItemRectMax().y) });
		ImGui::Dummy({ 0.0f, 0.0f });

		if (released)
			config::Save();

		EndCard();
	}

	void SelectPreset(int index)
	{
		g_selectedPreset = index;
		const auto& preset = config::presets[static_cast<size_t>(index)];
		recoil::strength = preset.strength;
		recoil::angle = preset.angle;
		std::snprintf(g_presetName, sizeof(g_presetName), "%s", preset.name.c_str());
	}

	void SavePreset()
	{
		std::string name = g_presetName;
		name.erase(0, name.find_first_not_of(" \t"));
		name.erase(name.find_last_not_of(" \t") + 1);
		if (name.empty())
			return;

		auto& presets = config::presets;
		const auto existing = std::find_if(presets.begin(), presets.end(),
			[&](const config::Preset& preset) { return preset.name == name; });

		if (existing != presets.end())
		{
			existing->strength = recoil::strength;
			existing->angle = recoil::angle;
			g_selectedPreset = static_cast<int>(existing - presets.begin());
		}
		else
		{
			presets.push_back({ name, recoil::strength, recoil::angle });
			g_selectedPreset = static_cast<int>(presets.size()) - 1;
		}
		config::Save();
	}

	void PresetsCard(float width)
	{
		auto& presets = config::presets;
		if (g_selectedPreset >= static_cast<int>(presets.size()))
			g_selectedPreset = -1;

		BeginCard(width);

		char count[32];
		std::snprintf(count, sizeof(count), "%zu saved", presets.size());
		CardHeader("Presets", count);

		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float buttonWidth = S(78.0f);
		const float fieldWidth = CardInnerWidth() - buttonWidth - spacing;

		// Row 1: pick / delete.
		std::string preview = "Choose a preset";
		if (g_selectedPreset >= 0)
		{
			const auto& selected = presets[static_cast<size_t>(g_selectedPreset)];
			preview = selected.name;
			if (selected.strength != recoil::strength || selected.angle != recoil::angle)
				preview += "  (edited)";
		}
		else if (presets.empty())
		{
			preview = "No presets yet";
		}

		const ImVec2 comboMin = ImGui::GetCursorScreenPos();
		const ImVec2 comboMax = comboMin + ImVec2{ fieldWidth, ImGui::GetFrameHeight() };
		ImDrawList* cardDrawList = ImGui::GetWindowDrawList();

		ImGui::SetNextItemWidth(fieldWidth);
		const bool comboOpen = ImGui::BeginCombo("##preset", preview.c_str(), ImGuiComboFlags_NoArrowButton);

		// Slim chevron instead of ImGui's solid arrow.
		const ImVec2 chevron{ comboMax.x - S(16.0f), (comboMin.y + comboMax.y) * 0.5f };
		const ImVec2 chevronPoints[] = {
			chevron + ImVec2{ -S(4.0f), -S(2.0f) }, chevron + ImVec2{ 0.0f, S(2.0f) }, chevron + ImVec2{ S(4.0f), -S(2.0f) } };
		cardDrawList->AddPolyline(chevronPoints, 3, Col(kTextDim), 0, S(1.5f));

		if (comboOpen)
		{
			for (int i = 0; i < static_cast<int>(presets.size()); ++i)
			{
				const auto& preset = presets[static_cast<size_t>(i)];
				ImGui::PushID(i);
				const float rowY = ImGui::GetCursorScreenPos().y;
				if (ImGui::Selectable(preset.name.c_str(), i == g_selectedPreset))
					SelectPreset(i);

				char values[32];
				std::snprintf(values, sizeof(values), "%d  \xC2\xB7  %d\xC2\xB0", preset.strength, preset.angle);
				const float valuesWidth = ImGui::CalcTextSize(values).x;
				const float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
				ImGui::GetWindowDrawList()->AddText({ right - valuesWidth, rowY }, Col(kTextDim), values);
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(g_selectedPreset < 0);
		if (ImGui::Button("Delete", { buttonWidth, 0.0f }))
		{
			presets.erase(presets.begin() + g_selectedPreset);
			g_selectedPreset = -1;
			g_presetName[0] = '\0';
			config::Save();
		}
		ImGui::EndDisabled();

		// Row 2: name / save.
		ImGui::SetNextItemWidth(fieldWidth);
		const bool submitted = ImGui::InputTextWithHint("##name", "Preset name, e.g. Ash", g_presetName,
			sizeof(g_presetName), ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::SameLine();
		const bool hasName = g_presetName[0] != '\0';
		ImGui::BeginDisabled(!hasName);
		const bool save = PrimaryButton("Save", { buttonWidth, 0.0f });
		ImGui::EndDisabled();

		if ((submitted || save) && hasName)
			SavePreset();

		EndCard();
	}

	int g_requiredHeight = 0;

	void Footer(ImVec2 display)
	{
		// Content ends at the cursor (just below the last card's gap).
		g_requiredHeight = static_cast<int>(std::ceil(ImGui::GetCursorScreenPos().y + ImGui::GetTextLineHeight() + S(12.0f)));

		const float y = display.y - S(12.0f) - ImGui::GetTextLineHeight();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddText({ S(kOuterPad) + S(2.0f), y }, Col(kTextDim), "v2.0");

		const char* credit = "by skai";
		const float width = ImGui::CalcTextSize(credit).x;
		drawList->AddText({ display.x - S(kOuterPad) - S(2.0f) - width, y }, Col(kTextDim), credit);
	}
}

void ui::Init(float dpiScale) noexcept
{
	g_scale = dpiScale > 0.0f ? dpiScale : 1.0f;
	LoadFonts();
	ApplyTheme();
}

bool ui::Render() noexcept
{
	bool animating = false;
	const ImGuiIO& io = ImGui::GetIO();
	const ImVec2 display = io.DisplaySize;

	ImGui::SetNextWindowPos({ 0.0f, 0.0f });
	ImGui::SetNextWindowSize(display);
	ImGui::Begin("##root", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);

	TitleBar(display.x);

	const float width = display.x - 2.0f * S(kOuterPad);
	ImGui::SetCursorScreenPos({ S(kOuterPad), S(kTitleHeight) + S(kOuterPad) });
	StatusCard(width, animating);
	PatternCard(width);
	PresetsCard(width);
	Footer(display);

	ImGui::End();

	// Don't let the hotkey fire while typing a preset name or picking a new key.
	recoil::suppressHotkey = io.WantTextInput || g_rebinding;

	return animating || g_rebinding || io.WantTextInput;
}

int ui::RequiredHeight() noexcept
{
	return g_requiredHeight;
}
