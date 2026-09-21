#pragma once
#include <string>
#include <vector>
#include "data/data_registries.h"
#include "world/npc.h"

namespace hl1 {

enum class CombatState { idle, player_turn, enemy_turn, victory, defeat };

struct CombatRound {
    std::string attacker;
    std::string target;
    uint32_t damage = 0;
    std::string action;
    bool crit = false;
};

class CombatEngine {
public:
    CombatEngine(const EnemyRegistry& enemies, const DifficultyManager& diff);

    // Start a fight
    void engage(const EnemyDef& enemy, float player_hp, float player_max_hp,
                const WeaponDef& weapon);

    // Turn-based action: returns log lines
    std::vector<std::string> attack();
    std::vector<std::string> use_weapon(size_t slot);
    std::vector<std::string> reload();
    std::vector<std::string> heal(uint32_t amount);
    std::vector<std::string> flee();

    bool is_over() const { return m_state == CombatState::victory || m_state == CombatState::defeat; }
    bool player_won() const { return m_state == CombatState::victory; }
    uint32_t enemy_hp() const { return m_enemy_hp; }
    uint32_t enemy_max_hp() const { return m_enemy_max_hp; }
    uint32_t player_hp() const { return m_player_hp; }
    CombatState state() const { return m_state; }
    const CombatRound& last_round() const { return m_last; }

private:
    const EnemyRegistry& m_enemy_db;
    const DifficultyManager& m_diff;
    CombatState m_state = CombatState::idle;
    uint32_t m_enemy_hp = 0;
    uint32_t m_enemy_max_hp = 0;
    uint32_t m_player_hp = 0;
    uint32_t m_player_max_hp = 0;
    const EnemyDef* m_enemy = nullptr;
    const WeaponDef* m_weapon = nullptr;
    uint32_t m_ammo_in_mag = 0;
    uint32_t m_mag_size = 30;
    uint32_t m_ammo_reserve = 90;
    CombatRound m_last;

    void enemy_attack();
    void check_victory();
};

// Boss fight special handler
class BossFight {
public:
    BossFight(const EnemyDef& boss, float player_hp);

    std::vector<std::string> phase_intro();
    std::vector<std::string> player_attack(uint32_t damage, const std::string& method);
    std::vector<std::string> boss_action();

    bool is_over() const;
    float boss_hp() const { return m_hp; }
    float boss_max_hp() const { return m_max_hp; }

private:
    float m_hp, m_max_hp;
    uint32_t m_phase = 1;
    std::string m_name;
};

} // namespace hl1
