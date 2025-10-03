# Muff Mode

Muff Mode is a server-side overhaul for [Quake II Remastered](https://github.com/id-Software/quake2-rerelease-dll). It packages updated game logic, curated configs, bot data, and entity overrides so hosts can spin up modernized multiplayer servers with minimal setup.

## Quick Start
1. Back up `rerelease/baseq2/game_x64.dll` in your Quake II install (`C:\Program Files (x86)\Steam\steamapps\common\Quake 2\` by default on Steam).
2. Download the latest [Muff Mode release](https://github.com/themuffinator/muffmode/releases/latest).
3. Extract the archive into the root Quake II folder and allow the DLL replacement.
4. Launch the game, create a lobby, then run `exec muff-sv.cfg` to load the bundled server preset.

## Project Highlights
- Round-based and objective-focused gametypes with supporting mutators.
- A purpose-built HUD, scoreboard, and in-game control menu for players.
- Extensive admin and voting commands, plus EyeCam spectating tools.
- Designer-facing entity additions enabling Quake III–style encounter scripting.

## Documentation
The full wiki covers installation details, gameplay systems, rulesets, level design helpers, and build information. Start here: [`docs/wiki/Home.md`](docs/wiki/Home.md).

## Contributing
Issues and pull requests are welcome. Please review the wiki and existing documentation before filing feature requests or submitting changes.
