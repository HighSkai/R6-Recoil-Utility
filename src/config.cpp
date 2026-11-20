#include "config.h"
#include "recoil.h"

#include <Windows.h>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

namespace
{
	std::filesystem::path ConfigPath()
	{
		std::wstring buffer(MAX_PATH, L'\0');
		for (;;)
		{
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0)
				return L"R6RecoilUtility.ini";
			if (length < buffer.size())
			{
				buffer.resize(length);
				break;
			}
			buffer.resize(buffer.size() * 2);
		}
		return std::filesystem::path(buffer).replace_filename(L"R6RecoilUtility.ini");
	}

	std::string_view Trim(std::string_view text)
	{
		const auto first = text.find_first_not_of(" \t\r\n");
		if (first == std::string_view::npos)
			return {};
		const auto last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	std::optional<int> ParseInt(std::string_view text)
	{
		text = Trim(text);
		int value = 0;
		const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
		if (error != std::errc{} || end != text.data() + text.size())
			return std::nullopt;
		return value;
	}

	int ClampStrength(int value) { return std::clamp(value, recoil::kMinStrength, recoil::kMaxStrength); }
	int ClampAngle(int value) { return ((value % 360) + 360) % 360; }

	// preset=<strength>,<angle>,<name>   (name last so it may contain commas)
	std::optional<config::Preset> ParsePreset(std::string_view value)
	{
		const auto firstComma = value.find(',');
		const auto secondComma = firstComma == std::string_view::npos ? firstComma : value.find(',', firstComma + 1);
		if (secondComma == std::string_view::npos)
			return std::nullopt;

		const auto strength = ParseInt(value.substr(0, firstComma));
		const auto angle = ParseInt(value.substr(firstComma + 1, secondComma - firstComma - 1));
		const auto name = Trim(value.substr(secondComma + 1));
		if (!strength || !angle || name.empty())
			return std::nullopt;

		return config::Preset{ std::string(name), ClampStrength(*strength), ClampAngle(*angle) };
	}
}

void config::Load() noexcept
{
	try
	{
		std::ifstream file(ConfigPath());
		if (!file)
			return;

		presets.clear();
		std::string line;
		while (std::getline(file, line))
		{
			const std::string_view text = Trim(line);
			if (text.empty() || text.front() == ';' || text.front() == '#')
				continue;

			const auto equals = text.find('=');
			if (equals == std::string_view::npos)
				continue;

			const std::string_view key = Trim(text.substr(0, equals));
			const std::string_view value = Trim(text.substr(equals + 1));

			if (key == "preset")
			{
				if (auto preset = ParsePreset(value))
					presets.push_back(std::move(*preset));
			}
			else if (const auto number = ParseInt(value))
			{
				if (key == "strength")
					recoil::strength = ClampStrength(*number);
				else if (key == "angle")
					recoil::angle = ClampAngle(*number);
				else if (key == "hotkey" && *number >= 0 && *number <= 0xFE)
					recoil::toggleKey = *number;
			}
		}
	}
	catch (...)
	{
		// A broken config file should never stop the app from starting.
	}
}

void config::Save() noexcept
{
	try
	{
		const auto path = ConfigPath();
		auto temp = path;
		temp += L".tmp";

		{
			std::ofstream file(temp, std::ios::trunc);
			if (!file)
				return;

			file << "; R6 Recoil Utility settings\n"
				<< "hotkey=" << recoil::toggleKey.load() << '\n'
				<< "strength=" << recoil::strength.load() << '\n'
				<< "angle=" << recoil::angle.load() << '\n';

			for (const auto& preset : presets)
				file << "preset=" << preset.strength << ',' << preset.angle << ',' << preset.name << '\n';

			if (!file.flush())
				return;
		}

		MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	}
	catch (...)
	{
	}
}
