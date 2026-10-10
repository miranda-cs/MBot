#pragma once

#include "Definitions.h"
#include "Includes.h"
#include "Misc.h"
#include "Config.h"

#pragma warning(disable : 4996)

class LoginTab
{
	static void ShowRiotClient()
	{
		if (const auto window = FindWindowA(nullptr, "Riot Client"))
		{
			ShowWindowAsync(window, SW_RESTORE);
			SetForegroundWindow(window);
		}
	}

	static std::string OpenRiotClient()
	{
		if (FindWindowA(nullptr, "Riot Client"))
		{
			ShowRiotClient();
			return "Riot Client opened.";
		}

		if (!std::filesystem::exists(std::filesystem::path(S.leaguePath) / "LeagueClient.exe"))
			return "Invalid client path, change it in Settings tab.";

		Misc::LaunchClient("");
		return "Launching client...";
	}

public:
	static void Render()
	{
		if (ImGui::BeginTabItem("Login"))
		{
			static std::string result;
			static bool once = true;
			static char leagueArgs[1024 * 16];
			static std::string sArgs;

			if (once)
			{
				once = false;
				std::ranges::copy(S.loginTab.leagueArgs, leagueArgs);
			}

			static std::vector<std::pair<std::string, std::string>> langs = {
				{"English (US)", "en_US"}, {"Japanese", "ja_JP"}, {"Korean", "ko_KR"}, {"Chinese (China)", "zh_CN"},
				{"German", "de_DE"}, {"Spanish (Spain)", "es_ES"}, {"Polish", "pl_PL"}, {"Russian", "ru_RU"},
				{"French", "fr_FR"}, {"Turkish", "tr_TR"}, {"Portuguese", "pt_BR"}, {"Czech", "cs_CZ"}, {"Greek", "el_GR"},
				{"Romanian", "ro_RO"}, {"Hungarian", "hu_HU"}, {"English (UK)", "en_GB"}, {"Italian", "it_IT"},
				{"Spanish (Mexico)", "es_MX"}, {"Spanish (Argentina)", "es_AR"}, {"English (Australia)", "en_AU"},
				{"Malay", "ms_MY"}, {"English (Philippines)", "en_PH"}, {"English (Singapore)", "en_SG"}, {"Thai", "th_TH"},
				{"Vietnamese", "vi_VN"}, {"Indonesian", "id_ID"}, {"Tagalog", "tl_PH"}, {"Chinese (Malaysia)", "zh_MY"}, {"Chinese (Taiwan)", "zh_TW"}
			};
			// find saved lang from cfg file
			auto findLang = std::ranges::find_if(langs, [](std::pair<std::string, std::string> k) {
				return k.second == S.loginTab.language;
				});

			static std::pair selectedLang = findLang != langs.end() ? *findLang : langs[0];

			if (ImGui::Button("Launch client"))
			{
				if (!std::filesystem::exists(S.leaguePath))
				{
					result = "Invalid path, change it in Settings tab";
				}
				else
				{
					Misc::LaunchClient(sArgs);
					result = S.leaguePath + "LeagueClient.exe " + sArgs;
				}
			}
			ImGui::SameLine();

			if (ImGui::BeginCombo("##language", selectedLang.first.c_str()))
			{
				for (const auto& [fst, snd] : langs)
				{
					if (ImGui::Selectable(fst.c_str(), fst == selectedLang.first))
					{
						selectedLang = { fst, snd };
						S.loginTab.language = snd;
						Config::Save();

						std::string localeArg = std::format("--locale={} ", selectedLang.second);
						size_t localePos = sArgs.find("--locale=");
						if (localePos != std::string::npos)
						{
							sArgs.replace(localePos, localeArg.size(), localeArg);
						}
						else
							sArgs += localeArg;
					}
				}
				ImGui::EndCombo();
			}

			std::ranges::copy(sArgs, leagueArgs);
			ImGui::Text(" Args: ");
			ImGui::SameLine();
			ImGui::InputText("##inputLeagueArgs", leagueArgs, IM_ARRAYSIZE(leagueArgs));

			sArgs = leagueArgs;
			S.loginTab.leagueArgs = sArgs;

			ImGui::Separator();

			if (ImGui::Button("Open Riot Client"))
			{
				result = OpenRiotClient();
			}

			ImGui::SameLine();
			ImGui::HelpMarker(
				"Automatic login was removed. Open the Riot Client and sign in there, then launch or reconnect the tool.");

			ImGui::TextWrapped("%s", result.c_str());

			ImGui::EndTabItem();
		}
	}
};
