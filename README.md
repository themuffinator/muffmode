# Muff Mode

Muff Mode is a server-side mod for Quake II Remaster which enhances or provides variety to the overall player experience with a heavy focus on developing the multiplayer. Pulling some inspiration (and code!) from Quake III Arena and Quake Live, it polishes and refines how multiplayer matches are handled and the tools for managing them. Muff Mode also introduces a number of familiar gameplay modifications and match features that will cater for both casual and competitive players alike.

BETA VERSION! There have been a lot of changes made to the source code, please report any bugs as there is a good chance there are some!

FEATURES:
---------
- Refined HUD and scoreboard, more purpose-built for what information is needed. Frag messages and mini scores on HUD (showing position in match) have been added.
- A welcome screen for joining a match and viewing mod and server settings.
- A selection of new game modifications: Vampiric Damage, Nade Fest, Weapons Frenzy.
- A number of minor balance tweaks and many new server settings added.
- Enhanced teamplay with team balancing, improved team handling, communicating joined team to players and friendly fire warnings.
- Extensive controls over specific map item spawns and entity string overrides.
- EyeCam, smooth with aim prediction
- Many more features, see release changelogs for a more info!

v0.12.24 BETA:
---------------
MuffMode 0.12.24

- UI Changes:
	* Integrated admin menu, "Admin" is listed in the game menu for all admins. Slightly tweaked the options, more to come!
	
- Fixes:
	* Fixed a major cause of crashes.
	
- New Game Settings:
	* g_warmup_ready_percentage: in match mode, sets percentile of ready players out of total players required to start the match. Set to 0 to disable readying up. (default: 0.51f)
	* g_teleporter_nofreeze: When set to 1, does not freeze player velocity when teleporting. (default: 0)

- Offhand Hook
	* Added 'hook' and 'unhook' commands to use off-hand hook. Use 'g_grapple_offhand 1' to enable this.
	* Players can use "alias +hook hook; alias -hook unhook; bind mouse2 +hook" to use it as a button command

- Other
	* Bot names are now prefixed with "[BOT] "
	* A flag capture time has been added to CTF, does not apply to capturing dropped flags.

TODO:
-----
- match mode: warmup, readyup, standardised settings etc. (WIP)
- general voting system (WIP)
- tastyspleen.net's mymap system for calling maps
- gametype: Freeze Tag (WIP)
- gametype: Clan Arena (WIP)
- game modifier: quad hog (WIP)
- cleanup and unify code, introduce a gametype and teams handling system similar to Q3's (WIP)
- gametype: Duel (ala-Q3 tournament queueing system, loser goes, winner stays)
- chat tokens
- location tags + .loc file support
- team balancing (WIP)
- Enhanced multiplayer match progression and controls. (WIP)
- Server-side player configs and stats (WIP)
- Mod-based language localisation
- Horde mode (WIP)

CREDITS:
--------
- The Stingy Hat Games YouTube channel for their excellent modding tutorial, without it I would never be able to compile the damned source!
- Nightdive team for the impressive remaster, also some on the team who patiently answered all my annoying modding questions (particularly Paril, sponge)
- id Software, both for Quake II and Quake III Arena (some code ported from the latter)
- ceeeKay for the eyecam code from Q2Eaks