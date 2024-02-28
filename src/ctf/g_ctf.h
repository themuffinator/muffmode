// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#define CTF_VERSION 1.52
#define CTF_VSTRING2(x) #x
#define CTF_VSTRING(x) CTF_VSTRING2(x)
#define CTF_STRING_VERSION CTF_VSTRING(CTF_VERSION)

constexpr const char *TEAM_RED_SKIN = "ctf_r";
constexpr const char *TEAM_BLUE_SKIN = "ctf_b";

constexpr int32_t CTF_CAPTURE_BONUS = 15;	  // what you get for capture
constexpr int32_t CTF_TEAM_BONUS = 10;   // what your team gets for capture
constexpr int32_t CTF_RECOVERY_BONUS = 1;	  // what you get for recovery
constexpr int32_t CTF_FLAG_BONUS = 0;   // what you get for picking up enemy flag
constexpr int32_t CTF_FRAG_CARRIER_BONUS = 2; // what you get for fragging enemy flag carrier
constexpr gtime_t CTF_FLAG_RETURN_TIME = 40_sec;  // seconds until auto return

constexpr int32_t CTF_CARRIER_DANGER_PROTECT_BONUS = 2; // bonus for fraggin someone who has recently hurt your flag carrier
constexpr int32_t CTF_CARRIER_PROTECT_BONUS = 1; // bonus for fraggin someone while either you or your target are near your flag carrier
constexpr int32_t CTF_FLAG_DEFENSE_BONUS = 1; 	// bonus for fraggin someone while either you or your target are near your flag
constexpr int32_t CTF_RETURN_FLAG_ASSIST_BONUS = 1; // awarded for returning a flag that causes a capture to happen almost immediately
constexpr int32_t CTF_FRAG_CARRIER_ASSIST_BONUS = 2;	// award for fragging a flag carrier if a capture happens almost immediately

constexpr float CTF_TARGET_PROTECT_RADIUS = 400;   // the radius around an object being defended where a target will be worth extra frags
constexpr float CTF_ATTACKER_PROTECT_RADIUS = 400; // the radius around an object being defended where an attacker will get extra frags when making kills

constexpr gtime_t CTF_CARRIER_DANGER_PROTECT_TIMEOUT = 8_sec;
constexpr gtime_t CTF_FRAG_CARRIER_ASSIST_TIMEOUT = 10_sec;
constexpr gtime_t CTF_RETURN_FLAG_ASSIST_TIMEOUT = 10_sec;

constexpr gtime_t CTF_AUTO_FLAG_RETURN_TIMEOUT = 30_sec; // number of seconds before dropped flag auto-returns

constexpr gtime_t CTF_TECH_TIMEOUT = 60_sec; // seconds before techs spawn again

constexpr int32_t CTF_DEFAULT_GRAPPLE_SPEED = 650; // speed of grapple in flight
constexpr float	  CTF_DEFAULT_GRAPPLE_PULL_SPEED = 650; // speed player is pulled at

bool GT_CTF_PickupFlag(edict_t *ent, edict_t *other);
void GT_CTF_DropFlag(edict_t *ent, gitem_t *item);
void GT_CTF_Effects(edict_t *player);
void Teams_CalcScores();
void Teams_CalcRankings(std::array<uint32_t, MAX_CLIENTS> &player_ranks); // [Paril-KEX]
void CheckEndTDMLevel(); // [Paril-KEX]
void GT_CTF_DeadDropFlag(edict_t *self);
//void CTFTeam_f(edict_t *ent);
void GT_CTF_FlagSetup(edict_t *ent);
void GT_CTF_ResetFlag(team_t team);
void GT_CTF_ScoreBonuses(edict_t *targ, edict_t *inflictor, edict_t *attacker);
void GT_CTF_CheckHurtCarrier(edict_t *targ, edict_t *attacker);
void Menu_Dirty();

void Menu_Open_Join(edict_t *ent);
void Cmd_Vote_f(edict_t *ent);
void Match_ResetAllPlayers();
void GT_CTF_ResetFlags();

bool Match_CheckRules();

void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);

void Team_Join(edict_t *ent, team_t desired_team, bool inactive);
