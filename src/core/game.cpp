#include "game.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace hl1 {

Game::Game() { std::srand(std::time(nullptr)); }

bool Game::initialize() {
    // Load data registries
    m_weapons.load("Content/Data/HL1_Weapons.json");
    m_enemies.load("Content/Data/HL1_Enemies.json");
    m_maps.load("Content/Data/HL1_Maps.json");
    m_diff.load("Content/Data/Difficulties.json");
    m_npcs.load("data/npcs.json");

    // Fallback NPC roster
    if (m_npcs.npcs().empty()) {
        register_hl1_npcs(m_npcs);
    }

    // Apply difficulty
    m_state.player.max_hp = 100 * m_diff.current().player_health_mult;
    m_state.player.hp = m_state.player.max_hp;
    m_state.player.suit = 0;

    std::printf("[Game] Initialized.\n");
    return true;
}

void Game::shutdown() {
    std::printf("[Game] Shutdown.\n");
}

void Game::show_menu() {
    std::printf("\x1b[38;5;208m\x1b[1m");
    std::printf("  HALF-LIFE ELYSIUM: BLACK MESA\n");
    std::printf("  ============================\n");
    std::printf("\x1b[0m");
    std::printf("  \x1b[2mAnomalous materials. Resonance cascade. Freeman.\x1b[0m\n\n");
    for (uint32_t i = 0; i < m_maps.maps().size(); ++i) {
        const MapDef* m = m_maps.at(i);
        std::printf("  %u) Chapter %u: %s\n", i + 1, m->chapter, m->name.c_str());
    }
    std::printf("\n  d) set difficulty (current: %s)\n", m_diff.current().profile.c_str());
    std::printf("  q) quit\n");
}

bool Game::run_chapter(uint32_t idx) {
    const MapDef* desc = m_maps.at(idx);
    if (!desc) { std::printf("Chapter not found.\n"); return false; }
    m_state.chapter = desc->chapter;

    std::printf("\n  \x1b[38;5;208m\x1b[1mCHAPTER %u: %s\x1b[0m\n", desc->chapter, desc->name.c_str());

    // Try load map JSON
    std::string map_path = "Content/Maps/" + std::string(desc->name == "Black Mesa Research Facility" ? "black_mesa" :
        desc->name == "Blast Pit" ? "blast_pit" :
        desc->name == "Surface Tension" ? "surface_tension" :
        desc->name == "Lambda Core" ? "lambda_core" : "black_mesa") + ".json";
    if (!m_world.load_from_json(map_path)) {
      m_world.generate_from_desc(*desc, idx * 7919 + 1);
    }
    // Place NPCs in map
    auto npc_list = m_npcs.in_world("black_mesa");
    for (size_t i = 0; i < npc_list.size() && i < 3; ++i) {
      uint32_t nx = 3 + (i * 5) % (m_world.width() - 6);
      uint32_t ny = 3 + (i * 7) % (m_world.height() - 6);
      m_world.place_entity('N', nx, ny);
    }

    m_state.in_menu = false;
    world_loop();
    return !m_state.game_over;
}

void Game::world_loop() {
    std::printf("  \x1b[2mwasd=move | j=attack | h=heal | r=reload | ?=help | q=quit\x1b[0m\n");
    draw_world();
    while (!m_state.game_over) {
        std::printf("  \x1b[2m[command]\x1b[0m ");
        char buf[64];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cmd;
        for (char* p = buf; *p; ++p)
            if (*p != '\n' && *p != '\r') cmd += (char)std::tolower(*p);
        if (cmd == "quit" || cmd == "q") break;
        handle_command(cmd);
    }
}

void Game::draw_world() {
    for (uint32_t y = 0; y < m_world.height(); ++y) {
        for (uint32_t x = 0; x < m_world.width(); ++x) {
            if (x == m_state.player.x && y == m_state.player.y) {
                std::printf("\x1b[1m@\x1b[0m");
                continue;
            }
            char ch = m_world.at(x, y).glyph;
            if (ch == 'S') std::printf("\x1b[38;5;46mS\x1b[0m");
            else if (ch == 'E') std::printf("\x1b[38;5;220mE\x1b[0m");
            else if (ch == 'N') std::printf("\x1b[38;5;81mN\x1b[0m");
            else if (ch == '#') std::printf("\x1b[38;5;236m#\x1b[0m");
            else if (ch == 'K') std::printf("\x1b[38;5;220mK\x1b[0m");
            else std::printf("%c", ch);
        }
        std::printf("\n");
    }
    std::printf("  \x1b[2mHP: %.0f/%0.0f | Ammo: %u | Suit: %u%%\x1b[0m\n",
                m_state.player.hp, m_state.player.max_hp, m_state.player.ammo, m_state.player.suit);
}

void Game::handle_command(const std::string& cmd) {
    if (cmd == "?" || cmd == "help") {
        std::printf("  wasd=move j=attack h=heal r=reload t=talk\n");
        return;
    }
    int nx = m_state.player.x, ny = m_state.player.y;
    for (char c : cmd) {
        if (c == 'w') ny--;
        else if (c == 's') ny++;
        else if (c == 'a') nx--;
        else if (c == 'd') nx++;
        else if (c == 'j') {
            // random enemy from registry
            auto e = m_enemies.of_type("beast");
            if (!e.empty()) {
                auto chosen = e[std::rand() % e.size()];
                start_combat(*chosen);
            } else if (!m_enemies.enemies().empty()) {
                start_combat(m_enemies.enemies()[std::rand() % m_enemies.enemies().size()]);
            }
        }
        else if (c == 'h') {
            m_state.player.hp = std::min(m_state.player.max_hp, m_state.player.hp + 25);
            std::printf("  \x1b[38;5;46m+25 HP (medkit)\x1b[0m\n");
        }
        else if (c == 'r') {
            std::printf("  \x1b[38;5;220mClick-click. Reloaded.\x1b[0m\n");
        }
        else if (c == 't') {
            // check for NPC at location
            auto npc_list = m_npcs.in_world("black_mesa");
            if (!npc_list.empty()) {
                encounter_npc(*npc_list[std::rand() % npc_list.size()]);
            }
        }

        if (nx >= 0 && ny >= 0 && ny < (int)m_world.height() && nx < (int)m_world.width() &&
            m_world.walkable(nx, ny)) {
            m_state.player.x = nx;
            m_state.player.y = ny;
        }
    }
    draw_world();
}

void Game::encounter_npc(const NPCDef& npc) {
    std::printf("\n  \x1b[38;5;81m\x1b[1m[%s]\x1b[0m \x1b[2m%s\x1b[0m\n",
                npc.name.c_str(), npc.appearance.c_str());
    for (auto& sched : npc.schedules) {
        if (!sched.lines.empty()) {
            auto it = sched.lines.lower_bound(12);
            if (it == sched.lines.end()) it = sched.lines.begin();
            std::printf("  \"%s\"\n", it->second.c_str());
            break;
        }
    }
    m_state.npcs_met++;
}

void Game::start_combat(const EnemyDef& enemy) {
    std::printf("\n  \x1b[38;5;196mA %s rises from the shadows!\x1b[0m\n", enemy.name.c_str());
    if (!enemy.note.empty()) std::printf("  \x1b[2m%s\x1b[0m\n", enemy.note.c_str());

    const WeaponDef* wep = m_weapons.find(m_state.player.weapon_name);
    if (!wep && !m_weapons.weapons().empty()) wep = &m_weapons.weapons()[0];
    if (!wep) {
        std::printf("  No weapon to fight with. Retreat.\n");
        return;
    }

    CombatEngine combat(m_enemies, m_diff);
    combat.engage(enemy, m_state.player.hp, m_state.player.max_hp, *wep);
    std::printf("  \x1b[1mCOMBAT: %s | HP: %u/%u | %s loaded\x1b[0m\n",
                enemy.name.c_str(), combat.enemy_hp(), combat.enemy_max_hp(), wep->name.c_str());
    std::printf("  \x1b[2m[a]ttack [r]eload [h]eal [f]lee | q=retreat\x1b[0m\n");

    while (!combat.is_over()) {
        std::printf("  \x1b[2m[combat]\x1b[0m ");
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cc;
        for (char* p = buf; *p; ++p)
            if (*p != '\n' && *p != '\r') cc += (char)std::tolower(*p);

        std::vector<std::string> log;
        if (cc == "a" || cc == "attack") log = combat.attack();
        else if (cc == "r" || cc == "reload") log = combat.reload();
        else if (cc == "h" || cc == "heal") log = combat.heal(30);
        else if (cc == "f" || cc == "flee") log = combat.flee();
        else if (cc == "q") break;
        else log.push_back("Unknown command. a/r/h/f");

        for (auto& l : log) std::printf("    %s\n", l.c_str());
        if (combat.is_over()) break;
        std::printf("    Enemy HP: %u/%u | Your HP: %u/%u\n",
                     combat.enemy_hp(), combat.enemy_max_hp(),
                     combat.player_hp(), (uint32_t)m_state.player.max_hp);
    }

    if (combat.player_won()) {
        m_state.enemies_killed++;
        std::printf("  \x1b[38;5;46mVictory! The %s collapses.\x1b[0m\n", enemy.name.c_str());
    } else if (combat.state() == CombatState::defeat) {
        m_state.game_over = true;
        std::printf("  \x1b[38;5;196mFreeman is down. The Resonance Cascade claims another.\x1b[0m\n");
    }
    m_state.player.hp = std::max(1u, combat.player_hp());
}

} // namespace hl1

int main() {
    hl1::Game game;
    if (!game.initialize()) return 1;
    std::printf("\x1b[2mLoaded: %zu weapons, %zu enemies, %zu maps, %zu NPCs\x1b[0m\n",
                game.weapons().weapons().size(), game.enemies().enemies().size(),
                game.maps().maps().size(), game.npcs().npcs().size());

    while (true) {
        game.show_menu();
        std::printf("  \x1b[2m[chapter]\x1b[0m ");
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cmd;
        for (char* p = buf; *p; ++p)
            if (*p != '\n' && *p != '\r') cmd += (char)std::tolower(*p);
        if (cmd == "q" || cmd == "quit") break;
        if (cmd == "d") {
            std::printf("    difficulty (casual/legend): ");
            char dbuf[32];
            std::fgets(dbuf, sizeof(dbuf), stdin);
            std::string dname;
            for (char* p = dbuf; *p; ++p)
                if (*p != '\n' && *p != '\r') dname += (char)std::tolower(*p);
            game.diff().set_profile(dname);
            std::printf("    set to %s\n", game.diff().current().profile.c_str());
            continue;
        }
        int ch = std::atoi(cmd.c_str());
        if (ch >= 1 && ch <= (int)game.maps().maps().size()) {
            game.run_chapter(ch - 1);
        }
    }
    game.shutdown();
    return 0;
}
