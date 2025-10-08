#pragma once

#include <unordered_set>

namespace Configs {
	extern bool g_enabled;
	extern bool g_isActiveOnlyIfPlayerWeaponDrawn;
	extern bool g_isActiveOnlyIfPlayerInCombat;
	extern bool g_useKeywords;
	extern std::unordered_set<RE::BGSKeyword*> g_keywords;

	void ReadINI();
	void ReadKeywords();
}
