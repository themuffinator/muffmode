# Intermission Flow Test: Scoreboard Persistence

## Objective
Confirm that deathmatch players remain on the scoreboard view throughout intermission instead of being forced back to other HUD layouts.

## Preconditions
- Start a multiplayer deathmatch session with at least one human or bot participant.
- Reach a state where fraglimit or timelimit is close to completion so intermission can be triggered quickly.

## Steps
1. Play until the match ends naturally (or use an admin command to end the match) to enter intermission.
2. Observe the client HUD as intermission begins.
3. Remain idle during the entire intermission countdown without pressing the score key or toggling menus.

## Expected Result
- The scoreboard remains visible for the entire intermission without reverting to another HUD state.
- Player input is not required to keep the scoreboard displayed during intermission.
