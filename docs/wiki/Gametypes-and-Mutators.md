# Gametypes & Mutators

Muff Mode rebuilds the Quake II gametype framework so hosts can pivot between arena, objective, and PvE experiences with a sing
le command. The `g_gametype` index instantly reconfigures the ruleset, stripping legacy toggles such as `duel` while keeping comp
atibility flags (`ctf`, `teamplay`) synchronized. 【F:docs/progress.txt†L4-L22】

## Gametype Catalog
- **Free for All (1)** – Classic deathmatch tuned for Muff Mode’s HUD, scoreboard, and inventory QoL improvements. 【F:docs/progr
ess.txt†L8-L22】
- **Duel (2)** – Queue-based 1v1 battles with refined scoreboard and ready checks for smooth handoffs between challengers. 【F:do
cs/progress.txt†L64-L70】
- **Team Deathmatch (3)** – Teamplay inherits smarter balancing, ready indicators, and EyeCam support for spectators. 【F:docs/wi
ki/Home.md†L8-L34】
- **Capture the Flag (4)** – Works with drop permissions (`g_drop_cmds`) so admins can govern flag handling alongside new vote/st
art commands. 【F:docs/progress.txt†L27-L33】
- **Clan Arena (5)** – Round-based elimination with full weapon loadouts, optional armor/health tuning, and no self-damage by def
ault. 【F:docs/progress.txt†L24-L41】
- **Freeze Tag (6)** – Team revival combat benefitting from round countdowns and arena loadout CVars. 【F:docs/progress.txt†L31-L
40】
- **CaptureStrike (7)** – Alternating attacker/defender rounds combining CTF objectives with clan-arena loadouts. 【F:docs/progre
ss.txt†L24-L33】
- **Red Rover (8)** – Players swap sides on death until one team is empty; round tracking and score limits leverage the new match
 flow CVars. 【F:docs/progress.txt†L24-L36】
- **Last Man Standing (9)** – Survival-driven arena that relies on `roundlimit` and `g_round_countdown` for pacing. 【F:docs/progr
ess.txt†L31-L37】
- **Horde (10)** – Cooperative monster waves where players chase scoreboard supremacy across up to 16 rounds. 【F:docs/progress.t
xt†L24-L29】
- **ProBall (11)** – Fast-paced arena soccer with grappling focus; benefits from hook command bindings and vote support. 【F:docs/
progress.txt†L16-L33】【F:docs/wiki/Home.md†L20-L34】

## Round & Arena Controls
Round-based modes share the same CVars: `roundlimit` (series length), `roundtimelimit` (per-round duration), `g_round_countdown`
(pre-spawn countdown), and arena starting stacks (`g_arena_start_health`, `g_arena_start_armor`, `g_arena_dmg_armor`). These sett
ings empower hosts to mimic Rocket Arena defaults or craft high-pressure sprint formats. 【F:docs/progress.txt†L31-L41】

## Mutator Suite
Mutators such as Vampiric Damage, Nade Fest, Weapons Frenzy, and Quad Hog can be mixed into any gametype, layering scoring twist
s or pickup chaos on top of the base rules. Vampiric play respects `g_vampiric_exp_min`, ensuring leeching never drops health bel
ow a chosen threshold. 【F:docs/wiki/Home.md†L20-L24】【F:docs/progress.txt†L40-L42】

## Rulesets & Balance Context
Three curated rulesets (Quake II Remastered, Muff Mode baseline, Quake III Arena) modulate weapon balance, armor behavior, and pi
ckup cadence. For example, Muff Mode trims Plasma Beam range to 768 units in its take on Q2R balance, while the Q3A profile mimic
s classic arena timing. 【F:docs/wiki/Home.md†L25-L27】【F:docs/progress.txt†L23-L52】

Use `gametype` and `ruleset` commands—plus matching vote options—to script playlists that jump between competitive duel nights, c
asual Horde co-op, and novelty mutator exhibitions without forcing clients to reconnect. 【F:src/g_cmds.cpp†L3519-L3568】【F:src/g_
cmds.cpp†L2632-L2645】
