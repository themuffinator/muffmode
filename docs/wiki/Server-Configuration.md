# Server Configuration

Muff Mode exposes an extended suite of server CVars so hosts can tune everything from match pacing to arena loadouts without re
compiling the DLL. The following tables summarize the most important variables, their defaults, and when to adjust them.

## Core Administration CVars
| CVar | Default | Description |
| --- | --- | --- |
| `g_ruleset` | `2` | Selects the active gameplay ruleset: Quake II Remastered (1), Muff Mode baseline (2), or Quake III Arena style (3); switching rulesets immediately reapplies armor, weapon, and pickup behaviors appropriate for that profile. 【F:docs/progress.txt†L23-L34】
| `g_gametype` | `1` | Chooses the gametype by index, enabling instant transitions between deathmatch, arena, objective, and Horde variants without restarting the server process. 【F:docs/progress.txt†L8-L22】
| `g_owner_auto_join` | `1` | Controls whether a lobby owner automatically joins the match on creation; disable when staging broadcast lobbies or ref-only sessions. 【F:docs/progress.txt†L41-L44】
| `g_motd_filename` | `motd.txt` | Points to the message-of-the-day file; clearing the value reverts to the default banner loaded from baseq2. 【F:docs/progress.txt†L45-L49】

## Match Flow & Round Control
| CVar | Default | Description |
| --- | --- | --- |
| `roundlimit` | `8` | Sets the number of round wins required to capture a series in round-based gametypes such as Clan Arena, CaptureStrike, and Red Rover. 【F:docs/progress.txt†L31-L35】
| `roundtimelimit` | `2` | Defines the per-round time limit (minutes) before a stalemate resolution kicks in. 【F:docs/progress.txt†L35-L36】
| `g_round_countdown` | `10` | Adjusts the pre-round countdown seconds used to synchronize spawns and spectator camera cues. 【F:docs/progress.txt†L36-L37】
| `g_owner_auto_join` | `1` | Prevents or enforces automatic lobby owner participation when matches roll forward, useful for tournament observers. 【F:docs/progress.txt†L41-L44】

## Arena Loadout Tweaks
| CVar | Default | Description |
| --- | --- | --- |
| `g_arena_start_health` | `200` | Defines the starting health pool in arena-derived gametypes, enabling Quake III–style 200/200 loadouts or leaner duel setups. 【F:docs/progress.txt†L37-L40】
| `g_arena_start_armor` | `200` | Controls initial armor value (1–999) and implicitly the tier of armor granted to each player. 【F:docs/progress.txt†L37-L40】
| `g_arena_dmg_armor` | `0` | Toggles whether self-damage subtracts armor in arena modes, letting servers mimic Rocket Arena (no self-damage) or traditional settings. 【F:docs/progress.txt†L37-L40】
| `g_starting_health` | `100` | Sets spawn health outside arena contexts, applying to FFA, TDM, and objective matches alike. 【F:docs/progress.txt†L45-L48】
| `g_starting_armor` | `0` | Mirrors `g_starting_health` for non-arena armor spawn values, allowing “full stack” warmups or barebones instagib loadouts. 【F:docs/progress.txt†L45-L48】

## Powerup, Drop, and Economy Controls
| CVar | Default | Description |
| --- | --- | --- |
| `g_drop_cmds` | `7` | Bitflag gate for what items can be voluntarily dropped: `&1` CTF flags, `&2` powerups, `&4` weapons and ammo. Combine flags (e.g., `3`) to enable subsets. 【F:docs/progress.txt†L27-L33】
| `g_vampiric_exp_min` | `0` | Sets the minimum health a player can decay toward when Vampiric Damage mutator is active, preventing overheal collapse below a configured floor. 【F:docs/progress.txt†L40-L42】
| `bot_name_prefix` | `"B|"` | Customizes bot name prefixes, useful for labeling AI slots or distinguishing skill tiers. 【F:docs/progress.txt†L24-L28】
| `g_allow_kill` | `1` | Enables or disables the classic `kill` self-termination command, often banned in competitive duel rotations. 【F:docs/progress.txt†L26-L27】

## File & Content Overrides
| CVar | Default | Description |
| --- | --- | --- |
| `g_entity_override_dir` | `maps` | Repoints the entity override directory inside baseq2, allowing per-map logic tweaks without touching packaged BSPs. 【F:docs/progress.txt†L40-L41】

## Tips for Hosts
- Pair `g_gametype` with server-side vote restrictions to guarantee that late-joiners see the intended rotation instantly.
- When running mixed-arena playlists, keep `roundlimit` conservative (6–8) to maintain brisk match turnover.
- Disable `g_arena_dmg_armor` if you expect players to rely heavily on rocket-jumps; combined with high `g_arena_start_armor`, this mirrors Rocket Arena expectations.
- Use unique `bot_name_prefix` values when orchestrating PvE Horde events so spectators can immediately identify AI allies.
