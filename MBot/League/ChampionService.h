#pragma once

#include <string>
#include <utility>
#include <vector>

namespace League::ChampionService
{
	std::vector<std::pair<int, std::string>> ParseOwnedChampions(const std::string& response);
	std::vector<std::pair<int, std::string>> GetOwnedChampions();
}
