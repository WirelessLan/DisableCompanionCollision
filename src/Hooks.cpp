#include "Hooks.h"

#include <xbyak/xbyak.h>

#include "Configs.h"

namespace Hooks {
	namespace
	{
		bool IsPlayerTeammate(RE::Actor* a_actor)
		{
			if (!a_actor) {
				return false;
			}
			return (a_actor->niFlags.flags >> 26) & 1;
		}

		bool IsValidActor(RE::Actor* a_actor)
		{
			if (IsPlayerTeammate(a_actor)) {
				return true;
			}

			if (Configs::g_useKeywords) {
				for (const auto& keyword : Configs::g_keywords) {
					if (a_actor->HasKeyword(keyword)) {
						return true;
					}
				}
			}

			return false;
		}

		bool CanOverlap(RE::TESObjectREFR* a_refr)
		{
			if (!Configs::g_enabled) {
				return false;
			}

			if (!a_refr) {
				return false;
			}

			RE::Actor* actor = a_refr->As<RE::Actor>();
			if (!actor) {
				return false;
			}

			if (!IsValidActor(actor)) {
				return false;
			}

			if (Configs::g_isActiveOnlyIfPlayerWeaponDrawn || Configs::g_isActiveOnlyIfPlayerInCombat) {
				RE::PlayerCharacter* g_player = RE::PlayerCharacter::GetSingleton();
				if (!g_player) {
					return false;
				}

				if (Configs::g_isActiveOnlyIfPlayerWeaponDrawn && !g_player->GetWeaponMagicDrawn()) {
					return false;
				}

				if (Configs::g_isActiveOnlyIfPlayerInCombat && !g_player->IsInCombat()) {
					return false;
				}
			}

			return true;
		}
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
}
