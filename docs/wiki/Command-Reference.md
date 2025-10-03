# Command Reference

Muff Mode expands the stock Quake II command set with a layered permission system so that everyday players, match officials, an
d mappers can share the same server. Commands are registered with flag combinations in `client_cmds[]`, determining who can exe
cute them, whether they function for spectators, and if cheats must be enabled. 【F:src/g_cmds.cpp†L3501-L3589】

## Admin & Match Control Commands
The `CF_ADMIN_ONLY` flag marks commands that require an authenticated admin session. They cover every phase of a match:

- **Team management** – `balance`, `lockteam`, `unlockteam`, `setteam`, `shuffle`, `balance`, and `boot` let staff reshuffle or re
move players to keep sides fair. 【F:src/g_cmds.cpp†L3505-L3580】
- **Match state** – `startmatch`, `resetmatch`, `endmatch`, `readyall`, `unreadyall`, and `time-out`/`time-in` pairs give referees
full control over countdowns and pauses. 【F:src/g_cmds.cpp†L3505-L3580】
- **Map rotation** – `nextmap`, `setmap`, `map_restart`, `mymap`, and `spawn` handle rotation changes, queue curation, and entity
spawning for exhibition events. 【F:src/g_cmds.cpp†L3509-L3584】
- **Rules adjustments** – `gametype`, `ruleset`, and `forcevote` instantly apply new modes or rulesets without restarting, while `
loadmotd` and `motd` update messaging. 【F:src/g_cmds.cpp†L3519-L3549】

Players may request admin status with the `admin` command, after which `CF_ADMIN_ONLY` commands become available when the underl
ying checks succeed. 【F:src/g_cmds.cpp†L3502-L3522】

## General Player Controls
Commands without admin requirements focus on quality-of-life and spectating support:

- **Inventory & equipment** – `drop`, `invdrop`, `invnext`, `invprev`, `invuse`, `weapnext`, `weapprev`, and `use` mirror the in-g
ame HUD controls, ensuring keyboard binds work even while navigating menus. 【F:src/g_cmds.cpp†L3511-L3589】
- **Spectator tools** – `follow`, `followkiller`, `followleader`, `followpowerup`, `where`, `ghost`, and `team` let observers jump
between points of interest and toggle chase cameras. 【F:src/g_cmds.cpp†L3515-L3590】
- **Communication & feedback** – `announcer`, `fm` (frag messages), `timer`, `kb`, and `alertall` toggle HUD messaging while `hel
p`, `score`, `stats`, and `players` report server state. 【F:src/g_cmds.cpp†L3503-L3577】
- **Match readiness** – `ready`, `readyup`, `notready`, `forfeit`, and `vote` manage individual participation in duel queues, tea
m warmups, and running ballots. 【F:src/g_cmds.cpp†L3520-L3589】

## Cheat-Protected & Debug Commands
The `CF_CHEAT_PROTECT` bit prevents potentially disruptive commands from running unless `g_cheats` is enabled. Typical examples
include developer aids (`give`, `god`, `noclip`, `notarget`, `teleport`, `clear_ai_enemy`, `kill_ai`) and diagnostic tools (`list
entities`, `listmonsters`, `target`, `setpoi`). 【F:src/g_cmds.cpp†L3508-L3576】

## Voting System
`callvote` (or its alias `cv`) exposes a server-configurable ballot system. When invoked, the command enumerates every vote type
whose flag is not masked out by `g_vote_flags`, then validates the request against a suite of CVars:

- Voting must be globally enabled via `g_allow_voting`, and servers can block mid-game calls with `g_allow_vote_midgame`.
- Spectator voting requires `g_allow_spec_vote`; otherwise only active players may initiate ballots.
- `g_vote_limit` caps how many proposals a single player can start per map.
- The server defers new votes while `level.vote_time` or `level.vote_execute_time` is non-zero to avoid overlaps.

These checks are all enforced inside `Cmd_CallVote_f`. 【F:src/g_cmds.cpp†L2758-L2816】

Once accepted, the vote stores the initiating player as `level.vote_client`, queues a centerprint prompt, and opens the voting m
enus for eligible participants. Casting a ballot via `vote` updates `level.vote_yes` or `level.vote_no`, with spectator access g
uarded again. 【F:src/g_cmds.cpp†L2824-L2858】

### Available Vote Commands
`vote_cmds[]` defines the ballot catalog, pairing validator/payload callbacks with help text and flag bits that can be disabled i
ndividually. 【F:src/g_cmds.cpp†L2632-L2645】 Supported votes include map changes, match restarts, gametype swaps, time/score lim
it adjustments, shuffle/balance requests, unlagged toggles, cointoss decisions, random number rolls, and ruleset changes.

## Tips
- Use `forcevote` if an admin needs to override the outcome of a stalled ballot without interrupting match flow. 【F:src/g_cmds.c
pp†L3519-L3550】
- Encourage spectators to rely on `followleader` and `followpowerup` so automated camera tracking highlights key plays, reducing
manual camera work during broadcasts. 【F:src/g_cmds.cpp†L3515-L3554】
