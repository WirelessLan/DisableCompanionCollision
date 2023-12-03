#pragma once

namespace Configs {
	extern bool g_enabled;
	extern bool g_isActiveOnlyIfPlayerWeaponDrawn;
	extern bool g_isActiveOnlyIfPlayerInCombat;

	void ReadINI();
}
