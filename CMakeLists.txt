cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add source files
set(SOURCES
    src/bots/bot_debug.cpp
    src/bots/bot_exports.cpp
    src/bots/bot_think.cpp
    src/bots/bot_utils.cpp
    src/cg_main.cpp
    src/cg_screen.cpp
    src/g_ai.cpp
    src/g_ai_new.cpp
    src/g_chase.cpp
    src/g_cmds.cpp
    src/g_combat.cpp
    src/g_ctf.cpp
    src/g_func.cpp
    src/g_items.cpp
    src/g_main.cpp
    src/g_menu.cpp
    src/g_misc.cpp
    src/g_monster.cpp
    src/g_monster_spawn.cpp
    src/g_phys.cpp
    src/g_save.cpp
    src/g_spawn.cpp
    src/g_svcmds.cpp
    src/g_target.cpp
    src/g_trigger.cpp
    src/g_activation.cpp
    src/g_turret.cpp
    src/g_utils.cpp
    src/g_weapon.cpp
    src/monsters/m_actor.cpp
    src/monsters/m_arachnid.cpp
    src/monsters/m_berserk.cpp
    src/monsters/m_boss2.cpp
    src/monsters/m_boss3.cpp
    src/monsters/m_boss31.cpp
    src/monsters/m_boss32.cpp
    src/monsters/m_brain.cpp
    src/monsters/m_carrier.cpp
    src/monsters/m_chick.cpp
    src/monsters/m_fixbot.cpp
    src/monsters/m_flipper.cpp
    src/monsters/m_float.cpp
    src/monsters/m_flyer.cpp
    src/monsters/m_gekk.cpp
    src/monsters/m_gladiator.cpp
    src/monsters/m_guardian.cpp
    src/monsters/m_guncmdr.cpp
    src/monsters/m_gunner.cpp
    src/monsters/m_hover.cpp
    src/monsters/m_infantry.cpp
    src/monsters/m_insane.cpp
    src/monsters/m_medic.cpp
    src/monsters/m_move.cpp
    src/monsters/m_mutant.cpp
    src/monsters/m_parasite.cpp
    src/monsters/m_shambler.cpp
    src/monsters/m_soldier.cpp
    src/monsters/m_stalker.cpp
    src/monsters/m_supertank.cpp
    src/monsters/m_tank.cpp
    src/monsters/m_turret.cpp
    src/monsters/m_widow.cpp
    src/monsters/m_widow2.cpp
    src/p_client.cpp
    src/p_hud.cpp
    src/p_hud_victor.cpp
    src/p_menu.cpp
    src/p_menu_statusbar.cpp
    src/p_move.cpp
    src/p_trail.cpp
    src/p_view.cpp
    src/p_weapon.cpp
    src/q_std.cpp

    # JsonCpp sources
    src/vcpkg_installed/x64-windows/vcpkg/blds/jsoncpp/src/1.9.5-13b47286ba.clean/src/lib_json/json_reader.cpp
    src/vcpkg_installed/x64-windows/vcpkg/blds/jsoncpp/src/1.9.5-13b47286ba.clean/src/lib_json/json_value.cpp
    src/vcpkg_installed/x64-windows/vcpkg/blds/jsoncpp/src/1.9.5-13b47286ba.clean/src/lib_json/json_writer.cpp
)

add_library(game SHARED ${SOURCES})

# Add include directories
target_include_directories(game PUBLIC src)

# Set compile definitions
target_compile_definitions(game PRIVATE
    KEX_Q2_GAME
    KEX_Q2GAME_EXPORTS
    NO_FMT_SOURCE
    KEX_Q2GAME_DYNAMIC
    _CRT_SECURE_NO_WARNINGS
    FMT_HEADER_ONLY=1
)
