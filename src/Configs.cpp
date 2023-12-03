#include "Configs.h"

#include <Windows.h>

namespace Configs {
	bool g_enabled = true;
	bool g_isActiveOnlyIfPlayerWeaponDrawn = false;
	bool g_isActiveOnlyIfPlayerInCombat = false;

	std::string GetINIOption(const char* section, const char* key) {
		std::string	result;
		char resultBuf[256] = { 0 };

		static const std::string& configPath = "Data\\MCM\\Settings\\" + std::string(Version::PROJECT) + ".ini";
		GetPrivateProfileStringA(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());
		return resultBuf;
	}

	void ReadINI() {
		std::string value;

		value = GetINIOption("Settings", "bEnabled");
		if (!value.empty()) {
			try {
				g_enabled = std::stoul(value);
			}
			catch (...) {}
		}
		logger::info(FMT_STRING("bEnabled: {}"), g_enabled);

		value = GetINIOption("Settings", "bIsActiveOnlyIfPlayerWeaponDrawn");
		if (!value.empty()) {
			try {
				g_isActiveOnlyIfPlayerWeaponDrawn = std::stoul(value);
			}
			catch (...) {}
		}
		logger::info(FMT_STRING("bIsActiveOnlyIfPlayerWeaponDrawn: {}"), g_isActiveOnlyIfPlayerWeaponDrawn);

		value = GetINIOption("Settings", "bIsActiveOnlyIfPlayerInCombat");
		if (!value.empty()) {
			try {
				g_isActiveOnlyIfPlayerInCombat = std::stoul(value);
			}
			catch (...) {}
		}
		logger::info(FMT_STRING("bIsActiveOnlyIfPlayerInCombat: {}"), g_isActiveOnlyIfPlayerInCombat);
	}
}
