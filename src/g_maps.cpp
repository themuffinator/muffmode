
#include "g_local.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <sstream>
#include <regex>

/*
==================
LoadMapPool
==================
*/
void LoadMapPool(gentity_t *ent) {
	bool entClient = ent && ent->client;
	game.mapSystem.mapPool.clear();

	const char *path = g_maps_pool_file->string;
	std::ifstream file(path);
	if (!file.is_open()) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] Failed to open file: {}\n", path);
		return;
	}

	nlohmann::json j;
	try {
		file >> j;
	} catch (const std::exception &e) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] JSON parsing failed: {}\n", e.what());
		return;
	}

	if (!j.is_array()) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] Root must be a JSON array\n");
		return;
	}

	for (const auto &entry : j) {
		if (!entry.contains("filename") || !entry["filename"].is_string())
			continue;

		MapEntry map;
		map.filename = entry["filename"].get<std::string>();

		if (entry.contains("longName")) map.longName = entry["longName"].get<std::string>();
		if (entry.contains("minPlayers")) map.minPlayers = entry["minPlayers"].get<int>();
		if (entry.contains("maxPlayers")) map.maxPlayers = entry["maxPlayers"].get<int>();
		if (entry.contains("gametype")) map.suggestedGametype = static_cast<gametype_t>(entry["gametype"].get<int>());
		if (entry.contains("ruleset")) map.suggestedRuleset = static_cast<ruleset_t>(entry["ruleset"].get<int>());
		if (entry.contains("scorelimit")) map.scoreLimit = entry["scorelimit"].get<int>();
		if (entry.contains("timelimit")) map.timeLimit = entry["timelimit"].get<int>();
		if (entry.contains("popular")) map.isPopular = entry["popular"].get<bool>();
		if (entry.contains("custom")) map.isCustom = entry["custom"].get<bool>();

		if (entry.contains("dm") && entry["dm"].get<bool>()) map.mapTypeFlags |= MAP_DM;
		if (entry.contains("sp") && entry["sp"].get<bool>()) map.mapTypeFlags |= MAP_SP;
		if (entry.contains("coop") && entry["coop"].get<bool>()) map.mapTypeFlags |= MAP_COOP;

		map.isCycleable = false;
		map.lastPlayed = 0_sec;

		game.mapSystem.mapPool.push_back(std::move(map));
	}
	
	if (entClient)
		gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] Loaded {} map entries from '{}'.\n", game.mapSystem.mapPool.size(), path);
}

/*
==================
LoadMapCycle
==================
*/
void LoadMapCycle(gentity_t *ent) {
	bool entClient = ent && ent->client;
	const char *path = g_maps_cycle_file->string;
	std::ifstream file(path);
	if (!file.is_open()) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapCycle] Failed to open file: {}\n", path);
		return;
	}

	// Reset cycleable flags
	for (auto &m : game.mapSystem.mapPool)
		m.isCycleable = false;

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// Remove single-line comments
	content = std::regex_replace(content, std::regex("//.*"), "");

	// Remove multi-line comments (match across lines safely)
	content = std::regex_replace(content, std::regex("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/"), "");

	std::istringstream stream(content);
	std::string token;
	int matched = 0, unmatched = 0;

	while (stream >> token) {
		for (auto &m : game.mapSystem.mapPool) {
			if (_stricmp(token.c_str(), m.filename.c_str()) == 0) {
				m.isCycleable = true;
				matched++;
				goto next_token;
			}
		}
		unmatched++;
	next_token:;
	}

	if (entClient)
		gi.LocClient_Print(ent, PRINT_HIGH, "[MapCycle] Marked {} maps cycleable, ignored {} unknown entries.\n", matched, unmatched);
}

std::optional<MapEntry> AutoSelectNextMap() {
	const auto &pool = game.mapSystem.mapPool;
	const int playerCount = level.num_playing_human_clients;

	std::vector<const MapEntry *> eligible;

	const gtime_t now = level.time;
	const bool avoidCustom = (level.num_console_clients > 0);

	// Step 1: Filter for cycleable maps
	for (const auto &map : pool) {
		// skip non-cycleable maps unless none are cycleable
		if (!map.isCycleable)
			continue;

		// Skip if played recently
		if (map.lastPlayed > 0_sec && (now - map.lastPlayed) < 30_min)
			continue;

		// Skip custom maps if console players present
		if (avoidCustom && map.isCustom)
			continue;

		// Skip if min/max not suitable
		if ((map.minPlayers > 0 && playerCount < map.minPlayers) ||
			(map.maxPlayers > 0 && playerCount > map.maxPlayers))
			continue;

		eligible.push_back(&map);
	}

	// Step 2: Fallback if none eligible
	if (eligible.empty()) {
		for (const auto &map : pool) {
			// skip maps played very recently
			if (map.lastPlayed > 0_sec && (now - map.lastPlayed) < 30_min)
				continue;
			if (avoidCustom && map.isCustom)
				continue;
			eligible.push_back(&map);
		}
	}

	// Final fallback: just use the pool if still empty
	if (eligible.empty()) {
		for (const auto &map : pool) {
			if (avoidCustom && map.isCustom)
				continue;
			eligible.push_back(&map);
		}
	}

	if (eligible.empty())
		return std::nullopt;

	// Step 3: Prefer popular maps (weighting)
	std::vector<const MapEntry *> weighted;
	for (const auto *map : eligible) {
		weighted.push_back(map);
		if (map->isPopular)
			weighted.push_back(map); // duplicate = increased chance
	}

	// Randomly choose one
	const MapEntry *chosen = weighted[rand() % weighted.size()];
	return *chosen;
}

std::vector<const MapEntry *> SelectVoteCandidates(int maxCandidates = 3) {
	std::vector<const MapEntry *> pool;
	const int playerCount = level.num_playing_human_clients;
	const bool avoidCustom = (level.num_console_clients > 0);
	const gtime_t now = level.time;

	for (const auto &map : game.mapSystem.mapPool) {
		if (!map.isCycleable)
			continue;
		if (map.lastPlayed > 0_sec && (now - map.lastPlayed) < 30_min)
			continue;
		if ((map.minPlayers > 0 && playerCount < map.minPlayers) ||
			(map.maxPlayers > 0 && playerCount > map.maxPlayers))
			continue;
		if (avoidCustom && map.isCustom)
			continue;
		pool.push_back(&map);
	}

	if (pool.size() < 2) {
		pool.clear();
		for (const auto &map : game.mapSystem.mapPool) {
			if (map.lastPlayed > 0_sec && (now - map.lastPlayed) < 30_min)
				continue;
			if (avoidCustom && map.isCustom)
				continue;
			pool.push_back(&map);
		}
	}

	std::shuffle(pool.begin(), pool.end(), std::default_random_engine(level.time.milliseconds()));
	if (pool.size() > maxCandidates)
		pool.resize(maxCandidates);
	return pool;
}
