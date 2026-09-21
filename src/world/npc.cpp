#include "npc.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace hl1 {

bool NPCDb::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::fprintf(stderr, "[NPC] missing: %s\n", path.c_str()); return false; }
    nlohmann::json j;
    try { f >> j; } catch (const std::exception& e) {
        std::fprintf(stderr, "[NPC] parse error: %s\n", e.what()); return false;
    }
    for (const auto& n : j.value("npcs", nlohmann::json::array())) {
        NPCDef def;
        def.id = n.value("id", "");
        def.name = n.value("name", def.id);
        def.role = n.value("role", "scientist");
        def.appearance = n.value("appearance", "");
        def.affiliation = n.value("affiliation", "black_mesa");
        def.current_world = n.value("world", "black_mesa");
        def.x = n.value("x", 0.0);
        def.y = n.value("y", 0.0);
        if (n.contains("schedules")) {
            for (const auto& s : n["schedules"]) {
                NPCSchedule sched;
                sched.location = s.value("location", "");
                for (auto& [h, text] : s.value("lines", nlohmann::json::object()).items())
                    sched.lines[std::stoi(h)] = text.get<std::string>();
                def.schedules.push_back(sched);
            }
        }
        m_npcs.push_back(std::move(def));
    }
    std::printf("[NPC] %zu loaded\n", m_npcs.size());
    return true;
}

const NPCDef* NPCDb::find(const std::string& id) const {
    for (const auto& n : m_npcs) if (n.id == id) return &n;
    return nullptr;
}

std::vector<const NPCDef*> NPCDb::in_world(const std::string& world) const {
    std::vector<const NPCDef*> out;
    for (const auto& n : m_npcs)
        if (n.current_world == world || n.current_world == "any") out.push_back(&n);
    return out;
}

const NPCDef* NPCDb::at_location(const std::string& world, float x, float y) const {
    for (const auto& n : m_npcs)
        if (n.current_world == world && n.x == x && n.y == y) return &n;
    return nullptr;
}

// ---- Hardcoded HL1 NPC roster -------------------------------------------

void register_hl1_npcs(NPCDb& db) {
    NPCDef barney;
    barney.id = "barney"; barney.name = "Barney Calhoun"; barney.role = "guard";
    barney.appearance = "security guard, glasses, blue uniform";
    barney.affiliation = "black_mesa";
    barney.current_world = "black_mesa";
    {
        NPCSchedule sched;
        sched.location = "entrance";
        sched.lines = {
            {0, "About that beer I owed ya, it's on the counter over there!"},
            {8, "Head crabs! I hate these things!"},
            {12, "Here, take this crowbar. Might need it."},
            {18, "You need to get to Lambda, Gordon."},
        };
        barney.schedules.push_back(sched);
    }
    db.npcs().push_back(barney);

    NPCDef gman;
    gman.id = "gman"; gman.name = "The G-Man"; gman.role = "employer";
    gman.appearance = "blue suit, briefcase, pale skin, unsettling smile";
    gman.affiliation = "employer";
    gman.current_world = "any";
    {
        NPCSchedule sched;
        sched.location = "liminal";
        sched.lines = {
            {2, "Rise and shine, Mister Freeman. Rise... and... shine."},
            {6, "The right man in the wrong place can make all the difference in the world."},
            {14, "I have received offers... a rather lot of them."},
            {22, "Most of the surviving scientists have agreed to follow my plan."},
        };
        gman.schedules.push_back(sched);
    }
    db.npcs().push_back(gman);

    NPCDef scientist;
    scientist.id = "scientist"; scientist.name = "Dr. Kleiner"; scientist.role = "scientist";
    scientist.appearance = "lab coat, bald, glasses";
    scientist.affiliation = "black_mesa";
    scientist.current_world = "black_mesa";
    {
        NPCSchedule sched;
        sched.location = "test_lab";
        sched.lines = {
            {3, "Gordon! Thank God you're here. We need to evacuate."},
            {9, "The specimen was... it wasn't supposed to react like this."},
            {15, "I've seen the resonance cascade — it tears reality apart."},
            {20, "Lambda team is our last hope. Get to them."},
        };
        scientist.schedules.push_back(sched);
    }
    db.npcs().push_back(scientist);

    NPCDef vort;
    vort.id = "vortigaunt"; vort.name = "Vortigaunt"; vort.role = "ally";
    vort.appearance = "mantis-like alien, red eye, shackled hands";
    vort.affiliation = "xen";
    vort.current_world = "xen";
    {
        NPCSchedule sched;
        sched.location = "borderworld";
        sched.lines = {
            {4, "We are the Vortigaunts. We were the Nihilanth's slaves."},
            {10, "The Nihilanth freed us from our enslavement — or so it claimed."},
            {16, "You killed the Nihilanth. You freed us."},
            {21, "In our language, 'freeman' means 'free man'."},
        };
        vort.schedules.push_back(sched);
    }
    db.npcs().push_back(vort);

    NPCDef h_ecu;
    h_ecu.id = "hecu"; h_ecu.name = "HECU Marine"; h_ecu.role = "enemy";
    h_ecu.appearance = "camo armor, gas mask, pulse rifle";
    h_ecu.affiliation = "HECU";
    h_ecu.current_world = "surface_tension";
    {
        NPCSchedule sched;
        sched.location = "rooftops";
        sched.lines = {
            {7, "All units, secure the area. No survivors."},
            {13, "Target acquired. Engaging hostile."},
            {19, "The science team must not leave the facility."},
        };
        h_ecu.schedules.push_back(sched);
    }
    db.npcs().push_back(h_ecu);

    std::printf("[NPC] %zu HL1 roster registered\n", db.npcs().size());
}

} // namespace hl1
