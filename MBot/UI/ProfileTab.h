#pragma once

#include "Definitions.h"
#include "Includes.h"
#include "LCU.h"
#include "Utils.h"

class ProfileTab
{
public:
	static void Render()
	{
		if (ImGui::BeginTabItem("Profile"))
		{
			static char statusText[1024 * 16];
			ImGui::Text("Status:");
			const ImVec2 label_size = ImGui::CalcTextSize("W", nullptr, true);

			ImGui::InputTextMultiline("##inputStatus", statusText, IM_ARRAYSIZE(statusText), ImVec2(S.Window.width - 230.f,
				(label_size.y + ImGui::GetStyle().FramePadding.y) * 6.f), ImGuiInputTextFlags_AllowTabInput);
			if (ImGui::Button("Submit status"))
			{
				std::string body = R"({"statusMessage":")" + std::string(statusText) + "\"}";

				size_t nPos = 0;
				while (nPos != std::string::npos)
				{
					nPos = body.find('\n', nPos);
					if (nPos != std::string::npos)
					{
						body.erase(body.begin() + nPos);
						body.insert(nPos, "\\n");
					}
				}
				std::string result = LCU::Request("PUT", "https://127.0.0.1/lol-chat/v1/me", body);
				if (result.find("errorCode") != std::string::npos)
					MessageBoxA(nullptr, result.c_str(), nullptr, 0);
			}

			ImGui::SameLine();
			static int availability = 0;
			static int lastAvailability = 0;
			ImGui::RadioButton("Online", &availability, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Mobile", &availability, 1);
			ImGui::SameLine();
			ImGui::RadioButton("Away", &availability, 2);
			ImGui::SameLine();
			ImGui::RadioButton("Offline", &availability, 3);

			if (availability != lastAvailability)
			{
				lastAvailability = availability;
				std::string body = R"({"availability":")";
				switch (availability)
				{
				case 0:
					body += "online";
					break;
				case 1:
					body += "mobile";
					break;
				case 2:
					body += "away";
					break;
				case 3:
					body += "offline";
					break;
				default:;
				}
				body += "\"}";
				LCU::Request("PUT", "https://127.0.0.1/lol-chat/v1/me", body);
			}

			ImGui::SameLine();
			ImGui::Text("\t   ");
			ImGui::SameLine();
			if (ImGui::Button("Disconnect from chat"))
			{
				LCU::SetCurrentClientRiotInfo();
				cpr::Post(cpr::Url{ std::format("https://127.0.0.1:{}/chat/v1/suspend", LCU::riot.port) },
					cpr::Body{ "{\"config\":\"disable\"}" },
					cpr::Header{ Utils::StringToHeader(LCU::riot.header) }, cpr::VerifySsl{ false });
			}

			ImGui::SameLine();
			if (ImGui::Button("Reconnect to chat"))
			{
				LCU::SetCurrentClientRiotInfo();
				cpr::Post(cpr::Url{ std::format("https://127.0.0.1:{}/chat/v1/resume", LCU::riot.port) },
					cpr::Header{ Utils::StringToHeader(LCU::riot.header) }, cpr::VerifySsl{ false });
			}

			ImGui::Separator();

			if (ImGui::Button("Invisible banner"))
			{
				std::string playerP = GetPlayerPreferences();
				Json::CharReaderBuilder builder;
				const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				JSONCPP_STRING err;
				Json::Value root;
				if (reader->parse(playerP.c_str(), playerP.c_str() + static_cast<int>(playerP.length()), &root, &err))
				{
					root["bannerAccent"] = "2";
					LCU::Request("POST", "/lol-challenges/v1/update-player-preferences/", root.toStyledString());
				}
			}

			ImGui::SameLine();
			ImGui::HelpMarker("Works if last season's rank is unranked");

			ImGui::Separator();

			static int iconID;
			ImGui::Text("Icon:");
			ImGui::InputInt("##inputIcon:", &iconID, 1, 100);
			ImGui::SameLine();
			if (ImGui::Button("Submit##submitIcon"))
			{
				std::string body = R"({"profileIconId":)" + std::to_string(iconID) + "}";
				std::string result = LCU::Request("PUT", "https://127.0.0.1/lol-summoner/v1/current-summoner/icon", body);
				if (result.find("errorCode") != std::string::npos)
				{
					MessageBoxA(nullptr, result.c_str(), nullptr, 0);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Submit 2##submitIcon2"))
			{
				std::string body = R"({"icon":)" + std::to_string(iconID) + "}";
				std::string result = LCU::Request("PUT", "https://127.0.0.1/lol-chat/v1/me/", body);
				if (result.find("errorCode") != std::string::npos)
				{
					MessageBoxA(nullptr, result.c_str(), nullptr, 0);
				}
			}

			if (ImGui::CollapsingHeader("Backgrounds"))
			{
				if (champSkins.empty())
				{
					ImGui::Text("Skin data is still being fetched");
				}
				else
				{
					for (const auto& [key, name, skins] : champSkins)
					{
						if (ImGui::TreeNode(name.c_str()))
						{
							for (const auto& [fst, snd] : skins)
							{
								if (ImGui::Button(snd.c_str()))
								{
									std::string body = R"({"key":"backgroundSkinId","value":)" + fst + "}";
									LCU::Request("POST", "https://127.0.0.1/lol-summoner/v1/current-summoner/summoner-profile/", body);
								}
							}
							ImGui::TreePop();
						}
					}
				}
			}

			ImGui::EndTabItem();
		}
	}

	static std::string GetPlayerPreferences()
	{
		std::string challengesData = LCU::Request("GET", "/lol-challenges/v1/summary-player-data/local-player");
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (reader->parse(challengesData.c_str(), challengesData.c_str() + static_cast<int>(challengesData.length()), &root, &err))
		{
			std::string titleId = root["title"]["itemId"].asString();
			std::string bannerId = root["bannerId"].asString();

			std::string result = "{";
			for (Json::Value::ArrayIndex i = 0; i < root["topChallenges"].size(); i++)
			{
				if (i == 0)
					result += "\"challengeIds\":[";

				result += root["topChallenges"][i]["id"].asString();

				if (i != root["topChallenges"].size() - 1)
					result += ",";
				else
					result += "]";
			}

			if (titleId != "-1")
			{
				if (result.size() != 1)
					result += ",";
				result += R"("title":")" + titleId + "\"";
			}

			if (!bannerId.empty())
			{
				if (result.size() != 1)
					result += ",";
				result += R"("bannerAccent":")" + bannerId + "\"";
			}
			result += "}";

			return result;
		}
		return "";
	}
};
