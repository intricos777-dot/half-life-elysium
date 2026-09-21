#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "data/data_registries.h"
#include "world/npc.h"
#include "combat/combat.h"

namespace hl1 {

struct PlayerState {
    float hp = 100;
    float max_hp = 100;
    uint32_t ammo = 90;
    uint32_t suit = 0;
    uint32_t chapter = 1;
    uint32_t x = 2, y = 2;
    std::string weapon_name = "Crowbar";
};

struct GameState {
    bool in_menu = true;
    bool game_over = false;
    PlayerState player;
    uint32_t chapter = 1;
    uint32_t maps_cleared = 0;
    uint32_t npcs_met = 0;
    uint32_t enemies_killed = 0;
};

class Game {
public:
    Game();
    bool initialize();
    void shutdown();

    // Flows
    void show_menu();
    bool run_chapter(uint32_t idx);
    void advance_map();
    void world_loop();
    void encounter_npc(const NPCDef& npc);
    void start_combat(const EnemyDef& enemy);

    // Access
    GameState& state() { return m_state; }
    WeaponRegistry& weapons() { return m_weapons; }
    EnemyRegistry& enemies() { return m_enemies; }
    MapRegistry& maps() { return m_maps; }
    NPCDb& npcs() { return m_npcs; }
    WorldMap& world() { return m_world; }
    DifficultyManager& diff() { return m_diff; }

private:
    GameState m_state;
    WeaponRegistry m_weapons;
    EnemyRegistry m_enemies;
    MapRegistry m_maps;
    NPCDb m_npcs;
    WorldMap m_world;
    DifficultyManager m_diff;

    void draw_world();
    void handle_command(const std::string& cmd);
};

} // namespace hl1
