#include "Scaleforms.h"

#include "Configs.h"

namespace Scaleforms {
	class SetOptionHandler : public RE::Scaleform::GFx::FunctionHandler {
	public:
		virtual void Call(const Params& a_params) override {
			if (a_params.argCount != 2 || a_params.args[0].GetType() != RE::Scaleform::GFx::Value::ValueType::kString)
				return;

			if (strcmp(a_params.args[0].GetString(), "bEnabled") == 0)
				Configs::g_enabled = a_params.args[1].GetBoolean();
			else if (strcmp(a_params.args[0].GetString(), "bIsActiveOnlyIfPlayerWeaponDrawn") == 0)
				Configs::g_isActiveOnlyIfPlayerWeaponDrawn = a_params.args[1].GetBoolean();
			else if (strcmp(a_params.args[0].GetString(), "bIsActiveOnlyIfPlayerInCombat") == 0)
				Configs::g_isActiveOnlyIfPlayerInCombat = a_params.args[1].GetBoolean();
		}
	};

	void RegisterFunction(RE::Scaleform::GFx::Movie* a_view, RE::Scaleform::GFx::Value* a_f4se_root, RE::Scaleform::GFx::FunctionHandler* a_handler, F4SE::stl::zstring a_name) {
		RE::Scaleform::GFx::Value fn;
		a_view->CreateFunction(&fn, a_handler);
		a_f4se_root->SetMember(a_name, fn);
	}

	void Register(RE::Scaleform::GFx::Movie* a_view, RE::Scaleform::GFx::Value* a_f4se_root) {
		RegisterFunction(a_view, a_f4se_root, new SetOptionHandler(), "SetOption"sv);
	}
}
