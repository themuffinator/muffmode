# Match Features & Balance

Beyond new gametypes, Muff Mode hardens match flow and rebalances combat to better suit competitive play.

## Match Lifecycle Improvements
- Instant gametype switching ensures playlists can jump between FFA, team, and arena modes without rebooting the lobby. 【F:docs/
progress.txt†L4-L22】【F:docs/progress.txt†L64-L70】
- Ready status indicators on every scoreboard reduce false starts and highlight eliminated players in round-based modes. 【F:docs
/progress.txt†L64-L70】
- Warmup quality-of-life additions—intermission pre-delays, smoother respawns, duel forfeits, and countdown CVars—keep matches f
air while giving admins precise timing control. 【F:docs/wiki/Home.md†L20-L34】【F:docs/progress.txt†L31-L38】
- Message-of-the-day management now pulls from `motd.txt`, with the `loadmotd` command refreshing broadcasts mid-session. 【F:doc
s/progress.txt†L74-L78】【F:src/g_cmds.cpp†L3543-L3549】

## Voting & Arbitration
- Players can democratically adjust maps, time/score limits, rulesets, or request shuffles via the expanded vote catalog, while a
dmins retain `forcevote` to resolve stalled ballots. 【F:src/g_cmds.cpp†L2632-L2645】【F:src/g_cmds.cpp†L3519-L3568】
- Spectator participation and mid-game votes are governed by CVars, helping tournament organizers lock down competitive environm
ents. 【F:src/g_cmds.cpp†L2758-L2816】

## Balance Highlights
- Plasma Beam range is capped at 768 units in the Quake II Remastered ruleset, tightening long-range pressure. 【F:docs/progress.
txt†L52-L54】
- Armor and weapon pickup behaviors shift with the active ruleset—Muff Mode adjusts ammo counts and protection values, while the
 Quake III Arena profile mirrors that series’ timing. 【F:docs/wiki/Home.md†L25-L27】【F:docs/progress.txt†L23-L52】
- Item drops are more permissive: blasters, grapples, and current weapons can now be dropped, supporting team economy plays. 【F:
docs/progress.txt†L69-L73】
- Smart weapon auto-switching prioritizes higher-tier weapons (SSG over SG, CG over MG) and avoids forced chainfist swaps, keepin
g player intent intact. 【F:docs/progress.txt†L69-L73】

## Arena Loadout Tuning
Round-based modes use configurable starting stacks and optional self-damage armor rules, enabling hosts to recreate Rocket Arena,
Quake III duel, or experimental high-health formats by simply editing CVars. 【F:docs/progress.txt†L31-L41】

## Cooperative & PvE Support
Horde waves scale up to 16 rounds, leveraging the same scoreboard infrastructure and drop rules so PvE nights feel consistent wit
h competitive events. 【F:docs/progress.txt†L24-L36】【F:docs/progress.txt†L64-L73】
