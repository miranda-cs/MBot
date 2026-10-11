#pragma once

#include "Definitions.h"
#include "Includes.h"
#include "ChampionService.h"

class SkinsTab
{
public:
	static void Render()
	{
		static bool bOnOpen = true;
		static bool bSortOnOpen = false;
		static std::vector<Skin> skins;

		if (ImGui::BeginTabItem("Skins"))
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
				skins = League::ChampionService::GetOwnedSkins();
			}

			if ((iLastSort != iSort) || bSortOnOpen)
			{
				bSortOnOpen = false;
				iLastSort = iSort;
				switch (iSort)
				{
					// alphabetically
				case 0:
					std::ranges::sort(skins, [](const Skin& lhs, const Skin& rhs) {
						return lhs.name < rhs.name;
						});
					break;
					// purchase date
				case 1:
					std::ranges::sort(skins, [](Skin lhs, Skin rhs) {
						return mktime(&lhs.purchaseDate) < mktime(&rhs.purchaseDate);
						});
					break;
				default:;
				}
			}

			ImGui::SameLine();
			static char allNamesSeparator[64] = ",";
			if (ImGui::Button("Copy names to clipboard##skinsTab"))
			{
				std::string allNames;
				for (const auto& [name, inventoryType, itemId, ownershipType, isVintage, purchaseDate, quantity, uuid] : skins)
				{
					if (name.empty())
						continue;
					allNames += name + allNamesSeparator;
				}
				Utils::CopyToClipboard(allNames);
			}
			ImGui::SameLine();

			const ImVec2 label_size = ImGui::CalcTextSize("W", nullptr, true);
			ImGui::InputTextMultiline("##separatorSkinsTab", allNamesSeparator, IM_ARRAYSIZE(allNamesSeparator),
				ImVec2(0, label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f), ImGuiInputTextFlags_AllowTabInput);

			ImGui::Separator();
			ImGui::Text("Skins owned: %d", skins.size());

			for (const auto& [name, inventoryType, itemId, ownershipType, isVintage, purchaseDate, quantity, uuid] : skins)
			{
				char timeBuff[50];
				strftime(timeBuff, sizeof(timeBuff), "%G-%m-%d %H:%M:%S", &purchaseDate); // cert-err33-c -> The value of this function should be used

				std::string inputId = "skinInput";
				inputId.append(std::to_string(itemId));
				char input[512];
				strcpy_s(input, std::format(R"(name: {}
inventoryType: {}
ownershipType: {}
isVintage: {}
purchaseDate: {}
quantity: {}
uuid: {})", name, inventoryType, ownershipType, isVintage, timeBuff, quantity, uuid).c_str());
				ImGui::PushID(inputId.c_str());
				ImGui::InputTextMultiline("", input, IM_ARRAYSIZE(input), ImVec2(ImGui::GetWindowSize().x, 0), ImGuiInputTextFlags_ReadOnly);
				ImGui::PopID();
			}

			ImGui::EndTabItem();
		}
		else
			bOnOpen = true;
	}
};
