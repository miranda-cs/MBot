#pragma once

#include "Definitions.h"
#include "Includes.h"
#include "LCU.h"
#include "Utils.h"
#include "Misc.h"

class GameTab
{
private:
	static inline bool onOpen = true;

public:
	static void Render()
	{
		if (ImGui::BeginTabItem("Game"))
		{
			static std::vector<std::string> itemsMultiSearch = {
				"OP.GG", "U.GG", "PORO.GG", "Porofessor.gg"
			};
			if (S.gameTab.indexMultiSearch >= itemsMultiSearch.size())
				S.gameTab.indexMultiSearch = 0;
			const char* selectedMultiSearch = itemsMultiSearch[S.gameTab.indexMultiSearch].c_str();

			if (ImGui::Button("Multi-Search"))
			{
				const std::string multiSearchResult = MultiSearch(itemsMultiSearch[S.gameTab.indexMultiSearch]);
				if (multiSearchResult.find("https://") != 0)
				{
					MessageBoxA(nullptr, multiSearchResult.c_str(), "Multi-Search failed", MB_OK);
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Dodge"))
			{
				const std::string result = Dodge();
				if (result.find("errorCode") != std::string::npos)
					MessageBoxA(nullptr, result.c_str(), "Dodge failed", MB_OK);
			}

			ImGui::SameLine();

			ImGui::SetNextItemWidth(static_cast<float>(S.Window.width / 6));
			if (ImGui::BeginCombo("##comboMultiSearch", selectedMultiSearch, 0))
			{
				for (size_t n = 0; n < itemsMultiSearch.size(); n++)
				{
					const bool is_selected = (S.gameTab.indexMultiSearch == n);
					if (ImGui::Selectable(itemsMultiSearch[n].c_str(), is_selected))
						S.gameTab.indexMultiSearch = n;

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			ImGui::Checkbox("Auto accept", &S.gameTab.autoAcceptEnabled);

			ImGui::Separator();

			static bool isStillBeingFetched = true;
			if (!champSkins.empty())
				isStillBeingFetched = false;

			static ImGui::ComboAutoSelectData instalockComboData;

			if (onOpen)
			{
				std::vector<std::pair<int, std::string>> instalockChamps = GetInstalockChamps();

				if (!instalockChamps.empty())
				{
					std::vector<std::string> instalockChampsNames;
					instalockChampsNames.reserve(instalockChamps.size() + 1);
					instalockChampsNames.emplace_back("Random");

					if (S.gameTab.instalockId == -1)
					{
						std::ranges::copy("Random", instalockComboData.input);
					}
					else
					{
						std::string selectedChamp = ChampIdToName(S.gameTab.instalockId);
						std::ranges::copy(selectedChamp, instalockComboData.input);
					}

					for (size_t i = 0; i < instalockChamps.size(); i++)
					{
						instalockChampsNames.emplace_back(instalockChamps[i].second);
						if (instalockComboData.input == instalockChamps[i].second)
						{
							instalockComboData.index = static_cast<int>(i + 1);
						}
					}
					instalockComboData.items = instalockChampsNames;
				}
			}

			ImGui::Checkbox("Instalock", &S.gameTab.instalockEnabled);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(static_cast<float>(S.Window.width / 6));
			if (ImGui::ComboAutoSelect("##comboInstalock", instalockComboData))
			{
				if (instalockComboData.index != -1)
				{
					if (std::string(instalockComboData.input) == "Random")
					{
						S.gameTab.instalockId = -1;
					}
					else
					{
						for (const auto& [key, name, skins] : champSkins)
						{
							if (instalockComboData.input == name)
							{
								S.gameTab.instalockId = key;
							}
						}
					}
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(static_cast<float>(S.Window.width / 6));

			ImGui::SliderInt("Delay##sliderInstalockDelay", &S.gameTab.instalockDelay, 0, 10000, "%d ms");

			ImGui::SameLine();

			ImGui::Checkbox("Dodge on champion ban", &S.gameTab.dodgeOnBan);

			ImGui::SameLine();
			ImGui::HelpMarker("Ignores backup pick");

			static std::string chosenBackup = "Backup pick \t\t\tChosen: " + Misc::ChampIdToName(S.gameTab.backupId) + "###AnimatedBackup";
			static int lastBackupId = 0;
			if ((lastBackupId != S.gameTab.backupId) && !isStillBeingFetched)
			{
				lastBackupId = S.gameTab.backupId;
				chosenBackup = "Backup pick \t\t\tChosen: " + Misc::ChampIdToName(S.gameTab.backupId) + "###AnimatedBackup";
			}
			if (ImGui::CollapsingHeader(chosenBackup.c_str()))
			{
				ImGui::Text("None");
				ImGui::SameLine();
				ImGui::RadioButton("##noneBackupPick", &S.gameTab.backupId, 0);
				std::vector<std::pair<int, std::string>> instalockChamps = GetInstalockChamps();
				for (const auto& [fst, snd] : instalockChamps)
				{
					char bufchamp[128];
					sprintf_s(bufchamp, "##Select %s", snd.c_str());
					ImGui::Text("%s", snd.c_str());
					ImGui::SameLine();
					ImGui::RadioButton(bufchamp, &S.gameTab.backupId, fst);
				}
			}

			ImGui::Checkbox("Auto ban", &S.gameTab.autoBanEnabled);
			ImGui::SameLine();

			static ImGui::ComboAutoSelectData autobanComboData;
			if (onOpen)
			{
				std::vector<std::string> autobanChampsNames;
				if (!champSkins.empty())
				{
					autobanChampsNames.reserve(champSkins.size() + 1);
					autobanChampsNames.emplace_back("None");

					if (S.gameTab.autoBanId == -1)
					{
						std::ranges::copy("None", autobanComboData.input);
					}
					else
					{
						std::string selectedChamp = ChampIdToName(S.gameTab.autoBanId);
						std::ranges::copy(selectedChamp, autobanComboData.input);
					}

					for (size_t i = 0; i < champSkins.size(); i++)
					{
						autobanChampsNames.emplace_back(champSkins[i].name);

						if (autobanComboData.input == champSkins[i].name)
						{
							autobanComboData.index = static_cast<int>(i + 1);
						}
					}
					autobanComboData.items = autobanChampsNames;
				}
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(static_cast<float>(S.Window.width / 6));
			if (ImGui::ComboAutoSelect("##comboAutoban", autobanComboData))
			{
				if (autobanComboData.index != -1)
				{
					if (std::string(autobanComboData.input) == "None")
					{
						S.gameTab.autoBanId = -1;
					}
					else
					{
						for (const auto& [key, name, skins] : champSkins)
						{
							if (autobanComboData.input == name)
							{
								S.gameTab.autoBanId = key;
							}
						}
					}
				}
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(static_cast<float>(S.Window.width / 6));
			ImGui::SliderInt("Delay##sliderautoBanDelay", &S.gameTab.autoBanDelay, 0, 10000, "%d ms");

			ImGui::SameLine();

			ImGui::Checkbox("Instant Mute", &S.gameTab.instantMute);

			if (onOpen)
				onOpen = false;

			ImGui::EndTabItem();
		}
		else
		{
			onOpen = true;
		}
	}

	static std::vector<std::pair<int, std::string>> GetInstalockChamps()
	{
		std::vector<std::pair<int, std::string>> temp;

		std::string result = LCU::Request("GET", "https://127.0.0.1/lol-champions/v1/owned-champions-minimal");
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (reader->parse(result.c_str(), result.c_str() + static_cast<int>(result.length()), &root, &err))
		{
			if (root.isArray())
			{
				for (auto& i : root)
				{
					if (i["freeToPlay"].asBool() == true || i["ownership"]["owned"].asBool() == true ||
						(i["ownership"].isMember("xboxGPReward") && i["ownership"]["xboxGPReward"].asBool() == true))
					{
						std::string loadScreenPath = i["baseLoadScreenPath"].asString();
						size_t nameStart = loadScreenPath.find("ASSETS/Characters/") + strlen("ASSETS/Characters/");
						std::string champName = loadScreenPath.substr(nameStart, loadScreenPath.find('/', nameStart) - nameStart);

						std::pair champ = { i["id"].asInt(), champName };
						temp.emplace_back(champ);
					}
				}
			}
		}
		std::ranges::sort(temp, [](std::pair<int, std::string> a, std::pair<int, std::string> b) { return a.second < b.second; });
		return temp;
	}

	static std::string Dodge()
	{
		return LCU::Request("POST", "/lol-lobby-team-builder/champ-select/v1/session/quit");
	}

	static void OnChampSelectReady(const bool instantMute = false)
	{
		auto start = std::chrono::system_clock::now();
		while (true)
		{
			auto now = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = now - start;
			if (diff.count() > 10) // took 10 seconds and still didn't connect to chat
			{
				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			Json::Value root;
			Json::CharReaderBuilder builder;
			const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
			JSONCPP_STRING err;

			LCU::SetCurrentClientRiotInfo();
			std::string getChat = cpr::Get(cpr::Url{ std::format("https://127.0.0.1:{}/chat/v5/participants", LCU::riot.port) },
				cpr::Header{ Utils::StringToHeader(LCU::riot.header) }, cpr::VerifySsl{ false }).text;
			if (!reader->parse(getChat.c_str(), getChat.c_str() + static_cast<int>(getChat.length()), &root, &err))
			{
				continue;
			}

			const auto& participantsArr = root["participants"];
			if (!participantsArr.isArray())
			{
				continue;
			}
			if (participantsArr.size() <= 1)
			{
				continue;
			}

			std::string cid = "";
			for (auto& i : participantsArr)
			{
				if (i["cid"].asString().contains("champ-select"))
				{
					cid = i["cid"].asString();
					break;
				}
			}
			if (cid == "")
			{
				continue;
			}

			if (instantMute)
			{
				std::string champSelect = LCU::Request("GET", "/lol-champ-select/v1/session");
				Json::Value rootCSelect;
				if (!champSelect.empty() && champSelect.find("RPC_ERROR") == std::string::npos)
				{
					if (reader->parse(champSelect.c_str(), champSelect.c_str() + static_cast<int>(champSelect.length()), &rootCSelect, &err))
					{
						if (instantMute)
						{
							int localPlayerCellId = rootCSelect["localPlayerCellId"].asInt();
							for (Json::Value::ArrayIndex i = 0; i < rootCSelect["myTeam"].size(); i++)
							{
								Json::Value player = rootCSelect["myTeam"][i];
								if (player["cellId"].asInt() == localPlayerCellId)
									continue;

								LCU::Request("POST", "/lol-champ-select/v1/toggle-player-muted",
									std::format(R"({{"summonerId":{0},"puuid":"{1}","obfuscatedSummonerId":{2},"obfuscatedPuuid":"{3}"}})",
										player["summonerId"].asString(), player["puuid"].asString(),
										player["obfuscatedSummonerId"].asString(),
										player["obfuscatedPuuid"].asString()));

								/*	LCU::Request("POST", "/telemetry/v1/events/general_metrics_number",
										R"({"eventName":"champ_select_toggle_player_muted_clicked","value":"0","spec":"high","isLowSpecModeOn":"false"})");

									LCU::Request("POST", std::format("/lol-chat/v1/conversations/{}/messages", cid),
										std::format("{{\"body\":\"{} is muted.\",\"type\":\"celebration\"}}", "player"));
								*/
							}
						}

					}
				}
			}
			return;
		}
	}

	static void AutoAccept()
	{
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			if (FindWindowA("RiotWindowClass", "League of Legends (TM) Client"))
			{
				// game is running, auto accept is not needed
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}

			if (!FindWindowA("RCLIENT", "League of Legends"))
			{
				continue;
			}

			if (S.gameTab.autoAcceptEnabled || (S.gameTab.autoBanEnabled && S.gameTab.autoBanId) ||
				(S.gameTab.dodgeOnBan && S.gameTab.instalockEnabled) ||
				(S.gameTab.instalockEnabled && S.gameTab.instalockId) ||
				S.gameTab.instantMute)
			{
				Json::Value rootSearch;
				Json::Value rootChampSelect;
				Json::Value rootSession;
				Json::CharReaderBuilder builder;
				const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
				JSONCPP_STRING err;

				cpr::Session session;
				session.SetVerifySsl(false);
				session.SetHeader(Utils::StringToHeader(LCU::league.header));
				session.SetUrl(std::format("https://127.0.0.1:{}/lol-lobby/v2/lobby/matchmaking/search-state", LCU::league.port));

				std::string getSearchState = session.Get().text;
				if (!reader->parse(getSearchState.c_str(), getSearchState.c_str() + static_cast<int>(getSearchState.length()), &rootSearch, &err))
				{
					continue;
				}

				static bool onChampSelect = true; //false when in champ select
				static int useBackupId = 0;
				static bool isPicked = false;

				if (rootSearch["searchState"].asString() != "Found") // not found, not in champ select
				{
					onChampSelect = true;
					useBackupId = 0;
					isPicked = false;
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					continue;
				}

				session.SetUrl(std::format("https://127.0.0.1:{}/lol-champ-select/v1/session", LCU::league.port));
				std::string getChampSelect = session.Get().text;
				if (getChampSelect.find("RPC_ERROR") != std::string::npos) // game found but champ select error means queue pop
				{
					onChampSelect = true;
					useBackupId = 0;
					isPicked = false;
					if (S.gameTab.autoAcceptEnabled)
					{
						session.SetUrl(std::format("https://127.0.0.1:{}/lol-matchmaking/v1/ready-check/accept", LCU::league.port));
						session.SetBody("");
						session.Post();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				else // in champ select
				{
					if (!reader->parse(getChampSelect.c_str(), getChampSelect.c_str() + static_cast<int>(getChampSelect.length()), &rootChampSelect,
						&err))
					{
						continue;
					}

					if (onChampSelect)
					{
						onChampSelect = false;

						if (S.gameTab.instantMute)
						{
							std::thread champSelectReadyThread(&GameTab::OnChampSelectReady, S.gameTab.instantMute);
							champSelectReadyThread.detach();
						}
					}

					if ((S.gameTab.instalockEnabled || S.gameTab.autoBanId) && !isPicked)
					{
						// get own summid
						session.SetUrl(std::format("https://127.0.0.1:{}/lol-login/v1/session", LCU::league.port));
						std::string getSession = session.Get().text;
						if (!reader->parse(getSession.c_str(), getSession.c_str() + static_cast<int>(getSession.length()), &rootSession, &err))
						{
							continue;
						}

						const int cellId = rootChampSelect["localPlayerCellId"].asInt();
						for (Json::Value::ArrayIndex j = 0; j < rootChampSelect["actions"].size(); j++)
						{
							auto actions = rootChampSelect["actions"][j];
							if (!actions.isArray())
							{
								continue;
							}
							for (auto& action : actions)
							{
								// search for own actions
								if (action["actorCellId"].asInt() == cellId)
								{
									if (std::string actionType = action["type"].asString(); actionType == "pick"
										&& S.gameTab.instalockId && S.gameTab.instalockEnabled)
									{
										// if haven't picked yet
										if (action["completed"].asBool() == false)
										{
											if (!isPicked)
											{
												std::this_thread::sleep_for(std::chrono::milliseconds(S.gameTab.instalockDelay));

												int currentPick = S.gameTab.instalockId;
												if (useBackupId)
													currentPick = useBackupId;

												if (S.gameTab.instalockId == -1)
												{
													std::vector<std::pair<int, std::string>> instalockChamps = GetInstalockChamps();
													currentPick = instalockChamps[Utils::RandomInt(0, static_cast<int>(instalockChamps.size() - 1))].first;
												}

												session.SetUrl(std::format("https://127.0.0.1:{}/lol-champ-select/v1/session/actions/{}",
													LCU::league.port,
													action["id"].asString()));
												session.SetBody(R"({"completed":true,"championId":)" + std::to_string(currentPick) + "}");
												session.Patch();
											}
										}
										else
										{
											isPicked = true;
										}
									}
									else if (actionType == "ban" && S.gameTab.autoBanId && S.gameTab.autoBanEnabled)
									{
										if (action["completed"].asBool() == false)
										{
											std::this_thread::sleep_for(std::chrono::milliseconds(S.gameTab.autoBanDelay));

											session.SetUrl(std::format("https://127.0.0.1:{}/lol-champ-select/v1/session/actions/{}",
												LCU::league.port,
												action["id"].asString()));
											session.SetBody(R"({"completed":true,"championId":)" + std::to_string(S.gameTab.autoBanId) + "}");
											session.Patch();
										}
									}
								}
								// action that isn't our player, if dodge on ban enabled or backup pick
								else if ((S.gameTab.dodgeOnBan || S.gameTab.backupId) && S.gameTab.instalockEnabled
									&& S.gameTab.instalockId && (S.gameTab.instalockId != -1))
								{
									if (isPicked)
										break;

									if (action["actorCellId"].asInt() == cellId)
										continue;

									if (action["type"].asString() == "ban" && action["completed"].asBool() == true)
									{
										if (action["championId"].asInt() == S.gameTab.instalockId)
										{
											if (S.gameTab.dodgeOnBan)
											{
												session.SetUrl(
													std::format("https://127.0.0.1:{}/lol-lobby-team-builder/champ-select/v1/session/quit",
														LCU::league.port));
												session.SetBody("");
												session.Post();
											}
											else if (S.gameTab.backupId)
											{
												useBackupId = S.gameTab.backupId;
											}
										}
									}
									else if (action["type"].asString() == "pick" && action["completed"].asBool() == true)
									{
										if (S.gameTab.backupId && (action["championId"].asInt() == S.gameTab.instalockId))
										{
											useBackupId = S.gameTab.backupId;
										}
									}
								}
							}
						}
					}
					else // instalock or autoban not enabled, we do nothing in champ select
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}
				}
			}
		}
	}

	enum class MultiSearchServer
	{
		Unknown,
		BR,
		EUNE,
		EUW,
		LAN,
		LAS,
		NA
	};

	static std::string UrlEncode(const std::string& value)
	{
		static constexpr char hex[] = "0123456789ABCDEF";
		std::string encoded;
		for (const unsigned char c : value)
		{
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
				c == '-' || c == '_' || c == '.' || c == '~')
			{
				encoded += static_cast<char>(c);
			}
			else
			{
				encoded += '%';
				encoded += hex[c >> 4];
				encoded += hex[c & 0x0F];
			}
		}
		return encoded;
	}

	static std::string JoinSummonerNames(const std::vector<std::string>& summonerNames, const bool encodeComma, const bool trailingComma)
	{
		const std::string separator = encodeComma ? "%2C" : ",";
		std::string joined;
		for (size_t i = 0; i < summonerNames.size(); i++)
		{
			joined += UrlEncode(summonerNames[i]);
			if (trailingComma || i + 1 < summonerNames.size())
				joined += separator;
		}
		return joined;
	}

	static MultiSearchServer ParseMultiSearchServer(std::string region)
	{
		region = Utils::ToUpper(region);
		if (region == "BR" || region == "BR1")
			return MultiSearchServer::BR;
		if (region == "EUNE" || region == "EUN1")
			return MultiSearchServer::EUNE;
		if (region == "EUW" || region == "EUW1")
			return MultiSearchServer::EUW;
		if (region == "LAN" || region == "LA1")
			return MultiSearchServer::LAN;
		if (region == "LAS" || region == "LA2")
			return MultiSearchServer::LAS;
		if (region == "NA" || region == "NA1")
			return MultiSearchServer::NA;
		return MultiSearchServer::Unknown;
	}

	static std::string ToOpggRegion(const MultiSearchServer server)
	{
		switch (server)
		{
		case MultiSearchServer::BR:
			return "br";
		case MultiSearchServer::EUNE:
			return "eune";
		case MultiSearchServer::EUW:
			return "euw";
		case MultiSearchServer::LAN:
			return "lan";
		case MultiSearchServer::LAS:
			return "las";
		case MultiSearchServer::NA:
			return "na";
		default:
			return "";
		}
	}

	static std::string MultiSearch(const std::string& website)
	{
		std::string champSelect = LCU::Request("GET", "https://127.0.0.1/lol-champ-select/v1/session");
		if (champSelect.empty() || champSelect.find("RPC_ERROR") != std::string::npos)
			return "Champion select not found";

		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value rootRegion;
		Json::Value rootCSelect;
		Json::Value rootSummoner;
		Json::Value rootPartcipants;

		if (!reader->parse(champSelect.c_str(), champSelect.c_str() + static_cast<int>(champSelect.length()), &rootCSelect, &err))
			return "Failed to parse champion select";

		auto teamArr = rootCSelect["myTeam"];
		if (!teamArr.isArray())
			return "Failed to get team";

		std::vector<std::string> summonerNames;
		bool isRanked = false;
		for (auto& i : teamArr)
		{
			if (i["nameVisibilityType"].asString() == "HIDDEN")
			{
				isRanked = true;
				break;
			}

			std::string summId = i["summonerId"].asString();
			if (summId == "0")
				continue;

			std::string summoner = LCU::Request("GET", "https://127.0.0.1/lol-summoner/v1/summoners/" + summId);
			if (reader->parse(summoner.c_str(), summoner.c_str() + static_cast<int>(summoner.length()), &rootSummoner, &err))
			{
				summonerNames.emplace_back(rootSummoner["gameName"].asString() + "#" + rootSummoner["tagLine"].asString());
			}
		}

		// Ranked lobby reveal
		if (isRanked)
		{
			summonerNames.clear();

			LCU::SetCurrentClientRiotInfo();
			std::string participants = cpr::Get(
				cpr::Url{ std::format("https://127.0.0.1:{}/chat/v5/participants", LCU::riot.port) },
				cpr::Header{ Utils::StringToHeader(LCU::riot.header) }, cpr::VerifySsl{ false }).text;
			if (reader->parse(participants.c_str(), participants.c_str() + static_cast<int>(participants.length()), &rootPartcipants, &err))
			{
				auto participantsArr = rootPartcipants["participants"];
				if (participantsArr.isArray())
				{
					for (auto& i : participantsArr)
					{
						if (!i["cid"].asString().contains("champ-select"))
							continue;
						summonerNames.emplace_back(i["game_name"].asString() + "#" + i["game_tag"].asString());
					}
				}
			}
		}

		if (summonerNames.empty())
			return "Failed to get summoner names";

		std::string region;
		if (website == "U.GG") // platformId (euw1, eun1, na1)
		{
			std::string getAuthorization = LCU::Request("GET", "/lol-rso-auth/v1/authorization");
			if (reader->parse(getAuthorization.c_str(), getAuthorization.c_str() + static_cast<int>(getAuthorization.length()),
				&rootRegion, &err))
			{
				region = rootRegion["currentPlatformId"].asString();
			}
		}
		else // region code (euw, eune, na)
		{
			std::string getRegion = LCU::Request("GET", "/riotclient/region-locale");
			if (reader->parse(getRegion.c_str(), getRegion.c_str() + static_cast<int>(getRegion.length()), &rootRegion, &err))
			{
				region = rootRegion["webRegion"].asString();
			}
		}

		if (region.empty())
			return "Failed to get region";

		std::string url;
		if (website == "OP.GG")
		{
			const std::string opggRegion = ToOpggRegion(ParseMultiSearchServer(region));
			if (opggRegion.empty())
				return "Unsupported OP.GG region: " + region;
			url = "https://op.gg/multisearch/" + opggRegion + "?summoners=" + JoinSummonerNames(summonerNames, true, true);
		}
		else if (website == "U.GG")
		{
			url = "https://u.gg/multisearch?summoners=" + JoinSummonerNames(summonerNames, false, false) + "&region=" + Utils::ToLower(region);
		}
		else if (website == "PORO.GG")
		{
			url = "https://poro.gg/multi?region=" + Utils::ToUpper(region) + "&q=" + JoinSummonerNames(summonerNames, false, false);
		}
		else if (website == "Porofessor.gg")
		{
			url = "https://porofessor.gg/pregame/" + region + "/" + JoinSummonerNames(summonerNames, false, false) + "/soloqueue/season";
		}

		if (url.empty())
			return "Unknown multi-search website";

		Utils::OpenUrl(url.c_str(), nullptr, SW_SHOW);
		return url;
	}

	static std::string ChangeRunesOpgg()
	{
		//std::string champSelect = LCU::Request("GET", "/lol-champ-select/v1/session");
		//if (champSelect.empty() || champSelect.find("RPC_ERROR") != std::string::npos)
		//{
		//	return "Champion select not found";
		//}

		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value rootCurrentPage;
		//Json::Value rootCSelect;

		//if (!reader->parse(champSelect.c_str(), champSelect.c_str() + static_cast<int>(champSelect.length()), &rootCSelect, &err))
		//{
		//	return "Failed to get champion select";
		//}

		std::string currentChampion = LCU::Request("GET", "/lol-champ-select/v1/current-champion");
		if (currentChampion == "0" || currentChampion.empty() || currentChampion.find("RPC_ERROR") != std::string::npos)
		{
			return "Champion not picked";
		}

		std::string currentChampionName;
		for (const auto& [key, name, skins] : champSkins)
		{
			if (std::stoi(currentChampion) == key)
			{
				currentChampionName = name;
				break;
			}
		}

		std::string getCurrentPage = LCU::Request("GET", "/lol-perks/v1/currentpage");
		if (!reader->parse(getCurrentPage.c_str(), getCurrentPage.c_str() + static_cast<int>(getCurrentPage.length()), &rootCurrentPage, &err))
		{
			return "Failed to get current rune page";
		}
		std::string currentPageId = rootCurrentPage["id"].asString();

		std::stringstream ssOpgg(cpr::Get(cpr::Url{ "https://www.op.gg/champions/" + Utils::ToLower(currentChampionName) }).text);
		std::vector<std::string> runes;
		std::string primaryPerk, secondaryPerk;

		std::string buf;
		while (ssOpgg >> buf)
		{
			if (runes.size() == 9)
				break;

			if (buf.find("src=\"https://opgg-static.akamaized.net/images/lol/perk") != std::string::npos
				|| buf.find("src=\"https://opgg-static.akamaized.net/meta/images/lol/perk") != std::string::npos)
			{
				if (buf.find("grayscale") != std::string::npos)
					continue;

				if (buf.find("/perkStyle/") != std::string::npos)
				{
					buf = buf.substr(buf.find("/perkStyle/") + strlen("/perkStyle/"), 4);
					if (primaryPerk.empty())
						primaryPerk = buf;
					else if (secondaryPerk.empty())
						secondaryPerk = buf;
				}
				else if (buf.find("/perk/") != std::string::npos)
				{
					buf = buf.substr(buf.find("/perk/") + strlen("/perk/"), 4);
					runes.emplace_back(buf);
				}
				else if (buf.find("/perkShard/") != std::string::npos)
				{
					buf = buf.substr(buf.find("/perkShard/") + strlen("/perkShard/"), 4);
					runes.emplace_back(buf);
				}
			}
		}
		if (runes.size() != 9 || primaryPerk.empty() || secondaryPerk.empty())
		{
			return "Failed to fetch op.gg runes";
		}

		LCU::Request("DELETE", "/lol-perks/v1/pages/" + currentPageId);

		Json::Value rootPage;
		rootPage["name"] = currentChampionName + " OP.GG";
		rootPage["primaryStyleId"] = primaryPerk;
		rootPage["subStyleId"] = secondaryPerk;
		rootPage["selectedPerkIds"] = Json::Value(Json::arrayValue);
		for (const std::string& rune : runes)
			rootPage["selectedPerkIds"].append(rune);
		rootPage["current"] = true;

		return LCU::Request("POST", "lol-perks/v1/pages", rootPage.toStyledString());
	}

	static std::string ChampIdToName(const int& id)
	{
		for (const auto& [key, name, skins] : champSkins)
		{
			if (id == key)
			{
				return name;
			}
		}
		return "";
	}

	// TODO: rewrite and move these to config file
	static std::string GetOldJWT(std::string accId, int& oldtimestamp)
	{
		char* pRoaming;
		size_t roamingLen;
		[[maybe_unused]] errno_t err = _dupenv_s(&pRoaming, &roamingLen, "APPDATA");
		std::string roaming = pRoaming;
		std::string filePath = roaming + "\\tempar.json";

		std::fstream file(filePath, std::ios_base::in);
		if (file.good())
		{
			std::string config;
			std::string temp;
			while (std::getline(file, temp))
			{
				config += temp + "\n";
			}

			Json::Value root;
			Json::CharReaderBuilder builder;
			const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
			JSONCPP_STRING local_err;

			if (reader->parse(config.c_str(), config.c_str() + static_cast<int>(config.length()), &root, &local_err))
			{
				if (auto t = root[accId]; !t.empty())
				{
					std::string oldJWT;
					if (auto t2 = root[accId]["time"]; !t2.empty())
						oldtimestamp = t2.asUInt();
					if (auto t2 = root[accId]["jwt"]; !t2.empty())
						oldJWT = t2.asString();
					file.close();
					return oldJWT;
				}
			}
		}
		file.close();
		return {};
	}

	// true if need new jwt
	static bool CheckJWT(std::string accId)
	{
		char* pRoaming;
		size_t roamingLen;
		[[maybe_unused]] errno_t err = _dupenv_s(&pRoaming, &roamingLen, "APPDATA");
		std::string roaming = pRoaming;
		std::string filePath = roaming + "\\tempar.json";
		unsigned timestamp = 0;

		std::fstream file(filePath, std::ios_base::in);
		if (file.good())
		{
			std::string config;
			std::string temp;
			while (std::getline(file, temp))
			{
				config += temp + "\n";
			}

			Json::Value root;
			Json::CharReaderBuilder builder;
			const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
			JSONCPP_STRING local_err;

			if (reader->parse(config.c_str(), config.c_str() + static_cast<int>(config.length()), &root, &local_err))
			{
				if (auto t = root[accId]; !t.empty())
				{
					std::string oldJWT; // CppEntityAssignedButNoRead
					if (auto t2 = root[accId]["time"]; !t2.empty())
						timestamp = t2.asUInt();
					if (auto t2 = root[accId]["jwt"]; !t2.empty())
						oldJWT = t2.asString();
				}
				else
					return true;
			}
		}
		file.close();

		// if old timestamp is still valid
		if (timestamp + 60 * 60 * 24 > time(nullptr))
		{
			return false;
		}
		return true;
	}

#pragma warning ( push )
#pragma warning (disable : 4996)
	static void SaveJWT(std::string accId, std::string jwt, unsigned timestamp)
	{
		char* pRoaming;
		size_t roamingLen;
		[[maybe_unused]] errno_t err = _dupenv_s(&pRoaming, &roamingLen, "APPDATA");
		std::string roaming = pRoaming;
		std::string filePath = roaming + "\\tempar.json";
		// if file doesn't exist, create new one with {} so it can be parsed
		if (!std::filesystem::exists(filePath))
		{
			std::ofstream file(filePath);
			file << "{}";
			file.close();
		}

		/** \
		* \deprecated Use CharReader and CharReaderBuilder.
		*/
		Json::Reader reader;
		Json::Value root;

		std::ifstream iFile(filePath);
		if (iFile.good())
		{
			if (reader.parse(iFile, root, false))
			{
				root[accId]["jwt"] = jwt;
				root[accId]["time"] = timestamp;

				if (!root.toStyledString().empty())
				{
					std::ofstream oFile(filePath);
					oFile << root.toStyledString() << std::endl;
					oFile.close();
				}
			}
		}
		iFile.close();
	}
#pragma warning( pop )
};
