MUFF MODE - v0.12.23 BETA

Muff Mode is a server-side mod for Quake II Remaster which enhances or provides variety to the overall player experience with a heavy focus on developing the multiplayer. Pulling some inspiration (and code!) from Quake III Arena and Quake Live, it polishes and refines how multiplayer matches are handled and the tools for managing them. Muff Mode also introduces a number of familiar gameplay modifications and match features that will cater for both casual and competitive players alike.

BETA VERSION! There have been a lot of changes made to the source code, please report any bugs as there is a good chance there are some!

FEATURES:
---------
- Refined HUD and scoreboard, more purpose-built for what information is needed. Frag messages and mini scores on HUD (showing position in match) have been added.
- A welcome screen for joining a match and viewing mod and server settings.
- A selection of new game modifications: Vampiric Damage, Nade Fest, Weapons Frenzy.
- A number of minor balance tweaks and optional settings.
- Enhanced teamplay with team balancing, improved team handling, communicating joined team to players and friendly fire warnings.
- Extensive controls over specific map item spawns and entity string overrides.
- EyeCam, smooth with aim prediction

INITIAL RELEASE v0.12.23 BETA:
---------------
- UI Changes:
	* reverted deathmatch scoreboard to original style (includes skin icons, removes dynamic tags, etc), added long name and file name of current map to top of scoreboard
	* minimalised hud for InstaGib, Nade Fest
	* welcome screen with server info, objective of the game type/mode, join game option/spectate
	
- Gameplay Balance Changes:
	* Plasma Beam DM damage reduced from 15 to 10.
	* Railgun: restored to 150 damage in campaigns, rail knockback is now equal to damage*2 (no difference in DM).
	* Rockets: removed randomised direct rocket damage value (rand 100-120), now a consistent 110.
	* Invulnerability powerup has been replaced by Protection - player receives no splash damage, full protection from slime damage, third protection from lava, half direct damage after armor protection.
	* Adrenaline: item now also increases max health by 5 in deathmatch.
	* Rebreather: increased holding time from 30 to 60 seconds.
	
- New Gameplay Modifications:
	Weapons Frenzy - faster reloads, faster rockets, regenerating ammo stock
		* g_frenzy: when set to 1, enables Weapons Frenzy mode (default 0)
	Nade Fest - a grenades-only mode
		* g_nadefest: when set to 1, enables Nade Fest (default 0)
	Vampiric Damage - your health slowly drains over time, the only way to boost your health is to inflict damage on your foes!
		* g_vampiric_damage: when set to 1, enables Vampiric Damage mode (default: 0)
	
- New Game Settings:
	* g_dm_no_self_damage: when set to 1, disables any self damage after calculating knockback (default: 0)
	* g_dm_powerup_drop: when set to 1, dropps carried powerups upon death (default: 1)
	* g_dm_powerups_style: when set to 1, enables Q3A powerup rules for all strong powerups - introduces an initial spawn delay (30 to 60 sec), sets 120 sec respawn delay, makes powerup spawn and pickup sounds global. (default: 1)
	* g_dm_respawn_delay_min: the counterpart to g_dm_force_respawn_time, this sets a minimum respawn delay after dying (default: 1)
	* g_dm_respawn_point_min_dist: sets minimum distance to respawn away from previous spawn point (default: 256, max = 512, 0 = disabled)
	* g_dm_respawn_point_min_dist_debug: when set to 1, prints avoiding spawn points when g_dm_respawn_point_min_dist is used (default: 0)
	* g_instagib_splash: enables a non-damaging explosion from railgun shots in instagib, allows for rail jumping or knocking foes about (default 0)
	* g_knockback_scale: scales all knockback resulting from damage received (default 1.0)
	* g_frag_messages: draw frag messages (default 1)
	* g_teamplay_force_balance: force team balancing (default 0)
	* g_mover_speed_scale: sets speed scaling factor for all movers in maps (doors, rotators, lifts etc.) (default: 1.0f)
	* g_corpse_sink_time: sets time in seconds for corpses to sink and disappear (default: 60)
	* g_inactivity: Values above 0 enables an inactivity timer for players, specifying number of seconds since last input to point of flagging the player as inactive. A warning is sent to the player 10 seconds before triggering and once triggered, the player is moved to spectators. Inactive clients are noted as such using the 'players' command. (default: 120)
	* g_no_powerups: disable powerup pickups (Quad, Protection, Double, DuelFire, Invisibility, etc.)
	* g_eyecam: enables eyecam, 0 reverts to chase cam (default: 1)
	* maxplayers: Set max number of players in the game (ie: non-spectators), it is capped to maxclients. In team games, team max size will be maxplayers/2 and rounded down.
	* g_dm_force_join: replaces g_teamplay_force_join, the menu forces the cvar change so this gets around that, it now applies to regular DM too so the change makes sense.
	* g_teamplay_allow_team_pick: When set to 0, denies the ability to pick a specific team during teamplay. This changes the join menu accordingly. (default 0)
	* g_teamplay_force_balance: When set to 1, prohibits joining a team with too many players. (default: 0)
	
- Gameplay Tweaks and Fixes:
	* Instagib and Nade Fest now give players regeneration to recover from environmental damage, falling damage etc.
	* trigger_coop_relay: annoying "all players must be present" feature in co-op has been removed as it proves a game breaker in games with afk players, always treated like a trigger_relay now
	* Quad and Protection player color shells don't change depending on team, avoids confusion
	* func_rotating: Rotating map entities now explode non-player ents such as dropped items, practically this means no more blocked rotator in dm5.
	
- Player Feedback:
	* Added Fragging Spree award - broadcasted message "x is on a fragging spree with x frags" per every 10 frags achieved without dying or killing a team mate
	
- Hacky Map Fixes:
	* bunk1: button for lift to ware2 now has a wait of -1 (never returns), stops co-op players from pushing the button again and toggling the lift!
	
- Map Entity Controls:
	Map Item Replacement Control:
		* <disable/replace>_[classname] and [mapname]_<disable/replace>_[classname] user cvars to remove or replace specific DM map items (by classname) or only in specific maps if desired
		Save and load .ent files to override entire map entity string, located in baseq2/maps/[mapname].ent:
	* g_entity_override_save: when set to 1, will save entity override file upon map load (should one not already be loaded) (default: 0)
	* g_entity_override_load: when set to 1, will load entity override file upon map load (default: 1)
	* New entity keys: "gametype" and "not_gametype": set conditional list of gametypes to respectively spawn or not spawn the entity in. The list can be comma or space separated. The following values correspond to a particular gametype:
		campaign: Campaigns
		ffa: Deathmatch
		tournament: Duel
		team: Team Deathmatch
		ctf: Capture the Flag
		ca: Clan Arena
		ft: Freeze Tag
			Example: "gametype" "ffa tournament" - this will spawn the entity only in deathmatches and duels.
	* New entity keys: "notteam" and "notfree": removes an entity from team gametypes or non-team gametypes respectively.
		Example: "notteam" "1" - the entity will not spawn in team gametypes such as TDM, CTF, FreezeTag and Clan Arena.

TODO:
-----
- match mode: warmup, readyup, standardised settings etc.
- general voting system (WIP)
- tastyspleen.net's mymap system for calling maps
- gametype: freeze tag (WIP)
- gametype: clan arena (WIP)
- game modifier: quad hog (WIP)
- off-hand grapple
- cleanup and unify code, introduce a gametype and teams handling system similar to Q3's (WIP)
- duel gametype (ala-Q3 tournament queueing system, loser goes, winner stays)
- chat tokens
- location tags + .loc file support
- team balancing (WIP)
- A selection of new gametypes: Duel, Freeze Tag, Clan Arena.
- Enhanced multiplayer match progression and controls. (WIP)
- Server-side player configs and stats (WIP)
- Mod-based language localisation

CREDITS:
--------
- The Stingy Hat Games YouTube channel for their excellent modding tutorial, without it I would never be able to compile the damned source!
- Nightdive team for the impressive remaster, also some on the team who patiently answered all my annoying modding questions (particularly Paril, sponge)
- id Software, both for Quake II and Quake III Arena (some code ported from the latter)
- ceeeKay for the eyecam code from Q2Eaks