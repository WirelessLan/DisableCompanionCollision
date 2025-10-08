#include "Configs.h"

#include <filesystem>
#include <fstream>
#include <Windows.h>

namespace Configs {
	bool g_enabled = true;
	bool g_isActiveOnlyIfPlayerWeaponDrawn = false;
	bool g_isActiveOnlyIfPlayerInCombat = false;
	bool g_useKeywords = true;
	std::unordered_set<RE::BGSKeyword*> g_keywords;

	namespace
	{
		std::string GetINIOption(const char* section, const char* key)
		{
			std::string result;
			char resultBuf[256] = { 0 };

			static const std::string& configPath = "Data\\MCM\\Settings\\" + std::string(Version::PROJECT) + ".ini";
			GetPrivateProfileStringA(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());
			return resultBuf;
		}

		std::string_view Trim(std::string_view sv)
		{
			size_t start = 0;
			while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start]))) {
				++start;
			}

			size_t end = sv.size();
			while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1]))) {
				--end;
			}

			return sv.substr(start, end - start);
		}

		RE::TESForm* GetFormByFormID(std::string_view formSv) {
			static constexpr unsigned char Delimeter = '|';

			std::size_t delimeterPos = formSv.find(Delimeter);
			if (delimeterPos == std::string_view::npos) {
				return nullptr;
			}

			std::string_view pluginName = Trim(formSv.substr(0, delimeterPos));
			std::string_view formIdSv = Trim(formSv.substr(delimeterPos + 1));

			std::uint32_t formId;
			try {
				formId = std::stoul(std::string(formIdSv), nullptr, 16);
			} catch (...) {
				return nullptr;
			}

			RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				return nullptr;
			}

			return dataHandler->LookupForm(formId, pluginName);
		}
	}

	void ReadINI() {
		std::string value;

		value = GetINIOption("Settings", "bEnabled");
		if (!value.empty()) {
			try {
				g_enabled = std::stoul(value) == 1;
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

		value = GetINIOption("Settings", "bUseKeywords");
		if (!value.empty()) {
			try {
				g_useKeywords = std::stoul(value) == 1;
			} catch (...) {}
		}
		logger::info(FMT_STRING("bUseKeywords: {}"), g_useKeywords);
	}

	void ReadKeywords() {
		g_keywords.clear();

		const std::filesystem::path directoryPath = std::filesystem::path("Data") / "F4SE" / "Plugins" / Version::PROJECT / "Keywords";
		std::error_code ec;

		if (!std::filesystem::exists(directoryPath, ec)) {
			logger::warn("Keyword path does not exist: {}", directoryPath.string());
			return;
		}

		if (!std::filesystem::is_directory(directoryPath, ec)) {
			logger::warn("Keyword path is not a directory (or inaccessible): {}", directoryPath.string());
			return;
		}

		const auto opts = std::filesystem::directory_options::skip_permission_denied;

		for (std::filesystem::recursive_directory_iterator it{ directoryPath, opts, ec }, end{}; it != end; it.increment(ec)) {
			if (ec) {
				logger::warn("Filesystem iteration error: {}", ec.message());
				ec.clear();
				continue;
			}

			const auto& entry = *it;

			if (!entry.is_regular_file(ec)) {
				ec.clear();
				continue;
			}

			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
			if (ext != ".txt") {
				continue;
			}

			std::string filePath = entry.path().string();
			logger::info("Reading keyword file: {}", filePath);
										
			std::ifstream file(entry.path());
			if (!file.is_open()) {
				logger::warn("Failed to open keyword file: {}", filePath);
				continue;
			}

			std::string line;
			while (std::getline(file, line)) {
				const std::string_view trimmedLine = Trim(line);
				if (trimmedLine.empty() || trimmedLine.front() == '#') {
					continue;
				}

				RE::TESForm* form = GetFormByFormID(trimmedLine);
				if (!form) {
					logger::warn("Form not found for ID: {}", trimmedLine);
					continue;
				}

				RE::BGSKeyword* keyword = form->As<RE::BGSKeyword>();
				if (!keyword) {
					logger::warn("Form is not a keyword: {}", trimmedLine);
					continue;
				}

				g_keywords.insert(keyword);
				logger::info("Added keyword: {:08X} ({})", keyword->formID, trimmedLine);
			}
		}
	}
}
