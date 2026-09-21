#include "data_registries.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace hl1 {

bool WeaponRegistry::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "[Data] missing: %s\n", path.c_str()); return false; }
    nlohmann::json j;
    try { f >> j; } catch (const std::exception& e) {
        std::fprintf(stderr, "[Weapons] parse error: %s\n", e.what()); return false;
    }
    if (j.is_null() || !j.is_array()) return false;
    for (const auto& w : j) {
        if (w.is_null()) continue;
        WeaponDef d;
        d.name = w.value("name", "");
        d.type = w.value("type", "melee");
        d.damage = w.value("damage", 20);
        d.fire_rate = w.value("fireRate", 1.0);
        d.pellets = w.value("pellets", 1);
        d.splash = w.value("splash", false);
        m_weapons.push_back(std::move(d));
    }
    std::printf("[Weapons] %zu loaded\n", m_weapons.size());
    return true;
}

const WeaponDef* WeaponRegistry::find(const std::string& name) const {
    for (const auto& w : m_weapons) if (w.name == name) return &w;
    return nullptr;
}

bool EnemyRegistry::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "[Data] missing: %s\n", path.c_str()); return false; }
    nlohmann::json j;
    try { f >> j; } catch (const std::exception& e) {
        std::fprintf(stderr, "[Enemies] parse error: %s\n", e.what()); return false;
    }
    for (const auto& e : j) {
        if (e.is_null() || !e.is_object()) continue;
        EnemyDef d;
        d.name = e.value("name", "");
        d.type = e.value("type", "beast");
        d.hp = e.value("hp", 20);
        d.damage = e.value("damage", 5);
        d.note = e.value("note", "");
        m_enemies.push_back(std::move(d));
    }
    std::printf("[Enemies] %zu loaded\n", m_enemies.size());
    return true;
}

const EnemyDef* EnemyRegistry::find(const std::string& name) const {
    for (const auto& e : m_enemies) if (e.name == name) return &e;
    return nullptr;
}

std::vector<const EnemyDef*> EnemyRegistry::of_type(const std::string& type) const {
    std::vector<const EnemyDef*> out;
    for (const auto& e : m_enemies) if (e.type == type) out.push_back(&e);
    return out;
}

bool MapRegistry::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "[Data] missing: %s\n", path.c_str()); return false; }
    nlohmann::json j;
    try { f >> j; } catch (const std::exception& e) {
        std::fprintf(stderr, "[Maps] parse error: %s\n", e.what()); return false;
    }
    for (const auto& m : j) {
        MapDef d;
        d.name = m.value("name", "");
        d.chapter = m.value("chapter", 1);
        m_maps.push_back(std::move(d));
    }
    std::printf("[Maps] %zu loaded\n", m_maps.size());
    return true;
}

const MapDef* MapRegistry::at(uint32_t idx) const {
    return idx < m_maps.size() ? &m_maps[idx] : nullptr;
}

bool DifficultyManager::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    nlohmann::json j;
    try { f >> j; } catch (...) { return false; }
    if (j.is_null() || !j.is_object()) return false;
    auto profiles = j.value("profiles", nlohmann::json::object());
    if (!profiles.is_object()) return false;
    for (auto& [name, prof] : profiles.items()) {
        if (!prof.is_object()) continue;
        Difficulty d;
        d.profile = name;
        d.player_health_mult = prof.value("player", nlohmann::json::object()).value("health", 100) / 100.0f;
        d.player_dmg_taken_mult = prof.value("player", nlohmann::json::object()).value("damageTakenMultiplier", 1.0);
        d.enemy_health_mult = prof.value("combat", nlohmann::json::object()).value("enemyHealthMultiplier", 1.0);
        d.enemy_damage_mult = prof.value("combat", nlohmann::json::object()).value("enemyDamageMultiplier", 1.0);
        d.enemy_accuracy = prof.value("combat", nlohmann::json::object()).value("enemyAccuracy", 0.7);
        d.ammo_mult = prof.value("economy", nlohmann::json::object()).value("ammoMultiplier", 1.0);
        d.show_enemy_health = prof.value("hud", nlohmann::json::object()).value("showEnemyHealth", true);
        m_profiles.push_back(std::move(d));
    }
    set_profile(j.value("default", "casual"));
    return !m_profiles.empty();
}

const Difficulty* DifficultyManager::get(const std::string& name) const {
    for (const auto& p : m_profiles) if (p.profile == name) return &p;
    return nullptr;
}

void DifficultyManager::set_profile(const std::string& name) {
    const Difficulty* d = get(name);
    if (d) m_current = *d;
    else if (!m_profiles.empty()) m_current = m_profiles[0];
}

// ---- WorldMap impl -------------------------------------------------------

WorldMap::WorldMap() = default;

bool WorldMap::load_from_json(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    nlohmann::json j;
    try { f >> j; } catch (...) { return false; }
    const auto& rows = j.value("map", nlohmann::json::array());
    m_h = rows.size();
    m_w = 0;
    for (const auto& r : rows) {
        auto s = r.get<std::string>();
        m_w = std::max(m_w, (uint32_t)s.size());
    }
    m_tiles.resize(m_w * m_h);
    for (uint32_t y = 0; y < m_h; ++y) {
        auto s = rows[y].get<std::string>();
        for (uint32_t x = 0; x < s.size(); ++x) {
            Tile t;
            t.glyph = s[x];
            t.collision = (s[x] == '#') ? 1 : 0;
            m_tiles[y * m_w + x] = t;
        }
    }
    return true;
}

bool WorldMap::generate_from_desc(const MapDef& desc, uint32_t seed) {
    // Procedural fallback based on chapter number
    m_w = 40; m_h = 20;
    m_tiles.resize(m_w * m_h);
    std::srand(seed);
    for (uint32_t y = 0; y < m_h; ++y)
        for (uint32_t x = 0; x < m_w; ++x) {
            Tile t;
            if (x == 0 || y == 0 || x == m_w-1 || y == m_h-1) { t.glyph = '#'; t.collision = 1; }
            else if ((std::rand() % 100) < 12) { t.glyph = '#'; t.collision = 1; }
            else { t.glyph = '.'; t.collision = 0; }
            m_tiles[y * m_w + x] = t;
        }
    // place start + exit
    at(2, 2).glyph = 'S';
    at(m_w-3, m_h-3).glyph = 'E';
    return true;
}

const Tile& WorldMap::at(uint32_t x, uint32_t y) const {
    return m_tiles[y * m_w + x];
}

Tile& WorldMap::at(uint32_t x, uint32_t y) {
    return m_tiles[y * m_w + x];
}

bool WorldMap::walkable(uint32_t x, uint32_t y) const {
    if (x >= m_w || y >= m_h) return false;
    return m_tiles[y * m_w + x].collision == 0;
}

void WorldMap::place_entity(char glyph, uint32_t x, uint32_t y) {
    if (x < m_w && y < m_h) m_tiles[y * m_w + x].glyph = glyph;
}

} // namespace hl1
