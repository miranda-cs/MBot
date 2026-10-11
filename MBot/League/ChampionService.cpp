#include "ChampionService.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <format>
#include <map>
#include <ranges>
#include <string_view>
#include <thread>

#include <cpr/cpr.h>
#include <json/json.h>

#include "JsonUtils.h"
#include "LCU.h"

namespace
{
	bool ParseJson(const std::string& response, Json::Value& root)
	{
		return JsonUtils::Parse(response, root);
	}

	bool IsJadeChampionAlias(const std::string& alias)
	{
		return alias.rfind("Jade_", 0) == 0;
	}

	std::string GetChampionAliasFromLoadScreenPath(const std::string& loadScreenPath)
	{
		constexpr std::string_view marker = "ASSETS/Characters/";
		const size_t nameStart = loadScreenPath.find(marker);
		if (nameStart == std::string::npos)
			return {};

		const size_t aliasStart = nameStart + marker.size();
		const size_t aliasEnd = loadScreenPath.find('/', aliasStart);
		if (aliasEnd == std::string::npos)
			return {};
		return loadScreenPath.substr(aliasStart, aliasEnd - aliasStart);
	}

	std::vector<ChampMinimal> ParseChampionCollection(const std::string& response)
	{
		Json::Value root;
		if (!ParseJson(response, root) || !root.isArray())
			return {};

		std::vector<ChampMinimal> champions;
		champions.reserve(root.size());
		for (const auto& championJson : root)
		{
			ChampMinimal champion{};
			champion.active = championJson["active"].asBool();
			champion.alias = championJson["alias"].asString();
			champion.banVoPath = championJson["banVoPath"].asString();
			champion.baseLoadScreenPath = championJson["baseLoadScreenPath"].asString();
			champion.botEnabled = championJson["botEnabled"].asBool();
			champion.chooseVoPath = championJson["chooseVoPath"].asString();
			champion.freeToPlay = championJson["freeToPlay"].asBool();
			champion.id = championJson["id"].asInt();
			champion.name = championJson["name"].asString();

			const Json::Value& ownership = championJson["ownership"];
			champion.freeToPlayReward = ownership["freeToPlayReward"].asBool();
			champion.owned = ownership["owned"].asInt();
			champion.purchased = champion.freeToPlay && !champion.owned
				? "0"
				: championJson["purchased"].asString();
			champion.rankedPlayEnabled = championJson["rankedPlayEnabled"].asBool();
			champion.squarePortraitPath = championJson["squarePortraitPath"].asString();
			champion.stingerSfxPath = championJson["stingerSfxPath"].asString();
			champion.title = championJson["title"].asString();
			champions.emplace_back(std::move(champion));
		}
		return champions;
	}

	std::vector<ChampMastery> ParseChampionMastery(const std::string& response)
	{
		Json::Value root;
		if (!ParseJson(response, root) || !root.isArray())
			return {};

		std::vector<ChampMastery> masteries;
		masteries.reserve(root.size());
		for (const auto& masteryJson : root)
		{
			ChampMastery mastery{};
			mastery.championId = masteryJson["championId"].asInt();
			mastery.championLevel = masteryJson["championLevel"].asInt();
			mastery.championPoints = masteryJson["championPoints"].asInt();
			mastery.championPointsSinceLastLevel = masteryJson["championPointsSinceLastLevel"].asInt();
			mastery.championPointsUntilNextLevel = masteryJson["championPointsUntilNextLevel"].asInt();
			mastery.chestGranted = masteryJson["chestGranted"].asBool();
			mastery.formattedChampionPoints = masteryJson["formattedChampionPoints"].asString();
			mastery.formattedMasteryGoal = masteryJson["formattedMasteryGoal"].asString();
			mastery.highestGrade = masteryJson["highestGrade"].asString();
			mastery.lastPlayTime = std::to_string(masteryJson["lastPlayTime"].asUInt64());
			mastery.playerId = std::to_string(masteryJson["playerId"].asInt64());
			mastery.tokensEarned = masteryJson["tokensEarned"].asInt();
			masteries.emplace_back(std::move(mastery));
		}
		return masteries;
	}

	std::string ResolveSkinName(const int itemId)
	{
		for (const auto& champion : champSkins)
		{
			for (const auto& [skinId, skinName] : champion.skins)
			{
				if (itemId == std::stoi(skinId))
					return skinName;
			}
		}
		return {};
	}
}

std::vector<std::pair<int, std::string>> League::ChampionService::ParseOwnedChampions(const std::string& response)
{
	Json::Value root;
	if (!ParseJson(response, root) || !root.isArray())
		return {};

	std::vector<std::pair<int, std::string>> champions;
	for (const auto& champion : root)
	{
		const bool available = champion["freeToPlay"].asBool() || champion["ownership"]["owned"].asBool() ||
			(champion["ownership"].isMember("xboxGPReward") && champion["ownership"]["xboxGPReward"].asBool());
		if (!available)
			continue;

		const std::string name = GetChampionAliasFromLoadScreenPath(champion["baseLoadScreenPath"].asString());
		if (name.empty() || IsJadeChampionAlias(name))
			continue;

		champions.emplace_back(champion["id"].asInt(), name);
	}

	std::ranges::sort(champions, [](const auto& left, const auto& right) { return left.second < right.second; });
	return champions;
}

std::vector<std::pair<int, std::string>> League::ChampionService::GetOwnedChampions()
{
	return ParseOwnedChampions(LCU::Request("GET", "/lol-champions/v1/owned-champions-minimal"));
}

std::string League::ChampionService::GetCurrentPatch()
{
	Json::Value root;
	const std::string response = cpr::Get(
		cpr::Url{ "http://ddragon.leagueoflegends.com/api/versions.json" }).text;
	if (JsonUtils::Parse(response, root) && root.isArray() && !root.empty())
		return root[0].asString();
	return "0.0.0";
}

void League::ChampionService::LoadChampionSkins()
{
	Json::Value root;
	for (int attempt = 0; attempt < 3; attempt++)
	{
		const std::string response = cpr::Get(cpr::Url{
			"https://raw.communitydragon.org/latest/plugins/rcp-be-lol-game-data/global/default/v1/skins.json"
			}).text;
		root.clear();
		if (JsonUtils::Parse(response, root) && root.isObject())
			break;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (!root.isObject())
		return;

	std::map<std::string, Champ> champions;
	for (const std::string& id : root.getMemberNames())
	{
		const Json::Value& skin = root[id];
		if (!skin["isBase"].asBool())
			continue;

		const std::string championName = GetChampionAliasFromLoadScreenPath(skin["loadScreenPath"].asString());
		if (championName.empty() || IsJadeChampionAlias(championName))
			continue;

		std::string championId = id;
		if (championId.size() >= 3 && championId.ends_with("000"))
			championId.erase(championId.size() - 3);

		Champ& champion = champions[championName];
		champion.name = championName;
		champion.key = std::stoi(championId);
		champion.skins.emplace(champion.skins.begin(), id, "default");
	}

	for (const std::string& id : root.getMemberNames())
	{
		const Json::Value& skin = root[id];
		if (skin["isBase"].asBool())
			continue;

		const std::string championName = GetChampionAliasFromLoadScreenPath(skin["loadScreenPath"].asString());
		if (championName.empty() || IsJadeChampionAlias(championName) || !champions.contains(championName))
			continue;

		Champ& champion = champions[championName];
		if (skin["questSkinInfo"])
		{
			for (const auto& tier : skin["questSkinInfo"]["tiers"])
				champion.skins.emplace_back(tier["id"].asString(), tier["name"].asString());
		}
		else
		{
			champion.skins.emplace_back(id, skin["name"].asString());
		}
	}

	std::vector<Champ> loadedChampions;
	loadedChampions.reserve(champions.size());
	for (auto& [name, champion] : champions)
		loadedChampions.emplace_back(std::move(champion));
	champSkins = std::move(loadedChampions);
}

std::vector<ChampAll> League::ChampionService::GetChampionCollection()
{
	Json::Value session;
	if (!ParseJson(LCU::Request("GET", "/lol-login/v1/session"), session))
		return {};

	const std::string summonerId = session["summonerId"].asString();
	if (summonerId.empty())
		return {};

	const std::vector<ChampMinimal> champions = ParseChampionCollection(LCU::Request("GET",
		std::format("/lol-champions/v1/inventories/{}/champions-minimal", summonerId)));
	const std::vector<ChampMastery> masteries = ParseChampionMastery(LCU::Request("GET",
		std::format("/lol-collections/v1/inventories/{}/champion-mastery", summonerId)));

	std::vector<ChampAll> collection;
	collection.reserve(champions.size());
	for (const ChampMinimal& champion : champions)
	{
		if (!champion.owned)
			continue;

		ChampAll entry{};
		entry.min = champion;
		if (const auto mastery = std::ranges::find_if(masteries, [&](const ChampMastery& candidate) {
			return candidate.championId == champion.id;
			}); mastery != masteries.end())
		{
			entry.mas = *mastery;
		}
		collection.emplace_back(std::move(entry));
	}
	return collection;
}

std::vector<Skin> League::ChampionService::GetOwnedSkins()
{
	Json::Value root;
	if (!ParseJson(LCU::Request("GET", "/lol-inventory/v2/inventory/CHAMPION_SKIN"), root) || !root.isArray())
		return {};

	std::vector<Skin> skins;
	skins.reserve(root.size());
	for (const auto& skinJson : root)
	{
		Skin skin{};
		skin.inventoryType = skinJson["inventoryType"].asString();
		skin.itemId = skinJson["itemId"].asInt();
		skin.ownershipType = skinJson["ownershipType"].asString();
		skin.isVintage = skinJson["payload"]["isVintage"].asBool();

		const std::string purchase = skinJson["purchaseDate"].asString();
		if (purchase.find('-') == std::string::npos && purchase.find(':') == std::string::npos)
		{
			sscanf_s(purchase.c_str(), "%04d%02d%02dT%02d%02d%02d",
				&skin.purchaseDate.tm_year, &skin.purchaseDate.tm_mon, &skin.purchaseDate.tm_mday,
				&skin.purchaseDate.tm_hour, &skin.purchaseDate.tm_min, &skin.purchaseDate.tm_sec);
		}
		else
		{
			sscanf_s(purchase.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d",
				&skin.purchaseDate.tm_year, &skin.purchaseDate.tm_mon, &skin.purchaseDate.tm_mday,
				&skin.purchaseDate.tm_hour, &skin.purchaseDate.tm_min, &skin.purchaseDate.tm_sec);
		}
		skin.purchaseDate.tm_year -= 1901;
		skin.purchaseDate.tm_mon -= 1;

		skin.quantity = skinJson["quantity"].asInt();
		skin.uuid = skinJson["uuid"].asString();
		skin.name = ResolveSkinName(skin.itemId);
		skins.emplace_back(std::move(skin));
	}
	return skins;
}
