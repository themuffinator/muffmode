# General Features

Muff Mode layers dozens of quality-of-life upgrades on top of Quake II Remastered so servers feel turnkey at launch and remain f
lexible for event play.

## HUD, Scoreboard, and Control Menu
- A custom HUD and scoreboard surface real-time team status, round results, and ready states, replacing the stock layout with to
urnament-grade overlays. 【F:docs/wiki/Home.md†L4-L13】【F:docs/progress.txt†L64-L70】
- The in-game control menu is synchronized with the status bar rendering pipeline, letting each client navigate match settings w
ithout text command knowledge. 【F:docs/wiki/Home.md†L36-L41】

## Administration & Player Quality-of-Life
- Bundled configs, updated game logic, and bot assets provide a turnkey deployment path for hosts while exposing expanded admin t
ools for match control. 【F:docs/wiki/Home.md†L6-L34】
- Players gain enhanced toggles for announcers, frag messages, timers, grappling hooks, and duel-specific actions, keeping casual
 users and competitors on equal footing. 【F:docs/wiki/Home.md†L31-L34】

## Spectator & Broadcast Enhancements
- EyeCam-style spectating, automated follow modes, and smart camera helpers give observers fine-grained control over match covera
ge, ideal for streamed events. 【F:docs/wiki/Home.md†L8-L9】
- Ready-state indicators and eliminated markers on team scoreboards let casters track momentum in round-based modes without spec
tator guesswork. 【F:docs/progress.txt†L64-L70】

## Level Design Tooling
- Entity override controls, additional target/trigger helpers, and new item classes (personal teleporter, small/large ammo) expan
d what map designers can script without custom DLL builds. 【F:docs/wiki/Home.md†L28-L29】【F:docs/progress.txt†L70-L104】
- Designers can suspend pickups, gate events behind death counts or monster presence, and fire projectile shooters for grenades,
 rockets, BFG, and more, enabling Quake III–style encounter logic inside Quake II maps. 【F:docs/progress.txt†L88-L131】

## Content Packaging
- The mod ships with “Muff Maps,” curated recreations of classic arenas such as Almost Lost and Longest Yard, ensuring consistent
 rotation baselines for leagues. 【F:docs/wiki/Home.md†L8-L9】
- Message-of-the-day management and entity override directories can be customized per event, supporting branded presentations wi
thout rebuilding the DLL. 【F:docs/progress.txt†L40-L49】
