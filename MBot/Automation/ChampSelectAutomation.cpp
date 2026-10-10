#include "ChampSelectAutomation.h"

#include <Windows.h>

#include <chrono>
#include <format>
#include <thread>

#include "Config.h"
#include "JsonUtils.h"
#include "Utils.h"
#include "ChampionService.h"
#include "LcuHttpClient.h"

namespace
{
	bool IsSuccessful(const cpr::Response& response)
	{
		return response.status_code >= 200 && response.status_code < 300;
	}

	std::string ToJson(const Json::Value& value)
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "";
		return Json::writeString(builder, value);
	}
}

GameAutomation::ChampSelectAutomation::ChampSelectAutomation(League::LcuHttpClient& http) : http_(http)
{
}

void GameAutomation::ChampSelectAutomation::Reset()
{
	useBackupId_ = 0;
	dodgeRequested_ = false;
	instantMuteCells_.clear();
	processedCompletedActions_.clear();
}

void GameAutomation::ChampSelectAutomation::Handle(const Json::Value& champSelect)
{
	ProcessCompletedActions(champSelect);
	TryInstantMute(champSelect);
	TryChampionAction(champSelect);
}

bool GameAutomation::ChampSelectAutomation::TeamHasPickIntent(const Json::Value& champSelect,
	const int championId)
{
	if (championId <= 0 || !champSelect["myTeam"].isArray())
		return false;

	const int localCellId = champSelect["localPlayerCellId"].asInt();
	for (const auto& player : champSelect["myTeam"])
	{
		if (player["cellId"].asInt() != localCellId && player["championPickIntent"].asInt() == championId)
			return true;
	}
	return false;
}

int GameAutomation::ChampSelectAutomation::GetAutoBanChampionId(const Json::Value& champSelect)
{
	if (S.gameTab.autoBanId <= 0)
		return 0;
	if (!TeamHasPickIntent(champSelect, S.gameTab.autoBanId))
		return S.gameTab.autoBanId;
	if (S.gameTab.backupBanId <= 0 || TeamHasPickIntent(champSelect, S.gameTab.backupBanId))
		return 0;
	return S.gameTab.backupBanId;
}

void GameAutomation::ChampSelectAutomation::ProcessCompletedActions(const Json::Value& champSelect)
{
	if (!champSelect["actions"].isArray())
		return;

	const int localCellId = champSelect["localPlayerCellId"].asInt();
	for (const auto& actionGroup : champSelect["actions"])
	{
		if (!actionGroup.isArray())
			continue;

		for (const auto& action : actionGroup)
		{
			if (!action["completed"].asBool())
				continue;

			const int actionId = action["id"].asInt();
			if (!processedCompletedActions_.insert(actionId).second || action["actorCellId"].asInt() == localCellId ||
				!S.gameTab.instalockEnabled || S.gameTab.instalockId <= 0)
			{
				continue;
			}

			if (action["championId"].asInt() != S.gameTab.instalockId)
				continue;

			if (action["type"].asString() == "ban")
			{
				if (S.gameTab.dodgeOnBan && !dodgeRequested_)
				{
					dodgeRequested_ = IsSuccessful(http_.Request("POST",
						"/lol-lobby-team-builder/champ-select/v1/session/quit"));
				}
				else if (S.gameTab.backupId > 0)
				{
					useBackupId_ = S.gameTab.backupId;
				}
			}
			else if (action["type"].asString() == "pick" && S.gameTab.backupId > 0)
			{
				useBackupId_ = S.gameTab.backupId;
			}
		}
	}
}

bool GameAutomation::ChampSelectAutomation::FindAction(const Json::Value& champSelect, const int actionId,
	Json::Value& found)
{
	for (const auto& actionGroup : champSelect["actions"])
	{
		if (!actionGroup.isArray())
			continue;

		for (const auto& action : actionGroup)
		{
			if (action["id"].asInt() == actionId)
			{
				found = action;
				return true;
			}
		}
	}
	return false;
}

void GameAutomation::ChampSelectAutomation::TryChampionAction(const Json::Value& champSelect)
{
	if (!champSelect["actions"].isArray())
		return;

	const int localCellId = champSelect["localPlayerCellId"].asInt();
	for (const auto& actionGroup : champSelect["actions"])
	{
		if (!actionGroup.isArray())
			continue;

		for (const auto& action : actionGroup)
		{
			const int actionId = action["id"].asInt();
			if (action["actorCellId"].asInt() != localCellId || action["completed"].asBool() ||
				!action["isInProgress"].asBool())
			{
				continue;
			}

			const std::string actionType = action["type"].asString();
			const bool shouldPick = actionType == "pick" && S.gameTab.instalockEnabled && S.gameTab.instalockId != 0;
			const bool shouldBan = actionType == "ban" && S.gameTab.autoBanEnabled && S.gameTab.autoBanId > 0;
			if (!shouldPick && !shouldBan)
				continue;
			if (shouldBan && champSelect["timer"]["phase"].asString() != "BAN_PICK")
				continue;

			const int delay = shouldPick ? S.gameTab.instalockDelay : S.gameTab.autoBanDelay;
			if (delay > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));

			Json::Value latestChampSelect;
			Json::Value latestAction;
			if (!FetchSession(latestChampSelect) || !FindAction(latestChampSelect, actionId, latestAction) ||
				latestAction["completed"].asBool() || !latestAction["isInProgress"].asBool())
			{
				return;
			}
			if (shouldBan && latestChampSelect["timer"]["phase"].asString() != "BAN_PICK")
				return;

			ProcessCompletedActions(latestChampSelect);
			int championId = 0;
			if (shouldPick)
			{
				championId = useBackupId_ > 0 ? useBackupId_ : S.gameTab.instalockId;
				if (S.gameTab.instalockId == -1)
				{
					const cpr::Response ownedResponse = http_.Request("GET", "/lol-champions/v1/owned-champions-minimal");
					const auto champions = League::ChampionService::ParseOwnedChampions(ownedResponse.text);
					if (champions.empty())
						return;
					championId = champions[Utils::RandomInt(0, static_cast<int>(champions.size() - 1))].first;
				}
			}
			else
			{
				championId = GetAutoBanChampionId(latestChampSelect);
			}

			if (championId <= 0)
				return;

			Json::Value body;
			body["completed"] = true;
			body["championId"] = championId;
			const std::string actionEndpoint = std::format(
				"/lol-champ-select/v1/session/actions/{}", actionId);
			if (IsSuccessful(http_.Request("PATCH", actionEndpoint, ToJson(body))))
				return;
			return;
		}
	}
}

void GameAutomation::ChampSelectAutomation::TryInstantMute(const Json::Value& champSelect)
{
	if (!S.gameTab.instantMute || !champSelect["myTeam"].isArray())
		return;

	const int localCellId = champSelect["localPlayerCellId"].asInt();
	for (const auto& player : champSelect["myTeam"])
	{
		const int cellId = player["cellId"].asInt();
		if (cellId == localCellId || instantMuteCells_.contains(cellId) ||
			(player["puuid"].asString().empty() && player["obfuscatedPuuid"].asString().empty()))
		{
			continue;
		}

		Json::Value body;
		body["summonerId"] = player["summonerId"];
		body["puuid"] = player["puuid"];
		body["obfuscatedSummonerId"] = player["obfuscatedSummonerId"];
		body["obfuscatedPuuid"] = player["obfuscatedPuuid"];
		if (IsSuccessful(http_.Request("POST", "/lol-champ-select/v1/toggle-player-muted", ToJson(body))))
			instantMuteCells_.insert(cellId);
	}
}

bool GameAutomation::ChampSelectAutomation::FetchSession(Json::Value& champSelect)
{
	const cpr::Response response = http_.Request("GET", "/lol-champ-select/v1/session");
	return IsSuccessful(response) && JsonUtils::Parse(response.text, champSelect) && champSelect.isObject();
}
