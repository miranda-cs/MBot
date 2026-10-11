#pragma once

#include <ctime>
#include <string>
#include <utility>
#include <vector>

#include "Definitions.h"

namespace League::ChampionService
{
	std::vector<std::pair<int, std::string>> ParseOwnedChampions(const std::string& response);
	std::vector<std::pair<int, std::string>> GetOwnedChampions();
	std::string GetCurrentPatch();
	void LoadChampionSkins();
	std::vector<ChampAll> GetChampionCollection();
	std::vector<Skin> GetOwnedSkins();
}
