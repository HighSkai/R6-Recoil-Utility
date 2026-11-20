#pragma once
#include <string>
#include <vector>

// Settings and presets, stored in R6RecoilUtility.ini next to the exe.
namespace config
{
	struct Preset
	{
		std::string name;
		int strength = 15;
		int angle = 90;
	};

	inline std::vector<Preset> presets;

	// Loads the file into recoil:: settings and config::presets. Missing or
	// malformed values fall back to defaults instead of crashing.
	void Load() noexcept;

	// Writes current settings and all presets (atomically, via a temp file).
	void Save() noexcept;
}
