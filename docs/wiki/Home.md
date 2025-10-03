# Muff Mode Wiki

## Home
**Muff Mode** is a server-side overhaul for Quake II Remastered aimed at server hosts, level designers, and players who want richer multiplayer tools, new creative hooks, and quality-of-life upgrades such as enhanced HUD information and configurable settings.

The distribution ships with updated game logic, curated server configuration, bot assets, and map entity overrides so hosts can deploy a turnkey experience out of the box.

## Reference Pages
- [Server Configuration](Server-Configuration.md)
- [Command Reference](Command-Reference.md)
- [General Features](General-Features.md)
- [Gametypes & Mutators](Gametypes-and-Mutators.md)
- [Match Features & Balance](Match-Features-and-Balance.md)

Headline features include a purpose-built HUD/scoreboard, an in-game control menu, expanded admin tooling, deeper match-flow logic, smarter teamplay helpers, entity override controls, EyeCam spectating, and bundled “Muff Maps” recreations of classic arenas (Almost Lost, Arena of Death, Hidden Fortress, Longest Yard, Proving Grounds, Vertical Vengeance).

## Installation & Setup
1. Locate the Quake II installation (Steam default: `C:\Program Files (x86)\Steam\steamapps\common\Quake 2\`).
2. Back up `rerelease/baseq2/game_x64.dll`.
3. Download the latest Muff Mode release archive.
4. Extract it into the root “Quake 2” folder (allow the DLL replacement).
5. Launch the game; optional configs can be executed once a session is active.
6. After creating a lobby, run `exec muff-sv.cfg` to load the server preset.

## Gameplay Overview

### New Modes & Mutators
Muff Mode introduces round-based clan arena variants, duels, CaptureStrike, Red Rover, Horde waves, and more, alongside mutators such as Vampiric Damage, Nade Fest, Weapons Frenzy, and Quad Hog, plus match refinements like intermission pre-delays, respawn smoothing, and kill feedback enhancements.

The bundled progress notes reinforce the rewired gametype system (instant switching, indexed `g_gametype`), enumerate the supported rotation (Campaign through ProBall), and highlight round limits, countdown controls, and other server CVars that back the new match flows.

### Rulesets
Three packaged rulesets let hosts mirror Quake II defaults, Muff Mode balance tweaks (e.g., adjusted Plasma Beam, rail ammo counts, revamped Protection), or Quake III–style weapon/armor behavior with respawn timing and pickup rules that better emulate arena shooters.

### Built-in Content & Items
Beyond the Muff Maps lineup, Muff Mode adds personal teleporters, small/large ammo variants, and numerous trigger/target helpers for designers (e.g., target_remove_powerups, target_cvar, projectile shooters) to script complex encounters without custom DLL work.

## Server Administration & Player Controls
Administrators gain direct commands for match control, team locking/shuffling, gametype and ruleset switching, vote arbitration, and on-demand entity spawning, while players receive client toggles (announcer, frag messages, timer), hook bindings, spectating aids, and duel forfeits. Voting commands mirror the admin toolkit for democratic map, gametype, and settings changes.

Progress updates document new CVars such as `g_ruleset`, `bot_name_prefix`, `g_drop_cmds`, `roundlimit`, and `g_motd_filename`, which surface prominently in Muff Mode’s configuration menus.

The code base centralizes these CVars via macro-driven descriptors so that every stage (pre-init vs. runtime) registers defaults and flags consistently.

Command handling lives in `g_cmds.cpp`, which annotates each command with flags (admin-only, dead-allowed, cheat-protected) and routes execution through helper checks before invoking gameplay logic; it also drives inventory navigation logic used by both menus and spectator systems.

In-game menus leverage `P_Menu_Open`, `P_Menu_UpdateEntry`, and `P_Menu_Do_Update` to duplicate static layouts per client, refresh localized text, and sync to the status bar rendering pipeline.

## Code Architecture & Build

### Core Modules
- `g_main.cpp` owns the global state (`game`, `level`, registry of localization buffers), registers all CVars per stage, and maps each `gametype_t` to behavior flags while keeping legacy CVars (`ctf`, `teamplay`) in sync.
- `p_client.cpp` defines spawnpoint entities for deathmatch, coop, and team modes, handles stuck-spawn fixes, instanced-item logic for coop, and maintains stock model/skin tables for character selection.
- `p_hud.cpp` transitions clients into intermission, resets powerup timers, updates scoreboard layouts, and drives end-of-unit summaries broadcast via `svc_layout` messages.
- `cg_main.cpp` exposes the client-game API (HUD drawing, layout flags, weapon/powerup wheel state, hit markers), propagates movement configuration (`pm_config`), and hooks configstring updates to toggle N64 physics or air acceleration on the fly.
- `bg_local.h` and `q_std.h` supply shared math/utilities, compressed ammo/powerup stat helpers, and formatting utilities (`G_Fmt`, `G_FmtTo`) that replace heap-heavy formatting at runtime.
- `g_spawn.cpp` catalogs every entity factory—from player spawns and movers to Quake III–style targets—so map parsing can bind classnames to spawn functions, including Muff Mode’s additional triggers and shooters.
- `bots/bot_utils.cpp` tracks per-entity state for bot awareness (movement flags, powerup timers, animation gestures), seeds armor metadata, and registers players with the bot subsystem, enabling AI to reason about equipment and situational context.

### Build System & Dependencies
The repository ships a Visual Studio 2022 solution and CMake-style project configured for a 64-bit dynamic library with v143 toolset, manifest-based vcpkg integration, and fmt/jsoncpp include paths for both Debug and Release builds.

Dependencies are tracked via `vcpkg.json`, which pins `fmt` and `jsoncpp` against a known baseline so Windows builds can bootstrap consistent vendor libraries.

## Level Design & Extensibility
The progress journal enumerates designer-facing enhancements: suspended item spawnflags, trigger variants gated by monster presence or death counts, scripted cvar setters, scoreboard-aware target_score, and projectile shooters for grenades, rockets, BFG, and more—each enabling Quake III–style logic within Quake II maps without external scripting.

Combined with the expanded item set, designers can craft arenas tailored to Muff Mode’s gametypes and mutators.

## Testing
⚠️ Tests not run (static repository review only).
