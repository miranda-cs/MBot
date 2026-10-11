#pragma once

#include "Definitions.h"
#include "Includes.h"
#include "ChampionService.h"

#pragma warning (disable : 4996)

class ChampsTab
{
public:
	static void Render()
	{
		static bool bOnOpen = true;
		static unsigned iChampsOwned = 0;
		static bool bSortOnOpen = false;
		static std::vector<ChampAll> champions;

		if (ImGui::BeginTabItem("Champs"))
		{
			ImGui::Text("Sort by: ");
			ImGui::SameLine();

			static int iSort = 0;
			static int iLastSort = -1;
			ImGui::RadioButton("Alphabetically", &iSort, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Purchase date", &iSort, 1);

			if (bOnOpen)
			{
				bOnOpen = false;
				bSortOnOpen = true;
				champions = League::ChampionService::GetChampionCollection();
				iChampsOwned = static_cast<unsigned>(champions.size());
			}

			if ((iLastSort != iSort) || bSortOnOpen)
			{
				bSortOnOpen = false;
				iLastSort = iSort;
				switch (iSort)
				{
					// alphabetically
				case 0:
					std::ranges::sort(champions, [](const ChampAll& lhs, const ChampAll& rhs) {
						return lhs.min.name < rhs.min.name;
						});
					break;
					// purchase date
				case 1:
					std::ranges::sort(champions, [](const ChampAll& lhs, const ChampAll& rhs) {
						return std::stoll(lhs.min.purchased) < std::stoll(rhs.min.purchased);
						});
					break;
				default:;
				}
			}

			ImGui::SameLine();
			static char allNamesSeparator[64] = ",";
			if (ImGui::Button("Copy names to clipboard##champsTab"))
			{
				std::string allNames;
				for (const auto& [min, mas] : champions)
				{
					if (!min.owned)
						continue;
					allNames += min.name + allNamesSeparator;
				}
				Utils::CopyToClipboard(allNames);
			}
			ImGui::SameLine();

			const ImVec2 label_size = ImGui::CalcTextSize("W", nullptr, true);
			ImGui::InputTextMultiline("##separatorChampsTab", allNamesSeparator, IM_ARRAYSIZE(allNamesSeparator),
				ImVec2(0, label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f), ImGuiInputTextFlags_AllowTabInput);

			ImGui::Separator();
			ImGui::Text("Champions owned: %d", iChampsOwned);
			for (const auto& [min, mas] : champions)
			{
				if (!min.owned)
					continue;

				int64_t t = std::stoll(min.purchased);
				t /= 1000;
				char buffer[50];
				strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&t));

				std::string inputId = "champInput";
				inputId.append(std::to_string(min.id));
				char input[768];

				float textHeight = (label_size.y + ImGui::GetStyle().FramePadding.y) * 3.f;
				std::string text = std::format(R"(name: {}
purchased: {}
id: {})", min.name, buffer, min.id);

				if (!mas.lastPlayTime.empty())
				{
					textHeight = (label_size.y + ImGui::GetStyle().FramePadding.y) * 12.f;

					t = std::stoll(mas.lastPlayTime);
					t /= 1000;
					strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&t));

					text += std::format(R"(
championLevel: {}
championPoints: {}
championPointsSinceLastLevel: {}
championPointsUntilNextLevel: {}
chestGranted: {}
formattedChampionPoints: {}
formattedMasteryGoal: {}
highestGrade: {}
lastPlayTime: {}
playerId: {}
tokensEarned: {})", mas.championLevel, mas.championPoints, mas.championPointsSinceLastLevel,
mas.championPointsUntilNextLevel, mas.chestGranted, mas.formattedChampionPoints, mas.formattedMasteryGoal,
mas.highestGrade, buffer, mas.playerId, mas.tokensEarned);
				}

				strcpy(input, text.c_str());
				ImGui::PushID(inputId.c_str());
				ImGui::InputTextMultiline("", input, IM_ARRAYSIZE(input), ImVec2(ImGui::GetWindowSize().x, textHeight),
					ImGuiInputTextFlags_ReadOnly);
				ImGui::PopID();
			}

			ImGui::EndTabItem();
		}
		else
			bOnOpen = true;
	}
};
