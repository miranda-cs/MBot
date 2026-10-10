#include "ChampionService.h"

#include <algorithm>
#include <memory>
#include <ranges>

#include <json/json.h>

#include "LCU.h"
#include "Utils.h"
#include "Misc.h"

std::vector<std::pair<int, std::string>> League::ChampionService::ParseOwnedChampions(const std::string& response)
{
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING error;
	Json::Value root;
	if (!reader->parse(response.data(), response.data() + response.size(), &root, &error) || !root.isArray())
		return {};

	std::vector<std::pair<int, std::string>> champions;
	for (const auto& champion : root)
	{
		const bool available = champion["freeToPlay"].asBool() || champion["ownership"]["owned"].asBool() ||
			(champion["ownership"].isMember("xboxGPReward") && champion["ownership"]["xboxGPReward"].asBool());
		if (!available)
			continue;

		const std::string name = Misc::GetChampionAliasFromLoadScreenPath(champion["baseLoadScreenPath"].asString());
		if (name.empty() || Misc::IsJadeChampionAlias(name))
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
