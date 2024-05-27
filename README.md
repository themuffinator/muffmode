# Welcome to the Muff Mode!

## What is Muff Mode?
Muff Mode is a server-side mod for [QUAKE II Remastered](https://github.com/id-Software/quake2-rerelease-dll). With a focus on multiplayer, it provides an in-game menu,  bug fixes, minor refinements and gameplay tweaks and a host of new settings and features to create a more well-rounded experience.

## Installation
1. Back up your original game_x64.dll in your rerelease dir.
2. Download the latest release zip.
3. Extract the entire zip to your "Quake 2" folder.

## What's in the Bag?
Muff Mode includes the game logic, a server config and some map entity overrides all straight out of the bag!

## Feature Overview
- Refined HUD and scoreboard, more purpose-built for what information is needed and some extra features:
	* Frag messages (also showing position in match)
	* Dynamic miniscores with scorelimit
	* Timer and match state
	* Help texts
- A game menu for joining a match, changing for or voting on settings and viewing mod and server info.
- New gametype: Duel (1v1 winner stays, loser goes mode)
- A selection of new game modifications: Vampiric Damage, Nade Fest, Weapons Frenzy, Quad Hog.
- A whole host of controls for admins, voting and more.
- A number of minor refinements, balance tweaks and many new server settings added.
- Refined match mode with conditional progression, including: warmups, readying, countdowns, post-match delays and more.
- Enhanced teamplay with team auto-balancing, forced balancing rules, improved team handling, communicating joined team to players and friendly fire warnings.
- Extensive controls over specific map item spawns and entity string overrides.
- EyeCam spectating, smooth and with aim prediction (mostly!)
- MyMap map queing system inspired by TastySpleen.net.
- A wide range of minor gameplay bug fixes.
- Many more features, see release changelogs for more info!

## Deathmatch Refinements
- Intermission pre-delay: a one second intermission pre-delay means you can see your winning frag or capture before final scores (no damage is taken or additional scoring during this delay).
- Minimum respawn delay: a short respawn delay helps avoid accidental respawning and creates a smoother transition.
- Kill beeps and frag messages highlight your frags.
- Sudden death: matches go into sudden death whilst scores are tied.

## Map Tweaks
Some entity overrides are included which add some subtle ambient sounds, mover sounds, intermission cams and gametype-specific item tweaks.

## Gameplay Balance Changes
* **Plasma Beam** DM damage reduced from 15 to 10.
* **Railgun**: restored to 150 damage in campaigns, rail knockback is now equal to damage*2 (no difference in DM).
* **Slugs/Railgun**: reduced slugs quantity from 10 to 5, should force more dynamic and challenging gameplay instead of an instagib approach to some matches.
* **Rockets**: removed randomised direct rocket damage value (rand 100-120), now a consistent 120.
* **Invulnerability** powerup has been replaced by Protection - player receives no splash damage, full protection from slime damage, third protection from lava, half direct damage after armor protection.
* **Adrenaline**: item now also increases max health by 5 during deathmatch.
* **Rebreather**: increased holding time from 30 to 45 seconds.
* **Auto Doc**: regen time increased from 500 ms to 1 sec, only regens either health or armor at a time
* **Power Armor**: CTF's 1 damage per cell now applies across deathmatch (originally 2 damage per cell in all DM bar CTF) -- this means same protection but consumes roughly twice the cells.

### Admin Commands
Use **[command] [arg]** for the below listed admin commands:
* **startmatch**: force the match to start, requires warmup.
* **endmatch**: force the match to end, requires a match in progress.
* **resetmatch**: force the match to reset to warmup, requires a match in progress.
* **lockteam [red/blue]**: locks a team from being joined
* **unlockteam [red/blue]**: unlocks a locked team so players can join
* **setteam [clientnum/name]**: forces a team change for a client
* **shuffle**: shuffles and balances the teams, resets the match. Requires a team gametype.
* **balance**: balances the teams without a shuffle, this switches last joined players from stacked team. Requires a team gametype.
* **vote [yes/no]**: passes or fails a voting in progress
* **spawn [entity_name] [spawn_args]**: spawns an entity, works the same as normal spawn server command but allows admins to do this without cheats enabled
* **map**: changes the level to the specified map, map needs to be a part of the map list.
* **nextmap**: forces level change to the next map.
* **map_restart**: restarts current level and session, applies latches cvar changes

### Client Commands
Use **[command] [arg]** for the below listed client commands:
* **help**: toggle help text drawing
* **id**: toggle crosshair ID drawing
* **fm**: toggle frag messages
* **kb**: toggle kill beeps
* **timer**: toggle match timer drawing
* **hook/unhook**: hook/unhook off-hand grapple
* **forfeit**: forfeits a match (currently only in duels, requires g_allow_forfeit 1).
* **ready/notready**: sets ready status.
* **readyup**: toggles ready status.
* **callvote/cv**: calls a vote (use vote commands).
* **vote [yes/no]**: vote or veto a callvote.
* **maplist**: show server map list.
* **mymap**: add a map to the queue, must be a valid map from map list.
* **team [arg]**: selects a team, args:
		- **blue/b**: select blue team
		- **red/r**: select red team
		- **auto/a**: auto-select team
		- **free/f**: join free team (non-team games)
		- **spectator/s**: spectate

### Vote Commands
Use **callvote [command] [arg]** for the below listed vote commands:
* **map**: changes the level to the specified map, map needs to be a part of the map list.
* **nextmap**: forces level change to the next map.
* **restart**: force the match to reset to warmup, requires a match in progress.
* **gametype**: changes gametype to the specified type (ffa|duel|tdm|ctf|ca|ft|horde)
* **timelimit**: changes timelimit to the minutes specified.
* **scorelimit**: changes scorelimit to the value specified.
* **shuffle**: shuffles and balances the teams, resets the match. Requires a team gametype.
* **balance**: balances the teams without a shuffle, this switches last joined players from stacked team. Requires a team gametype.
* **unlagged**: enables or disables lag compensation.
* **cointoss**: randomly returns either HEADS or TAILS.
* **random**: randomly returns a number from 2 to argument value, 100 max.

## Cvar Changes
* g_dm_spawn_farthest: added an option, valid values are as follows:
	- 0: high random (selects random spawn point except the 2 nearest)
	- 1: half farthest (selects random spawn point from the furthest 50% of spawn points
	- 2: spawn farthest to current position
* g_teamplay_force_join: renamed to g_dm_force_join
* sv_*: all mod-based sv_* cvars renamed g_*

## New Cvars
 - **g_allow_admin**: allows administrative powers (default 1)
 - **g_allow_custom_skins**: when set to 0, reverts any custom player models or skins to stock replacements (default: 0)
 - **g_allow_forfeit**: Allows a player to forfeit the match, currently only for Duels (default 1)
 - **g_allow_mymap**: allow mymap (map queuing function) (default 1)
 - **g_allow_spec_vote**: Allows/prohibits voting from spectators. (default 1)
 - **g_allow_vote_midgame**: Allows/prohibits voting during a match. (default 0)
 - **g_allow_voting**: General control over voting, 0 prohibits any voting. (default 0)
 - **g_corpse_sink_time**: sets time in seconds for corpses to sink and disappear (default: 60)
 - **g_dm_do_readyup**: Enforce players to ready up to progress from match warmup stage (requires g_dm_do_warmup 1). (default 0)
 - **g_dm_do_warmup**: Allow match warmup stage. (default 1)
 - **g_dm_force_join**: replaces g_teamplay_force_join, the menu forces the cvar change so this gets around that, it now applies to regular DM too so the change makes sense.
 - **g_dm_no_self_damage**: when set to 1, disables any self damage after calculating knockback (default: 0)
 - **g_dm_overtime**: Set stoppage time for each overtime session in seconds. Currently only applies to Duels. (default 120)
 - **g_dm_powerup_drop**: when set to 1, drops carried powerups upon death (default: 1)
 - **g_dm_powerups_minplayers**: Sets minimum current player load to allow powerup pickups, 0 to disable (default 0)
 - **g_dm_powerups_style**: when set to 1, enables Q3A powerup rules for all major powerups - introduces an initial spawn delay (30 to 60 sec), sets 120 sec respawn delay, makes powerup spawn and pickup sounds global. (default: 1)
 - **g_dm_respawn_delay_min**: the counterpart to g_dm_force_respawn_time, this sets a minimum respawn delay after dying (default: 1)
 - **g_dm_respawn_point_min_dist**: sets minimum distance to respawn away from previous spawn point (default: 256, max = 512, 0 = disabled)
 - **g_dm_respawn_point_min_dist_debug**: when set to 1, prints avoiding spawn points when g_dm_respawn_point_min_dist is used (default: 0)
 - **g_dm_spawnpads**: Controls spawning of deathmatch spawn pads, removes pads when set to 0, 1 only removes in no item game modes, 2 forces pads in all dm matches. (default: 1)
 - **g_fast_doors**: When set to 1, doubles the default speed of standard and rotating doors (default 1)
 - **g_frag_messages**: draw frag messages (default 1)
 - **g_inactivity**: Values above 0 enables an inactivity timer for players, specifying number of seconds since last input to point of flagging the player as inactive. A warning is sent to the player 10 seconds before triggering and once triggered, the player is moved to spectators. Inactive clients are noted as such using the 'players' command. (default: 120)
 - **g_instagib_splash**: enables a non-damaging explosion from railgun shots in instagib, allows for rail jumping or knocking foes about (default 0)
 - **g_knockback_scale**: scales all knockback resulting from damage received (default 1.0)
 - **g_ladder_steps**: Allow ladder step sounds, 1 = only in campaigns, 2 = always on (default 1)
 - **g_match_lock**: when set to 1, prohibits joining the match while in progress (default 0)
 - **g_mover_speed_scale**: sets speed scaling factor for all movers in maps (doors, rotators, lifts etc.) (default: 1.0f)
 - **g_no_powerups**: disable powerup pickups (Quad, Protection, Double, DuelFire, Invisibility, etc.)
 - **g_showhelp**: when set to 1, prints a quick explanation about game modifications to players. (default: 1)
 - **g_teamplay_allow_team_pick**: When set to 0, denies the ability to pick a specific team during teamplay. This changes the join menu accordingly. (default 0)
 - **g_teamplay_auto_balance**: Set to 1, enforces team rebalancing during a match. The last joined player(s) of the stacked team switches teams but retain their scores. (default 1)
 - **g_teamplay_force_balance**: When set to 1, prohibits joining a team with too many players. (default: 0)
 - **g_teamplay_item_drop_notice**: When set to 1, sends team notice of item drops. (default 1)
 - **g_teleporter_nofreeze**: When set to 1, does not freeze player velocity when teleporting. (default: 0)
 - **g_vote_flags**: Bitmask to disable specific vote options. (default 0)
 - **g_vote_limit**: Sets maximum number of votes per match per client, 0 for no limit. (default 3)
 - **g_warmup_ready_percentage**: in match mode, sets percentile of ready players out of total players required to start the match. Set to 0 to disable readying up. (default: 0.51f)
 - **g_weapon_projection**: changes weapon projection offset. 0 = normal, 1 = always force central handedness, 2 = force central view projection. looks strange with view weapons. (default: 0)
 - **hostname**: set string for server name, this gets printed at top of game menu for all to see. Limit this to 26 chars max.
 - **maxplayers**: Set max number of players in the game (ie: non-spectators), it is capped to maxclients. In team games, team max size will be maxplayers/2 and rounded down.
 - **mercylimit**: Sets score gap limit to end match, 0 to disable (default 0)
 - **noplayerstime**: Sets time in minutes in which there have been no players to force a change of map, 0 to disable (default 0)

### Offhand Hook
- Added 'hook' and 'unhook' commands to use off-hand hook. Use `g_grapple_offhand 1` to enable this.
- Players can use ``alias +hook hook; alias -hook unhook; bind mouse2 +hook`` to use it as a button command

### New Gameplay Modifications
* **Weapons Frenzy**: faster reloads, faster rockets, regenerating ammo stock
	* **g_frenzy**: when set to 1, enables Weapons Frenzy mode (default 0)
* **Nade Fest**: a grenades-only mode
	* **g_nadefest**: when set to 1, enables Nade Fest (default 0)
* **Vampiric Damage**: your health slowly drains over time, the only way to boost your health is to inflict damage on your foes!
	* **g_vampiric_damage**: when set to 1, enables Vampiric Damage mode (default: 0)
* **Quad Hog**: find the Quad Damage to become the Quad Hog! Score frags by killing the Quad Hog or killing while the Quad Hog.
	* **g_quadhog**: when set to 1, enables Quad Hog mode (default: 0)

### Gameplay Tweaks and Fixes
 * Instagib and Nade Fest now give players regeneration to recover from environmental damage, falling damage etc.
 * trigger_coop_relay: annoying "all players must be present" feature in co-op has been removed as it proves a game breaker in games with afk players, always treated like a trigger_relay now
 * Quad and Protection player color shells don't change depending on team, avoids confusion
 * func_rotating: Rotating map entities now explode non-player ents such as dropped items, practically this means no more blocked rotator in dm5.
 * Player Feedback:
	* Added Fragging Spree award - broadcasted message "x is on a fragging spree with x frags" per every 10 frags achieved without dying or killing a team mate
 * Techs: fixed not being able to pick up your dropped tech
 * Many more!
 
### Map Entity Controls
 * Map Item Replacement Control:
	* **<disable/replace>_[classname]** and **[mapname]_<disable/replace>_[classname]** user cvars to remove or replace specific DM map items (by classname) or only in specific maps if desired
 * Save and load .ent files to override entire map entity string, located in baseq2/maps/[mapname].ent:
	* **g_entity_override_save**: when set to 1, will save entity override file upon map load (should one not already be loaded) (default: 0)
	* **g_entity_override_load**: when set to 1, will load entity override file upon map load (default: 1)
 * New entity keys**: "gametype" and "not_gametype": set conditional list of gametypes to respectively spawn or not spawn the entity in. The list can be comma or space separated. The following values correspond to a particular gametype:
	campaign: Campaigns
	ffa: Deathmatch
	tournament: Duel
	team: Team Deathmatch
	ctf: Capture the Flag
	ca: Clan Arena
	ft: Freeze Tag
		Example: "gametype" "ffa tournament" - this will spawn the entity only in deathmatches and duels.
 * New entity keys**: "**notteam**" and "**notfree**": removes an entity from team gametypes or non-team gametypes respectively.
	Example: "**notteam**" "1" - the entity will not spawn in team gametypes such as TDM, CTF, FreezeTag and Clan Arena.
 * misc_teleporter: **"mins"/"maxs" "x y z"** entity keys to override teleport trigger size, removes teleporter pad if either keys are set
 * new item spawnflag & 32768: item spawns in suspended state (does not drop to floor)
 * Hacky Map Fixes:
	* bunk1: button for lift to ware2 now has a wait of -1 (never returns), stops co-op players from pushing the button again and toggling the lift!

## TODO
- tastyspleen.net's mymap system: add support for level flags
- gametype: Freeze Tag (WIP)
- gametype: Clan Arena (WIP)
- Server-side player configs and stats (WIP)
- Horde mode (WIP)

## FIXME
- the occasional bugged lifts with no fix in sight :(

## Credits:
- The Stingy Hat Games YouTube channel for their excellent modding tutorial, without it I would never be able to compile the damned source!
- Nightdive team for the impressive remaster, also some on the team who patiently answered all my annoying modding questions (particularly Paril, sponge)
- id Software, both for Quake II and Quake III Arena (some code ported from the latter)
- ceeeKay for the eyecam code from Q2Eaks
