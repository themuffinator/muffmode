#include "g_local.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include "nlohmann/json.hpp"

#include <set>

using json = nlohmann::json;

const int DEFAULT_RATING = 1500;
const std::string PLAYER_CONFIG_PATH = GAMEVERSION + "/pcfg";

// Function to create a new client config file with default ratings
static void ClientConfig_Create(gclient_t *cl, const std::string &playerID, const std::string &playerName, const std::string &gameType) {
	json newFile = {
		{"socialID", playerID},
		{"playerName", playerName},
		{"originalPlayerName", playerName},
		{"playerAliases", json::array()},
		{"config", {
			{"drawCrosshairID", 1},
			{"drawFragMessages", 1},
			{"drawTimer", 1},
			{"eyeCam", 1},
			{"killBeep", 1}
		}},
		{"ratings", {
			{gameType, DEFAULT_RATING}
		}},
		{"admin",              false},
		{"banned",             false},
		{"lastUpdated",        TimeStamp()}
	};

	try {
		std::ofstream file(G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data());
		if (file.is_open()) {
			file << newFile.dump(4); // Pretty print with 4 spaces
			file.close();
			gi.Com_PrintFmt("Created new client config file: {}/{}.json\n", PLAYER_CONFIG_PATH, playerID);
		} else {
			gi.Com_PrintFmt("Failed to create client config file: {}/{}.json\n", PLAYER_CONFIG_PATH, playerID);
		}
	} catch (const std::exception &e) {
		gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", __FUNCTION__, e.what());
	}
}

void ClientConfig_Init(gclient_t *cl, const std::string &playerID, const std::string &playerName, const std::string &gameType) {
	std::ifstream file(G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, cl->sess.socialID).data());
	json playerData;
	bool modified = false;

	cl->sess.skillRating = 0;

	// Check if the file exists
	if (!file.is_open()) {
		// If the file doesn't exist, create it with default ratings
		ClientConfig_Create(cl, cl->sess.socialID, playerName, gameType);
		return;
	}

	try {
		file >> playerData;
	} catch (const json::parse_error &) {
		gi.Com_PrintFmt("Failed to parse client config file for {}: {}/{}.json\n", playerName, PLAYER_CONFIG_PATH, playerID.c_str());
		return;
	}
	file.close();

	// Handle mismatched player name
	if (playerData.contains("playerName") && playerData["playerName"] != playerName) {
		// Step 1a: Retain the original player name
		if (!playerData.contains("originalPlayerName")) {
			playerData["originalPlayerName"] = playerData["playerName"];
		}

		// Step 1b: Handle player aliases
		if (!playerData.contains("playerAliases")) {
			playerData["playerAliases"] = json::array();
		}

		// Add the current player name to the aliases if not already present
		if (std::find(playerData["playerAliases"].begin(), playerData["playerAliases"].end(), playerName) == playerData["playerAliases"].end()) {
			playerData["playerAliases"].push_back(playerName);
		}

		// Step 1c: Update the player name to the current one
		playerData["playerName"] = playerName;
		modified = true;
	}

	// Check if config exist
	if (!playerData.contains("config") || playerData["config"].empty()) {
		playerData["config"] = {
			{"drawCrosshairID", 1},
			{"drawFragMessages", 1},
			{"drawTimer", 1},
			{"eyeCam", 1},
			{"killBeep", 1}
		};
		modified = true;
	}

	if (!playerData.contains("lastConnected") || playerData["lastConnected"].empty()) {
		playerData["lastConnected"] = TimeStamp();
		modified = true;
	}

	// Check if ratings exist
	if (!playerData.contains("ratings") || playerData["ratings"].empty()) {
		// No skill ratings exist; initialize current to default
		playerData["ratings"] = {
			{gameType, DEFAULT_RATING}
		};
		modified = true;
	} else if (!playerData["ratings"].contains(gameType)) {
		// If the specific game type is missing, set it to the highest rating from other game types
		int maxRating = DEFAULT_RATING;
		for (auto &[type, rating] : playerData["ratings"].items()) {
			maxRating = std::max(maxRating, rating.get<int>());
		}
		playerData["ratings"][gameType] = maxRating;
		modified = true;
	}

	if (modified) {
		playerData["lastUpdated"] = TimeStamp();

		// Write the updated data back to the file
		try {
			std::ofstream outFile(G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data());
			if (outFile.is_open()) {
				outFile << playerData.dump(4); // Pretty print with 4 spaces
				outFile.close();
			} else {
				gi.Com_PrintFmt("Failed to write to client config file for {}: {}/{}.json\n", playerName, PLAYER_CONFIG_PATH, playerID);
			}
		} catch (const std::exception &e) {
			gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", __FUNCTION__, e.what());
		}
	}

	cl->sess.skillRating = playerData["ratings"][gameType];

	if (playerData.contains("admin") && playerData["admin"].is_boolean()) {
		cl->sess.admin = playerData["admin"];
	} else {
		cl->sess.admin = false;
	}
	if (playerData.contains("banned") && playerData["banned"].is_boolean()) {
		cl->sess.banned = playerData["banned"];
	} else {
		cl->sess.banned = false;
	}
}

// Loads PLAYER_CONFIG_PATH/<playerID>.json, runs your updater,
// and if anything actually changed, stamps lastUpdated and saves.
// Returns true if the file was modified & written, false otherwise.
bool ClientConfig_Update(
	const std::string &playerID,
	const std::function<void(json &)> &updater
) {
	const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();

	// 1) load
	std::ifstream in(path);
	if (!in.is_open()) {
		gi.Com_PrintFmt("{}: failed to open {}\n", __FUNCTION__, path.c_str());
		return false;
	}

	json cfg;
	try {
		in >> cfg;
	} catch (const json::parse_error &e) {
		gi.Com_PrintFmt("{}: parse error in {}: {}\n", __FUNCTION__, path.c_str(), e.what());
		return false;
	}
	in.close();

	// 2) snapshot before
	json before = cfg;

	// 3) let caller mutate
	updater(cfg);

	// 4) if nothing changed, do nothing
	if (cfg == before) {
		return false;
	}

	// 5) stamp update time
	cfg["lastUpdated"] = TimeStamp();

	// 6) write back
	try {
		std::ofstream out(path);
		if (!out.is_open()) {
			gi.Com_PrintFmt("{}: failed to write {}\n", __FUNCTION__, path.c_str());
			return false;
		}
		out << cfg.dump(4);
		out.close();

		gi.Com_PrintFmt("{}: saved updates for {}\n", __FUNCTION__, playerID.c_str());
	}

	catch (const std::exception &e) {
		gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", e.what());
		return false;
	}

	return true;
}


// Bulk‑update any top‑level fields in the player’s JSON.
// Example call at the bottom.
bool ClientConfig_BulkUpdate(
	const std::string &playerID,
	const std::initializer_list<std::pair<std::string, json>> &updates) {
	return ClientConfig_Update(playerID, [&](json &cfg) {
		for (auto &kv : updates) {
			cfg[kv.first] = kv.second;
		}
		});
}


bool ClientConfig_DoRatingsUpdate(const std::string &playerID) {
	constexpr int RATING_DELTA = 25;

	ClientConfig_Update(playerID, [&](json &cfg) {
		// 1) Ensure we have a ratings object
		if (!cfg.contains("ratings") || !cfg["ratings"].is_object()) {
			cfg["ratings"] = json::object();
		}
		auto &ratings = cfg["ratings"];

		// 2) Fetch the old rating (or DEFAULT_RATING if missing)
		int oldRating = ratings.value(gt_short_name_upper[g_gametype->integer], DEFAULT_RATING);

		// 3) Write the new rating back
		ratings[gt_short_name_upper[g_gametype->integer]] = oldRating + RATING_DELTA;
		});

	return true;
}
