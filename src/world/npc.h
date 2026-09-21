#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace hl1 {

using Hour = uint32_t;

struct NPCSchedule {
    std::string location;
    std::map<Hour, std::string> lines;
};

struct NPCDef {
    std::string id;
    std::string name;
    std::string role;
    std::string affiliation;
    std::string appearance;
    std::string current_world;
    float x = 0, y = 0;
    std::vector<NPCSchedule> schedules;
};

class NPCDb {
public:
    bool load(const std::string& path);
    std::vector<NPCDef>& npcs() { return m_npcs; }
    const std::vector<NPCDef>& npcs() const { return m_npcs; }
    const NPCDef* find(const std::string& id) const;
    std::vector<const NPCDef*> in_world(const std::string& world) const;
    const NPCDef* at_location(const std::string& world, float x, float y) const;
private:
    std::vector<NPCDef> m_npcs;
};

// Hardcoded NPC roster if JSON missing
void register_hl1_npcs(NPCDb& db);

} // namespace hl1
