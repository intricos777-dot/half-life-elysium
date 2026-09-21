#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace hl1 {

struct WeaponDef {
    std::string name;
    std::string type;
    uint32_t damage = 0;
    float fire_rate = 1.0f;
    uint32_t pellets = 1;
    bool splash = false;
};

class WeaponRegistry {
public:
    bool load(const std::string& path);
    const std::vector<WeaponDef>& weapons() const { return m_weapons; }
    const WeaponDef* find(const std::string& name) const;
private:
    std::vector<WeaponDef> m_weapons;
};

struct EnemyDef {
    std::string name;
    std::string type;
    uint32_t hp = 20;
    uint32_t damage = 5;
    std::string note;
};

class EnemyRegistry {
public:
    bool load(const std::string& path);
    const std::vector<EnemyDef>& enemies() const { return m_enemies; }
    const EnemyDef* find(const std::string& name) const;
    std::vector<const EnemyDef*> of_type(const std::string& type) const;
private:
    std::vector<EnemyDef> m_enemies;
};

struct MapDef {
    std::string name;
    uint32_t chapter = 1;
};

class MapRegistry {
public:
    bool load(const std::string& path);
    const std::vector<MapDef>& maps() const { return m_maps; }
    const MapDef* at(uint32_t idx) const;
private:
    std::vector<MapDef> m_maps;
};

struct Difficulty {
    std::string profile;
    float player_health_mult = 1.0f;
    float player_dmg_taken_mult = 1.0f;
    float enemy_health_mult = 1.0f;
    float enemy_damage_mult = 1.0f;
    float enemy_accuracy = 0.7f;
    float ammo_mult = 1.0f;
    bool show_enemy_health = true;
};

class DifficultyManager {
public:
    bool load(const std::string& path);
    const Difficulty* get(const std::string& name) const;
    const Difficulty& current() const { return m_current; }
    void set_profile(const std::string& name);
private:
    std::vector<Difficulty> m_profiles;
    Difficulty m_current;
};

struct Tile {
    char glyph = '.';
    uint8_t collision = 0;
};

class WorldMap {
public:
    WorldMap();
    bool load_from_json(const std::string& path);
    bool generate_from_desc(const MapDef& desc, uint32_t seed);
    uint32_t width() const { return m_w; }
    uint32_t height() const { return m_h; }
    const Tile& at(uint32_t x, uint32_t y) const;
    Tile& at(uint32_t x, uint32_t y);
    bool walkable(uint32_t x, uint32_t y) const;
    void place_entity(char glyph, uint32_t x, uint32_t y);
private:
    std::vector<Tile> m_tiles;
    uint32_t m_w = 0, m_h = 0;
};

} // namespace hl1
