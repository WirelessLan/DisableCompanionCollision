#include <xbyak/xbyak.h>

bool g_enabled = true;
bool g_activeOnlyIfPlayerDrawn = false;
bool g_isActiveOnlyWhenPlayerInCombat = false;

std::string GetINIOption(const char* section, const char* key) {
	std::string	result;
	char resultBuf[256] = { 0 };

	static const std::string& configPath = "Data\\MCM\\Settings\\" + std::string(Version::PROJECT) + ".ini";
	GetPrivateProfileStringA(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());
	return resultBuf;
}

class SetOptionHandler : public RE::Scaleform::GFx::FunctionHandler {
public:
	virtual void Call(const Params& a_params) override {
		if (a_params.argCount == 0 || a_params.args[0].GetType() != RE::Scaleform::GFx::Value::ValueType::kString)
			return;

		if (a_params.argCount == 2) {
			if (strcmp(a_params.args[0].GetString(), "bEnabled") == 0)
				g_enabled = a_params.args[1].GetBoolean();
			else if (strcmp(a_params.args[0].GetString(), "bActiveOnlyIfPlayerDrawn") == 0)
				g_activeOnlyIfPlayerDrawn = a_params.args[1].GetBoolean();
			else if (strcmp(a_params.args[0].GetString(), "bIsActiveOnlyWhenPlayerInCombat") == 0)
				g_isActiveOnlyWhenPlayerInCombat = a_params.args[1].GetBoolean();
		}
	}
};

void RegisterFunction(RE::Scaleform::GFx::Movie* a_view, RE::Scaleform::GFx::Value* a_f4se_root, RE::Scaleform::GFx::FunctionHandler* a_handler, F4SE::stl::zstring a_name) {
	RE::Scaleform::GFx::Value fn;
	a_view->CreateFunction(&fn, a_handler);
	a_f4se_root->SetMember(a_name, fn);
}

bool IsPlayerTeammate(RE::Actor* a_actor) {
	return (a_actor->niFlags.flags >> 26) & 1;
}


bool CanOverlap(RE::TESObjectREFR* a_refr) {
	if (!g_enabled)
		return false;

	if (!a_refr)
		return false;

	RE::Actor* actor = a_refr->As<RE::Actor>();
	if (!actor)
		return false;

	if (!IsPlayerTeammate(actor))
		return false;

	if (g_activeOnlyIfPlayerDrawn || g_isActiveOnlyWhenPlayerInCombat) {
		RE::PlayerCharacter* g_player = RE::PlayerCharacter::GetSingleton();
		if (!g_player)
			return false;

		if (g_activeOnlyIfPlayerDrawn && !g_player->GetWeaponMagicDrawn())
			return false;

		if (g_isActiveOnlyWhenPlayerInCombat && !g_player->IsInCombat())
			return false;
	}

	return true;
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

	value = GetINIOption("Settings", "bActiveOnlyIfPlayerDrawn");
	if (!value.empty()) {
		try {
			g_activeOnlyIfPlayerDrawn = std::stoul(value);
		}
		catch (...) {}
	}
	logger::info(FMT_STRING("bActiveOnlyIfPlayerDrawn: {}"), g_activeOnlyIfPlayerDrawn);

	value = GetINIOption("Settings", "bIsActiveOnlyWhenPlayerInCombat");
	if (!value.empty()) {
		try {
			g_isActiveOnlyWhenPlayerInCombat = std::stoul(value);
		}
		catch (...) {}
	}
	logger::info(FMT_STRING("bIsActiveOnlyWhenPlayerInCombat: {}"), g_isActiveOnlyWhenPlayerInCombat);
}


void Install() {
	struct asm_code : Xbyak::CodeGenerator {
		asm_code(std::uintptr_t a_srcAddr) {
			Xbyak::Label funcLabel, defLabel, skipLabel;

			push(rax);
			mov(rcx, ptr[rax + 0x000003E0]);
			call(ptr[rip + funcLabel]);
			test(al, al);
			pop(rax);

			je("DEFAULT");
			jmp(ptr[rip + skipLabel]);

			L("DEFAULT");
			mov(ecx, ptr[rax + 0x00000300]);
			jmp(ptr[rip + defLabel]);

			L(funcLabel);
			dq((std::uintptr_t)CanOverlap);

			L(defLabel);
			dq(a_srcAddr + 0x06);

			L(skipLabel);
			dq(a_srcAddr + 0x0E);
		}
	};

	REL::Relocation<std::uintptr_t> target{ REL::ID(1192093), 0x5EA };
	asm_code p{ target.address() };
	auto& trampoline = F4SE::GetTrampoline();
	void* codeBuf = trampoline.allocate(p);
	trampoline.write_branch<6>(target.address(), codeBuf);
}

bool RegisterScaleforms(RE::Scaleform::GFx::Movie* a_view, RE::Scaleform::GFx::Value* a_value) {
	RegisterFunction(a_view, a_value, new SetOptionHandler(), "SetOption"sv);
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface * a_f4se, F4SE::PluginInfo * a_info) {
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface * a_f4se) {
	F4SE::Init(a_f4se);
	F4SE::AllocTrampoline(1 << 10u);

	ReadINI();
	Install();

	const F4SE::ScaleformInterface* scaleform = F4SE::GetScaleformInterface();
	if (scaleform)
		scaleform->Register(Version::PROJECT, RegisterScaleforms);

	return true;
}
