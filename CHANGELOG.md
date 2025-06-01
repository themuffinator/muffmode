## [0.19.60] - 2025-06-01

### Gameplay Tweaks

- **Renamed CVar:** `g_dm_spawn_farthest` ➜ `g_dm_player_spawn_rule`  
  The original name caused confusion. New values and their effects:
  - `0`: **Full Random** – spawn randomly at any spawn point.
  - `1`: **Half-Random** – spawn randomly within the furthest half of spawn points (default).
  - `2`: **Farthest** – always spawn at the furthest point (poor for low player counts).
  - `3`: **Nearest** – spawn near other players (generally a bad idea).

- **Starting Health Bonus:**  
  Default `g_starting_health_bonus` increased to `25`  
  Aids in balancing matches and reduces dominant railgun abuse. Also helps new players stay in the fight.

---

### Map Item Layout

- **New Entity Keys:**
  - `bfg_on` / `bfg_off`: Allows adjusting of item layout involving/excluding BFG on maps using these keys.
    - Server control: `g_mapspawn_no_bfg 0/1`
  - `plasmabeam_on` / `plasmabeam_off`: Allows adjusting of item layout involving/excluding Plasma Beam on maps using these keys.
    - Server control: `g_mapspawn_no_plasmabeam 0/1`

---

### Gametype Changes

- **Race Gametype Removed**  
  It was non-functional and caused scoreboard bugs in unrelated gametypes.

---

### Bug Fixes

- **Entity Reset Improvements:**  
  Transition from warmup to match start no longer breaks bot weapon selection.

- **Scoreboard Fix:**  
  End-of-match team scores now update correctly. No more outdated scores.

